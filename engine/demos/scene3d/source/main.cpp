// FORGE 3D-Demo: Arena mit Texturen, Hardware-Licht (Punktlichter + Mond), Nebel, Stereo, 2D-HUD darüber
#include <cstdio>
#include "forge/forge.hpp"
#include "forge/ui.hpp"
#include "forge/gfx3d.hpp"
#include "sprites_t3x.h"
#include "floor_t3x.h"
#include "pillar_t3x.h"
using namespace forge;

constexpr int NPILLAR = 8, NORB = 3;
static const Color ORB_COL[NORB] = {Color(0xFF9A3A), Color(0x3AD8FF), Color(0xFF4AC8)};

class Scene3DDemo : public App {
	SpriteSheet sheet;
	Image glow;
	Texture3D texFloor, texPillar;
	Mesh floor, pillar, cap, crystal, body, head, orb, shard;
	Camera3D cam;
	Scene3D scene;
	Vec3 hero{0, 0, 4};
	float heroYaw = 0, camYaw = 0, bob = 0, moved = 0;
	bool fog = true, lights = true, textures = true, orbit = false;
	Vec3 orbPos[NORB];
	Sound sClick;
	int tris = 0;

public:
	void init() override {
		sheet.loadMem(FORGE_SHEET(sprites));
		glow = sheet.get(0);
		texFloor.loadMem(FORGE_SHEET(floor));
		texPillar.loadMem(FORGE_SHEET(pillar));
		floor = Mesh::plane(Vec2(26, 26), 16, Color(0xFFFFFF), 6);
		pillar = Mesh::cylinder(0.6f, 3.2f, 16, Color(0xFFFFFF));
		cap = Mesh::cube(Vec3(1.6f, 0.35f, 1.6f), Color(0xC8B8B0));
		crystal = Mesh::sphere(0.7f, 4, 2, Color(0x9AF0FF)); // grobe Kugel = Kristall
		body = Mesh::cylinder(0.32f, 0.8f, 12, Color(0xE85A3A));
		head = Mesh::sphere(0.26f, 12, 8, Color(0xF0D0B0));
		orb = Mesh::sphere(0.18f, 10, 6, Color(0xFFFFFF));
		shard = Mesh::cube(Vec3(0.25f, 0.25f, 0.25f), Color(0xFFFFFF));
		tris = (floor.ni + NPILLAR * (pillar.ni + cap.ni) + crystal.ni + body.ni + head.ni + NORB * orb.ni) / 3;
		sClick = synth::click();
		cam.target = hero + Vec3(0, 0.6f, 0);
		cam.eye = cam.target + Vec3(0, 7.0f, 7.5f);
	}

	void update(float dt, const Input &in) override {
		ui::process(in, dt);
		ui::updateToasts(dt);
		if (in.pressed(KEY_START)) quit = true;
		if (ui::clicked(1)) { fog = !fog; audio::play(sClick); }
		if (ui::clicked(2)) { lights = !lights; audio::play(sClick); }
		if (ui::clicked(3)) { textures = !textures; audio::play(sClick); }
		if (ui::clicked(4)) { orbit = !orbit; audio::play(sClick); }
		// Bewegung relativ zur Kamera
		Vec2 mv = in.stick;
		if (in.holding(KEY_DLEFT)) mv.x = -1;
		if (in.holding(KEY_DRIGHT)) mv.x = 1;
		if (in.holding(KEY_DUP)) mv.y = 1;
		if (in.holding(KEY_DDOWN)) mv.y = -1;
		float c = cosf(camYaw), s = sinf(camYaw);
		Vec3 fwd(-s, 0, -c), right(c, 0, -s);
		Vec3 d = right * mv.x + fwd * mv.y;
		float sp = d.len();
		if (sp > 0.1f) {
			hero += d * (5.0f * dt);
			heroYaw = atan2f(d.x, d.z);
			bob += dt * 12;
			moved += dt;
		}
		float r = hero.len();
		if (r > 11) hero = hero * (11 / r);
		// Säulen als Hindernisse (Kreis gegen Kreis)
		for (int i = 0; i < NPILLAR; i++) {
			float a = (i + 0.5f) * 2 * PI / NPILLAR;
			Vec3 pp(cosf(a) * 7.5f, 0, sinf(a) * 7.5f);
			Vec3 dv = hero - pp;
			float dl = dv.len();
			if (dl < 0.95f && dl > 1e-4f) hero = pp + dv * (0.95f / dl);
		}
		if (in.holding(KEY_L)) camYaw -= dt * 1.8f;
		if (in.holding(KEY_R)) camYaw += dt * 1.8f;
		if (in.dragging && in.touchStart.y < 160) camYaw -= in.touchDelta.x * 0.01f;
		if (orbit) camYaw += dt * 0.35f;
		for (int i = 0; i < NORB; i++) {
			float a = time * (0.6f + i * 0.25f) + i * 2.1f;
			orbPos[i] = Vec3(cosf(a) * (3.5f + i * 1.6f), 1.4f + sinf(time * 1.7f + i) * 0.5f, sinf(a) * (3.5f + i * 1.6f));
		}
		// Kamera: schräg von oben, folgt dem Helden weich
		Vec3 tgt = hero + Vec3(0, 0.6f, 0);
		Vec3 eye = tgt + Vec3(sinf(camYaw) * 7.5f, 7.0f, cosf(camYaw) * 7.5f);
		float k = 1 - expf(-6 * dt);
		cam.eye = cam.eye + (eye - cam.eye) * k;
		cam.target = cam.target + (tgt - cam.target) * k;
		cam.fov = 50;
		cam.focus = (cam.eye - cam.target).len();
	}

