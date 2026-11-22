// SKYBREAKER – Kampfsystem (Heldin, Gegner-KI, Elemente, Effekte, Raum-Darstellung)
#include <cstdlib>
#include "combat.hpp"

Arena AR;

enum Act { ACT_NONE, ACT_ATTACK, ACT_SPECIAL, ACT_CAST, ACT_DASH, ACT_DSTRIKE };
enum { HF_HEAVY = 1, HF_NOSTOP = 2, HF_NOCHAIN = 4, HF_NONUM = 8 };

u32 godColor(int god) { return god >= 0 && god < CH_COUNT ? CHAR_INFO[god].color : 0x8AF0FF; }

struct AnimKey { float t, yaw, lift, blend, twist; };
static const AnimKey SWING[3][5] = {
	{{0, 0.3f, 0.4f, 0.5f, 0.0f}, {0.22f, 1.45f, 1.45f, 1, 0.38f}, {0.5f, -1.35f, 1.4f, 1, -0.42f}, {0.75f, -1.1f, 1.2f, 1, -0.3f}, {1, 0.0f, 0.3f, 0.3f, 0}},
	{{0, -0.6f, 1.2f, 1, -0.3f}, {0.2f, -1.15f, 1.3f, 1, -0.35f}, {0.48f, 1.55f, 1.35f, 1, 0.45f}, {0.75f, 1.2f, 1.1f, 1, 0.3f}, {1, 0.2f, 0.3f, 0.3f, 0}},
	{{0, 0.2f, 1.0f, 1, 0}, {0.32f, 0.15f, 2.95f, 1, 0.1f}, {0.55f, 0.1f, 0.75f, 1, -0.05f}, {0.8f, 0.1f, 0.8f, 1, 0}, {1, 0.0f, 0.3f, 0.3f, 0}},
};
// Waffen: Schattenklinge (3 Hiebe), Schattendolche (4 schnelle Stiche), Mondsense (2 weite, schwere Schwünge)
struct WeaponDef { int hits; float dur[4], hitAt[4], dmg[4], range[4], half[4], slash[4]; int keys[4]; bool heavy[4], leftThrust[4]; };
static const WeaponDef WEAPONS[3] = {
	{3, {0.3f, 0.3f, 0.46f}, {0.36f, 0.34f, 0.52f}, {20, 22, 42}, {1.75f, 1.75f, 2.0f}, {1.35f, 1.35f, 0.75f}, {1.75f, 1.75f, 1.9f}, {0, 1, 2}, {false, false, true}, {}},
	{4, {0.18f, 0.18f, 0.18f, 0.32f}, {0.35f, 0.4f, 0.35f, 0.5f}, {11, 11, 12, 28}, {1.45f, 1.45f, 1.45f, 1.6f}, {1.1f, 1.1f, 1.1f, 0.8f}, {1.35f, 1.2f, 1.35f, 1.5f}, {0, 0, 1, 2}, {false, false, false, true}, {false, true, false, true}},
	{2, {0.44f, 0.56f}, {0.42f, 0.45f}, {34, 54}, {2.55f, 2.7f}, {1.9f, 1.9f}, {2.6f, 2.8f}, {0, 1}, {true, true}, {}},
};
#define SWING_DUR(c) (WEAPONS[AR.mods.weapon].dur[c])

struct EDef { float hp, speed, r, dmg, scale; int body, glow; };
static const EDef EDEFS[EN_COUNT] = {
	{1e9f, 0, 0.35f, 0, 1, WM_DUMMY_TOP, -1},
	{55, 3.0f, 0.38f, 12, 1, WM_E_SHARD_BODY, WM_E_SHARD_GLOW},
	{24, 4.4f, 0.32f, 6, 1, WM_E_CRAWLER_BODY, WM_E_CRAWLER_GLOW},
	{36, 2.2f, 0.35f, 8, 1, WM_E_WISP_BODY, WM_E_WISP_GLOW},
	{28, 3.8f, 0.36f, 22, 1, WM_E_BOMBER_BODY, WM_E_BOMBER_GLOW},
	{170, 1.7f, 0.6f, 20, 1, WM_E_BRUTE_BODY, WM_E_BRUTE_GLOW},
	{1500, 1.9f, 1.25f, 26, 2.1f, WM_E_BRUTE_BODY, WM_E_BRUTE_GLOW},
	{34, 4.0f, 0.36f, 12, 1, WM_E_SPIDER_BODY, WM_E_SPIDER_GLOW},
	{40, 2.0f, 0.34f, 14, 1, WM_E_ARCHER_BODY, WM_E_ARCHER_GLOW},
	{120, 1.3f, 0.55f, 16, 1, WM_E_GEODE_BODY, WM_E_GEODE_GLOW},
	{18, 3.8f, 0.28f, 6, 0.45f, WM_E_GEODE_BODY, WM_E_GEODE_GLOW},
	{1800, 1.4f, 0.9f, 22, 1, WM_E_QUEEN_BODY, WM_E_QUEEN_GLOW},
};

// ------------------------------------------------------------------ Effekte
struct Ghost { Vec3 p; float yaw, t; Pose pose; };
struct Num { Vec3 p; float t; int v; bool crit; Color c; };
struct HProj { Vec3 p, v; float life, dmg, r; int kind, god; bool pierce; u32 hitMask; int bounces; };
enum { HK_SHARD, HK_WAVE, HK_BOULDER, HK_DAGGER, HK_BOUNCE, HK_BEAM };
// Beschwörungen der Götter-Segen
struct Summon { int kind, god; Vec3 p, v; float t, dur, cd, pow, yaw; bool active; u32 hitMask; };
enum { SM_TORNADO, SM_WOLF, SM_STAR, SM_CLONE, SM_FIRERING, SM_RING };
struct EProj { Vec3 p, v; float life, dmg, r; };
struct SlashFx { Mat4 xf; float t, dur; Color c; };
struct Tele { Vec3 p; float r, t, dur; Color c; };
struct Bolt { Vec3 a, b; float t; Color c; };
struct Blast { Vec3 p; float r, t, dmg; int god; bool hurtsHero, hurtsEnemies; };

static Ghost g_ghost[8];
static Num g_num[32];
static HProj g_hp[16];
static Summon g_sum[12];
static EProj g_ep[24];
static SlashFx g_slash[10];
static Tele g_tele[16];
static Bolt g_bolt[8];
static Blast g_blast[8];
static int g_ghostN, g_numN, g_slashN, g_boltN;
static Fx g_fx;
static float g_hitStop, g_shake, g_comboT, g_lastHitT, g_hurtFlash;
static struct { float t; int d; } g_log[64];
static int g_logN;
static Vec3 g_camFocus;
static Rng g_r(31);

static float easeIO(float x) { return x < 0.5f ? 2 * x * x : 1 - powf(-2 * x + 2, 2) / 2; }
static Vec3 fwd(float yaw) { return Vec3(sinf(yaw), 0, cosf(yaw)); }
static float angDiff(float a, float b) { return fmodf(a - b + 3 * PI, 2 * PI) - PI; }
static Vec3 flat(Vec3 v) { return Vec3(v.x, 0, v.z); }

static void poseFromKeys(const AnimKey *k, int n, float x, Pose &p) {
	int i = 0;
	while (i < n - 2 && x > k[i + 1].t) i++;
	float u = easeIO(clampf((x - k[i].t) / fmaxf(0.001f, k[i + 1].t - k[i].t), 0, 1));
	p.armYaw = k[i].yaw + (k[i + 1].yaw - k[i].yaw) * u;
	p.armLift = k[i].lift + (k[i + 1].lift - k[i].lift) * u;
	p.weaponBlend = k[i].blend + (k[i + 1].blend - k[i].blend) * u;
	p.bodyTwist = k[i].twist + (k[i + 1].twist - k[i].twist) * u;
}

static void summon(int kind, int god, Vec3 p, float dur, float pow, float yaw = 0) {
	for (Summon &m : g_sum) if (!m.active) { m = {kind, god, p, Vec3(), 0, dur, 0, pow, yaw, true, 0}; return; }
}
static Enemy *nearestEnemy(Vec3 p, float maxD, const Enemy *skip = nullptr) {
	Enemy *best = nullptr;
	float bd = maxD;
	for (Enemy &e : AR.enemies) {
		if (!e.alive || e.state == 0 || e.state == 5 || &e == skip || e.type == EN_DUMMY && false) continue;
		float d = Vec3(e.pos.x - p.x, 0, e.pos.z - p.z).len();
		if (d < bd) { bd = d; best = &e; }
	}
	return best;
}
static void shoot(int kind, int god, Vec3 p, Vec3 v, float life, float dmg, float r, bool pierce, int bounces = 0) {
	for (HProj &q : g_hp) if (q.life <= 0) { q = {p, v, life, dmg, r, kind, god, pierce, 0, bounces}; return; }
}
static void spawnSlash(const Mat4 &xf, float dur, Color c) { g_slash[g_slashN] = {xf, 0, dur, c}; g_slashN = (g_slashN + 1) % 10; }
static void tele(Vec3 p, float r, float dur, Color c) {
	for (Tele &t : g_tele) if (t.t >= t.dur) { t = {p, r, 0, dur, c}; return; }
}
static void blast(Vec3 p, float r, float dmg, int god, bool hero, bool enemies) {
	for (Blast &b : g_blast) if (b.t >= 0.35f) { b = {p, r, 0, dmg, god, hero, enemies}; return; }
}

// ------------------------------------------------------------------ Einrichtung
void Arena::begin(bool trainingMode, const Mods &m) {
	training = trainingMode;
	mods = m;
	hero = Hero();
	hero.hp = float(mods.maxHp);
	hero.dashCharges = mods.dashCharges;
	hero.castCharges = mods.castCharges;
	hero.pos = Vec3(0, 0.075f, 2.6f);
	totalKills = coinsGained = 0;
	resetStats();
	newRoom(1, false);
}

void Arena::resetStats() { hits = total = chain = maxChain = lastDmg = 0; g_logN = 0; }

void Arena::clearEnemies() { for (Enemy &e : enemies) e.alive = false; }

void Arena::newRoom(int seed, bool keepHero) {
	clearEnemies();
	pickup.active = false;
	for (Door &d : doors) d.active = false;
	g_fx.clear();
	for (auto &n : g_num) n.t = 9;
	for (auto &p : g_hp) p.life = 0;
	for (auto &p : g_ep) p.life = 0;
	for (auto &m : g_sum) m.active = false;
	for (auto &s : g_slash) s.t = 9;
	for (auto &t : g_tele) t.t = t.dur = 0;
	for (auto &b : g_blast) b.t = 9;
	for (auto &gh : g_ghost) gh.t = 9;
	g_hitStop = g_shake = g_hurtFlash = 0;
	dmgMulRoom = speedMulRoom = 1;
	if (keepHero || !training) {
		hero.pos = Vec3(0, 0.075f, 3.3f);
		hero.vel = Vec3();
		hero.yaw = PI;
		hero.act = ACT_NONE;
	}
	g_camFocus = flat(hero.pos);
	// Hindernisse: 2 Feuerschalen am Rand + zufällige Säulen/Felsen
	Rng r(u32(seed * 7919 + 13));
	nObst = 0;
	gridMode = !training && !hub;
	if (gridMode) {
		buildRoom(grid, layout, r.f() < 0.5f, biome, u32(seed * 2654435761u), roomMesh, roomGlow);
		hero.pos = grid.entry;
		for (int k = 0; k < grid.nPillar; k++) obst[nObst++] = {WM_R_COLUMN, grid.pillar[k], 0.42f, r.range(0, 6.28f), 1.1f, false};
		if (biome == 0)
			for (int k = 0; k < 2 && nObst < 10; k++) {
				Vec3 p = grid.randomFloor(r, hero.pos, 3.0f);
				obst[nObst++] = {WM_BRAZIER, p, 0.36f, 0, 1, true};
			}
		int extra = layout == LY_PILLARS ? 0 : r.irange(0, 2);
		for (int k = 0; k < extra && nObst < 10; k++) {
			Vec3 p = grid.randomFloor(r, hero.pos, 2.5f);
			bool col = r.f() < 0.5f;
			obst[nObst++] = {col ? WM_R_COLUMN : WM_R_ROCK, p, col ? 0.4f : 0.55f, r.range(0, 6.28f), r.range(0.9f, 1.15f), false};
		}
		g_camFocus = flat(hero.pos);
		static const u32 TINT2[4] = {0xFFFFFF, 0xE8E0F0, 0xF0E4D8, 0xDCE8E8};
		floorTint = Color(TINT2[seed & 3]);
		return;
	}
	float a0 = r.range(0.6f, 1.0f);
	obst[nObst++] = {WM_BRAZIER, Vec3(sinf(PI + a0) * 4.0f, 0.07f, cosf(PI + a0) * 4.0f), 0.36f, 0, 1, true};
	obst[nObst++] = {WM_BRAZIER, Vec3(sinf(PI - a0) * 4.0f, 0.07f, cosf(PI - a0) * 4.0f), 0.36f, 0, 1, true};
	if (hub) {
		nObst = 0;
		obst[nObst++] = {WM_BRAZIER, Vec3(-1.5f, 0.07f, -3.9f), 0.36f, 0, 1, true};
		obst[nObst++] = {WM_BRAZIER, Vec3(1.5f, 0.07f, -3.9f), 0.36f, 0, 1, true};
		obst[nObst++] = {WM_R_ALTAR, Vec3(-2.7f, 0.07f, -2.5f), 0.62f, 0.5f, 1, false};
		obst[nObst++] = {WM_R_RACK, Vec3(2.9f, 0.07f, -2.8f), 0.7f, -0.75f, 1, false};
	}
	if (!training && !hub) {
		int n = r.irange(1, 3);
		for (int i = 0; i < n && nObst < 10; i++) {
			float a = r.range(0, 2 * PI), d = r.range(2.0f, 3.5f);
			Vec3 p(sinf(a) * d, 0.07f, cosf(a) * d);
			if (p.z > 1.8f && fabsf(p.x) < 1.6f) continue; // Eingang frei lassen
			bool col = r.f() < 0.5f;
			obst[nObst++] = {col ? WM_R_COLUMN : WM_R_ROCK, p, col ? 0.4f : 0.55f, r.range(0, 6.28f), r.range(0.9f, 1.15f), false};
		}
	}
	static const u32 TINT[4] = {0xFFFFFF, 0xE8E0F0, 0xF0E4D8, 0xDCE8E8};
	floorTint = Color(TINT[seed & 3]);
}

