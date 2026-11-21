// Sternenschmiede - Spiellogik
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int g_sci = 0;

const GenDef GEN[NGEN] = {
	{"Solar Cell", "Fängt Sternenlicht ein.", 15, 0.1, 0xFFD84A},
	{"Mining Drone", "Schürft Erz auf Asteroiden.", 100, 1, 0xB8C4D8},
	{"Moon Mine", "Gräbt tief im Mondgestein.", 1100, 8, 0x9A8CFF},
	{"Fusion Reactor", "Verschmilzt Wasserstoff.", 12000, 47, 0x4AD8FF},
	{"Asteroid Catcher", "Holt ganze Brocken heran.", 130000, 260, 0xFF8A4A},
	{"Plasma Forge", "Schmiedet reines Plasma.", 1.4e6, 1400, 0xFF5AD8},
	{"Ringworld", "Eine Welt als Kraftwerk.", 2e7, 7800, 0x6AFF8A},
	{"Dyson Swarm", "Umhüllt einen ganzen Stern.", 3.3e8, 44000, 0xFFB84A},
	{"Quantum Factory", "Produziert in allen Zuständen.", 5.1e9, 260000, 0x8AFFFF},
	{"Star Lifter", "Saugt Materie aus Sonnen.", 7.5e10, 1.6e6, 0xFF6A6A},
	{"Wormhole Gate", "Energie aus fernen Galaxien.", 1e12, 1e7, 0xB46AFF},
	{"Galaxy Core", "Ein schwarzes Loch als Motor.", 1.4e13, 6.5e7, 0xF0F0FF},
	{"Reality Loom", "Webt Energie aus Naturgesetzen.", 1.7e14, 4.3e8, 0x6AFFD8},
	{"Multiverse Anchor", "Zapft Paralleluniversen an.", 2.1e15, 2.9e9, 0xFFD8F0},
};

const PerkDef PERK[NPERK] = {
	[PK_LIGHT] = {"Primordial Light", 1, 1.6, 50, -1, 0},
	[PK_CLICK] = {"Spark Hand", 1, 2.0, 25, -1, 0},
	[PK_START] = {"Head Start", 3, 3, 10, PK_LIGHT, 1},
	[PK_OFFEFF] = {"Time Capsule", 2, 2.2, 5, -1, 0},
	[PK_OFFCAP] = {"Time Dilation", 2, 2.0, 10, PK_OFFEFF, 1},
	[PK_METFREQ] = {"Meteor Shower", 3, 2.5, 5, -1, 0},
	[PK_METGOLD] = {"Gold Vein", 5, 2.5, 5, PK_METFREQ, 1},
	[PK_COST] = {"Efficient Blueprints", 10, 5, 4, PK_LIGHT, 3},
	[PK_AUTOGEN] = {"Drone Swarm", 4, 2.0, 14, PK_LIGHT, 1},
	[PK_AUTOUPG] = {"Hive Mind", 25, 1, 1, PK_AUTOGEN, 3},
	[PK_FOUNDATION] = {"Foundation", 4, 2.5, 10, PK_START, 1},
	[PK_COMBO] = {"Combo Master", 3, 3, 5, PK_CLICK, 2},
	[PK_DUSTGAIN] = {"Star Harvest", 10, 2.5, 10, PK_LIGHT, 5},
	[PK_ACHBOOST] = {"Hall of Fame", 8, 3, 4, -1, 0},
	[PK_AUTOCLICK] = {"Ghost Hand", 2, 1.8, 20, PK_CLICK, 1},
	[PK_NETWORK] = {"Star Network", 20, 3, 5, PK_DUSTGAIN, 1},
};

const SPerkDef SPERK[NSPERK] = {
	[SP_PROD] = {"Primal Force", 1, 3, 30},
	[SP_DUST] = {"Dust Stream", 1, 2, 20},
	[SP_TIME] = {"Time Accelerator", 2, 3, 5},
	[SP_KEEPAUTO] = {"Eternal Swarm", 3, 1, 1},
	[SP_STARTDUST] = {"Star Seed", 2, 3, 6},
	[SP_OFFLINE] = {"Eternal Clock", 2, 1, 1},
	[SP_MAGNET] = {"Meteor Magnet", 3, 1, 1},
	[SP_CHALX2] = {"Master Trial", 4, 1, 1},
};

const ChalDef CHAL[NCHAL] = {
	[CH_NOCLICK] = {"Silent Hands", "Klicks und Geisterhand wirkungslos.", "Klickkraft x10", 1e24},
	[CH_DARK] = {"Dark Matter", "Produktion x0,01.", "Produktion x3", 1e22},
	[CH_INFLATION] = {"Inflation", "Gebäudepreise steigen viel schneller.", "Preisanstieg -0,01", 1e20},
	[CH_ASCETIC] = {"Ascetic", "Keine Upgrades kaufbar.", "Upgrades 50% billiger", 1e18},
	[CH_LONELY] = {"Solitude", "Nur die ersten 7 Gebäude.", "Gebäude 8-14 x5", 1e21},
	[CH_RUSH] = {"Short Fuse", "Ziel in 15 Minuten erreichen.", "Stardust x2", 1e21},
};

UpgDef UPG[MAX_UPG];
int nUpg;
AchDef ACH[MAX_ACH];
int nAch;

#define DUST_DIV 1e8
#define DUST_EXP 0.4
#define DUST_BONUS 0.05
#define BANG_DIV 1e18
#define BANG_EXP 0.125
#define RUSH_TIME 900.0

static const int TIER_N[12] = {1, 5, 25, 50, 100, 150, 200, 250, 300, 350, 400, 500};
static const char *TIER_NAME[12] = {"Refined", "Optimized", "Overcharged", "Crystal Lattice",
	"Quantum Core", "Starfire", "Nebula Weave", "Cosmic Seal", "Eternity", "Singularity",
	"Infinity", "Omnipotence"};
static const char *ROMAN[20] = {"I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X",
	"XI", "XII", "XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX", "XX"};

static UpgDef *addU(int type, int rq, double rqVal, double cost, double val) {
	UpgDef *u = &UPG[nUpg++];
	memset(u, 0, sizeof *u);
	u->type = type; u->rq = rq; u->rqVal = rqVal; u->cost = cost; u->val = val;
	return u;
}

static void addA(int cat, int gen, double v) {
	ACH[nAch].cat = cat; ACH[nAch].gen = gen; ACH[nAch].val = v; nAch++;
}

