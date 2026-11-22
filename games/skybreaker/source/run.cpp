// SKYBREAKER – ein Lauf durch den Himmelsturm: Räume, Türen, Belohnungen, Segen, Händler, Ereignisse, Boss
#include "meta.hpp"

enum RunState { RS_ENTER, RS_FIGHT, RS_CLEARED, RS_CHOICE, RS_DOORS, RS_DEAD, RS_WIN };
enum Choice { CK_NONE, CK_BOON, CK_ANVIL, CK_SHOP, CK_EVENT };
static const int LAST_ROOM = 14, BOSS1_ROOM = 7;
static const char *ROOM_NAME[RT_COUNT] = {"Kampf", "Elite", "Schatzkammer", "Händler", "Sternenquelle", "Ereignis", "Prüfung", "Wächter der Tiefe"};
static const char *MOD_NAME[RM_COUNT] = {"", "Eile: Gegner schneller", "Glas: alle nehmen 50% mehr Schaden", "Schwarm: viele Krabbler", "Sternregen: Meteore!"};

struct ShopItem { int kind, price; bool sold; Offer o; };
enum { SH_HEAL, SH_BOON, SH_ANVIL, SH_HEART, SH_GEM, SH_POWER, SH_COUNT };
static const char *SHOP_NAME[SH_COUNT] = {"Heiltrank", "Segen", "Amboss", "Lebensherz", "Sternsplitter x5", "Stärkung"};
static const char *SHOP_DESC[SH_COUNT] = {"Heilt 35 Leben.", "Segen eines zufälligen Gottes.", "Wähle eine Waffen-Verbesserung.",
                                        "+20 max. Leben.", "Für die Sternenkarte.", "Verstärkt einen deiner Segen."};
static const int SHOP_PRICE[SH_COUNT] = {35, 90, 110, 70, 60, 80};

struct Event { const char *title, *text, *a, *b; };
static const Event EVENTS[] = {
	{"Gefallener Stern", "Ein glühender Stern liegt im Krater und summt leise.", "Berühren (-15 Leben, epischer Segen)", "Weitergehen"},
	{"Würfelgeist", "Ein kichernder Geist klappert mit Knochenwürfeln.", "Setze 40 Münzen (50%: +100)", "Ablehnen"},
	{"Verfallener Schrein", "Opfere Lebenskraft für rohe Stärke.", "-20 max. Leben, +20% Schaden", "Gehen"},
	{"Wandernde Schmiedin", "Eine alte Bekannte aus der Esse winkt dir zu.", "Amboss-Verbesserung", "Heile 30 Leben"},
	{"Sternenquelle", "Kaltes Licht sprudelt aus dem Fels.", "+6 Sternsplitter", "Heile 50% Leben"},
	{"Spiegel der Götter", "Dein Spiegelbild trägt einen deiner Segen - stärker.", "Ein Segen wird verstärkt", "Nichts anfassen"},
};
static const int NEVENTS = sizeof(EVENTS) / sizeof(EVENTS[0]);

static struct Run {
	int room, state, roomType, reward, god, mod, wave, waves;
	float st, bannerT, meteorT, quitArm, transT, fadeIn, clearT = 9;
	Door nextDoor;
	int coins, splitter, startKills;
	int choice, nOffers, sel, eventId, cameoGod;
	Offer offers[3];
	ShopItem shop[3];
	bool shopVisited, startChoice, returnToShop;
	Rng rng{1};
} R;

static float luck() { return baseLuck(); }
#ifndef __3DS__
static bool getenvBot() { return getenv("SKY_BOT") != nullptr; }
#else
static bool getenvBot() { return false; }
#endif

// ------------------------------------------------------------------ Gegnerwellen
static void spawnWave() {
	Rng &r = R.rng;
	float budget = 3.5f + R.room * 1.3f;
	if (R.roomType == RT_ELITE) budget *= 0.6f;
	if (R.roomType == RT_CHALLENGE) budget *= 1.2f;
	int n = 0;
	while (budget > 0.5f && n < 14) {
		int type;
		float x = r.f();
		if (AR.biome == 1) {
			if (R.mod == RM_SWARM && x < 0.55f) type = EN_SPIDER;
			else if (x < 0.22f) type = EN_SPIDER;
			else if (x < 0.42f) type = EN_ARCHER;
			else if (x < 0.56f) type = EN_GEODE;
			else if (x < 0.68f) type = EN_BOMBER;
			else if (x < 0.84f) type = EN_SHARD;
			else type = EN_WISP;
		} else if (R.mod == RM_SWARM && x < 0.55f) type = EN_CRAWLER;
		else if (R.room >= 5 && x < 0.12f) type = EN_BRUTE;
		else if (R.room >= 3 && x < 0.3f) type = EN_BOMBER;
		else if (R.room >= 2 && x < 0.5f) type = EN_WISP;
		else if (x < 0.75f) type = EN_SHARD;
		else type = EN_CRAWLER;
		float cost = type == EN_BRUTE ? 5 : type == EN_GEODE ? 4 : type == EN_CRAWLER ? 1 : 2;
		if (cost > budget + 0.5f) continue;
		budget -= cost;
		Vec3 p;
		for (int k = 0; k < 10; k++) {
			float a = r.range(0, 2 * PI), d = r.range(1.6f, 3.9f);
			p = Vec3(sinf(a) * d, 0, cosf(a) * d);
			if ((p - AR.hero.pos).len() > 2.3f) break;
		}
		bool elite = R.roomType == RT_ELITE && R.wave == 0 && n < (R.room >= 7 ? 2 : 1);
		int id = AR.spawn(type, p, elite, n * 0.12f);
		if (id >= 0) {
			Enemy &e = AR.enemies[id];
			float k = 1 + 0.07f * R.room;
			e.hp *= k;
			e.maxHp *= k;
			e.dmg *= 0.85f + 0.035f * R.room;
		}
		n++;
	}
	R.wave++;
}