int Arena::spawn(int type, Vec3 pos, bool elite, float delay) {
	for (int i = 0; i < 24; i++) {
		Enemy &e = enemies[i];
		if (e.alive) continue;
		e = Enemy();
		const EDef &d = EDEFS[type];
		e.type = type;
		e.alive = true;
		e.elite = elite;
		e.pos = Vec3(pos.x, 0.075f, pos.z);
		e.hp = e.maxHp = d.hp * (elite ? 2.2f : 1);
		e.r = d.r * (elite ? 1.15f : 1);
		e.speed = d.speed;
		e.dmg = d.dmg * (elite ? 1.35f : 1);
		e.scale = d.scale * (elite ? 1.18f : 1);
		e.state = 0;
		e.st = -delay;
		e.cd = g_r.range(0.3f, 1.2f);
		e.yaw = atan2f(hero.pos.x - pos.x, hero.pos.z - pos.z);
		e.bob = g_r.range(0, 6);
		if (type == EN_DUMMY) { e.state = 1; e.st = 0; }
		else tele(e.pos, e.r * 1.8f, 0.8f + delay, Color(elite ? 0xFFC850 : 0xB070FF, 0.6f));
		return i;
	}
	return -1;
}

int Arena::aliveCount() const {
	int n = 0;
	for (const Enemy &e : enemies) if (e.alive && e.type != EN_DUMMY) n++;
	return n;
}

void Arena::heal(float a) {
	if (a <= 0) return;
	hero.hp = fminf(float(mods.maxHp), hero.hp + a);
	g_fx.emit(hero.pos + Vec3(0, 1, 0), 8, UI_P_SPARK, Color(0xA0FFB0), Color(0x40FF80, 0), 2, 0.6f, 0.2f, 0.05f, -2, Vec3(0, 1, 0), 0.6f);
}

void Arena::meteor(Vec3 p) {
	p.y = 0.075f;
	tele(p, 1.2f, 1.2f, Color(0xFF9A3A, 0.55f));
	for (Blast &b : g_blast) if (b.t >= 0.35f) { b = {p, 1.2f, -1.2f, 14, -1, true, true}; break; }
}

float Arena::dps() const {
	int sum = 0;
	float first = time;
	for (int i = 0; i < 64 && i < g_logN; i++)
		if (time - g_log[i].t < 4) { sum += g_log[i].d; first = fminf(first, g_log[i].t); }
	return sum / fmaxf(1, fminf(4, time - first));
}

// ------------------------------------------------------------------ Schaden
static void killEnemy(Enemy &e);

static void damageEnemy(Enemy &e, float dmg, Vec3 from, int god, float pow, int flags, Color c) {
	Arena &a = AR;
	if (!e.alive || e.state == 0 || e.state == 5) return;
	float chance = a.mods.crit + (god == CH_VEXA ? 0.25f * pow : 0) + (a.hero.critNext > 0 ? 1.0f : 0);
	bool crit = g_r.f() < chance;
	if (a.hero.critNext > 0 && !(flags & HF_NONUM)) a.hero.critNext = 0;
	dmg *= a.mods.dmg * a.dmgMulRoom;
	if (god == CH_KORRIN) dmg *= 1 + 0.35f * pow;
	if (a.mods.passive[CH_IGNARA] && e.burnT > 0) dmg *= 1.25f;
	if (a.mods.passive[CH_NERYS] && e.slowT > 0) dmg *= 1.25f;
	if (crit) dmg *= a.mods.critMul + (a.mods.passive[CH_VEXA] ? 0.6f : 0);
	if (e.type == EN_BRUTE && e.state != 2 && e.state != 3) {      // Schild vorne: weniger Schaden von vorne
		Vec3 d = flat(from - e.pos).norm();
		if (d.dot(fwd(e.yaw)) > 0.4f) dmg *= 0.5f;
	}
	int di = int(dmg + 0.5f);
	if (e.type != EN_DUMMY) e.hp -= di;
	e.flash = 1;
	Vec3 dir = flat(e.pos - from);
	dir = dir.len() > 0.01f ? dir.norm() : Vec3(0, 0, -1);
	bool heavyBody = e.type == EN_BRUTE || e.type == EN_BOSS || e.type == EN_QUEEN || e.type == EN_GEODE;
	if (e.type == EN_DUMMY) { e.wvx += dir.x * ((flags & HF_HEAVY) ? 5.5f : 3.2f); e.wvz += dir.z * ((flags & HF_HEAVY) ? 5.5f : 3.2f); }
	else if (!heavyBody) {
		float k = ((flags & HF_HEAVY) ? 6.5f : 3.5f) * (god == CH_NERYS ? 1 + 1.5f * pow : 1);
		e.vel = e.vel + dir * k;
		if (e.state == 1 || e.state == 4) e.stun = 0.18f;
	}
	// Elemente
	switch (god) {
	case CH_IGNARA: e.burnT = 3; e.burnDps = fmaxf(e.burnDps, 6 * pow); break;
	case CH_NERYS: e.slowT = 2.2f; break;
	case CH_SYLV: e.poison = e.poison < 5 ? e.poison + 1 : 5; e.poisonT = 4; break;
	case CH_LYRA: a.heal(1.0f * pow); break;
	case CH_THALOR:
		if (!(flags & HF_NOCHAIN)) {
			int jumps = 0;
			Vec3 last = e.pos;
			for (Enemy &o : a.enemies) {
				if (jumps >= 2 || &o == &e || !o.alive || o.state == 0 || o.state == 5) continue;
				if ((o.pos - last).len() > 4) continue;
				Bolt &b = g_bolt[g_boltN];
				g_boltN = (g_boltN + 1) % 8;
				b = {last + Vec3(0, 0.8f, 0), o.pos + Vec3(0, 0.8f, 0), 0, Color(0xFFF07A)};
				damageEnemy(o, 9 * pow, last, -1, 1, HF_NOSTOP | HF_NOCHAIN, Color(0xFFF07A));
				last = o.pos;
				jumps++;
			}
			if (jumps) sfx(A.cast, 1.6f, 0.4f);
		}
		break;
	}
	// Rückmeldung
	Vec3 hp = e.pos + Vec3(0, 0.75f * e.scale, 0) - dir * 0.25f;
	g_fx.emit(hp, (flags & HF_HEAVY) ? 14 : 8, UI_P_SPARK, Color(0xFFFFFF), c.alpha(0), 7, 0.35f, 0.3f, 0, 4, -dir + Vec3(0, 0.3f, 0), 0.9f);
	g_fx.emit(hp, 1, UI_P_GLOW, c.alpha(0.9f), c.alpha(0), 0, 0.16f, (flags & HF_HEAVY) ? 1.6f : 1.0f, 0.2f);
	if (e.type == EN_DUMMY) g_fx.emit(hp, 5, UI_P_DUST, Color(0xE8C870), Color(0xC8A050, 0), 3.5f, 0.9f, 0.09f, 0.06f, 9, -dir + Vec3(0, 0.8f, 0), 0.7f);
	if (!(flags & HF_NONUM)) {
		Num &n = g_num[g_numN];
		g_numN = (g_numN + 1) % 32;
		n = {hp + Vec3(g_r.range(-0.2f, 0.2f), 0.3f, 0), 0, di, crit, crit ? Color(0xFFD24A) : c};
	}
	if (!(flags & HF_NOSTOP)) {
		g_hitStop = fmaxf(g_hitStop, (flags & HF_HEAVY) ? 0.085f : 0.045f);
		if (S.shake) g_shake = fmaxf(g_shake, (flags & HF_HEAVY) ? 0.3f : 0.13f);
		sfx((flags & HF_HEAVY) ? A.hitHeavy : A.hit, g_r.range(0.9f, 1.15f), 0.85f);
	}
	if (crit) sfx(A.select, 1.6f, 0.4f);
	a.hits++;
	a.total += di;
	a.lastDmg = di;
	g_log[g_logN % 64] = {a.time, di};
	g_logN++;
	a.chain = a.time - g_lastHitT < 1.6f ? a.chain + 1 : 1;
	if (a.chain > a.maxChain) a.maxChain = a.chain;
	g_lastHitT = a.time;
	g_comboT = 0;
	if (e.hp <= 0) killEnemy(e);
}

static void killEnemy(Enemy &e) {
	Arena &a = AR;
	e.state = 5;
	e.st = 0;
	a.kills++;
	a.totalKills++;
	Color c = e.elite ? Color(0xFFD25A) : Color(0xC8A0FF);
	g_fx.emit(e.pos + Vec3(0, 0.6f * e.scale, 0), 22, UI_P_SPARK, Color(0xFFFFFF), c.alpha(0), 6, 0.6f, 0.3f, 0.05f, 3, Vec3(0, 1, 0), 1);
	g_fx.emit(e.pos + Vec3(0, 0.4f, 0), 10, UI_P_WISP, Color(0x6A5AA0, 0.6f), Color(0x2A1A40, 0), 2, 0.8f, 0.4f, 0.9f, -1);
	int coins = (e.type == EN_BRUTE || e.type == EN_GEODE ? 4 : e.type == EN_BOSS || e.type == EN_QUEEN || e.type == EN_GEODELET ? 0 : 1) + (e.elite ? 5 : 0);
	a.coinsGained += int(coins * a.mods.coinMul + 0.5f);
	if (coins) g_fx.emit(e.pos + Vec3(0, 0.6f, 0), coins + 2, UI_P_EMBER, Color(0xFFE070), Color(0xFFB030, 0), 3, 0.7f, 0.12f, 0.05f, 8, Vec3(0, 1, 0), 0.8f);
	sfx(A.hitHeavy, 0.7f, 0.6f);
	if (e.type == EN_GEODE)
		for (int k = 0; k < 3; k++) {
			int id = a.spawn(EN_GEODELET, e.pos + fwd(k * 2.1f) * 0.5f, false, 0);
			if (id >= 0) { a.enemies[id].state = 1; a.enemies[id].st = 0; a.enemies[id].vel = fwd(k * 2.1f) * 4; }
		}
	if (a.mods.passive[CH_SYLV] && e.poison > 0)
		for (Enemy &o : a.enemies)
			if (o.alive && &o != &e && (o.pos - e.pos).len() < 2.6f) { o.poison = o.poison < 5 ? o.poison + 2 : 5; o.poisonT = 4; }
}

static void hurtHero(float dmg, Vec3 from) {
	Arena &a = AR;
	Arena::Hero &h = a.hero;
	if (a.training || h.dead > 0) return;
	if (h.iframe > 0 || h.act == ACT_DASH) return;
#ifndef __3DS__
	static bool godMode = getenv("SKY_GOD") != nullptr;   // nur PC-Tests
	if (godMode) return;
#endif
	if (h.armorT > 0) dmg *= 0.6f;
	dmg *= a.dmgMulRoom;
	h.hp -= dmg;
	h.iframe = 0.85f;
	h.flash = 1;
	g_hurtFlash = 1;
	Vec3 d = flat(h.pos - from);
	d = d.len() > 0.01f ? d.norm() : fwd(h.yaw + PI);
	h.vel = h.vel + d * 6;
	g_hitStop = 0.07f;
	if (S.shake) g_shake = fmaxf(g_shake, 0.4f);
	sfx(A.hitHeavy, 0.75f, 1);
	Num &n = g_num[g_numN];
	g_numN = (g_numN + 1) % 32;
	n = {h.pos + Vec3(0, 1.8f, 0), 0, -int(dmg + 0.5f), false, Color(0xFF5A5A)};
	if (h.hp <= 0) {
		if (a.mods.revives > 0) {
			a.mods.revives--;
			h.hp = a.mods.maxHp * 0.4f;
			h.iframe = 2;
			g_fx.emit(h.pos + Vec3(0, 1, 0), 30, UI_P_SPARK, Color(0xFFFFFF), Color(0xFFE08A, 0), 6, 0.9f, 0.35f, 0, -2, Vec3(0, 1, 0), 1);
			sfx(A.chime, 1, 1);
			ui::toast("Zweiter Atem", "Du stehst wieder auf!", Color(0xFFE08A));
		} else {
			h.hp = 0;
			h.dead = 0.001f;
			sfx(A.hitHeavy, 0.5f, 1);
		}
	}
}

// ------------------------------------------------------------------ Heldin: Aktionen
static void faceAim(Vec2 stick) {
	Arena &a = AR;
	if (stick.len() > 0.3f) { a.hero.yaw = atan2f(stick.x, stick.y); return; }
	float best = 4.0f;
	for (const Enemy &e : a.enemies) {                    // sanfte Zielhilfe
		if (!e.alive || e.state == 0 || e.state == 5) continue;
		Vec3 d = flat(e.pos - a.hero.pos);
		float l = d.len();
		if (l < best && fabsf(angDiff(atan2f(d.x, d.z), a.hero.yaw)) < 1.0f) { best = l; a.hero.yaw = atan2f(d.x, d.z); }
	}
}

static void startAttack(Vec2 stick) {
	Arena::Hero &h = AR.hero;
	int c = h.act == ACT_ATTACK || h.comboWindow > 0 ? (h.combo + 1) % WEAPONS[AR.mods.weapon].hits : 0;
	if (h.act == ACT_ATTACK && h.actT < h.actDur * 0.55f) { h.queued = true; return; }
	faceAim(stick);
	h.act = ACT_ATTACK;
	h.combo = c;
	h.actT = 0;
	h.actDur = SWING_DUR(c) / AR.mods.atkSpeed / (AR.mods.passive[CH_THALOR] ? 1.15f : 1);
	h.hitDone = false;
	h.queued = false;
	const WeaponDef &wd = WEAPONS[AR.mods.weapon];
	sfx(wd.heavy[c] ? A.swing3 : A.swing, g_r.range(0.95f, 1.1f) * (AR.mods.weapon == 1 ? 1.25f : AR.mods.weapon == 2 ? 0.8f : 1), 0.7f);
}

static void hitArc(float range, float half, float dmg, int god, float pow, int flags, Color c) {
	Arena &a = AR;
	for (Enemy &e : a.enemies) {
		if (!e.alive) continue;
		Vec3 d = flat(e.pos - a.hero.pos);
		float l = d.len() - e.r;
		if (l > range) continue;
		if (l > 0.3f && fabsf(angDiff(atan2f(d.x, d.z), a.hero.yaw)) > half) continue;
		damageEnemy(e, dmg, a.hero.pos, god, pow, flags, c);
	}
}

static void hitRadius(Vec3 p, float r, float dmg, int god, float pow, int flags, Color c) {
	for (Enemy &e : AR.enemies)
		if (e.alive && flat(e.pos - p).len() - e.r < r) damageEnemy(e, dmg, p, god, pow, flags, c);
}

