// SKYBREAKER – Trainingsplatz: gleiches Kampfsystem wie im Lauf, mit Übungspuppe und DPS-Anzeige
#include "meta.hpp"

static bool g_helpShown;
static int g_spawnN;

void trainingEnter() {
	AR.begin(true, baseMods());
	AR.spawn(EN_DUMMY, Vec3(0, 0, -1.4f));
	g_spawnN = 0;
	if (S.music) audio::musicVolume(0.45f);
	if (!g_helpShown) { ui::toast("Trainingsplatz", "A Kombo (halten) · B+A Sprint-Schlag · X Wirbel · Y Splitter", Color(0x8AF0FF)); g_helpShown = true; }
}

void trainingUpdate(float dt, const Input &in) {
	if (ui::clicked(71) || in.pressed(KEY_START)) { sfx(A.click, 0.8f); if (S.music) audio::musicVolume(0.75f); gotoScene(SC_TITLE); return; }
	if (ui::clicked(72)) { sfx(A.click); AR.resetStats(); }
	if (ui::clicked(73)) {                       // Gegner zum Üben (im Training nimmst du keinen Schaden)
		sfx(A.click);
		static const int T[4] = {EN_SHARD, EN_CRAWLER, EN_WISP, EN_BRUTE};
		for (int i = 0; i < 3; i++) {
			float a = i * 2.1f + g_spawnN;
			AR.spawn(T[(g_spawnN + i) % 4], Vec3(sinf(a) * 2.8f, 0, cosf(a) * 2.8f - 0.5f), false, i * 0.15f);
		}
		g_spawnN++;
	}
	AR.update(dt, in);
	AR.hero.hp = float(AR.mods.maxHp);
}

void trainingDraw(Gfx &g) {
	if (g.screen == Screen::Top) { AR.draw(g); ui::drawToasts(g, 4); return; }
	g.imageAt(A.bg[BG_BOTTOM], 0, 0);
	g.text(160, 6, 0.46f, Color(0xE8C060), "TRAININGSPLATZ", ALIGN_CENTER);
	drawButton(g, 71, 6, 4, UI_BTN_SMALL, "", false, 0);
	g.text(38, 9, 0.38f, Color(0xFFFFFF), "< Menü", ALIGN_CENTER);
	ui::panel(g, 10, 32, 300, 70, Color(0x0E1026, 0.85f), Color(0x4A4A80));
	char b[64];
	snprintf(b, sizeof b, "%.0f", AR.dps());
	g.text(24, 38, 0.36f, Color(0xA8A4C8), "Schaden / Sek.", ALIGN_LEFT);
	g.text(24, 52, 0.9f, Color(0xFFE08A), b, ALIGN_LEFT);
	snprintf(b, sizeof b, "Treffer  %d", AR.hits); g.text(160, 40, 0.38f, Color(0xE8E4F8), b, ALIGN_LEFT);
	snprintf(b, sizeof b, "Gesamt  %d", AR.total); g.text(160, 56, 0.38f, Color(0xE8E4F8), b, ALIGN_LEFT);
	snprintf(b, sizeof b, "Beste Serie  %d", AR.maxChain); g.text(160, 72, 0.38f, Color(0xE8E4F8), b, ALIGN_LEFT);
	snprintf(b, sizeof b, "Letzter  %d", AR.lastDmg); g.text(160, 88, 0.34f, Color(0xA8A4C8), b, ALIGN_LEFT);
	const Arena::Hero &h = AR.hero;
	struct Sk { const char *key, *name; float ready; int pips, maxPips; Color c; } sk[4] = {
		{"A", "Kombo", 1, h.act == 1 ? h.combo + 1 : 0, 3, Color(0x8AF0FF)},
		{"X", "Wirbel", 1 - h.specialCd / AR.mods.specialCd, -1, 0, Color(0xFFE08A)},
		{"Y", "Splitter", h.castCharges >= AR.mods.castCharges ? 1 : h.castRecharge / 2.2f, h.castCharges, AR.mods.castCharges, Color(0x6AD8FF)},
		{"B", "Ausweichen", h.dashCharges >= AR.mods.dashCharges ? 1 : h.dashRecharge / AR.mods.dashRecharge, h.dashCharges, AR.mods.dashCharges, Color(0xE8E0D0)},
	};
	for (int i = 0; i < 4; i++) {
		float x = 12 + i * 75, y = 110;
		bool rdy = sk[i].ready >= 1;
		ui::panel(g, x, y, 70, 62, Color(rdy ? 0x1A1E44 : 0x101228, 0.9f), rdy ? sk[i].c.alpha(0.8f) : Color(0x3A3A5A));
		g.circle(x + 18, y + 18, 11, rdy ? sk[i].c : Color(0x4A4A6A));
		g.text(x + 18, y + 10, 0.42f, Color(0x0A0A1A), sk[i].key, ALIGN_CENTER, 0, false);
		g.text(x + 35, y + 40, 0.34f, Color(0xE8E4F8), sk[i].name, ALIGN_CENTER, 66);
		if (!rdy) g.rect(x + 6, y + 55, 58 * clampf(sk[i].ready, 0, 1), 3, sk[i].c);
		for (int p = 0; p < sk[i].maxPips; p++) g.circle(x + 38 + p * 9, y + 18, 3, p < sk[i].pips ? sk[i].c : Color(0x3A3A5A));
	}
	drawButton(g, 72, 8, 182, UI_BTN_SMALL, "", false, 0);
	g.text(40, 187, 0.3f, Color(0xFFFFFF), "Reset", ALIGN_CENTER);
	drawButton(g, 73, 80, 182, UI_BTN, "Gegner rufen", false, 0);
	g.text(160, 224, 0.3f, Color(0x6A6490), "Im Training nimmst du keinen Schaden.", ALIGN_CENTER);
}