void game_init_tables(void) {
	nUpg = 0;
	for (int i = 0; i < NGEN; i++)
		for (int t = 0; t < 12; t++) {
			double c = GEN[i].cost * 10.0 * pow(1.15, TIER_N[t]) * (1 + t);
			UpgDef *u = addU(UT_GEN, RQ_GEN, TIER_N[t], c, 2);
			u->gen = i; u->tier = t;
			snprintf(u->name, sizeof u->name, "%s: %s", GEN[i].name, TIER_NAME[t]);
		}
	for (int k = 0; k + 1 < NGEN; k++) {
		UpgDef *u = addU(UT_SYNERGY, RQ_SYN, 15, GEN[k + 1].cost * 150, 0.05);
		u->gen = k; u->gen2 = k + 1;
		snprintf(u->name, sizeof u->name, "Synergy %s", ROMAN[k]);
	}
	for (int j = 0; j < 20; j++) {
		double rq = pow(10, 4 + 2 * j);
		UpgDef *u = addU(UT_GLOBAL, RQ_EARNED, rq, rq * 0.4, 1.5);
		u->tier = j;
		snprintf(u->name, sizeof u->name, "Resonance %s", ROMAN[j]);
	}
	static const double cr[8] = {1, 50, 200, 500, 1000, 2500, 5000, 10000};
	static const double cc[8] = {100, 2e3, 1e5, 1e7, 1e9, 1e12, 1e15, 1e19};
	for (int j = 0; j < 8; j++) {
		UpgDef *u = addU(UT_CLICK, RQ_CLICKS, cr[j], cc[j], 2);
		u->tier = j;
		snprintf(u->name, sizeof u->name, "Spark Finger %s", ROMAN[j]);
	}
	static const double pr[6] = {100, 500, 1500, 4000, 10000, 25000};
	static const double pc[6] = {1e4, 1e6, 1e8, 1e11, 1e14, 1e18};
	for (int j = 0; j < 6; j++) {
		UpgDef *u = addU(UT_CLICKPCT, RQ_CLICKS, pr[j], pc[j], 0.01);
		u->tier = j;
		snprintf(u->name, sizeof u->name, "Energy Grip %s", ROMAN[j]);
	}
	static const double mr[6] = {1, 3, 10, 25, 60, 120};
	static const double mc[6] = {1e4, 1e7, 1e10, 1e14, 1e18, 1e23};
	for (int j = 0; j < 6; j++) {
		UpgDef *u = addU(UT_METEOR, RQ_METEORS, mr[j], mc[j], 1);
		u->tier = j;
		snprintf(u->name, sizeof u->name, "Meteor Radar %s", ROMAN[j]);
	}
	static const double kr[4] = {150, 750, 3000, 12000};
	static const double kc[4] = {1e3, 1e6, 1e10, 1e15};
	for (int j = 0; j < 4; j++) {
		UpgDef *u = addU(UT_COMBO, RQ_CLICKS, kr[j], kc[j], 1);
		u->tier = j;
		snprintf(u->name, sizeof u->name, "Combo Chain %s", ROMAN[j]);
	}

	nAch = 0;
	static const double ae[20] = {1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e12, 1e14, 1e16,
		1e18, 1e21, 1e24, 1e27, 1e30, 1e35, 1e40, 1e50};
	for (int j = 0; j < 20; j++) addA(AC_EARNED, 0, ae[j]);
	static const double ap[14] = {1, 10, 100, 1e3, 1e4, 1e5, 1e6, 1e8, 1e10, 1e12, 1e15, 1e18, 1e21, 1e25};
	for (int j = 0; j < 14; j++) addA(AC_EPS, 0, ap[j]);
	static const double ak[8] = {1, 100, 500, 1000, 2500, 5000, 10000, 25000};
	for (int j = 0; j < 8; j++) addA(AC_CLICKS, 0, ak[j]);
	static const double ag[4] = {1, 50, 100, 200};
	for (int i = 0; i < NGEN; i++)
		for (int j = 0; j < 4; j++) addA(AC_GEN, i, ag[j]);
	static const double at[6] = {100, 250, 500, 1000, 2000, 4000};
	for (int j = 0; j < 6; j++) addA(AC_TOTALGEN, 0, at[j]);
	static const double am[6] = {1, 10, 25, 50, 100, 250};
	for (int j = 0; j < 6; j++) addA(AC_METEORS, 0, am[j]);
	static const double an[6] = {1, 3, 5, 10, 25, 50};
	for (int j = 0; j < 6; j++) addA(AC_NOVAS, 0, an[j]);
	static const double au[6] = {10, 25, 50, 100, 150, 200};
	for (int j = 0; j < 6; j++) addA(AC_UPGS, 0, au[j]);
	for (int j = 2; j <= 5; j++) addA(AC_COMBO, 0, j);
	static const double ab[3] = {1, 3, 10};
	for (int j = 0; j < 3; j++) addA(AC_BANGS, 0, ab[j]);
	for (int c = 0; c < NCHAL; c++) addA(AC_CHAL, c, 1);
	static const double ati[7] = {600, 3600, 3 * 3600, 5 * 3600, 10 * 3600, 24 * 3600, 100 * 3600};
	for (int j = 0; j < 7; j++) addA(AC_TIME, 0, ati[j]);
	// v2 (nur anhaengen, damit alte Bits gueltig bleiben)
	static const double aex[4] = {1, 10, 50, 200};
	for (int j = 0; j < 4; j++) addA(AC_EXPED, 0, aex[j]);
	static const double ase[4] = {3, 6, 9, 12};
	for (int j = 0; j < 4; j++) addA(AC_SECTOR, 0, ase[j]);
	static const double aar[4] = {1, 5, 10, 20};
	for (int j = 0; j < 4; j++) addA(AC_ART, 0, aar[j]);
	static const double aco[3] = {5, 25, 100};
	for (int j = 0; j < 3; j++) addA(AC_CONTRACT, 0, aco[j]);
}

// ---------- Hilfen ----------
static uint32_t rng_state = 2463534242u;
static double frand(void) {
	rng_state ^= rng_state << 13; rng_state ^= rng_state >> 17; rng_state ^= rng_state << 5;
	return (rng_state & 0xFFFFFF) / (double)0x1000000;
}
void game_seed(uint32_t s) { rng_state = s ? s : 1; }

static bool chal_done(Game *g, int c) { return (g->s.chalDone >> c) & 1; }
static double chal_rew(Game *g) { return g->s.sperk[SP_CHALX2] ? 2 : 1; }

void game_push(Game *g, int type, int id) {
	int n = (g->evHead + 1) % 16;
	if (n == g->evTail) return;
	g->ev[g->evHead].type = type; g->ev[g->evHead].id = id;
	g->evHead = n;
}
bool game_pop(Game *g, GameEvent *e) {
	if (g->evTail == g->evHead) return false;
	*e = g->ev[g->evTail];
	g->evTail = (g->evTail + 1) % 16;
	return true;
}

// ---------- Neues Spiel / Reset ----------
static void reset_run(Game *g) {
	Save *s = &g->s;
	s->energy = 0;
	if (s->perk[PK_START]) s->energy = 100.0 * pow(10, s->perk[PK_START]);
	s->runEarned = 0;
	memset(s->gens, 0, sizeof s->gens);
	memset(s->upg, 0, sizeof s->upg);
	memset(s->buffT, 0, sizeof s->buffT);
	for (int i = 0; i < 4; i++) s->gens[i] = 10 * s->perk[PK_FOUNDATION];
	s->clicksRun = 0;
	s->runTime = 0;
	s->chalActive = -1;
	s->chalTime = 0;
	g->comboHeat = 0;
	g->meteorLife = 0;
	g->meteorTimer = 45;
	g->dirty = true;
}