static void dashElement(bool start) {
	Arena &a = AR;
	int god = a.mods.slotGod[SL_DASH];
	float pw = a.mods.slotPow[SL_DASH];
	Vec3 p = a.hero.pos;
	Color c(godColor(god));
	switch (god) {
	case CH_IGNARA: if (start) { blast(p, 1.6f, 14 * pw, god, false, true); g_fx.emit(p + Vec3(0, 0.3f, 0), 16, UI_P_EMBER, Color(0xFFE070), Color(0xFF5A1E, 0), 5, 0.5f, 0.25f, 0.05f, -2); } break;
	case CH_THALOR: if (!start) { blast(p, 1.7f, 18 * pw, god, false, true); g_bolt[g_boltN] = {p + Vec3(0, 5, 0), p, 0, Color(0xFFF07A)}; g_boltN = (g_boltN + 1) % 8; } break;
	case CH_NERYS: if (!start) {
		for (Enemy &e : a.enemies) if (e.alive && flat(e.pos - p).len() < 2.2f && e.type != EN_BOSS) { e.vel = e.vel + flat(e.pos - p).norm() * 9; e.slowT = 2; }
		g_fx.emit(p + Vec3(0, 0.2f, 0), 18, UI_P_BUBBLE, Color(0xB0FFFF), Color(0x3CDCD2, 0), 6, 0.5f, 0.2f, 0.1f); } break;
	case CH_VEXA: if (!start) a.hero.critNext = 1.0f; break;
	case CH_KORRIN: if (start) a.hero.armorT = 1.3f * pw; break;
	case CH_LYRA: if (!start) a.heal(2.5f * pw); break;
	case CH_SYLV: if (start) { for (Enemy &e : a.enemies) if (e.alive && flat(e.pos - p).len() < 1.6f) { e.rootT = 1.5f; e.poison = e.poison < 5 ? e.poison + 1 : 5; e.poisonT = 4; }
		g_fx.emit(p + Vec3(0, 0.2f, 0), 12, UI_P_LEAF, Color(0xB0FF80), Color(0x3E8A3A, 0), 3, 0.7f, 0.2f, 0.15f, 2); } break;
	}
	(void)c;
}

// ------------------------------------------------------------------ Gegner-KI
static void updateEnemy(Enemy &e, float dt) {
	Arena &a = AR;
	Arena::Hero &h = a.hero;
	e.st += dt;
	e.flash = fmaxf(0, e.flash - dt * 7);
	e.bob += dt;
	if (e.type == EN_DUMMY) {
		e.wvx += (-60 * e.wobX - 5.5f * e.wvx) * dt;
		e.wvz += (-60 * e.wobZ - 5.5f * e.wvz) * dt;
		e.wobX += e.wvx * dt;
		e.wobZ += e.wvz * dt;
		return;
	}
	if (e.state == 0) { if (e.st >= 0.8f) { e.state = 1; e.st = 0; } return; }
	if (e.state == 5) { if (e.st > 0.45f) e.alive = false; return; }
	// Zustände über Zeit
	if (e.burnT > 0) { e.burnT -= dt; e.hp -= e.burnDps * dt; if (g_r.f() < dt * 8) g_fx.emit(e.pos + Vec3(0, 0.6f, 0), 1, UI_P_EMBER, Color(0xFFD070), Color(0xFF5A1E, 0), 1, 0.6f, 0.1f, 0.02f, -2, Vec3(0, 1, 0), 0.5f); }
	if (e.poisonT > 0) { e.poisonT -= dt; e.hp -= 3 * e.poison * dt; if (g_r.f() < dt * 5) g_fx.emit(e.pos + Vec3(0, 0.5f, 0), 1, UI_P_BUBBLE, Color(0xB0FF70), Color(0x6AA030, 0), 0.8f, 0.7f, 0.1f, 0.05f, -1); }
	else e.poison = 0;
	if (e.hp <= 0) { killEnemy(e); return; }
	e.slowT -= dt;
	e.rootT -= dt;
	e.stun -= dt;
	e.cd -= dt;
	float spd = e.speed * a.speedMulRoom * (e.slowT > 0 ? 0.55f : 1) * (e.rootT > 0 ? 0 : 1) * (e.elite ? 1.1f : 1);
	Vec3 toH = flat(h.pos - e.pos);
	float dist = toH.len();
	Vec3 dirH = dist > 0.01f ? toH * (1 / dist) : Vec3(0, 0, 1);
	Vec3 want;
	auto faceH = [&](float k) { e.yaw += angDiff(atan2f(dirH.x, dirH.z), e.yaw) * fminf(1, dt * k); };
	if (e.stun > 0) { e.vel = e.vel * (1 - fminf(1, dt * 6)); e.pos += e.vel * dt; return; }
	bool heroDown = h.dead > 0;
	switch (e.type) {
	case EN_SHARD:
	case EN_CRAWLER: {
		bool crawl = e.type == EN_CRAWLER;
		float reach = crawl ? 1.0f : 1.7f;
		if (e.state == 1) {
			want = dirH * spd;
			if (crawl) want = want + Vec3(-dirH.z, 0, dirH.x) * (sinf(e.bob * 5) * spd * 0.5f);
			faceH(10);
			if (dist < reach && e.cd <= 0 && !heroDown) { e.state = 2; e.st = 0; e.aim = dirH; }
		} else if (e.state == 2) {
			faceH(14);
			e.aim = dirH;
			if (e.st > (crawl ? 0.28f : 0.45f)) { e.state = 3; e.st = 0; e.hitDone = false; }
		} else if (e.state == 3) {
			want = e.aim * (crawl ? 6.0f : 9.0f);
			if (!e.hitDone && dist < e.r + 0.55f) { e.hitDone = true; hurtHero(e.dmg, e.pos); }
			if (e.st > (crawl ? 0.12f : 0.2f)) { e.state = 4; e.st = 0; }
		} else if (e.state == 4) {
			if (e.st > (crawl ? 0.5f : 0.65f)) { e.state = 1; e.cd = g_r.range(0.4f, 1.0f); }
		}
		break;
	}
	case EN_WISP: {
		if (e.state == 1) {
			if (dist < 4) want = dirH * -spd;
			else if (dist > 6.5f) want = dirH * spd;
			else want = Vec3(-dirH.z, 0, dirH.x) * (spd * 0.6f * (sinf(e.bob * 0.7f) > 0 ? 1 : -1));
			faceH(6);
			if (e.cd <= 0 && dist < 9 && !heroDown) { e.state = 2; e.st = 0; }
		} else if (e.state == 2) {
			faceH(8);
			if (e.st > 0.55f) {
				for (int k = -1; k <= 1; k++) {
					float yy = atan2f(dirH.x, dirH.z) + k * 0.32f;
					for (EProj &p : g_ep) if (p.life <= 0) { p = {e.pos + Vec3(0, 0.9f, 0) + fwd(yy) * 0.4f, fwd(yy) * 6.0f, 2.2f, e.dmg, 0.25f}; break; }
				}
				sfx(A.cast, 0.6f, 0.5f);
				e.state = 4; e.st = 0;
			}
		} else if (e.state == 4) { if (e.st > 0.4f) { e.state = 1; e.cd = g_r.range(1.8f, 2.6f); } }
		break;
	}
	case EN_BOMBER: {
		if (e.state == 1) {
			want = dirH * spd;
			faceH(10);
			if (dist < 1.6f && !heroDown) { e.state = 2; e.st = 0; tele(e.pos, 1.9f, 0.9f, Color(0xFF5A1E, 0.55f)); sfx(A.select, 0.5f, 0.5f); }
		} else if (e.state == 2) {
			if (e.st > 0.9f) {
				blast(e.pos, 1.9f, e.dmg, -1, true, false);
				g_fx.emit(e.pos + Vec3(0, 0.4f, 0), 26, UI_P_EMBER, Color(0xFFE070), Color(0xFF3A1E, 0), 7, 0.7f, 0.3f, 0.05f, 2, Vec3(0, 1, 0), 1);
				g_fx.emit(e.pos + Vec3(0, 0.4f, 0), 1, UI_P_GLOW, Color(0xFF8A3A), Color(0xFF3A1E, 0), 0, 0.3f, 3.5f, 1.0f);
				sfx(A.hitHeavy, 0.55f, 1);
				if (S.shake) g_shake = fmaxf(g_shake, 0.35f);
				e.hp = 0;
				e.state = 5; e.st = 0;
				return;
			}
		}
		break;
	}
	case EN_BRUTE:
	case EN_BOSS: {
		bool boss = e.type == EN_BOSS;
		float reach = boss ? 3.2f : 2.3f;
		if (e.state == 1) {
			want = dirH * spd;
			faceH(4);
			if (dist < reach && e.cd <= 0 && !heroDown) {
				e.state = 2; e.st = 0;
				e.attackN++;
				Vec3 at = e.pos + fwd(e.yaw) * (boss ? 2.0f : 1.3f);
				e.aim = at;
				tele(at, boss ? 2.6f : 1.6f, boss ? 1.0f : 0.85f, Color(0xFF3A3A, 0.5f));
			} else if (boss && e.cd <= 0 && e.attackN % 3 == 2) {        // Boss: Steinregen
				e.attackN++;
				e.cd = 2.5f;
				for (int k = 0; k < 5; k++) {
					Vec3 p = h.pos + Vec3(g_r.range(-2.5f, 2.5f), 0, g_r.range(-2.5f, 2.5f));
					if (k == 0) p = h.pos;
					tele(p, 1.1f, 1.1f + k * 0.15f, Color(0xFF7A3A, 0.5f));
					for (Blast &b : g_blast) if (b.t >= 0.35f) { b = {p, 1.1f, -(1.1f + k * 0.15f), e.dmg * 0.7f, -1, true, false}; break; }
				}
			}
		} else if (e.state == 2) {
			if (e.st > (boss ? 1.0f : 0.85f)) {
				blast(e.aim, boss ? 2.6f : 1.6f, e.dmg, -1, true, false);
				g_fx.emit(e.aim + Vec3(0, 0.1f, 0), 18, UI_P_DUST, Color(0xD8D0C8), Color(0x8A8090, 0), 5, 0.6f, 0.18f, 0.08f, 6, Vec3(0, 1, 0), 1);
				sfx(A.hitHeavy, 0.5f, 1);
				if (S.shake) g_shake = fmaxf(g_shake, boss ? 0.5f : 0.3f);
				e.state = 4; e.st = 0;
				if (boss && e.attackN % 4 == 0) {                          // Boss ruft Krabbler
					for (int k = 0; k < 3; k++) a.spawn(EN_CRAWLER, e.pos + fwd(e.yaw + k * 2.1f) * 2.5f, false, k * 0.2f);
				}
			}
		} else if (e.state == 4) { if (e.st > (boss ? 1.0f : 0.9f)) { e.state = 1; e.cd = boss ? 0.6f : g_r.range(0.5f, 1.2f); } }
		break;
	}
	case EN_SPIDER: {
		if (e.state == 1) {
			want = dirH * spd + Vec3(-dirH.z, 0, dirH.x) * (sinf(e.bob * 3) * spd * 0.4f);
			faceH(10);
			if (dist < 3.8f && dist > 1.2f && e.cd <= 0 && !heroDown) { e.state = 2; e.st = 0; }
		} else if (e.state == 2) {
			faceH(14);
			if (e.st > 0.35f) {
				e.state = 3; e.st = 0; e.hitDone = false;
				e.from = e.pos;
				e.aim = h.pos + flat(h.vel) * 0.25f;
				tele(e.aim, 1.0f, 0.45f, Color(0xFF4A8A, 0.55f));
			}
		} else if (e.state == 3) {
			float u = clampf(e.st / 0.45f, 0, 1);
			Vec3 p = e.from + (e.aim - e.from) * u;
			e.pos.x = p.x; e.pos.z = p.z;
			e.air = sinf(PI * u) * 1.3f;
			e.vel = Vec3();
			if (u >= 1) {
				e.air = 0;
				blast(e.pos, 1.0f, e.dmg, -1, true, false);
				g_fx.emit(e.pos + Vec3(0, 0.1f, 0), 10, UI_P_SPARK, Color(0xB0F0FF), Color(0x8A6AFF, 0), 4, 0.4f, 0.2f, 0.04f, 3, Vec3(0, 1, 0), 1);
				e.state = 4; e.st = 0;
			}
			return;
		} else if (e.state == 4) { if (e.st > 0.7f) { e.state = 1; e.cd = g_r.range(1.0f, 1.8f); } }
		break;
	}
	case EN_ARCHER: {
		if (e.state == 1) {
			if (dist < 4.5f) want = dirH * -spd;
			else if (dist > 7) want = dirH * spd;
			else want = Vec3(-dirH.z, 0, dirH.x) * (spd * 0.5f * (sinf(e.bob * 0.6f) > 0 ? 1 : -1));
			faceH(6);
			if (e.cd <= 0 && dist < 10 && !heroDown) { e.state = 2; e.st = 0; }
		} else if (e.state == 2) {
			if (e.st < 0.55f) { faceH(12); e.aim = fwd(e.yaw); }        // zielen, dann festhalten
			if (e.st > 0.85f) {
				for (EProj &p : g_ep) if (p.life <= 0) { p = {e.pos + Vec3(0, 1.0f, 0) + e.aim * 0.5f, e.aim * 16.0f, 1.0f, e.dmg, 0.2f}; break; }
				sfx(A.cast, 1.4f, 0.5f);
				e.state = 4; e.st = 0;
			}
		} else if (e.state == 4) { if (e.st > 0.5f) { e.state = 1; e.cd = g_r.range(1.6f, 2.4f); } }
		break;
	}
	case EN_GEODE:
	case EN_GEODELET: {
		bool small = e.type == EN_GEODELET;
		if (e.state == 1) {
			want = dirH * spd;
			faceH(small ? 10 : 5);
			if (dist < (small ? 0.95f : 1.6f) && e.cd <= 0 && !heroDown) {
				e.state = 2; e.st = 0;
				if (!small) tele(e.pos + dirH * 0.8f, 1.3f, 0.7f, Color(0xD08AFF, 0.5f));
				e.aim = e.pos + dirH * 0.8f;
			}
		} else if (e.state == 2) {
			if (e.st > (small ? 0.25f : 0.7f)) {
				if (small) { if (dist < e.r + 0.6f) hurtHero(e.dmg, e.pos); }
				else { blast(e.aim, 1.3f, e.dmg, -1, true, false); sfx(A.hitHeavy, 0.7f, 0.6f); }
				e.state = 4; e.st = 0;
			}
		} else if (e.state == 4) { if (e.st > (small ? 0.45f : 0.9f)) { e.state = 1; e.cd = g_r.range(0.4f, 1.0f); } }
		break;
	}
	case EN_QUEEN: {
		e.air = 0.35f + 0.15f * sinf(e.bob * 1.5f);
		if (e.state == 1) {
			if (dist > 4.5f) want = dirH * spd; else if (dist < 2.5f) want = dirH * -spd;
			faceH(4);
			if (e.cd <= 0 && !heroDown) { e.state = 2; e.st = 0; e.attackN++; }
		} else if (e.state == 2) {
			faceH(6);
			int pat = e.attackN % 4;
			if (pat == 0 && e.st > 0.6f) {                                 // Kristallregen
				for (int k = 0; k < 6; k++) {
					Vec3 p = h.pos + Vec3(g_r.range(-2.6f, 2.6f), 0, g_r.range(-2.6f, 2.6f));
					if (k == 0) p = h.pos;
					tele(p, 1.0f, 1.0f + k * 0.12f, Color(0xB08AFF, 0.5f));
					for (Blast &b : g_blast) if (b.t >= 0.35f) { b = {p, 1.0f, -(1.0f + k * 0.12f), e.dmg * 0.8f, -1, true, false}; break; }
				}
				e.state = 4; e.st = 0;
			} else if (pat == 1 && e.st > 0.8f) {                          // Splitterkranz
				float off = g_r.range(0, 0.5f);
				for (int k = 0; k < 14; k++) {
					float yy = off + k * 2 * PI / 14;
					for (EProj &p : g_ep) if (p.life <= 0) { p = {e.pos + Vec3(0, 1.0f, 0) + fwd(yy) * 0.6f, fwd(yy) * 5.5f, 2.5f, e.dmg * 0.6f, 0.22f}; break; }
				}
				sfx(A.cast, 0.7f, 0.7f);
				e.state = 4; e.st = 0;
			} else if (pat == 2 && e.st > 0.7f) {                          // Spinnen rufen
				for (int k = 0; k < 3; k++) a.spawn(EN_SPIDER, e.pos + fwd(e.yaw + k * 2.1f) * 2.2f, false, k * 0.2f);
				e.state = 4; e.st = 0;
			} else if (pat == 3 && e.st > 0.5f) {                          // Sprung durch den Raum
				float aa = g_r.range(0, 2 * PI);
				Vec3 np(sinf(aa) * 2.8f, 0.075f, cosf(aa) * 2.8f);
				g_fx.emit(e.pos + Vec3(0, 1, 0), 20, UI_P_SPARK, Color(0xFFFFFF), Color(0xB08AFF, 0), 5, 0.5f, 0.3f, 0, 0, Vec3(), 1);
				e.pos = np;
				g_fx.emit(np + Vec3(0, 1, 0), 20, UI_P_SPARK, Color(0xFFFFFF), Color(0x8AE8FF, 0), 5, 0.5f, 0.3f, 0, 0, Vec3(), 1);
				sfx(A.dash, 0.6f, 0.6f);
				e.state = 4; e.st = 0;
			}
		} else if (e.state == 4) { if (e.st > 0.8f) { e.state = 1; e.cd = 0.4f; } }
		break;
	}
	}
	e.vel = e.vel + (want - e.vel) * fminf(1, dt * 8);
	e.pos += e.vel * dt;
}

