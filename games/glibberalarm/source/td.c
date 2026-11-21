// Glibberalarm - Spiellogik
#include "td.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const EnemyDef EDEF[NETYPE] = {
	[E_TROPFEN] = {"Tropfen", 1, 58, 4.5f, 0x7CE85A, -1, 0},
	[E_KLECKS] = {"Klecks", 1, 64, 6, 0x4AC8F0, E_TROPFEN, 2},
	[E_GLIBBER] = {"Glibber", 2, 70, 7.5f, 0xF0D040, E_KLECKS, 2},
	[E_WABBEL] = {"Wabbel", 3, 74, 8.5f, 0xF08A30, E_GLIBBER, 2},
	[E_RIESE] = {"Riesenglibber", 6, 46, 11, 0xE0508C, E_WABBEL, 3},
	[E_KOENIG] = {"Glibberkönig", 120, 26, 17, 0x9A5AF0, E_RIESE, 4},
	[E_URGLIBBER] = {"Urglibber", 700, 19, 22, 0x3A3A5A, E_KOENIG, 3},
	[E_TITAN] = {"Glibber-Titan", 3200, 15, 27, 0xC02A2A, E_URGLIBBER, 3},
};
int RBE[NETYPE];

const DiffDef DIFF[NDIFF] = {
	{"Leicht", 150, 40, 750, 0.85f, 1.0f},
	{"Normal", 100, 60, 650, 1.0f, 1.0f},
	{"Schwer", 50, 80, 600, 1.08f, 1.1f},
};

const MapDef MAPS[NMAP] = {
	{"Grüne Wiese", "Lange Wege, ideal zum Lernen.", 10,
		{{-20, 50}, {80, 50}, {80, 190}, {180, 190}, {180, 60}, {290, 60}, {290, 190}, {350, 190}, {350, 110}, {420, 110}},
		0x4A9A3A, 0x3E8A32, 0xD8C08A, 0xA88C5A, 0x2E6A28, 0},
	{"Sonnenstrand", "Zickzack am Meer entlang.", 7,
		{{-20, 120}, {70, 40}, {150, 120}, {230, 40}, {310, 120}, {390, 200}, {420, 200}},
		0xE8D08A, 0xDCC27A, 0x9AD8F0, 0x5AA8D0, 0xC0A060, 1},
	{"Kristallhöhle", "Eine Spirale in die Tiefe.", 8,
		{{420, 30}, {30, 30}, {30, 210}, {370, 210}, {370, 90}, {100, 90}, {100, 150}, {290, 150}},
		0x3A3A6A, 0x34345E, 0x7A6AB0, 0x52468A, 0x8AE0FF, 2},
	{"Vulkan", "Kurzer Weg. Nur für Profis!", 5,
		{{-20, 120}, {120, 120}, {200, 40}, {280, 200}, {420, 120}},
		0x5A3A30, 0x4E322A, 0x3A2A26, 0xE0602A, 0xFF8A3A, 3},
};

#define U(...) {__VA_ARGS__}
const TowerDef TDEF[NTOWER] = {
	[T_IGEL] = {"Stachel-Igel", "Wirft Stacheln auf einzelne Glibber. Billig und zuverlässig.", 170, K_PROJ, 0xB08050,
		{"Stacheln", "Tempo"},
		U(U("Spitze Stacheln", "Glühstacheln", "Stachelspeer", "Durchbohrer"), U("Flinke Pfoten", "Doppelwurf", "Dreifachsalve", "Stachelsturm")),
		U(U("Durchschlag +1", "Schaden +1, trifft Panzer", "Durchschlag +6, Schaden +1", "Durchschlag +12, Schaden +3"),
			U("Wirft 25% schneller", "Wirft 2 Stacheln", "3 Stacheln, noch schneller", "5 Stacheln, extrem schnell")),
		U(U(90, 200, 550, 2400), U(100, 220, 500, 2600))},
	[T_KRABBE] = {"Kracher-Krabbe", "Schießt Bomben mit Flächenschaden. Knackt Panzer, sieht keine Geister.", 450, K_BOMB, 0xE0503A,
		{"Sprengkraft", "Splitter"},
		U(U("Größere Bomben", "Schwere Ladung", "Bunkerknacker", "Megabombe"), U("Schnellzünder", "Splitterbomben", "Streubomben", "Bombenteppich")),
		U(U("Größerer Radius, mehr Treffer", "Schaden +1", "x4 Schaden gegen Bosse", "Riesiger Radius, Schaden +4"),
			U("Feuert 25% schneller", "Explosion schleudert 6 Splitter", "Explosion streut 4 Minibomben", "Doppelt so schnell, 8 Minibomben")),
		U(U(250, 450, 1100, 3500), U(220, 400, 900, 3200))},
	[T_PINGUIN] = {"Frost-Pinguin", "Friert Glibber in der Nähe ein. Bosse sind anfangs immun.", 300, K_FREEZE, 0x8AD0F0,
		{"Kälte", "Arktis"},
		U(U("Bitterkalt", "Eissplitter", "Permafrost", "Eiszeit"), U("Weite Kälte", "Frostfeld", "Eisschollen", "Gletscherherz")),
		U(U("Friert länger ein", "Einfrieren macht 1 Schaden", "Danach 50% langsamer, mehr Reichweite", "Friert auch Bosse, Schaden 3"),
			U("Mehr Reichweite", "Glibber in Reichweite 35% langsamer", "Schießt Eisschollen", "Einfrieren entfernt Panzer und Heilung")),
		U(U(150, 300, 900, 3000), U(120, 350, 800, 2800))},
	[T_EULE] = {"Scharf-Eule", "Trifft jeden Glibber auf der Karte. Langsam, aber stark.", 350, K_SNIPER, 0xA08AD0,
		{"Präzision", "Feuerrate"},
		U(U("Scharfe Krallen", "Nachtsicht", "Panzerbrecher", "Todesstoß"), U("Wachsam", "Doppelflug", "Sturzflug", "Eulenschwarm")),
		U(U("Schaden +2", "Sieht Geister-Glibber", "Schaden +4, trifft Panzer", "Schaden +18, x3 gegen Bosse"),
			U("Greift 30% schneller an", "Greift noch schneller an", "Trifft auch Glibber daneben", "Extrem schnelle Angriffe")),
		U(U(300, 250, 1200, 4500), U(300, 600, 1500, 4200))},
	[T_SEEIGEL] = {"Seeigel", "Schießt Stacheln in alle Richtungen. Gut an Kurven.", 280, K_RADIAL, 0x7A4AB0,
		{"Stacheln", "Wirbel"},
		U(U("Mehr Stacheln", "Lange Stacheln", "Stachelkranz", "Feuerring"), U("Schneller", "Noch schneller", "Klingenwirbel", "Wirbelsturm")),
		U(U("10 statt 8 Stacheln", "Mehr Reichweite, Durchschlag +1", "16 Stacheln, schneller", "Brennender Ring trifft alles"),
			U("Feuert 25% schneller", "Feuert nochmal 25% schneller", "Klingen: +1 Schaden, trifft Panzer", "Irrsinnig schnell")),
		U(U(150, 200, 700, 2800), U(160, 300, 900, 3000))},
	[T_KAENGURU] = {"Bumerang-Känguru", "Bumerang fliegt im Bogen und trifft viele Glibber.", 330, K_BOOMER, 0xD09A5A,
		{"Wurf", "Abpraller"},
		U(U("Scharfe Kante", "Glühbumerang", "Doppelbumerang", "Plasmabumerang"), U("Abpraller", "Kraftwurf", "Bumerang-Hagel", "Goldener Bumerang")),
		U(U("Durchschlag +4", "Schaden +1, trifft Panzer", "Wirft 2 Bumerangs", "Schaden +3, Durchschlag +20, schneller"),
			U("Springt von Glibber zu Glibber", "Wirft 30% schneller", "Wirft 40% schneller", "x3 gegen Bosse, Durchschlag +30")),
		U(U(200, 350, 900, 3200), U(260, 400, 1100, 3600))},
	[T_AAL] = {"Blitz-Aal", "Blitze springen zwischen Glibbern. Trifft Panzer.", 500, K_CHAIN, 0xF0E04A,
		{"Spannung", "Frequenz"},
		U(U("Starkstrom", "Hochspannung", "Kugelblitz", "Gewittersturm"), U("Schnellladung", "Doppelaal", "Elektrofeld", "Supraleiter")),
		U(U("Blitz springt 3x mehr", "Schaden +1", "8 Sprünge mehr, Schaden +1", "Alle 6 s trifft ein Sturm ALLE Glibber"),
			U("Blitzt 25% schneller", "Zwei Blitze gleichzeitig", "Lädt Glibber in der Nähe auf", "Extrem schnell, mehr Sprünge")),
		U(U(300, 550, 1400, 5000), U(300, 600, 1300, 4000))},
	[T_BIENE] = {"Honig-Bienen", "Produziert Honig (Münzen) jede Welle. Tippe die Gläser an!", 900, K_FARM, 0xF0C02A,
		{"Ertrag", "Bank"},
		U(U("Mehr Blumen", "Fleißige Bienen", "Königinnen", "Honigimperium"), U("Bienenwachs", "Sammelbienen", "Wabenbank", "Goldhonig")),
		U(U("+30 Münzen pro Welle", "+60 Münzen pro Welle", "+150 Münzen pro Welle", "+450 Münzen pro Welle"),
			U("Honig 15% wertvoller", "Sammelt Gläser automatisch", "+10% Zinsen pro Welle (max 500)", "Alle Plopps geben 25% mehr")),
		U(U(400, 800, 2200, 7000), U(350, 500, 1800, 6000))},
	[T_DACHS] = {"Trommel-Dachs", "Trommelt und macht Türme in der Nähe schneller.", 600, K_BUFF, 0x707A8A,
		{"Rhythmus", "Lärm"},
		U(U("Takt", "Lauscher", "Kriegstrommel", "Donnertrommel"), U("Laut", "Schallwelle", "Panzerknacker-Takt", "Schallkanone")),
		U(U("Türme 25% schneller", "Türme sehen Geister", "Türme +15% Reichweite", "Türme 55% schneller, +1 Schaden"),
			U("Mehr Reichweite", "Schallwelle: 1 Schaden in Reichweite", "Glibber in Reichweite verlieren Panzer", "Starke, schnelle Schallwellen")),
		U(U(400, 600, 1500, 5000), U(250, 500, 1200, 4000))},
	[T_SCHNECKE] = {"Säure-Schnecke", "Verlangsamt mit Schleim, Säure ätzt durch Panzer.", 260, K_SLIME, 0x9AD04A,
		{"Säure", "Kleister"},
		U(U("Ätzend", "Zersetzend", "Säurebad", "Auflöser"), U("Zäh", "Klebespritzer", "Dauerkleber", "Schleimflut")),
		U(U("Säure wirkt schneller", "Schleim entfernt Panzer", "Säure-Schaden 3", "Säure-Schaden 8"),
			U("55% langsamer", "Spritzt auf mehrere Glibber", "Schleim hält 8 s", "Sehr schnell, riesige Spritzer")),
		U(U(250, 450, 1100, 3800), U(180, 350, 800, 3000))},
};

