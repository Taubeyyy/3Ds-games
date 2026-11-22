// SKYBREAKER – Sternenkarte (dauerhafte Fähigkeiten) und Spielstand
#include "meta.hpp"

Profile PROF;

enum Eff { E_NONE, E_DMG, E_CRIT, E_FINISHER, E_ATKSPD, E_HP, E_HEALROOM, E_REVIVE, E_MOVE, E_DASHRECH, E_DASHCHG, E_DSTRIKE,
           E_CASTDMG, E_CASTCHG, E_SPECDMG, E_SPECCD, E_COINS, E_STARTCOINS, E_LUCK, E_STARTBOON };
struct Node { const char *name, *desc; int cost, parent; float x, y; int eff; float v; };
static const Node NODES[] = {
	{"Funke", "Dein Anfang. Alles wächst von hier.", 0, -1, 160, 104, E_NONE, 0},
	{"Scharfe Klinge", "+10% Schaden", 4, 0, 160, 74, E_DMG, 0.10f},
	{"Schärfere Klinge", "+10% Schaden", 8, 1, 160, 44, E_DMG, 0.10f},
	{"Krit-Auge", "+5% Krit-Chance", 12, 2, 128, 26, E_CRIT, 0.05f},
	{"Donnernder Abschluss", "3. Kombo-Schlag stärker + Schockwelle", 16, 2, 192, 26, E_FINISHER, 0},
	{"Zähigkeit", "+20 max. Leben", 4, 0, 196, 104, E_HP, 20},
	{"Eiserne Haut", "+20 max. Leben", 8, 5, 230, 104, E_HP, 20},
	{"Atempause", "Heilt 8% Leben nach jedem Raum", 12, 6, 262, 84, E_HEALROOM, 0.08f},
	{"Zweiter Atem", "Einmal pro Lauf wieder aufstehen", 24, 6, 262, 124, E_REVIVE, 1},
	{"Lebensquell", "+25 max. Leben", 16, 7, 294, 104, E_HP, 25},
	{"Leichtfuß", "+8% Lauftempo", 4, 0, 160, 134, E_MOVE, 0.08f},
	{"Schneller Atem", "Ausweichen lädt 20% schneller", 8, 10, 160, 164, E_DASHRECH, 0.2f},
	{"Dritter Schritt", "+1 Ausweich-Ladung", 16, 11, 128, 186, E_DASHCHG, 1},
	{"Flinke Hand", "+12% Angriffstempo", 14, 11, 192, 186, E_ATKSPD, 0.12f},
	{"Sternenwurf", "+25% Splitter-Schaden", 5, 0, 124, 104, E_CASTDMG, 0.25f},
	{"Splittervorrat", "+1 Splitter-Ladung", 12, 14, 90, 104, E_CASTCHG, 1},
	{"Wirbelkraft", "+25% Wirbel-Schaden", 8, 14, 58, 84, E_SPECDMG, 0.25f},
	{"Rascher Wirbel", "Wirbel lädt 30% schneller", 12, 14, 58, 124, E_SPECCD, 0.3f},
	{"Münzsucher", "+25% Münzen", 5, 0, 128, 76, E_COINS, 0.25f},
	{"Startkapital", "Starte mit 60 Münzen", 8, 18, 96, 54, E_STARTCOINS, 60},
	{"Glückskind", "Seltenere Segen öfter", 14, 19, 64, 36, E_LUCK, 0.08f},
	{"Göttliche Gunst", "Jeder Lauf startet mit einem Segen", 20, 20, 32, 20, E_STARTBOON, 1},
	{"Sprintmeister", "Sprint-Schlag +50% Schaden", 10, 10, 192, 140, E_DSTRIKE, 0.5f},
};
static const int NN = sizeof(NODES) / sizeof(NODES[0]);

static bool owned(int i) { return PROF.nodes & (1u << i); }

void loadProfile() {
	if (save::read("profil", &PROF, sizeof PROF) == 0) PROF = Profile();
	PROF.nodes |= 1;
}
void saveProfile() { save::write("profil", &PROF, sizeof PROF, 1); }

