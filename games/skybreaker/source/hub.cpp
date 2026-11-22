// SKYBREAKER – Zuflucht: begehbarer Ort zwischen den Läufen (Sternenaltar, Waffenständer, Übungspuppe, Tor)
#include "meta.hpp"

const char *WEAPON_NAME[3] = {"Schattenklinge", "Schattendolche", "Mondsense"};
const char *WEAPON_DESC[3] = {
	"Ausgewogen: 3 Hiebe, der letzte trifft schwer.",
	"Blitzschnell: 4 Stiche, links und rechts im Wechsel. Kurze Reichweite.",
	"Langsam und gewaltig: 2 weite Schwünge, der zweite zieht Feinde heran.",
};
const int WEAPON_COST[3] = {0, 20, 35};

enum { ST_NONE, ST_ALTAR, ST_RACK, ST_GATE, ST_DUMMY };
static const Vec3 ST_POS[5] = {Vec3(), Vec3(-2.7f, 0, -2.5f), Vec3(2.9f, 0, -2.8f), Vec3(0, 0, -4.2f), Vec3(2.4f, 0, 0.6f)};
static const char *ST_LABEL[5] = {"", "Sternenaltar", "Waffenständer", "Abstieg", "Übungspuppe"};
static int g_near, g_overlay, g_sel;
static float g_t;

void hubEnter() {
	Mods m = baseMods();
	AR.hub = true;
	AR.biome = 0;
	AR.begin(true, m);
	AR.hero.pos = Vec3(0, 0.075f, 2.4f);
	AR.spawn(EN_DUMMY, ST_POS[ST_DUMMY]);
	g_near = ST_NONE;
	g_overlay = ST_NONE;
	g_sel = int(PROF.weapon);
	g_t = 0;
	if (S.music) audio::musicVolume(0.6f);
}

static void leaveHub() { AR.hub = false; }

void hubUpdate(float dt, const Input &in) {
	g_t += dt;
	if (g_overlay == ST_RACK) {
		if (in.pressed(KEY_DLEFT) || in.pressed(KEY_DUP)) { g_sel = (g_sel + 2) % 3; sfx(A.select); }
		if (in.pressed(KEY_DRIGHT) || in.pressed(KEY_DDOWN)) { g_sel = (g_sel + 1) % 3; sfx(A.select); }
		for (int i = 0; i < 3; i++) if (ui::clicked(120 + i)) { g_sel = i; sfx(A.select); }
		bool owned = PROF.weapons & (1u << g_sel);
		if (ui::clicked(124) || in.pressed(KEY_A)) {
			if (!owned) {
				if (PROF.splitter >= u32(WEAPON_COST[g_sel])) { PROF.splitter -= WEAPON_COST[g_sel]; PROF.weapons |= 1u << g_sel; sfx(A.chime, 1.2f); saveProfile(); }
				else sfx(A.error, 1, 0.5f);
			} else { PROF.weapon = g_sel; AR.mods.weapon = g_sel; saveProfile(); sfx(A.chime, 1.4f, 0.7f); }
		}
		if (ui::clicked(125) || in.pressed(KEY_B)) { g_overlay = ST_NONE; sfx(A.click, 0.8f); }
		return;
	}
	if (g_overlay == ST_GATE) {
		if (ui::clicked(126) || in.pressed(KEY_A)) { leaveHub(); sfx(A.chime, 1.0f); gotoScene(SC_RUN); return; }
		if (ui::clicked(127) || in.pressed(KEY_B)) { g_overlay = ST_NONE; sfx(A.click, 0.8f); }
		return;
	}
	// Nächste Station bestimmen
	g_near = ST_NONE;
	float best = 1.5f;
	for (int i = ST_ALTAR; i <= ST_GATE; i++) {
		float d = Vec3(AR.hero.pos.x - ST_POS[i].x, 0, AR.hero.pos.z - ST_POS[i].z).len();
		if (d < best) { best = d; g_near = i; }
	}
	Input fin = in;
	bool act = in.pressed(KEY_A) || ui::clicked(128);
	if (g_near != ST_NONE && act) {
		fin.down &= ~KEY_A;
		sfx(A.select);
		if (g_near == ST_ALTAR) { leaveHub(); gotoScene(SC_TREE); return; }
		g_overlay = g_near;
		g_sel = int(PROF.weapon);
	}
	if (ui::clicked(129)) { leaveHub(); sfx(A.click, 0.8f); gotoScene(SC_TITLE); return; }
	if (ui::clicked(130)) { leaveHub(); sfx(A.click); gotoScene(SC_TRAINING); return; }
	AR.update(dt, fin);
	AR.hero.hp = float(AR.mods.maxHp);
}

