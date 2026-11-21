// SKYBREAKER - Trainingsplatz: Kampfsystem-Prototyp mit Übungspuppe
// A: Klingen-Kombo (3 Schläge) · X: Sternwirbel · Y: Splitterwurf · B/R: Ausweichen · Circle Pad: laufen
#include "game.hpp"

enum Act { ACT_NONE, ACT_ATTACK, ACT_SPECIAL, ACT_CAST, ACT_DASH, ACT_DSTRIKE };

struct AnimKey { float t, yaw, lift, blend, twist; };
// Schlag-Animationen (Zeit 0..1)
static const AnimKey SWING[3][5] = {
	{{0, 0.3f, 0.4f, 0.5f, 0.0f}, {0.22f, 1.45f, 1.45f, 1, 0.38f}, {0.5f, -1.35f, 1.4f, 1, -0.42f}, {0.75f, -1.1f, 1.2f, 1, -0.3f}, {1, 0.0f, 0.3f, 0.3f, 0}},
	{{0, -0.6f, 1.2f, 1, -0.3f}, {0.2f, -1.15f, 1.3f, 1, -0.35f}, {0.48f, 1.55f, 1.35f, 1, 0.45f}, {0.75f, 1.2f, 1.1f, 1, 0.3f}, {1, 0.2f, 0.3f, 0.3f, 0}},
	{{0, 0.2f, 1.0f, 1, 0}, {0.32f, 0.15f, 2.95f, 1, 0.1f}, {0.55f, 0.1f, 0.75f, 1, -0.05f}, {0.8f, 0.1f, 0.8f, 1, 0}, {1, 0.0f, 0.3f, 0.3f, 0}},
};
static const float SWING_DUR[3] = {0.3f, 0.3f, 0.46f};
static const float SWING_HIT[3] = {0.36f, 0.34f, 0.52f};
static const int SWING_DMG[3] = {20, 22, 42};

static struct Hero {
	Vec3 pos, vel;
	float yaw = PI;
	int act = ACT_NONE, combo = 0;
	float actT = 0, actDur = 0;
	bool hitDone = false, queued = false;
	float comboWindow = 0;
	float dashCd = 0, specialCd = 0, castRecharge = 0;
	int castCharges = 2;
	int dashCharges = 2;
	float dashRecharge = 0, afterDash = 9;
	bool strikeQueued = false;
	Vec3 dashDir;
	float ghostTimer = 0, stepT = 0, flash = 0;
	Pose pose;
} H;

static struct Dummy {
	Vec3 pos{0, 0, -1.4f};
	float tx = 0, tz = 0, vx = 0, vz = 0, flash = 0;
} D;

struct Ghost { Vec3 p; float yaw, t; Pose pose; };
struct Num { Vec3 p; float t; int v; bool crit; Color c; };
struct Proj { Vec3 p, v; float life; };
struct SlashFx { Mat4 xf; float t, dur; Color c; };

static Ghost g_ghost[8];
static Num g_num[24];
static Proj g_proj[4];
static SlashFx g_slash[8];
static int g_ghostN, g_numN, g_slashN;
static Fx g_fx;
static float g_hitStop, g_shake, g_comboT, g_lastHitT;
static int g_chain, g_maxChain, g_hits, g_total, g_lastDmg;
static struct { float t; int d; } g_log[64];
static int g_logN;
static Camera3D g_cam;
static Vec3 g_camFocus;
static Rng g_r(31);
static float g_tt; // Szenenzeit
static bool g_helpShown;

static const struct Obst { float x, z, r; } OBST[] = {{3.9f, -2.9f, 0.36f}, {-3.9f, -2.9f, 0.36f}};
static const float FLOOR_Y = 0.075f;

void trainingEnter() {
	H = Hero();
	H.pos = Vec3(0, FLOOR_Y, 1.6f);
	D = Dummy();
	D.pos.y = FLOOR_Y;
	g_fx.clear();
	for (auto &n : g_num) n.t = 9;
	for (auto &p : g_proj) p.life = 0;
	for (auto &s : g_slash) s.t = 9;
	for (auto &gh : g_ghost) gh.t = 9;
	g_hitStop = g_shake = 0;
	g_chain = g_maxChain = g_hits = g_total = g_lastDmg = 0;
	g_logN = 0;
	g_tt = 0;
	g_camFocus = H.pos;
	if (S.music) audio::musicVolume(0.45f);
	if (!g_helpShown) { ui::toast("Trainingsplatz", "A Kombo (halten) · B+A Sprint-Schlag · X Wirbel · Y Splitter", Color(0x8AF0FF)); g_helpShown = true; }
}

// ------------------------------------------------------------------ Hilfen
static float easeIO(float x) { return x < 0.5f ? 2 * x * x : 1 - powf(-2 * x + 2, 2) / 2; }
static Vec3 fwd(float yaw) { return Vec3(sinf(yaw), 0, cosf(yaw)); }
static float angDiff(float a, float b) { float d = fmodf(a - b + 3 * PI, 2 * PI) - PI; return d; }

