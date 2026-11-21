// Star Forge v2 - Expedition, Hangar, Auftraege, Artefakte (wird in main.c eingebunden)

static Exp EX;
static bool inExp;
static int expSector = 1;
static float expIntroT, expFullT, expShootCd, expPickCd, camX = 700, camY = 1300;

static void ring(float x, float y, float r, float th, u32 c, int seg) {
	for (int k = 0; k < seg; k++) {
		float a0 = 2 * PI * k / seg, a1 = 2 * PI * (k + 1) / seg;
		line(x + cosf(a0) * r, y + sinf(a0) * r, x + cosf(a1) * r, y + sinf(a1) * r, th, c);
	}
}

static void drawToast(void) {
	if (toastTail == toastHead) return;
	Toast *t = &toasts[toastTail];
	float in = fminf(1, toastT * 5), out = fminf(1, (3.0f - toastT) * 5);
	float a = fminf(in, out);
	float y = 168 + (1 - a) * 20;
	float w = 300;
	rrect(px(50, -2), y, w, 40, 8, RGBA(darken(t->col, 0.7f), 0.92f * a));
	rect(px(50, -2) + 8, y, w - 16, 2, RGBA(t->col, a));
	textFit(px(200, -2), y + 3, 0.5f, RGBA(t->col, a), C2D_AlignCenter, w - 16, t->title);
	textFit(px(200, -2), y + 21, 0.4f, RGBA(C_TEXT, a), C2D_AlignCenter, w - 16, t->msg);
}

static void gemIcon(float x, float y, float r, u32 c, float al) {
	tri(x, y - r, RGBA(lighten(c, 0.5f), al), x - r * 0.75f, y, RGBA(c, al), x + r * 0.75f, y, RGBA(lighten(c, 0.2f), al));
	tri(x - r * 0.75f, y, RGBA(c, al), x, y + r, RGBA(darken(c, 0.45f), al), x + r * 0.75f, y, RGBA(darken(c, 0.2f), al));
}

static void shipShape(float x, float y, float ang, float sc, float thrust, bool boost, float flash, float al) {
	float c = cosf(ang) * sc, s = sinf(ang) * sc;
#define RX(a, b) (x + (a) * c - (b) * s)
#define RY(a, b) (y + (a) * s + (b) * c)
	if (thrust > 0.05f) {
		float fl = (boost ? 17 : 9) * thrust * (0.75f + 0.5f * frnd());
		u32 f1 = boost ? 0x8AE8FF : 0xFFB04A, f2 = boost ? 0x2A6AFF : 0xFF4A2A;
		tri(RX(-6, -3.5f), RY(-6, -3.5f), RGBA(f1, al), RX(-6 - fl, 0), RY(-6 - fl, 0), RGBA(f2, 0), RX(-6, 3.5f), RY(-6, 3.5f), RGBA(f1, al));
	}
	u32 body = flash > 0 ? 0xFF8A8A : 0xD8E4FF, wing = flash > 0 ? 0xC04A4A : 0x4A7AD8;
	tri(RX(-7, -10), RY(-7, -10), RGBA(darken(wing, 0.2f), al), RX(4, -3), RY(4, -3), RGBA(wing, al), RX(-3, 0), RY(-3, 0), RGBA(wing, al));
	tri(RX(-7, 10), RY(-7, 10), RGBA(darken(wing, 0.2f), al), RX(4, 3), RY(4, 3), RGBA(wing, al), RX(-3, 0), RY(-3, 0), RGBA(wing, al));
	tri(RX(12, 0), RY(12, 0), RGBA(0xFFFFFF, al), RX(-7, -5), RY(-7, -5), RGBA(body, al), RX(-7, 5), RY(-7, 5), RGBA(darken(body, 0.3f), al));
	circ(RX(3, 0), RY(3, 0), 2.4f * sc, RGBA(0x8AE8FF, al));
#undef RX
#undef RY
}