// ---------- Hilfen
static float frand(void) { return (rand() & 0xFFFF) / 65535.0f; }
static float dist2(float ax, float ay, float bx, float by) { float dx = ax - bx, dy = ay - by; return dx * dx + dy * dy; }

static void fx(TD *g, int kind, float x, float y, float x2, float y2, float max, float r, uint32_t col) {
	for (int i = 0; i < MAX_FX; i++)
		if (!g->fx[i].alive) {
			Fx *f = &g->fx[i];
			f->alive = 1; f->kind = kind; f->x = x; f->y = y; f->x2 = x2; f->y2 = y2; f->t = 0; f->max = max; f->r = r; f->col = col;
			return;
		}
}

void td_init_tables(void) {
	for (int t = 0; t < NETYPE; t++) {
		int r = 1;
		if (EDEF[t].nchild) r += EDEF[t].nchild * RBE[EDEF[t].child];
		RBE[t] = r;
	}
}

static void setup_path(TD *g) {
	const MapDef *m = &MAPS[g->map];
	g->nseg = m->n - 1;
	float c = 0;
	g->cum[0] = 0;
	for (int i = 0; i < g->nseg; i++) {
		c += sqrtf(dist2(m->pts[i][0], m->pts[i][1], m->pts[i + 1][0], m->pts[i + 1][1]));
		g->cum[i + 1] = c;
	}
	g->pathLen = c;
}

void td_new(TD *g, int map, int diff) {
	memset(g, 0, sizeof *g);
	g->map = map;
	g->diff = diff;
	g->lives = DIFF[diff].lives;
	g->coins = DIFF[diff].startCoins;
	g->popMul = 1;
	setup_path(g);
}

void td_path_pos(TD *g, float d, float *x, float *y) {
	const MapDef *m = &MAPS[g->map];
	int s = 0;
	while (s < g->nseg - 1 && d > g->cum[s + 1]) s++;
	float L = g->cum[s + 1] - g->cum[s];
	float t = L > 0 ? (d - g->cum[s]) / L : 0;
	if (t < 0) t = 0;
	if (t > 1) t = 1;
	*x = m->pts[s][0] + (m->pts[s + 1][0] - m->pts[s][0]) * t;
	*y = m->pts[s][1] + (m->pts[s + 1][1] - m->pts[s][1]) * t;
}

static void enemy_pos(TD *g, Enemy *e) {
	const MapDef *m = &MAPS[g->map];
	int s = e->seg < 0 ? 0 : e->seg;
	while (s < g->nseg - 1 && e->d > g->cum[s + 1]) s++;
	e->seg = s;
	float L = g->cum[s + 1] - g->cum[s];
	float t = L > 0 ? (e->d - g->cum[s]) / L : 0;
	if (t < 0) t = 0;
	if (t > 1) t = 1;
	float dx = m->pts[s + 1][0] - m->pts[s][0], dy = m->pts[s + 1][1] - m->pts[s][1];
	e->x = m->pts[s][0] + dx * t;
	e->y = m->pts[s][1] + dy * t;
	float l = sqrtf(dx * dx + dy * dy);
	if (l > 0) { e->dirx = dx / l; e->diry = dy / l; }
}

static int spawn_enemy(TD *g, int type, int mods, float d, float hpMul) {
	for (int i = 0; i < MAX_EN; i++)
		if (!g->en[i].alive) {
			Enemy *e = &g->en[i];
			memset(e, 0, sizeof *e);
			e->alive = 1;
			e->type = type;
			e->mods = mods;
			e->maxType = type;
			e->id = ++g->nextId;
			if (!e->id) e->id = ++g->nextId;
			e->d = d;
			e->hp = EDEF[type].hp * hpMul;
			e->wob = frand() * 6.28f;
			e->seg = 0;
			enemy_pos(g, e);
			return i;
		}
	return -1;
}

// ---------- Raster fuer schnelle Nachbarsuche
#define CELL 24
#define GW (FW / CELL + 1)
#define GH (FH / CELL + 1)
static int16_t gridHead[GW * GH], gridNext[MAX_EN];

static void build_grid(TD *g) {
	for (int i = 0; i < GW * GH; i++) gridHead[i] = -1;
	for (int i = 0; i < MAX_EN; i++) {
		Enemy *e = &g->en[i];
		if (!e->alive) continue;
		int cx = (int)(e->x / CELL), cy = (int)(e->y / CELL);
		if (cx < 0) cx = 0;
		if (cy < 0) cy = 0;
		if (cx >= GW) cx = GW - 1;
		if (cy >= GH) cy = GH - 1;
		int c = cy * GW + cx;
		gridNext[i] = gridHead[c];
		gridHead[c] = i;
	}
}