Mods baseMods() {
	Mods m;
	for (int i = 0; i < NN; i++) {
		if (!owned(i)) continue;
		const Node &n = NODES[i];
		switch (n.eff) {
		case E_DMG: m.dmg += n.v; break;
		case E_CRIT: m.crit += n.v; break;
		case E_FINISHER: m.finisher = true; break;
		case E_ATKSPD: m.atkSpeed += n.v; break;
		case E_HP: m.maxHp += int(n.v); break;
		case E_HEALROOM: m.healRoom += n.v; break;
		case E_REVIVE: m.revives += 1; break;
		case E_MOVE: m.move += n.v; break;
		case E_DASHRECH: m.dashRecharge *= 1 - n.v; break;
		case E_DASHCHG: m.dashCharges += 1; break;
		case E_CASTDMG: m.castMul += n.v; break;
		case E_CASTCHG: m.castCharges += 1; break;
		case E_SPECDMG: m.specialMul += n.v; break;
		case E_SPECCD: m.specialCd *= 1 - n.v; break;
		case E_COINS: m.coinMul += n.v; break;
		case E_DSTRIKE: m.dstrikeMul += n.v; break;
		default: break;
		}
	}
	return m;
}
float baseLuck() { return owned(20) ? 0.08f : 0; }
int startCoins() { return owned(19) ? 60 : 0; }
bool startBoon() { return owned(21); }

// ------------------------------------------------------------------ Bildschirm
static int g_sel;
static float g_t, g_buyT;

void treeEnter() { g_sel = 0; g_t = 0; g_buyT = 9; }

void treeUpdate(float dt, const Input &in) {
	g_t += dt;
	g_buyT += dt;
	for (int i = 0; i < NN; i++) if (ui::clicked(100 + i)) { g_sel = i; sfx(A.select, 0.9f + i * 0.01f); }
	// Steuerkreuz: nächster Stern in der Richtung
	Vec2 d;
	if (in.pressed(KEY_DUP)) d = Vec2(0, -1);
	if (in.pressed(KEY_DDOWN)) d = Vec2(0, 1);
	if (in.pressed(KEY_DLEFT)) d = Vec2(-1, 0);
	if (in.pressed(KEY_DRIGHT)) d = Vec2(1, 0);
	if (d.len() > 0) {
		int best = -1;
		float bs = 1e9f;
		for (int i = 0; i < NN; i++) {
			if (i == g_sel) continue;
			Vec2 v(NODES[i].x - NODES[g_sel].x, NODES[i].y - NODES[g_sel].y);
			float along = v.x * d.x + v.y * d.y;
			if (along <= 1) continue;
			float side = fabsf(v.x * d.y - v.y * d.x);
			float s = along + side * 2;
			if (s < bs) { bs = s; best = i; }
		}
		if (best >= 0) { g_sel = best; sfx(A.select); }
	}
	const Node &n = NODES[g_sel];
	bool can = !owned(g_sel) && (n.parent < 0 || owned(n.parent)) && PROF.splitter >= u32(n.cost);
	if ((ui::clicked(98) || in.pressed(KEY_A)) && !owned(g_sel)) {
		if (can) {
			PROF.splitter -= n.cost;
			PROF.nodes |= 1u << g_sel;
			saveProfile();
			g_buyT = 0;
			sfx(A.chime, 1.2f);
		} else sfx(A.error, 1, 0.5f);
	}
	if (ui::clicked(99) || in.pressed(KEY_B)) { sfx(A.click, 0.8f); gotoScene(SC_TITLE); }
}