static void startExpedition(void) {
	if (expSector < 1) expSector = 1;
	if (expSector > G.s.ext.x.sectorMax) expSector = G.s.ext.x.sectorMax < 1 ? 1 : G.s.ext.x.sectorMax;
	exp_start(&EX, &G, expSector);
	camX = EX.x; camY = EX.y;
	inExp = true;
	expIntroT = G.s.ext.x.expTutorial < 3 ? 6 : 2.5f;
	if (G.s.ext.x.expTutorial < 3) G.s.ext.x.expTutorial++;
	play(SND_NOVA, 1.6f, 0.5f);
}

static void endExpedition(void) {
	double en, al;
	bool dead = EX.destroyed;
	int sec = EX.sector, nArt = EX.nArt;
	char arts[80] = "";
	for (int k = 0; k < nArt; k++) {
		if (k) strncat(arts, ", ", sizeof arts - strlen(arts) - 1);
		strncat(arts, ART[EX.artFound[k]].name, sizeof arts - strlen(arts) - 1);
	}
	exp_finish(&EX, &G, &en, &al);
	inExp = false;
	char b[32], b2[32];
	num(en, b);
	num(al, b2);
	openModal(MD_INFO, 0, dead ? "Schiff zerstört!" : "Expedition beendet", "%s+%s Energie, +%s Alloy. Sektor %d erreicht.%s%s",
		dead ? "Die Hälfte der Fracht ging verloren. " : "", b, b2, sec, nArt ? " Artefakt: " : "", arts);
	play(dead ? SND_ERR : SND_ACH, dead ? 0.7f : 1, 0.6f);
	saveGame();
}

// ---------- Weltdarstellung (oben)
static inline float WX(float x, float depth) { return x - camX + 200 + eyeSign * depth * slider3d; }
static inline float WY(float y) { return y - camY + 120; }
static bool onScr(float x, float y, float r) { return fabsf(x - camX) < 200 + r && fabsf(y - camY) < 120 + r; }
static u32 rockColor(void) { return hexMix(0x8A8078, 0x6A5A90, (EX.sector - 1) / 11.0f); }

static void drawAsteroid(Asteroid *a) {
	float cx = WX(a->x, 2), cy = WY(a->y);
	u32 rc = rockColor();
	if (a->hitT > 0) rc = lighten(rc, 0.35f);
	if (a->relic) circ(cx, cy, a->r * 1.7f, RGBA(C_GOLD, 0.1f + 0.08f * sinf(T * 3)));
	float pxp = 0, pyp = 0;
	for (int k = 0; k <= 10; k++) {
		int kk = k % 10;
		float an = a->rot + kk * 0.6283f, rr = a->r * a->shape[kk];
		float x = cx + cosf(an) * rr, y = cy + sinf(an) * rr;
		if (k > 0) {
			float l = 0.5f + 0.5f * cosf(an - 0.31f + 2.36f);
			u32 ec = hexMix(darken(rc, 0.5f), lighten(rc, 0.2f), l);
			tri(cx, cy, C(rc), pxp, pyp, C(ec), x, y, C(ec));
		}
		pxp = x; pyp = y;
	}
	u32 oc = ORE[a->ore].color;
	for (int k = 0; k < 3; k++) {
		float an = a->rot + k * 2.1f + a->shape[k] * 3, rr = a->r * 0.45f * a->shape[k + 3];
		circ(cx + cosf(an) * rr, cy + sinf(an) * rr, a->r * 0.15f + 0.8f, C(oc));
	}
	if (a->relic) ring(cx, cy, a->r * 1.2f, 1.5f, RGBA(C_GOLD, 0.7f + 0.3f * sinf(T * 4)), 18);
	if (a->hp < a->maxHp) {
		rect(cx - a->r, cy - a->r - 7, a->r * 2, 2.5f, RGBA(0x000000, 0.6f));
		rect(cx - a->r, cy - a->r - 7, a->r * 2 * a->hp / a->maxHp, 2.5f, C(oc));
	}
}

