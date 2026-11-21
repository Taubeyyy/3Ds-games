// FORGE UI-Demo: Szenen, Knöpfe, Schalter, Scroll-Liste, Toasts, Synth-Sounds
#include "forge/forge.hpp"
#include "forge/ui.hpp"
#include "sprites_t3x.h"
#include <cstdio>
using namespace forge;

static SpriteSheet sheet;
static Image orb, orbP, glow;
static SceneStack scenes;
static Sound sClick, sCoin, sPower, sErr;
static bool optSound = true, opt3d = true, optFast = false;

static void background(Gfx &g, float t) {
	if (g.screen == Screen::Top) {
		g.rectGrad(0, 0, TOP_W, SCR_H, Color(0x14205A), Color(0x3A1450), 8);
		for (int i = 0; i < 10; i++) {
			float x = fmodf(i * 61 + t * (12 + i * 3), TOP_W + 40) - 20, y = 40 + fmodf(i * 37.0f, 160) + sinf(t + i) * 8;
			g.image(glow, x, y, 1.6f, 1.6f, 0, Color(i % 2 ? 0x5AC8FF : 0xFF6AD5, 0.35f), 1, 6 - i * 0.5f);
		}
	} else g.rectGrad(0, 0, BOT_W, SCR_H, Color(0x1A2058), Color(0x0A0E22));
}

class ListScene : public Scene {
	ui::Scroll sc;
public:
	void update(float dt, const Input &in) override {
		ui::scrollUpdate(sc, 50, in, dt);
		for (int i = 0; i < 30; i++)
			if (ui::clicked(100 + i)) { char b[48]; snprintf(b, 48, "Eintrag %d angetippt", i + 1); ui::toast("Liste", b, Color(0x4AD8FF)); audio::play(sClick); }
		if (ui::clicked(1) || in.pressed(KEY_B)) { audio::play(sClick, 0.8f); scenes.pop(Transition::SlideRight); }
	}
	void draw(Gfx &g) override {
		extern App *gApp;
		background(g, gApp->time);
		if (g.screen == Screen::Top) { g.text(200, 100, 0.8f, Color(0xFFFFFF), "Scroll-Liste", ALIGN_CENTER, 0, true, -2); return; }
		ui::scrollArea(sc, 50, 0, 36, BOT_W, 204);
		sc.setContent(30 * 40 + 6);
		for (int i = 0; i < 30; i++) {
			float y = 40 + i * 40 - sc.pos;
			if (y < 0 || y > 240) continue;
			ui::panel(g, 8, y, 304, 36, Color(0x161C50), Color(i % 3 ? 0x3A4A90 : 0x4AD8FF));
			g.image(i % 2 ? orb : orbP, 28, y + 18, 0.7f, 0.7f);
			char b[48];
			snprintf(b, 48, "Eintrag %d", i + 1);
			g.text(48, y + 4, 0.45f, Color(0xFFFFFF), b);
			g.text(48, y + 20, 0.34f, Color(0xA8B0D8), "Antippen, wischen, Trägheit testen");
			ui::hit(100 + i, 8, fmaxf(y, 36), 304, 36);
		}
		g.rectGrad(0, 0, BOT_W, 36, Color(0x1A2058), Color(0x1A2058, 0.9f));
		ui::button(g, 1, 6, 6, 80, 24, "< Zurück", Color(0x4A5AA0));
		g.text(200, 8, 0.5f, Color(0xFFFFFF), "Liste", ALIGN_CENTER);
		ui::drawToasts(g, 190);
	}
};

class SettingsScene : public Scene {
public:
	void update(float, const Input &in) override {
		if (ui::clicked(10)) { optSound = !optSound; audio::setMasterVolume(optSound ? 1 : 0); audio::play(sClick); }
		if (ui::clicked(11)) { opt3d = !opt3d; audio::play(sClick); }
		if (ui::clicked(12)) { optFast = !optFast; audio::play(sClick); }
		if (ui::clicked(1) || in.pressed(KEY_B)) { audio::play(sClick, 0.8f); scenes.pop(Transition::SlideRight); }
	}
	void draw(Gfx &g) override {
		extern App *gApp;
		background(g, gApp->time);
		if (g.screen == Screen::Top) { g.text(200, 100, 0.8f, Color(0xFFFFFF), "Einstellungen", ALIGN_CENTER, 0, true, -2); return; }
		ui::button(g, 1, 6, 6, 80, 24, "< Zurück", Color(0x4A5AA0));
		ui::toggle(g, 10, 10, 50, "Ton", optSound);
		ui::toggle(g, 11, 10, 84, "3D-Effekt", opt3d);
		ui::toggle(g, 12, 10, 118, "Schneller Modus", optFast);
		ui::bar(g, 10, 166, 300, 12, 0.5f + 0.5f * sinf(gApp->time), Color(0x4AD8FF), Color(0x2A6AD8));
	}
};

static ListScene listScene;
static SettingsScene settingsScene;

class MenuScene : public Scene {
public:
	void update(float, const Input &) override {
		if (ui::clicked(1)) { audio::play(sPower); ui::toast("Los geht's!", "Hier würde das Spiel starten.", Color(0x5AE07A)); }
		if (ui::clicked(2)) { audio::play(sClick); scenes.push(&listScene, Transition::SlideLeft); }
		if (ui::clicked(3)) { audio::play(sClick); scenes.push(&settingsScene, Transition::SlideLeft); }
		if (ui::clicked(4)) { audio::play(sCoin); ui::toast("Münze!", "+10 Münzen", Color(0xFFD25A)); }
	}
	void draw(Gfx &g) override {
		extern App *gApp;
		float t = gApp->time;
		background(g, t);
		if (g.screen == Screen::Top) {
			g.text(200, 70 + sinf(t * 2) * 4, 1.3f, Color(0xFFFFFF), "FORGE UI", ALIGN_CENTER, 0, true, -4);
			g.text(200, 120, 0.45f, Color(0xA8C0FF), "Szenen  ·  Knöpfe  ·  Listen  ·  Toasts", ALIGN_CENTER, 0, true, -2);
			ui::drawToasts(g, 180);
			return;
		}
		ui::button(g, 1, 30, 20, 260, 46, "Spielen", Color(0x5AE07A), true, &orb, 0.62f);
		ui::button(g, 2, 30, 76, 125, 38, "Liste", Color(0x7A5AE0));
		ui::button(g, 3, 165, 76, 125, 38, "Optionen", Color(0x3A8AD0));
		ui::button(g, 4, 30, 124, 260, 34, "Münze sammeln", Color(0xFFD25A), true, &orbP);
		ui::button(g, 5, 30, 168, 260, 30, "Deaktiviert", Color(0xFF5A6E), false);
	}
};
static MenuScene menuScene;

App *gApp;
class UiDemo : public App {
public:
	void init() override {
		gApp = this;
		sheet.loadMem(FORGE_SHEET(sprites));
		orb = sheet.get(0); orbP = sheet.get(1); glow = sheet.get(2);
		sClick = synth::click(); sCoin = synth::coin(); sPower = synth::powerup(); sErr = synth::error();
		scenes.push(&menuScene, Transition::None);
	}
	void update(float dt, const Input &in) override {
		ui::process(in, dt);
		ui::updateToasts(dt);
		scenes.update(dt, in);
		if (in.pressed(KEY_START)) quit = true;
	}
	void draw(Gfx &g) override {
		ui::beginFrame(g);
		scenes.draw(g);
	}
};

int main() {
	static UiDemo app;
	Config cfg;
	cfg.name = "forge_ui";
	return run(app, cfg);
}
