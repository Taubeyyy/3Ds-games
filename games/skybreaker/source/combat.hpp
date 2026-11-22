// SKYBREAKER – Kampf: Heldin, Gegner, Elemente der Götter, Effekte, Türen und Belohnungen im Raum
#pragma once
#include "game.hpp"

enum EnemyType { EN_DUMMY, EN_SHARD, EN_CRAWLER, EN_WISP, EN_BOMBER, EN_BRUTE, EN_BOSS, EN_COUNT };
// Element = Gott (CH_IGNARA..CH_SYLV); -1 = keins
enum Slot { SL_ATTACK, SL_SPECIAL, SL_CAST, SL_DASH, SL_COUNT };

// Alle Werte der Heldin (Sternenkarte + Lauf-Upgrades)
struct Mods {
	int maxHp = 100;
	float dmg = 1, atkSpeed = 1, move = 1, crit = 0.05f, critMul = 2;
	int dashCharges = 2, castCharges = 2;
	float dashRecharge = 0.85f, specialMul = 1, castMul = 1, specialCd = 0.9f;
	float healRoom = 0, coinMul = 1, dstrikeMul = 1;
	int revives = 0;
	int slotGod[SL_COUNT] = {-1, -1, -1, -1};
	float slotPow[SL_COUNT] = {1, 1, 1, 1};
	bool passive[CH_COUNT] = {};
	// Amboss-Verbesserungen
	bool wide = false, finisher = false, doubleSpin = false, fanCast = false, bladeWave = false;
};

struct Enemy {
	int type = EN_DUMMY;
	bool alive = false, elite = false;
	Vec3 pos, vel, aim;
	float yaw = 0, hp = 1, maxHp = 1, r = 0.35f, speed = 2, dmg = 10, scale = 1;
	int state = 0;          // 0 Erscheinen, 1 Jagen, 2 Ausholen, 3 Angriff, 4 Erholen, 5 Sterben
	float st = 0, cd = 0, flash = 0, stun = 0, wobX = 0, wobZ = 0, wvx = 0, wvz = 0, bob = 0;
	float burnT = 0, burnDps = 0, slowT = 0, rootT = 0, poisonT = 0;
	int poison = 0, attackN = 0;
	bool hitDone = false;
};

struct Pickup { int kind = 0, god = -1; Vec3 pos; bool active = false; float t = 0; };
enum PickKind { PK2_NONE, PK2_BOON, PK2_COINS, PK2_HEART, PK2_ANVIL, PK2_GEM, PK2_POWER, PK2_CHEST, PK2_SHOP, PK2_FOUNTAIN, PK2_EVENT };

struct Door {
	Vec3 pos;
	float yaw = 0;
	int room = 0, reward = 0, god = -1, mod = 0;
	bool active = false;
	float t = 0;
};

struct Obstacle { int mesh; Vec3 pos; float r, yaw, scale; bool fire; };

class Arena {
public:
	// --- Heldin
	struct Hero {
		Vec3 pos, vel;
		float yaw = PI, hp = 100;
		int act = 0, combo = 0;
		float actT = 0, actDur = 0;
		bool hitDone = false, queued = false, strikeQueued = false;
		float comboWindow = 0, dashCd = 0, specialCd = 0, castRecharge = 0, dashRecharge = 0, afterDash = 9;
		int castCharges = 2, dashCharges = 2;
		Vec3 dashDir;
		float ghostTimer = 0, stepT = 0, flash = 0, iframe = 0, armorT = 0, critNext = 0, dead = 0, runPhase = 0, runK = 0;
		Pose pose;
	} hero;
	Mods mods;
	Enemy enemies[24];
	Pickup pickup;
	Door doors[3];
	Obstacle obst[10];
	int nObst = 0;
	float radius = 4.35f;
	bool training = false;
	// Ereignisse dieses Bildes (für die Lauf-Logik)
	int kills = 0, doorEntered = -1;
	bool pickupTaken = false, heroDied = false;
	int totalKills = 0, coinsGained = 0;
	float dmgMulRoom = 1, speedMulRoom = 1;  // Raum-Modifikatoren
	Color floorTint{0xFFFFFF};
	// Statistik (Training)
	int hits = 0, total = 0, chain = 0, maxChain = 0, lastDmg = 0;

	void begin(bool trainingMode, const Mods &m);
	void newRoom(int layoutSeed, bool keepHero = true);
	void clearEnemies();
	int spawn(int type, Vec3 pos, bool elite = false, float delay = 0);
	int aliveCount() const;
	void update(float dt, const Input &in);
	void draw(Gfx &g);
	void heal(float amount);
	void meteor(Vec3 p);
	float dps() const;
	void resetStats();
	Camera3D cam;
	float time = 0;
};
extern Arena AR;

// Element-Farben (Index = Gott)
u32 godColor(int god);

enum RoomType { RT_COMBAT, RT_ELITE, RT_TREASURE, RT_SHOP, RT_FOUNTAIN, RT_EVENT, RT_CHALLENGE, RT_BOSS, RT_COUNT };
enum RoomMod { RM_NONE, RM_HASTE, RM_GLASS, RM_SWARM, RM_METEOR, RM_COUNT };
