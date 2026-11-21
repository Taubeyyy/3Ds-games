// Star Forge - Expedition (Action-Modus): Bergbau-Schiff im Asteroidenfeld
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "game.h"

#define NSHIPUP 10
#define MAX_AST 170
#define MAX_CHUNK 240
#define MAX_DRONE 24
#define MAX_MINE 40
#define MAX_BUL 64
#define MAX_EFX 96
#define WORLD 1400.0f
#define MAX_SECTOR 12

enum { SU_LASER, SU_RATE, SU_RANGE, SU_THRUST, SU_HULL, SU_FUEL, SU_CARGO, SU_MAGNET, SU_TWIN, SU_SHIELD };
typedef struct {
	const char *name, *desc;
	double cost, growth;
	int max;
} ShipUpDef;
extern const ShipUpDef SHIPUP[NSHIPUP];

typedef struct {
	uint8_t alive, ore, relic;
	float x, y, r, hp, maxHp, rot, rotV, hitT;
	float shape[10];
} Asteroid;
typedef struct { uint8_t alive, ore; int8_t art; float x, y, vx, vy, t; } Chunk;
typedef struct { uint8_t alive; float x, y, vx, vy, hp, maxHp, cd, ang, hitT; } Drone;
typedef struct { uint8_t alive; float x, y, t; } Mine;
typedef struct { uint8_t alive; float x, y, vx, vy, t; } Bullet;
enum { XF_BEAM, XF_BOOM, XF_SPARK, XF_PICK, XF_RING };
typedef struct { uint8_t alive, kind; float x, y, x2, y2, t, max, r; uint32_t col; } XFx;

typedef struct {
	int sector, startSector;
	float x, y, vx, vy, ang, hull, maxHull, fuel, maxFuel, laserCd, hitFlash, time, thrust;
	float cargo, maxCargo;
	double ore[NORE];
	int artFound[6], nArt;
	float gateX, gateY;
	bool done, destroyed, autopilot, boosting;
	float apX, apY;
	Asteroid ast[MAX_AST];
	Chunk ch[MAX_CHUNK];
	Drone dr[MAX_DRONE];
	Mine mn[MAX_MINE];
	Bullet bl[MAX_BUL];
	XFx fx[MAX_EFX];
	int evHit, evBreak, evPickup, evHurt, evBoom, evGate, evFull, evArt, evShoot;
	float dmg, rate, range, speed, magnet, regen;
	int beams, lock;
	uint32_t seed;
} Exp;

void exp_start(Exp *e, Game *g, int sector);
void exp_step(Exp *e, Game *g, float ix, float iy, bool boost, float dt);
void exp_finish(Exp *e, Game *g, double *energy, double *alloy);
double ship_up_cost(Game *g, int i);
bool ship_up_buy(Game *g, int i);
void ship_up_desc(Game *g, int i, char *b, int n);