// ------------------------------------------------------------------ Räume und Türen
static void enterRoom(int type, int reward, int god, int mod) {
	R.roomType = type;
	R.reward = reward;
	R.god = god;
	R.mod = mod;
	R.wave = 0;
	R.waves = type == RT_COMBAT || type == RT_CHALLENGE ? 1 + (R.room >= 3) + (R.room >= 8) + (type == RT_CHALLENGE) : type == RT_ELITE ? 1 + (R.room >= 6) : 0;
	R.state = RS_ENTER;
	R.st = 0;
	R.bannerT = 0;
	R.meteorT = 2;
	R.choice = CK_NONE;
	int prevBiome = AR.biome;
	AR.biome = R.room > BOSS1_ROOM ? 1 : 0;
	AR.newRoom(int(R.rng.next() % 1000) + R.room * 31);
	if (AR.biome == 1 && prevBiome == 0) ui::toast("Neuer Bereich", "Die Kristallgrotten", Color(0x8AE8FF));
	AR.dmgMulRoom = mod == RM_GLASS ? 1.5f : 1;
	AR.speedMulRoom = mod == RM_HASTE ? 1.35f : 1;
	// Räume ohne Kampf: Objekt in der Mitte
	int center = type == RT_TREASURE ? PK2_CHEST : type == RT_SHOP ? PK2_SHOP : type == RT_FOUNTAIN ? PK2_FOUNTAIN : type == RT_EVENT ? PK2_EVENT : 0;
	if (center) { AR.pickup = Pickup(); AR.pickup.kind = center; AR.pickup.pos = Vec3(0, 0.075f, -0.6f); AR.pickup.active = true; }
	if (type == RT_BOSS) {
		bool queen = R.room >= LAST_ROOM;
		int id = AR.spawn(queen ? EN_QUEEN : EN_BOSS, Vec3(0, 0, -1.8f), false, 0.4f);
		if (id >= 0) { AR.enemies[id].hp = AR.enemies[id].maxHp = queen ? 2000.f : 1300.f; }
		audio::musicVolume(S.music ? 0.85f : 0);
	}
	sfx(A.whoosh, 0.9f, 0.6f);
}

static void genDoor(Door &d, int i, int n, Rng &r) {
	d = Door();
	float a = n == 2 ? (i ? 0.42f : -0.42f) : (i - 1) * 0.62f;
	d.pos = Vec3(sinf(a) * 4.75f, 0.075f, -cosf(a) * 4.75f);
	d.yaw = -a;
	d.active = true;
	int next = R.room + 1;
	float x = r.f() * 100;
	if (next == LAST_ROOM || next == BOSS1_ROOM) { d.room = RT_BOSS; return; }
	if (next == LAST_ROOM - 1 || next == BOSS1_ROOM - 1) { d.room = i == 0 ? RT_FOUNTAIN : RT_SHOP; return; }
	if (x < 52) d.room = RT_COMBAT;
	else if (x < 62 && next >= 3) d.room = RT_ELITE;
	else if (x < 68) d.room = RT_TREASURE;
	else if (x < 76 && next >= 3 && !R.shopVisited) d.room = RT_SHOP;
	else if (x < 82 && next >= 3) d.room = RT_FOUNTAIN;
	else if (x < 91) d.room = RT_EVENT;
	else if (next >= 2) d.room = RT_CHALLENGE;
	else d.room = RT_COMBAT;
	if (d.room == RT_COMBAT || d.room == RT_ELITE || d.room == RT_CHALLENGE) {
		float y = r.f() * 100;
		bool anySlot = false;
		for (int s = 0; s < SL_COUNT; s++) if (AR.mods.slotGod[s] >= 0) anySlot = true;
		if (y < 46 || d.room == RT_ELITE) { d.reward = PK2_BOON; d.god = r.irange(CH_IGNARA, CH_SYLV); }
		else if (y < 60) d.reward = PK2_COINS;
		else if (y < 70) d.reward = PK2_HEART;
		else if (y < 80 && next >= 2) d.reward = PK2_ANVIL;
		else if (y < 92) d.reward = PK2_GEM;
		else d.reward = anySlot ? PK2_POWER : PK2_COINS;
		if (next >= 3 && d.room == RT_COMBAT && r.f() < 0.28f) d.mod = r.irange(RM_HASTE, RM_METEOR);
	}
}

static void openDoors() {
	R.state = RS_DOORS;
	R.st = 0;
	if (R.room >= LAST_ROOM) return;
	Rng &r = R.rng;
	int nx = R.room + 1;
	int n = (nx == LAST_ROOM || nx == BOSS1_ROOM) ? 1 : (nx == LAST_ROOM - 1 || nx == BOSS1_ROOM - 1) ? 2 : (r.f() < 0.45f ? 3 : 2);
	for (int i = 0; i < n; i++) {
		for (int tries = 0; tries < 8; tries++) {
			genDoor(AR.doors[i], i, n, r);
			bool dup = false;
			for (int j = 0; j < i; j++)
				if (AR.doors[j].room == AR.doors[i].room && AR.doors[j].reward == AR.doors[i].reward && AR.doors[j].god == AR.doors[i].god) dup = true;
			if (!dup) break;
		}
	}
	if (n == 1) AR.doors[0].pos = Vec3(0, 0.075f, -4.75f), AR.doors[0].yaw = 0;
	sfx(A.chime, 0.8f, 0.6f);
}

static void finishRun(bool win) {
	R.state = win ? RS_WIN : RS_DEAD;
	R.st = 0;
	R.splitter += win ? 15 : 0;
	PROF.splitter += R.splitter;
	PROF.runs++;
	if (win) PROF.wins++;
	if (u32(R.room) > PROF.bestRoom) PROF.bestRoom = R.room;
	PROF.kills += AR.totalKills;
	saveProfile();
	audio::musicVolume(S.music ? 0.5f : 0);
}

