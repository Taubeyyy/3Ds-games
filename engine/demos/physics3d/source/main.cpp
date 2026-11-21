// FORGE Physik-3D-Demo: Figur (kinematische Kapsel) läuft über Gelände, Treppe, Rampe, Aufzug,
// schiebt Kisten, Schockwelle (B) wirft alles um. Bullet darunter, gfx3d obendrauf.
#include <cstdio>
#include "forge/forge.hpp"
#include "forge/ui.hpp"
#include "forge/gfx3d.hpp"
#include "forge/phys3d.hpp"
#include "sprites_t3x.h"
#include "grass_t3x.h"
#include "crate_t3x.h"
#include "stone_t3x.h"
using namespace forge;
namespace p3 = forge::phys3d;

enum { ID_TERRAIN = 0, ID_STAIRS = 1, ID_RAMP = 2, ID_LEDGE = 3, ID_LIFT = 4, ID_CRATE = 100, ID_BALL = 300 };
constexpr int GN = 40;          // Gelände-Raster
constexpr float CELL = 1.0f;
constexpr int MAXC = 64, MAXB = 8;

static float smooth01(float t) { t = clampf(t, 0, 1); return t * t * (3 - 2 * t); }
static float terrainH(float x, float z) {
	float r = sqrtf(x * x + z * z);
	float hills = 1.3f * sinf(x * 0.27f) * cosf(z * 0.21f) + 0.55f * sinf(x * 0.63f + 1) * sinf(z * 0.52f);
	float flat = smooth01((r - 9) / 5);            // Mitte eben
	float rim = r > 15 ? (r - 15) * (r - 15) * 0.22f : 0; // Schüssel-Rand
	return hills * flat + rim + flat * 0.6f;
}

struct Thing { p3::Body *b; Vec3 half; };

class Physics3DDemo : public App {
	SpriteSheet sheet;
	Image glow;
	Texture3D tGrass, tCrate, tStone;
	Mesh terrain, unitCube, ball, ramp, body, head, disc;
	float hgt[(GN + 1) * (GN + 1)];
	u32 col[(GN + 1) * (GN + 1)];
	p3::World world;
	p3::Character hero;
	p3::Body *lift = nullptr;
	Mat4 rampXf;
	Thing crates[MAXC];
	int nCrates = 0;
	p3::Body *balls[MAXB];
	int nBalls = 0;
	struct Static { Vec3 c, half; } statics[16];
	int nStatics = 0;
	Camera3D cam;
	Scene3D scene;
	float camYaw = 0, liftY = 0.3f, heroYaw = 0, wave = -1, bob = 0;
	Vec3 waveAt;
	bool debug = false;
	int contactsShown = 0;
	Sound sClick, sThud, sJump, sBoom;

