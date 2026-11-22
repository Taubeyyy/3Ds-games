// SKYBREAKER - Hauptprogramm: Titelbild, Hauptmenü, Pantheon (Figuren-Galerie), Optionen
#include <initializer_list>
#include "meta.hpp"

float g_time = 0;
static int g_scene = SC_TITLE, g_next = -1;
static float g_fade = 1;          // 1 = schwarz
static float g_sceneT = 0;

void gotoScene(int id) { if (g_next < 0) g_next = id; }

// ================================================================== Titelbild
struct Mote { float x, y, d, sp, ph, s; };
static Mote g_motes[48];
static int g_phase;               // 0 = "Drücke A", 1 = Menü
static float g_menuT, g_pressT;
static int g_focus;
static Camera3D g_titleCam;

static void initMotes() {
	Rng r(5);
	for (Mote &m : g_motes) m = {r.range(0, 400), r.range(0, 240), r.range(-5, 9), r.range(4, 14), r.range(0, 6.28f), r.range(0.08f, 0.2f)};
}

void titleEnter() {
	g_sceneT = 0;
	if (g_phase == 0) g_pressT = 0;
	initMotes();
	if (S.music) audio::music(A.theme, 0);
}

void titleUpdate(float dt, const Input &in) {
	g_menuT = g_phase ? fminf(1, g_menuT + dt * 2.5f) : 0;
	if (S.music) audio::musicVolume(fminf(1, g_sceneT / 2.5f) * 0.75f);
	for (Mote &m : g_motes) {
		m.y -= m.sp * dt;
		m.x += sinf(g_time * 0.5f + m.ph) * 4 * dt;
		if (m.y < -10) { m.y = 250; m.x = fmodf(m.x + 137, 400); }
	}
	if (g_phase == 0) {
		if (g_sceneT > 0.8f && (in.pressed(KEY_A | KEY_START) || in.tapped)) {
			g_phase = 1;
			g_menuT = 0;
			g_focus = 0;
			sfx(A.chime, 1, 0.8f);
			sfx(A.whoosh, 0.8f, 0.5f);
		}
		return;
	}
	// Menü: Steuerkreuz + A, oder Touch
	if (in.pressed(KEY_DUP)) { g_focus = (g_focus + 3) % 4; sfx(A.select); }
	if (in.pressed(KEY_DDOWN)) { g_focus = (g_focus + 1) % 4; sfx(A.select); }
	int act = -1;
	for (int i = 0; i < 4; i++) if (ui::clicked(10 + i)) { act = i; g_focus = i; }
	if (in.pressed(KEY_A)) act = g_focus;
	if (in.pressed(KEY_B)) { g_phase = 0; sfx(A.click, 0.8f); }
	switch (act) {
	case 0: sfx(A.chime, 1.2f); gotoScene(SC_RUN); break;
	case 1: sfx(A.click); gotoScene(SC_TREE); break;
	case 2: sfx(A.click); gotoScene(SC_TRAINING); break;
	case 3: sfx(A.click); gotoScene(SC_OPTIONS); break;
	}
}

