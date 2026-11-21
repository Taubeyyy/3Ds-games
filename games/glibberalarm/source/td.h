// Glibberalarm - Spiellogik (plattformunabhaengig)
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FW 400
#define FH 240
#define MAX_EN 600
#define MAX_PR 700
#define MAX_TW 64
#define MAX_FX 192
#define MAX_JAR 32
#define NTOWER 14
#define NMAP 6
#define NETYPE 8
#define NDIFF 3
#define PATH_W 11.0f
#define TOWER_R 8.0f
#define STEP (1.0f / 60.0f)

// ---------- Gegner
enum { E_TROPFEN, E_KLECKS, E_GLIBBER, E_WABBEL, E_RIESE, E_KOENIG, E_URGLIBBER, E_TITAN };
enum { M_GEIST = 1, M_PANZER = 2, M_REGEN = 4 };
typedef struct {
	const char *name;
	float hp, speed, radius;
	uint32_t color;
	int child, nchild;
} EnemyDef;
extern const EnemyDef EDEF[NETYPE];
extern int RBE[NETYPE];

typedef struct {
	uint8_t alive, type, mods, maxType;
	uint16_t id;
	int16_t seg;
	float d, hp, x, y, dirx, diry;
	float slowT, slowF, freezeT, acidT, acidTick, acidDmg, acidInt, regenT, wob, hitT;
} Enemy;

// ---------- Tuerme
enum { T_IGEL, T_KRABBE, T_PINGUIN, T_EULE, T_SEEIGEL, T_KAENGURU, T_AAL, T_BIENE, T_DACHS, T_SCHNECKE, T_FUCHS, T_FROSCH, T_MAULWURF, T_KATZE };
enum { K_PROJ, K_BOMB, K_FREEZE, K_SNIPER, K_RADIAL, K_BOOMER, K_CHAIN, K_FARM, K_BUFF, K_SLIME, K_PILE };
enum { TG_FIRST, TG_LAST, TG_STRONG, TG_CLOSE, NTARGET };

typedef struct {
	const char *name, *desc;
	int cost, kind;
	uint32_t color;
	const char *pathName[2];
	const char *upName[2][4];
	const char *upDesc[2][4];
	int upCost[2][4];
} TowerDef;
extern const TowerDef TDEF[NTOWER];

typedef struct {
	uint8_t alive, type, a, b, target;
	float x, y, cd, cd2, cd3, ang, flash;
	int spent;
	double pops;
	float bSpeed, bRange;
	uint8_t bCamo, bDmg, bStrip;
} Tower;

typedef struct {
	int kind;
	float range, interval, dmg, speed, spread, blast, freeze, slow, slowDur, acid, acidInt, splash, bossMult;
	float chainRange, life, radius, shockDmg, shockInt, auraSlow, iceDmg, iceInt, stormInt, fieldDmg;
	int pierce, count, blastMax, jumps, shards, minis, splashMax, chains, freezeMax;
	bool camo, ap, bounce, freezeBoss, strip, fireRing, buffCamo, stripAura;
	float buffSpeed, buffRange;
	int buffDmg, income;
	float jarBonus;
	bool homing, pileKeep;
	float hitBlast, fbInt, fbDmg, fbR, auraInt, auraDmg, auraR, pileLife, pileBlast, sprayInt, novaInt, novaDmg, novaR;
	int zapJumps, sprayN;
} TStats;

// ---------- Projektile & Effekte
enum { P_SPIKE, P_BOMB, P_MINIBOMB, P_SHARD, P_BOOMER, P_SLIME, P_ICE, P_BLADE, P_SHURIKEN, P_MAGIC, P_FIREBALL, P_PILE, P_LASER };
enum { PF_AP = 1, PF_BOUNCE = 2, PF_CAMO = 4, PF_HOMING = 8, PF_EXPLODE = 16, PF_KEEP = 32 };
typedef struct {
	uint8_t alive, kind, tower, flags, nh;
	float x, y, vx, vy, life, dmg, radius, turn, rot, bossMult;
	int pierce;
	uint16_t hits[12];
	float blast;
} Proj;

enum { FX_RING, FX_POP, FX_BOLT, FX_TRACER, FX_BLAST, FX_SPARK, FX_FREEZE, FX_FIRE, FX_COIN, FX_SHOCK };
typedef struct {
	uint8_t alive, kind;
	float x, y, x2, y2, t, max, r;
	uint32_t col;
} Fx;

typedef struct {
	uint8_t alive;
	float x, y, t;
	int value;
} Jar;

// ---------- Karten & Wellen
typedef struct {
	const char *name, *desc;
	int n;
	float pts[12][2];
	uint32_t ground1, ground2, path1, path2, deco;
	int theme;
} MapDef;
extern const MapDef MAPS[NMAP];

typedef struct {
	uint8_t type, mods;
	uint16_t count;
	float start, gap;
} Group;
typedef struct {
	Group g[14];
	int ng;
	float hpMul;
} Wave;

typedef struct {
	const char *name;
	int lives, finalWave, startCoins;
	float costMul, speedMul;
} DiffDef;
extern const DiffDef DIFF[NDIFF];

// ---------- Spielzustand
typedef struct {
	int map, diff, wave, lives;
	double coins;
	bool waveActive, won, lost, endless, autoStart;
	float waveT;
	Wave cur;
	int spawned[14];
	float nextSpawn[14];
	Enemy en[MAX_EN];
	Proj pr[MAX_PR];
	Tower tw[MAX_TW];
	Fx fx[MAX_FX];
	Jar jar[MAX_JAR];
	uint16_t nextId;
	float pathLen;
	float cum[12];
	int nseg;
	double popsTotal, popMul;
	int leaked;
	// Ereigniszaehler fuer Sound/UI (vom Frontend zurueckgesetzt)
	int evPop, evShoot, evBoom, evZap, evFreeze, evCoin, evLeak, evBossPop, evWaveEnd, evBlock;
	int waveEndBonus;
	// Fortschritt (Bloons-artig)
	float towerXp[NTOWER];
	double xpGained;
	int evUnlock; // Turm-Index+1, dessen Stufe gerade freigeschaltet wurde
	int unlockTier;
} TD;

#define TIER3_XP 1500
#define TIER4_XP 6000
enum { UL_OK, UL_MAX, UL_PATH, UL_XP };
int td_upgrade_lock(TD *g, int ti, int path);
extern const int TOWER_RANK[NTOWER];
extern const int MAP_RANK[NMAP];
int rank_of(double xp);
double rank_xp(int r); // Gesamt-XP fuer Rang r

void td_init_tables(void);
void td_new(TD *g, int map, int diff);
void td_step(TD *g);
bool td_start_wave(TD *g);
int td_wave_rbe(int n);
void td_make_wave(int n, Wave *w);

bool td_can_place(TD *g, int type, float x, float y);
int td_place(TD *g, int type, float x, float y);
int td_cost(TD *g, int type);
int td_up_cost(TD *g, int ti, int path);
bool td_can_upgrade(TD *g, int ti, int path); // ohne Geldpruefung
bool td_upgrade(TD *g, int ti, int path);
int td_sell_value(TD *g, int ti);
void td_sell(TD *g, int ti);
void td_stats(TD *g, const Tower *t, TStats *s);
int td_tower_at(TD *g, float x, float y);
void td_path_pos(TD *g, float d, float *x, float *y);
int td_collect_jar(TD *g, float x, float y, float r);
int td_enemies_alive(TD *g);