static int query(TD *g, float x, float y, float r, int *out, int max) {
	int n = 0;
	int x0 = (int)((x - r - 28) / CELL), x1 = (int)((x + r + 28) / CELL);
	int y0 = (int)((y - r - 28) / CELL), y1 = (int)((y + r + 28) / CELL);
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= GW) x1 = GW - 1;
	if (y1 >= GH) y1 = GH - 1;
	for (int cy = y0; cy <= y1; cy++)
		for (int cx = x0; cx <= x1; cx++)
			for (int i = gridHead[cy * GW + cx]; i >= 0; i = gridNext[i]) {
				Enemy *e = &g->en[i];
				if (!e->alive) continue;
				float rr = r + EDEF[e->type].radius;
				if (dist2(x, y, e->x, e->y) <= rr * rr && n < max) out[n++] = i;
			}
	return n;
}

// ---------- Schaden
static void pop_enemy(TD *g, int ei, float leftover, int tower);

// Rueckgabe: 1 = geplatzt, 0 = getroffen, -1 = abgeblockt
static int damage(TD *g, int ei, float dmg, int flags, int tower, float bossMult) {
	Enemy *e = &g->en[ei];
	if (!e->alive) return 0;
	if ((e->mods & M_PANZER) && !(flags & PF_AP)) {
		g->evBlock++;
		return -1;
	}
	if (e->type >= E_KOENIG) dmg *= bossMult;
	e->hp -= dmg;
	e->hitT = 0.12f;
	if (e->hp <= 0) {
		pop_enemy(g, ei, -e->hp, tower);
		return 1;
	}
	return 0;
}

static void pop_enemy(TD *g, int ei, float leftover, int tower) {
	Enemy *e = &g->en[ei];
	Enemy old = *e;
	e->alive = 0;
	g->coins += 1 * g->popMul;
	g->popsTotal += 1;
	g->evPop++;
	if (old.type >= E_KOENIG) g->evBossPop++;
	if (tower >= 0 && tower < MAX_TW && g->tw[tower].alive) g->tw[tower].pops += 1;
	fx(g, FX_POP, old.x, old.y, 0, 0, 0.35f, EDEF[old.type].radius, EDEF[old.type].color);
	const EnemyDef *d = &EDEF[old.type];
	if (d->nchild <= 0) return;
	float hpMul = g->cur.hpMul > 0 ? g->cur.hpMul : 1;
	int first = -1;
	for (int k = 0; k < d->nchild; k++) {
		float nd = old.d - k * 5.0f;
		if (nd < 0) nd = 0;
		int ci = spawn_enemy(g, d->child, old.mods & (M_GEIST | M_REGEN), nd, hpMul);
		if (ci < 0) break;
		Enemy *c = &g->en[ci];
		c->maxType = old.maxType;
		c->slowT = old.slowT;
		c->slowF = old.slowF;
		c->seg = old.seg;
		enemy_pos(g, c);
		if (first < 0) first = ci;
	}
	if (first >= 0 && leftover > 0.001f) damage(g, first, leftover, PF_AP, tower, 1);
}

// ---------- Tuerme
int td_cost(TD *g, int type) { return (int)(roundf(TDEF[type].cost * DIFF[g->diff].costMul / 5) * 5); }
int td_up_cost(TD *g, int ti, int p) {
	Tower *t = &g->tw[ti];
	int cur = p ? t->b : t->a;
	if (cur >= 4) return 0;
	return (int)(roundf(TDEF[t->type].upCost[p][cur] * DIFF[g->diff].costMul / 5) * 5);
}

void td_stats(TD *g, const Tower *t, TStats *s) {
	(void)g;
	memset(s, 0, sizeof *s);
	int a = t->a, b = t->b;
	s->kind = TDEF[t->type].kind;
	s->bossMult = 1;
	s->freezeMax = 30;
	s->chains = 1;
	switch (t->type) {
	case T_IGEL:
		s->range = 62; s->interval = 0.95f; s->dmg = 1; s->pierce = 2; s->speed = 320; s->count = 1; s->spread = 0.17f; s->radius = 3; s->life = 0.45f;
		if (a >= 1) s->pierce += 1;
		if (a >= 2) { s->dmg += 1; s->ap = true; }
		if (a >= 3) { s->pierce += 6; s->dmg += 1; s->radius = 5; }
		if (a >= 4) { s->pierce += 12; s->dmg += 3; }
		if (b >= 1) s->interval *= 0.8f;
		if (b >= 2) s->count = 2;
		if (b >= 3) { s->count = 3; s->interval *= 0.85f; }
		if (b >= 4) { s->count = 5; s->interval *= 0.4f; }
		break;
	case T_KRABBE:
		s->range = 70; s->interval = 1.3f; s->dmg = 1; s->blast = 18; s->blastMax = 14; s->speed = 220; s->radius = 4; s->life = 0.8f; s->ap = true;
		if (a >= 1) { s->blast += 6; s->blastMax += 8; }
		if (a >= 2) s->dmg += 1;
		if (a >= 3) s->bossMult = 4;
		if (a >= 4) { s->blast += 16; s->dmg += 4; s->blastMax += 20; }
		if (b >= 1) s->interval *= 0.8f;
		if (b >= 2) s->shards = 6;
		if (b >= 3) s->minis = 4;
		if (b >= 4) { s->interval *= 0.5f; s->minis = 8; }
		break;
	case T_PINGUIN:
		s->range = 44; s->interval = 2.2f; s->freeze = 1.2f; s->dmg = 0; s->ap = true; s->camo = true;
		if (a >= 1) s->freeze = 1.7f;
		if (a >= 2) s->dmg = 1;
		if (a >= 3) { s->slow = 0.5f; s->slowDur = 2; s->range += 10; }
		if (a >= 4) { s->freezeBoss = true; s->dmg = 3; s->interval *= 0.7f; }
		if (b >= 1) s->range += 12;
		if (b >= 2) s->auraSlow = 0.35f;
		if (b >= 3) { s->iceDmg = 2; s->iceInt = 0.6f; }
		if (b >= 4) s->strip = true;
		break;
	case T_EULE:
		s->range = 999; s->interval = 1.6f; s->dmg = 2;
		if (a >= 1) s->dmg += 2;
		if (a >= 2) s->camo = true;
		if (a >= 3) { s->dmg += 4; s->ap = true; }
		if (a >= 4) { s->dmg += 18; s->bossMult = 3; }
		if (b >= 1) s->interval *= 0.7f;
		if (b >= 2) s->interval *= 0.6f;
		if (b >= 3) { s->splash = 16; s->splashMax = 5; }
		if (b >= 4) s->interval *= 0.3f;
		break;
	case T_SEEIGEL:
		s->range = 40; s->interval = 1.4f; s->count = 8; s->dmg = 1; s->pierce = 1; s->speed = 220; s->radius = 3;
		if (a >= 1) s->count = 10;
		if (a >= 2) { s->range += 8; s->pierce += 1; }
		if (a >= 3) { s->count = 16; s->interval *= 0.8f; }
		if (a >= 4) { s->fireRing = true; s->dmg = 2; s->interval = 0.6f; s->ap = true; }
		if (b >= 1) s->interval *= 0.75f;
		if (b >= 2) s->interval *= 0.75f;
		if (b >= 3) { s->dmg += 1; s->pierce += 2; s->ap = true; }
		if (b >= 4) s->interval *= 0.4f;
		s->life = s->range / s->speed;
		break;
	case T_KAENGURU:
		s->range = 60; s->interval = 1.2f; s->dmg = 1; s->pierce = 5; s->speed = 190; s->radius = 5; s->count = 1;
		if (a >= 1) s->pierce += 4;
		if (a >= 2) { s->dmg += 1; s->ap = true; }
		if (a >= 3) s->count = 2;
		if (a >= 4) { s->dmg += 3; s->pierce += 20; s->interval *= 0.6f; }
		if (b >= 1) { s->bounce = true; s->pierce += 3; }
		if (b >= 2) s->interval *= 0.7f;
		if (b >= 3) s->interval *= 0.6f;
		if (b >= 4) { s->bossMult = 3; s->pierce += 30; }
		break;
	case T_AAL:
		s->range = 58; s->interval = 1.1f; s->dmg = 1; s->jumps = 4; s->chainRange = 32; s->ap = true;
		if (a >= 1) s->jumps += 3;
		if (a >= 2) s->dmg += 1;
		if (a >= 3) { s->jumps += 8; s->dmg += 1; }
		if (a >= 4) s->stormInt = 6;
		if (b >= 1) s->interval *= 0.75f;
		if (b >= 2) s->chains = 2;
		if (b >= 3) s->fieldDmg = 1;
		if (b >= 4) { s->interval *= 0.4f; s->jumps += 6; }
		break;
	case T_BIENE:
		s->range = 30;
		s->income = 60;
		if (a >= 1) s->income += 30;
		if (a >= 2) s->income += 60;
		if (a >= 3) s->income += 150;
		if (a >= 4) s->income += 450;
		if (b >= 1) s->jarBonus = 0.15f;
		break;
	case T_DACHS:
		s->range = 55; s->buffSpeed = 0.15f; s->ap = true; s->camo = true;
		if (a >= 1) s->buffSpeed = 0.25f;
		if (a >= 2) s->buffCamo = true;
		if (a >= 3) s->buffRange = 0.15f;
		if (a >= 4) { s->buffSpeed = 0.55f; s->buffDmg = 1; }
		if (b >= 1) s->range += 15;
		if (b >= 2) { s->shockDmg = 1; s->shockInt = 1.5f; }
		if (b >= 3) s->stripAura = true;
		if (b >= 4) { s->shockDmg = 4; s->shockInt = 0.8f; }
		break;
	case T_SCHNECKE:
		s->range = 54; s->interval = 1.1f; s->speed = 200; s->radius = 4; s->slow = 0.4f; s->slowDur = 3; s->acid = 1; s->acidInt = 1.5f;
		s->splash = 6; s->splashMax = 2; s->ap = true; s->life = 0.6f;
		if (a >= 1) s->acidInt = 0.8f;
		if (a >= 2) s->strip = true;
		if (a >= 3) s->acid = 3;
		if (a >= 4) { s->acid = 8; s->bossMult = 2; }
		if (b >= 1) s->slow = 0.55f;
		if (b >= 2) { s->splash = 20; s->splashMax = 6; }
		if (b >= 3) s->slowDur = 8;
		if (b >= 4) { s->interval *= 0.4f; s->splash = 30; s->splashMax = 12; }
		break;
	}
	if (s->interval > 0) s->interval /= 1 + t->bSpeed;
	if (s->range < 900 && s->kind != K_BUFF && s->kind != K_FARM) s->range *= 1 + t->bRange;
	if (t->bCamo) s->camo = true;
	if (s->dmg > 0 && t->bDmg) s->dmg += t->bDmg;
}

