// SKYBREAKER - Assets, Figuren-Darstellung, Partikel, Einstellungen
#include <cstring>
#include "game.hpp"
#include "ui_t3x.h"
#include "bg_t3x.h"
#include "theme_wav.h"
#include "motif_ignara_wav.h"
#include "motif_thalor_wav.h"
#include "motif_nerys_wav.h"
#include "motif_vexa_wav.h"
#include "motif_korrin_wav.h"
#include "motif_lyra_wav.h"
#include "motif_sylv_wav.h"

Assets A;
CharModel CM[CH_COUNT];
Settings S;
const Vec3 SEREN_SHOULDER(0.2f, 1.32f, 0.0f), SEREN_HAND(0.275f, 0.81f, 0.11f);
static const Vec3 HIP_L(-0.1f, 0.84f, 0), HIP_R(0.1f, 0.84f, 0), SHOULDER_L(-0.2f, 1.32f, 0);

const CharInfo CHAR_INFO[CH_COUNT] = {
	{"Seren", "Die Sternenschmiedin", "Sternstahl",
	 "Schmiedet Klingen aus gefallenen Sternen. Als der Himmel zerbrach, fiel ein Splitter in ihre Esse - und begann zu sprechen.",
	 0x8AF0FF, 0x26366A},
	{"Ignara", "Göttin des Feuers", "Feuer",
	 "Stolz, ungeduldig, warmherzig. Ihre Segen setzen Feinde in Brand und lassen dein Ausweichen explodieren.",
	 0xFF6E32, 0x9A1E24},
	{"Thalor", "Gott des Sturms", "Sturm",
	 "Laut, eitel und schneller als jeder Gedanke. Seine Segen springen als Kettenblitz von Feind zu Feind.",
	 0x6EB4FF, 0xFFF07A},
	{"Nerys", "Göttin der Gezeiten", "Gezeiten",
	 "Geduldig wie das Meer, unaufhaltsam wie die Flut. Ihre Wellen stoßen Feinde zurück und bremsen sie.",
	 0x3CDCD2, 0x123C78},
	{"Vexa", "Göttin der Schatten", "Schatten",
	 "Spricht in Rätseln und lacht zu spät. Ihre Segen schenken kritische Treffer und kurze Unsichtbarkeit.",
	 0xAA64FF, 0x1A1228},
	{"Korrin", "Gott der Erde und Schmiede", "Erde",
	 "Serens alter Lehrmeister, brummig und treu. Rüstung, Schockwellen und Gegenschläge.",
	 0xEB9646, 0x6A4028},
	{"Lyra", "Göttin des Sternenlichts", "Sternenlicht",
	 "Hüterin der Sterne, sanft und unerschütterlich. Heilung, Lichtstrahlen und Schilde.",
	 0xFFE28C, 0xF4F2FF},
	{"Sylv", "Hüter der Wildnis", "Wildnis",
	 "Wortkarg, folgt nur seinem Geistwolf. Gift, Ranken und ein Tierbegleiter an deiner Seite.",
	 0x82E15A, 0x3E8A3A},
};

// ------------------------------------------------------------------ Laden
static Vec3 topPivot(const Mesh &m) {
	float maxY = -1e9f;
	for (int i = 0; i < m.nv; i++) maxY = fmaxf(maxY, m.v[i].pos[1]);
	Vec3 acc;
	int n = 0;
	for (int i = 0; i < m.nv; i++)
		if (m.v[i].pos[1] > maxY - 0.04f) { acc += Vec3(m.v[i].pos[0], m.v[i].pos[1], m.v[i].pos[2]); n++; }
	return n ? acc * (1.0f / n) : Vec3(0, maxY, 0);
}
static Vec3 centroid(const Mesh &m) {
	Vec3 acc;
	for (int i = 0; i < m.nv; i++) acc += Vec3(m.v[i].pos[0], m.v[i].pos[1], m.v[i].pos[2]);
	return m.nv ? acc * (1.0f / m.nv) : Vec3();
}