// ------------------------------------------------------------------ Belohnungen
static void openBoon(int god) {
	if (god > 0 && god < CH_COUNT) sfx(A.motif[god], 1, 0.9f);
	R.choice = CK_BOON;
	R.cameoGod = god;
	R.nOffers = makeOffers(god, R.offers, luck() + (R.roomType == RT_ELITE ? 0.15f : 0), R.rng);
	R.sel = 0;
	R.st = 0;
	R.state = RS_CHOICE;
}
static void openAnvil() {
	R.choice = CK_ANVIL;
	R.nOffers = makeAnvilOffers(AR.mods, R.offers, R.rng);
	R.sel = 0;
	R.st = 0;
	R.state = RS_CHOICE;
	if (!R.nOffers) { R.coins += 60; openDoors(); }
}
static void powerUp() {
	int s[SL_COUNT], n = 0;
	for (int i = 0; i < SL_COUNT; i++) if (AR.mods.slotGod[i] >= 0) s[n++] = i;
	if (!n) { R.coins += 50; ui::toast("Stärkung", "Noch kein Segen - 50 Münzen", Color(0xFFD25A)); return; }
	int k = s[R.rng.irange(0, n - 1)];
	AR.mods.slotPow[k] += 0.5f;
	char b[64];
	snprintf(b, sizeof b, "%s verstärkt", boonName(AR.mods.slotGod[k], k));
	ui::toast("Stärkung", b, Color(godColor(AR.mods.slotGod[k])));
}

static void takePickup() {
	switch (AR.pickup.kind) {
	case PK2_BOON: openBoon(AR.pickup.god); return;
	case PK2_ANVIL: openAnvil(); return;
	case PK2_COINS: { int c = int((40 + R.rng.irange(0, 25)) * AR.mods.coinMul * (R.mod ? 1.5f : 1)); R.coins += c; char b[32]; snprintf(b, sizeof b, "+%d Münzen", c); ui::toast("Beute", b, Color(0xFFD25A)); break; }
	case PK2_HEART: AR.mods.maxHp += 20; AR.heal(20); ui::toast("Lebensherz", "+20 max. Leben", Color(0xFF6A8A)); break;
	case PK2_GEM: R.splitter += 5; ui::toast("Sternsplitter", "+5 für die Sternenkarte", Color(0x8AF0FF)); break;
	case PK2_POWER: powerUp(); break;
	case PK2_CHEST: R.coins += int(70 * AR.mods.coinMul); R.splitter += 3; ui::toast("Schatz", "+70 Münzen, +3 Sternsplitter", Color(0xFFD25A)); break;
	case PK2_FOUNTAIN: AR.heal(AR.mods.maxHp * 0.4f); ui::toast("Sternenquelle", "Du fühlst dich erfrischt (+40%)", Color(0x7AE8FF)); break;
	case PK2_SHOP: {
		R.choice = CK_SHOP;
		R.state = RS_CHOICE;
		R.sel = 0;
		R.shopVisited = true;
		int kinds[SH_COUNT] = {0, 1, 2, 3, 4, 5};
		for (int i = SH_COUNT - 1; i > 0; i--) { int j = R.rng.irange(0, i); int t = kinds[i]; kinds[i] = kinds[j]; kinds[j] = t; }
		for (int i = 0; i < 3; i++) R.shop[i] = {kinds[i], SHOP_PRICE[kinds[i]], false, Offer()};
		return;
	}
	case PK2_EVENT: R.choice = CK_EVENT; R.state = RS_CHOICE; R.eventId = R.rng.irange(0, NEVENTS - 1); R.sel = 0; return;
	}
	openDoors();
}

static void roomCleared() {
	R.state = RS_CLEARED;
	R.st = 0;
	R.clearT = 0;
	R.splitter += 1 + (R.roomType == RT_ELITE ? 3 : 0) + (R.roomType == RT_CHALLENGE ? 3 : 0);
	if (R.roomType == RT_CHALLENGE) R.coins += 40;
	if (R.roomType == RT_ELITE) R.coins += 30;
	float h = 0.05f + AR.mods.healRoom + (AR.mods.passive[CH_LYRA] ? 0.15f : 0);
	AR.heal(AR.mods.maxHp * h);
	if (R.roomType == RT_BOSS && R.room >= LAST_ROOM) { finishRun(true); return; }
	if (R.roomType == RT_BOSS) {                       // erster Boss: große Belohnung, dann weiter in die Grotten
		R.splitter += 8;
		AR.heal(AR.mods.maxHp * 0.3f);
		R.reward = PK2_BOON;
		R.god = R.rng.irange(CH_IGNARA, CH_SYLV);
		ui::toast("Wächter besiegt!", "+8 Sternsplitter, Heilung und ein Segen", Color(0xFFE08A));
	}
	sfx(A.chime, 1.0f, 0.9f);
	if (R.reward) {
		AR.pickup = Pickup();
		AR.pickup.kind = R.reward;
		AR.pickup.god = R.god;
		AR.pickup.pos = Vec3(0, 0.075f, -0.2f);
		AR.pickup.active = true;
	} else openDoors();
}

// ------------------------------------------------------------------ Lauf starten
void runEnter() {
	R = Run();
	R.rng = Rng(u32(nowMs() * 2654435761u) | 1);
	R.room = 1;
	R.coins = startCoins();
	AR.begin(false, baseMods());
	enterRoom(RT_COMBAT, PK2_BOON, R.rng.irange(CH_IGNARA, CH_SYLV), RM_NONE);
	if (startBoon()) { R.startChoice = true; openBoon(R.rng.irange(CH_IGNARA, CH_SYLV)); }
	if (S.music) audio::musicVolume(0.5f);
}