bool td_can_place(TD *g, int type, float x, float y) {
	(void)type;
	if (x < 8 || y < 8 || x > FW - 8 || y > FH - 8) return false;
	const MapDef *m = &MAPS[g->map];
	float minD = PATH_W + TOWER_R - 1;
	for (int i = 0; i + 1 < m->n; i++) {
		float ax = m->pts[i][0], ay = m->pts[i][1], bx = m->pts[i + 1][0], by = m->pts[i + 1][1];
		float dx = bx - ax, dy = by - ay, l2 = dx * dx + dy * dy;
		float t = l2 > 0 ? ((x - ax) * dx + (y - ay) * dy) / l2 : 0;
		if (t < 0) t = 0;
		if (t > 1) t = 1;
		if (dist2(x, y, ax + dx * t, ay + dy * t) < minD * minD) return false;
	}
	for (int i = 0; i < MAX_TW; i++)
		if (g->tw[i].alive && dist2(x, y, g->tw[i].x, g->tw[i].y) < (2 * TOWER_R) * (2 * TOWER_R)) return false;
	return true;
}

int td_place(TD *g, int type, float x, float y) {
	int c = td_cost(g, type);
	if (g->coins < c || !td_can_place(g, type, x, y)) return -1;
	for (int i = 0; i < MAX_TW; i++)
		if (!g->tw[i].alive) {
			Tower *t = &g->tw[i];
			memset(t, 0, sizeof *t);
			t->alive = 1; t->type = type; t->x = x; t->y = y; t->spent = c; t->ang = -1.57f;
			g->coins -= c;
			fx(g, FX_RING, x, y, 0, 0, 0.4f, 16, 0xFFFFFF);
			return i;
		}
	return -1;
}

bool td_can_upgrade(TD *g, int ti, int p) {
	Tower *t = &g->tw[ti];
	int cur = p ? t->b : t->a, other = p ? t->a : t->b;
	if (cur >= 4) return false;
	if (cur + 1 > 2 && other > 2) return false;
	return true;
}

bool td_upgrade(TD *g, int ti, int p) {
	if (!td_can_upgrade(g, ti, p)) return false;
	int c = td_up_cost(g, ti, p);
	if (g->coins < c) return false;
	Tower *t = &g->tw[ti];
	g->coins -= c;
	t->spent += c;
	if (p) t->b++;
	else t->a++;
	fx(g, FX_RING, t->x, t->y, 0, 0, 0.5f, 22, 0xFFD25A);
	return true;
}

int td_sell_value(TD *g, int ti) { return (int)(g->tw[ti].spent * 0.7f); }
void td_sell(TD *g, int ti) {
	Tower *t = &g->tw[ti];
	if (!t->alive) return;
	g->coins += td_sell_value(g, ti);
	fx(g, FX_COIN, t->x, t->y, 0, 0, 0.6f, 10, 0xFFD25A);
	t->alive = 0;
}

int td_tower_at(TD *g, float x, float y) {
	int best = -1;
	float bd = 13 * 13;
	for (int i = 0; i < MAX_TW; i++) {
		if (!g->tw[i].alive) continue;
		float d = dist2(x, y, g->tw[i].x, g->tw[i].y);
		if (d < bd) { bd = d; best = i; }
	}
	return best;
}

int td_enemies_alive(TD *g) {
	int n = 0;
	for (int i = 0; i < MAX_EN; i++) n += g->en[i].alive;
	return n;
}

// ---------- Projektile
static Proj *new_proj(TD *g) {
	for (int i = 0; i < MAX_PR; i++)
		if (!g->pr[i].alive) {
			memset(&g->pr[i], 0, sizeof(Proj));
			g->pr[i].alive = 1;
			return &g->pr[i];
		}
	return NULL;
}

static float enemy_speed(TD *g, Enemy *e) {
	float v = EDEF[e->type].speed * DIFF[g->diff].speedMul;
	if (g->wave > 80) v *= 1 + fminf(0.5f, (g->wave - 80) * 0.01f);
	if (e->slowT > 0) v *= 1 - e->slowF;
	if (e->freezeT > 0) v *= e->type >= E_KOENIG ? 0.4f : 0;
	return v;
}

static void lead(TD *g, Enemy *e, float sx, float sy, float speed, float *tx, float *ty) {
	float d = sqrtf(dist2(sx, sy, e->x, e->y));
	float t = speed > 0 ? d / speed : 0;
	float fd = e->d + enemy_speed(g, e) * t;
	if (fd > g->pathLen) fd = g->pathLen;
	td_path_pos(g, fd, tx, ty);
}

static int find_target(TD *g, Tower *t, TStats *s, float range, int exclude) {
	static int cand[MAX_EN];
	int n;
	if (range >= 900) {
		n = 0;
		for (int i = 0; i < MAX_EN; i++) if (g->en[i].alive) cand[n++] = i;
	} else n = query(g, t->x, t->y, range, cand, MAX_EN);
	int best = -1;
	float bv = -1e30f;
	for (int k = 0; k < n; k++) {
		int i = cand[k];
		Enemy *e = &g->en[i];
		if (i == exclude || e->d < 2) continue;
		if ((e->mods & M_GEIST) && !s->camo) continue;
		float v;
		switch (t->target) {
		case TG_LAST: v = -e->d; break;
		case TG_STRONG: v = RBE[e->type] * 10000.0f + e->d; break;
		case TG_CLOSE: v = -dist2(t->x, t->y, e->x, e->y); break;
		default: v = e->d; break;
		}
		if (v > bv) { bv = v; best = i; }
	}
	return best;
}