// ------------------------------------------------------------------ Update
void Arena::update(float dt, const Input &in) {
	kills = 0;
	doorEntered = -1;
	pickupTaken = false;
	heroDied = false;
	coinsGained = 0;
	time += dt;
	g_fx.update(dt);
	static float emb;
	emb += dt;
	while (emb > 0.1f) {
		emb -= 0.1f;
		const Obstacle &o = obst[g_r.irange(0, nObst > 0 ? nObst - 1 : 0)];
		if (!o.fire) continue;
		g_fx.emit(o.pos + Vec3(g_r.range(-0.15f, 0.15f), 1.25f, 0), 1, UI_P_EMBER, Color(0xFFD070), Color(0xFF5A1E, 0), 1.2f, 1.4f, 0.09f, 0.02f, -1.2f, Vec3(0, 1, 0), 0.5f);
	}
	for (auto &n : g_num) n.t += dt;
	for (auto &s : g_slash) s.t += dt;
	for (auto &gh : g_ghost) gh.t += dt;
	for (auto &b : g_bolt) b.t += dt;
	for (auto &t : g_tele) t.t += dt;
	g_shake = fmaxf(0, g_shake - dt * 1.6f);
	g_hurtFlash = fmaxf(0, g_hurtFlash - dt * 2.5f);
	if (g_hitStop > 0) { g_hitStop -= dt; return; }
	Hero &h = hero;
	if (h.dead > 0) {
		h.dead += dt;
		h.pose.capeLift = 0.5f;
		if (h.dead > 1.6f) heroDied = true;
		for (Enemy &e : enemies) if (e.alive) updateEnemy(e, dt * 0.3f);
		return;
	}
	Vec2 stick(in.stick.x, in.stick.y);
	if (in.holding(KEY_DLEFT)) stick.x = -1;
	if (in.holding(KEY_DRIGHT)) stick.x = 1;
	if (in.holding(KEY_DUP)) stick.y = -1;
	if (in.holding(KEY_DDOWN)) stick.y = 1;
	if (stick.len() > 1) stick = stick.norm();
	Vec3 wish(stick.x, 0, stick.y);

	h.dashCd = fmaxf(0, h.dashCd - dt);
	h.specialCd = fmaxf(0, h.specialCd - dt);
	h.comboWindow = fmaxf(0, h.comboWindow - dt);
	h.flash = fmaxf(0, h.flash - dt * 5);
	h.iframe = fmaxf(0, h.iframe - dt);
	h.armorT = fmaxf(0, h.armorT - dt);
	h.critNext = fmaxf(0, h.critNext - dt * 0.5f);
	h.afterDash += dt;
	if (h.dashCharges < mods.dashCharges) { h.dashRecharge += dt; if (h.dashRecharge > mods.dashRecharge) { h.dashCharges++; h.dashRecharge = 0; } }
	if (h.castCharges < mods.castCharges) { h.castRecharge += dt; if (h.castRecharge > 2.2f) { h.castCharges++; h.castRecharge = 0; } }

	// Eingaben
	if ((in.pressed(KEY_B) || in.pressed(KEY_R)) && h.act != ACT_DASH && h.dashCd <= 0 && h.dashCharges > 0) {
		h.dashCharges--;
		h.strikeQueued = false;
		h.act = ACT_DASH;
		h.actT = 0;
		h.actDur = 0.2f;
		h.dashDir = wish.len() > 0.2f ? wish.norm() : fwd(h.yaw);
		h.yaw = atan2f(h.dashDir.x, h.dashDir.z);
		h.ghostTimer = 0;
		sfx(A.dash, g_r.range(0.65f, 0.75f), 0.8f);
		g_fx.emit(h.pos + Vec3(0, 0.3f, 0), 12, UI_P_WISP, Color(0x3A2060, 0.85f), Color(0x120820, 0), 2.0f, 0.55f, 0.4f, 0.8f, -1.2f, Vec3(0, 1, 0), 0.9f);
		g_fx.emit(h.pos + Vec3(0, 0.2f, 0), 8, UI_P_SPARK, Color(0xC8A0FF), Color(0x6A3AC0, 0), 3, 0.35f, 0.18f, 0.02f, 0, Vec3(0, 1, 0), 1);
		dashElement(true);
	} else if (h.act == ACT_DASH) {
		if (in.pressed(KEY_A)) h.strikeQueued = true;
	} else if (h.act != ACT_DSTRIKE) {
		if (in.pressed(KEY_A)) {
			if (h.afterDash < 0.12f && h.act == ACT_NONE) { h.strikeQueued = true; h.afterDash = 9; }
			else startAttack(stick);
		}
		if (in.pressed(KEY_X) && h.specialCd <= 0 && h.act != ACT_SPECIAL) {
			h.act = ACT_SPECIAL; h.actT = 0; h.actDur = 0.52f; h.hitDone = false;
			sfx(A.spin, 1, 0.8f);
		}
		if (in.pressed(KEY_Y) && h.castCharges > 0 && h.act != ACT_CAST) {
			faceAim(stick);
			h.act = ACT_CAST; h.actT = 0; h.actDur = 0.3f; h.hitDone = false;
		}
	}

	float moveK = 1;
	if (h.act != ACT_NONE) {
		h.actT += dt;
		float x = clampf(h.actT / h.actDur, 0, 1);
		int ag = mods.slotGod[SL_ATTACK];
		float ap = mods.slotPow[SL_ATTACK];
		Color ac = Color(ag >= 0 ? godColor(ag) : 0xC8F8FF).lighten(0.35f);
		switch (h.act) {
		case ACT_ATTACK: {
			int c = h.combo;
			const WeaponDef &wd = WEAPONS[mods.weapon];
			bool last = c == wd.hits - 1;
			poseFromKeys(SWING[wd.keys[c]], 5, x, h.pose);
			if (wd.leftThrust[c]) h.pose.armSwingL = 1.7f * sinf(x * PI);
			moveK = 0.25f;
			if (x < 0.45f) h.pos += fwd(h.yaw) * ((wd.heavy[c] ? 2.4f : 1.6f) * dt);
			if (!h.hitDone && x >= wd.hitAt[c]) {
				h.hitDone = true;
				float rng = wd.range[c] * (mods.wide ? 1.3f : 1);
				Mat4 base = Mat4::translate(h.pos + Vec3(0, 1.0f, 0)) * Mat4::rotY(h.yaw);
				float ss = wd.slash[c] * (mods.wide ? 1.3f : 1);
				int key = wd.keys[c];
				if (key == 2) spawnSlash(base * Mat4::translate(Vec3(0, 0.1f, 0.15f)) * Mat4::rotZ(PI / 2) * Mat4::scale(Vec3(ss, 1, ss)), 0.2f, ac);
				else spawnSlash(base * Mat4::rotZ(key ? -0.18f : 0.18f) * Mat4::scale(Vec3(key ? -ss : ss, 1, ss)), wd.heavy[c] ? 0.22f : 0.15f, ac);
				if (wd.leftThrust[c]) spawnSlash(base * Mat4::translate(Vec3(-0.2f, 0, 0.2f)) * Mat4::rotZ(0.6f) * Mat4::scale(Vec3(-1.0f, 1, 1.3f)), 0.12f, ac);
				float dm = wd.dmg[c] * (last && mods.finisher ? 1.6f : 1);
				hitArc(rng, wd.half[c], dm, ag, ap, wd.heavy[c] ? HF_HEAVY : 0, ac);
				if (mods.weapon == 2 && c == 1)                          // Sense zieht Feinde heran
					for (Enemy &e : enemies) if (e.alive && e.type != EN_BOSS && e.type != EN_QUEEN && flat(e.pos - h.pos).len() < rng + 0.5f) e.vel = e.vel + flat(h.pos - e.pos).norm() * 3;
				if (last && mods.finisher) { blast(h.pos + fwd(h.yaw) * 1.2f, 1.8f, 16, ag, false, true); }
				if (mods.bladeWave)
					for (HProj &p : g_hp) if (p.life <= 0) { p = {h.pos + Vec3(0, 0.9f, 0) + fwd(h.yaw) * 0.6f, fwd(h.yaw) * 12, 0.38f, dm * 0.5f, 0.6f, HK_WAVE, ag, true, 0, 0}; break; }
				if (last) {
					g_fx.emit(h.pos + fwd(h.yaw) * 1.3f + Vec3(0, 0.05f, 0), 12, UI_P_DUST, Color(0xD8E8FF), ac.alpha(0), 4, 0.5f, 0.12f, 0.05f, 6, Vec3(0, 1, 0), 1.2f);
					if (S.shake) g_shake = fmaxf(g_shake, 0.18f);
				}
			}
			if (x >= 1) {
				h.act = ACT_NONE;
				h.comboWindow = 0.28f;
				if (h.queued || in.holding(KEY_A)) { h.queued = false; startAttack(stick); }
			}
			break;
		}
		case ACT_SPECIAL: {
			float spin = easeIO(x) * 2 * PI;
			h.pose.armYaw = 1.5f; h.pose.armLift = 1.5f; h.pose.weaponBlend = 1; h.pose.bodyTwist = spin;
			moveK = 0.45f;
			int sg = mods.slotGod[SL_SPECIAL];
			Color sc = Color(sg >= 0 ? godColor(sg) : 0xFFE8A0).lighten(0.3f);
			bool second = mods.doubleSpin && !h.hitDone ? false : mods.doubleSpin && x > 0.75f && h.hitDone && h.combo != 9;
			if ((!h.hitDone && x > 0.3f) || second) {
				if (second) h.combo = 9; else h.hitDone = true;
				Mat4 base = Mat4::translate(h.pos + Vec3(0, 0.95f, 0));
				for (int i = 0; i < 3; i++) spawnSlash(base * Mat4::rotY(h.yaw + i * 2.09f + x * 3) * Mat4::scale(Vec3(2.3f, 1, 2.3f)), 0.3f, sc.alpha(0.5f));
				float sp = mods.slotPow[SL_SPECIAL];
				hitRadius(h.pos, 2.4f, 34 * mods.specialMul, sg, sp, HF_HEAVY, sc);
				g_fx.emit(h.pos + Vec3(0, 0.9f, 0), 18, UI_P_SPARK, sc.lighten(0.4f), sc.alpha(0), 8, 0.4f, 0.22f, 0, 0, Vec3(), 1);
				switch (sg) {
				case CH_IGNARA: summon(SM_FIRERING, sg, h.pos, 2.5f, sp); break;
				case CH_THALOR: summon(SM_TORNADO, sg, h.pos + fwd(h.yaw) * 1.2f, 3.0f + sp, sp); break;
				case CH_NERYS:
					for (Enemy &e : enemies) if (e.alive && e.type != EN_BOSS && e.type != EN_QUEEN && flat(e.pos - h.pos).len() < 4.0f) { e.vel = flat(h.pos - e.pos).norm() * 7; e.slowT = 2.5f; }
					g_fx.emit(h.pos + Vec3(0, 0.3f, 0), 24, UI_P_BUBBLE, Color(0xB0FFFF), Color(0x3CDCD2, 0), 6, 0.6f, 0.22f, 0.1f, 0, Vec3(), 1);
					break;
				case CH_VEXA:
					summon(SM_CLONE, sg, h.pos + fwd(h.yaw + PI / 2) * 1.7f, 0.5f, sp, h.yaw);
					summon(SM_CLONE, sg, h.pos + fwd(h.yaw - PI / 2) * 1.7f, 0.5f, sp, h.yaw);
					break;
				case CH_KORRIN: summon(SM_RING, sg, h.pos, 0.45f, sp); if (S.shake) g_shake = fmaxf(g_shake, 0.3f); break;
				case CH_LYRA:
					for (int k = 0; k < 6; k++) shoot(HK_BEAM, sg, h.pos + Vec3(0, 0.9f, 0), fwd(h.yaw + k * PI / 3) * 14, 0.45f, 18 * sp, 0.4f, true);
					break;
				case CH_SYLV:
					for (Enemy &e : enemies) if (e.alive && flat(e.pos - h.pos).len() < 2.6f) { e.rootT = 2.0f; e.poison = e.poison < 5 ? e.poison + 1 : 5; e.poisonT = 4; }
					g_fx.emit(h.pos + Vec3(0, 0.2f, 0), 20, UI_P_LEAF, Color(0xB0FF80), Color(0x3E8A3A, 0), 4, 0.8f, 0.2f, 0.15f, 2, Vec3(), 1);
					break;
				}
			}
			if (x >= 1) { h.act = ACT_NONE; h.specialCd = mods.specialCd; h.pose.bodyTwist = 0; h.combo = 0; }
			break;
		}
		case ACT_CAST: {
			static const AnimKey k[3] = {{0, 0.3f, 2.7f, 1, 0.2f}, {0.45f, 0.1f, 1.25f, 1, -0.15f}, {1, 0.0f, 0.4f, 0.4f, 0}};
			poseFromKeys(k, 3, x, h.pose);
			moveK = 0.3f;
			if (!h.hitDone && x > 0.4f) {
				h.hitDone = true;
				h.castCharges--;
				int n = mods.fanCast ? 3 : 1;
				int cg = mods.slotGod[SL_CAST];
				float cp = mods.slotPow[SL_CAST], cd = 40 * mods.castMul;
				Vec3 hand = h.pos + Vec3(0, 1.15f, 0);
				switch (cg) {
				case CH_THALOR:                                            // Blitz schlägt im nächsten Gegner ein (zielsuchend)
					for (int i = 0; i < n; i++) {
						Enemy *t = nearestEnemy(h.pos, 9);
						Vec3 at = t ? t->pos : h.pos + fwd(h.yaw) * 3;
						g_bolt[g_boltN] = {at + Vec3(0, 6, 0), at + Vec3(0, 0.3f, 0), 0, Color(0xFFF07A)};
						g_boltN = (g_boltN + 1) % 8;
						if (t) damageEnemy(*t, cd * 1.1f * cp, h.pos, CH_THALOR, cp, HF_HEAVY, Color(0xFFF07A));
						blast(at, 1.3f, 12 * cp, CH_THALOR, false, true);
					}
					sfx(A.hitHeavy, 1.4f, 0.8f);
					break;
				case CH_NERYS: for (int i = 0; i < n; i++) shoot(HK_BOUNCE, cg, hand + fwd(h.yaw) * 0.5f, fwd(h.yaw + (i - (n - 1) * 0.5f) * 0.25f) * 15, 1.2f, cd * 0.8f, 0.45f, false, 3); break;
				case CH_VEXA: for (int i = 0; i < 3 * n; i++) shoot(HK_DAGGER, cg, hand + fwd(h.yaw) * 0.4f, fwd(h.yaw + (i - (3 * n - 1) * 0.5f) * 0.35f) * 11, 1.4f, cd * 0.45f * cp, 0.35f, false); break;
				case CH_KORRIN: for (int i = 0; i < n; i++) shoot(HK_BOULDER, cg, h.pos + Vec3(0, 0.55f, 0) + fwd(h.yaw) * 0.7f, fwd(h.yaw + (i - (n - 1) * 0.5f) * 0.3f) * 8.5f, 1.2f, cd * 1.25f * cp, 0.65f, true); break;
				case CH_LYRA: summon(SM_STAR, cg, h.pos, 6 + cp * 2, cp); if (n > 1) summon(SM_STAR, cg, h.pos, 6 + cp * 2, cp, PI); break;
				case CH_SYLV: summon(SM_WOLF, cg, h.pos + fwd(h.yaw) * 0.8f, 8 + cp * 2, cp, h.yaw); break;
				default:
					for (int i = 0; i < n; i++) shoot(HK_SHARD, cg, hand + fwd(h.yaw) * 0.5f, fwd(h.yaw + (i - (n - 1) * 0.5f) * 0.25f) * 17, 0.7f, cd, 0.5f, false);
				}
				sfx(A.cast, g_r.range(0.95f, 1.08f), 0.8f);
			}
			if (x >= 1) h.act = ACT_NONE;
			break;
		}
		case ACT_DASH: {
			h.pos += h.dashDir * (20.0f * dt);
			h.pose.sink = 1.9f * clampf(fminf(h.actT / 0.035f, (h.actDur - h.actT) / 0.045f), 0, 1);
			h.ghostTimer -= dt;
			if (h.ghostTimer <= 0) { h.ghostTimer = 0.035f; g_ghost[g_ghostN] = {h.pos, h.yaw, 0, h.pose}; g_ghostN = (g_ghostN + 1) % 8; }
			h.pose.capeLift = 0.9f;
			if (h.actT >= h.actDur) {
				h.act = ACT_NONE; h.dashCd = 0.1f; h.vel = h.dashDir * 4; h.afterDash = 0; h.pose.sink = 0; dashElement(false);
				g_fx.emit(h.pos + Vec3(0, 0.2f, 0), 12, UI_P_WISP, Color(0x3A2060, 0.8f), Color(0x120820, 0), 2.2f, 0.5f, 0.35f, 0.7f, -1.5f, Vec3(0, 1, 0), 0.9f);
			}
			moveK = 0;
			break;
		}
		case ACT_DSTRIKE: {
			static const AnimKey k[4] = {{0, 1.3f, 1.4f, 1, 0.4f}, {0.35f, -1.4f, 1.35f, 1, -0.5f}, {0.7f, -1.2f, 1.1f, 1, -0.3f}, {1, 0, 0.3f, 0.3f, 0}};
			poseFromKeys(k, 4, x, h.pose);
			moveK = 0;
			if (x < 0.4f) h.pos += fwd(h.yaw) * (9.0f * dt);
			if (!h.hitDone && x >= 0.3f) {
				h.hitDone = true;
				spawnSlash(Mat4::translate(h.pos + Vec3(0, 1.0f, 0)) * Mat4::rotY(h.yaw) * Mat4::scale(Vec3(2.2f, 1, 2.0f)), 0.2f, Color(0xFFFFFF));
				hitArc(2.1f, 1.4f, 30 * mods.dstrikeMul, ag, ap, HF_HEAVY, ac);
			}
			if (x >= 1) { h.act = ACT_NONE; h.comboWindow = 0; }
			break;
		}
		}
	}
	if (h.strikeQueued && h.act == ACT_NONE) {
		h.strikeQueued = false;
		faceAim(stick);
		h.act = ACT_DSTRIKE; h.actT = 0; h.actDur = 0.3f; h.hitDone = false;
		sfx(A.swing3, 1.2f, 0.8f);
	}
	{
		float spd = flat(h.vel).len();
		float want = h.act == ACT_NONE ? clampf(spd / 5.0f, 0, 1) : 0;
		h.runK += (want - h.runK) * fminf(1, dt * 10);
		h.runPhase += dt * (4 + spd * 1.6f);
		bool atk = h.act == ACT_ATTACK || h.act == ACT_DSTRIKE || h.act == ACT_CAST;
		float legT = sinf(h.runPhase) * 0.75f * h.runK + (atk ? 0.38f : 0) + (h.act == ACT_SPECIAL ? 0.25f : 0);
		float armT = sinf(h.runPhase) * 0.6f * h.runK + (atk ? 0.5f : 0) + (h.act == ACT_SPECIAL ? 1.2f : 0);
		h.pose.legSwing += (legT - h.pose.legSwing) * fminf(1, dt * 18);
		h.pose.armSwingL += (armT - h.pose.armSwingL) * fminf(1, dt * 16);
	}
	if (h.act == ACT_NONE) {
		h.pose.armYaw += (-0.15f * h.runK - h.pose.armYaw) * fminf(1, dt * 12);
		h.pose.armLift += (0.05f * sinf(time * 2) + sinf(h.runPhase) * 0.45f * h.runK - h.pose.armLift) * fminf(1, dt * 14);
		h.pose.weaponBlend += (0 - h.pose.weaponBlend) * fminf(1, dt * 10);
		h.pose.bodyTwist += (0 - h.pose.bodyTwist) * fminf(1, dt * 12);
	}
	float maxSpd = 6.4f * mods.move * (mods.passive[CH_THALOR] ? 1.1f : 1);
	Vec3 target = wish * (maxSpd * moveK);
	h.vel = h.vel + (target - h.vel) * fminf(1, dt * 14);
	h.pos += h.vel * dt;
	float sp = h.vel.len();
	if (h.act == ACT_NONE && sp > 0.4f) h.yaw += angDiff(atan2f(h.vel.x, h.vel.z), h.yaw) * fminf(1, dt * 14);
	if (h.act != ACT_DASH) h.pose.capeLift += (clampf(sp / maxSpd, 0, 1) * 0.45f - h.pose.capeLift) * fminf(1, dt * 6);
	if (sp > 1 && h.act == ACT_NONE) {
		h.stepT += dt * sp * 0.55f;
		if (h.stepT > 1) { h.stepT = 0; sfx(A.step, g_r.range(0.8f, 1.2f), 0.6f); g_fx.emit(h.pos, 2, UI_P_WISP, Color(0xD8D0C0, 0.4f), Color(0xD8D0C0, 0), 0.8f, 0.4f, 0.2f, 0.35f); }
	}

	// Gegner
	for (Enemy &e : enemies) if (e.alive) updateEnemy(e, dt);
	// Kollisionen: Gegner untereinander, mit Heldin, Hindernissen, Rand
	for (int i = 0; i < 24; i++) {
		Enemy &a = enemies[i];
		if (!a.alive || a.state == 5) continue;
		for (int j = i + 1; j < 24; j++) {
			Enemy &b = enemies[j];
			if (!b.alive || b.state == 5) continue;
			Vec3 d = flat(b.pos - a.pos);
			float l = d.len(), m = a.r + b.r;
			if (l < m && l > 1e-4f) { Vec3 push = d * ((m - l) / l * 0.5f); a.pos = a.pos - push; b.pos = b.pos + push; }
		}
		if (a.state != 0) {
			Vec3 d = flat(h.pos - a.pos);
			float l = d.len(), m = a.r + 0.3f;
			if (l < m && l > 1e-4f && h.act != ACT_DASH) {
				Vec3 push = d * ((m - l) / l);
				if (a.type == EN_DUMMY || a.type == EN_BRUTE || a.type == EN_BOSS) h.pos += push;
				else { h.pos += push * 0.5f; a.pos = a.pos - push * 0.5f; }
			}
		}
	}
	auto bound = [&](Vec3 &p, float r) {
		if (gridMode) grid.collide(p, r * 0.9f);
		else {
			Vec3 f = flat(p);
			float lim = radius - r + 0.3f;
			if (f.len() > lim) { f = f * (lim / f.len()); p.x = f.x; p.z = f.z; }
		}
		for (int k = 0; k < nObst; k++) {
			const Obstacle &o = obst[k];
			Vec3 d = flat(p - o.pos);
			float l = d.len(), m = o.r + r;
			if (l < m && l > 1e-4f) { p.x = o.pos.x + d.x / l * m; p.z = o.pos.z + d.z / l * m; }
		}
	};
	bound(h.pos, 0.3f);
	h.pos.y = 0.075f;
	for (Enemy &e : enemies) if (e.alive) { bound(e.pos, e.r); e.pos.y = 0.075f; }

	// Projektile der Heldin
	for (HProj &p : g_hp) {
		if (p.life <= 0) continue;
		p.life -= dt;
		if (p.kind == HK_DAGGER) {
			Enemy *t = nearestEnemy(p.p, 6);
			if (t) { Vec3 want = flat(t->pos - p.p).norm() * 11; p.v = p.v + (want - p.v) * fminf(1, dt * 7); }
		}
		p.p += p.v * dt;
		Color pc(p.god >= 0 ? godColor(p.god) : 0x6AD8FF);
		if (g_r.f() < 0.6f) g_fx.emit(p.p, 1, p.kind == HK_BOULDER ? UI_P_DUST : UI_P_SPARK, pc.lighten(0.6f).alpha(0.9f), pc.alpha(0), 0.5f, 0.3f, p.kind == HK_WAVE ? 0.3f : 0.16f, 0.02f);
		for (int i = 0; i < 24; i++) {
			Enemy &e = enemies[i];
			if (!e.alive || e.state == 0 || e.state == 5 || (p.hitMask & (1u << i))) continue;
			if (flat(p.p - e.pos).len() < e.r + p.r) {
				p.hitMask |= 1u << i;
				damageEnemy(e, p.dmg, p.p - p.v * 0.1f, p.god, mods.slotPow[SL_CAST], p.kind == HK_SHARD || p.kind == HK_BOULDER ? 0 : HF_NOSTOP, pc);
				if (p.god == CH_IGNARA && p.kind == HK_SHARD) blast(p.p, 1.5f, 18 * mods.slotPow[SL_CAST], CH_IGNARA, false, true);
				if (p.kind == HK_BOULDER && e.type != EN_BOSS && e.type != EN_QUEEN) e.vel = e.vel + flat(p.v).norm() * 6;
				if (p.kind == HK_BOUNCE && p.bounces > 0) {
					Enemy *nt = nearestEnemy(p.p, 5.5f, &e);
					if (nt) { p.bounces--; p.v = flat(nt->pos - p.p).norm() * 15; p.hitMask = 1u << i; p.life = 1.0f; sfx(A.select, 1.5f, 0.3f); break; }
				}
				if (!p.pierce) { g_fx.emit(p.p, 14, UI_P_SPARK, Color(0xFFFFFF), pc.alpha(0), 6, 0.4f, 0.26f, 0); p.life = 0; break; }
			}
		}
		if (gridMode ? !grid.floorAt(p.p.x, p.p.z) : flat(p.p).len() > radius + 1.5f) { if (p.life > 0) g_fx.emit(p.p, 6, UI_P_SPARK, Color(0xFFFFFF), Color(0x8A8098, 0), 3, 0.3f, 0.15f, 0.02f); p.life = 0; }
	}
	// Beschwörungen
	for (Summon &m : g_sum) {
		if (!m.active) continue;
		m.t += dt;
		m.cd -= dt;
		if (m.t >= m.dur) { m.active = false; continue; }
		Color mc(godColor(m.god));
		switch (m.kind) {
		case SM_TORNADO: {
			Enemy *t = nearestEnemy(m.p, 8);
			Vec3 want = t ? flat(t->pos - m.p).norm() * 3.2f : Vec3();
			m.v = m.v + (want - m.v) * fminf(1, dt * 3);
			m.p += m.v * dt;
			if (m.cd <= 0) {
				m.cd = 0.3f;
				for (Enemy &e : enemies)
					if (e.alive && e.state != 0 && e.state != 5 && flat(e.pos - m.p).len() < 1.5f) {
						damageEnemy(e, 7 * m.pow, m.p, CH_THALOR, m.pow, HF_NOSTOP | HF_NOCHAIN, mc);
						if (e.type != EN_BOSS && e.type != EN_QUEEN && e.type != EN_BRUTE) e.vel = e.vel + flat(m.p - e.pos).norm() * 3;
					}
			}
			if (g_r.f() < 0.7f) g_fx.emit(m.p + Vec3(g_r.range(-0.4f, 0.4f), g_r.range(0.2f, 1.8f), g_r.range(-0.4f, 0.4f)), 1, UI_P_WISP, Color(0xD8E8FF, 0.5f), Color(0x8AA8FF, 0), 1.5f, 0.4f, 0.25f, 0.4f, -1);
			break;
		}
		case SM_WOLF: {
			Enemy *t = nearestEnemy(m.p, 12);
			if (t) {
				Vec3 d = flat(t->pos - m.p);
				m.yaw = atan2f(d.x, d.z);
				if (d.len() > t->r + 0.5f) m.p += d.norm() * (6.5f * dt);
				else if (m.cd <= 0) { m.cd = 0.55f; damageEnemy(*t, 14 * m.pow, m.p, CH_SYLV, m.pow, HF_NOSTOP, Color(0x9AFFC0)); }
			} else {
				Vec3 d = flat(h.pos + fwd(h.yaw + 2.2f) * 1.2f - m.p);
				if (d.len() > 0.3f) { m.p += d.norm() * (5.5f * dt); m.yaw = atan2f(d.x, d.z); }
			}
			break;
		}
		case SM_STAR: {
			float a = m.t * 4 + m.yaw;
			m.p = h.pos + Vec3(cosf(a) * 1.6f, 1.0f, sinf(a) * 1.6f);
			if (m.cd <= 0)
				for (Enemy &e : enemies)
					if (e.alive && e.state != 0 && e.state != 5 && flat(e.pos - m.p).len() < e.r + 0.45f) { damageEnemy(e, 12 * m.pow, m.p, CH_LYRA, m.pow, HF_NOSTOP, mc); m.cd = 0.18f; break; }
			break;
		}
		case SM_CLONE:
			if (m.cd > -1 && m.t > 0.22f) { m.cd = -9; hitRadius(m.p, 1.9f, 26 * m.pow, CH_VEXA, m.pow, HF_NOSTOP, mc); g_fx.emit(m.p + Vec3(0, 0.9f, 0), 12, UI_P_WISP, Color(0x6A3AA0, 0.7f), Color(0x2A1040, 0), 3, 0.4f, 0.3f, 0.5f); }
			break;
		case SM_FIRERING:
			if (m.cd <= 0) { m.cd = 0.4f; hitRadius(m.p, 2.4f, 6 * m.pow, CH_IGNARA, m.pow, HF_NOSTOP | HF_NONUM, mc); }
			if (g_r.f() < 0.8f) { float a = g_r.range(0, 6.28f), r = g_r.range(0.3f, 2.4f); g_fx.emit(m.p + Vec3(cosf(a) * r, 0.15f, sinf(a) * r), 1, UI_P_EMBER, Color(0xFFD070), Color(0xFF4A1E, 0), 1.0f, 0.6f, 0.12f, 0.03f, -2, Vec3(0, 1, 0), 0.4f); }
			break;
		case SM_RING: {
			float r = 0.5f + m.t / m.dur * 3.8f;
			for (int i = 0; i < 24; i++) {
				Enemy &e = enemies[i];
				if (!e.alive || (m.hitMask & (1u << i))) continue;
				float d = flat(e.pos - m.p).len();
				if (fabsf(d - r) < e.r + 0.35f) { m.hitMask |= 1u << i; damageEnemy(e, 30 * m.pow, m.p, CH_KORRIN, m.pow, HF_NOSTOP, mc); }
			}
			break;
		}
		}
	}
	// Projektile der Gegner
	for (EProj &p : g_ep) {
		if (p.life <= 0) continue;
		p.life -= dt;
		p.p += p.v * dt;
		if (g_r.f() < 0.5f) g_fx.emit(p.p, 1, UI_P_SPARK, Color(0xB0FFF8, 0.8f), Color(0x3AA8C0, 0), 0.3f, 0.3f, 0.14f, 0.02f);
		if (gridMode && !grid.floorAt(p.p.x, p.p.z)) { p.life = 0; continue; }
		if (flat(p.p - h.pos).len() < 0.35f + p.r && h.dead <= 0) {
			if (h.act == ACT_DASH || h.iframe > 0) continue;
			hurtHero(p.dmg, p.p - p.v * 0.1f);
			p.life = 0;
		}
	}
	// Explosionen (verzögert: t < 0)
	for (Blast &b : g_blast) {
		if (b.t >= 0.35f) continue;
		float before = b.t;
		b.t += dt;
		if (before < 0 || (before >= 0 && before > 0.0001f)) { if (!(before < 0 && b.t >= 0)) continue; }
		// Auslösen genau einmal (t überschreitet 0 oder neu bei 0)
		if (b.hurtsHero && flat(h.pos - b.p).len() < b.r + 0.2f) hurtHero(b.dmg, b.p);
		if (b.hurtsEnemies) hitRadius(b.p, b.r, b.dmg, b.god, 1, HF_NOSTOP | HF_NOCHAIN, Color(godColor(b.god)));
		g_fx.emit(b.p + Vec3(0, 0.2f, 0), 14, b.hurtsHero ? UI_P_DUST : UI_P_SPARK, b.hurtsHero ? Color(0xE8C8B0) : Color(godColor(b.god)).lighten(0.5f), Color(0x806070, 0), 5, 0.5f, 0.2f, 0.05f, 4, Vec3(0, 1, 0), 1);
		if (b.hurtsHero) { sfx(A.hitHeavy, 0.6f, 0.5f); if (S.shake) g_shake = fmaxf(g_shake, 0.2f); }
	}
	// Belohnung und Türen
	if (pickup.active) {
		float pb = pickup.t;
		pickup.t += dt;
		if (pb < 0.5f && pickup.t >= 0.5f && pickup.kind < PK2_SHOP) {
			g_fx.emit(pickup.pos + Vec3(0, 0.6f, 0), 20, UI_P_SPARK, Color(0xFFFFFF), Color(pickup.kind == PK2_BOON ? godColor(pickup.god) : 0xFFE08A, 0), 5, 0.6f, 0.25f, 0.03f, 1, Vec3(0, 1, 0), 1);
			sfx(A.select, 1.3f, 0.6f);
		}
		if (flat(h.pos - pickup.pos).len() < (pickup.kind >= PK2_SHOP ? 1.25f : 0.8f) && pickup.t > 0.4f) { pickup.active = false; pickupTaken = true; sfx(A.chime, 1.1f, 0.8f); }
	}
	for (int i = 0; i < 3; i++) {
		Door &d = doors[i];
		if (!d.active) continue;
		float before = d.t;
		d.t += dt;
		if (before < 0.05f && d.t >= 0.05f) g_fx.emit(d.pos + Vec3(0, 0.1f, 0), 16, UI_P_DUST, Color(0xD8D0E0), Color(0x8A8098, 0), 3, 0.8f, 0.2f, 0.1f, 2, Vec3(0, 1, 0), 1.2f);
		if (before < 0.7f && d.t >= 0.7f) { sfx(A.hitHeavy, 0.6f, 0.4f); if (S.shake) g_shake = fmaxf(g_shake, 0.12f); }
		if (d.t > 0.75f && !pickup.active && flat(h.pos - d.pos).len() < 1.0f) doorEntered = i;
	}
	g_comboT += dt;
	if (time - g_lastHitT > 1.6f) chain = 0;
	{
		Vec3 want = Vec3(h.pos.x * 0.7f, 0, h.pos.z * 0.6f - 0.4f);
		if (gridMode) want = Vec3(clampf(h.pos.x, grid.minX + 2.5f, grid.maxX - 2.5f), 0, clampf(h.pos.z - 0.5f, grid.minZ + 1.0f, grid.maxZ - 1.5f));
		if (gridMode && grid.maxX - grid.minX < 5) want.x = (grid.minX + grid.maxX) * 0.5f;
		g_camFocus = g_camFocus + (want - g_camFocus) * fminf(1, dt * 5);
	}
}