static void drawExpTop(void) {
	u32 bg1 = hexMix(0x050818, 0x14051A, (EX.sector - 1) / 11.0f), bg2 = hexMix(0x0E1438, 0x2A0A30, (EX.sector - 1) / 11.0f);
	rectG(0, 0, TW, SH, C(bg1), C(bg2));
	for (int i = 0; i < NSTAR; i++) {
		Star *s = &stars[i];
		float par = 0.05f + s->z * 0.25f;
		float x = fmodf(s->x * 3 - camX * par + 40000, TW + 40) - 20;
		float y = fmodf(s->y * 3 - camY * par + 40000, SH + 20) - 10;
		rect(x + eyeSign * (8 - s->z * 5) * slider3d, y, s->sz, s->sz, RGBA(0xFFFFFF, 0.3f + 0.4f * s->z));
	}
	// Grenze
	if (camX < 220) rect(WX(0, 1) - 4, 0, 4, SH, RGBA(C_RED, 0.4f));
	if (camX > WORLD - 220) rect(WX(WORLD, 1), 0, 4, SH, RGBA(C_RED, 0.4f));
	if (camY < 140) rect(0, WY(0) - 4, TW, 4, RGBA(C_RED, 0.4f));
	if (camY > WORLD - 140) rect(0, WY(WORLD), TW, 4, RGBA(C_RED, 0.4f));
	// Tor
	if (onScr(EX.gateX, EX.gateY, 40)) {
		float gx = WX(EX.gateX, 2), gy = WY(EX.gateY);
		circ(gx, gy, 30, RGBA(C_PURPLE, 0.15f));
		circ(gx, gy, 18, RGBA(0x2A1A5A, 0.8f));
		for (int k = 0; k < 8; k++) {
			float a = T * 1.5f + k * PI / 4;
			line(gx + cosf(a) * 22, gy + sinf(a) * 22, gx + cosf(a + 0.5f) * 22, gy + sinf(a + 0.5f) * 22, 3, C(k % 2 ? C_CYAN : C_PURPLE));
		}
		for (int k = 0; k < 3; k++) ring(gx, gy, 6 + k * 4 + fmodf(T * 8, 4), 1, RGBA(0xFFFFFF, 0.5f - k * 0.12f), 12);
		textf(gx, gy + 26, 0.34f, C(C_CYAN), C2D_AlignCenter, EX.sector < MAX_SECTOR ? "Sektor %d" : "Ende", EX.sector + 1);
	}
	for (int i = 0; i < MAX_AST; i++) if (EX.ast[i].alive && onScr(EX.ast[i].x, EX.ast[i].y, 34)) drawAsteroid(&EX.ast[i]);
	for (int i = 0; i < MAX_MINE; i++) {
		Mine *m = &EX.mn[i];
		if (!m->alive || !onScr(m->x, m->y, 16)) continue;
		float x = WX(m->x, 1), y = WY(m->y);
		for (int k = 0; k < 6; k++) {
			float a = k * PI / 3 + m->t * 0.5f;
			line(x, y, x + cosf(a) * 8, y + sinf(a) * 8, 1.5f, C(0x6A2A2A));
		}
		circ(x, y, 5.5f, C(0x8A2A2A));
		circ(x, y, 2.2f, C(fmodf(m->t, 1) < 0.5f ? 0xFF4A4A : 0x5A1010));
		float dd = (m->x - EX.x) * (m->x - EX.x) + (m->y - EX.y) * (m->y - EX.y);
		if (dd < 60 * 60) ring(x, y, 14, 1, RGBA(C_RED, 0.6f), 14);
	}
	for (int i = 0; i < MAX_CHUNK; i++) {
		Chunk *c = &EX.ch[i];
		if (!c->alive || !onScr(c->x, c->y, 10)) continue;
		float x = WX(c->x, 0), y = WY(c->y);
		if (c->art >= 0) {
			circ(x, y, 9 + sinf(T * 6) * 2, RGBA(C_GOLD, 0.3f));
			starIcon(x, y, 6, C_GOLD);
		} else {
			circ(x, y, 5, RGBA(ORE[c->ore].color, 0.25f));
			gemIcon(x, y, 3.5f, ORE[c->ore].color, 1);
		}
	}
	for (int i = 0; i < MAX_DRONE; i++) {
		Drone *d = &EX.dr[i];
		if (!d->alive || !onScr(d->x, d->y, 14)) continue;
		float x = WX(d->x, 0), y = WY(d->y), c = cosf(d->ang), s = sinf(d->ang);
		circ(x - c * 6, y - s * 6, 3, RGBA(0xFF6A3A, 0.5f + 0.3f * frnd()));
		u32 col = d->hitT > 0 ? 0xFFFFFF : 0xE04A5A;
		tri(x + c * 9, y + s * 9, C(lighten(col, 0.3f)), x - c * 6 - s * 7, y - s * 6 + c * 7, C(col), x - c * 6 + s * 7, y - s * 6 - c * 7, C(darken(col, 0.4f)));
		circ(x, y, 2, C(0xFFE04A));
		if (d->hp < d->maxHp) rect(x - 8, y - 13, 16 * d->hp / d->maxHp, 2, C(C_RED));
	}
	for (int i = 0; i < MAX_BUL; i++) {
		Bullet *b = &EX.bl[i];
		if (!b->alive || !onScr(b->x, b->y, 6)) continue;
		float x = WX(b->x, -1), y = WY(b->y);
		lineG(x, y, C(0xFF6A6A), x - b->vx * 0.04f, y - b->vy * 0.04f, RGBA(0xFF6A6A, 0), 2.5f);
		circ(x, y, 2.2f, C(0xFFD0D0));
	}
	for (int i = 0; i < MAX_EFX; i++) {
		XFx *f = &EX.fx[i];
		if (!f->alive) continue;
		float p = f->t / f->max;
		float x = WX(f->x, -1), y = WY(f->y);
		switch (f->kind) {
		case XF_BEAM: {
			float x2 = WX(f->x2, -1), y2 = WY(f->y2);
			line(x, y, x2, y2, 4, RGBA(f->col, 0.35f * (1 - p)));
			line(x, y, x2, y2, 1.4f, RGBA(0xFFFFFF, 1 - p));
			break;
		}
		case XF_BOOM:
			circ(x, y, f->r * (0.4f + p * 0.7f), RGBA(f->col, 0.5f * (1 - p)));
			circ(x, y, f->r * (0.2f + p * 0.4f), RGBA(0xFFF0C0, 0.6f * (1 - p)));
			break;
		case XF_SPARK:
			for (int k = 0; k < 4; k++) {
				float a = k * 1.57f + i;
				line(x, y, x + cosf(a) * f->r * (1 + p), y + sinf(a) * f->r * (1 + p), 1, RGBA(f->col, 1 - p));
			}
			break;
		case XF_PICK: ring(x, y, f->r * (0.5f + p), 1.2f, RGBA(f->col, 1 - p), 10); break;
		case XF_RING: ring(x, y, f->r * p, 2, RGBA(f->col, 1 - p), 24); break;
		}
	}
	shipShape(WX(EX.x, 0), WY(EX.y), EX.ang, 1, EX.thrust, EX.boosting, EX.hitFlash, 1);
	// Reichweite andeuten
	ring(WX(EX.x, 0), WY(EX.y), EX.range, 1, RGBA(C_CYAN, 0.08f), 32);
	// Pfeil zum Tor
	if (!onScr(EX.gateX, EX.gateY, 0)) {
		float dx = EX.gateX - camX, dy = EX.gateY - camY, l = sqrtf(dx * dx + dy * dy);
		float sx = 200 + dx / l * 180, sy = 120 + dy / l * 100;
		float a = atan2f(dy, dx);
		u32 c = RGBA(C_PURPLE, 0.6f + 0.4f * sinf(T * 5));
		tri(sx + cosf(a) * 9, sy + sinf(a) * 9, c, sx + cosf(a + 2.4f) * 7, sy + sinf(a + 2.4f) * 7, c, sx + cosf(a - 2.4f) * 7, sy + sinf(a - 2.4f) * 7, c);
		text(sx - cosf(a) * 14, sy - sinf(a) * 14 - 6, 0.3f, C(C_PURPLE), C2D_AlignCenter, "Tor");
	}
	// HUD
	float d = -1;
	rrect(px(4, d), 4, 150, 34, 6, RGBA(0x060818, 0.7f));
	text(px(10, d), 5, 0.32f, C(C_DIM), 0, "Rumpf");
	bar(px(48, d), 9, 100, 6, EX.hull / EX.maxHull, C_RED, 0xFF9A8A);
	text(px(10, d), 20, 0.32f, C(C_DIM), 0, "Treibstoff");
	bar(px(64, d), 24, 84, 6, EX.fuel / EX.maxFuel, 0x2A8AD8, C_CYAN);
	rrect(px(TW - 124, d), 4, 120, 34, 6, RGBA(0x060818, 0.7f));
	textf(px(TW - 10, d), 5, 0.4f, C(C_TEXT), C2D_AlignRight, "Sektor %d", EX.sector);
	bool full = EX.cargo >= EX.maxCargo;
	textf(px(TW - 10, d), 20, 0.36f, C(full ? C_GOLD : C_DIM), C2D_AlignRight, "Fracht %.0f/%.0f", EX.cargo, EX.maxCargo);
	if (full && fmodf(T, 1) < 0.65f) textS(px(200, d), 44, 0.42f, C(C_GOLD), C2D_AlignCenter, "Laderaum voll! Zum Tor fliegen oder START");
	else if (EX.fuel < 10 && fmodf(T, 1) < 0.65f) textS(px(200, d), 44, 0.42f, C(C_RED), C2D_AlignCenter, "Treibstoff knapp!");
	if (expIntroT > 0) {
		float a = fminf(1, expIntroT);
		rrect(px(40, d), 150, 320, 46, 8, RGBA(0x060818, 0.8f * a));
		text(px(200, d), 154, 0.4f, RGBA(C_CYAN, a), C2D_AlignCenter, "Circle Pad: fliegen  ·  B: Boost  ·  Laser feuert selbst");
		text(px(200, d), 174, 0.38f, RGBA(C_TEXT, a), C2D_AlignCenter, "Radar antippen: Autopilot  ·  START: zurück");
	}
	if (EX.hitFlash > 0) rect(0, 0, TW, SH, RGBA(C_RED, EX.hitFlash * 0.5f));
	drawToast();
}