static void explode(TD *g, float x, float y, float blast, int maxHits, float dmg, int tower, float bossMult, bool mini) {
	static int cand[MAX_EN];
	int n = query(g, x, y, blast, cand, MAX_EN);
	int hit = 0;
	for (int k = 0; k < n && hit < maxHits; k++) {
		if (!g->en[cand[k]].alive) continue;
		damage(g, cand[k], dmg, PF_AP, tower, bossMult);
		hit++;
	}
	fx(g, FX_BLAST, x, y, 0, 0, mini ? 0.25f : 0.4f, blast, 0xFF9A3A);
	g->evBoom++;
	if (mini || tower < 0 || !g->tw[tower].alive || g->tw[tower].type != T_KRABBE) return;
	TStats s;
	td_stats(g, &g->tw[tower], &s);
	for (int i = 0; i < s.shards; i++) {
		Proj *p = new_proj(g);
		if (!p) break;
		float a = i * 6.2832f / s.shards;
		p->kind = P_SHARD; p->x = x; p->y = y; p->vx = cosf(a) * 230; p->vy = sinf(a) * 230;
		p->life = 0.22f; p->dmg = 1; p->pierce = 1; p->radius = 2; p->tower = tower; p->bossMult = 1;
	}
	for (int i = 0; i < s.minis; i++) {
		Proj *p = new_proj(g);
		if (!p) break;
		float a = i * 6.2832f / s.minis + 0.4f;
		p->kind = P_MINIBOMB; p->x = x; p->y = y; p->vx = cosf(a) * 110; p->vy = sinf(a) * 110;
		p->life = 0.3f; p->dmg = s.dmg; p->pierce = 6; p->radius = 3; p->tower = tower; p->bossMult = s.bossMult; p->turn = 0;
	}
}

static void slime_hit(TD *g, Proj *p, float x, float y) {
	Tower *t = &g->tw[p->tower];
	if (!t->alive || t->type != T_SCHNECKE) return;
	TStats s;
	td_stats(g, t, &s);
	static int cand[MAX_EN];
	int n = query(g, x, y, s.splash, cand, MAX_EN);
	int hit = 0;
	for (int k = 0; k < n && hit < s.splashMax + 1; k++) {
		Enemy *e = &g->en[cand[k]];
		if (!e->alive) continue;
		hit++;
		if (s.slow >= e->slowF || e->slowT <= 0) e->slowF = s.slow;
		if (e->slowT < s.slowDur) e->slowT = s.slowDur;
		e->acidT = s.slowDur;
		e->acidDmg = s.acid * (e->type >= E_KOENIG ? s.bossMult : 1);
		e->acidInt = s.acidInt;
		if (e->acidTick <= 0) e->acidTick = s.acidInt;
		if (s.strip) e->mods &= ~M_PANZER;
	}
	fx(g, FX_POP, x, y, 0, 0, 0.3f, s.splash > 10 ? s.splash * 0.7f : 6, 0x9AD04A);
}

static float bomb_blast(TD *g, Proj *p) {
	if (p->kind == P_MINIBOMB) return 12;
	Tower *t = &g->tw[p->tower];
	if (t->alive && t->type == T_KRABBE) {
		TStats s;
		td_stats(g, t, &s);
		return s.blast;
	}
	return 18;
}

static void step_projectiles(TD *g) {
	static int cand[MAX_EN];
	for (int i = 0; i < MAX_PR; i++) {
		Proj *p = &g->pr[i];
		if (!p->alive) continue;
		p->life -= STEP;
		if (p->life <= 0) {
			if (p->kind == P_BOMB || p->kind == P_MINIBOMB)
				explode(g, p->x, p->y, bomb_blast(g, p), p->pierce, p->dmg, p->tower, p->bossMult, p->kind == P_MINIBOMB);
			p->alive = 0;
			continue;
		}
		if (p->turn != 0) {
			float c = cosf(p->turn * STEP), s = sinf(p->turn * STEP);
			float vx = p->vx * c - p->vy * s, vy = p->vx * s + p->vy * c;
			p->vx = vx; p->vy = vy;
		}
		p->x += p->vx * STEP;
		p->y += p->vy * STEP;
		p->rot += STEP * 14;
		if (p->x < -30 || p->y < -30 || p->x > FW + 30 || p->y > FH + 30) { p->alive = 0; continue; }
		int n = query(g, p->x, p->y, p->radius, cand, MAX_EN);
		for (int k = 0; k < n && p->alive; k++) {
			int ei = cand[k];
			Enemy *e = &g->en[ei];
			if (!e->alive) continue;
			bool seen = false;
			for (int h = 0; h < p->nh; h++) if (p->hits[h] == e->id) seen = true;
			if (seen) continue;
			if (p->kind == P_BOMB || p->kind == P_MINIBOMB) {
				explode(g, p->x, p->y, bomb_blast(g, p), p->pierce, p->dmg, p->tower, p->bossMult, p->kind == P_MINIBOMB);
				p->alive = 0;
				break;
			}
			if (p->kind == P_SLIME) {
				slime_hit(g, p, p->x, p->y);
				p->alive = 0;
				break;
			}
			int r = damage(g, ei, p->dmg, p->flags, p->tower, p->bossMult);
			if (r < 0) fx(g, FX_SPARK, p->x, p->y, 0, 0, 0.2f, 4, 0xC0C8D8);
			if (p->nh < 12) p->hits[p->nh++] = e->id;
			else { memmove(p->hits, p->hits + 1, 11 * sizeof(uint16_t)); p->hits[11] = e->id; }
			p->pierce--;
			if (p->pierce <= 0 || r < 0) { p->alive = 0; break; }
			if (p->flags & PF_BOUNCE) {
				// zum naechsten Glibber springen
				static int c2[MAX_EN];
				int m = query(g, p->x, p->y, 60, c2, MAX_EN);
				int best = -1;
				float bd = 1e9f;
				for (int q = 0; q < m; q++) {
					Enemy *o = &g->en[c2[q]];
					if (!o->alive || o == e) continue;
					bool s2 = false;
					for (int h = 0; h < p->nh; h++) if (p->hits[h] == o->id) s2 = true;
					if (s2) continue;
					float dd = dist2(p->x, p->y, o->x, o->y);
					if (dd < bd) { bd = dd; best = c2[q]; }
				}
				if (best >= 0) {
					float sp = sqrtf(p->vx * p->vx + p->vy * p->vy);
					float dx = g->en[best].x - p->x, dy = g->en[best].y - p->y, l = sqrtf(dx * dx + dy * dy) + 1e-4f;
					p->vx = dx / l * sp; p->vy = dy / l * sp;
					p->life = fmaxf(p->life, 0.5f);
				}
				break;
			}
		}
	}
}