static Mesh slashMesh() {
	// Sichel: flacher Ring-Ausschnitt in der XZ-Ebene, innen transparent, außen hell
	MeshBuilder b;
	const int N = 14;
	for (int i = 0; i < N; i++) {
		float a0 = -1.2f + 2.4f * i / N, a1 = -1.2f + 2.4f * (i + 1) / N;
		float w0 = sinf(PI * i / N), w1 = sinf(PI * (i + 1) / N);
		float r0 = 0.55f, r1 = 1.0f;
		Vec3 p00(sinf(a0) * r0, 0, cosf(a0) * r0), p01(sinf(a1) * r0, 0, cosf(a1) * r0);
		Vec3 p10(sinf(a0) * r1, 0, cosf(a0) * r1), p11(sinf(a1) * r1, 0, cosf(a1) * r1);
		Color ci0(0xFFFFFF, 0), ci1(0xFFFFFF, 0), co0(0xFFFFFF, w0), co1(0xFFFFFF, w1);
		b.quad(p00, p10, p11, p01, ci0, co0, co1, ci1);
		b.quad(p01, p11, p10, p00, ci1, co1, co0, ci0); // Rückseite
	}
	Mesh m = b.build();
	b.free();
	return m;
}

void loadAssets() {
	A.uiSheet.loadMem(FORGE_SHEET(ui));
	for (int i = 0; i < UI_COUNT; i++) A.ui[i] = A.uiSheet.get(i);
	A.bgSheet.loadMem(FORGE_SHEET(bg));
	for (int i = 0; i < BG_COUNT; i++) A.bg[i] = A.bgSheet.get(i);
	for (const PartSrc &p : PART_SRC) {
		CharModel &c = CM[p.ch];
		c.part[p.kind].loadMem(p.data, p.size);
		c.has[p.kind] = true;
		c.pivot[p.kind] = (p.kind == PK_FX || p.kind == PK_PET) ? centroid(c.part[p.kind]) : topPivot(c.part[p.kind]);
	}
	for (const PartSrc &w : WORLD_SRC) A.world[w.ch].loadMem(w.data, w.size);
	A.slash = slashMesh();
	A.ring = Mesh::cylinder(1, 0.02f, 28, Color(0xFFFFFF));
	A.disc = Mesh::cylinder(1, 0.01f, 20, Color(0xFFFFFF));
	A.shard = Mesh::sphere(0.12f, 4, 2, Color(0xFFFFFF));
	A.theme = audio::loadWav(theme_wav, theme_wav_size);
	A.motif[CH_IGNARA] = audio::loadWav(motif_ignara_wav, motif_ignara_wav_size);
	A.motif[CH_THALOR] = audio::loadWav(motif_thalor_wav, motif_thalor_wav_size);
	A.motif[CH_NERYS] = audio::loadWav(motif_nerys_wav, motif_nerys_wav_size);
	A.motif[CH_VEXA] = audio::loadWav(motif_vexa_wav, motif_vexa_wav_size);
	A.motif[CH_KORRIN] = audio::loadWav(motif_korrin_wav, motif_korrin_wav_size);
	A.motif[CH_LYRA] = audio::loadWav(motif_lyra_wav, motif_lyra_wav_size);
	A.motif[CH_SYLV] = audio::loadWav(motif_sylv_wav, motif_sylv_wav_size);
	using namespace synth;
	A.click = click();
	A.error = error();
	A.whoosh = whoosh();
	Params p;
	p.wave = SINE; p.freq = 880; p.freqEnd = 1320; p.dur = 0.08f; p.decay = 30; p.volume = 0.35f;
	A.select = make(p);
	p = Params(); p.wave = TRIANGLE; p.freq = 660; p.dur = 0.5f; p.decay = 5; p.arpStep = 4; p.arpTime = 0.07f; p.volume = 0.4f;
	A.chime = make(p);
	p = Params(); p.wave = NOISE; p.freq = 500; p.dur = 0.16f; p.attack = 0.03f; p.decay = 16; p.lowpass = 0.25f; p.volume = 0.5f;
	A.swing = make(p);
	p.dur = 0.24f; p.decay = 10; p.lowpass = 0.18f; p.volume = 0.6f;
	A.swing3 = make(p);
	p = Params(); p.wave = SQUARE; p.freq = 190; p.freqEnd = 60; p.dur = 0.12f; p.noiseMix = 0.55f; p.decay = 20; p.lowpass = 0.5f; p.volume = 0.6f;
	A.hit = make(p);
	p.freq = 130; p.freqEnd = 35; p.dur = 0.22f; p.decay = 12; p.volume = 0.75f;
	A.hitHeavy = make(p);
	p = Params(); p.wave = SAW; p.freq = 260; p.freqEnd = 900; p.dur = 0.18f; p.noiseMix = 0.7f; p.decay = 12; p.lowpass = 0.45f; p.volume = 0.45f;
	A.dash = make(p);
	p = Params(); p.wave = TRIANGLE; p.freq = 1400; p.freqEnd = 700; p.dur = 0.22f; p.decay = 10; p.vibrato = 0.04f; p.vibratoRate = 30; p.volume = 0.4f;
	A.cast = make(p);
	p = Params(); p.wave = SAW; p.freq = 140; p.freqEnd = 420; p.dur = 0.36f; p.noiseMix = 0.6f; p.decay = 6; p.lowpass = 0.35f; p.volume = 0.5f;
	A.spin = make(p);
	p = Params(); p.wave = NOISE; p.dur = 0.05f; p.decay = 40; p.lowpass = 0.2f; p.volume = 0.18f;
	A.step = make(p);
	if (save::read("settings", &S, sizeof S) == 0) S = Settings();
}