static void poseFromKeys(const AnimKey *k, int n, float x, Pose &p) {
	int i = 0;
	while (i < n - 2 && x > k[i + 1].t) i++;
	float u = clampf((x - k[i].t) / fmaxf(0.001f, k[i + 1].t - k[i].t), 0, 1);
	u = easeIO(u);
	p.armYaw = k[i].yaw + (k[i + 1].yaw - k[i].yaw) * u;
	p.armLift = k[i].lift + (k[i + 1].lift - k[i].lift) * u;
	p.weaponBlend = k[i].blend + (k[i + 1].blend - k[i].blend) * u;
	p.bodyTwist = k[i].twist + (k[i + 1].twist - k[i].twist) * u;
}

static void addLog(int d) {
	g_log[g_logN % 64] = {g_tt, d};
	g_logN++;
}
static float dps() {
	int sum = 0;
	float first = g_tt;
	for (int i = 0; i < 64 && i < g_logN; i++) {
		const auto &l = g_log[i];
		if (g_tt - l.t < 4) { sum += l.d; first = fminf(first, l.t); }
	}
	float span = fmaxf(1, fminf(4, g_tt - first));
	return sum / span;
}

static void hitDummy(int dmg, Vec3 from, bool heavy, Color c) {
	bool crit = g_r.f() < 0.12f;
	if (crit) dmg *= 2;
	Vec3 d = D.pos - from;
	d.y = 0;
	d = d.len() > 0.01f ? d.norm() : Vec3(0, 0, -1);
	float imp = heavy ? 5.5f : 3.2f;
	D.vx += d.x * imp;
	D.vz += d.z * imp;
	D.flash = 1;
	g_hitStop = heavy ? 0.09f : 0.05f;
	if (S.shake) g_shake = fmaxf(g_shake, heavy ? 0.32f : 0.14f);
	Vec3 hp = D.pos + Vec3(0, 0.85f, 0) - d * 0.25f;
	g_fx.emit(hp, heavy ? 16 : 10, UI_P_SPARK, Color(0xFFFFFF), c.alpha(0), 7, 0.35f, 0.32f, 0.0f, 4, -d + Vec3(0, 0.3f, 0), 0.9f);
	g_fx.emit(hp, 6, UI_P_DUST, Color(0xE8C870), Color(0xC8A050, 0), 3.5f, 0.9f, 0.09f, 0.06f, 9, -d + Vec3(0, 0.8f, 0), 0.7f);
	g_fx.emit(hp, 1, UI_P_GLOW, c.alpha(0.9f), c.alpha(0), 0, 0.18f, heavy ? 1.8f : 1.1f, 0.2f);
	Num &n = g_num[g_numN];
	g_numN = (g_numN + 1) % 24;
	n = {hp + Vec3(g_r.range(-0.2f, 0.2f), 0.3f, 0), 0, dmg, crit, crit ? Color(0xFFD24A) : c};
	g_hits++;
	g_total += dmg;
	g_lastDmg = dmg;
	addLog(dmg);
	g_chain = g_tt - g_lastHitT < 1.6f ? g_chain + 1 : 1;
	g_maxChain = g_chain > g_maxChain ? g_chain : g_maxChain;
	g_lastHitT = g_tt;
	g_comboT = 0;
	sfx(heavy ? A.hitHeavy : A.hit, g_r.range(0.9f, 1.15f), 0.9f);
	if (crit) sfx(A.select, 1.6f, 0.5f);
}

static void spawnSlash(const Mat4 &xf, float dur, Color c) {
	g_slash[g_slashN] = {xf, 0, dur, c};
	g_slashN = (g_slashN + 1) % 8;
}

// Kegel-Treffer vor der Heldin
static bool inArc(float range, float halfAngle) {
	Vec3 d = D.pos - H.pos;
	d.y = 0;
	float l = d.len();
	if (l > range + 0.3f) return false;
	if (l < 0.5f) return true;
	return fabsf(angDiff(atan2f(d.x, d.z), H.yaw)) < halfAngle;
}

static void faceAim(Vec2 stick) {
	if (stick.len() > 0.3f) { H.yaw = atan2f(stick.x, stick.y); return; }
	Vec3 d = D.pos - H.pos; // sanfte Zielhilfe zur Puppe
	d.y = 0;
	if (d.len() < 3.2f && fabsf(angDiff(atan2f(d.x, d.z), H.yaw)) < 1.1f) H.yaw = atan2f(d.x, d.z);
}