void treeDraw(Gfx &g) {
	if (g.screen == Screen::Top) {
		titleDrawTop(g, 0);
		g.rrect(100, 10, 200, 52, 12, Color(0x0A0E2A, 0.75f), Color(0x161C48, 0.75f), -2);
		g.text(200, 14, 0.56f, Color(0xE8C060), "STERNENKARTE", ALIGN_CENTER, 0, true, -2);
		char b[48];
		snprintf(b, sizeof b, "%u Sternsplitter", unsigned(PROF.splitter));
		g.image(A.ui[UI_ICON_GEM], 140, 47, 0.45f, 0.45f, 0, Color(0xFFFFFF), 0, -2);
		g.text(156, 39, 0.46f, Color(0x8AF0FF), b, ALIGN_LEFT, 0, true, -2);
		snprintf(b, sizeof b, "Läufe %u  ·  Bester Raum %u  ·  Siege %u", unsigned(PROF.runs), unsigned(PROF.bestRoom), unsigned(PROF.wins));
		g.text(200, 222, 0.34f, Color(0xD8D0F0), b, ALIGN_CENTER, 0, true, -1);
		return;
	}
	g.rectGrad(0, 0, BOT_W, SCR_H, Color(0x0A0A22), Color(0x1A1238));
	Rng r(3);
	for (int i = 0; i < 60; i++) {
		float x = r.range(0, 320), y = r.range(0, 240);
		g.rect(x, y, 1, 1, Color(0xFFFFFF, 0.2f + 0.3f * (0.5f + 0.5f * sinf(g_t * 2 + i))));
	}
	for (int i = 1; i < NN; i++) {                                    // Verbindungen
		const Node &n = NODES[i], &p = NODES[n.parent];
		bool on = owned(i);
		g.line(p.x, p.y, n.x, n.y, on ? 2.5f : 1.5f, on ? Color(0xFFE08A, 0.9f) : Color(0x6A6A9A, 0.6f));
	}
	for (int i = 0; i < NN; i++) {
		const Node &n = NODES[i];
		bool on = owned(i), avail = !on && (n.parent < 0 || owned(n.parent));
		float k = i == g_sel ? 1.25f + 0.08f * sinf(g_t * 5) : 1;
		if (on) g.image(A.ui[UI_P_GLOW], n.x, n.y, 0.55f * k, 0.55f * k, 0, Color(0xFFE08A, 0.6f), 1);
		g.circle(n.x, n.y, 7 * k, on ? Color(0xFFF0C0) : avail ? Color(0x8AA0E8) : Color(0x3A3A5A));
		g.circle(n.x, n.y, 4 * k, on ? Color(0xFFD25A) : avail ? Color(0x2A3A7A) : Color(0x1A1A2A));
		if (i == g_sel) g.image(A.ui[UI_SELECT], n.x, n.y, 0.32f, 0.32f, g_t, Color(0xFFFFFF));
		ui::hit(100 + i, n.x - 12, n.y - 12, 24, 24);
	}
	if (g_buyT < 0.6f) {
		const Node &n = NODES[g_sel];
		g.image(A.ui[UI_P_GLOW], n.x, n.y, 1 + g_buyT * 4, 1 + g_buyT * 4, 0, Color(0xFFE08A, 1 - g_buyT / 0.6f), 1);
	}
	// Info
	const Node &n = NODES[g_sel];
	g.rrect(6, 196, 236, 40, 8, Color(0x10102A, 0.92f), Color(0x0A0A1A, 0.92f));
	g.text(14, 199, 0.4f, Color(0xFFE08A), n.name, ALIGN_LEFT);
	g.text(14, 216, 0.34f, Color(0xD8D8F0), n.desc, ALIGN_LEFT, 226);
	bool on = owned(g_sel), avail = n.parent < 0 || owned(n.parent);
	char b[24];
	if (on) snprintf(b, sizeof b, "Besitzt du");
	else snprintf(b, sizeof b, "%d", n.cost);
	Color bc = on ? Color(0x5AE07A) : !avail ? Color(0x6A6A8A) : PROF.splitter >= u32(n.cost) ? Color(0xFFE08A) : Color(0xFF7A6A);
	drawButton(g, 98, 246, 200, UI_BTN_SMALL, "", false, 0);
	if (!on) g.image(A.ui[UI_ICON_GEM], 262, 214, 0.38f, 0.38f, 0, Color(0xFFFFFF));
	g.text(on ? 278 : 286, 205, 0.4f, bc, b, ALIGN_CENTER);
	drawButton(g, 99, 4, 4, UI_BTN_SMALL, "", false, 0);
	g.text(36, 9, 0.38f, Color(0xFFFFFF), "< Menü", ALIGN_CENTER);
	snprintf(b, sizeof b, "%u", unsigned(PROF.splitter));
	g.image(A.ui[UI_ICON_GEM], 278, 18, 0.45f, 0.45f, 0, Color(0xFFFFFF));
	g.text(294, 10, 0.45f, Color(0x8AF0FF), b, ALIGN_LEFT);
}