void saveSettings() { save::write("settings", &S, sizeof S, 1); }
void sfx(const Sound &s, float pitch, float vol) { if (S.sfx) audio::play(s, pitch, vol); }

// ------------------------------------------------------------------ Figuren zeichnen
static Mat4 around(const Vec3 &p, const Mat4 &r) { return Mat4::translate(p) * r * Mat4::translate(-p); }

Mat4 serenWeaponXf(const Mat4 &xf, const Pose &p) {
	Mat4 body = (p.sink != 0 ? Mat4::translate(Vec3(0, -p.sink, 0)) * xf : xf) * Mat4::rotY(p.bodyTwist);
	Mat4 arm = body * around(SEREN_SHOULDER, Mat4::rotY(p.armYaw) * Mat4::rotX(-p.armLift));
	float rest = p.weapon == 2 ? 0.12f : 0.78f * PI;                // Sense wird aufrecht gehalten
	float rx = rest + (PI - rest) * p.weaponBlend;
	return arm * Mat4::translate(SEREN_HAND) * Mat4::rotZ(-0.12f * (1 - p.weaponBlend)) * Mat4::rotX(rx) * Mat4::rotY(p.weaponRoll);
}

void drawChar(int ch, const Mat4 &xf, float t, const Pose *pose, float flash, bool weapon) {
	CharModel &m = CM[ch];
	Pose rest;
	const Pose &P = pose ? *pose : rest;
	Material body;
	body.specular = 0.2f;
	body.shininess = 14;
	if (flash > 0) body.emission = Color(0x000000).mix(Color(0xFFFFFF), clampf(flash, 0, 1));
	Material glow;
	glow.lit = false;
	float breathe = 1 + 0.012f * sinf(t * 2.2f + ch);
	Mat4 base = P.sink != 0 ? Mat4::translate(Vec3(0, -P.sink, 0)) * xf : xf;
	Mat4 bx = base * Mat4::rotY(P.bodyTwist);
	Mat4 bodyXf = bx * Mat4::scale(Vec3(1, breathe, 1));
	gfx3d::draw(m.part[PK_BODY], bodyXf, body);
	if (m.has[PK_LEG_L]) {                                        // Beine schwingen gegengleich um die Hüfte
		gfx3d::draw(m.part[PK_LEG_L], base * around(HIP_L, Mat4::rotX(P.legSwing)), body);
		gfx3d::draw(m.part[PK_LEG_R], base * around(HIP_R, Mat4::rotX(-P.legSwing)), body);
	}
	if (m.has[PK_ARM_L]) gfx3d::draw(m.part[PK_ARM_L], bx * around(SHOULDER_L, Mat4::rotX(-P.armSwingL) * Mat4::rotZ(-0.05f)), body);
	// schwingende Teile
	static const int SW[3] = {PK_HAIR, PK_SCARF, PK_CAPE};
	for (int k = 0; k < 3; k++) {
		int pk = SW[k];
		if (!m.has[pk]) continue;
		float amp = pk == PK_CAPE ? 0.07f : 0.1f;
		float a = sinf(t * (1.6f + k * 0.3f) + ch) * amp + P.capeLift * (pk == PK_CAPE ? 0.9f : 0.5f);
		float b = sinf(t * 1.1f + k + ch * 0.7f) * amp * 0.5f;
		gfx3d::draw(m.part[pk], bodyXf * around(m.pivot[pk], Mat4::rotX(a) * Mat4::rotZ(b)), body);
	}
	if (m.has[PK_ARM]) {
		Mat4 arm = bx * around(SEREN_SHOULDER, Mat4::rotY(P.armYaw) * Mat4::rotX(-P.armLift));
		gfx3d::draw(m.part[PK_ARM], arm, body);
	}
	if (m.has[PK_GLOW]) gfx3d::draw(m.part[PK_GLOW], bodyXf, glow);
	if (m.has[PK_FX]) {
		float f = 1 + 0.025f * sinf(t * 11) + 0.015f * sinf(t * 23);
		Material fm = glow;
		fm.color = Color(0xFFFFFF).mix(Color(0xFFD0A0), 0.5f + 0.5f * sinf(t * 7));
		gfx3d::draw(m.part[PK_FX], bodyXf * around(m.pivot[PK_FX], Mat4::scale(Vec3(f, 1 + (f - 1) * 2, f))), fm);
	}
	if (weapon) {
		Mat4 wx = ch == CH_SEREN ? serenWeaponXf(xf, P) : bodyXf;
		Material wm;
		wm.specular = 0.8f;
		wm.shininess = 40;
		if (flash > 0) wm.emission = body.emission;
		if (ch == CH_SEREN && P.weapon == 1) {                        // Schattendolche: in jeder Hand einer
			gfx3d::draw(A.world[WM_R_DAGGER], wx, wm);
			gfx3d::draw(A.world[WM_R_DAGGER_GLOW], wx, glow);
			Mat4 al = bx * around(SHOULDER_L, Mat4::rotX(-P.armSwingL) * Mat4::rotZ(-0.05f)) * Mat4::translate(Vec3(-0.275f, 0.81f, 0.11f)) * Mat4::rotZ(0.12f) * Mat4::rotX(0.78f * PI);
			gfx3d::draw(A.world[WM_R_DAGGER], al, wm);
			gfx3d::draw(A.world[WM_R_DAGGER_GLOW], al, glow);
		} else if (ch == CH_SEREN && P.weapon == 2) {
			gfx3d::draw(A.world[WM_R_SCYTHE], wx, wm);
			gfx3d::draw(A.world[WM_R_SCYTHE_GLOW], wx, glow);
		} else {
			if (m.has[PK_WEAPON]) gfx3d::draw(m.part[PK_WEAPON], wx, wm);
			if (m.has[PK_WEAPON_GLOW]) gfx3d::draw(m.part[PK_WEAPON_GLOW], wx, glow);
		}
	}
	Mat4 orb = xf * Mat4::rotY(t * 0.8f);
	if (m.has[PK_ORBIT]) gfx3d::draw(m.part[PK_ORBIT], orb, body);
	if (m.has[PK_ORBIT_GLOW]) gfx3d::draw(m.part[PK_ORBIT_GLOW], orb, glow);
	if (m.has[PK_PET]) {
		Material pm = glow;
		pm.color = Color(0xFFFFFF, 0.8f);
		gfx3d::draw(m.part[PK_PET], xf * Mat4::translate(Vec3(0, 0.04f * sinf(t * 2), 0)), pm);
	}
}

