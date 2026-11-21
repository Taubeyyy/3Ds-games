// FORGE Plattformer-Demo: Box2D, Tilemap, Kamera, Partikel, UI unten
#include <cstdio>
#include "forge/forge.hpp"
#include "forge/ui.hpp"
#include "forge/world2d.hpp"
#include <box2d/box2d.h>
#include "sprites_t3x.h"
using namespace forge;

enum { IMG_GRASS, IMG_DIRT, IMG_STONE, IMG_CRATE, IMG_COIN, IMG_HERO, IMG_GLOW };
enum { ID_GROUND = 0, ID_HERO = 1, ID_CRATE = 100, ID_COIN = 1000 };
constexpr int MAPW = 120, MAPH = 24;
constexpr float TS = 16;

class Platformer : public App {
	SpriteSheet sheet;
	Image img[8];
	Image tileImgs[4];
	Tilemap map;
	phys::World world;
	Camera2D cam;
	Particles fx;
	Rng rng{3};
	b2Body *hero = nullptr;
	b2Body *crates[24] = {};
	int nCrates = 0;
	struct Coin { Vec2 p; bool alive; b2Body *b; } coins[40];
	int nCoins = 0, score = 0;
	bool grounded = false, wasGrounded = false, facingLeft = false;
	float coyote = 0, jumpBuf = 0;
	Sound sJump, sCoin, sLand, sClick;
	bool debug = false;

