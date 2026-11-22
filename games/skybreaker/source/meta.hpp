// SKYBREAKER – Segen, Amboss, Profil (Sternenkarte), Lauf
#pragma once
#include "combat.hpp"

// ------------------------------------------------------------------ Segen der Götter
enum { SL_PASSIVE = SL_COUNT };
struct Offer {
	int god = -1, slot = 0, rarity = 0;
	bool upgrade = false;
	int anvil = -1;     // >= 0: Amboss-Verbesserung statt Segen
};
const char *boonName(int god, int slot);
void boonDesc(const Offer &o, char *out, int n);
int makeOffers(int god, Offer *out, float luck, Rng &r);   // 3 Angebote eines Gottes
void applyOffer(Mods &m, const Offer &o, Arena &ar);
int makeAnvilOffers(const Mods &m, Offer *out, Rng &r);
const char *anvilName(int i);
const char *anvilDesc(int i);
extern const char *RARITY_NAME[3];
extern const u32 RARITY_COL[3];
extern const char *GOD_LINES[CH_COUNT][2];

// ------------------------------------------------------------------ Profil + Sternenkarte
struct Profile {
	u32 splitter = 0, nodes = 1, runs = 0, wins = 0, bestRoom = 0, kills = 0;
	u32 weapons = 1, weapon = 0;   // freigeschaltete Waffen (Bits), gewählte Waffe
};
extern Profile PROF;
void loadProfile();
void saveProfile();
Mods baseMods();
float baseLuck();
int startCoins();
bool startBoon();

void hubEnter();
void hubUpdate(float dt, const Input &in);
void hubDraw(Gfx &g);
extern const char *WEAPON_NAME[3];
extern const char *WEAPON_DESC[3];
extern const int WEAPON_COST[3];

void treeEnter();
void treeUpdate(float dt, const Input &in);
void treeDraw(Gfx &g);

// ------------------------------------------------------------------ Lauf
void runEnter();
void runUpdate(float dt, const Input &in);
void runDraw(Gfx &g);
void drawGodCameo(Gfx &g, int god, float t, float appear);
void titleDrawTop(Gfx &g, float logoA);