void hubDraw(Gfx &g) {
	if (g.screen == Screen::Top) {
		AR.draw(g);
		Vec2 sp;
		float dep;
		for (int i = ST_ALTAR; i <= ST_DUMMY; i++) {
			Vec3 p = ST_POS[i] + Vec3(0, i == ST_GATE ? 3.6f : i == ST_DUMMY ? 1.9f : 2.3f, 0);
			if (!gfx3d::project(g, AR.cam, p, &sp, &dep)) continue;
			bool n = i == g_near;
			float k = n ? 1.0f : 0.75f;
			float w = g.textWidth(ST_LABEL[i], 0.4f * k) + 16;
			g.rrect(sp.x - w / 2, sp.y - 10, w, 20, 9, Color(0x0A0818, n ? 0.85f : 0.55f), Color(0x161028, n ? 0.85f : 0.55f), dep - 0.5f);
			g.text(sp.x, sp.y - 8, 0.4f * k, n ? Color(0xFFE8B0) : Color(0xC8C0E0), ST_LABEL[i], ALIGN_CENTER, 0, true, dep - 0.5f);
			if (n && i != ST_DUMMY) g.text(sp.x, sp.y + 12, 0.34f, Color(0xFFFFFF, 0.6f + 0.4f * sinf(g_t * 5)), "A", ALIGN_CENTER, 0, true, dep - 0.5f);
		}
		g.rrect(8, 8, 130, 22, 10, Color(0x0A0818, 0.7f), Color(0x161028, 0.7f), -2);
		g.text(16, 11, 0.42f, Color(0xE8C060), "ZUFLUCHT", ALIGN_LEFT, 0, true, -2);
		char b[24];
		snprintf(b, sizeof b, "%u", unsigned(PROF.splitter));
		g.image(A.ui[UI_ICON_GEM], 104, 19, 0.38f, 0.38f, 0, Color(0xFFFFFF), 0, -2);
		g.text(114, 12, 0.4f, Color(0x8AF0FF), b, ALIGN_LEFT, 0, true, -2);
		ui::drawToasts(g, 186);
		return;
	}
	g.imageAt(A.bg[BG_BOTTOM], 0, 0);
	char b[96];
	if (g_overlay == ST_RACK) {
		g.text(160, 6, 0.5f, Color(0xE8C060), "WAFFENSTÄNDER", ALIGN_CENTER);
		for (int i = 0; i < 3; i++) {
			float x = 8 + i * 104, y = 30;
			bool own = PROF.weapons & (1u << i), sel = g_sel == i, eq = PROF.weapon == u32(i);
			ui::panel(g, x, y, 96, 70, Color(sel ? 0x2A2450 : 0x14122C, 0.95f), eq ? Color(0x8AF0FF) : sel ? Color(0xE8C060) : Color(0x4A4470));
			g.text(x + 48, y + 8, 0.36f, own ? Color(0xFFFFFF) : Color(0x8A86A0), WEAPON_NAME[i], ALIGN_CENTER, 90);
			if (eq) g.text(x + 48, y + 44, 0.32f, Color(0x8AF0FF), "ausgerüstet", ALIGN_CENTER);
			else if (!own) { snprintf(b, sizeof b, "%d", WEAPON_COST[i]); g.image(A.ui[UI_ICON_GEM], x + 36, y + 52, 0.36f, 0.36f, 0, Color(0xFFFFFF)); g.text(x + 46, y + 44, 0.36f, Color(0x8AF0FF), b, ALIGN_LEFT); }
			ui::hit(120 + i, x, y, 96, 70);
		}
		g.rrect(8, 108, 304, 62, 10, Color(0x14102C, 0.95f), Color(0x0A0818, 0.95f));
		g.text(18, 114, 0.46f, Color(0xE8C060), WEAPON_NAME[g_sel], ALIGN_LEFT);
		g.text(18, 136, 0.4f, Color(0xF0ECFF), WEAPON_DESC[g_sel], ALIGN_LEFT, 286);
		bool own = PROF.weapons & (1u << g_sel);
		const char *lbl = PROF.weapon == u32(g_sel) ? "Ausgerüstet" : own ? "Ausrüsten" : PROF.splitter >= u32(WEAPON_COST[g_sel]) ? "Freischalten" : "Zu wenig Splitter";
		drawButton(g, 124, 44, 178, UI_BTN_MAIN, lbl, true, 0);
		drawButton(g, 125, 4, 4, UI_BTN_SMALL, "", false, 0);
		g.text(36, 9, 0.38f, Color(0xFFFFFF), "< Zurück", ALIGN_CENTER);
		return;
	}
	if (g_overlay == ST_GATE) {
		g.text(160, 30, 0.6f, Color(0x8AE8FF), "Abstieg beginnen?", ALIGN_CENTER);
		snprintf(b, sizeof b, "Waffe: %s", WEAPON_NAME[PROF.weapon]);
		g.text(160, 64, 0.42f, Color(0xE8E4F8), b, ALIGN_CENTER);
		g.text(160, 86, 0.36f, Color(0xB8B4D8), "Dungeon · Wächter der Tiefe · Kristallgrotten · Kristallkönigin", ALIGN_CENTER, 300);
		drawButton(g, 126, 44, 120, UI_BTN_MAIN, "Hinab in die Tiefe", true, 0);
		drawButton(g, 127, 44, 176, UI_BTN, "Noch nicht", false, 0);
		return;
	}
	g.text(160, 6, 0.5f, Color(0xE8C060), "ZUFLUCHT", ALIGN_CENTER);
	ui::panel(g, 8, 30, 304, 96, Color(0x0E1026, 0.88f), Color(0x4A4A80));
	struct L { int icon; const char *t, *d; } L4[4] = {
		{UI_ICON_GEM, "Sternenaltar", "Fähigkeiten mit Sternsplittern"},
		{UI_ICON_ANVIL, "Waffenständer", "Waffen freischalten und wählen"},
		{UI_ICON_CHALLENGE, "Übungspuppe", "Kampf ausprobieren"},
		{UI_ICON_BOSS, "Tor", "Abstieg in die Tiefe"},
	};
	for (int i = 0; i < 4; i++) {
		float y = 36 + i * 22;
		g.image(A.ui[L4[i].icon], 24, y + 9, 0.42f, 0.42f, 0, Color(0xFFFFFF));
		g.text(40, y + 1, 0.38f, Color(0xFFE8B0), L4[i].t, ALIGN_LEFT);
		g.text(140, y + 3, 0.32f, Color(0xC8C4E8), L4[i].d, ALIGN_LEFT);
	}
	snprintf(b, sizeof b, "Waffe: %s   ·   Splitter: %u", WEAPON_NAME[PROF.weapon], unsigned(PROF.splitter));
	g.text(160, 132, 0.36f, Color(0x8AF0FF), b, ALIGN_CENTER);
	if (g_near != ST_NONE) drawButton(g, 128, 44, 150, UI_BTN_MAIN, g_near == ST_ALTAR ? "Sternenkarte öffnen" : g_near == ST_RACK ? "Waffen ansehen" : "Zum Tor", true, 0);
	else g.text(160, 164, 0.36f, Color(0xB8B4D8), "Lauf zu einer Station und drücke A", ALIGN_CENTER);
	drawButton(g, 129, 6, 204, UI_BTN_SMALL, "", false, 0);
	g.text(38, 209, 0.32f, Color(0xFFFFFF), "Menü", ALIGN_CENTER);
	drawButton(g, 130, 250, 204, UI_BTN_SMALL, "", false, 0);
	g.text(282, 209, 0.32f, Color(0xFFFFFF), "Götter", ALIGN_CENTER);
	snprintf(b, sizeof b, "Läufe %u · Bester Raum %u · Siege %u", unsigned(PROF.runs), unsigned(PROF.bestRoom), unsigned(PROF.wins));
	g.text(160, 226, 0.3f, Color(0x7A74A0), b, ALIGN_CENTER);
}