// ---------- Turmaktionen
static bool fire_tower(TD *g, int ti, TStats *s) {
	Tower *t = &g->tw[ti];
	switch (s->kind) {
	case K_PROJ:
	case K_BOMB:
	case K_SLIME: {
		int e = find_target(g, t, s, s->range, -1);
		if (e < 0) return false;
		float tx, ty;
		lead(g, &g->en[e], t->x, t->y, s->speed, &tx, &ty);
		float base = atan2f(ty - t->y, tx - t->x);
		t->ang = base;
		int cnt = s->count > 0 ? s->count : 1;
		for (int k = 0; k < cnt; k++) {
			Proj *p = new_proj(g);
			if (!p) break;
			float a = base + (k - (cnt - 1) * 0.5f) * s->spread;
			p->x = t->x; p->y = t->y;
			p->vx = cosf(a) * s->speed; p->vy = sinf(a) * s->speed;
			p->tower = ti; p->dmg = s->dmg; p->radius = s->radius; p->bossMult = s->bossMult;
			p->flags = (s->ap ? PF_AP : 0);
			if (s->kind == K_PROJ) { p->kind = s->radius > 4 ? P_BLADE : P_SPIKE; p->pierce = s->pierce; p->life = s->life + s->range / s->speed; }
			if (s->kind == K_BOMB) {
				p->kind = P_BOMB; p->pierce = s->blastMax;
				float d = sqrtf(dist2(t->x, t->y, tx, ty));
				p->life = fminf(s->life, d / s->speed + 0.05f);
			}
			if (s->kind == K_SLIME) {
				p->kind = P_SLIME; p->pierce = 1;
				float d = sqrtf(dist2(t->x, t->y, tx, ty));
				p->life = d / s->speed + 0.15f;
			}
		}
		g->evShoot++;
		t->flash = 0.12f;
		return true;
	}
	case K_SNIPER: {
		int e = find_target(g, t, s, 999, -1);
		if (e < 0) return false;
		Enemy *en = &g->en[e];
		float ex = en->x, ey = en->y;
		t->ang = atan2f(ey - t->y, ex - t->x);
		int r = damage(g, e, s->dmg, s->ap ? PF_AP : 0, ti, s->bossMult);
		if (r < 0) fx(g, FX_SPARK, ex, ey, 0, 0, 0.2f, 5, 0xC0C8D8);
		if (s->splash > 0) {
			static int cand[MAX_EN];
			int n = query(g, ex, ey, s->splash, cand, MAX_EN), hit = 0;
			for (int k = 0; k < n && hit < s->splashMax; k++) {
				if (!g->en[cand[k]].alive) continue;
				damage(g, cand[k], 2, s->ap ? PF_AP : 0, ti, 1);
				hit++;
			}
			fx(g, FX_BLAST, ex, ey, 0, 0, 0.25f, s->splash, 0xC0A0FF);
		}
		fx(g, FX_TRACER, t->x, t->y, ex, ey, 0.18f, 1.5f, 0xE0D0FF);
		g->evShoot++;
		t->flash = 0.15f;
		return true;
	}
	case K_RADIAL: {
		static int cand[MAX_EN];
		if (query(g, t->x, t->y, s->range, cand, 1) == 0) return false;
		if (s->fireRing) {
			int n = query(g, t->x, t->y, s->range, cand, MAX_EN);
			for (int k = 0; k < n; k++) damage(g, cand[k], s->dmg, PF_AP, ti, 1);
			fx(g, FX_FIRE, t->x, t->y, 0, 0, 0.35f, s->range, 0xFF7A2A);
			g->evBoom++;
		} else {
			for (int k = 0; k < s->count; k++) {
				Proj *p = new_proj(g);
				if (!p) break;
				float a = k * 6.2832f / s->count + t->cd2;
				p->kind = s->ap ? P_BLADE : P_SPIKE;
				p->x = t->x; p->y = t->y; p->vx = cosf(a) * s->speed; p->vy = sinf(a) * s->speed;
				p->life = s->life; p->dmg = s->dmg; p->pierce = s->pierce; p->radius = s->radius; p->tower = ti;
				p->flags = s->ap ? PF_AP : 0; p->bossMult = 1;
			}
			t->cd2 += 0.2f;
			g->evShoot++;
		}
		t->flash = 0.12f;
		return true;
	}
	case K_BOOMER: {
		int e = find_target(g, t, s, s->range, -1);
		if (e < 0) return false;
		float tx = g->en[e].x, ty = g->en[e].y;
		float D = fmaxf(12, sqrtf(dist2(t->x, t->y, tx, ty)));
		float base = atan2f(ty - t->y, tx - t->x);
		t->ang = base;
		for (int k = 0; k < s->count; k++) {
			Proj *p = new_proj(g);
			if (!p) break;
			p->kind = P_BOOMER; p->x = t->x; p->y = t->y; p->tower = ti; p->dmg = s->dmg; p->pierce = s->pierce;
			p->radius = s->radius; p->bossMult = s->bossMult; p->flags = (s->ap ? PF_AP : 0) | (s->bounce ? PF_BOUNCE : 0);
			if (s->bounce) {
				p->vx = cosf(base) * s->speed * 1.3f; p->vy = sinf(base) * s->speed * 1.3f;
				p->life = 1.2f;
			} else {
				float R = D / 1.6f;
				float alpha = asinf(fminf(1, D / (2 * R)));
				float dir = k == 0 ? 1 : -1;
				float a = base - alpha * dir;
				p->vx = cosf(a) * s->speed; p->vy = sinf(a) * s->speed;
				p->turn = dir * s->speed / R;
				p->life = 6.2832f * R / s->speed;
			}
		}
		g->evShoot++;
		t->flash = 0.12f;
		return true;
	}
	case K_CHAIN: {
		int prevFirst = -1;
		for (int c = 0; c < s->chains; c++) {
			int e = find_target(g, t, s, s->range, prevFirst);
			if (e < 0) break;
			prevFirst = e;
			uint16_t hitIds[32];
			int nh = 0;
			float lx = t->x, ly = t->y;
			int cur = e;
			for (int j = 0; j <= s->jumps && cur >= 0 && nh < 32; j++) {
				Enemy *en = &g->en[cur];
				float ex = en->x, ey = en->y;
				hitIds[nh++] = en->id;
				fx(g, FX_BOLT, lx, ly, ex, ey, 0.16f, 1.5f, 0xFFF080);
				damage(g, cur, s->dmg, PF_AP, ti, s->bossMult);
				lx = ex; ly = ey;
				static int cand[MAX_EN];
				int n = query(g, ex, ey, s->chainRange, cand, MAX_EN);
				int nx = -1;
				float bd = 1e9f;
				for (int k = 0; k < n; k++) {
					Enemy *o = &g->en[cand[k]];
					if (!o->alive) continue;
					bool seen = false;
					for (int h = 0; h < nh; h++) if (hitIds[h] == o->id) seen = true;
					if (seen) continue;
					float dd = dist2(ex, ey, o->x, o->y);
					if (dd < bd) { bd = dd; nx = cand[k]; }
				}
				cur = nx;
			}
			t->ang = atan2f(g->en[e].y - t->y, g->en[e].x - t->x);
		}
		if (prevFirst >= 0) { g->evZap++; t->flash = 0.12f; return true; }
		return false;
	}
	}
	return false;
}

static void freeze_pulse(TD *g, int ti, TStats *s) {
	Tower *t = &g->tw[ti];
	static int cand[MAX_EN];
	int n = query(g, t->x, t->y, s->range, cand, MAX_EN);
	if (n == 0) return;
	int hit = 0;
	for (int k = 0; k < n && hit < s->freezeMax; k++) {
		Enemy *e = &g->en[cand[k]];
		if (!e->alive) continue;
		bool boss = e->type >= E_KOENIG;
		if (boss && !s->freezeBoss) continue;
		hit++;
		e->freezeT = boss ? s->freeze * 0.6f : s->freeze;
		if (s->slow > 0) { e->slowF = fmaxf(e->slowF, s->slow); e->slowT = fmaxf(e->slowT, s->freeze + s->slowDur); }
		if (s->strip) e->mods &= ~(M_PANZER | M_REGEN);
		if (s->dmg > 0) damage(g, cand[k], s->dmg, PF_AP, ti, 1);
	}
	fx(g, FX_FREEZE, t->x, t->y, 0, 0, 0.45f, s->range, 0xBFEFFF);
	g->evFreeze++;
	t->flash = 0.2f;
}

// ---------- Wellen
static void G(Wave *w, int t, int m, int c, float start, float gap) {
	if (w->ng >= 14 || c <= 0) return;
	w->g[w->ng++] = (Group){(uint8_t)t, (uint8_t)m, (uint16_t)c, start, gap};
}