	void buildLevel() {
		map.create(MAPW, MAPH, TS);
		map.solid[1] = map.solid[2] = map.solid[3] = 1;
		map.images = tileImgs;
		int h = 16;
		for (int x = 0; x < MAPW; x++) {
			if (x > 8 && x % 9 == 0) h += rng.irange(-2, 2);
			h = h < 10 ? 10 : h > 20 ? 20 : h;
			bool gap = x > 15 && (x % 23 == 0 || x % 23 == 1);
			for (int y = 0; y < MAPH; y++) {
				if (gap) continue;
				if (y == h) map.set(x, y, 1);
				else if (y > h) map.set(x, y, y > h + 3 ? 3 : 2);
			}
			if (x % 17 == 8) for (int k = 0; k < 4; k++) map.set(x + k, h - 4, 3); // Plattformen
			if (x % 11 == 5 && nCoins < 40) coins[nCoins++] = {Vec2((x + 0.5f) * TS, (h - 1.2f) * TS), true, nullptr};
			if (x % 13 == 6 && nCrates < 24) crates[nCrates++] = (b2Body *)(intptr_t)(x * 1000 + h); // Platzhalter, unten ersetzt
		}
		for (int x = 0; x < MAPW; x++) { map.set(x, MAPH - 1, 3); }
	}
	void resetWorld() {
		world.destroy();
		world.create(Vec2(0, 900));
		nCrates = nCoins = 0;
		score = 0;
		rng = Rng(3);
		buildLevel();
		world.addTilemap(map, ID_GROUND);
		for (int i = 0; i < nCrates; i++) {
			int code = int((intptr_t)crates[i]);
			int x = code / 1000, h = code % 1000;
			crates[i] = world.box(Vec2((x + 0.5f) * TS, (h - 3) * TS), Vec2(TS, TS), phys::DYNAMIC, ID_CRATE + i, 0.6f, 0.5f);
		}
		for (int i = 0; i < nCoins; i++) coins[i].b = world.circle(coins[i].p, 5, phys::STATIC, ID_COIN + i, 0, 0, true);
		hero = world.box(Vec2(3 * TS, 5 * TS), Vec2(12, 18), phys::DYNAMIC, ID_HERO, 1, 0.0f, false, true);
		cam.pos = phys::pos(hero);
		cam.boundsMin = Vec2(200, 120);
		cam.boundsMax = Vec2(MAPW * TS - 200, MAPH * TS - 120);
	}
public:
	void init() override {
		sheet.loadMem(FORGE_SHEET(sprites));
		for (int i = 0; i < 7; i++) img[i] = sheet.get(i);
		tileImgs[1] = img[IMG_GRASS]; tileImgs[2] = img[IMG_DIRT]; tileImgs[3] = img[IMG_STONE];
		synth::Params p; p.wave = synth::SQUARE; p.freq = 300; p.freqEnd = 700; p.dur = 0.14f; p.decay = 14; p.lowpass = 0.4f; p.volume = 0.5f;
		sJump = synth::make(p);
		sCoin = synth::coin(); sLand = synth::hit(); sClick = synth::click();
		resetWorld();
	}
	void update(float dt, const Input &in) override {
		ui::process(in, dt);
		ui::updateToasts(dt);
		if (ui::clicked(1)) { audio::play(sClick); resetWorld(); ui::toast("Neustart", "Level neu aufgebaut", Color(0x4AD8FF)); }
		if (ui::clicked(2)) { audio::play(sClick); debug = !debug; }
		if (in.pressed(KEY_START)) quit = true;
		// Steuerung
		float dir = in.stick.x;
		if (in.holding(KEY_DLEFT)) dir = -1;
		if (in.holding(KEY_DRIGHT)) dir = 1;
		if (dir < -0.1f) facingLeft = true;
		if (dir > 0.1f) facingLeft = false;
		Vec2 v = phys::vel(hero);
		float target = dir * 130;
		v.x = approach(v.x, target, grounded ? 14 : 6, dt);
		Vec2 hp = phys::pos(hero);
		Vec2 hit;
		wasGrounded = grounded;
		grounded = world.raycast(hp + Vec2(-5, 0), hp + Vec2(-5, 11), &hit, nullptr) || world.raycast(hp + Vec2(5, 0), hp + Vec2(5, 11), &hit, nullptr);
		coyote = grounded ? 0.1f : coyote - dt;
		jumpBuf = in.pressed(KEY_A) || in.pressed(KEY_B) ? 0.12f : jumpBuf - dt;
		if (jumpBuf > 0 && coyote > 0) {
			v.y = -330;
			jumpBuf = coyote = 0;
			audio::play(sJump, rng.range(0.95f, 1.05f), 0.6f);
			EmitDesc d; d.img = &img[IMG_GLOW]; d.count = 8; d.angle = PI / 2; d.spread = 2.4f; d.speedMin = 20; d.speedMax = 60; d.size0 = 0.25f; d.c0 = Color(0xE8D8B0, 0.8f); d.c1 = Color(0xE8D8B0, 0); d.gravity = -20;
			fx.emit(hp + Vec2(0, 9), d);
		}
		if (!(in.holding(KEY_A) || in.holding(KEY_B)) && v.y < -120) v.y = approach(v.y, -120, 20, dt); // variable Sprunghoehe
		phys::setVel(hero, v);
		if (grounded && !wasGrounded) {
			audio::play(sLand, 1.4f, 0.25f);
			cam.addShake(0.15f);
			EmitDesc d; d.img = &img[IMG_GLOW]; d.count = 10; d.angle = -PI / 2; d.spread = 2.8f; d.speedMin = 30; d.speedMax = 70; d.size0 = 0.3f; d.c0 = Color(0xE8D8B0, 0.9f); d.c1 = Color(0xE8D8B0, 0);
			fx.emit(hp + Vec2(0, 9), d);
		}
		world.step(dt);
		const phys::Contact *cs;
		int nc = world.contacts(&cs);
		for (int i = 0; i < nc; i++) {
			int a = cs[i].a, b = cs[i].b;
			if (!cs[i].begin) continue;
			int coin = a == ID_HERO && b >= ID_COIN ? b - ID_COIN : (b == ID_HERO && a >= ID_COIN ? a - ID_COIN : -1);
			if (coin >= 0 && coins[coin].alive) {
				coins[coin].alive = false;
				score++;
				audio::play(sCoin);
				EmitDesc d; d.img = &img[IMG_GLOW]; d.count = 16; d.speedMin = 40; d.speedMax = 140; d.size0 = 0.4f; d.c0 = Color(0xFFD25A); d.c1 = Color(0xFF8A2A, 0);
				fx.emit(coins[coin].p, d);
			}
		}
		for (int i = 0; i < nCoins; i++) if (!coins[i].alive && coins[i].b) { world.remove(coins[i].b); coins[i].b = nullptr; }
		if (hp.y > MAPH * TS + 40) resetWorld();
		fx.update(dt);
		cam.follow(phys::pos(hero) + Vec2(facingLeft ? -30 : 30, -10), dt, 5);
		cam.update(dt, rng);
	}
	void drawWorld(Gfx &g, Camera2D &c) {
		g.rectGrad(0, 0, g.width, g.height, Color(0x6AB8FF), Color(0xD8F0FF), 10);
		for (int layer = 0; layer < 2; layer++) { // Parallax-Huegel
			float par = layer ? 0.5f : 0.25f;
			Color col = layer ? Color(0x6AC060) : Color(0x9AD8A0);
			for (int i = -1; i < 8; i++) {
				float x = fmodf(i * 90 - c.pos.x * par, 720) + (i * 90 - c.pos.x * par < 0 ? 720 : 0) - 90;
				g.circle(x, g.height - 20 + layer * 10, 80 - layer * 15, col, 8 - layer * 3);
			}
		}
		map.draw(g, c, 2);
		for (int i = 0; i < nCoins; i++) {
			if (!coins[i].alive) continue;
			Vec2 s = c.toScreen(g, coins[i].p + Vec2(0, sinf(time * 4 + i) * 2));
			g.image(img[IMG_GLOW], s.x, s.y, 0.6f, 0.6f, 0, Color(0xFFD25A, 0.4f), 1, 0);
			g.image(img[IMG_COIN], s.x, s.y, c.zoom * fabsf(cosf(time * 3 + i)) + 0.05f, c.zoom, 0, Color(0xFFFFFF), 0, 0);
		}
		for (int i = 0; i < nCrates; i++) {
			Vec2 s = c.toScreen(g, phys::pos(crates[i]));
			g.image(img[IMG_CRATE], s.x, s.y, c.zoom, c.zoom, phys::angle(crates[i]), Color(0xFFFFFF), 0, 0.5f);
		}
		Vec2 hs = c.toScreen(g, phys::pos(hero));
		float squash = grounded ? 1 : 0.92f;
		g.image(img[IMG_HERO], hs.x, hs.y, (facingLeft ? -1.f : 1.f) * c.zoom / squash, c.zoom * squash, 0, Color(0xFFFFFF), 0, -0.5f);
		fx.draw(g, &c, -1);
		if (debug) world.debugDraw(g, c);
	}
	void draw(Gfx &g) override {
		ui::beginFrame(g);
		if (g.screen == Screen::Top) {
			drawWorld(g, cam);
			g.rrect(6, 6, 96, 22, 11, Color(0x0A0E2A, 0.75f), Color(0x161C48, 0.75f), -2);
			g.image(img[IMG_COIN], 20, 17, 1, 1, 0, Color(0xFFFFFF), 0, -2);
			g.textf(32, 7, 0.5f, Color(0xFFD25A), ALIGN_LEFT, "%d / %d", score, nCoins);
			ui::drawToasts(g, 186);
		} else {
			Camera2D mini = cam;
			mini.zoom = 0.45f;
			g.rectGrad(0, 0, BOT_W, SCR_H, Color(0x1A2058), Color(0x0A0E22));
			drawWorld(g, mini);
			g.rect(0, 168, BOT_W, 72, Color(0x0A0E22, 0.85f));
			g.text(10, 174, 0.38f, Color(0xA8B0D8), "Steuerkreuz/Circle Pad: laufen  ·  A: springen");
			ui::button(g, 1, 10, 196, 145, 34, "Neustart", Color(0x5AE07A));
			ui::button(g, 2, 165, 196, 145, 34, debug ? "Debug: an" : "Debug: aus", Color(0x4A5AB0));
		}
	}
};

int main() {
	static Platformer app;
	Config cfg;
	cfg.name = "forge_platformer";
	return run(app, cfg);
}