	void addStatic(Vec3 c, Vec3 half, int id) {
		world.box(c, half, p3::STATIC, id, 0, 0.8f);
		statics[nStatics++] = {c, half};
	}
	void addCrate(Vec3 c, float h) {
		if (nCrates >= MAXC) return;
		crates[nCrates] = {world.box(c, Vec3(h, h, h), p3::DYNAMIC, ID_CRATE + nCrates, h * h * h * 8 * 0.6f, 0.7f, 0.05f), Vec3(h, h, h)};
		nCrates++;
	}
	void build() {
		world.create(Vec3(0, -20, 0));
		nCrates = nBalls = nStatics = 0;
		world.mesh(terrain, Mat4::identity(), ID_TERRAIN);
		// Treppe zur Plattform
		for (int i = 0; i < 6; i++) { float top = (i + 1) * 0.25f; addStatic(Vec3(4, (top - 0.6f) * 0.5f, -2 - i * 0.6f), Vec3(1, (top + 0.6f) * 0.5f, 0.3f), ID_STAIRS); }
		addStatic(Vec3(4, 0.45f, -6.3f), Vec3(1.6f, 1.05f, 1.0f), ID_LEDGE);
		// Rampe (gedrehter Quader als statisches Netz)
		world.mesh(ramp, rampXf, ID_RAMP);
		addStatic(Vec3(-4, 0.14f, -6.9f), Vec3(1.2f, 1.34f, 1.4f), ID_LEDGE);
		// Schwebende Insel + Aufzug
		addStatic(Vec3(0, 3.3f, -12.5f), Vec3(3, 0.3f, 1.6f), ID_LEDGE);
		lift = world.box(Vec3(0, liftY, -9.6f), Vec3(1.1f, 0.15f, 1.1f), p3::KINEMATIC, ID_LIFT);
		// Kistenpyramide und Kistenhaufen
		for (int r = 0; r < 3; r++)
			for (int i = 0; i < 3 - r; i++) addCrate(Vec3(6 + (i - (2 - r) * 0.5f) * 0.82f, 0.42f + r * 0.81f, 4), 0.4f);
		for (int i = 0; i < 10; i++) addCrate(Vec3(-5 + (i % 3) * 0.55f, 0.9f + (i / 3) * 0.55f, 4 + (i % 2) * 0.2f), 0.25f);
		for (int i = 0; i < 4; i++) balls[nBalls++] = world.sphere(Vec3(-2 + i * 1.2f, 1.5f, 6.5f), 0.35f, p3::DYNAMIC, ID_BALL + i, 1.2f, 0.5f, 0.5f);
		hero = p3::Character();
		hero.teleport(Vec3(0, 0.7f, 3));
	}

public:
	void init() override {
		sheet.loadMem(FORGE_SHEET(sprites));
		glow = sheet.get(0);
		tGrass.loadMem(FORGE_SHEET(grass));
		tCrate.loadMem(FORGE_SHEET(crate));
		tStone.loadMem(FORGE_SHEET(stone));
		// Gelände mit Farben nach Höhe/Steigung
		for (int j = 0; j <= GN; j++)
			for (int i = 0; i <= GN; i++) {
				float x = (i - GN * 0.5f) * CELL, z = (j - GN * 0.5f) * CELL;
				float h = terrainH(x, z);
				hgt[j * (GN + 1) + i] = h;
				float sx = terrainH(x + 0.5f, z) - terrainH(x - 0.5f, z), sz = terrainH(x, z + 0.5f) - terrainH(x, z - 0.5f);
				float steep = clampf(sqrtf(sx * sx + sz * sz) * 1.4f, 0, 1);
				float n = 0.5f + 0.5f * sinf(x * 1.7f + z * 0.9f) * cosf(z * 1.3f - x * 0.4f);
				Color grass = Color(0x4E9A3A).mix(Color(0x86C24A), n * 0.7f + clampf(h * 0.15f, 0, 0.3f));
				Color c = grass.mix(Color(0x8A8290), smooth01((steep - 0.45f) * 3));
				col[j * (GN + 1) + i] = c.hex;
			}
		terrain = Mesh::grid(GN, GN, CELL, hgt, col, 10);
		unitCube = Mesh::cube(Vec3(1, 1, 1), Color(0xFFFFFF));
		ramp = Mesh::cube(Vec3(2.2f, 0.25f, 5.2f), Color(0xFFFFFF));
		rampXf = Mat4::translate(Vec3(-4, 0.74f, -3.3f)) * Mat4::rotX(0.3f);
		ball = Mesh::sphere(0.35f, 14, 10, Color(0xFFFFFF));
		body = Mesh::cylinder(0.33f, 1.0f, 12, Color(0x3A7AE8));
		head = Mesh::sphere(0.27f, 12, 8, Color(0xF0D0B0));
		disc = Mesh::cylinder(1, 0.02f, 24, Color(0xFFFFFF));
		sClick = synth::click();
		sThud = synth::hit();
		synth::Params jp; jp.wave = synth::SQUARE; jp.freq = 260; jp.freqEnd = 620; jp.dur = 0.13f; jp.decay = 14; jp.lowpass = 0.4f; jp.volume = 0.45f;
		sJump = synth::make(jp);
		sBoom = synth::explosion();
		build();
		cam.target = hero.pos + Vec3(0, 1, 0);
		cam.eye = cam.target + Vec3(0, 4.5f, 6.5f);
	}