// ------------------------------------------------------------------ Update
static void choose(int i) {
	switch (R.choice) {
	case CK_BOON:
	case CK_ANVIL:
		if (i < 0 || i >= R.nOffers) return;
		applyOffer(AR.mods, R.offers[i], AR);
		sfx(A.chime, 1.3f);
		R.choice = CK_NONE;
		if (R.startChoice) { R.startChoice = false; R.state = RS_ENTER; R.st = 0; }
		else if (R.returnToShop) { R.returnToShop = false; R.choice = CK_SHOP; R.state = RS_CHOICE; R.sel = 0; }
		else openDoors();
		break;
	case CK_EVENT: {
		R.choice = CK_NONE;
		if (i == 0) {
			switch (R.eventId) {
			case 0: AR.hero.hp = fmaxf(1, AR.hero.hp - 15); { Offer o[3]; int g = R.rng.irange(CH_IGNARA, CH_SYLV); int n = makeOffers(g, o, 1, R.rng); (void)n; R.cameoGod = g; R.choice = CK_BOON; R.nOffers = n; for (int k = 0; k < n; k++) { R.offers[k] = o[k]; R.offers[k].rarity = 2; } R.state = RS_CHOICE; return; }
			case 1: if (R.coins >= 40) { R.coins -= 40; if (R.rng.f() < 0.5f) { R.coins += 100; ui::toast("Gewonnen!", "+100 Münzen", Color(0xFFD25A)); } else ui::toast("Verloren", "Der Geist kichert...", Color(0xB070FF)); } else ui::toast("Zu wenig Münzen", "Der Geist ist enttäuscht.", Color(0xB070FF)); break;
			case 2: AR.mods.maxHp = AR.mods.maxHp > 40 ? AR.mods.maxHp - 20 : AR.mods.maxHp; AR.hero.hp = fminf(AR.hero.hp, float(AR.mods.maxHp)); AR.mods.dmg += 0.2f; ui::toast("Schrein", "+20% Schaden", Color(0xFF6A6A)); break;
			case 3: openAnvil(); return;
			case 4: R.splitter += 6; ui::toast("Sternenquelle", "+6 Sternsplitter", Color(0x8AF0FF)); break;
			case 5: powerUp(); break;
			}
		} else {
			if (R.eventId == 3) AR.heal(30);
			if (R.eventId == 4) AR.heal(AR.mods.maxHp * 0.5f);
		}
		sfx(A.click);
		openDoors();
		break;
	}
	case CK_SHOP: {
		if (i == 3) { R.choice = CK_NONE; sfx(A.click); openDoors(); return; }
		ShopItem &it = R.shop[i];
		if (it.sold || R.coins < it.price) { sfx(A.error, 1, 0.5f); return; }
		R.coins -= it.price;
		it.sold = true;
		sfx(A.chime, 1.4f, 0.8f);
		switch (it.kind) {
		case SH_HEAL: AR.heal(35); break;
		case SH_HEART: AR.mods.maxHp += 20; AR.heal(20); break;
		case SH_GEM: R.splitter += 5; break;
		case SH_POWER: powerUp(); break;
		case SH_BOON: R.returnToShop = true; openBoon(R.rng.irange(CH_IGNARA, CH_SYLV)); break;
		case SH_ANVIL: R.returnToShop = true; openAnvil(); break;
		}
		break;
	}
	}
}

// Test-Bot (nur PC, Umgebungsvariable SKY_BOT): kämpft, sammelt ein, nimmt Türen, wählt die erste Karte
#ifndef __3DS__
#include <cstdlib>
static Input botInput(const Input &real) {
	static bool on = getenv("SKY_BOT") != nullptr;
	if (!on) return real;
	Input in;
	static int frame;
	frame++;
	Vec3 h = AR.hero.pos, target = h;
	bool fight = false;
	float best = 1e9f;
	for (const Enemy &e : AR.enemies)
		if (e.alive && e.state != 5 && e.type != EN_DUMMY) { float d = (e.pos - h).len(); if (d < best) { best = d; target = e.pos; fight = true; } }
	if (!fight && AR.pickup.active) target = AR.pickup.pos;
	else if (!fight) for (const Door &d : AR.doors) if (d.active) { target = d.pos; break; }
	Vec3 d = target - h;
	d.y = 0;
	if (d.len() > (fight ? 1.2f : 0.2f)) { d = d.norm(); in.stick = Vec2(d.x, d.z); }
	if (fight && best < 2.2f && frame % 6 == 0) in.down |= KEY_A;
	if (fight && frame % 50 == 0) in.down |= KEY_X;
	if (fight && frame % 70 == 0) in.down |= KEY_Y;
	if (fight && frame % 45 == 0 && AR.hero.hp < AR.mods.maxHp * 0.5f) in.down |= KEY_B;
	if (R.state == RS_CHOICE && frame % 30 == 0) in.down |= KEY_A;
	if ((R.state == RS_DEAD || R.state == RS_WIN) && frame % 200 == 0) in.down |= KEY_B;
	in.held = in.down;
	return in;
}
#else
static Input botInput(const Input &real) { return real; }
#endif