// ---------- Radar (unten)
#define RAD_X 6.0f
#define RAD_Y 6.0f
#define RAD_S 228.0f
static void drawExpBottom(void) {
	nHits = 0;
	rectG(0, 0, BW, SH, C(0x0A0E22), C(0x121838));
	float sc = RAD_S / WORLD;
	panel(RAD_X - 2, RAD_Y - 2, RAD_S + 4, RAD_S + 4, 6, 0x081028, 0x2E4A88);
	for (int k = 1; k < 4; k++) {
		rect(RAD_X + k * RAD_S / 4, RAD_Y, 1, RAD_S, RGBA(C_CYAN, 0.08f));
		rect(RAD_X, RAD_Y + k * RAD_S / 4, RAD_S, 1, RGBA(C_CYAN, 0.08f));
	}
	float sw = fmodf(T * 0.8f, 1) * RAD_S;
	rect(RAD_X, RAD_Y + sw, RAD_S, 1.5f, RGBA(C_CYAN, 0.15f));
	for (int i = 0; i < MAX_AST; i++) {
		Asteroid *a = &EX.ast[i];
		if (!a->alive) continue;
		float r = fmaxf(1.2f, a->r * sc * 1.3f);
		circ(RAD_X + a->x * sc, RAD_Y + a->y * sc, r, RGBA(a->relic ? C_GOLD : ORE[a->ore].color, a->relic ? 1 : 0.7f));
	}
	for (int i = 0; i < MAX_CHUNK; i++) if (EX.ch[i].alive) rect(RAD_X + EX.ch[i].x * sc, RAD_Y + EX.ch[i].y * sc, 1, 1, C(EX.ch[i].art >= 0 ? C_GOLD : 0xFFFFFF));
	for (int i = 0; i < MAX_MINE; i++) if (EX.mn[i].alive) circ(RAD_X + EX.mn[i].x * sc, RAD_Y + EX.mn[i].y * sc, 1.3f, C(0xC03A3A));
	for (int i = 0; i < MAX_DRONE; i++) if (EX.dr[i].alive) circ(RAD_X + EX.dr[i].x * sc, RAD_Y + EX.dr[i].y * sc, 2, C(C_RED));
	float gx = RAD_X + EX.gateX * sc, gy = RAD_Y + EX.gateY * sc;
	ring(gx, gy, 5 + sinf(T * 4), 1.5f, C(C_PURPLE), 12);
	rect(RAD_X + (camX - 200) * sc, RAD_Y + (camY - 120) * sc, 400 * sc, 1, RGBA(0xFFFFFF, 0.3f));
	rect(RAD_X + (camX - 200) * sc, RAD_Y + (camY + 120) * sc, 400 * sc, 1, RGBA(0xFFFFFF, 0.3f));
	rect(RAD_X + (camX - 200) * sc, RAD_Y + (camY - 120) * sc, 1, 240 * sc, RGBA(0xFFFFFF, 0.3f));
	rect(RAD_X + (camX + 200) * sc, RAD_Y + (camY - 120) * sc, 1, 240 * sc, RGBA(0xFFFFFF, 0.3f));
	if (EX.autopilot) {
		float ax = RAD_X + EX.apX * sc, ay = RAD_Y + EX.apY * sc;
		line(ax - 4, ay, ax + 4, ay, 1, C(C_GREEN));
		line(ax, ay - 4, ax, ay + 4, 1, C(C_GREEN));
		line(RAD_X + EX.x * sc, RAD_Y + EX.y * sc, ax, ay, 1, RGBA(C_GREEN, 0.4f));
	}
	shipShape(RAD_X + EX.x * sc, RAD_Y + EX.y * sc, EX.ang, 0.45f, 0, false, 0, 1);
	addHit(RAD_X, RAD_Y, RAD_S, RAD_S, A_RADAR, 0);
	// rechte Spalte
	float x = 240;
	textf(x + 38, 4, 0.48f, C(C_TEXT), C2D_AlignCenter, "Sektor %d", EX.sector);
	text(x, 24, 0.32f, C(C_DIM), 0, "Rumpf");
	bar(x, 35, 74, 6, EX.hull / EX.maxHull, C_RED, 0xFF9A8A);
	text(x, 44, 0.32f, C(C_DIM), 0, "Treibstoff");
	bar(x, 55, 74, 6, EX.fuel / EX.maxFuel, 0x2A8AD8, C_CYAN);
	text(x, 64, 0.32f, C(C_DIM), 0, "Fracht");
	bar(x, 75, 74, 6, EX.cargo / EX.maxCargo, 0x8A6A20, C_GOLD);
	for (int i = 0; i < NORE; i++) {
		float y = 88 + i * 18;
		gemIcon(x + 6, y + 7, 5, ORE[i].color, EX.ore[i] > 0 ? 1 : 0.3f);
		textFit(x + 16, y, 0.36f, C(EX.ore[i] > 0 ? C_TEXT : 0x4A5280), 0, 58, ORE[i].name);
		textf(x + 74, y, 0.36f, C(C_TEXT), C2D_AlignRight, "%.0f", EX.ore[i]);
	}
	if (EX.nArt) {
		starIcon(x + 6, 185, 5, C_GOLD);
		textf(x + 16, 178, 0.36f, C(C_GOLD), 0, "Artefakt x%d", EX.nArt);
	}
	button(x - 2, 200, 78, 34, C_GOLD, true, 0.42f, "Zurück");
	addHit(x - 4, 196, 82, 44, A_EXPRET, 0);
	drawModal();
}