	void update(float dt, const Input &in) override {
		ui::process(in, dt);
		ui::updateToasts(dt);
		if (in.pressed(KEY_START)) quit = true;
		if (ui::clicked(1)) { debug = !debug; audio::play(sClick); }
		if (ui::clicked(2)) { audio::play(sClick); world.destroy(); build(); ui::toast("Neu aufgebaut", "Welt zurückgesetzt", Color(0x5AE07A)); }
		if (ui::clicked(3)) {
			audio::play(sClick);
			for (int i = 0; i < 8; i++) addCrate(hero.pos + Vec3((i % 3 - 1) * 0.9f, 6 + i * 0.9f, (i / 3 - 1) * 0.9f), 0.3f);
		}
		// Aufzug (kinematisch) bewegen und Figur mitnehmen
		float ny = 1.8f - 1.5f * cosf(time * 0.7f);
		float dy = ny - liftY;
		liftY = ny;
		p3::setPos(lift, Vec3(0, liftY, -9.6f));
		if (hero.grounded && hero.groundUserId == ID_LIFT) hero.pos.y += dy;
		// Steuerung relativ zur Kamera
		Vec2 mv = in.stick;
		if (in.holding(KEY_DLEFT)) mv.x = -1;
		if (in.holding(KEY_DRIGHT)) mv.x = 1;
		if (in.holding(KEY_DUP)) mv.y = 1;
		if (in.holding(KEY_DDOWN)) mv.y = -1;
		float c = cosf(camYaw), s = sinf(camYaw);
		Vec3 fwd(-s, 0, -c), right(c, 0, -s);
		Vec3 wish = (right * mv.x + fwd * mv.y) * 6.0f;
		if (wish.len() > 0.5f) { heroYaw = atan2f(wish.x, wish.z); bob += dt * 12; }
		bool wasGrounded = hero.grounded;
		hero.update(world, wish, in.pressed(KEY_A), dt);
		if (!wasGrounded && hero.grounded) audio::play(sThud, 1.5f, 0.25f);
		if (hero.vel.y > 8.5f && wasGrounded) audio::play(sJump);
		if (hero.pos.y < -20) hero.teleport(Vec3(0, 2, 3));
		// Schockwelle
		if (in.pressed(KEY_B)) {
			waveAt = hero.pos + Vec3(0, 0.5f, 0);
			world.explode(waveAt, 5, 9);
			wave = 0;
			audio::play(sBoom, 1.2f, 0.6f);
		}
		if (wave >= 0) wave += dt;
		if (wave > 0.6f) wave = -1;
		world.step(dt);
		// Aufprall-Geräusche aus Kontakt-Ereignissen
		const p3::Contact *cs;
		int nc = world.contacts(&cs);
		contactsShown = nc;
		int sounds = 0;
		for (int i = 0; i < nc && sounds < 2; i++)
			if (cs[i].begin && cs[i].impulse > 1.5f && (cs[i].a >= ID_CRATE || cs[i].b >= ID_CRATE)) {
				audio::play(sThud, 0.8f + 0.1f * (i % 4), clampf(cs[i].impulse / 12, 0.1f, 0.5f));
				sounds++;
			}
		// Kamera
		if (in.holding(KEY_L)) camYaw -= dt * 1.8f;
		if (in.holding(KEY_R)) camYaw += dt * 1.8f;
		if (in.dragging && in.touchStart.y > 196) camYaw -= in.touchDelta.x * 0.01f;
		Vec3 tgt = hero.pos + Vec3(0, 1.0f, 0);
		Vec3 eye = tgt + Vec3(sinf(camYaw) * 6.5f, 4.5f, cosf(camYaw) * 6.5f);
		float k = 1 - expf(-7 * dt);
		cam.eye = cam.eye + (eye - cam.eye) * k;
		cam.target = cam.target + (tgt - cam.target) * k;
		cam.fov = 55;
		cam.focus = (cam.eye - cam.target).len();
	}