// ------------------------------------------------------------------ Zeichnen
static int pickIcon(int kind, int god) {
	switch (kind) {
	case PK2_BOON: return UI_SIGIL_SEREN + (god >= 0 ? god : 0);
	case PK2_COINS: return UI_ICON_COIN;
	case PK2_HEART: return UI_ICON_HEART;
	case PK2_ANVIL: return UI_ICON_ANVIL;
	case PK2_GEM: return UI_ICON_GEM;
	case PK2_POWER: return UI_ICON_POWER;
	case PK2_CHEST: return UI_ICON_CHEST;
	case PK2_SHOP: return UI_ICON_SHOP;
	case PK2_FOUNTAIN: return UI_ICON_FOUNTAIN;
	case PK2_EVENT: return UI_ICON_EVENT;
	}
	return UI_ICON_EVENT;
}
static const int ROOM_ICON[RT_COUNT] = {-1, UI_ICON_ELITE, UI_ICON_CHEST, UI_ICON_SHOP, UI_ICON_FOUNTAIN, UI_ICON_EVENT, UI_ICON_CHALLENGE, UI_ICON_BOSS};

static void drawEnemy(const Enemy &e, float t) {
	const EDef &d = EDEFS[e.type];
	Material m;
	m.specular = 0.25f;
	float fl = e.flash;
	if (e.state == 2) fl = fmaxf(fl, 0.35f + 0.35f * sinf(t * 40));
	if (fl > 0) m.emission = Color(0x000000).mix(e.state == 2 && e.flash < 0.1f ? Color(0xFF4A3A) : Color(0xFFFFFF), clampf(fl, 0, 1));
	else if (e.elite) m.emission = Color(0x3A2A00);
	float s = e.scale;
	float rise = 0, k = 1;
	if (e.state == 0) { float u = clampf(e.st / 0.8f, 0, 1); rise = -(1 - u) * 1.2f; k = 0.3f + 0.7f * u; }
	if (e.state == 5) { float u = clampf(e.st / 0.45f, 0, 1); k = 1 - u; rise = -u * 0.4f; m.emission = Color(0x000000).mix(Color(0xFFFFFF), 1 - u); }
	float hover = (e.type == EN_WISP ? 0.25f + 0.12f * sinf(e.bob * 2.5f) : 0) + e.air;
	float squash = (e.state == 2 ? 0.92f : 1) * (e.type == EN_CRAWLER || e.type == EN_BOMBER ? 1 + 0.06f * sinf(e.bob * 18) : 1 + 0.02f * sinf(e.bob * 3));
	float lean = e.state == 3 ? 0.35f : e.state == 2 ? -0.18f : 0;
	float moving = clampf(Vec3(e.vel.x, 0, e.vel.z).len() / 2.5f, 0, 1);
	float roll = 0;
	if (e.type == EN_CRAWLER || e.type == EN_BOMBER) roll = sinf(e.bob * 22) * 0.09f * moving;
	else if (e.type == EN_SHARD) { roll = sinf(e.bob * 9) * 0.07f * moving; rise += fabsf(sinf(e.bob * 9)) * 0.06f * moving; }
	else if (e.type == EN_BRUTE || e.type == EN_BOSS) roll = sinf(e.bob * 5) * 0.05f * moving;
	float hitSq = e.flash > 0.3f && e.state != 2 ? (e.flash - 0.3f) * 0.25f : 0;
	float spinDie = e.state == 5 ? clampf(e.st / 0.45f, 0, 1) * 4.0f : 0;
	if (e.type == EN_BRUTE || e.type == EN_BOSS) lean = e.state == 2 ? -0.3f * clampf(e.st * 2, 0, 1) : e.state == 4 && e.st < 0.2f ? 0.4f : 0;
	Mat4 xf = Mat4::translate(e.pos + Vec3(0, rise + hover, 0)) * Mat4::rotY(e.yaw + spinDie) * Mat4::rotX(lean) * Mat4::rotZ(roll) *
		Mat4::scale(Vec3(s * k * (1 + hitSq), s * k * squash * (1 - hitSq), s * k * (1 + hitSq)));
	if (e.type == EN_DUMMY) {
		gfx3d::draw(A.world[WM_DUMMY_BASE], Mat4::translate(e.pos) * Mat4::rotY(0.3f), m);
		gfx3d::draw(A.world[WM_DUMMY_TOP], Mat4::translate(e.pos + Vec3(0, 0.5f, 0)) * Mat4::rotX(e.wobZ) * Mat4::rotZ(-e.wobX) * Mat4::rotY(0.3f), m);
		return;
	}
	if (e.type == EN_BOSS) m.emission = fl > 0 ? m.emission : Color(0x200808);
	if (e.type == EN_QUEEN && fl <= 0) m.emission = Color(0x10102A);
	gfx3d::draw(A.world[d.body], xf, m);
	if (d.glow >= 0) {
		Material g;
		g.lit = false;
		if (e.type == EN_BOSS) g.color = Color(0xFF6A3A);
		gfx3d::draw(A.world[d.glow], xf, g);
	}
}

