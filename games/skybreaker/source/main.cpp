// SKYBREAKER - Hauptprogramm: Titelbild, Hauptmenü, Pantheon (Figuren-Galerie), Optionen
#include "game.hpp"

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
	case 0: sfx(A.chime, 1.2f); gotoScene(SC_TRAINING); break;
	case 1: sfx(A.click); gotoScene(SC_PANTHEON); break;
	case 2: sfx(A.error, 1, 0.6f); ui::toast("Das Lager", "Kommt mit dem nächsten Update", Color(0xE8C060)); break;
	case 3: sfx(A.click); gotoScene(SC_OPTIONS); break;
	}
}

// Oberer Bildschirm: Himmel, Turm, Insel, Seren, Logo
static const Vec3 SHARDS[] = {Vec3(-7, -1.5f, -9), Vec3(6.5f, 1.2f, -13), Vec3(-14, 3.5f, -26), Vec3(10, -3, -6), Vec3(-4, 5, -30), Vec3(16, 6, -40)};

void titleDrawTop(Gfx &g, float logoA) {
	float t = g_time;
	g.imageAt(A.bg[BG_SKY], 0, 0, 1, 1, Color(0xFFFFFF), 0, 10.5f);
	g.imageAt(A.bg[BG_CRACK], 0, 0, 1, 1, Color(0xFFFFFF, 0.25f + 0.25f * sinf(t * 1.3f)), 0, 10.5f);
	for (int i = 0; i < 3; i++) {                                   // ferne Wolken
		float x = fmodf(i * 170 + t * (4 + i * 2), 560) - 80;
		g.image(A.ui[UI_CLOUD0 + i], x, 168 + i * 14, 1.4f, 1.0f, 0, Color(0xFFD8E8, 0.45f), 0, 8 - i);
	}
	g.image(A.ui[UI_P_GLOW], 345, 205, 5, 3, 0, Color(0xFFB070, 0.35f), 1, 10);   // Sonnenglühen
	// --- 3D
	Camera3D &cam = g_titleCam;
	cam.target = Vec3(-1.4f, 2.0f, -1.0f);
	cam.eye = Vec3(2.55f + sinf(t * 0.21f) * 0.22f, 1.0f + sinf(t * 0.27f) * 0.08f, 5.0f);
	cam.fov = 45;
	cam.focus = 5.0f;
	cam.zfar = 400;
	Scene3D sc;
	sc.ambient = Color(0x3E3860);
	sc.nLights = 0;
	Light3D sun;
	sun.pos = Vec3(0.55f, 0.35f, -0.9f); sun.color = Color(0xFFB27A); sun.intensity = 1.05f;
	sc.lights[sc.nLights++] = sun;
	Light3D fill;
	fill.pos = Vec3(-0.6f, 0.55f, 0.75f); fill.color = Color(0x7080D8); fill.intensity = 0.6f;
	sc.lights[sc.nLights++] = fill;
	Light3D star;
	star.point = true; star.pos = Vec3(0.45f, 1.25f, 0.55f); star.color = Color(0x8AF0FF); star.intensity = 0.7f + 0.2f * sinf(t * 2.4f); star.range = 1.8f;
	sc.lights[sc.nLights++] = star;
	sc.fog = true;
	sc.fogColor = Color(0x8A5A98);
	sc.fogNear = 45; sc.fogFar = 260;
	gfx3d::begin(g, cam, sc);
	Material lit;
	lit.specular = 0.1f;
	Material glow;
	glow.lit = false;
	Mat4 tower = Mat4::translate(Vec3(-120, -42, -150)) * Mat4::scale(Vec3(1.7f, 1.7f, 1.7f));
	gfx3d::draw(A.world[WM_TOWER], tower, lit);
	gfx3d::draw(A.world[WM_TOWER_GLOW], tower, glow);
	gfx3d::draw(A.world[WM_TOWER_CRYSTALS], tower * Mat4::rotY(t * 0.15f), glow);
	for (int i = 0; i < 6; i++) {
		Vec3 p = SHARDS[i] + Vec3(0, sinf(t * 0.6f + i) * 0.25f, 0);
		gfx3d::draw(A.world[WM_SHARD_A + i % 3], Mat4::translate(p) * Mat4::rotY(t * 0.05f * (i % 2 ? 1 : -1) + i), lit);
	}
	gfx3d::draw(A.world[WM_ISLAND], Mat4::identity(), lit);
	Pose pose;
	pose.capeLift = 0.12f + 0.08f * sinf(t * 0.9f);
	drawChar(CH_SEREN, Mat4::translate(Vec3(0.3f, 0.02f, 0.35f)) * Mat4::rotY(-0.55f), t, &pose);
	gfx3d::end(g);
	// Leuchten über 3D
	Vec2 s;
	float dep;
	if (gfx3d::project(g, cam, (Mat4::translate(Vec3(0.3f, 0.02f, 0.35f)) * Mat4::rotY(-0.55f)).point(Vec3(0.07f, 1.2f, 0.16f)), &s, &dep)) g.image(A.ui[UI_P_GLOW], s.x, s.y, 0.55f + 0.1f * sinf(t * 2.4f), 0.55f, 0, Color(0x8AF0FF, 0.7f), 1, dep);
	Vec3 crystal = tower.point(Vec3(0, 37.8f + 6.5f, 0));
	if (gfx3d::project(g, cam, crystal, &s, &dep)) g.image(A.ui[UI_P_GLOW], s.x, s.y, 1.6f, 1.6f, 0, Color(0x9AF4FF, 0.5f + 0.2f * sinf(t * 1.7f)), 1, dep);
	for (const Mote &m : g_motes) {                                 // schwebende Lichtfunken
		float a = 0.35f + 0.35f * sinf(t * 2 + m.ph);
		g.image(A.ui[UI_P_EMBER], m.x, m.y, m.s * 6, m.s * 6, 0, Color(m.d > 3 ? 0xFFD8A0 : 0x9AF0FF, a), 1, m.d);
	}
	for (int i = 0; i < 2; i++) {                                   // nahe Wolken
		float x = fmodf(i * 260 + t * 14, 700) - 150;
		g.image(A.ui[UI_CLOUD1 + i], x, 228 - i * 6, 2.2f, 1.2f, 0, Color(0xFFE0F0, 0.28f), 0, -2);
	}
	// Logo
	if (logoA > 0) {
		float k = 0.92f + 0.08f * fminf(1, logoA);
		const Image &lg = A.ui[UI_LOGO];
		g.image(A.ui[UI_P_GLOW], 200, 52, 7, 2.2f, 0, Color(0x5AA0FF, 0.25f * logoA), 1, -1.5f);
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
	static const char *LBL[4] = {"Aufbruch", "Pantheon", "Lager", "Optionen"};
	static const float Y[4] = {34, 92, 136, 180};
	for (int i = 0; i < 4; i++) {
		float k = clampf(e * 1.6f - i * 0.15f, 0, 1);
		float y = Y[i] + (1 - k) * 60;
		int img = i == 0 ? UI_BTN_MAIN : i == 2 ? UI_BTN_OFF : UI_BTN;
		Color tc = i == 2 ? Color(0x8A86A0, k) : Color(i == 0 ? 0xFFF0C8 : 0xFFFFFF, k);
		drawButton(g, 10 + i, 44, y, img, i == 0 ? "" : LBL[i], g_focus == i, 0, tc);
		if (i == 0) {
			g.text(160, y + 5, 0.58f, tc, LBL[0], ALIGN_CENTER);
			g.text(160, y + 27, 0.32f, Color(0xE8C060, k * 0.9f), "Trainingsplatz · Kampf testen", ALIGN_CENTER);
		}
		if (i == 2) { g.image(A.ui[UI_LOCK], 104, y + 18, 1, 1, 0, Color(0xFFFFFF, k)); g.text(228, y + 11, 0.32f, Color(0x8A86A0, k), "bald", ALIGN_CENTER); }
	}
	g.text(160, 224, 0.3f, Color(0x6A6490), "Steuerkreuz + A  ·  B: zurück", ALIGN_CENTER);
}

// ================================================================== Pantheon
static int g_sel, g_prevSel;
static float g_yaw, g_swap = 1, g_charH[CH_COUNT];
struct Pt { Vec2 p, v; float life, max, s, rot; };
static Pt g_pts[64];
static int g_ptN;
static Rng g_rng(9);

void pantheonEnter() {
	g_sceneT = 0;
	g_swap = 1;
	for (int i = 0; i < CH_COUNT; i++) {
		const Mesh &b = CM[i].part[PK_BODY];
		float my = 0;
		for (int k = 0; k < b.nv; k++) my = fmaxf(my, b.v[k].pos[1]);
		g_charH[i] = my;
	}
	for (Pt &p : g_pts) p.life = 0;
}

static void selectChar(int i) {
	if (i == g_sel) return;
	g_prevSel = g_sel;
	g_sel = (i + CH_COUNT) % CH_COUNT;
	g_swap = 0;
	g_yaw = 0;
	sfx(A.whoosh, 1.2f, 0.4f);
	sfx(A.select, 0.8f + g_sel * 0.05f);
}

static void spawnPt() {
	Pt &p = g_pts[g_ptN];
	g_ptN = (g_ptN + 1) % 64;
	int el = g_sel;
	p.p = Vec2(g_rng.range(110, 290), el == CH_SYLV ? -10.f : g_rng.range(150, 250));
	p.v = el == CH_SYLV ? Vec2(g_rng.range(-10, 10), g_rng.range(18, 30)) : Vec2(g_rng.range(-8, 8), -g_rng.range(14, 40));
	p.max = p.life = g_rng.range(2, 4);
	p.s = g_rng.range(0.5f, 1.1f);
	p.rot = g_rng.range(0, 6);
}

void pantheonUpdate(float dt, const Input &in) {
	g_swap = fminf(1, g_swap + dt * 3);
	if (in.pressed(KEY_R) || in.pressed(KEY_DRIGHT)) selectChar(g_sel + 1);
	if (in.pressed(KEY_L) || in.pressed(KEY_DLEFT)) selectChar(g_sel - 1);
	if (in.pressed(KEY_DDOWN)) selectChar(g_sel + 4);
	if (in.pressed(KEY_DUP)) selectChar(g_sel - 4);
	for (int i = 0; i < CH_COUNT; i++) if (ui::clicked(30 + i)) selectChar(i);
	if (ui::clicked(29) || in.pressed(KEY_B)) { sfx(A.click, 0.8f); gotoScene(SC_TITLE); }
	if (in.dragging && in.touchStart.y > 138) g_yaw += in.touchDelta.x * 0.012f;
	g_yaw += in.stick.x * dt * 2.5f;
	if (!in.touching) g_yaw += dt * 0.35f;
	static float acc;
	acc += dt;
	while (acc > 0.12f) { acc -= 0.12f; spawnPt(); }
	for (Pt &p : g_pts) {
		if (p.life <= 0) continue;
		p.life -= dt;
		p.p += p.v * dt;
		p.p.x += sinf(g_time * 2 + p.rot) * 10 * dt;
		p.rot += dt;
	}
}

static int ptImg(int ch) {
	switch (ch) {
	case CH_IGNARA: case CH_KORRIN: return UI_P_EMBER;
	case CH_THALOR: return UI_P_SPARK;
	case CH_NERYS: return UI_P_BUBBLE;
	case CH_VEXA: return UI_P_WISP;
	case CH_SYLV: return UI_P_LEAF;
	case CH_LYRA: return UI_P_SPARK;
	default: return UI_P_DUST;
	}
}

void pantheonDraw(Gfx &g) {
	const CharInfo &ci = CHAR_INFO[g_sel];
	Color col(ci.color);
	if (g.screen == Screen::Top) {
		g.imageAt(A.bg[BG_PANTHEON], 0, 0, 1, 1, Color(0xFFFFFF), 0, 9);
		g.image(A.ui[UI_P_GLOW], 230, 130, 5.5f, 4.5f, 0, col.alpha(0.32f), 1, 6);
		g.image(A.ui[UI_P_GLOW], 230, 215, 4.5f, 1.0f, 0, col.alpha(0.4f), 1, 2);
		float h = g_charH[g_sel];
		float e = 1 - powf(1 - g_swap, 3);
		Camera3D cam;
		cam.target = Vec3(-0.55f - h * 0.12f, 0.32f + h * 0.52f, 0);
		float dist = 1.2f + h * 1.32f;
		cam.eye = cam.target + Vec3(0, 0.05f + h * 0.08f, dist);
		cam.fov = 45;
		cam.focus = dist;
		Scene3D sc;
		sc.ambient = Color(0x2E2A44);
		sc.nLights = 0;
		Light3D key; key.pos = Vec3(0.6f, 0.7f, 0.8f); key.color = Color(0xFFF2E0); key.intensity = 0.85f;
		Light3D rim; rim.pos = Vec3(-0.4f, 0.45f, -1); rim.color = col; rim.intensity = 1.3f;
		Light3D low; low.point = true; low.pos = Vec3(0, 1.0f, 1.2f); low.color = col; low.intensity = 0.45f; low.range = 2.6f;
		sc.lights[sc.nLights++] = key;
		sc.lights[sc.nLights++] = rim;
		sc.lights[sc.nLights++] = low;
		gfx3d::begin(g, cam, sc);
		Material lit;
		lit.specular = 0.15f;
		Mat4 ped = Mat4::rotY(g_yaw * 0.2f);
		gfx3d::draw(A.world[WM_PEDESTAL], ped, lit);
		Material rg;
		rg.lit = false;
		rg.color = col.lighten(0.3f);
		gfx3d::draw(A.world[WM_PEDESTAL_GLOW], ped, rg);
		float k = 0.6f + 0.4f * e;
		Mat4 xf = Mat4::translate(Vec3(0, 0.32f - (1 - e) * 0.3f, 0)) * Mat4::rotY(g_yaw) * Mat4::scale(Vec3(k, k, k));
		drawChar(g_sel, xf, g_time, nullptr, (1 - e) * 0.8f);
		gfx3d::end(g);
		for (const Pt &p : g_pts) {
			if (p.life <= 0) continue;
			float a = fminf(1, p.life) * fminf(1, (p.max - p.life) * 2);
			Color pc = g_sel == CH_VEXA ? Color(0x6A3AA0, a * 0.6f) : col.lighten(0.3f).alpha(a * 0.8f);
			g.image(A.ui[ptImg(g_sel)], p.p.x, p.p.y, p.s, p.s, p.rot, pc, 1, -1 + p.s * 3);
		}
		// Name und Titel
		const Image &nm = A.ui[UI_NAME_SEREN + g_sel];
		float slide = (1 - e) * -30;
		g.image(A.ui[UI_P_GLOW], 20 + nm.w * 0.5f + slide, 34, nm.w / 40.0f, 1.2f, 0, col.alpha(0.25f), 1, -1);
		g.imageAt(nm, 14 + slide, 12, 1, 1, Color(0xFFFFFF, e), 0, -2);
		g.text(16 + slide, 12 + nm.height() + 2, 0.45f, col.lighten(0.35f).alpha(e), ci.title, ALIGN_LEFT, 0, true, -1.5f);
		g.text(16 + slide, 12 + nm.height() + 20, 0.34f, Color(0xA8A4C8, e), ci.element, ALIGN_LEFT, 0, true, -1.5f);
		g.text(392, 222, 0.32f, Color(0x8A86B0), "L / R  wechseln  ·  Circle Pad drehen", ALIGN_RIGHT, 0, true, -1);
		return;
	}
	g.imageAt(A.bg[BG_BOTTOM], 0, 0);
	g.text(160, 6, 0.46f, Color(0xE8C060), "PANTHEON", ALIGN_CENTER);
	drawButton(g, 29, 6, 4, UI_BTN_SMALL, "", false, 0);
	g.text(38, 9, 0.38f, Color(0xFFFFFF), "< Zurück", ALIGN_CENTER);
	for (int i = 0; i < CH_COUNT; i++) {
		float x = 55 + (i % 4) * 70, y = 56 + (i / 4) * 56;
		bool s = i == g_sel;
		float k = s ? 0.78f + 0.03f * sinf(g_time * 4) : 0.66f;
		if (ui::held(30 + i)) k *= 0.92f;
		if (s) {
			g.image(A.ui[UI_P_GLOW], x, y, 1.6f, 1.6f, 0, Color(CHAR_INFO[i].color, 0.45f), 1);
			g.image(A.ui[UI_SELECT], x, y, 0.82f, 0.82f, g_time * 0.6f, Color(0xFFFFFF, 0.9f));
		}
		g.image(A.ui[UI_SIGIL_SEREN + i], x, y, k, k, 0, Color(0xFFFFFF, s ? 1.f : 0.75f));
		ui::hit(30 + i, x - 30, y - 28, 60, 56);
	}
	g.imageAt(A.ui[UI_DESC], 8, 142);
	g.text(20, 150, 0.46f, col.lighten(0.3f), ci.name, ALIGN_LEFT);
	g.text(20 + g.textWidth(ci.name, 0.46f) + 8, 153, 0.36f, Color(0xB8B0D8), ci.title, ALIGN_LEFT);
	g.text(20, 172, 0.36f, Color(0xE8E4F8), ci.text, ALIGN_LEFT, 286);
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
				case SC_PANTHEON: pantheonEnter(); break;
				case SC_OPTIONS: optionsEnter(); break;
				case SC_TRAINING: trainingEnter(); break;
				}
			}
			return;
		}
		g_fade = fmaxf(0, g_fade - dt * 3);
		switch (g_scene) {
		case SC_TITLE: titleUpdate(dt, in); break;
		case SC_PANTHEON: pantheonUpdate(dt, in); break;
		case SC_OPTIONS: optionsUpdate(dt, in); break;
		case SC_TRAINING: trainingUpdate(dt, in); break;
		}
	}
	void draw(Gfx &g) override {
		ui::beginFrame(g);
		switch (g_scene) {
		case SC_TITLE: titleDraw(g); break;
		case SC_PANTHEON: pantheonDraw(g); break;
		case SC_OPTIONS: optionsDraw(g); break;
		case SC_TRAINING: trainingDraw(g); break;
		}
		if (g.screen == Screen::Bottom && g_scene != SC_TITLE) ui::drawToasts(g, 200);
		if (g_fade > 0) g.rect(0, 0, g.width, g.height, Color(0x000000, g_fade), -4);
	}
};

int main() {
	static Skybreaker app;
	Config cfg;
	cfg.name = "skybreaker";
	return run(app, cfg);
}