void game_new(Game *g) {
	memset(g, 0, sizeof *g);
	g->s.magic = SAVE_MAGIC;
	g->s.version = SAVE_VERSION;
	g->s.sound = 1;
	g->s.stereo = 1;
	g->s.autoGen = 1;
	g->s.autoUpg = 1;
	g->s.autoClickOn = 1;
	reset_run(g);
	g->meteorTimer = 60;
	game_recalc(g);
}

// ---------- Berechnung ----------
double gen_unit_prod(Game *g, int i) { return GEN[i].prod * g->genMult[i] * g->globalMult; }

static double milestone(double n) {
	double m = 1;
	if (n >= 25) m *= 2;
	if (n >= 50) m *= 2;
	for (double k = 100; k <= n; k += 100) m *= 2;
	return m;
}

void game_recalc(Game *g) {
	Save *s = &g->s;
	int ch = s->chalActive;
	double up[MAX_GEN], syn[MAX_GEN];
	for (int i = 0; i < NGEN; i++) { up[i] = 1; syn[i] = 1; }
	double gmul = 1, cmul = 1, cpct = 0;
	int met = 0, combo = 0, cnt = 0;
	for (int i = 0; i < nUpg; i++) {
		if (!upg_bought(g, i)) continue;
		cnt++;
		UpgDef *u = &UPG[i];
		switch (u->type) {
		case UT_GEN: up[u->gen] *= u->val; break;
		case UT_SYNERGY: syn[u->gen] += u->val * s->gens[u->gen2]; break;
		case UT_GLOBAL: gmul *= u->val; break;
		case UT_CLICK: cmul *= u->val; break;
		case UT_CLICKPCT: cpct += u->val; break;
		case UT_METEOR: met++; break;
		case UT_COMBO: combo++; break;
		}
	}
	g->upgCount = cnt;
	g->metUpg = met;
	g->comboMax = 2 + combo + s->perk[PK_COMBO];
	g->clickPct = cpct;

	double total = 0;
	for (int i = 0; i < NGEN; i++) total += s->gens[i];
	g->totalGens = total;

	double m = 1;
	m *= 1 + ach_count(g) * (0.01 + 0.005 * s->perk[PK_ACHBOOST]);
	m *= 1 + DUST_BONUS * s->dustTotal;
	m *= 1 + 0.10 * s->perk[PK_LIGHT];
	m *= 1 + 0.002 * s->perk[PK_NETWORK] * total;
	m *= sqrt(1 + s->singTotal);
	m *= pow(1.5, s->sperk[SP_PROD]);
	m *= gmul;
	m *= 1 + art_bonus(g, AK_PROD);
	m *= refinery_mult(g);
	if (chal_done(g, CH_DARK)) m *= 1 + 2 * chal_rew(g);
	if (ch == CH_DARK) m *= 0.01;
	g->globalMultBase = m;
	if (s->buffT[BUFF_FRENZY] > 0) m *= 7;
	g->globalMult = m;

	double eps = 0, epsBase = 0;
	for (int i = 0; i < NGEN; i++) {
		double gm = up[i] * syn[i] * milestone(s->gens[i]);
		if (i >= 7 && chal_done(g, CH_LONELY)) gm *= chal_rew(g) > 1 ? 25 : 5;
		g->genMult[i] = gm;
		double e = s->gens[i] * GEN[i].prod * gm;
		g->genEps[i] = e * m;
		eps += e * m;
		epsBase += e * g->globalMultBase;
	}
	g->eps = eps;
	g->epsBase = epsBase;

	double cg = 1.15 - 0.005 * s->perk[PK_COST];
	if (chal_done(g, CH_INFLATION)) cg -= 0.01 * chal_rew(g);
	if (ch == CH_INFLATION) cg += 0.10;
	if (cg < 1.07) cg = 1.07;
	g->costGrowth = cg;

	double cv = pow(2, s->perk[PK_CLICK]) * cmul + cpct * eps;
	if (chal_done(g, CH_NOCLICK)) cv *= chal_rew(g) > 1 ? 100 : 10;
	cv *= 1 + art_bonus(g, AK_CLICK);
	if (s->buffT[BUFF_CLICKRUSH] > 0) cv *= 77;
	if (ch == CH_NOCLICK) cv = 0;
	g->clickVal = cv;
	g->combo = 1 + g->comboHeat;
}

// ---------- Gebaeude ----------
double gen_cost(Game *g, int i, int n) {
	double r = g->costGrowth;
	double c = GEN[i].cost * pow(r, g->s.gens[i]);
	if (n == 1) return c;
	return c * (pow(r, n) - 1) / (r - 1);
}

int gen_max_afford(Game *g, int i) {
	double r = g->costGrowth;
	double c = GEN[i].cost * pow(r, g->s.gens[i]);
	if (g->s.energy < c) return 0;
	double n = floor(log(g->s.energy * (r - 1) / c + 1) / log(r));
	if (n < 1) n = 1;
	if (n > 100000) n = 100000;
	while (n > 1 && gen_cost(g, i, (int)n) > g->s.energy) n--;
	return (int)n;
}

bool gen_locked(Game *g, int i) { return g->s.chalActive == CH_LONELY && i >= 7; }

int gen_buy(Game *g, int i, int n) {
	if (gen_locked(g, i)) return 0;
	if (n <= 0) n = gen_max_afford(g, i);
	if (n <= 0) return 0;
	double c = gen_cost(g, i, n);
	if (c > g->s.energy) return 0;
	g->s.energy -= c;
	g->s.gens[i] += n;
	game_recalc(g);
	return n;
}

bool gen_visible(Game *g, int i) {
	return i == 0 || g->s.gens[i] > 0 || g->s.gens[i - 1] > 0;
}

// ---------- Upgrades ----------
bool upg_bought(Game *g, int i) { return (g->s.upg[i >> 5] >> (i & 31)) & 1; }

bool upg_unlocked(Game *g, int i) {
	UpgDef *u = &UPG[i];
	Save *s = &g->s;
	switch (u->rq) {
	case RQ_GEN: return s->gens[u->gen] >= u->rqVal;
	case RQ_EARNED: return s->runEarned >= u->rqVal;
	case RQ_CLICKS: return s->clicksLife >= u->rqVal;
	case RQ_METEORS: return s->meteorsLife >= u->rqVal;
	case RQ_SYN: return s->gens[u->gen] >= u->rqVal && s->gens[u->gen2] >= u->rqVal;
	}
	return false;
}

double upg_cost(Game *g, int i) {
	double c = UPG[i].cost;
	if (chal_done(g, CH_ASCETIC)) c *= chal_rew(g) > 1 ? 0.25 : 0.5;
	return c;
}