	void drawScene(Gfx &g) {
		scene.ambient = Color(0x2A2440);
		scene.nLights = 0;
		Light3D moon;
		moon.pos = Vec3(-0.4f, 1, 0.3f);
		moon.color = Color(0x8090D0);
		moon.intensity = 0.55f;
		scene.lights[scene.nLights++] = moon;
		if (lights)
			for (int i = 0; i < NORB; i++) {
				Light3D L;
				L.point = true;
				L.pos = orbPos[i];
				L.color = ORB_COL[i];
				L.intensity = 1.6f;
				L.range = 7;
				scene.lights[scene.nLights++] = L;
			}
		Light3D crys;
		crys.point = true; crys.pos = Vec3(0, 2.2f, 0); crys.color = Color(0x7AE8FF); crys.intensity = 1.0f + 0.3f * sinf(time * 3); crys.range = 6;
		scene.lights[scene.nLights++] = crys;
		scene.fog = fog;
		scene.fogColor = Color(0x1A1430);
		scene.fogNear = 8; scene.fogFar = 30;

		gfx3d::begin(g, cam, scene);
		Material m;
		m.tex = textures ? &texFloor : nullptr;
		m.specular = 0.15f;
		gfx3d::draw(floor, Mat4::identity(), m);
		for (int i = 0; i < NPILLAR; i++) {
			float a = (i + 0.5f) * 2 * PI / NPILLAR;
			Vec3 pp(cosf(a) * 7.5f, 0, sinf(a) * 7.5f);
			Material pm;
			pm.tex = textures ? &texPillar : nullptr;
			pm.specular = 0.2f;
			gfx3d::draw(pillar, Mat4::translate(pp), pm);
			Material cm; cm.color = Color(0xD8C8C0); cm.specular = 0.35f;
			gfx3d::draw(cap, Mat4::translate(pp + Vec3(0, 3.35f, 0)), cm);
		}
		// Kristall auf dem Altar
		Material km; km.color = Color(0x9AF0FF); km.emission = Color(0x1A5A70); km.specular = 1; km.shininess = 40;
		gfx3d::draw(crystal, Mat4::translate(Vec3(0, 2.2f + sinf(time * 2) * 0.15f, 0)) * Mat4::rotY(time * 0.8f) * Mat4::scale(Vec3(0.8f, 1.5f, 0.8f)), km);
		for (int i = 0; i < 6; i++) {
			float a = time * 1.2f + i * PI / 3;
			Material sm; sm.color = Color(0x7AE8FF); sm.emission = Color(0x206080);
			gfx3d::draw(shard, Mat4::translate(Vec3(cosf(a) * 1.3f, 2.2f + sinf(time * 3 + i) * 0.3f, sinf(a) * 1.3f)) * Mat4::rotY(a * 2) * Mat4::rotX(a), sm);
		}
		// Held
		float b = moved > 0 ? fabsf(sinf(bob)) * 0.08f : 0;
		Mat4 hm = Mat4::translate(hero + Vec3(0, b, 0)) * Mat4::rotY(heroYaw);
		Material hb; hb.color = Color(0xFFFFFF); hb.specular = 0.4f;
		gfx3d::draw(body, hm, hb);
		gfx3d::draw(head, hm * Mat4::translate(Vec3(0, 1.05f, 0)), hb);
		Material sw; sw.color = Color(0xFFE08A); sw.emission = Color(0x403010); sw.specular = 0.9f; sw.shininess = 30;
		gfx3d::draw(shard, hm * Mat4::translate(Vec3(0.42f, 0.55f, 0.1f)) * Mat4::scale(Vec3(0.3f, 3.2f, 0.3f)), sw);
		// Lichtkugeln (unbeleuchtet, leuchtend)
		if (lights)
			for (int i = 0; i < NORB; i++) {
				Material om; om.lit = false; om.color = ORB_COL[i].lighten(0.5f);
				gfx3d::draw(orb, Mat4::translate(orbPos[i]), om);
			}
		gfx3d::end(g);
		// Glow als 2D über den Lichtern – mit passender Stereotiefe
		Vec2 sp;
		float dep;
		if (lights)
			for (int i = 0; i < NORB; i++)
				if (gfx3d::project(g, cam, orbPos[i], &sp, &dep)) g.image(glow, sp.x, sp.y, 1.6f, 1.6f, 0, ORB_COL[i].alpha(0.55f), 1, dep);
		if (gfx3d::project(g, cam, Vec3(0, 2.2f, 0), &sp, &dep)) g.image(glow, sp.x, sp.y, 2.4f, 2.4f, 0, Color(0x7AE8FF, 0.35f), 1, dep);
		if (gfx3d::project(g, cam, hero + Vec3(0, 1.7f, 0), &sp, &dep)) {
			g.rrect(sp.x - 26, sp.y - 9, 52, 16, 8, Color(0x0A0E2A, 0.7f), Color(0x161C48, 0.7f), dep);
			g.text(sp.x, sp.y - 8, 0.38f, Color(0xFFE08A), "Held", ALIGN_CENTER, 0, true, dep);
		}
	}