	void drawScene(Gfx &g) {
		scene.ambient = Color(0x44506A);
		scene.nLights = 2;
		scene.lights[0] = Light3D();
		scene.lights[0].pos = Vec3(0.5f, 1, 0.35f);
		scene.lights[0].color = Color(0xFFF0D8);
		scene.lights[0].intensity = 0.9f;
		scene.lights[1] = Light3D();
		scene.lights[1].pos = Vec3(-0.6f, 0.3f, -0.6f);
		scene.lights[1].color = Color(0x6A90D0);
		scene.lights[1].intensity = 0.35f;
		scene.fog = true;
		scene.fogColor = Color(0xA8D0F0);
		scene.fogNear = 14; scene.fogFar = 40;
		gfx3d::begin(g, cam, scene);
		Material mt; mt.tex = &tGrass; mt.specular = 0.05f;
		gfx3d::draw(terrain, Mat4::identity(), mt);
		Material ms; ms.tex = &tStone; ms.specular = 0.15f;
		for (int i = 0; i < nStatics; i++)
			gfx3d::draw(unitCube, Mat4::translate(statics[i].c) * Mat4::scale(statics[i].half * 2), ms);
		gfx3d::draw(ramp, rampXf, ms);
		Material ml; ml.color = Color(0xFFC85A); ml.emission = Color(0x302008); ml.specular = 0.6f;
		gfx3d::draw(unitCube, Mat4::translate(p3::pos(lift)) * Mat4::scale(Vec3(2.2f, 0.3f, 2.2f)), ml);
		Material mc; mc.tex = &tCrate; mc.specular = 0.1f;
		for (int i = 0; i < nCrates; i++)
			gfx3d::draw(unitCube, p3::transform(crates[i].b) * Mat4::scale(crates[i].half * 2), mc);
		static const Color BC[4] = {Color(0xFF5A5A), Color(0x5AC8FF), Color(0xFFD25A), Color(0xB07AFF)};
		for (int i = 0; i < nBalls; i++) {
			Material mb; mb.color = BC[i % 4]; mb.specular = 0.9f; mb.shininess = 40;
			gfx3d::draw(ball, p3::transform(balls[i]), mb);
		}
		// Figur
		float b = hero.grounded && Vec3(hero.vel.x, 0, hero.vel.z).len() > 0.5f ? fabsf(sinf(bob)) * 0.07f : 0;
		Mat4 hm = Mat4::translate(hero.pos + Vec3(0, b, 0)) * Mat4::rotY(heroYaw);
		Material mh; mh.specular = 0.4f;
		gfx3d::draw(body, hm * Mat4::translate(Vec3(0, 0.05f, 0)), mh);
		gfx3d::draw(head, hm * Mat4::translate(Vec3(0, 1.32f, 0)), mh);
		Material me; me.color = Color(0x101010); me.lit = false;
		gfx3d::draw(unitCube, hm * Mat4::translate(Vec3(0.1f, 1.36f, 0.24f)) * Mat4::scale(Vec3(0.06f, 0.09f, 0.04f)), me);
		gfx3d::draw(unitCube, hm * Mat4::translate(Vec3(-0.1f, 1.36f, 0.24f)) * Mat4::scale(Vec3(0.06f, 0.09f, 0.04f)), me);
		// Blob-Schatten per Strahl nach unten
		p3::Hit h;
		if (world.raycast(hero.pos + Vec3(0, 0.5f, 0), hero.pos + Vec3(0, -6, 0), &h)) {
			float k = clampf(1 - (hero.pos.y - h.pos.y) / 5, 0.2f, 1);
			Material msh; msh.lit = false; msh.color = Color(0x000000, 0.35f * k);
			gfx3d::draw(disc, Mat4::translate(h.pos + Vec3(0, 0.03f, 0)) * Mat4::scale(Vec3(0.38f * k, 1, 0.38f * k)), msh);
		}
		// Schockwellen-Ring
		if (wave >= 0) {
			float r = 0.5f + wave * 8;
			Material mw; mw.lit = false; mw.color = Color(0xFFF0B0, 0.5f * (1 - wave / 0.6f));
			gfx3d::draw(disc, Mat4::translate(waveAt) * Mat4::scale(Vec3(r, 1, r)), mw);
		}
		gfx3d::end(g);
		if (debug) world.debugDraw(g, cam);
		Vec2 sp;
		float dep;
		if (wave >= 0 && gfx3d::project(g, cam, waveAt, &sp, &dep)) g.image(glow, sp.x, sp.y, 3 + wave * 10, 3 + wave * 10, 0, Color(0xFFF0B0, 0.6f * (1 - wave / 0.6f)), 1, dep);
	}