void td_make_wave(int n, Wave *w) {
	memset(w, 0, sizeof *w);
	w->hpMul = 1;
	int k = n;
	if (n > 80) {
		k = 71 + ((n - 81) % 10);
		w->hpMul = powf(1.07f, (float)(n - 80));
	}
	const int TR = E_TROPFEN, KL = E_KLECKS, GL = E_GLIBBER, WA = E_WABBEL, RI = E_RIESE, KO = E_KOENIG, UR = E_URGLIBBER, TI = E_TITAN;
	const int GE = M_GEIST, PA = M_PANZER, RE = M_REGEN;
	if (k <= 5) G(w, TR, 0, 8 + 4 * k, 0, 0.85f - 0.07f * k);
	else if (k <= 10) { G(w, TR, 0, 15 + k, 0, 0.45f); G(w, KL, 0, 3 * (k - 5), 3, 0.8f - 0.05f * (k - 5)); }
	else if (k <= 15) {
		G(w, KL, 0, 10 + 2 * (k - 10), 0, 0.45f);
		G(w, GL, 0, 3 * (k - 10), 2, 0.9f);
		if (k == 15) G(w, KL, 0, 24, 12, 0.2f);
	} else if (k <= 19) {
		G(w, GL, 0, 8 + 2 * (k - 15), 0, 0.65f);
		G(w, WA, 0, 1 + (k - 15), 5, 1.3f);
		G(w, KL, 0, 16, 9, 0.3f);
	} else if (k == 20) {
		G(w, GL, 0, 20, 0, 0.4f); G(w, TR, GE, 12, 6, 0.5f); G(w, WA, 0, 6, 10, 0.8f);
	} else if (k <= 23) {
		G(w, WA, 0, 6 + 2 * (k - 20), 0, 0.7f); G(w, KL, GE, 8, 3, 0.5f); G(w, GL, 0, 15, 6, 0.3f);
	} else if (k <= 25) {
		G(w, RI, 0, 1 + (k - 24) * 2, 0, 2.5f); G(w, WA, 0, 12, 2, 0.5f); G(w, GL, GE, 6, 8, 0.6f);
	} else if (k <= 29) {
		G(w, RI, 0, 2 + (k - 25), 0, 2.0f); G(w, GL, PA, 4 + 2 * (k - 25), 3, 0.8f);
		G(w, GL, GE, 10, 6, 0.4f); G(w, WA, 0, 10, 10, 0.4f);
	} else if (k == 30) {
		G(w, KO, 0, 1, 0, 1); G(w, WA, 0, 20, 3, 0.35f); G(w, GL, PA, 8, 10, 0.6f);
	} else if (k <= 39) {
		int j = k - 30;
		G(w, RI, 0, 2 + 2 * j, 0, 1.4f - 0.06f * j);
		G(w, WA, PA, 2 * j, 4, 0.7f);
		G(w, WA, GE, 2 + j, 6, 0.6f);
		if (k >= 33) G(w, GL, RE, 10 + j, 8, 0.4f);
		if (k % 2) G(w, KL, 0, 40 + 5 * j, 10, 0.1f);
	} else if (k == 40) {
		G(w, KO, 0, 3, 0, 6); G(w, RI, 0, 10, 2, 1.0f); G(w, RI, GE, 4, 10, 1.5f);
	} else if (k <= 49) {
		int j = k - 40;
		G(w, RI, 0, 10 + 2 * j, 0, 0.9f);
		G(w, RI, PA, j, 5, 1.6f);
		G(w, WA, GE | RE, 15, 8, 0.3f);
		if (j % 2 == 0) G(w, KO, 0, 1 + j / 4, 12, 5);
		if (k == 45) G(w, RI, GE, 8, 15, 0.8f);
	} else if (k == 50) {
		G(w, UR, 0, 1, 0, 1); G(w, KO, 0, 2, 6, 8); G(w, RI, RE, 12, 4, 0.8f);
	} else if (k <= 59) {
		int j = k - 50;
		G(w, RI, 0, 20 + 3 * j, 0, 0.6f);
		G(w, RI, PA | GE, 2 + j, 4, 1.2f);
		G(w, KO, 0, 1 + j / 2, 6, 4);
		G(w, WA, RE | GE, 20, 2, 0.2f);
		if (j >= 5) G(w, UR, 0, 1, 20, 1);
	} else if (k == 60) {
		G(w, UR, 0, 2, 0, 10); G(w, KO, 0, 4, 5, 4); G(w, RI, PA, 20, 10, 0.6f);
	} else if (k <= 69) {
		int j = k - 60;
		G(w, UR, 0, 1 + j / 3, 0, 8);
		G(w, KO, 0, 3 + j, 3, 3);
		G(w, RI, GE | RE, 25 + 2 * j, 2, 0.4f);
		G(w, RI, PA, 10 + j, 10, 0.6f);
	} else if (k == 70) {
		G(w, TI, 0, 1, 0, 1); G(w, UR, 0, 2, 10, 10);
	} else if (k <= 79) {
		int j = k - 70;
		G(w, UR, 0, 2 + j / 2, 0, 6);
		G(w, KO, 0, 6 + j, 4, 2.5f);
		G(w, RI, GE | PA | RE, 30 + 3 * j, 2, 0.3f);
		if (j >= 5) G(w, TI, 0, 1, 15, 1);
	} else {
		G(w, TI, 0, 2, 0, 15); G(w, UR, 0, 4, 5, 6); G(w, KO, 0, 10, 10, 2);
	}
}

int td_wave_rbe(int n) {
	Wave w;
	td_make_wave(n, &w);
	int r = 0;
	for (int i = 0; i < w.ng; i++) r += w.g[i].count * RBE[w.g[i].type];
	return r;
}

bool td_start_wave(TD *g) {
	if (g->waveActive || g->lost) return false;
	g->wave++;
	td_make_wave(g->wave, &g->cur);
	memset(g->spawned, 0, sizeof g->spawned);
	g->waveActive = true;
	g->waveT = 0;
	return true;
}

int td_collect_jar(TD *g, float x, float y, float r) {
	int got = 0;
	for (int i = 0; i < MAX_JAR; i++) {
		Jar *j = &g->jar[i];
		if (!j->alive || dist2(x, y, j->x, j->y) > r * r) continue;
		g->coins += j->value;
		got += j->value;
		fx(g, FX_COIN, j->x, j->y, 0, 0, 0.6f, 10, 0xFFD25A);
		j->alive = 0;
		g->evCoin++;
	}
	return got;
}

static void spawn_jar(TD *g, Tower *t, int value) {
	for (int i = 0; i < MAX_JAR; i++)
		if (!g->jar[i].alive) {
			Jar *j = &g->jar[i];
			j->alive = 1;
			float a = frand() * 6.28f, r = 10 + frand() * 10;
			j->x = t->x + cosf(a) * r;
			j->y = t->y + sinf(a) * r;
			if (j->x < 8) j->x = 8;
			if (j->x > FW - 8) j->x = FW - 8;
			if (j->y < 8) j->y = 8;
			if (j->y > FH - 8) j->y = FH - 8;
			j->t = 0;
			j->value = value;
			return;
		}
	g->coins += value;
}