// ---------- Hangar
static bool shipAffordable(void) {
	for (int i = 0; i < NSHIPUP; i++)
		if (G.s.ext.x.ship[i] < SHIPUP[i].max && ship_up_cost(&G, i) <= G.s.ext.x.alloy) return true;
	return false;
}

static void pageHangar(void) {
	float y0 = 104, rowH = 36;
	scrollMaxSet(P_HANGAR, NSHIPUP * rowH + 4, LIST_BOT - y0);
	float sc = scrollY[P_HANGAR];
	char b[32], d[64];
	for (int i = 0; i < NSHIPUP; i++) {
		float y = y0 + i * rowH - sc;
		if (y + rowH < 100 || y > LIST_BOT) continue;
		int l = G.s.ext.x.ship[i];
		bool maxed = l >= SHIPUP[i].max;
		double c = ship_up_cost(&G, i);
		bool can = !maxed && c <= G.s.ext.x.alloy;
		panel(6, y, 308, 32, 7, can ? 0x1A2A50 : C_PANEL, can ? 0x2A7AB0 : 0x2A3166);
		textFit(12, y + 2, 0.42f, C(C_TEXT), 0, 130, SHIPUP[i].name);
		textf(226, y + 3, 0.34f, C(maxed ? C_GREEN : C_DIM), C2D_AlignRight, maxed ? "MAX" : "%d/%d", l, SHIPUP[i].max);
		ship_up_desc(&G, i, d, sizeof d);
		textFit(12, y + 17, 0.34f, C(C_DIM), 0, 210, d);
		if (!maxed) {
			num(c, b);
			button(234, y + 4, 74, 24, C_CYAN, can, 0.4f, b);
			addHitC(6, y, 308, 32, A_SHIPUP, i, y0, LIST_BOT);
		}
	}
	maskBg(31, y0 - 2);
	panel(6, 32, 308, 68, 8, 0x14203E, 0x2E5A90);
	shipShape(32, 60, -0.6f + sinf(T) * 0.1f, 1.7f, 0.6f + 0.3f * sinf(T * 6), false, 0, 1);
	gemIcon(62, 44, 6, 0x8AC8FF, 1);
	num(G.s.ext.x.alloy, b);
	textFit(72, 35, 0.52f, C(0x8AC8FF), 0, 100, b);
	text(72 + fminf(100, 0), 52, 0.32f, C(C_DIM), 0, "Alloy");
	num(refinery_mult(&G), b);
	textf(62, 66, 0.34f, C(C_DIM), 0, "Raffinerie: Produktion x%s", b);
	textf(62, 81, 0.34f, C(C_DIM), 0, "Artefakte %d/%d  ·  Expeditionen %.0f", art_count(&G), NART, G.s.ext.x.expeditions);
	int smax = G.s.ext.x.sectorMax < 1 ? 1 : G.s.ext.x.sectorMax;
	if (expSector > smax) expSector = smax;
	if (expSector < 1) expSector = 1;
	button(226, 36, 84, 32, C_GREEN, true, 0.48f, "STARTEN");
	addHit(224, 34, 88, 36, A_HSTART, 0);
	button(226, 74, 20, 20, C_PANEL2, expSector > 1, 0.4f, "<");
	button(290, 74, 20, 20, C_PANEL2, expSector < smax, 0.4f, ">");
	textf(268, 76, 0.38f, C(C_TEXT), C2D_AlignCenter, "Sektor %d", expSector);
	addHit(222, 72, 30, 26, A_HSEC, -1);
	addHit(286, 72, 30, 26, A_HSEC, 1);
}