	void draw(Gfx &g) override {
		ui::beginFrame(g);
		if (g.screen == Screen::Top) {
			g.rectGrad(0, 0, g.width, g.height, Color(0x5A9AE0), Color(0xA8D0F0), 14);
			drawScene(g);
			g.rrect(6, 6, 150, 22, 11, Color(0x0A0E2A, 0.7f), Color(0x161C48, 0.7f), -2);
			g.text(16, 8, 0.45f, Color(0xFFFFFF), "FORGE Physik", ALIGN_LEFT, 0, true, -2);
			g.text(110, 10, 0.34f, hero.grounded ? Color(0x5AE07A) : Color(0xFFD25A), hero.grounded ? "Boden" : "Luft", ALIGN_LEFT, 0, true, -2);
			ui::drawToasts(g, 186);
		} else {
			g.rectGrad(0, 0, BOT_W, SCR_H, Color(0x1A3A58), Color(0x0A1622));
			g.text(BOT_W / 2, 8, 0.5f, Color(0xFFFFFF), "3D-Physik (Bullet)", ALIGN_CENTER);
			g.text(BOT_W / 2, 30, 0.34f, Color(0xA8C0D8), "Pad: laufen  ·  A: springen  ·  B: Schockwelle", ALIGN_CENTER);
			g.text(BOT_W / 2, 44, 0.34f, Color(0xA8C0D8), "L/R oder unten wischen: Kamera", ALIGN_CENTER);
			ui::toggle(g, 1, 10, 62, "Physik-Umrisse", debug);
			ui::button(g, 2, 10, 98, 145, 34, "Neu aufbauen", Color(0x5AE07A));
			ui::button(g, 3, 165, 98, 145, 34, "Kisten-Regen", Color(0xE8A040));
			ui::panel(g, 10, 142, 300, 46, Color(0x0A1622, 0.8f), Color(0x2A4A68));
			g.textf(20, 148, 0.36f, Color(0xD8E8F8), ALIGN_LEFT, "Körper: %d   Kisten: %d   Ereignisse: %d", world.bodyCount(), nCrates, contactsShown);
			g.textf(20, 166, 0.36f, Color(0x8AA8C8), ALIGN_LEFT, "Figur  x %.1f  y %.1f  z %.1f", hero.pos.x, hero.pos.y, hero.pos.z);
			g.rrect(10, 198, 300, 34, 10, Color(0x1A2A3A, 0.8f), Color(0x10202E, 0.8f));
			g.text(BOT_W / 2, 207, 0.36f, Color(0x6A88A8), "<   hier wischen zum Drehen   >", ALIGN_CENTER);
		}
	}
};

int main() {
	static Physics3DDemo app;
	Config cfg;
	cfg.name = "forge_physics3d";
	return run(app, cfg);
}