bool upg_buy(Game *g, int i) {
	if (g->s.chalActive == CH_ASCETIC) return false;
	if (upg_bought(g, i) || !upg_unlocked(g, i)) return false;
	double c = upg_cost(g, i);
	if (c > g->s.energy) return false;
	g->s.energy -= c;
	g->s.upg[i >> 5] |= 1u << (i & 31);
	game_recalc(g);
	return true;
}

int upg_buy_all(Game *g) {
	int n = 0;
	for (;;) {
		int best = -1;
		double bc = 0;
		for (int i = 0; i < nUpg; i++) {
			if (upg_bought(g, i) || !upg_unlocked(g, i)) continue;
			double c = upg_cost(g, i);
			if (c <= g->s.energy && (best < 0 || c < bc)) { best = i; bc = c; }
		}
		if (best < 0 || !upg_buy(g, best)) break;
		n++;
	}
	return n;
}

void upg_desc(Game *g, int i, char *buf, int n) {
	UpgDef *u = &UPG[i];
	char a[32];
	switch (u->type) {
	case UT_GEN: snprintf(buf, n, "%s produziert doppelt so viel.", GEN[u->gen].name); break;
	case UT_SYNERGY:
		snprintf(buf, n, "%s +5%% pro %s.", GEN[u->gen].name, GEN[u->gen2].name); break;
	case UT_GLOBAL: snprintf(buf, n, "Gesamte Produktion x1,5."); break;
	case UT_CLICK: snprintf(buf, n, "Klickkraft x2."); break;
	case UT_CLICKPCT: snprintf(buf, n, "Klicks erhalten +1%% deiner Produktion/s."); break;
	case UT_METEOR: snprintf(buf, n, "Meteore häufiger (+10%%) und stärker (+30%%)."); break;
	case UT_COMBO: snprintf(buf, n, "Maximale Kombo +1."); break;
	default: buf[0] = 0;
	}
	if (!upg_unlocked(g, i)) {
		switch (u->rq) {
		case RQ_GEN: snprintf(buf, n, "Benötigt %d %s.", (int)u->rqVal, GEN[u->gen].name); break;
		case RQ_EARNED: fmt_num(u->rqVal, a, sizeof a); snprintf(buf, n, "Verdiene %s in diesem Lauf.", a); break;
		case RQ_CLICKS: snprintf(buf, n, "Benötigt %d Klicks.", (int)u->rqVal); break;
		case RQ_METEORS: snprintf(buf, n, "Fange %d Meteore.", (int)u->rqVal); break;
		case RQ_SYN: snprintf(buf, n, "Je 15 %s und %s.", GEN[u->gen].name, GEN[u->gen2].name); break;
		}
	}
}

// ---------- Klicken ----------
double click(Game *g, bool manual) {
	double v = g->clickVal;
	if (manual) {
		v *= 1 + g->comboHeat;
		g->comboHeat += 0.12;
		if (g->comboHeat > g->comboMax) g->comboHeat = g->comboMax;
		g->comboIdle = 0;
		if (1 + g->comboHeat > g->s.bestCombo) g->s.bestCombo = 1 + g->comboHeat;
		if (g->s.chalActive != CH_NOCLICK) {
			g->s.clicksLife += 1;
			g->s.clicksRun += 1;
		}
	}
	g->s.energy += v;
	g->s.runEarned += v;
	g->s.lifeEarned += v;
	g->s.bangEarned += v;
	return v;
}

// ---------- Meteore ----------
static double meteor_interval(Game *g) {
	double f = (1 + 0.1 * g->metUpg) * (1 + 0.15 * g->s.perk[PK_METFREQ]);
	return (90 + frand() * 120) / f;
}

int meteor_collect(Game *g, double *gainOut) {
	if (g->meteorLife <= 0) return -1;
	g->meteorLife = 0;
	g->meteorTimer = meteor_interval(g);
	g->s.meteorsLife += 1;
	double gold = (1 + 0.3 * g->metUpg) * (1 + 0.5 * g->s.perk[PK_METGOLD]);
	double dur = 1 + 0.2 * g->s.perk[PK_METGOLD];
	double r = frand();
	*gainOut = 0;
	if (r < 0.5) {
		double a = g->epsBase * 60 * gold;
		double b = fmin(g->s.energy * 0.15, g->epsBase * 900) * gold;
		double v = fmax(a, b) + 25;
		g->s.energy += v; g->s.runEarned += v; g->s.lifeEarned += v; g->s.bangEarned += v;
		*gainOut = v;
		return 0;
	} else if (r < 0.85) {
		g->s.buffT[BUFF_FRENZY] = fmax(g->s.buffT[BUFF_FRENZY], 30 * dur);
		game_recalc(g);
		return 1;
	}
	g->s.buffT[BUFF_CLICKRUSH] = fmax(g->s.buffT[BUFF_CLICKRUSH], 12 * dur);
	game_recalc(g);
	return 2;
}

// ---------- Update ----------
void game_update(Game *g, double dt) {
	Save *s = &g->s;
	if (dt <= 0) return;
	double speed = 1 + 0.1 * s->sperk[SP_TIME];
	double v = g->eps * dt * speed;
	s->energy += v; s->runEarned += v; s->lifeEarned += v; s->bangEarned += v;

	// Geisterhand
	if (s->autoClickOn && s->perk[PK_AUTOCLICK] && s->chalActive != CH_NOCLICK) {
		g->autoClickAcc += s->perk[PK_AUTOCLICK] * dt;
		while (g->autoClickAcc >= 1) { g->autoClickAcc -= 1; click(g, false); }
	}
	// Kombo-Abklingen
	g->comboIdle += dt;
	if (g->comboIdle > 0.45) {
		g->comboHeat -= dt * (0.8 + g->comboHeat * 0.5);
		if (g->comboHeat < 0) g->comboHeat = 0;
	}
	// Buffs
	bool changed = false;
	for (int b = 0; b < NBUFF; b++)
		if (s->buffT[b] > 0) {
			s->buffT[b] -= dt;
			if (s->buffT[b] <= 0) { s->buffT[b] = 0; changed = true; }
		}
	// Meteore
	if (g->meteorLife > 0) {
		g->meteorLife -= dt;
		if (s->sperk[SP_MAGNET] && g->meteorLife < 10) {
			double gain;
			int e = meteor_collect(g, &gain);
			game_push(g, EV_METEOR_AUTO, e);
		} else if (g->meteorLife <= 0) {
			g->meteorLife = 0;
			g->meteorTimer = meteor_interval(g);
		}
	} else {
		g->meteorTimer -= dt;
		if (g->meteorTimer <= 0) {
			g->meteorLife = 13;
			game_push(g, EV_METEOR, 0);
		}
	}
	// Automatik
	g->autoTimer += dt;
	if (g->autoTimer >= 0.25) {
		g->autoTimer = 0;
		if (s->autoUpg && s->perk[PK_AUTOUPG]) upg_buy_all(g);
		if (s->autoGen && s->perk[PK_AUTOGEN]) {
			int n = s->perk[PK_AUTOGEN];
			if (n > NGEN) n = NGEN;
			for (int i = n - 1; i >= 0; i--) gen_buy(g, i, 0);
		}
	}
	s->playTime += dt;
	s->runTime += dt;
	// Herausforderung
	if (s->chalActive >= 0) {
		s->chalTime += dt;
		int c = s->chalActive;
		if (s->runEarned >= CHAL[c].goal) {
			s->chalDone |= 1u << c;
			s->chalActive = -1;
			game_push(g, EV_CHAL_DONE, c);
			changed = true;
		} else if (c == CH_RUSH && s->chalTime > RUSH_TIME) {
			s->chalActive = -1;
			game_push(g, EV_CHAL_FAIL, c);
			changed = true;
		}
	}
	game_recalc(g);
	if (g->eps > s->bestEps) s->bestEps = g->eps;
	(void)changed;
	int a = ach_check(g);
	if (a >= 0) { game_push(g, EV_ACH, a); game_recalc(g); }
}