// ---------- Auftraege
static void pageContracts(void) {
	contract_fill(&G);
	char b[96], v1[32], v2[32];
	for (int k = 0; k < 3; k++) {
		float y = 60 + k * 49;
		double t, p = contract_progress(&G, k, &t);
		bool done = p >= t;
		panel(6, y, 308, 45, 8, done ? 0x1A3A2A : C_PANEL, done ? C_GREEN : 0x2A3166);
		contract_text(&G, k, b, sizeof b);
		textFit(12, y + 3, 0.42f, C(C_TEXT), 0, 214, b);
		bar(12, y + 23, 150, 6, (float)(p / t), 0x2A7A50, C_GREEN);
		num(fmin(p, t), v1);
		num(t, v2);
		textf(168, y + 19, 0.32f, C(C_DIM), 0, "%s/%s", v1, v2);
		num(G.s.ext.x.contract[k].reward, v1);
		textf(12, y + 31, 0.32f, C(C_GOLD), 0, "+%s Alloy  +10 min Produktion", v1);
		button(234, y + 9, 74, 26, C_GREEN, done, 0.4f, "Einlösen");
		if (done) addHit(230, y, 84, 45, A_CLAIM, k);
	}
	subHeader("Aufträge");
}

// ---------- Artefakte
static void pageArts(void) {
	float y0 = 58, rowH = 30;
	scrollMaxSet(P_ARTS, NART * rowH + 22, LIST_BOT - y0);
	float sc = scrollY[P_ARTS];
	text(160, y0 - sc, 0.34f, C(C_DIM), C2D_AlignCenter, "Findest du in Relikt-Asteroiden (goldener Ring).");
	char d[48];
	for (int i = 0; i < NART; i++) {
		float y = y0 + 18 + i * rowH - sc;
		if (y + rowH < 54 || y > LIST_BOT) continue;
		bool has = art_has(&G, i);
		panel(6, y, 308, 27, 6, has ? 0x2A2448 : 0x10142E, has ? 0x8A6A30 : 0x1C2350);
		if (has) starIcon(20, y + 13.5f, 7, C_GOLD);
		else circ(20, y + 13.5f, 6, C(0x1C2350));
		textFit(34, y + 1, 0.4f, C(has ? C_TEXT : 0x5A6290), 0, 140, has ? ART[i].name : "???");
		art_desc(i, d, sizeof d);
		textFit(306, y + 6, 0.36f, C(has ? C_GOLD : 0x4A5280), C2D_AlignRight, 130, has ? d : "unentdeckt");
	}
	char t[32];
	snprintf(t, sizeof t, "Artefakte %d/%d", art_count(&G), NART);
	subHeader(t);
}