void drawGhost(int ch, const Mat4 &xf, const Pose *pose, Color c) {
	CharModel &m = CM[ch];
	Material g;
	g.lit = false;
	g.color = c;
	Pose rest;
	const Pose &P = pose ? *pose : rest;
	Mat4 bx = xf * Mat4::rotY(P.bodyTwist);
	gfx3d::draw(m.part[PK_BODY], bx, g);
	if (m.has[PK_CAPE]) gfx3d::draw(m.part[PK_CAPE], bx, g);
	if (m.has[PK_LEG_L]) { gfx3d::draw(m.part[PK_LEG_L], bx, g); gfx3d::draw(m.part[PK_LEG_R], bx, g); }
	if (m.has[PK_ARM_L]) gfx3d::draw(m.part[PK_ARM_L], bx, g);
	if (m.has[PK_ARM]) gfx3d::draw(m.part[PK_ARM], bx * around(SEREN_SHOULDER, Mat4::rotY(P.armYaw) * Mat4::rotX(-P.armLift)), g);
}

// ------------------------------------------------------------------ Partikel
void Fx::emit(Vec3 at, int n, int img, Color c0, Color c1, float speed, float life, float s0, float s1, float grav, Vec3 dir, float spread) {
	for (int i = 0; i < n; i++) {
		P &q = p[next];
		next = (next + 1) % MAX;
		Vec3 r(rng.range(-1, 1), rng.range(-1, 1), rng.range(-1, 1));
		Vec3 d = dir.len() > 0.01f ? (dir.norm() + r * spread).norm() : r.norm();
		q.p = at;
		q.v = d * (speed * rng.range(0.4f, 1.0f));
		q.max = q.life = life * rng.range(0.6f, 1.0f);
		q.s0 = s0 * rng.range(0.7f, 1.2f);
		q.s1 = s1;
		q.grav = grav;
		q.drag = 2.5f;
		q.c0 = c0;
		q.c1 = c1;
		q.img = img;
	}
}
void Fx::update(float dt) {
	for (P &q : p) {
		if (q.life <= 0) continue;
		q.life -= dt;
		q.v = q.v * (1 - fminf(1, q.drag * dt));
		q.v.y -= q.grav * dt;
		q.p += q.v * dt;
	}
}
void Fx::draw(Gfx &g, const Camera3D &cam, float scale) const {
	float ppm = g.height * 0.5f / tanf(cam.fov * PI / 360); // Pixel pro Meter bei Abstand 1
	for (const P &q : p) {
		if (q.life <= 0) continue;
		Vec2 s;
		float dep;
		if (!gfx3d::project(g, cam, q.p, &s, &dep)) continue;
		float d = (q.p - cam.eye).len();
		float k = 1 - q.life / q.max;
		float sz = (q.s0 + (q.s1 - q.s0) * k) * ppm / d * scale;
		const Image &im = A.ui[q.img];
		Color c = q.c0.mix(q.c1, k);
		g.image(im, s.x, s.y, sz / im.w, sz / im.height(), 0, c, 1, dep);
	}
}

// ------------------------------------------------------------------ UI-Hilfen
void drawButton(Gfx &g, int id, float x, float y, int img, const char *label, bool focus, float press, Color text) {
	const Image &im = A.ui[img];
	float k = (ui::held(id) ? 0.95f : 1.0f) * (1 + press * 0.04f);
	float cx = x + im.w * 0.5f, cy = y + im.height() * 0.5f;
	if (focus) g.image(A.ui[UI_P_GLOW], cx, cy, im.w / 40.0f, im.height() / 26.0f, 0, Color(0xFFD08A, 0.35f + 0.15f * sinf(g_time * 5)), 1);
	g.image(im, cx, cy, k, k, 0, Color(0xFFFFFF), 0);
	g.text(cx, cy - 8 * k, 0.52f * k, text, label, ALIGN_CENTER, 0, true);
	ui::hit(id, x, y, im.w, im.height());
}

void drawTextGlow(Gfx &g, float x, float y, float size, Color c, const char *s, int align, float depth) {
	g.text(x, y, size, c, s, align, 0, true, depth);
}