double game_offline_eff(Game *g) {
	if (g->s.sperk[SP_OFFLINE]) return 1.0;
	double e = 0.5 + 0.1 * g->s.perk[PK_OFFEFF] + art_bonus(g, AK_OFFLINE);
	return e > 1 ? 1 : e;
}
double game_offline_cap(Game *g) {
	return 3600.0 * (6 + 3 * g->s.perk[PK_OFFCAP] + (g->s.sperk[SP_OFFLINE] ? 24 : 0));
}

double game_offline(Game *g, double sec) {
	Save *s = &g->s;
	memset(s->buffT, 0, sizeof s->buffT);
	game_recalc(g);
	double t = fmin(sec, game_offline_cap(g));
	double speed = 1 + 0.1 * s->sperk[SP_TIME];
	double v = g->eps * t * game_offline_eff(g) * speed;
	if (s->autoClickOn && s->perk[PK_AUTOCLICK] && s->chalActive != CH_NOCLICK)
		v += g->clickVal * s->perk[PK_AUTOCLICK] * t * game_offline_eff(g);
	s->energy += v; s->runEarned += v; s->lifeEarned += v; s->bangEarned += v;
	if (s->chalActive >= 0) s->chalTime += sec;
	g->meteorLife = 0;
	if (g->meteorTimer > 20) g->meteorTimer = 20;
	return v;
}

// ---------- Supernova ----------
double dust_mult(Game *g) {
	Save *s = &g->s;
	double m = (1 + 0.1 * s->perk[PK_DUSTGAIN]) * (1 + 0.5 * s->sperk[SP_DUST]) * pow(1 + s->singTotal, 0.25);
	if (chal_done(g, CH_RUSH)) m *= 1 + chal_rew(g);
	m *= 1 + art_bonus(g, AK_DUST);
	return m;
}

double nova_gain(Game *g) {
	double pot = floor(pow(g->s.bangEarned / DUST_DIV, DUST_EXP) * dust_mult(g));
	double gain = pot - g->s.dustBang;
	return gain > 0 ? gain : 0;
}

void nova_do(Game *g) {
	Save *s = &g->s;
	double gain = nova_gain(g);
	s->dust += gain; s->dustTotal += gain; s->dustBang += gain;
	if (gain >= 1) s->novas += 1;
	reset_run(g);
	game_recalc(g);
}

bool perk_available(Game *g, int i) {
	const PerkDef *p = &PERK[i];
	return p->needPerk < 0 || g->s.perk[p->needPerk] >= p->needLvl;
}

double perk_cost(Game *g, int i) {
	return ceil(PERK[i].cost * pow(PERK[i].growth, g->s.perk[i]));
}

bool perk_buy(Game *g, int i) {
	if (!perk_available(g, i) || g->s.perk[i] >= PERK[i].max) return false;
	double c = perk_cost(g, i);
	if (c > g->s.dust) return false;
	g->s.dust -= c;
	g->s.perk[i]++;
	game_recalc(g);
	return true;
}

void perk_desc(Game *g, int i, char *buf, int n) {
	int l = g->s.perk[i];
	switch (i) {
	case PK_LIGHT: snprintf(buf, n, "+10%% Produktion je Stufe. Jetzt: +%d%%", l * 10); break;
	case PK_CLICK: snprintf(buf, n, "Klickkraft x2 je Stufe. Jetzt: x%.0f", pow(2, l)); break;
	case PK_START: {
		char a[24]; fmt_num(100.0 * pow(10, l + 1), a, sizeof a);
		snprintf(buf, n, "Starte jeden Lauf mit Energie. Nächste: %s", a); break; }
	case PK_OFFEFF: snprintf(buf, n, "Offline-Effizienz +10%%. Jetzt: %d%%", 50 + l * 10); break;
	case PK_OFFCAP: snprintf(buf, n, "Offline-Limit +3 h. Jetzt: %d h", 6 + 3 * l); break;
	case PK_METFREQ: snprintf(buf, n, "Meteore +15%% häufiger. Jetzt: +%d%%", l * 15); break;
	case PK_METGOLD: snprintf(buf, n, "Meteor-Wirkung +50%%, Dauer +20%%."); break;
	case PK_COST: snprintf(buf, n, "Preisanstieg -0,005. Jetzt: %.3f", 1.15 - 0.005 * l); break;
	case PK_AUTOGEN: snprintf(buf, n, "Kauft automatisch die ersten %d Gebäude.", l + 1 > NGEN ? NGEN : l + 1); break;
	case PK_AUTOUPG: snprintf(buf, n, "Kauft Upgrades automatisch."); break;
	case PK_FOUNDATION: snprintf(buf, n, "Starte mit je %d der ersten 4 Gebäude.", (l + 1) * 10); break;
	case PK_COMBO: snprintf(buf, n, "Maximale Kombo +1. Jetzt: +%d", l); break;
	case PK_DUSTGAIN: snprintf(buf, n, "Stardust +10%%. Jetzt: +%d%%", l * 10); break;
	case PK_ACHBOOST: snprintf(buf, n, "Erfolge +0,5%% stärker. Jetzt: %.1f%%", 1 + 0.5 * l); break;
	case PK_AUTOCLICK: snprintf(buf, n, "Automatisch %d Klicks/s.", l + 1); break;
	case PK_NETWORK: snprintf(buf, n, "+0,2%% Produktion je Gebäude. Jetzt: %.1f%%", 0.2 * l); break;
	default: buf[0] = 0;
	}
}

// ---------- Urknall ----------
double bang_need(Game *g) { return BANG_DIV * pow(100, g->s.bangs); }
double bang_gain(Game *g) {
	double v = floor(10 * pow(g->s.bangEarned / bang_need(g), BANG_EXP));
	return v > 0 ? v : 0;
}
bool bang_ready(Game *g) { return bang_gain(g) >= 10; }