// ---------- Eingabe & Ereignisse waehrend der Expedition
static void expEvents(void) {
	expShootCd -= 1 / 60.0f;
	expPickCd -= 1 / 60.0f;
	expFullT -= 1 / 60.0f;
	if (EX.evShoot && expShootCd <= 0) { play(SND_TAB, 1.8f + frnd() * 0.2f, 0.12f); expShootCd = 0.12f; }
	if (EX.evBreak) play(SND_NOVA, 2.2f, 0.3f);
	if (EX.evPickup && expPickCd <= 0) { play(SND_TAP, 1.5f + frnd() * 0.3f, 0.3f); expPickCd = 0.06f; }
	if (EX.evHurt) play(SND_ERR, 0.8f, 0.45f);
	if (EX.evBoom) play(SND_NOVA, 1.4f, 0.5f);
	if (EX.evGate) { play(SND_ACH, 1, 0.6f); toast(C_PURPLE, "Sprungtor!", "Sektor %d erreicht. Treibstoff +25%%.", EX.sector); }
	if (EX.evArt) { play(SND_ACH, 1.3f, 0.7f); toast(C_GOLD, "Artefakt gefunden!", "%s", ART[EX.artFound[EX.nArt - 1]].name); }
	if (EX.evFull && expFullT <= 0) { toast(C_GOLD, "Laderaum voll!", "Fliege zum Sprungtor oder kehre zurück."); expFullT = 15; }
	EX.evShoot = EX.evBreak = EX.evPickup = EX.evHurt = EX.evBoom = EX.evGate = EX.evArt = EX.evFull = EX.evHit = 0;
}