// ---------- Simulationsschritt
void td_step(TD *g) {
	if (g->lost) return;
	float dt = STEP;
	// Effekte altern
	for (int i = 0; i < MAX_FX; i++)
		if (g->fx[i].alive) {
			g->fx[i].t += dt;
			if (g->fx[i].t >= g->fx[i].max) g->fx[i].alive = 0;
		}
	// Honig
	bool autoJar = false;
	g->popMul = 1;
	for (int i = 0; i < MAX_TW; i++)
		if (g->tw[i].alive && g->tw[i].type == T_BIENE) {
			if (g->tw[i].b >= 2) autoJar = true;
			if (g->tw[i].b >= 4) g->popMul = 1.25;
		}
	for (int i = 0; i < MAX_JAR; i++) {
		Jar *j = &g->jar[i];
		if (!j->alive) continue;
		j->t += dt;
		if (j->t > (autoJar ? 0.8f : 6.0f)) td_collect_jar(g, j->x, j->y, 1);
	}
	if (!g->waveActive) return;
	float prevT = g->waveT;
	g->waveT += dt;

	// Spawnen
	bool spawning = false;
	for (int i = 0; i < g->cur.ng; i++) {
		Group *gr = &g->cur.g[i];
		while (g->spawned[i] < gr->count && g->waveT >= gr->start + g->spawned[i] * gr->gap) {
			int e = spawn_enemy(g, gr->type, gr->mods, 0, g->cur.hpMul);
			if (e < 0) break;
			g->spawned[i]++;
		}
		if (g->spawned[i] < gr->count) spawning = true;
	}

	// Gegner bewegen
	for (int i = 0; i < MAX_EN; i++) {
		Enemy *e = &g->en[i];
		if (!e->alive) continue;
		e->wob += dt * 6;
		if (e->hitT > 0) e->hitT -= dt;
		float v = enemy_speed(g, e);
		if (e->freezeT > 0) e->freezeT -= dt;
		if (e->slowT > 0) { e->slowT -= dt; if (e->slowT <= 0) e->slowF = 0; }
		e->d += v * dt;
		if (e->d >= g->pathLen) {
			e->alive = 0;
			g->lives -= RBE[e->type];
			g->leaked += RBE[e->type];
			g->evLeak++;
			if (g->lives <= 0) { g->lives = 0; g->lost = true; g->waveActive = false; return; }
			continue;
		}
		enemy_pos(g, e);
		if ((e->mods & M_REGEN) && e->type < e->maxType) {
			e->regenT += dt;
			if (e->regenT >= 2.5f) {
				e->regenT = 0;
				e->type++;
				e->hp = EDEF[e->type].hp * g->cur.hpMul;
				fx(g, FX_RING, e->x, e->y, 0, 0, 0.3f, EDEF[e->type].radius + 3, 0xFF8AC0);
			}
		}
	}
	build_grid(g);
	// Saeure
	for (int i = 0; i < MAX_EN; i++) {
		Enemy *e = &g->en[i];
		if (!e->alive || e->acidT <= 0) continue;
		e->acidT -= dt;
		e->acidTick -= dt;
		if (e->acidTick <= 0) {
			e->acidTick = e->acidInt > 0 ? e->acidInt : 1;
			damage(g, i, e->acidDmg, PF_AP, -1, 1);
		}
	}

	// Buffs berechnen
	for (int i = 0; i < MAX_TW; i++) {
		Tower *t = &g->tw[i];
		t->bSpeed = t->bRange = 0;
		t->bCamo = t->bDmg = t->bStrip = 0;
	}
	for (int i = 0; i < MAX_TW; i++) {
		Tower *d = &g->tw[i];
		if (!d->alive || d->type != T_DACHS) continue;
		TStats s;
		td_stats(g, d, &s);
		for (int j = 0; j < MAX_TW; j++) {
			Tower *t = &g->tw[j];
			if (!t->alive || j == i || t->type == T_DACHS || t->type == T_BIENE) continue;
			if (dist2(t->x, t->y, d->x, d->y) > s.range * s.range) continue;
			if (s.buffSpeed > t->bSpeed) t->bSpeed = s.buffSpeed;
			if (s.buffRange > t->bRange) t->bRange = s.buffRange;
			if (s.buffCamo) t->bCamo = 1;
			if (s.buffDmg > t->bDmg) t->bDmg = s.buffDmg;
		}
	}

	// Tuerme
	static int cand[MAX_EN];
	for (int i = 0; i < MAX_TW; i++) {
		Tower *t = &g->tw[i];
		if (!t->alive) continue;
		if (t->flash > 0) t->flash -= dt;
		TStats s;
		td_stats(g, t, &s);
		switch (s.kind) {
		case K_FARM: {
			static const float times[4] = {1, 5, 9, 13};
			for (int k = 0; k < 4; k++)
				if (prevT < times[k] && g->waveT >= times[k]) spawn_jar(g, t, (int)(s.income * (1 + s.jarBonus) / 4));
			break;
		}
		case K_BUFF:
			if (s.stripAura) {
				int n = query(g, t->x, t->y, s.range, cand, MAX_EN);
				for (int k = 0; k < n; k++) g->en[cand[k]].mods &= ~M_PANZER;
			}
			if (s.shockDmg > 0) {
				t->cd2 += dt;
				if (t->cd2 >= s.shockInt) {
					int n = query(g, t->x, t->y, s.range, cand, MAX_EN);
					if (n > 0) {
						t->cd2 = 0;
						for (int k = 0; k < n && k < 40; k++) damage(g, cand[k], s.shockDmg, PF_AP, i, 1);
						fx(g, FX_SHOCK, t->x, t->y, 0, 0, 0.4f, s.range, 0xD0D8FF);
						g->evBoom++;
						t->flash = 0.15f;
					}
				}
			}
			break;
		case K_FREEZE:
			if (s.auraSlow > 0) {
				int n = query(g, t->x, t->y, s.range, cand, MAX_EN);
				for (int k = 0; k < n; k++) {
					Enemy *e = &g->en[cand[k]];
					if (e->slowF < s.auraSlow) e->slowF = s.auraSlow;
					if (e->slowT < 0.15f) e->slowT = 0.15f;
				}
			}
			t->cd -= dt;
			if (t->cd <= 0) { t->cd = s.interval; freeze_pulse(g, i, &s); }
			if (s.iceDmg > 0) {
				t->cd2 -= dt;
				if (t->cd2 <= 0) {
					int e = find_target(g, t, &s, s.range, -1);
					if (e >= 0) {
						t->cd2 = s.iceInt;
						Proj *p = new_proj(g);
						if (p) {
							float tx, ty;
							lead(g, &g->en[e], t->x, t->y, 260, &tx, &ty);
							float a = atan2f(ty - t->y, tx - t->x);
							p->kind = P_ICE; p->x = t->x; p->y = t->y; p->vx = cosf(a) * 260; p->vy = sinf(a) * 260;
							p->life = s.range / 260 + 0.2f; p->dmg = s.iceDmg; p->pierce = 3; p->radius = 3; p->tower = i;
							p->flags = PF_AP; p->bossMult = 1;
						}
					} else t->cd2 = 0;
				}
			}
			break;
		default:
			t->cd -= dt;
			if (t->cd <= 0) {
				t->cd = fire_tower(g, i, &s) ? s.interval : 0;
			}
			if (s.kind == K_CHAIN && s.fieldDmg > 0) {
				t->cd3 += dt;
				if (t->cd3 >= 0.5f) {
					t->cd3 = 0;
					int n = query(g, t->x, t->y, 40, cand, MAX_EN);
					for (int k = 0; k < n; k++) damage(g, cand[k], s.fieldDmg, PF_AP, i, 1);
					if (n) fx(g, FX_SHOCK, t->x, t->y, 0, 0, 0.25f, 40, 0xFFF080);
				}
			}
			if (s.kind == K_CHAIN && s.stormInt > 0) {
				t->cd2 += dt;
				if (t->cd2 >= s.stormInt && td_enemies_alive(g) > 0) {
					t->cd2 = 0;
					int shown = 0;
					for (int k = 0; k < MAX_EN; k++) {
						if (!g->en[k].alive) continue;
						if (shown < 10 && (k & 1)) { fx(g, FX_BOLT, g->en[k].x + 10, -10, g->en[k].x, g->en[k].y, 0.3f, 2, 0xFFFFA0); shown++; }
						damage(g, k, 3, PF_AP, i, 1);
					}
					g->evZap += 3;
				}
			}
			break;
		}
	}
	step_projectiles(g);

	// Wellenende
	if (!spawning && td_enemies_alive(g) == 0) {
		g->waveActive = false;
		int bonus = 100 + g->wave;
		bool interest = false;
		for (int i = 0; i < MAX_TW; i++) if (g->tw[i].alive && g->tw[i].type == T_BIENE && g->tw[i].b >= 3) interest = true;
		if (interest) bonus += (int)fmin(500, g->coins * 0.1);
		g->coins += bonus;
		g->waveEndBonus = bonus;
		g->evWaveEnd++;
		// Projektile aufraeumen
		for (int i = 0; i < MAX_PR; i++) g->pr[i].alive = 0;
		if (g->wave >= DIFF[g->diff].finalWave && !g->endless) g->won = true;
	}
}