void runUpdate(float dt, const Input &inReal) {
	Input in = botInput(inReal);
	R.st += dt;
	R.bannerT += dt;
	R.quitArm = fmaxf(0, R.quitArm - dt);
	if (ui::clicked(90)) {
		if (R.state == RS_DEAD || R.state == RS_WIN) { sfx(A.click); gotoScene(SC_HUB); return; }
		if (R.quitArm > 0) { finishRun(false); return; }
		R.quitArm = 2.5f;
		sfx(A.click);
	}
	if (R.state == RS_DEAD || R.state == RS_WIN) {
		if (ui::clicked(91) || in.pressed(KEY_A)) { sfx(A.chime); runEnter(); }
		if (in.pressed(KEY_B)) gotoScene(SC_HUB);
		AR.update(dt * 0.25f, Input());
		return;
	}
	if (R.state == RS_CHOICE) {
		int n = R.choice == CK_SHOP ? 4 : R.choice == CK_EVENT ? 2 : R.nOffers;
		if (in.pressed(KEY_DLEFT) || in.pressed(KEY_DUP)) { R.sel = (R.sel + n - 1) % n; sfx(A.select); }
		if (in.pressed(KEY_DRIGHT) || in.pressed(KEY_DDOWN)) { R.sel = (R.sel + 1) % n; sfx(A.select); }
		for (int i = 0; i < 4; i++)
			if (ui::clicked(80 + i)) {
				bool cards = R.choice == CK_BOON || R.choice == CK_ANVIL;
				if (cards && R.sel != i) { R.sel = i; sfx(A.select); return; }
				R.sel = i; choose(i); return;
			}
		if (in.pressed(KEY_A) && R.st > 0.3f) choose(R.choice == CK_SHOP && getenvBot() ? 3 : R.sel);
		return;
	}
	R.fadeIn = fmaxf(0, R.fadeIn - dt * 2.5f);
	R.clearT += dt;
	if (R.transT > 0) {
		R.transT -= dt;
		AR.hero.pos += Vec3(R.nextDoor.pos.x - AR.hero.pos.x, 0, R.nextDoor.pos.z - AR.hero.pos.z) * fminf(1, dt * 6);
		if (R.transT <= 0) {
			R.room++;
			enterRoom(R.nextDoor.room, R.nextDoor.reward, R.nextDoor.god, R.nextDoor.mod);
			R.fadeIn = 1;
		}
		return;
	}
	AR.update(dt, in);
	if (AR.coinsGained) R.coins += AR.coinsGained;
	if (AR.heroDied) { finishRun(false); return; }
	if (AR.pickupTaken) { takePickup(); return; }
	switch (R.state) {
	case RS_ENTER:
		if (R.st > 0.6f) {
			R.state = R.waves > 0 || R.roomType == RT_BOSS ? RS_FIGHT : RS_CLEARED;
			R.st = 0;
			if (R.waves > 0) spawnWave();
		}
		break;
	case RS_FIGHT:
		if (R.mod == RM_METEOR) {
			R.meteorT -= dt;
			if (R.meteorT <= 0) { R.meteorT = R.rng.range(1.2f, 2.2f); AR.meteor(AR.hero.pos + Vec3(R.rng.range(-2, 2), 0, R.rng.range(-2, 2))); }
		}
		if (AR.aliveCount() == 0 && R.st > 0.9f) {
			if (R.wave < R.waves) { spawnWave(); R.st = 0; }
			else roomCleared();
		}
		break;
	case RS_CLEARED:
		break;
	case RS_DOORS:
		if (AR.doorEntered >= 0 && R.transT <= 0) {
			R.nextDoor = AR.doors[AR.doorEntered];
			R.transT = 0.35f;
			sfx(A.whoosh, 1.2f, 0.7f);
		}
		break;
	}
}

// ------------------------------------------------------------------ Zeichnen
void drawGodCameo(Gfx &g, int god, float t, float appear) {
	Color col(godColor(god));
	g.imageAt(A.bg[BG_PANTHEON], 0, 0, 1, 1, Color(0xFFFFFF), 0, 9);
	g.image(A.ui[UI_P_GLOW], 250, 130, 5.5f, 4.5f, 0, col.alpha(0.32f), 1, 6);
	CharModel &cm = CM[god];
	float h = 0;
	for (int k = 0; k < cm.part[PK_BODY].nv; k++) h = fmaxf(h, cm.part[PK_BODY].v[k].pos[1]);
	float e = 1 - powf(1 - clampf(appear, 0, 1), 3);
	Camera3D cam;
	cam.target = Vec3(-0.8f - h * 0.15f, 0.32f + h * 0.55f, 0);
	float dist = 1.2f + h * 1.3f;
	cam.eye = cam.target + Vec3(0, 0.05f + h * 0.08f, dist);
	cam.fov = 45;
	cam.focus = dist;
	Scene3D sc;
	sc.ambient = Color(0x2E2A44);
	Light3D key; key.pos = Vec3(0.6f, 0.7f, 0.8f); key.color = Color(0xFFF2E0); key.intensity = 0.85f;
	Light3D rim; rim.pos = Vec3(-0.4f, 0.45f, -1); rim.color = col; rim.intensity = 1.3f;
	sc.lights[sc.nLights++] = key;
	sc.lights[sc.nLights++] = rim;
	gfx3d::begin(g, cam, sc);
	Material lit;
	lit.specular = 0.15f;
	gfx3d::draw(A.world[WM_PEDESTAL], Mat4::identity(), lit);
	Material rg; rg.lit = false; rg.color = col.lighten(0.3f);
	gfx3d::draw(A.world[WM_PEDESTAL_GLOW], Mat4::identity(), rg);
	float k = 0.6f + 0.4f * e;
	drawChar(god, Mat4::translate(Vec3(0, 0.32f - (1 - e) * 0.3f, 0)) * Mat4::rotY(0.35f + sinf(t * 0.4f) * 0.25f) * Mat4::scale(Vec3(k, k, k)), t, nullptr, (1 - e) * 0.8f);
	gfx3d::end(g);
	const Image &nm = A.ui[UI_NAME_SEREN + god];
	float slide = (1 - e) * -30;
	g.imageAt(nm, 14 + slide, 12, 1, 1, Color(0xFFFFFF, e), 0, -2);
	g.text(16 + slide, 12 + nm.height() + 2, 0.42f, col.lighten(0.35f).alpha(e), CHAR_INFO[god].title, ALIGN_LEFT, 0, true, -1.5f);
	g.rrect(10, 178, 250, 50, 10, Color(0x0A0A20, 0.8f * e), Color(0x14102C, 0.8f * e), -1.5f);
	g.text(20, 184, 0.4f, Color(0xF0ECFF, e), GOD_LINES[god][(god + R.room) % 2], ALIGN_LEFT, 232, true, -1.5f);
}

