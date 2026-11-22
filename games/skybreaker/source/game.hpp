// SKYBREAKER – gemeinsame Typen und Zustand
#pragma once
#include <cstdio>
#include "forge/forge.hpp"
#include "forge/ui.hpp"
#include "forge/gfx3d.hpp"
#include "gfx_ids.hpp"
#include "parts_gen.hpp"
using namespace forge;

// ------------------------------------------------------------------ Figuren-Steckbriefe
struct CharInfo {
	const char *name, *title, *element, *text;
	u32 color, color2;
};
extern const CharInfo CHAR_INFO[CH_COUNT];

// ------------------------------------------------------------------ Assets
struct Assets {
	SpriteSheet uiSheet, bgSheet;
	Image ui[UI_COUNT];
	Image bg[BG_COUNT];
	Mesh world[WM_COUNT];
	Mesh slash, ring, disc, shard;
	Sound theme;
	Sound click, select, whoosh, chime, error, swing, swing3, hit, hitHeavy, dash, cast, spin, step;
	Sound motif[CH_COUNT];
};
extern Assets A;

struct CharModel {
	Mesh part[PK_COUNT];
	Vec3 pivot[PK_COUNT];
	bool has[PK_COUNT];
};
extern CharModel CM[CH_COUNT];

void loadAssets();

// Pose (nur Seren hat einen beweglichen Schwertarm)
struct Pose {
	float armYaw = 0, armLift = 0;    // Drehung um die Schulter
	float weaponBlend = 0;            // 0 = Ruhehaltung, 1 = Klinge in Armrichtung
	float weaponRoll = 0;
	float bodyTwist = 0;
	float capeLift = 0;               // Umhang weht bei schneller Bewegung
	float legSwing = 0, armSwingL = 0; // Laufzyklus (Beine, linker Arm)
	float sink = 0;                   // Schatten-Dash: im Boden versinken (Meter)
	int weapon = 0;                   // 0 Schattenklinge, 1 Schattendolche, 2 Mondsense
};
extern const Vec3 SEREN_SHOULDER, SEREN_HAND;
void drawChar(int ch, const Mat4 &xf, float t, const Pose *pose = nullptr, float flash = 0, bool weapon = true);
void drawGhost(int ch, const Mat4 &xf, const Pose *pose, Color c);
Mat4 serenWeaponXf(const Mat4 &xf, const Pose &p); // Welt-Matrix der Klinge (für Effekte)

// ------------------------------------------------------------------ Einstellungen
struct Settings {
	u8 music = 1, sfx = 1, numbers = 1, shake = 1;
};
extern Settings S;
void saveSettings();
void sfx(const Sound &s, float pitch = 1, float vol = 1);

// ------------------------------------------------------------------ 3D-Partikel (2D-Sprites über der Szene)
struct Fx {
	struct P { Vec3 p, v; float life, max, s0, s1, grav, drag; Color c0, c1; int img; };
	static constexpr int MAX = 320;
	P p[MAX];
	int next = 0;
	Rng rng{77};
	void emit(Vec3 at, int n, int img, Color c0, Color c1, float speed, float life, float s0, float s1, float grav = 0, Vec3 dir = Vec3(), float spread = 1);
	void update(float dt);
	void draw(Gfx &g, const Camera3D &cam, float scale = 1) const;
	void clear() { for (auto &q : p) q.life = 0; }
};

// ------------------------------------------------------------------ Szenen
enum SceneId { SC_TITLE, SC_RUN, SC_TREE, SC_OPTIONS, SC_TRAINING, SC_HUB };
void gotoScene(int id);
extern float g_time;

void titleEnter();
void titleUpdate(float dt, const Input &in);
void titleDraw(Gfx &g);
void optionsEnter();
void optionsUpdate(float dt, const Input &in);
void optionsDraw(Gfx &g);
void trainingEnter();
void trainingUpdate(float dt, const Input &in);
void trainingDraw(Gfx &g);

// Hilfen
inline Color colA(u32 hex, float a) { return Color(hex, a); }
void drawButton(Gfx &g, int id, float x, float y, int img, const char *label, bool focus, float press, Color text = Color(0xFFFFFF));
void drawTextGlow(Gfx &g, float x, float y, float size, Color c, const char *s, int align, float depth = 0);