static void startAttack(Vec2 stick) {
	int c = H.act == ACT_ATTACK || H.comboWindow > 0 ? (H.combo + 1) % 3 : 0;
	if (H.act == ACT_ATTACK && H.actT < H.actDur * 0.55f) { H.queued = true; return; }
	faceAim(stick);
	H.act = ACT_ATTACK;
	H.combo = c;
	H.actT = 0;
	H.actDur = SWING_DUR[c];
	H.hitDone = false;
	H.queued = false;
	sfx(c == 2 ? A.swing3 : A.swing, g_r.range(0.95f, 1.1f), 0.7f);
}

// ------------------------------------------------------------------ Update
void trainingUpdate(float dt, const Input &in) {
	if (ui::clicked(71) || in.pressed(KEY_START)) { sfx(A.click, 0.8f); if (S.music) audio::musicVolume(0.75f); gotoScene(SC_TITLE); return; }
	if (ui::clicked(72)) { sfx(A.click); g_hits = g_total = g_maxChain = g_chain = 0; g_logN = 0; }
	g_tt += dt;
	g_fx.update(dt);
	static float emb;
	emb += dt;
	while (emb > 0.09f) {
		emb -= 0.09f;
		const Obst &o = OBST[g_r.irange(0, 1)];
		g_fx.emit(Vec3(o.x + g_r.range(-0.15f, 0.15f), 1.3f, o.z), 1, UI_P_EMBER, Color(0xFFD070), Color(0xFF5A1E, 0), 1.2f, 1.4f, 0.09f, 0.02f, -1.2f, Vec3(0, 1, 0), 0.5f);
	}
	for (auto &n : g_num) n.t += dt;
	for (auto &s : g_slash) s.t += dt;
	for (auto &gh : g_ghost) gh.t += dt;
	g_shake = fmaxf(0, g_shake - dt * 1.6f);
	if (g_hitStop > 0) { g_hitStop -= dt; return; }

	Vec2 stick(in.stick.x, in.stick.y);
	if (in.holding(KEY_DLEFT)) stick.x = -1;
	if (in.holding(KEY_DRIGHT)) stick.x = 1;
	if (in.holding(KEY_DUP)) stick.y = -1;
	if (in.holding(KEY_DDOWN)) stick.y = 1;
	if (stick.len() > 1) stick = stick.norm();
	Vec3 wish(stick.x, 0, stick.y); // Kamera schaut nach -z: Stick oben (y<0) = -z

	H.dashCd = fmaxf(0, H.dashCd - dt);
	H.specialCd = fmaxf(0, H.specialCd - dt);
	H.comboWindow = fmaxf(0, H.comboWindow - dt);
	H.flash = fmaxf(0, H.flash - dt * 5);
	H.afterDash += dt;
	if (H.dashCharges < 2) {
		H.dashRecharge += dt;
		if (H.dashRecharge > 0.85f) { H.dashCharges++; H.dashRecharge = 0; }
	}
	if (H.castCharges < 2) {
		H.castRecharge += dt;
		if (H.castRecharge > 2.2f) { H.castCharges++; H.castRecharge = 0; }
	}

	// Eingaben (Ausweichen bricht alles ab außer einem laufenden Ausweichen)
	if ((in.pressed(KEY_B) || in.pressed(KEY_R)) && H.act != ACT_DASH && H.dashCd <= 0 && H.dashCharges > 0) {
		H.dashCharges--;
		H.strikeQueued = false;
		H.act = ACT_DASH;
		H.actT = 0;
		H.actDur = 0.17f;
		H.dashDir = wish.len() > 0.2f ? wish.norm() : fwd(H.yaw);
		H.yaw = atan2f(H.dashDir.x, H.dashDir.z);
		H.ghostTimer = 0;
		sfx(A.dash, g_r.range(0.95f, 1.1f), 0.7f);
		g_fx.emit(H.pos + Vec3(0, 0.1f, 0), 10, UI_P_WISP, Color(0xE8E0D0, 0.6f), Color(0xE8E0D0, 0), 2.5f, 0.5f, 0.35f, 0.7f, -0.5f, -H.dashDir + Vec3(0, 0.3f, 0), 0.8f);
	} else if (H.act == ACT_DASH) {
		if (in.pressed(KEY_A)) H.strikeQueued = true;          // Sprint-Schlag vormerken
	} else if (H.act != ACT_DSTRIKE) {
		if (in.pressed(KEY_A)) {
			if (H.afterDash < 0.12f && H.act == ACT_NONE) H.strikeQueued = true, H.afterDash = 9;
			else startAttack(stick);
		}
		if (in.pressed(KEY_X) && H.specialCd <= 0 && H.act != ACT_SPECIAL) {
			H.act = ACT_SPECIAL; H.actT = 0; H.actDur = 0.52f; H.hitDone = false;
			sfx(A.spin, 1, 0.8f);
		}
		if (in.pressed(KEY_Y) && H.castCharges > 0 && H.act != ACT_CAST) {
			faceAim(stick);
			H.act = ACT_CAST; H.actT = 0; H.actDur = 0.3f; H.hitDone = false;
		}
	}

	// Aktion ausführen
	float moveK = 1;
	if (H.act != ACT_NONE) {
		H.actT += dt;
		float x = clampf(H.actT / H.actDur, 0, 1);
		switch (H.act) {
		case ACT_ATTACK: {
			int c = H.combo;
			poseFromKeys(SWING[c], 5, x, H.pose);
			moveK = 0.25f;
			if (x < 0.45f) H.pos += fwd(H.yaw) * ((c == 2 ? 2.4f : 1.6f) * dt); // Ausfallschritt
			if (!H.hitDone && x >= SWING_HIT[c]) {
				H.hitDone = true;
				Mat4 base = Mat4::translate(H.pos + Vec3(0, 1.0f, 0)) * Mat4::rotY(H.yaw);
				if (c == 2) spawnSlash(base * Mat4::translate(Vec3(0, 0.1f, 0.15f)) * Mat4::rotZ(PI / 2) * Mat4::scale(Vec3(1.9f, 1, 1.9f)), 0.2f, Color(0xC8F8FF));
				else spawnSlash(base * Mat4::rotZ(c ? -0.18f : 0.18f) * Mat4::scale(Vec3(c ? -1.75f : 1.75f, 1, 1.75f)), 0.17f, Color(0xC8F8FF));
				if (inArc(c == 2 ? 2.0f : 1.75f, c == 2 ? 0.75f : 1.35f)) hitDummy(SWING_DMG[c], H.pos, c == 2, Color(0x8AF0FF));
				if (c == 2) {
					g_fx.emit(H.pos + fwd(H.yaw) * 1.3f + Vec3(0, 0.05f, 0), 12, UI_P_DUST, Color(0xD8E8FF), Color(0x8AF0FF, 0), 4, 0.5f, 0.12f, 0.05f, 6, Vec3(0, 1, 0), 1.2f);
					if (S.shake) g_shake = fmaxf(g_shake, 0.18f);
				}
			}
			if (x >= 1) {
				H.act = ACT_NONE;
				H.comboWindow = 0.28f;
				if (H.queued || in.holding(KEY_A)) { H.queued = false; startAttack(stick); } // A halten = weiterschlagen
			}
			break;
		}
		case ACT_SPECIAL: {
			float spin = easeIO(x) * 2 * PI;
			H.pose.armYaw = 1.5f; H.pose.armLift = 1.5f; H.pose.weaponBlend = 1; H.pose.bodyTwist = spin;
			moveK = 0.45f;
			if (!H.hitDone && x > 0.3f) {
				H.hitDone = true;
				Mat4 base = Mat4::translate(H.pos + Vec3(0, 0.95f, 0));
				for (int i = 0; i < 3; i++) spawnSlash(base * Mat4::rotY(H.yaw + i * 2.09f) * Mat4::scale(Vec3(2.3f, 1, 2.3f)), 0.3f, Color(0xFFE8A0));
				Vec3 d = D.pos - H.pos; d.y = 0;
				if (d.len() < 2.5f) hitDummy(34, H.pos, true, Color(0xFFE08A));
				g_fx.emit(H.pos + Vec3(0, 0.9f, 0), 18, UI_P_SPARK, Color(0xFFF0C0), Color(0xFFB040, 0), 8, 0.4f, 0.22f, 0, 0, Vec3(), 1);
			}
			if (x >= 1) { H.act = ACT_NONE; H.specialCd = 0.9f; H.pose.bodyTwist = 0; }
			break;
		}
		case ACT_CAST: {
			AnimKey k[3] = {{0, 0.3f, 2.7f, 1, 0.2f}, {0.45f, 0.1f, 1.25f, 1, -0.15f}, {1, 0.0f, 0.4f, 0.4f, 0}};
			poseFromKeys(k, 3, x, H.pose);
			moveK = 0.3f;
			if (!H.hitDone && x > 0.4f) {
				H.hitDone = true;
				H.castCharges--;
				for (auto &p : g_proj)
					if (p.life <= 0) { p.p = H.pos + Vec3(0, 1.15f, 0) + fwd(H.yaw) * 0.5f; p.v = fwd(H.yaw) * 17; p.life = 0.7f; break; }
				sfx(A.cast, g_r.range(0.95f, 1.08f), 0.8f);
			}
			if (x >= 1) H.act = ACT_NONE;
			break;
		}
		case ACT_DASH: {
			H.pos += H.dashDir * (23.0f * dt);
			H.ghostTimer -= dt;
			if (H.ghostTimer <= 0) {
				H.ghostTimer = 0.035f;
				g_ghost[g_ghostN] = {H.pos, H.yaw, 0, H.pose};
				g_ghostN = (g_ghostN + 1) % 8;
			}
			H.pose.capeLift = 0.9f;
			if (H.actT >= H.actDur) { H.act = ACT_NONE; H.dashCd = 0.1f; H.vel = H.dashDir * 4; H.afterDash = 0; }
			moveK = 0;
			break;
		}
		case ACT_DSTRIKE: {
			static const AnimKey k[4] = {{0, 1.3f, 1.4f, 1, 0.4f}, {0.35f, -1.4f, 1.35f, 1, -0.5f}, {0.7f, -1.2f, 1.1f, 1, -0.3f}, {1, 0, 0.3f, 0.3f, 0}};
			poseFromKeys(k, 4, x, H.pose);
			moveK = 0;
			if (x < 0.4f) H.pos += fwd(H.yaw) * (9.0f * dt);
			if (!H.hitDone && x >= 0.3f) {
				H.hitDone = true;
				Mat4 base = Mat4::translate(H.pos + Vec3(0, 1.0f, 0)) * Mat4::rotY(H.yaw);
				spawnSlash(base * Mat4::scale(Vec3(2.2f, 1, 2.0f)), 0.2f, Color(0xFFFFFF));
				if (inArc(2.1f, 1.4f)) hitDummy(30, H.pos, true, Color(0xE8FFFF));
			}
			if (x >= 1) { H.act = ACT_NONE; H.comboWindow = 0; }
			break;
		}
		}
	}
	if (H.strikeQueued && H.act == ACT_NONE) {
		H.strikeQueued = false;
		faceAim(stick);
		H.act = ACT_DSTRIKE; H.actT = 0; H.actDur = 0.3f; H.hitDone = false;
		sfx(A.swing3, 1.2f, 0.8f);
	}
	// Laufen
	if (H.act == ACT_NONE) {
		H.pose.armYaw += (0 - H.pose.armYaw) * fminf(1, dt * 12);
		H.pose.armLift += (0.05f * sinf(g_tt * 2) - H.pose.armLift) * fminf(1, dt * 12);
		H.pose.weaponBlend += (0 - H.pose.weaponBlend) * fminf(1, dt * 10);
		H.pose.bodyTwist += (0 - H.pose.bodyTwist) * fminf(1, dt * 12);
	}
	Vec3 target = wish * (6.4f * moveK);
	H.vel = H.vel + (target - H.vel) * fminf(1, dt * 14);
	H.pos += H.vel * dt;
	float sp = H.vel.len();
	if (H.act == ACT_NONE && sp > 0.4f) H.yaw += angDiff(atan2f(H.vel.x, H.vel.z), H.yaw) * fminf(1, dt * 14);
	if (H.act != ACT_DASH) H.pose.capeLift += (clampf(sp / 5.6f, 0, 1) * 0.45f - H.pose.capeLift) * fminf(1, dt * 6);
	if (sp > 1 && H.act == ACT_NONE) {
		H.stepT += dt * sp * 0.55f;
		if (H.stepT > 1) { H.stepT = 0; sfx(A.step, g_r.range(0.8f, 1.2f), 0.6f); g_fx.emit(H.pos, 2, UI_P_WISP, Color(0xD8D0C0, 0.4f), Color(0xD8D0C0, 0), 0.8f, 0.4f, 0.2f, 0.35f); }
	}
	// Kollision: Inselrand, Hindernisse, Puppe
	Vec3 flat(H.pos.x, 0, H.pos.z);
	if (flat.len() > 4.35f) { flat = flat * (4.35f / flat.len()); H.pos.x = flat.x; H.pos.z = flat.z; }
	for (const Obst &o : OBST) {
		Vec3 d(H.pos.x - o.x, 0, H.pos.z - o.z);
		float l = d.len(), r = o.r + 0.3f;
		if (l < r && l > 1e-4f) { H.pos.x = o.x + d.x / l * r; H.pos.z = o.z + d.z / l * r; }
	}
	{
		Vec3 d = H.pos - D.pos;
		d.y = 0;
		float l = d.len();
		if (l < 0.62f && l > 1e-4f) { H.pos.x = D.pos.x + d.x / l * 0.62f; H.pos.z = D.pos.z + d.z / l * 0.62f; }
	}
	H.pos.y = FLOOR_Y;
	// Projektile
	for (auto &p : g_proj) {
		if (p.life <= 0) continue;
		p.life -= dt;
		p.p += p.v * dt;
		if (g_r.f() < 0.6f) g_fx.emit(p.p, 1, UI_P_SPARK, Color(0xC8FFFF, 0.9f), Color(0x4AB8FF, 0), 0.5f, 0.3f, 0.16f, 0.02f);
		Vec3 d = p.p - (D.pos + Vec3(0, 0.9f, 0));
		d.y *= 0.5f;
		if (d.len() < 0.5f) {
			hitDummy(40, p.p - p.v * 0.1f, false, Color(0x6AD8FF));
			g_fx.emit(p.p, 14, UI_P_SPARK, Color(0xFFFFFF), Color(0x4AB8FF, 0), 6, 0.4f, 0.26f, 0);
			p.life = 0;
		}
		if (p.life <= 0 && d.len() >= 0.5f) g_fx.emit(p.p, 6, UI_P_SPARK, Color(0xC8FFFF), Color(0x4AB8FF, 0), 3, 0.3f, 0.2f, 0);
	}
	// Puppe federt zurück
	D.vx += (-60 * D.tx - 5.5f * D.vx) * dt;
	D.vz += (-60 * D.tz - 5.5f * D.vz) * dt;
	D.tx += D.vx * dt;
	D.tz += D.vz * dt;
	D.flash = fmaxf(0, D.flash - dt * 7);
	g_comboT += dt;
	if (g_tt - g_lastHitT > 1.6f) g_chain = 0;
	// Kamera
	g_camFocus = g_camFocus + (Vec3(H.pos.x * 0.7f, 0, H.pos.z * 0.6f - 0.4f) - g_camFocus) * fminf(1, dt * 5);
}