static void drawCard(Gfx &g, int id, float x, float y, const Offer &o, bool sel) {
	float k = sel ? 1.0f : 0.92f;
	g.image(A.ui[sel ? UI_CARD : UI_CARD_OFF], x + 48, y + 44, k, k * 0.74f, 0, Color(0xFFFFFF));
	if (sel) g.image(A.ui[UI_P_GLOW], x + 48, y + 44, 2.2f, 2.0f, 0, Color(o.anvil >= 0 ? 0xE8C060 : godColor(o.god), 0.3f), 1);
	int icon = o.anvil >= 0 ? UI_ICON_ANVIL : UI_SIGIL_SEREN + o.god;
	g.image(A.ui[icon], x + 48, y + 26, o.anvil >= 0 ? 0.75f : 0.5f, o.anvil >= 0 ? 0.75f : 0.5f, 0, Color(0xFFFFFF, sel ? 1.f : 0.8f));
	const char *nm = o.anvil >= 0 ? anvilName(o.anvil) : boonName(o.god, o.slot);
	g.text(x + 48, y + 50, 0.4f, Color(sel ? 0xFFFFFF : 0xC8C4E0), nm, ALIGN_CENTER, 92);
	if (o.anvil < 0) g.text(x + 48, y + 68, 0.34f, Color(RARITY_COL[o.rarity]), RARITY_NAME[o.rarity], ALIGN_CENTER, 92);
	ui::hit(id, x, y, 96, 90);
}

// große Beschreibung der ausgewählten Karte
static void drawOfferInfo(Gfx &g, const Offer &o) {
	g.rrect(6, 128, 308, 84, 10, Color(0x14102C, 0.95f), Color(0x0A0818, 0.95f));
	Color c(o.anvil >= 0 ? 0xE8C060 : godColor(o.god));
	g.rect(6, 128, 308, 3, c);
	const char *nm = o.anvil >= 0 ? anvilName(o.anvil) : boonName(o.god, o.slot);
	g.text(16, 134, 0.5f, c.lighten(0.3f), nm, ALIGN_LEFT);
	if (o.anvil < 0) {
		const char *sl[5] = {"Angriff", "Wirbel", "Splitter", "Ausweichen", "Gabe"};
		char b[64];
		snprintf(b, sizeof b, "%s · %s", o.upgrade ? "Verstärkung" : sl[o.slot], RARITY_NAME[o.rarity]);
		g.text(304, 137, 0.36f, Color(RARITY_COL[o.rarity]), b, ALIGN_RIGHT);
	}
	char d[200];
	boonDesc(o, d, sizeof d);
	g.text(16, 158, 0.44f, Color(0xF0ECFF), d, ALIGN_LEFT, 290);
}

static void drawHud(Gfx &g) {
	char b[64];
	float hp = AR.hero.hp, mx = float(AR.mods.maxHp);
	snprintf(b, sizeof b, "Raum %d/%d", R.room, LAST_ROOM);
	g.text(8, 6, 0.4f, Color(0xE8C060), b, ALIGN_LEFT);
	g.text(160, 6, 0.4f, Color(0xD8D0F0), ROOM_NAME[R.roomType], ALIGN_CENTER);
	g.image(A.ui[UI_ICON_COIN], 248, 14, 0.4f, 0.4f, 0, Color(0xFFFFFF));
	snprintf(b, sizeof b, "%d", R.coins);
	g.text(259, 6, 0.4f, Color(0xFFD25A), b, ALIGN_LEFT);
	g.image(A.ui[UI_ICON_GEM], 292, 14, 0.4f, 0.4f, 0, Color(0xFFFFFF));
	snprintf(b, sizeof b, "%d", R.splitter);
	g.text(303, 6, 0.4f, Color(0x8AF0FF), b, ALIGN_LEFT);
	// Leben
	g.rrect(8, 26, 304, 18, 6, Color(0x1A0A14), Color(0x1A0A14));
	float k = clampf(hp / mx, 0, 1);
	if (k > 0) g.rrect(10, 28, 300 * k, 14, 5, Color(0xFF6A7A), Color(0xC8283A));
	snprintf(b, sizeof b, "%d / %d", int(hp + 0.5f), int(mx));
	g.text(160, 28, 0.38f, Color(0xFFFFFF), b, ALIGN_CENTER);
	// Segen-Plätze
	static const char *SL[4] = {"Angriff", "Wirbel", "Splitter", "Ausweichen"};
	for (int i = 0; i < SL_COUNT; i++) {
		float x = 8 + i * 77, y = 52;
		int god = AR.mods.slotGod[i];
		ui::panel(g, x, y, 72, 50, Color(god >= 0 ? 0x1A1E44 : 0x101228, 0.9f), god >= 0 ? Color(godColor(god), 0.8f) : Color(0x3A3A5A));
		if (god >= 0) g.image(A.ui[UI_SIGIL_SEREN + god], x + 18, y + 25, 0.34f, 0.34f, 0, Color(0xFFFFFF));
		g.text(god >= 0 ? x + 48 : x + 36, y + 6, 0.3f, Color(0xB8B4D8), SL[i], ALIGN_CENTER, 44);
		if (god >= 0) {
			snprintf(b, sizeof b, "%.1fx", AR.mods.slotPow[i]);
			g.text(x + 50, y + 28, 0.34f, Color(godColor(god)), b, ALIGN_CENTER);
		}
	}
	float px = 10;
	g.text(8, 108, 0.3f, Color(0x8A86B0), "Gaben:", ALIGN_LEFT);
	px = 44;
	for (int gd = CH_IGNARA; gd < CH_COUNT; gd++)
		if (AR.mods.passive[gd]) { g.image(A.ui[UI_SIGIL_SEREN + gd], px, 114, 0.22f, 0.22f, 0, Color(0xFFFFFF)); px += 18; }
	const char *an[5] = {"Weite Hiebe", "Abschluss", "Doppelwirbel", "Fächer", "Windklinge"};
	bool ao[5] = {AR.mods.wide, AR.mods.finisher, AR.mods.doubleSpin, AR.mods.fanCast, AR.mods.bladeWave};
	float ax = 8;
	for (int i = 0; i < 5; i++)
		if (ao[i]) { g.text(ax, 126, 0.28f, Color(0xE8C060), an[i], ALIGN_LEFT); ax += g.textWidth(an[i], 0.28f) + 8; }
	// Ladungen
	snprintf(b, sizeof b, "Ausweichen %d/%d   Splitter %d/%d   Wirbel %s", AR.hero.dashCharges, AR.mods.dashCharges, AR.hero.castCharges, AR.mods.castCharges,
	         AR.hero.specialCd > 0 ? "lädt" : "bereit");
	g.text(160, 142, 0.32f, Color(0xC8C4E8), b, ALIGN_CENTER);
	if (R.mod) g.text(160, 160, 0.34f, Color(0xFF9A7A), MOD_NAME[R.mod], ALIGN_CENTER);
	const char *hint = R.state == RS_CLEARED ? (AR.pickup.active ? "Raum geschafft! Hol dir die Belohnung." : "") : R.state == RS_DOORS ? "Wähle eine Tür: Lauf hinein." : "";
	g.text(160, 178, 0.36f, Color(0xFFF0C8), hint, ALIGN_CENTER);
	drawButton(g, 90, 250, 206, UI_BTN_SMALL, "", false, 0);
	g.text(282, 211, 0.32f, R.quitArm > 0 ? Color(0xFF7A6A) : Color(0xB8B4D8), R.quitArm > 0 ? "Sicher?" : "Aufgeben", ALIGN_CENTER);
	g.text(8, 212, 0.28f, Color(0x6A6490), "A Kombo · B Ausweichen · X Wirbel · Y Splitter", ALIGN_LEFT);
}