// Oberer Bildschirm: Dungeon-Halle mit Himmelstor, Seren, Logo
void titleDrawTop(Gfx &g, float logoA) {
	float t = g_time;
	g.rectGrad(0, 0, 400, 240, Color(0x040308), Color(0x0E0A16), 10.5f);
	Camera3D &cam = g_titleCam;
	cam.target = Vec3(-0.9f + sinf(t * 0.17f) * 0.1f, 1.85f, -1.4f);
	cam.eye = Vec3(2.6f + sinf(t * 0.21f) * 0.2f, 1.2f + sinf(t * 0.27f) * 0.07f, 4.3f);
	cam.fov = 45;
	cam.focus = 4.4f;
	cam.zfar = 60;
	const Vec3 gate(-1.1f, 0.075f, -4.35f), b1(-3.0f, 0.07f, -3.4f), b2(0.9f, 0.07f, -3.8f), hero(0.55f, 0.075f, 0.9f);
	Scene3D sc;
	sc.ambient = Color(0x342E42);
	Light3D key; key.pos = Vec3(0.4f, 0.8f, 0.7f); key.color = Color(0x9890C0); key.intensity = 0.4f;
	sc.lights[sc.nLights++] = key;
	Light3D gl; gl.point = true; gl.pos = gate + Vec3(0, 1.6f, 0.6f); gl.color = Color(0x7ADCFF); gl.intensity = 1.6f + 0.25f * sinf(t * 1.3f); gl.range = 8;
	sc.lights[sc.nLights++] = gl;
	for (Vec3 b : {b1, b2}) { Light3D f; f.point = true; f.pos = b + Vec3(0, 1.4f, 0); f.color = Color(0xFF9A4A); f.intensity = 1.3f + 0.15f * sinf(t * 11 + b.x); f.range = 4.5f; sc.lights[sc.nLights++] = f; }
	Light3D star; star.point = true; star.pos = hero + Vec3(0.1f, 1.25f, 0.2f); star.color = Color(0x8AF0FF); star.intensity = 0.7f + 0.2f * sinf(t * 2.4f); star.range = 1.8f;
	sc.lights[sc.nLights++] = star;
	sc.fog = true;
	sc.fogColor = Color(0x07050C);
	sc.fogNear = 7; sc.fogFar = 20;
	gfx3d::begin(g, cam, sc);
	Material lit;
	lit.specular = 0.2f;
	Material glow;
	glow.lit = false;
	gfx3d::draw(A.world[WM_ARENA_DARK], Mat4::identity(), lit);
	Material ag = glow; ag.color = Color(0xFFFFFF).mix(Color(0x8AF0FF), 0.5f + 0.5f * sinf(t * 1.5f));
	gfx3d::draw(A.world[WM_ARENA_GLOW], Mat4::identity(), ag);
	gfx3d::draw(A.world[WM_DWALL], Mat4::identity(), lit);
	float fl = 1 + 0.07f * sinf(t * 14) + 0.04f * sinf(t * 31);
	gfx3d::draw(A.world[WM_DWALL_FIRE], Mat4::translate(Vec3(0, 2.18f, 0)) * Mat4::scale(Vec3(1, fl, 1)) * Mat4::translate(Vec3(0, -2.18f, 0)), glow);
	Mat4 gm = Mat4::translate(gate) * Mat4::scale(Vec3(1.45f, 1.45f, 1.45f));
	gfx3d::draw(A.world[WM_R_DOOR], gm, lit);
	Material pm = glow; pm.color = Color(0x8AE8FF, 0.75f + 0.15f * sinf(t * 1.3f));
	gfx3d::draw(A.world[WM_R_PORTAL], gm, pm);
	for (Vec3 b : {b1, b2}) {
		gfx3d::draw(A.world[WM_BRAZIER], Mat4::translate(b), lit);
		gfx3d::draw(A.world[WM_BRAZIER_FIRE], Mat4::translate(b + Vec3(0, 1.1f, 0)) * Mat4::scale(Vec3(1, fl, 1)) * Mat4::translate(Vec3(0, -1.1f, 0)), glow);
	}
	Material sg = glow; sg.color = Color(0xB0F4FF);
	for (int i = 0; i < 5; i++) {                                   // schwebende Himmelssplitter vor dem Tor
		float a = t * 0.5f + i * 1.25f;
		Vec3 p = gate + Vec3(cosf(a) * 1.3f, 1.8f + sinf(a * 1.7f) * 0.5f + i * 0.15f, 0.7f + sinf(a) * 0.4f);
		gfx3d::draw(A.shard, Mat4::translate(p) * Mat4::rotY(a * 2) * Mat4::scale(Vec3(0.9f, 1.6f, 0.9f)), sg);
	}
	Pose pose;
	pose.capeLift = 0.1f + 0.06f * sinf(t * 0.9f);
	Mat4 hx = Mat4::translate(hero) * Mat4::rotY(-0.45f);
	drawChar(CH_SEREN, hx, t, &pose);
	gfx3d::end(g);
	Vec2 s;
	float dep;
	if (gfx3d::project(g, cam, gate + Vec3(0, 1.7f, 0.3f), &s, &dep)) g.image(A.ui[UI_P_GLOW], s.x, s.y, 3.2f, 3.6f, 0, Color(0x7ADCFF, 0.3f + 0.08f * sinf(t * 1.3f)), 1, dep);
	for (Vec3 b : {b1, b2})
		if (gfx3d::project(g, cam, b + Vec3(0, 1.3f, 0), &s, &dep)) g.image(A.ui[UI_P_GLOW], s.x, s.y, 1.3f, 1.3f, 0, Color(0xFF9A4A, 0.5f + 0.1f * sinf(t * 11)), 1, dep);
	if (gfx3d::project(g, cam, hx.point(Vec3(0.07f, 1.2f, 0.16f)), &s, &dep)) g.image(A.ui[UI_P_GLOW], s.x, s.y, 0.55f + 0.1f * sinf(t * 2.4f), 0.55f, 0, Color(0x8AF0FF, 0.7f), 1, dep);
	for (const Mote &m : g_motes) {                                 // Staub und Glut in der Luft
		float a = 0.25f + 0.3f * sinf(t * 2 + m.ph);
		g.image(A.ui[UI_P_EMBER], m.x, m.y, m.s * 5, m.s * 5, 0, Color(m.d > 3 ? 0xFFB070 : 0x9AE8FF, a), 1, m.d);
	}
	// Logo
	if (logoA > 0) {
		float k = 0.92f + 0.08f * fminf(1, logoA);
		const Image &lg = A.ui[UI_LOGO];
		g.image(A.ui[UI_P_GLOW], 200, 52, 7, 2.2f, 0, Color(0x3A70C0, 0.3f * logoA), 1, -1.5f);
		g.image(lg, 200, 50, k, k, 0, Color(0xFFFFFF, logoA), 0, -2.5f);
		g.image(A.ui[UI_SUBTITLE], 200, 50 + lg.height() * 0.5f + 10, 1, 1, 0, Color(0xFFFFFF, logoA * 0.95f), 0, -1.5f);
	}
}