// ------------------------------------------------------------------ Zeichnen
void trainingDraw(Gfx &g) {
	if (g.screen == Screen::Top) {
		float t = g_time;
		g.rectGrad(0, 0, 400, 120, Color(0x1A1440), Color(0x5A3478), 10.5f);
		g.rectGrad(0, 120, 400, 120, Color(0x5A3478), Color(0xC8787A), 10.5f);
		for (int i = 0; i < 3; i++) {
			float x = fmodf(i * 170 + t * (5 + i * 3), 560) - 80;
			g.image(A.ui[UI_CLOUD0 + i], x, 150 + i * 30, 2.0f, 1.4f, 0, Color(0xFFD8E8, 0.4f), 0, 9 - i);
		}
		Vec3 sh(0, 0, 0);
		if (g_shake > 0) sh = Vec3(g_r.range(-1, 1), g_r.range(-1, 1), g_r.range(-1, 1)) * (g_shake * 0.16f);
		g_cam.target = g_camFocus + Vec3(0, 0.6f, 0) + sh;
		g_cam.eye = g_camFocus + Vec3(0, 6.9f, 6.3f) + sh;
		g_cam.fov = 42;
		g_cam.focus = (g_cam.eye - g_cam.target).len();
		g_cam.zfar = 300;
		Scene3D sc;
		sc.ambient = Color(0x48446A);
		sc.nLights = 0;
		Light3D sun; sun.pos = Vec3(0.5f, 0.9f, 0.35f); sun.color = Color(0xFFE6C8); sun.intensity = 0.85f;
		Light3D rim; rim.pos = Vec3(-0.5f, 0.4f, -0.8f); rim.color = Color(0x8A70E0); rim.intensity = 0.55f;
		sc.lights[sc.nLights++] = sun;
		sc.lights[sc.nLights++] = rim;
		Light3D star; star.point = true; star.pos = H.pos + Vec3(0.1f, 1.25f, 0.2f); star.color = Color(0x8AF0FF); star.intensity = 0.6f; star.range = 1.6f;
		sc.lights[sc.nLights++] = star;
		for (const Obst &o : OBST) {
			Light3D f; f.point = true; f.pos = Vec3(o.x, 1.4f, o.z); f.color = Color(0xFF9A4A);
			f.intensity = 1.1f + 0.15f * sinf(t * 9 + o.x); f.range = 4.2f;
			sc.lights[sc.nLights++] = f;
		}
		for (auto &p : g_proj)
			if (p.life > 0 && sc.nLights < 8) { Light3D l; l.point = true; l.pos = p.p; l.color = Color(0x6AD8FF); l.intensity = 1.6f; l.range = 3.2f; sc.lights[sc.nLights++] = l; }
		sc.fog = true;
		sc.fogColor = Color(0x6A5090);
		sc.fogNear = 25; sc.fogFar = 120;
		gfx3d::begin(g, g_cam, sc);
		Material lit;
		lit.specular = 0.08f;
		gfx3d::draw(A.world[WM_ISLAND_PLAIN], Mat4::translate(Vec3(0, -0.34f, 0)) * Mat4::scale(Vec3(2.6f, 1.7f, 2.6f)), lit);
		Material al = lit;
		al.specular = 0.25f;
		gfx3d::draw(A.world[WM_ARENA], Mat4::identity(), al);
		Material ag; ag.lit = false; ag.color = Color(0xFFFFFF).mix(Color(0x8AF0FF), 0.5f + 0.5f * sinf(t * 1.5f));
		gfx3d::draw(A.world[WM_ARENA_GLOW], Mat4::identity(), ag);
		for (const Obst &o : OBST) {
			Mat4 bm = Mat4::translate(Vec3(o.x, 0.07f, o.z));
			gfx3d::draw(A.world[WM_BRAZIER], bm, lit);
			Material fm; fm.lit = false;
			float fl = 1 + 0.08f * sinf(t * 13 + o.x) + 0.05f * sinf(t * 29);
			gfx3d::draw(A.world[WM_BRAZIER_FIRE], bm * Mat4::translate(Vec3(0, 1.1f, 0)) * Mat4::scale(Vec3(1, fl, 1)) * Mat4::translate(Vec3(0, -1.1f, 0)), fm);
		}
		gfx3d::draw(A.world[WM_SHARD_B], Mat4::translate(Vec3(-9, -4 + sinf(t * 0.5f) * 0.3f, -6)), lit);
		gfx3d::draw(A.world[WM_SHARD_A], Mat4::translate(Vec3(8, -6 + sinf(t * 0.4f + 1) * 0.3f, -3)) * Mat4::rotY(t * 0.05f), lit);
		gfx3d::draw(A.world[WM_SHARD_C], Mat4::translate(Vec3(5, -3 + sinf(t * 0.6f + 2) * 0.2f, 6)), lit);
		// Puppe
		Mat4 dm = Mat4::translate(D.pos) * Mat4::rotY(0.3f);
		gfx3d::draw(A.world[WM_DUMMY_BASE], dm, lit);
		Material dl = lit;
		if (D.flash > 0) dl.emission = Color(0x000000).mix(Color(0xFFFFFF), D.flash);
		gfx3d::draw(A.world[WM_DUMMY_TOP], Mat4::translate(D.pos + Vec3(0, 0.5f, 0)) * Mat4::rotX(D.tz) * Mat4::rotZ(-D.tx) * Mat4::rotY(0.3f), dl);
		// Schatten
		Material shm; shm.lit = false; shm.color = Color(0x000000, 0.3f);
		gfx3d::draw(A.disc, Mat4::translate(H.pos + Vec3(0, 0.03f, 0)) * Mat4::scale(Vec3(0.42f, 1, 0.42f)), shm);
		gfx3d::draw(A.disc, Mat4::translate(D.pos + Vec3(0, 0.03f, 0)) * Mat4::scale(Vec3(0.5f, 1, 0.5f)), shm);
		// Heldin + Nachbilder
		for (const Ghost &gh : g_ghost)
			if (gh.t < 0.22f) drawGhost(CH_SEREN, Mat4::translate(gh.p) * Mat4::rotY(gh.yaw), &gh.pose, Color(0x6AD8FF, 0.45f * (1 - gh.t / 0.22f)));
		drawChar(CH_SEREN, Mat4::translate(H.pos) * Mat4::rotY(H.yaw), t, &H.pose, H.flash);
		// Projektile
		Material pg; pg.lit = false; pg.color = Color(0xC8FFFF);
		for (auto &p : g_proj)
			if (p.life > 0) gfx3d::draw(A.shard, Mat4::translate(p.p) * Mat4::rotY(t * 20) * Mat4::rotX(PI / 2) * Mat4::scale(Vec3(1.2f, 2.2f, 1.2f)), pg);
		// Hiebe
		for (const SlashFx &s : g_slash) {
			if (s.t >= s.dur) continue;
			float k = s.t / s.dur;
			Material sm; sm.lit = false; sm.color = s.c.alpha(1 - k * k);
			float sc2 = 0.85f + 0.3f * k;
			gfx3d::draw(A.slash, s.xf * Mat4::scale(Vec3(sc2, 1, sc2)), sm);
		}
		gfx3d::end(g);
		// Leuchten, Partikel, Zahlen
		Vec2 sp;
		float dep;
		for (auto &p : g_proj)
			if (p.life > 0 && gfx3d::project(g, g_cam, p.p, &sp, &dep)) g.image(A.ui[UI_P_GLOW], sp.x, sp.y, 0.9f, 0.9f, 0, Color(0x6AD8FF, 0.7f), 1, dep);
		if (gfx3d::project(g, g_cam, H.pos + Vec3(0.08f, 1.2f, 0.15f), &sp, &dep)) g.image(A.ui[UI_P_GLOW], sp.x, sp.y, 0.35f, 0.35f, 0, Color(0x8AF0FF, 0.55f), 1, dep);
		for (const Obst &o : OBST)
			if (gfx3d::project(g, g_cam, Vec3(o.x, 1.35f, o.z), &sp, &dep)) g.image(A.ui[UI_P_GLOW], sp.x, sp.y, 1.1f, 1.1f, 0, Color(0xFF9A4A, 0.45f + 0.1f * sinf(t * 11)), 1, dep);
		g_fx.draw(g, g_cam);
		if (S.numbers)
			for (const Num &n : g_num) {
				if (n.t > 0.85f) continue;
				Vec3 p = n.p + Vec3(0, n.t * 1.2f, 0);
				if (!gfx3d::project(g, g_cam, p, &sp, &dep)) continue;
				float pop = n.t < 0.08f ? 1.6f - n.t * 7 : 1.0f;
				float a = n.t > 0.6f ? 1 - (n.t - 0.6f) / 0.25f : 1;
				char b[16];
				snprintf(b, sizeof b, "%d", n.v);
				float sz = (n.crit ? 0.8f : 0.6f) * pop;
				g.text(sp.x, sp.y - 10, sz, n.c.alpha(a), b, ALIGN_CENTER, 0, true, dep - 1);
				if (n.crit) g.text(sp.x, sp.y - 24, 0.36f, Color(0xFFD24A, a), "KRIT", ALIGN_CENTER, 0, true, dep - 1);
			}
		if (g_chain >= 2) {
			float pop = g_comboT < 0.1f ? 1.3f - g_comboT * 3 : 1;
			char b[24];
			snprintf(b, sizeof b, "%d", g_chain);
			g.text(386, 12, 0.9f * pop, Color(0xFFE08A), b, ALIGN_RIGHT, 0, true, -2);
			g.text(386, 44, 0.36f, Color(0xFFFFFF, 0.8f), "TREFFER", ALIGN_RIGHT, 0, true, -2);
		}
		ui::drawToasts(g, 4);
		return;
	}
	// Unterer Bildschirm: Werte und Fähigkeiten
	g.imageAt(A.bg[BG_BOTTOM], 0, 0);
	g.text(160, 6, 0.46f, Color(0xE8C060), "TRAININGSPLATZ", ALIGN_CENTER);
	drawButton(g, 71, 6, 4, UI_BTN_SMALL, "", false, 0);
	g.text(38, 9, 0.38f, Color(0xFFFFFF), "< Menü", ALIGN_CENTER);
	ui::panel(g, 10, 32, 300, 70, Color(0x0E1026, 0.85f), Color(0x4A4A80));
	char b[64];
	snprintf(b, sizeof b, "%.0f", dps());
	g.text(24, 38, 0.36f, Color(0xA8A4C8), "Schaden / Sek.", ALIGN_LEFT);
	g.text(24, 52, 0.9f, Color(0xFFE08A), b, ALIGN_LEFT);
	snprintf(b, sizeof b, "Treffer  %d", g_hits); g.text(160, 40, 0.38f, Color(0xE8E4F8), b, ALIGN_LEFT);
	snprintf(b, sizeof b, "Gesamt  %d", g_total); g.text(160, 56, 0.38f, Color(0xE8E4F8), b, ALIGN_LEFT);
	snprintf(b, sizeof b, "Beste Serie  %d", g_maxChain); g.text(160, 72, 0.38f, Color(0xE8E4F8), b, ALIGN_LEFT);
	snprintf(b, sizeof b, "Letzter  %d", g_lastDmg); g.text(160, 88, 0.34f, Color(0xA8A4C8), b, ALIGN_LEFT);
	// Fähigkeiten
	struct Sk { const char *key, *name; float ready; int pips, maxPips; Color c; } sk[4] = {
		{"A", "Kombo", 1, H.act == ACT_ATTACK ? H.combo + 1 : 0, 3, Color(0x8AF0FF)},
		{"X", "Wirbel", 1 - H.specialCd / 0.9f, -1, 0, Color(0xFFE08A)},
		{"Y", "Splitter", H.castCharges >= 2 ? 1 : H.castRecharge / 2.2f, H.castCharges, 2, Color(0x6AD8FF)},
		{"B", "Ausweichen", H.dashCharges >= 2 ? 1 : H.dashRecharge / 0.85f, H.dashCharges, 2, Color(0xE8E0D0)},
	};
	for (int i = 0; i < 4; i++) {
		float x = 12 + i * 75, y = 110;
		bool rdy = sk[i].ready >= 1;
		ui::panel(g, x, y, 70, 62, Color(rdy ? 0x1A1E44 : 0x101228, 0.9f), rdy ? sk[i].c.alpha(0.8f) : Color(0x3A3A5A));
		g.circle(x + 18, y + 18, 11, rdy ? sk[i].c : Color(0x4A4A6A));
		g.text(x + 18, y + 10, 0.42f, Color(0x0A0A1A), sk[i].key, ALIGN_CENTER, 0, false);
		g.text(x + 35, y + 40, 0.34f, Color(0xE8E4F8), sk[i].name, ALIGN_CENTER, 66);
		if (!rdy) g.rect(x + 6, y + 55, 58 * clampf(sk[i].ready, 0, 1), 3, sk[i].c);
		for (int p = 0; p < sk[i].maxPips; p++) g.circle(x + 38 + p * 9, y + 18, 3, p < sk[i].pips ? sk[i].c : Color(0x3A3A5A));
	}
	drawButton(g, 72, 44, 182, UI_BTN, "Werte zurücksetzen", false, 0);
	g.text(160, 224, 0.3f, Color(0x6A6490), "Feedback zum Kampfgefühl? Sag mir, was sich gut anfühlt!", ALIGN_CENTER);
}