static void handleExpInput(u32 kDown, u32 kHeld) {
	touchPosition tp;
	hidTouchRead(&tp);
	if (modal) {
		if ((kDown & (KEY_A | KEY_B)) || (kDown & KEY_TOUCH)) modal = MD_NONE;
		return;
	}
	if (kHeld & KEY_TOUCH) {
		Hit *h = hitAt(tp.px, tp.py);
		if (h && h->act == A_RADAR) {
			EX.autopilot = true;
			EX.apX = (tp.px - RAD_X) / RAD_S * WORLD;
			EX.apY = (tp.py - RAD_Y) / RAD_S * WORLD;
		} else if (h && h->act == A_EXPRET && (kDown & KEY_TOUCH)) EX.done = true;
	}
	if (kDown & (KEY_START | KEY_X)) EX.done = true;
}

static void expInput(u32 kHeld, float *ix, float *iy, bool *boost) {
	circlePosition cp;
	hidCircleRead(&cp);
	float x = cp.dx / 150.0f, y = -cp.dy / 150.0f;
	if (fabsf(x) < 0.15f) x = 0;
	if (fabsf(y) < 0.15f) y = 0;
	if (kHeld & KEY_LEFT) x = -1;
	if (kHeld & KEY_RIGHT) x = 1;
	if (kHeld & KEY_UP) y = -1;
	if (kHeld & KEY_DOWN) y = 1;
	*ix = x;
	*iy = y;
	*boost = (kHeld & (KEY_B | KEY_R | KEY_L)) != 0;
}