void runDraw(Gfx &g) {
	if (g.screen == Screen::Top) {
		if (R.state == RS_CHOICE && R.choice == CK_BOON) { drawGodCameo(g, R.cameoGod, g_time, R.st * 2); return; }
		AR.draw(g);
		// Kopfzeile oben: Leben
		float k = clampf(AR.hero.hp / AR.mods.maxHp, 0, 1);
		g.rrect(8, 8, 104, 10, 4, Color(0x000000, 0.6f), Color(0x000000, 0.6f), -2);
		if (k > 0) g.rrect(9, 9, 102 * k, 8, 3, Color(0xFF6A7A), Color(0xC8283A), -2);
		for (const Enemy &e : AR.enemies)
			if (e.alive && (e.type == EN_BOSS || e.type == EN_QUEEN)) {
				g.text(200, 204, 0.42f, Color(0xFF9A7A), R.room >= LAST_ROOM ? "Kristallkönigin" : "Wächter der Tiefe", ALIGN_CENTER, 0, true, -2);
				g.rrect(60, 222, 280, 10, 4, Color(0x000000, 0.7f), Color(0x000000, 0.7f), -2);
				g.rrect(61, 223, 278 * clampf(e.hp / e.maxHp, 0, 1), 8, 3, Color(0xFF5A3A), Color(0xA0201A), -2);
			}
		if (R.bannerT < 2.2f && R.state != RS_DEAD && R.state != RS_WIN && R.roomType != RT_BOSS) {
			float a = R.bannerT < 0.3f ? R.bannerT / 0.3f : R.bannerT > 1.7f ? (2.2f - R.bannerT) / 0.5f : 1;
			char b[48];
			snprintf(b, sizeof b, "Raum %d · %s", R.room, ROOM_NAME[R.roomType]);
			g.rrect(80, 56, 240, R.mod ? 50 : 34, 12, Color(0x0A0A20, 0.75f * a), Color(0x1A1430, 0.75f * a), -2);
			g.text(200, 62, 0.55f, Color(0xFFE8B0, a), b, ALIGN_CENTER, 0, true, -2);
			if (R.mod) g.text(200, 86, 0.36f, Color(0xFF9A7A, a), MOD_NAME[R.mod], ALIGN_CENTER, 0, true, -2);
		}
		if (R.state == RS_DEAD || R.state == RS_WIN) {
			float a = clampf(R.st * 1.5f, 0, 1);
			g.rect(0, 0, 400, 240, Color(0x000000, 0.55f * a), -2);
			g.text(200, 80, 1.0f, R.state == RS_WIN ? Color(0xFFE08A, a) : Color(0xFF8A8A, a), R.state == RS_WIN ? "Die Tiefe erbebt!" : "Gefallen", ALIGN_CENTER, 0, true, -3);
			g.text(200, 120, 0.45f, Color(0xFFFFFF, a), R.state == RS_WIN ? "Die Kristallkönigin ist gefallen." : "Die Sterne tragen dich zurück an die Oberfläche.", ALIGN_CENTER, 0, true, -3);
		}
		if (R.clearT < 1.6f && R.state != RS_WIN) {
			float k = R.clearT;
			float a = k < 1.1f ? 1 : 1 - (k - 1.1f) / 0.5f;
			float sc = k < 0.15f ? 1.6f - k * 4 : 1.0f;
			g.image(A.ui[UI_P_GLOW], 200, 108, 6, 1.6f, 0, Color(0xFFE08A, 0.35f * a), 1, -2);
			g.text(200, 92, 0.95f * sc, Color(0xFFF0C8, a), "GESCHAFFT!", ALIGN_CENTER, 0, true, -3);
		}
		if (R.roomType == RT_BOSS && R.bannerT < 2.6f) {
			float a = R.bannerT < 0.3f ? R.bannerT / 0.3f : R.bannerT > 2.0f ? (2.6f - R.bannerT) / 0.6f : 1;
			g.rect(0, 92, 400, 54, Color(0x200008, 0.7f * a), -2);
			bool q = R.room >= LAST_ROOM;
			g.text(200, 98, 0.9f, q ? Color(0xC8A0FF, a) : Color(0xFF8A6A, a), q ? "KRISTALLKÖNIGIN" : "WÄCHTER DER TIEFE", ALIGN_CENTER, 0, true, -3);
			g.text(200, 128, 0.36f, Color(0xFFD8C8, a), q ? "Sie hütet das Herz des gefallenen Himmels" : "Er versperrt den Weg in die Grotten", ALIGN_CENTER, 0, true, -3);
		}
		float fa = R.transT > 0 ? 1 - R.transT / 0.35f : R.fadeIn;
		if (fa > 0) g.rect(0, 0, 400, 240, Color(0x06040E, clampf(fa, 0, 1)), -4);
		ui::drawToasts(g, 4);
		return;
	}
	g.imageAt(A.bg[BG_BOTTOM], 0, 0);
	if (R.state == RS_DEAD || R.state == RS_WIN) {
		g.text(160, 16, 0.6f, R.state == RS_WIN ? Color(0xFFE08A) : Color(0xFF9A9A), R.state == RS_WIN ? "SIEG" : "LAUF BEENDET", ALIGN_CENTER);
		char b[64];
		ui::panel(g, 20, 48, 280, 110, Color(0x0E1026, 0.9f), Color(0x4A4A80));
		snprintf(b, sizeof b, "Räume geschafft: %d", R.room - (R.state == RS_DEAD ? 1 : 0)); g.text(36, 58, 0.42f, Color(0xE8E4F8), b, ALIGN_LEFT);
		snprintf(b, sizeof b, "Besiegte Gegner: %d", AR.totalKills); g.text(36, 80, 0.42f, Color(0xE8E4F8), b, ALIGN_LEFT);
		snprintf(b, sizeof b, "Münzen: %d", R.coins); g.text(36, 102, 0.42f, Color(0xFFD25A), b, ALIGN_LEFT);
		snprintf(b, sizeof b, "+%d Sternsplitter", R.splitter); g.text(36, 126, 0.5f, Color(0x8AF0FF), b, ALIGN_LEFT);
		g.text(160, 166, 0.34f, Color(0xB8B4D8), "Gib Splitter in der Sternenkarte aus!", ALIGN_CENTER);
		drawButton(g, 91, 44, 184, UI_BTN_MAIN, "Neuer Lauf", false, 0);
		drawButton(g, 90, 250, 4, UI_BTN_SMALL, "", false, 0);
		g.text(282, 9, 0.34f, Color(0xFFFFFF), "Zuflucht", ALIGN_CENTER);
		return;
	}
	if (R.state == RS_CHOICE) {
		if (R.choice == CK_BOON || R.choice == CK_ANVIL) {
			g.text(160, 8, 0.5f, Color(0xE8C060), R.choice == CK_BOON ? "WÄHLE EINEN SEGEN" : "AMBOSS: WÄHLE EINE VERBESSERUNG", ALIGN_CENTER);
			for (int i = 0; i < R.nOffers; i++) drawCard(g, 80 + i, 8 + i * 104, 30, R.offers[i], R.sel == i);
			if (R.sel < R.nOffers) drawOfferInfo(g, R.offers[R.sel]);
			g.text(160, 218, 0.36f, Color(0xB8B4D8), "Antippen = ansehen, nochmal = nehmen  ·  A: nehmen", ALIGN_CENTER);
		} else if (R.choice == CK_SHOP) {
			g.text(160, 8, 0.5f, Color(0xFFD25A), "HÄNDLERIN", ALIGN_CENTER);
			char b[48];
			snprintf(b, sizeof b, "%d", R.coins);
			g.image(A.ui[UI_ICON_COIN], 272, 16, 0.45f, 0.45f, 0, Color(0xFFFFFF));
			g.text(286, 8, 0.45f, Color(0xFFD25A), b, ALIGN_LEFT);
			for (int i = 0; i < 3; i++) {
				const ShopItem &it = R.shop[i];
				float y = 34 + i * 50;
				bool can = !it.sold && R.coins >= it.price;
				ui::panel(g, 10, y, 300, 44, Color(R.sel == i ? 0x2A2450 : 0x14122C, 0.92f), it.sold ? Color(0x3A3A4A) : R.sel == i ? Color(0xFFD25A) : Color(0x5A5280));
				static const int IC[SH_COUNT] = {UI_ICON_FOUNTAIN, UI_ICON_POWER, UI_ICON_ANVIL, UI_ICON_HEART, UI_ICON_GEM, UI_ICON_POWER};
				g.image(A.ui[IC[it.kind]], 32, y + 22, 0.7f, 0.7f, 0, Color(0xFFFFFF, it.sold ? 0.3f : 1));
				g.text(58, y + 5, 0.42f, Color(it.sold ? 0x6A6A7A : 0xFFFFFF), SHOP_NAME[it.kind], ALIGN_LEFT);
				g.text(58, y + 23, 0.38f, Color(0xD8D4F0), SHOP_DESC[it.kind], ALIGN_LEFT);
				snprintf(b, sizeof b, it.sold ? "verkauft" : "%d", it.price);
				g.text(298, y + 12, 0.45f, it.sold ? Color(0x6A6A7A) : can ? Color(0xFFD25A) : Color(0xFF7A6A), b, ALIGN_RIGHT);
				ui::hit(80 + i, 10, y, 300, 44);
			}
			drawButton(g, 83, 44, 190, UI_BTN, "Weiter", R.sel == 3, 0);
		} else if (R.choice == CK_EVENT) {
			const Event &ev = EVENTS[R.eventId];
			g.image(A.ui[UI_ICON_EVENT], 160, 30, 0.8f, 0.8f, 0, Color(0xFFFFFF));
			g.text(160, 54, 0.5f, Color(0xC8A0FF), ev.title, ALIGN_CENTER);
			g.text(20, 76, 0.46f, Color(0xE8E4F8), ev.text, ALIGN_LEFT, 280);
			drawButton(g, 80, 44, 128, UI_BTN_MAIN, "", R.sel == 0, 0);
			g.text(160, 140, 0.42f, Color(0xFFF0C8), ev.a, ALIGN_CENTER, 224);
			drawButton(g, 81, 44, 182, UI_BTN, "", R.sel == 1, 0);
			g.text(160, 190, 0.42f, Color(0xFFFFFF), ev.b, ALIGN_CENTER, 224);
		}
		return;
	}
	drawHud(g);
}