void bang_do(Game *g) {
	Save *s = &g->s;
	double gain = bang_gain(g);
	if (gain < 10) return;
	s->sing += gain; s->singTotal += gain; s->bangs += 1;
	uint16_t keepA = s->perk[PK_AUTOGEN], keepU = s->perk[PK_AUTOUPG], keepC = s->perk[PK_AUTOCLICK];
	memset(s->perk, 0, sizeof s->perk);
	if (s->sperk[SP_KEEPAUTO]) {
		s->perk[PK_AUTOGEN] = keepA; s->perk[PK_AUTOUPG] = keepU; s->perk[PK_AUTOCLICK] = keepC;
	}
	s->dust = s->dustTotal = s->dustBang = 0;
	if (s->sperk[SP_STARTDUST]) s->dust = s->dustTotal = 25.0 * s->sperk[SP_STARTDUST] * s->sperk[SP_STARTDUST];
	s->bangEarned = 0;
	reset_run(g);
	game_recalc(g);
}

double sperk_cost(Game *g, int i) { return ceil(SPERK[i].cost * pow(SPERK[i].growth, g->s.sperk[i])); }

bool sperk_buy(Game *g, int i) {
	if (g->s.sperk[i] >= SPERK[i].max) return false;
	double c = sperk_cost(g, i);
	if (c > g->s.sing) return false;
	g->s.sing -= c;
	g->s.sperk[i]++;
	game_recalc(g);
	return true;
}

void sperk_desc(Game *g, int i, char *buf, int n) {
	int l = g->s.sperk[i];
	switch (i) {
	case SP_PROD: snprintf(buf, n, "Produktion x1,5 je Stufe. Jetzt: x%.1f", pow(1.5, l)); break;
	case SP_DUST: snprintf(buf, n, "Stardust +50%% je Stufe. Jetzt: +%d%%", 50 * l); break;
	case SP_TIME: snprintf(buf, n, "Spielzeit läuft +10%% schneller. Jetzt: +%d%%", 10 * l); break;
	case SP_KEEPAUTO: snprintf(buf, n, "Automatik-Konstellationen bleiben beim Big Bang."); break;
	case SP_STARTDUST: snprintf(buf, n, "Starte jeden Big Bang mit %d Stardust.", 25 * (l + 1) * (l + 1)); break;
	case SP_OFFLINE: snprintf(buf, n, "Offline 100%% effizient, +24 h Limit."); break;
	case SP_MAGNET: snprintf(buf, n, "Meteore werden automatisch gefangen."); break;
	case SP_CHALX2: snprintf(buf, n, "Belohnungen der Prüfungen verdoppelt."); break;
	default: buf[0] = 0;
	}
}

// ---------- Herausforderungen ----------
void chal_start(Game *g, int c) {
	nova_do(g);
	g->s.chalActive = c;
	g->s.chalTime = 0;
	game_recalc(g);
}
void chal_abort(Game *g) { g->s.chalActive = -1; game_recalc(g); }
double chal_progress(Game *g) {
	if (g->s.chalActive < 0) return 0;
	double p = log10(fmax(1, g->s.runEarned)) / log10(CHAL[g->s.chalActive].goal);
	return p > 1 ? 1 : p;
}

// ---------- Erfolge ----------
bool ach_has(Game *g, int i) { return (g->s.ach[i >> 5] >> (i & 31)) & 1; }
int ach_count(Game *g) {
	int n = 0;
	for (int i = 0; i < nAch; i++) n += ach_has(g, i);
	return n;
}

static bool ach_met(Game *g, int i) {
	Save *s = &g->s;
	AchDef *a = &ACH[i];
	switch (a->cat) {
	case AC_EARNED: return s->lifeEarned >= a->val;
	case AC_EPS: return g->eps >= a->val;
	case AC_CLICKS: return s->clicksLife >= a->val;
	case AC_GEN: return s->gens[a->gen] >= a->val;
	case AC_TOTALGEN: return g->totalGens >= a->val;
	case AC_METEORS: return s->meteorsLife >= a->val;
	case AC_NOVAS: return s->novas >= a->val;
	case AC_UPGS: return g->upgCount >= a->val;
	case AC_COMBO: return s->bestCombo >= a->val;
	case AC_BANGS: return s->bangs >= a->val;
	case AC_CHAL: return chal_done(g, a->gen);
	case AC_TIME: return s->playTime >= a->val;
	case AC_EXPED: return s->ext.x.expeditions >= a->val;
	case AC_SECTOR: return s->ext.x.sectorMax >= a->val;
	case AC_ART: return art_count(g) >= a->val;
	case AC_CONTRACT: return s->ext.x.contractsDone >= a->val;
	}
	return false;
}

int ach_check(Game *g) {
	for (int i = 0; i < nAch; i++)
		if (!ach_has(g, i) && ach_met(g, i)) {
			g->s.ach[i >> 5] |= 1u << (i & 31);
			return i;
		}
	return -1;
}

static const char *N_EARN[20] = {"First Spark", "Glimmer", "Campfire", "Beacon", "Little Sun",
	"Solar Wind", "Star Glow", "Supergiant", "Quasar", "Pulsar", "Nebula Heart", "Galaxy",
	"Galaxy Cluster", "Supercluster", "Filament", "Cosmic Web", "Infinite", "Beyond",
	"Omniscient", "Creator"};
static const char *N_EPS[14] = {"It Runs", "Assembly Line", "Power Plant", "Industry", "Planetary",
	"Stellar", "Interstellar", "Galactic", "Intergalactic", "Cosmic", "Universal",
	"Multiversal", "Omniversal", "Absolute"};
static const char *N_CLICK[8] = {"First Touch", "Busy Finger", "Click Happy", "Jack of Taps",
	"Daily Grind", "Tendon Strain", "Tireless", "Touchscreen Legend"};
static const char *N_GENL[4] = {"Beginner", "Collector", "Master", "Legend"};
static const char *N_TOT[6] = {"Settlement", "Colony", "Empire", "Civilization", "Hegemony", "Eternal Realm"};
static const char *N_MET[6] = {"Shooting Star", "Meteor Hunter", "Comet Catcher", "Sky Fisher",
	"Star Rain", "Meteor King"};
static const char *N_NOVA[6] = {"First Blast", "Fireworks", "Star Death", "Rebirth",
	"Cycle", "Eternal Return"};
static const char *N_UPG[6] = {"Tinkerer", "Engineer", "Inventor", "Visionary", "Genius", "Mastermind"};
static const char *N_COMBO[4] = {"Rhythm", "In the Flow", "Frenzy", "Ecstasy"};
static const char *N_BANG[3] = {"Big Bang", "New Universes", "World Smith"};
static const char *N_TIME[7] = {"Taste Test", "One Hour", "Endurance", "Train Rider",
	"Iron Butt", "A Whole Day", "Hundred Hours"};
static const char *N_EXP[4] = {"Liftoff", "Space Miner", "Belt Veteran", "Void Walker"};
static const char *N_SEC[4] = {"Gate Crasher", "Deep Space", "Abyss", "Edge of Reality"};
static const char *N_ART[4] = {"Relic Hunter", "Curator", "Archaeologist", "Museum of Stars"};
static const char *N_CON[3] = {"Contractor", "Reliable", "Legendary Agent"};

