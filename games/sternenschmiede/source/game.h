// Sternenschmiede - Spiellogik (plattformunabhaengig)
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define NGEN 14
#define MAX_GEN 32
#define MAX_UPG 384
#define MAX_PERK 32
#define MAX_SPERK 16
#define MAX_ACH 192
#define MAX_CHAL 16
#define NCHAL 6

typedef struct {
	const char *name;
	const char *desc;
	double cost, prod;
	uint32_t color; // 0xRRGGBB
} GenDef;

enum { UT_GEN, UT_CLICK, UT_CLICKPCT, UT_GLOBAL, UT_METEOR, UT_COMBO, UT_SYNERGY };
enum { RQ_GEN, RQ_EARNED, RQ_CLICKS, RQ_METEORS, RQ_SYN };

typedef struct {
	uint8_t type, rq, gen, gen2, tier;
	double rqVal, cost, val;
	char name[40];
} UpgDef;

enum {
	PK_LIGHT, PK_CLICK, PK_START, PK_OFFEFF, PK_OFFCAP, PK_METFREQ, PK_METGOLD,
	PK_COST, PK_AUTOGEN, PK_AUTOUPG, PK_FOUNDATION, PK_COMBO, PK_DUSTGAIN,
	PK_ACHBOOST, PK_AUTOCLICK, PK_NETWORK, NPERK
};
typedef struct {
	const char *name;
	double cost, growth;
	int max;
	int needPerk, needLvl; // Voraussetzung (-1 = keine)
} PerkDef;

enum { SP_PROD, SP_DUST, SP_TIME, SP_KEEPAUTO, SP_STARTDUST, SP_OFFLINE, SP_MAGNET, SP_CHALX2, NSPERK };
typedef struct {
	const char *name;
	double cost, growth;
	int max;
} SPerkDef;

enum { CH_NOCLICK, CH_DARK, CH_INFLATION, CH_ASCETIC, CH_LONELY, CH_RUSH };
typedef struct {
	const char *name, *rule, *reward;
	double goal;
} ChalDef;

enum { AC_EARNED, AC_EPS, AC_CLICKS, AC_GEN, AC_METEORS, AC_NOVAS, AC_UPGS, AC_COMBO, AC_BANGS, AC_CHAL, AC_TIME, AC_TOTALGEN, AC_EXPED, AC_SECTOR, AC_ART, AC_CONTRACT };
typedef struct {
	uint8_t cat, gen;
	double val;
} AchDef;

enum { BUFF_FRENZY, BUFF_CLICKRUSH, NBUFF };

// Spielstand (fest dimensioniert -> spaetere Erweiterungen bleiben kompatibel)
#define SAVE_MAGIC 0x53544E53u
#define SAVE_VERSION 2
typedef struct {
	uint32_t magic, version;
	double energy, runEarned, lifeEarned, bangEarned;
	double gens[MAX_GEN];
	uint32_t upg[MAX_UPG / 32];
	uint32_t ach[MAX_ACH / 32];
	double dust, dustTotal, dustBang; // ausgebbar / Bonus / seit Urknall verdient
	uint16_t perk[MAX_PERK];
	double sing, singTotal;
	uint16_t sperk[MAX_SPERK];
	uint32_t chalDone; // Bitmaske
	int32_t chalActive; // -1 = keine
	double chalTime;
	double clicksLife, clicksRun, meteorsLife;
	double novas, bangs, bestEps, bestCombo;
	double playTime, runTime;
	uint64_t lastSave; // ms
	double buffT[8];
	uint8_t autoGen, autoUpg, sciNotation, sound, stereo, buyMode, autoClickOn, pad;
	// v2: Erweiterungen im frueheren Reservebereich (gleiche Groesse -> v1-Saves bleiben gueltig)
	union {
		struct {
			double alloy, oreTotal[8], oreValueTotal;
			uint8_t ship[16];
			uint32_t artifacts;
			uint8_t sectorMax, expTutorial, pad[2];
			double expeditions;
			struct { uint8_t type, arg, done, pad; float pad2; double target, start, reward; } contract[3];
			double contractsDone;
		} x;
		double reserved[64];
	} ext;
} Save;

enum { EV_ACH, EV_METEOR, EV_METEOR_AUTO, EV_CHAL_DONE, EV_CHAL_FAIL };
typedef struct { int type, id; } GameEvent;

