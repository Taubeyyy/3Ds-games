// FORGE Hallo-Demo: Sprites mit Stereo-Tiefe, Text, Touch-Partikel, Speichern
#include "forge/forge.hpp"
#include "sprites_t3x.h"
using namespace forge;

struct Spark { Vec2 p, v; float life; int img; };

class Hello : public App {
	SpriteSheet sheet;
	Image orbB, orbP, glow;
	Spark sparks[256];
	int nextSpark = 0;
	Rng rng{7};
	u32 taps = 0;
public:
	void init() override {
		sheet.loadMem(FORGE_SHEET(sprites));
		orbB = sheet.get(0); orbP = sheet.get(1); glow = sheet.get(2);
		save::read("hello", &taps, sizeof taps);
	}
	void burst(Vec2 p, int n) {
		for (int i = 0; i < n; i++) {
			Spark &s = sparks[nextSpark++ % 256];
			s.p = p; s.v = Vec2::fromAngle(rng.range(0, 2 * PI)) * rng.range(40, 160); s.life = 1; s.img = rng.irange(0, 1);
		}
	}
	void update(float dt, const Input &in) override {
		if (in.tapped) { taps++; burst(in.touch, 24); }
		if (in.touching && in.dragging) burst(in.touch, 2);
		if (in.pressed(KEY_START)) quit = true;
		for (auto &s : sparks) if (s.life > 0) { s.life -= dt; s.p += s.v * dt; s.v *= 0.96f; s.v.y += 120 * dt; }
	}
	void suspend() override { save::write("hello", &taps, sizeof taps, 1); }
	void draw(Gfx &g) override {
		if (g.screen == Screen::Top) {
			g.rectGrad(0, 0, TOP_W, SCR_H, Color(0x101848), Color(0x3A1450), 8);
			for (int i = 0; i < 12; i++) {
				float d = 6 - i;                        // unterschiedliche Tiefen fuer Stereo-3D
				float x = 200 + cosf(time * 0.8f + i * 0.52f) * (60 + i * 8);
				float y = 120 + sinf(time * 1.1f + i * 0.52f) * (30 + i * 4);
				g.image(glow, x, y, 1.4f, 1.4f, 0, Color(i % 2 ? 0x5AC8FF : 0xFF6AD5, 0.5f), 1, d);
				g.image(i % 2 ? orbB : orbP, x, y, 0.8f + i * 0.04f, 0.8f + i * 0.04f, 0, Color(0xFFFFFF), 0, d);
			}
			g.text(200, 12, 1.0f, Color(0xFFFFFF), "FORGE ENGINE", ALIGN_CENTER, 0, true, -3);
			g.textf(200, 206, 0.45f, Color(0xA8B8FF), ALIGN_CENTER, "3D-Regler: Tiefe  ·  Zeit %.1f s", time);
		} else {
			g.rectGrad(0, 0, BOT_W, SCR_H, Color(0x161C44), Color(0x0A0E22));
			g.rrect(20, 20, 280, 50, 12, Color(0x4AD8FF, 0.9f), Color(0x2A6AD8, 0.9f));
			g.text(160, 32, 0.6f, Color(0xFFFFFF), "Tippe und wische!", ALIGN_CENTER);
			g.textf(160, 90, 0.5f, Color(0xFFD25A), ALIGN_CENTER, "Taps (gespeichert): %u", (unsigned)taps);
			for (auto &s : sparks) if (s.life > 0) g.image(s.img ? orbP : orbB, s.p.x, s.p.y, 0.4f * s.life, 0.4f * s.life, 0, Color(0xFFFFFF, s.life));
		}
	}
};

int main() {
	static Hello app; // gross -> statisch, nicht auf den Stack
	Config cfg;
	cfg.name = "forge_hello";
	return run(app, cfg);
}