void ach_text(int i, char *nm, int nn, char *ds, int nd) {
	AchDef *a = &ACH[i];
	char v[32];
	fmt_num(a->val, v, sizeof v);
	int k = 0;
	for (int j = 0; j < i; j++) k += ACH[j].cat == a->cat;
	switch (a->cat) {
	case AC_EARNED: snprintf(nm, nn, "%s", N_EARN[k]); snprintf(ds, nd, "Verdiene insgesamt %s Energie.", v); break;
	case AC_EPS: snprintf(nm, nn, "%s", N_EPS[k]); snprintf(ds, nd, "Erreiche %s Energie pro Sekunde.", v); break;
	case AC_CLICKS: snprintf(nm, nn, "%s", N_CLICK[k]); snprintf(ds, nd, "Tippe %s Mal auf den Kern.", v); break;
	case AC_GEN: snprintf(nm, nn, "%s: %s", GEN[a->gen].name, N_GENL[k % 4]); snprintf(ds, nd, "Besitze %s %s.", v, GEN[a->gen].name); break;
	case AC_TOTALGEN: snprintf(nm, nn, "%s", N_TOT[k]); snprintf(ds, nd, "Besitze %s Gebäude.", v); break;
	case AC_METEORS: snprintf(nm, nn, "%s", N_MET[k]); snprintf(ds, nd, "Fange %s Meteore.", v); break;
	case AC_NOVAS: snprintf(nm, nn, "%s", N_NOVA[k]); snprintf(ds, nd, "Löse %s Supernovas aus.", v); break;
	case AC_UPGS: snprintf(nm, nn, "%s", N_UPG[k]); snprintf(ds, nd, "Besitze %s Upgrades gleichzeitig.", v); break;
	case AC_COMBO: snprintf(nm, nn, "%s", N_COMBO[k]); snprintf(ds, nd, "Erreiche eine Kombo von x%s.", v); break;
	case AC_BANGS: snprintf(nm, nn, "%s", N_BANG[k]); snprintf(ds, nd, "Löse %s Big Bang aus.", v); break;
	case AC_CHAL: snprintf(nm, nn, "Trial: %s", CHAL[a->gen].name); snprintf(ds, nd, "Meistere \"%s\".", CHAL[a->gen].name); break;
	case AC_EXPED: snprintf(nm, nn, "%s", N_EXP[k]); snprintf(ds, nd, "Schließe %s Expeditionen ab.", v); break;
	case AC_SECTOR: snprintf(nm, nn, "%s", N_SEC[k]); snprintf(ds, nd, "Erreiche Sektor %s.", v); break;
	case AC_ART: snprintf(nm, nn, "%s", N_ART[k]); snprintf(ds, nd, "Finde %s Artefakte.", v); break;
	case AC_CONTRACT: snprintf(nm, nn, "%s", N_CON[k]); snprintf(ds, nd, "Erfülle %s Aufträge.", v); break;
	case AC_TIME: {
		char t[32]; fmt_time(a->val, t, sizeof t);
		snprintf(nm, nn, "%s", N_TIME[k]); snprintf(ds, nd, "Spiele insgesamt %s.", t); break; }
	}
}

// ---------- Formatierung ----------
static const char *SUF[] = {"", "K", "M", "B", "T", "Qa", "Qi", "Sx", "Sp", "Oc", "No", "Dc",
	"UDc", "DDc", "TDc", "QaDc", "QiDc", "SxDc", "SpDc", "OcDc", "NoDc", "Vg"};
#define NSUF ((int)(sizeof SUF / sizeof SUF[0]))

static void comma(char *b) { for (; *b; b++) if (*b == '.') *b = ','; }

void fmt_num(double v, char *buf, int n) {
	if (!isfinite(v)) { snprintf(buf, n, "Unendlich"); return; }
	if (v < 0) v = 0;
	if (v < 1000) {
		if (v == floor(v) || v >= 100) snprintf(buf, n, "%.0f", floor(v));
		else if (v < 10) snprintf(buf, n, "%.2f", v);
		else snprintf(buf, n, "%.1f", v);
		comma(buf);
		return;
	}
	int e = (int)floor(log10(v));
	int k = e / 3;
	if (g_sci || k >= NSUF) {
		double m = v / pow(10, e);
		if (m >= 9.995) { m /= 10; e++; }
		snprintf(buf, n, "%.2fe%d", m, e);
		comma(buf);
		return;
	}
	double m = v / pow(10, k * 3);
	if (m >= 999.5 && k + 1 < NSUF) { m /= 1000; k++; }
	if (m < 10) snprintf(buf, n, "%.2f %s", m, SUF[k]);
	else if (m < 100) snprintf(buf, n, "%.1f %s", m, SUF[k]);
	else snprintf(buf, n, "%.0f %s", m, SUF[k]);
	comma(buf);
}

void fmt_time(double sec, char *buf, int n) {
	long s = (long)sec;
	if (s < 60) snprintf(buf, n, "%ld s", s);
	else if (s < 3600) snprintf(buf, n, "%ld min %02ld s", s / 60, s % 60);
	else if (s < 86400) snprintf(buf, n, "%ld h %02ld min", s / 3600, (s / 60) % 60);
	else snprintf(buf, n, "%ld T %ld h", s / 86400, (s / 3600) % 24);
}

// ---------- v2: Erze, Artefakte, Auftraege
const OreDef ORE[NORE] = {
	{"Ferrite", 0xB8A890, 1, 4},
	{"Cobalt", 0x4A8AFF, 3, 10},
	{"Luminite", 0x5AFFC8, 8, 25},
	{"Voidstone", 0xB45AFF, 20, 60},
	{"Starheart", 0xFFD25A, 60, 150},
};

const ArtDef ART[NART] = {
	{"Ancient Lens", AK_PROD, 0.15f}, {"Void Crystal", AK_CLICK, 0.5f}, {"Comet Heart", AK_PROD, 0.15f},
	{"Star Compass", AK_LASER, 0.25f}, {"Frozen Time", AK_OFFLINE, 0.1f}, {"Dust Crown", AK_DUST, 0.15f},
	{"Nebula Pearl", AK_PROD, 0.15f}, {"Titan Gear", AK_CARGO, 0.25f}, {"Quantum Die", AK_PROD, 0.15f},
	{"Solar Feather", AK_CLICK, 0.5f}, {"Echo Stone", AK_DUST, 0.15f}, {"Dark Prism", AK_PROD, 0.2f},
	{"Pulsar Ring", AK_LASER, 0.25f}, {"Moon Idol", AK_OFFLINE, 0.1f}, {"Rift Key", AK_PROD, 0.2f},
	{"Graviton Seed", AK_CARGO, 0.25f}, {"Phoenix Ash", AK_DUST, 0.2f}, {"Chrono Shard", AK_PROD, 0.25f},
	{"Galaxy Egg", AK_CLICK, 1.0f}, {"Omega Relic", AK_PROD, 0.5f},
};