	void draw(Gfx &g) override {
		ui::beginFrame(g);
		if (g.screen == Screen::Top) {
			g.rectGrad(0, 0, g.width, g.height, Color(0x0E0A20), Color(0x2A1A40), 12);
			drawScene(g);
			g.rrect(6, 6, 132, 22, 11, Color(0x0A0E2A, 0.75f), Color(0x161C48, 0.75f), -2);
			g.text(16, 8, 0.45f, Color(0xFFFFFF), "FORGE 3D", ALIGN_LEFT, 0, true, -2);
			g.textf(84, 10, 0.34f, Color(0x9AF0FF), ALIGN_LEFT, "%d Dreiecke", tris);
		} else {
			g.rectGrad(0, 0, BOT_W, SCR_H, Color(0x1A2058), Color(0x0A0E22));
			g.text(BOT_W / 2, 10, 0.5f, Color(0xFFFFFF), "3D-Arena", ALIGN_CENTER);
			g.text(BOT_W / 2, 32, 0.36f, Color(0xA8B0D8), "Circle Pad: laufen  ·  L/R oder Wischen: Kamera", ALIGN_CENTER);
			g.text(BOT_W / 2, 48, 0.36f, Color(0xA8B0D8), "3D-Regler: echtes Stereo-3D", ALIGN_CENTER);
			ui::toggle(g, 1, 10, 70, "Nebel", fog);
			ui::toggle(g, 2, 10, 102, "Punktlichter", lights);
			ui::toggle(g, 3, 10, 134, "Texturen", textures);
			ui::toggle(g, 4, 10, 166, "Kamera kreist", orbit);
			g.textf(BOT_W / 2, 214, 0.36f, Color(0x6A78B8), ALIGN_CENTER, "%d Lichter aktiv · Hardware-Fragmentlicht", lights ? NORB + 2 : 2);
		}
	}
};

int main() {
	static Scene3DDemo app;
	Config cfg;
	cfg.name = "forge_scene3d";
	return run(app, cfg);
}