void titleDraw(Gfx &g) {
	if (g.screen == Screen::Top) {
		titleDrawTop(g, fminf(1, (g_sceneT - 0.3f) * 1.2f));
		ui::drawToasts(g, 186);
		return;
	}
	g.imageAt(A.bg[BG_BOTTOM], 0, 0);
	if (g_phase == 0) {
		float a = fminf(1, g_sceneT * 1.5f);
		g.image(A.ui[UI_SELECT], 160, 100, 1.5f, 1.5f, g_time * 0.4f, Color(0xFFE0A0, 0.6f * a), 0);
		g.image(A.ui[UI_P_GLOW], 160, 100, 2.6f, 2.6f, 0, Color(0x8AF0FF, 0.25f * a), 1);
		g.image(A.ui[UI_SIGIL_SEREN], 160, 100, 1.2f, 1.2f, 0, Color(0xFFFFFF, a), 0);
		float p = 0.55f + 0.45f * sinf(g_time * 3);
		g.text(160, 166, 0.55f, Color(0xFFF0D0, a * p), "Berühre den Bildschirm", ALIGN_CENTER);
		g.text(160, 186, 0.4f, Color(0xB8B0D8, a * p), "oder drücke A", ALIGN_CENTER);
		g.text(160, 222, 0.32f, Color(0x7A74A0), "v0.1 Konzept-Vorschau  ·  3DS Sammlung", ALIGN_CENTER);
		return;
	}
	float e = 1 - powf(1 - g_menuT, 3);
	g.image(A.ui[UI_SUBTITLE], 160, 16, 0.9f, 0.9f, 0, Color(0xFFFFFF, e));
	static const char *LBL[4] = {"Aufbruch", "Sternenkarte", "Training", "Optionen"};
	static const float Y[4] = {34, 92, 136, 180};
	for (int i = 0; i < 4; i++) {
		float k = clampf(e * 1.6f - i * 0.15f, 0, 1);
		float y = Y[i] + (1 - k) * 60;
		int img = i == 0 ? UI_BTN_MAIN : UI_BTN;
		Color tc = Color(i == 0 ? 0xFFF0C8 : 0xFFFFFF, k);
		drawButton(g, 10 + i, 44, y, img, i == 0 ? "" : LBL[i], g_focus == i, 0, tc);
		if (i == 0) {
			g.text(160, y + 5, 0.58f, tc, LBL[0], ALIGN_CENTER);
			g.text(160, y + 27, 0.32f, Color(0xE8C060, k * 0.9f), "Neuer Abstieg in die Tiefe", ALIGN_CENTER);
		}
		if (i == 1 && PROF.splitter) { char b[16]; snprintf(b, sizeof b, "%u", unsigned(PROF.splitter)); g.image(A.ui[UI_ICON_GEM], 238, y + 18, 0.36f, 0.36f, 0, Color(0xFFFFFF, k)); g.text(248, y + 11, 0.34f, Color(0x8AF0FF, k), b, ALIGN_LEFT); }
	}
	g.text(160, 224, 0.3f, Color(0x6A6490), "Steuerkreuz + A  ·  B: zurück", ALIGN_CENTER);
}