bool art_has(Game *g, int i) { return (g->s.ext.x.artifacts >> i) & 1; }
int art_count(Game *g) {
	int n = 0;
	for (int i = 0; i < NART; i++) n += art_has(g, i);
	return n;
}
float art_bonus(Game *g, int kind) {
	float b = 0;
	for (int i = 0; i < NART; i++) if (art_has(g, i) && ART[i].kind == kind) b += ART[i].val;
	return b;
}
void art_desc(int i, char *b, int n) {
	int v = (int)(ART[i].val * 100 + 0.5f);
	switch (ART[i].kind) {
	case AK_PROD: snprintf(b, n, "Produktion +%d%%", v); break;
	case AK_CLICK: snprintf(b, n, "Klickkraft +%d%%", v); break;
	case AK_OFFLINE: snprintf(b, n, "Offline-Effizienz +%d%%", v); break;
	case AK_DUST: snprintf(b, n, "Stardust +%d%%", v); break;
	case AK_LASER: snprintf(b, n, "Schiffslaser +%d%%", v); break;
	case AK_CARGO: snprintf(b, n, "Laderaum +%d%%", v); break;
	}
}

double refinery_mult(Game *g) { return 1 + 0.25 * log10(1 + g->s.ext.x.oreValueTotal); }

static double nice(double v) {
	if (v < 10) return ceil(v);
	double p = pow(10, floor(log10(v)) - 1);
	return ceil(v / p) * p;
}

static double ct_value(Game *g, int k) {
	Save *s = &g->s;
	int arg = s->ext.x.contract[k].arg;
	switch (s->ext.x.contract[k].type) {
	case CT_OWN_GEN: return s->gens[arg];
	case CT_EARN: return s->lifeEarned;
	case CT_CLICKS: return s->clicksLife;
	case CT_METEORS: return s->meteorsLife;
	case CT_ORE: return s->ext.x.oreTotal[arg];
	case CT_SECTOR: return s->ext.x.sectorMax;
	case CT_EXPED: return s->ext.x.expeditions;
	}
	return 0;
}

static void ct_new(Game *g, int k) {
	Save *s = &g->s;
	typeof(s->ext.x.contract[0]) *c = &s->ext.x.contract[k];
	memset(c, 0, sizeof *c);
	int types[8], nt = 0;
	types[nt++] = CT_OWN_GEN;
	types[nt++] = CT_EARN;
	types[nt++] = CT_CLICKS;
	types[nt++] = CT_EXPED;
	types[nt++] = CT_ORE;
	if (s->clicksLife > 50) types[nt++] = CT_METEORS;
	if (s->ext.x.sectorMax < 12 && s->ext.x.expeditions > 0) types[nt++] = CT_SECTOR;
	// keine doppelten Typen
	int t = CT_EARN;
	for (int tries = 0; tries < 20; tries++) {
		t = types[(int)(frand() * nt) % nt];
		bool dup = false;
		for (int j = 0; j < 3; j++) if (j != k && s->ext.x.contract[j].type == t) dup = true;
		if (!dup) break;
	}
	c->type = t;
	double lvl = 1 + s->ext.x.sectorMax + log10(1 + s->lifeEarned) * 0.5;
	switch (t) {
	case CT_OWN_GEN: {
		int hi = 0;
		for (int i = 0; i < NGEN; i++) if (s->gens[i] > 0) hi = i;
		if (frand() < 0.4 && hi + 1 < NGEN && s->gens[hi] >= 5) hi++;
		c->arg = hi;
		c->target = s->gens[hi] + 5 + (int)(frand() * 4) * 5;
		break;
	}
	case CT_EARN: c->target = nice(fmax(2000, g->epsBase * (300 + frand() * 600))); break;
	case CT_CLICKS: c->target = 100 + (int)(frand() * 5) * 50; break;
	case CT_METEORS: c->target = 2 + (int)(frand() * 3); break;
	case CT_EXPED: c->target = 1 + (int)(frand() * 2); break;
	case CT_ORE: {
		int maxOre = s->ext.x.sectorMax >= 6 ? 4 : s->ext.x.sectorMax >= 4 ? 3 : s->ext.x.sectorMax >= 2 ? 2 : 1;
		c->arg = (int)(frand() * (maxOre + 1)) % (maxOre + 1);
		c->target = nice(25 + frand() * 40) / (1 + c->arg * 0.6);
		c->target = ceil(c->target);
		break;
	}
	case CT_SECTOR: c->target = s->ext.x.sectorMax + 1; break;
	}
	c->start = (t == CT_OWN_GEN || t == CT_SECTOR) ? 0 : ct_value(g, k);
	c->reward = nice(15 * pow(lvl, 1.4) * (0.8 + frand() * 0.4));
}

void contract_fill(Game *g) {
	for (int k = 0; k < 3; k++)
		if (g->s.ext.x.contract[k].type == CT_NONE) ct_new(g, k);
}

double contract_progress(Game *g, int k, double *target) {
	*target = g->s.ext.x.contract[k].target;
	double v = ct_value(g, k) - g->s.ext.x.contract[k].start;
	return v < 0 ? 0 : v;
}

int contracts_ready(Game *g) {
	int n = 0;
	for (int k = 0; k < 3; k++) {
		double t;
		if (g->s.ext.x.contract[k].type && contract_progress(g, k, &t) >= t) n++;
	}
	return n;
}

void contract_text(Game *g, int k, char *b, int n) {
	typeof(g->s.ext.x.contract[0]) *c = &g->s.ext.x.contract[k];
	char v[32];
	fmt_num(c->target, v, sizeof v);
	switch (c->type) {
	case CT_OWN_GEN: snprintf(b, n, "Besitze %s %s", v, GEN[c->arg].name); break;
	case CT_EARN: snprintf(b, n, "Verdiene %s Energie", v); break;
	case CT_CLICKS: snprintf(b, n, "Tippe %s Mal auf den Kern", v); break;
	case CT_METEORS: snprintf(b, n, "Fange %s Meteore", v); break;
	case CT_ORE: snprintf(b, n, "Baue %s %s ab", v, ORE[c->arg].name); break;
	case CT_SECTOR: snprintf(b, n, "Erreiche Sektor %s", v); break;
	case CT_EXPED: snprintf(b, n, "Fliege %s Expedition%s", v, c->target > 1 ? "en" : ""); break;
	default: b[0] = 0;
	}
}

bool contract_claim(Game *g, int k, double *energyOut, double *alloyOut) {
	double t;
	if (!g->s.ext.x.contract[k].type || contract_progress(g, k, &t) < t) return false;
	double al = g->s.ext.x.contract[k].reward;
	double en = fmax(g->epsBase, 5) * 600;
	g->s.ext.x.alloy += al;
	g->s.energy += en; g->s.runEarned += en; g->s.lifeEarned += en; g->s.bangEarned += en;
	g->s.ext.x.contractsDone += 1;
	*energyOut = en;
	*alloyOut = al;
	ct_new(g, k);
	game_recalc(g);
	return true;
}