typedef struct {
	Save s;
	// abgeleitete Werte (jedes Frame neu)
	double eps, epsBase, clickVal, globalMult, globalMultBase, costGrowth;
	double genMult[MAX_GEN], genEps[MAX_GEN];
	double clickPct, totalGens;
	int upgCount, metUpg, comboMax;
	GameEvent ev[16];
	int evHead, evTail;
	double combo, comboHeat, comboIdle;
	double meteorTimer, meteorLife; // meteorLife>0 -> Meteor aktiv
	double autoTimer, autoClickAcc;
	bool dirty;
} Game;

extern const GenDef GEN[NGEN];
extern const PerkDef PERK[NPERK];
extern const SPerkDef SPERK[NSPERK];
extern const ChalDef CHAL[NCHAL];
extern UpgDef UPG[MAX_UPG];
extern int nUpg;
extern AchDef ACH[MAX_ACH];
extern int nAch;

extern int g_sci;
void game_init_tables(void);
void game_seed(uint32_t s);
void game_push(Game *g, int type, int id);
bool game_pop(Game *g, GameEvent *e);
double game_offline_eff(Game *g);
double game_offline_cap(Game *g);
bool gen_locked(Game *g, int i);
double dust_mult(Game *g);
void game_new(Game *g);
void game_recalc(Game *g);
void game_update(Game *g, double dt); // Echtzeit
double game_offline(Game *g, double seconds); // gibt Gewinn zurueck

double gen_cost(Game *g, int i, int n);
int gen_max_afford(Game *g, int i);
int gen_buy(Game *g, int i, int n); // n<=0 = max
bool gen_visible(Game *g, int i);
double gen_unit_prod(Game *g, int i);

bool upg_bought(Game *g, int i);
bool upg_unlocked(Game *g, int i);
double upg_cost(Game *g, int i);
bool upg_buy(Game *g, int i);
void upg_desc(Game *g, int i, char *buf, int n);
int upg_buy_all(Game *g);

double click(Game *g, bool manual); // gibt Gewinn zurueck

double nova_gain(Game *g);
void nova_do(Game *g);
double perk_cost(Game *g, int i);
bool perk_buy(Game *g, int i);
bool perk_available(Game *g, int i);
void perk_desc(Game *g, int i, char *buf, int n);

bool bang_ready(Game *g);
double bang_need(Game *g);
double bang_gain(Game *g);
void bang_do(Game *g);
double sperk_cost(Game *g, int i);
bool sperk_buy(Game *g, int i);
void sperk_desc(Game *g, int i, char *buf, int n);

void chal_start(Game *g, int c);
void chal_abort(Game *g);
double chal_progress(Game *g);

int meteor_collect(Game *g, double *gainOut); // gibt Effekt-ID zurueck
bool ach_has(Game *g, int i);
void ach_text(int i, char *name, int nn, char *desc, int nd);
int ach_count(Game *g);
int ach_check(Game *g); // gibt neuen Erfolg zurueck oder -1

void fmt_num(double v, char *buf, int n);
void fmt_time(double sec, char *buf, int n);

// ---------- v2: Erze, Artefakte, Auftraege
#define NORE 5
enum { ORE_FERRITE, ORE_COBALT, ORE_LUMINITE, ORE_VOIDSTONE, ORE_STARHEART };
typedef struct { const char *name; uint32_t color; double alloy, energySec; } OreDef;
extern const OreDef ORE[NORE];

#define NART 20
enum { AK_PROD, AK_CLICK, AK_OFFLINE, AK_DUST, AK_LASER, AK_CARGO };
typedef struct { const char *name; int kind; float val; } ArtDef;
extern const ArtDef ART[NART];
bool art_has(Game *g, int i);
float art_bonus(Game *g, int kind);
int art_count(Game *g);
void art_desc(int i, char *b, int n);

enum { CT_NONE, CT_OWN_GEN, CT_EARN, CT_CLICKS, CT_METEORS, CT_ORE, CT_SECTOR, CT_EXPED, NCT };
void contract_fill(Game *g);
double contract_progress(Game *g, int k, double *target);
void contract_text(Game *g, int k, char *b, int n);
bool contract_claim(Game *g, int k, double *energyOut, double *alloyOut);
int contracts_ready(Game *g);
double refinery_mult(Game *g);