void Arena::draw(Gfx &g) {
	float t = time;
	g.rectGrad(0, 0, 400, 240, Color(biome == 1 ? 0x03050C : 0x040308), Color(biome == 1 ? 0x0A1020 : 0x100C18), 10.5f);
	Hero &h = hero;
	Vec3 sh;
	if (g_shake > 0) sh = Vec3(g_r.range(-1, 1), g_r.range(-1, 1), g_r.range(-1, 1)) * (g_shake * 0.16f);
	cam.target = g_camFocus + Vec3(0, 0.6f, 0) + sh;
	cam.eye = g_camFocus + Vec3(0, 6.9f, 6.3f) + sh;
	cam.fov = 42;
	cam.focus = (cam.eye - cam.target).len();
	cam.zfar = 300;
	Scene3D sc;
	sc.ambient = Color(0x3C3648);
	Light3D sun; sun.pos = Vec3(0.3f, 1.0f, 0.5f); sun.color = Color(0xB8B0D8); sun.intensity = 0.42f;
	sc.lights[sc.nLights++] = sun;
	{                                                                  // zwei nächste Wandfackeln
		float best[2] = {1e9f, 1e9f};
		Vec3 bp[2];
		int nt = gridMode ? grid.nTorch : 28;
		for (int k = 0; k < nt; k += gridMode ? 1 : 3) {
			Vec3 p;
			if (gridMode) p = grid.torch[k];
			else {
				float a0 = 2 * PI * k / 28, am = 2 * PI * (k + 0.5f) / 28;
				if (cosf(am) > 0.35f) continue;
				p = Vec3(sinf(a0) * 4.83f, 2.3f, cosf(a0) * 4.83f);
			}
			float d = (p - h.pos).len();
			if (d < best[0]) { best[1] = best[0]; bp[1] = bp[0]; best[0] = d; bp[0] = p; }
			else if (d < best[1]) { best[1] = d; bp[1] = p; }
		}
		for (int i = 0; i < 2; i++) {
			Light3D l; l.point = true; l.pos = bp[i]; l.range = 5.5f;
			if (biome == 1) { l.color = Color(i ? 0xB08AFF : 0x6AD8FF); l.intensity = 1.1f + 0.2f * sinf(t * 1.3f + i * 2); l.pos.y = 1.0f; }
			else { l.color = Color(0xFFA050); l.intensity = 1.2f + 0.12f * sinf(t * 11 + i * 3); }
			sc.lights[sc.nLights++] = l;
		}
	}
	Light3D star; star.point = true; star.pos = h.pos + Vec3(0.1f, 1.25f, 0.2f); star.color = Color(0x8AF0FF); star.intensity = 0.6f; star.range = 1.6f;
	sc.lights[sc.nLights++] = star;
	for (int k = 0; k < nObst && sc.nLights < 6; k++)
		if (obst[k].fire) { Light3D f; f.point = true; f.pos = obst[k].pos + Vec3(0, 1.35f, 0); f.color = Color(0xFF9A4A); f.intensity = 1.1f + 0.15f * sinf(t * 9 + k); f.range = 4.2f; sc.lights[sc.nLights++] = f; }
	for (const HProj &p : g_hp)
		if (p.life > 0 && sc.nLights < 8) { Light3D l; l.point = true; l.pos = p.p; l.color = Color(p.god >= 0 ? godColor(p.god) : 0x6AD8FF); l.intensity = 1.5f; l.range = 3; sc.lights[sc.nLights++] = l; }
	sc.fog = true;
	sc.fogColor = Color(biome == 1 ? 0x060A16 : 0x08060E);
	sc.fogNear = 9; sc.fogFar = 26;
	gfx3d::begin(g, cam, sc);
	Material lit;
	lit.specular = 0.08f;
	Material al = lit;
	al.specular = 0.3f;
	al.color = floorTint;
	if (gridMode) {
		gfx3d::draw(roomMesh, Mat4::identity(), al);
		Material rg; rg.lit = false;
		rg.color = biome == 1 ? Color(0xFFFFFF).mix(Color(0xC8D8FF), 0.5f + 0.5f * sinf(t * 1.2f)) : Color(0xFFFFFF).mix(Color(0xFFD0A0), 0.5f + 0.5f * sinf(t * 13));
		gfx3d::draw(roomGlow, Mat4::identity(), rg);
	} else if (biome == 1) {
		gfx3d::draw(A.world[WM_ARENA_CAVE], Mat4::identity(), al);
		gfx3d::draw(A.world[WM_CAVE], Mat4::identity(), lit);
		Material cg; cg.lit = false; cg.color = Color(0xFFFFFF).mix(Color(0xC8D8FF), 0.5f + 0.5f * sinf(t * 1.2f));
		gfx3d::draw(A.world[WM_CAVE_GLOW], Mat4::identity(), cg);
	} else {
		gfx3d::draw(A.world[WM_ARENA_DARK], Mat4::identity(), al);
		gfx3d::draw(A.world[WM_DWALL], Mat4::identity(), lit);
		Material wf; wf.lit = false;
		float fl = 1 + 0.07f * sinf(t * 14) + 0.04f * sinf(t * 31);
		gfx3d::draw(A.world[WM_DWALL_FIRE], Mat4::translate(Vec3(0, 2.18f, 0)) * Mat4::scale(Vec3(1, fl, 1)) * Mat4::translate(Vec3(0, -2.18f, 0)), wf);
	}
	Material ag; ag.lit = false; ag.color = Color(0xFFFFFF).mix(Color(0x8AF0FF), 0.5f + 0.5f * sinf(t * 1.5f));
	if (!gridMode) gfx3d::draw(A.world[WM_ARENA_GLOW], Mat4::identity(), ag);
	for (int k = 0; k < nObst; k++) {
		const Obstacle &o = obst[k];
		Mat4 bm = Mat4::translate(o.pos) * Mat4::rotY(o.yaw) * Mat4::scale(Vec3(o.scale, o.scale, o.scale));
		gfx3d::draw(A.world[o.mesh], bm, lit);
		if (o.fire) {
			Material fm; fm.lit = false;
			float fl = 1 + 0.08f * sinf(t * 13 + k) + 0.05f * sinf(t * 29);
			gfx3d::draw(A.world[WM_BRAZIER_FIRE], bm * Mat4::translate(Vec3(0, 1.1f, 0)) * Mat4::scale(Vec3(1, fl, 1)) * Mat4::translate(Vec3(0, -1.1f, 0)), fm);
		}
	}
	if (hub) {                                                         // Zuflucht: Teppich, Regale, Banner, Tor, Altar, Waffen
		gfx3d::draw(A.world[WM_R_HUBDECOR], Mat4::identity(), lit);
		Mat4 gm = Mat4::translate(Vec3(0, 0.075f, -4.6f)) * Mat4::scale(Vec3(1.3f, 1.3f, 1.3f));
		gfx3d::draw(A.world[WM_R_DOOR], gm, lit);
		Material pm; pm.lit = false; pm.color = Color(0x8AE8FF, 0.7f + 0.15f * sinf(t * 1.3f));
		gfx3d::draw(A.world[WM_R_PORTAL], gm, pm);
		Material ag2; ag2.lit = false;
		Mat4 am = Mat4::translate(Vec3(-2.7f, 0.07f + 0.06f * sinf(t * 1.5f), -2.5f));
		gfx3d::draw(A.world[WM_R_ALTAR_GLOW], am * Mat4::translate(Vec3(0, 1.75f, 0)) * Mat4::rotY(t * 0.8f) * Mat4::translate(Vec3(0, -1.75f, 0)), ag2);
		Mat4 rm = Mat4::translate(Vec3(2.9f, 0.07f, -2.8f)) * Mat4::rotY(-0.75f);
		Material wm; wm.specular = 0.7f; wm.shininess = 30;
		gfx3d::draw(CM[CH_SEREN].part[PK_WEAPON], rm * Mat4::translate(Vec3(-0.45f, 1.28f, 0.1f)) * Mat4::rotX(PI), wm);
		gfx3d::draw(A.world[WM_R_DAGGER], rm * Mat4::translate(Vec3(-0.05f, 1.28f, 0.1f)) * Mat4::rotX(PI), wm);
		gfx3d::draw(A.world[WM_R_DAGGER], rm * Mat4::translate(Vec3(0.05f, 1.28f, 0.1f)) * Mat4::rotX(PI), wm);
		gfx3d::draw(A.world[WM_R_SCYTHE], rm * Mat4::translate(Vec3(0.45f, 0.55f, 0.1f)), wm);
		gfx3d::draw(A.world[WM_R_SCYTHE_GLOW], rm * Mat4::translate(Vec3(0.45f, 0.55f, 0.1f)), ag2);
	}
	// Türen
	for (const Door &d : doors) {
		if (!d.active) continue;
		float up = clampf(d.t / 0.7f, 0, 1);
		up = 1 - (1 - up) * (1 - up) * (1 - up);
		float shakeD = d.t < 0.7f ? sinf(d.t * 70) * 0.03f : 0;
		Mat4 dm = Mat4::translate(d.pos + Vec3(shakeD, -(1 - up) * 2.8f, 0)) * Mat4::rotY(d.yaw) * Mat4::scale(Vec3(0.8f, 0.8f, 0.8f));
		gfx3d::draw(A.world[WM_R_DOOR], dm, lit);
		Material pm; pm.lit = false;
		Color pc(d.reward == PK2_BOON ? godColor(d.god) : d.room == RT_SHOP ? 0xFFD25A : d.room == RT_BOSS ? 0xFF4A3A : 0xB0D8FF);
		pm.color = pc.alpha((pickup.active ? 0.15f : 0.55f + 0.2f * sinf(t * 3)) * clampf((d.t - 0.5f) * 3, 0, 1));
		gfx3d::draw(A.world[WM_R_PORTAL], dm, pm);
	}
	// Belohnung
	if (pickup.active) {
		float dropK = clampf(pickup.t / 0.5f, 0, 1);
		float drop = (1 - dropK) * (1 - dropK) * 4;
		Vec3 pp = pickup.pos + Vec3(0, 0.6f + 0.12f * sinf(t * 2.5f) + drop, 0);
		Mat4 pm = Mat4::translate(pp) * Mat4::rotY(t * 1.5f);
		Material g2; g2.lit = false;
		switch (pickup.kind) {
		case PK2_COINS: gfx3d::draw(A.world[WM_R_COINS], Mat4::translate(pickup.pos + Vec3(0, 0.08f + drop, 0)) * Mat4::scale(Vec3(1.6f, 1.6f, 1.6f)), lit); break;
		case PK2_HEART: gfx3d::draw(A.world[WM_R_HEART], pm * Mat4::scale(Vec3(1.6f, 1.6f, 1.6f)), g2); break;
		case PK2_ANVIL: gfx3d::draw(A.world[WM_R_ANVIL], Mat4::translate(pickup.pos + Vec3(0, 0.08f + drop, 0)) * Mat4::rotY(0.4f), lit); break;
		case PK2_CHEST: gfx3d::draw(A.world[WM_R_CHEST], Mat4::translate(pickup.pos + Vec3(0, 0.08f + drop, 0)), lit); break;
		case PK2_GEM: case PK2_POWER: gfx3d::draw(A.world[WM_R_GEM], pm * Mat4::scale(Vec3(1.5f, 1.5f, 1.5f)), g2); break;
		case PK2_BOON: g2.color = Color(godColor(pickup.god)); gfx3d::draw(A.shard, pm * Mat4::scale(Vec3(2.4f, 2.4f, 2.4f)), g2); break;
		case PK2_SHOP:
			gfx3d::draw(A.world[WM_R_CHEST], Mat4::translate(pickup.pos + Vec3(-0.3f, 0, 0)) * Mat4::rotY(0.3f), lit);
			gfx3d::draw(A.world[WM_R_COINS], Mat4::translate(pickup.pos + Vec3(0.45f, 0, 0.2f)) * Mat4::scale(Vec3(1.8f, 1.8f, 1.8f)), lit);
			break;
		case PK2_FOUNTAIN: {
			gfx3d::draw(A.world[WM_R_FOUNTAIN], Mat4::translate(pickup.pos), lit);
			Material w; w.lit = false; w.color = Color(0xFFFFFF, 0.8f + 0.2f * sinf(t * 3));
			gfx3d::draw(A.world[WM_R_WATER], Mat4::translate(pickup.pos), w);
			break;
		}
		case PK2_EVENT: gfx3d::draw(A.world[WM_R_SHRINE], Mat4::translate(pickup.pos), lit); break;
		}
	}
	// Bodenwarnungen
	for (const Tele &tl : g_tele) {
		if (tl.t >= tl.dur) continue;
		float k = tl.t / tl.dur;
		Material tm; tm.lit = false; tm.color = tl.c.alpha(tl.c.a * (0.4f + 0.6f * k));
		gfx3d::draw(A.disc, Mat4::translate(tl.p + Vec3(0, 0.09f, 0)) * Mat4::scale(Vec3(tl.r * k, 1, tl.r * k)), tm);
		Material rm; rm.lit = false; rm.color = tl.c.alpha(0.25f);
		gfx3d::draw(A.disc, Mat4::translate(tl.p + Vec3(0, 0.085f, 0)) * Mat4::scale(Vec3(tl.r, 1, tl.r)), rm);
	}
	for (const Blast &b : g_blast) {
		if (b.t < 0 || b.t >= 0.35f) continue;
		float k = b.t / 0.35f;
		Material bm; bm.lit = false; bm.color = Color(b.hurtsHero ? 0xFFB080 : godColor(b.god)).alpha(0.7f * (1 - k));
		gfx3d::draw(A.disc, Mat4::translate(b.p + Vec3(0, 0.1f, 0)) * Mat4::scale(Vec3(b.r * (0.6f + 0.5f * k), 1, b.r * (0.6f + 0.5f * k))), bm);
	}
	// Schatten
	Material shm; shm.lit = false; shm.color = Color(0x000000, 0.3f);
	gfx3d::draw(A.disc, Mat4::translate(h.pos + Vec3(0, 0.03f, 0)) * Mat4::scale(Vec3(0.42f, 1, 0.42f)), shm);
	for (const Enemy &e : enemies)
		if (e.alive && e.state != 0) gfx3d::draw(A.disc, Mat4::translate(e.pos + Vec3(0, 0.03f, 0)) * Mat4::scale(Vec3(e.r * 1.2f, 1, e.r * 1.2f)), shm);
	for (const Enemy &e : enemies) if (e.alive) drawEnemy(e, t);
	// Heldin
	for (const Ghost &gh : g_ghost)
		if (gh.t < 0.35f) {
			float k = 1 - gh.t / 0.35f;
			Material pm; pm.lit = false; pm.color = Color(0x08040E, 0.55f * k);
			gfx3d::draw(A.disc, Mat4::translate(gh.p + Vec3(0, 0.085f, 0)) * Mat4::scale(Vec3(0.45f * k + 0.1f, 1, 0.45f * k + 0.1f)), pm);
		}
	if (h.act == ACT_DASH) {
		Material pm; pm.lit = false; pm.color = Color(0x8A5AFF, 0.45f);
		gfx3d::draw(A.disc, Mat4::translate(h.pos + Vec3(0, 0.088f, 0)) * Mat4::scale(Vec3(0.68f, 1, 0.68f)), pm);
		pm.color = Color(0x06030C, 0.85f);
		gfx3d::draw(A.disc, Mat4::translate(h.pos + Vec3(0, 0.092f, 0)) * Mat4::scale(Vec3(0.55f, 1, 0.55f)), pm);
	}
	bool blink = h.iframe > 0 && h.act != ACT_DASH && fmodf(t * 20, 2) < 1;
	if (!blink) {
		float fall = h.dead > 0 ? clampf(h.dead * 2, 0, 1) : 0;
		float bob = fabsf(sinf(h.runPhase)) * 0.07f * h.runK;
		float lean = 0.16f * h.runK + (h.act == ACT_DASH ? 0.35f : 0) + (h.act == ACT_ATTACK || h.act == ACT_DSTRIKE ? 0.1f : 0);
		float roll = sinf(h.runPhase) * 0.05f * h.runK;
		Mat4 hx = Mat4::translate(h.pos + Vec3(0, bob, 0)) * Mat4::rotY(h.yaw) * Mat4::rotX(lean - fall * 1.3f) * Mat4::rotZ(roll);
		h.pose.weapon = mods.weapon;
		drawChar(CH_SEREN, hx, t, &h.pose, fmaxf(h.flash, h.armorT > 0 ? 0.25f : 0));
	}
	Material pg; pg.lit = false;
	for (const HProj &p : g_hp) {
		if (p.life <= 0) continue;
		pg.color = Color(p.god >= 0 ? godColor(p.god) : 0xC8FFFF).lighten(0.4f);
		float yy = atan2f(p.v.x, p.v.z);
		switch (p.kind) {
		case HK_WAVE: gfx3d::draw(A.slash, Mat4::translate(p.p) * Mat4::rotY(yy) * Mat4::scale(Vec3(0.9f, 1, 0.6f)), pg); break;
		case HK_BOULDER: gfx3d::draw(A.world[WM_R_ROCK], Mat4::translate(p.p + Vec3(0, -0.45f, 0)) * Mat4::rotY(yy) * Mat4::rotX(t * 9) * Mat4::scale(Vec3(0.7f, 0.9f, 0.7f)), lit); break;
		case HK_DAGGER: gfx3d::draw(A.shard, Mat4::translate(p.p) * Mat4::rotY(yy) * Mat4::rotX(PI / 2) * Mat4::scale(Vec3(0.6f, 2.6f, 0.6f)), pg); break;
		case HK_BEAM: gfx3d::draw(A.shard, Mat4::translate(p.p) * Mat4::rotY(yy) * Mat4::rotX(PI / 2) * Mat4::scale(Vec3(0.8f, 3.5f, 0.8f)), pg); break;
		default: gfx3d::draw(A.shard, Mat4::translate(p.p) * Mat4::rotY(t * 20) * Mat4::rotX(PI / 2) * Mat4::scale(Vec3(1.2f, 2.2f, 1.2f)), pg);
		}
	}
	for (const Summon &m : g_sum) {
		if (!m.active) continue;
		float fade = clampf((m.dur - m.t) * 3, 0, 1) * clampf(m.t * 6, 0, 1);
		Material sm; sm.lit = false;
		switch (m.kind) {
		case SM_TORNADO:
			sm.color = Color(0xF0F8FF, 0.7f * fade);
			for (int k = 0; k < 6; k++)
				gfx3d::draw(A.slash, Mat4::translate(m.p + Vec3(0, 0.2f + k * 0.42f, 0)) * Mat4::rotY(t * (10 + k) + k * 1.3f) * Mat4::scale(Vec3(0.35f + k * 0.2f, 1, 0.35f + k * 0.2f)), sm);
			break;
		case SM_WOLF: {
			sm.color = Color(0xFFFFFF, 0.85f * fade);
			float run = sinf(m.t * 18) * 0.06f;
			Mat4 wm = Mat4::translate(m.p + Vec3(0, fabsf(run), 0)) * Mat4::rotY(m.yaw) * Mat4::scale(Vec3(1.25f, 1.25f, 1.25f)) * Mat4::rotY(0.5f) *
			          Mat4::translate(Vec3(-0.62f, 0, -0.25f)) * Mat4::scale(Vec3(1 / 1.3f, 1 / 1.3f, 1 / 1.3f));
			gfx3d::draw(CM[CH_SYLV].part[PK_PET], wm, sm);
			break;
		}
		case SM_STAR: sm.color = Color(0xFFF2B0, fade); gfx3d::draw(A.shard, Mat4::translate(m.p) * Mat4::rotY(t * 6) * Mat4::scale(Vec3(1.8f, 2.4f, 1.8f)), sm); break;
		case SM_CLONE: {
			Pose cp;
			cp.armYaw = 1.5f; cp.armLift = 1.5f; cp.weaponBlend = 1; cp.bodyTwist = m.t / m.dur * 2 * PI;
			drawGhost(CH_SEREN, Mat4::translate(m.p) * Mat4::rotY(m.yaw), &cp, Color(0x8A5AFF, 0.55f * fade));
			break;
		}
		case SM_FIRERING: sm.color = Color(0xFF6A2A, (0.25f + 0.1f * sinf(t * 12)) * fade); gfx3d::draw(A.disc, Mat4::translate(m.p + Vec3(0, 0.1f, 0)) * Mat4::scale(Vec3(2.4f, 1, 2.4f)), sm); break;
		case SM_RING: {
			float r = 0.5f + m.t / m.dur * 3.8f;
			sm.color = Color(0xE8B070, 0.5f * (1 - m.t / m.dur));
			for (int k = 0; k < 6; k++) gfx3d::draw(A.slash, Mat4::translate(m.p + Vec3(0, 0.25f, 0)) * Mat4::rotY(k * PI / 3) * Mat4::scale(Vec3(r, 1, r)), sm);
			break;
		}
		}
	}
	Material ep; ep.lit = false; ep.color = Color(0x9AFFF8);
	for (const EProj &p : g_ep) if (p.life > 0) gfx3d::draw(A.shard, Mat4::translate(p.p) * Mat4::scale(Vec3(1.8f, 1.8f, 1.8f)), ep);
	for (const SlashFx &s : g_slash) {
		if (s.t >= s.dur) continue;
		float k = s.t / s.dur;
		Material sm; sm.lit = false; sm.color = s.c.alpha(s.c.a * (1 - k * k));
		float sc2 = 0.85f + 0.3f * k;
		gfx3d::draw(A.slash, s.xf * Mat4::scale(Vec3(sc2, 1, sc2)), sm);
	}
	gfx3d::end(g);
	// 2D über der Szene
	Vec2 sp, sp2;
	float dep, dep2;
	for (int k = 0; k < nObst; k++)
		if (obst[k].fire && gfx3d::project(g, cam, obst[k].pos + Vec3(0, 1.3f, 0), &sp, &dep)) g.image(A.ui[UI_P_GLOW], sp.x, sp.y, 1.1f, 1.1f, 0, Color(0xFF9A4A, 0.45f + 0.1f * sinf(t * 11)), 1, dep);
	if (gridMode) {
		for (int k = 0; k < grid.nTorch; k++)
			if (gfx3d::project(g, cam, grid.torch[k], &sp, &dep))
				g.image(A.ui[UI_P_GLOW], sp.x, sp.y, biome ? 1.2f : 0.8f, biome ? 1.2f : 0.8f, 0, Color(biome ? (k % 2 ? 0xB08AFF : 0x6AD8FF) : 0xFFA050, (biome ? 0.25f : 0.4f) + 0.1f * sinf(t * 9 + k)), 1, dep);
	} else
		for (int k = 0; k < 28 && biome == 0; k += 3) {
			float a0 = 2 * PI * k / 28, am = 2 * PI * (k + 0.5f) / 28;
			if (cosf(am) > 0.35f) continue;
			if (gfx3d::project(g, cam, Vec3(sinf(a0) * 4.83f, 2.32f, cosf(a0) * 4.83f), &sp, &dep))
				g.image(A.ui[UI_P_GLOW], sp.x, sp.y, 0.8f, 0.8f, 0, Color(0xFFA050, 0.4f + 0.1f * sinf(t * 9 + k)), 1, dep);
		}
	for (const HProj &p : g_hp)
		if (p.life > 0 && gfx3d::project(g, cam, p.p, &sp, &dep)) g.image(A.ui[UI_P_GLOW], sp.x, sp.y, 0.9f, 0.9f, 0, Color(p.god >= 0 ? godColor(p.god) : 0x6AD8FF, 0.7f), 1, dep);
	for (const EProj &p : g_ep)
		if (p.life > 0 && gfx3d::project(g, cam, p.p, &sp, &dep)) g.image(A.ui[UI_P_GLOW], sp.x, sp.y, 0.6f, 0.6f, 0, Color(0x6AFFF0, 0.8f), 1, dep);
	if (gfx3d::project(g, cam, h.pos + Vec3(0.08f, 1.2f, 0.15f), &sp, &dep)) g.image(A.ui[UI_P_GLOW], sp.x, sp.y, 0.35f, 0.35f, 0, Color(0x8AF0FF, 0.55f), 1, dep);
	for (const Bolt &b : g_bolt) {
		if (b.t > 0.14f) continue;
		if (!gfx3d::project(g, cam, b.a, &sp, &dep) || !gfx3d::project(g, cam, b.b, &sp2, &dep2)) continue;
		Vec2 prev = sp;
		for (int i = 1; i <= 5; i++) {
			float u = i / 5.0f;
			Vec2 q = sp + (sp2 - sp) * u + (i < 5 ? Vec2(g_r.range(-6, 6), g_r.range(-6, 6)) : Vec2());
			g.line(prev.x, prev.y, q.x, q.y, 2.5f, b.c, dep);
			g.line(prev.x, prev.y, q.x, q.y, 1, Color(0xFFFFFF), dep);
			prev = q;
		}
	}
	for (const Enemy &e : enemies) {                                  // Lebensbalken
		if (e.alive && e.type == EN_ARCHER && e.state == 2) {         // Ziellaser
			Vec3 a0 = e.pos + Vec3(0, 1.0f, 0), a1 = a0 + e.aim * 10;
			if (gfx3d::project(g, cam, a0, &sp, &dep) && gfx3d::project(g, cam, a1, &sp2, &dep2)) {
				float k = clampf(e.st / 0.85f, 0, 1);
				g.line(sp.x, sp.y, sp2.x, sp2.y, e.st > 0.55f ? 2.0f : 1.0f, Color(0xFF5AD0, 0.25f + 0.6f * k), dep);
			}
		}
		if (!e.alive || e.type == EN_DUMMY || e.state == 0 || e.state == 5 || e.hp >= e.maxHp) continue;
		if (!gfx3d::project(g, cam, e.pos + Vec3(0, (e.type == EN_BRUTE ? 2.2f : 1.5f) * e.scale, 0), &sp, &dep)) continue;
		float w = e.type == EN_BOSS || e.type == EN_QUEEN ? 0 : e.elite ? 40 : 28;
		if (w <= 0) continue;
		g.rect(sp.x - w / 2 - 1, sp.y - 1, w + 2, 5, Color(0x000000, 0.7f), dep);
		g.rect(sp.x - w / 2, sp.y, w * clampf(e.hp / e.maxHp, 0, 1), 3, e.elite ? Color(0xFFC850) : Color(0xFF5A6A), dep);
		if (e.burnT > 0) g.circle(sp.x + w / 2 + 5, sp.y + 1, 2.5f, Color(0xFF7A2A), dep);
		if (e.poison > 0) g.circle(sp.x + w / 2 + 11, sp.y + 1, 2.5f, Color(0x9AE040), dep);
		if (e.slowT > 0) g.circle(sp.x + w / 2 + 17, sp.y + 1, 2.5f, Color(0x3CDCD2), dep);
	}
	if (pickup.active && pickup.t > 0.45f && gfx3d::project(g, cam, pickup.pos + Vec3(0, 0.6f, 0), &sp, &dep)) {
		g.image(A.ui[UI_P_GLOW], sp.x, sp.y, 1.4f, 1.4f, 0, Color(pickup.kind == PK2_BOON ? godColor(pickup.god) : 0xFFE08A, 0.55f), 1, dep);
		g.image(A.ui[pickIcon(pickup.kind, pickup.god)], sp.x, sp.y - 34, 0.6f, 0.6f, 0, Color(0xFFFFFF), 0, dep - 0.5f);
	}
	for (const Door &d : doors) {
		if (!d.active || d.t < 0.6f || !gfx3d::project(g, cam, d.pos + Vec3(0, 3.0f, 0), &sp, &dep)) continue;
		float a = (pickup.active ? 0.35f : 1) * clampf((d.t - 0.6f) * 4, 0, 1);
		float bob = sinf(t * 2.5f + d.pos.x) * 2;
		int ri = ROOM_ICON[d.room];
		if (ri >= 0 && d.room != RT_COMBAT) g.image(A.ui[ri], sp.x - 16, sp.y + bob + 4, 0.55f, 0.55f, 0, Color(0xFFFFFF, a), 0, dep - 0.5f);
		if (d.reward) g.image(A.ui[pickIcon(d.reward, d.god)], sp.x + (ri >= 0 && d.room != RT_COMBAT ? 10 : 0), sp.y + bob, d.reward == PK2_BOON ? 0.48f : 0.7f, d.reward == PK2_BOON ? 0.48f : 0.7f, 0, Color(0xFFFFFF, a), 0, dep - 0.5f);
		if (d.mod) g.image(A.ui[UI_ICON_CHALLENGE], sp.x + 22, sp.y + bob + 12, 0.35f, 0.35f, 0, Color(0xFF8A6A, a), 0, dep - 0.5f);
	}
	g_fx.draw(g, cam);
	if (S.numbers)
		for (const Num &n : g_num) {
			if (n.t > 0.85f) continue;
			Vec3 p = n.p + Vec3(0, n.t * 1.2f, 0);
			if (!gfx3d::project(g, cam, p, &sp, &dep)) continue;
			float pop = n.t < 0.08f ? 1.6f - n.t * 7 : 1.0f;
			float a = n.t > 0.6f ? 1 - (n.t - 0.6f) / 0.25f : 1;
			char b[16];
			snprintf(b, sizeof b, "%d", n.v);
			g.text(sp.x, sp.y - 10, (n.crit ? 0.8f : 0.6f) * pop, n.c.alpha(a), b, ALIGN_CENTER, 0, true, dep - 1);
			if (n.crit) g.text(sp.x, sp.y - 24, 0.36f, Color(0xFFD24A, a), "KRIT", ALIGN_CENTER, 0, true, dep - 1);
		}
	if (chain >= 3) {
		float pop = g_comboT < 0.1f ? 1.3f - g_comboT * 3 : 1;
		char b[24];
		snprintf(b, sizeof b, "%d", chain);
		g.text(386, 26, 0.9f * pop, Color(0xFFE08A), b, ALIGN_RIGHT, 0, true, -2);
		g.text(386, 58, 0.34f, Color(0xFFFFFF, 0.8f), "TREFFER", ALIGN_RIGHT, 0, true, -2);
	}
	if (g_hurtFlash > 0) {
		Color c(0xFF2A2A, 0.35f * g_hurtFlash);
		g.rect(0, 0, 400, 14, c, -3); g.rect(0, 226, 400, 14, c, -3); g.rect(0, 0, 14, 240, c, -3); g.rect(386, 0, 14, 240, c, -3);
	}
}