// ================================================================== Optionen
void optionsEnter() { g_sceneT = 0; }
void optionsUpdate(float dt, const Input &in) {
	(void)dt;
	bool ch = false;
	if (ui::clicked(51)) { S.music = !S.music; ch = true; if (S.music) audio::music(A.theme, 0.75f); else audio::stopMusic(); }
	if (ui::clicked(52)) { S.sfx = !S.sfx; ch = true; }
	if (ui::clicked(53)) { S.numbers = !S.numbers; ch = true; }
	if (ui::clicked(54)) { S.shake = !S.shake; ch = true; }
	if (ch) { sfx(A.click); saveSettings(); }
	if (ui::clicked(59) || in.pressed(KEY_B)) { sfx(A.click, 0.8f); gotoScene(SC_TITLE); }
}
void optionsDraw(Gfx &g) {
	if (g.screen == Screen::Top) { titleDrawTop(g, 1); return; }
	g.imageAt(A.bg[BG_BOTTOM], 0, 0);
	g.text(160, 8, 0.5f, Color(0xE8C060), "OPTIONEN", ALIGN_CENTER);
	ui::toggle(g, 51, 10, 34, "Musik", S.music);
	ui::toggle(g, 52, 10, 68, "Soundeffekte", S.sfx);
	ui::toggle(g, 53, 10, 102, "Schadenszahlen", S.numbers);
	ui::toggle(g, 54, 10, 136, "Bildschirmwackeln", S.shake);
	drawButton(g, 59, 44, 186, UI_BTN, "Zurück", false, 0);
}

// ================================================================== App
class Skybreaker : public App {
public:
	void init() override {
		loadAssets();
		loadProfile();
		ui::theme().accent = Color(0xE8C060);
		titleEnter();
	}
	void update(float dt, const Input &in) override {
		g_time += dt;
		g_sceneT += dt;
		ui::process(in, dt);
		ui::updateToasts(dt);
		if (g_next >= 0) {
			g_fade = fminf(1, g_fade + dt * 4);
			if (g_fade >= 1) {
				g_scene = g_next;
				g_next = -1;
				switch (g_scene) {
				case SC_TITLE: titleEnter(); break;
				case SC_RUN: runEnter(); break;
				case SC_TREE: treeEnter(); break;
				case SC_OPTIONS: optionsEnter(); break;
				case SC_TRAINING: trainingEnter(); break;
				}
			}
			return;
		}
		g_fade = fmaxf(0, g_fade - dt * 3);
		switch (g_scene) {
		case SC_TITLE: titleUpdate(dt, in); break;
		case SC_RUN: runUpdate(dt, in); break;
		case SC_TREE: treeUpdate(dt, in); break;
		case SC_OPTIONS: optionsUpdate(dt, in); break;
		case SC_TRAINING: trainingUpdate(dt, in); break;
		}
	}
	void draw(Gfx &g) override {
		ui::beginFrame(g);
		switch (g_scene) {
		case SC_TITLE: titleDraw(g); break;
		case SC_RUN: runDraw(g); break;
		case SC_TREE: treeDraw(g); break;
		case SC_OPTIONS: optionsDraw(g); break;
		case SC_TRAINING: trainingDraw(g); break;
		}
		if (g.screen == Screen::Bottom && g_scene != SC_TITLE && g_scene != SC_RUN && g_scene != SC_TRAINING) ui::drawToasts(g, 200);
		if (g_fade > 0) g.rect(0, 0, g.width, g.height, Color(0x000000, g_fade), -4);
	}
};

int main() {
	static Skybreaker app;
	Config cfg;
	cfg.name = "skybreaker";
	return run(app, cfg);
}
