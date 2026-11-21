// Goo Alert (Glibberalarm) v2 - 3DS Frontend mit Sprite-Grafik (citro2d)
#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "td.h"
#include "gfx_idx.h"
#include "sprites_t3x.h"
#include "bg0_t3x.h"
#include "bg1_t3x.h"
#include "bg2_t3x.h"
#include "bg3_t3x.h"
#include "bg4_t3x.h"
#include "bg5_t3x.h"
#include "title_t3x.h"
#include "menubg_t3x.h"

#define TW 400
#define BW 320
#define SH 240
#define FONT_H 30.0f
#define PI 3.14159265f
#define BS 0.8f

#define C_TEXT 0xF4F6FF
#define C_DIM 0xA8B0D8
#define C_CYAN 0x4AD8FF
#define C_GOLD 0xFFD25A
#define C_GREEN 0x5AE07A
#define C_RED 0xFF5A6E
#define C_PINK 0xFF6AB0

#define SAVE_DIR "sdmc:/3ds/glibberalarm"
#define SAVE_PATH SAVE_DIR "/save.dat"
#define SAVE_TMP SAVE_DIR "/save.tmp"
#define SAVE_MAGIC 0x474C4942u
#define SAVE_VERSION 2
#define MAXMAP 8

static TD G;
u32 __stacksize__ = 256 * 1024; // groesserer Stack (Standard: 32 KB)
static C3D_RenderTarget *tgtTop, *tgtTopR, *tgtBot;
static C2D_TextBuf tbuf;
static float T, eyeSign, slider3d;

// ============================================================ Speicherstand
typedef struct {
	uint8_t alive, type, a, b, target, pad[3];
	float x, y;
	int32_t spent;
	double pops;
} SaveTower;
typedef struct { // Version 1 (nur zum Uebernehmen alter Spielstaende)
	uint32_t magic, version;
	uint8_t sound, stereo, autoStart, pad;
	uint8_t best[4][3], won[4][3];
	uint8_t hasResume, endless, pad2[2];
	int32_t map, diff, wave, lives;
	double coins;
	SaveTower tw[MAX_TW];
	double totalPops;
	double reserved[32];
} SaveV1;
typedef struct {
	uint32_t magic, version;
	uint8_t sound, stereo, autoStart, pad;
	uint8_t best[MAXMAP][3], won[MAXMAP][3];
	uint8_t hasResume, endless, pad2[2];
	int32_t map, diff, wave, lives;
	double coins;
	SaveTower tw[MAX_TW];
	double totalPops, xp, games;
	float towerXp[16];
	double reserved[32];
} SaveData;
static SaveData SD;
static SaveData snap;

static void saveWrite(void) {
	mkdir("sdmc:/3ds", 0777);
	mkdir(SAVE_DIR, 0777);
	FILE *f = fopen(SAVE_TMP, "wb");
	if (!f) return;
	size_t n = fwrite(&SD, 1, sizeof SD, f);
	fclose(f);
	if (n != sizeof SD) return;
	remove(SAVE_PATH);
	rename(SAVE_TMP, SAVE_PATH);
}
static void saveRead(void) {
	memset(&SD, 0, sizeof SD);
	SD.magic = SAVE_MAGIC;
	SD.version = SAVE_VERSION;
	SD.sound = 1;
	SD.stereo = 1;
	FILE *f = fopen(SAVE_PATH, "rb");
	if (!f) return;
	static uint8_t buf[sizeof(SaveData) + sizeof(SaveV1)];
	size_t n = fread(buf, 1, sizeof buf, f);
	fclose(f);
	uint32_t magic, ver;
	if (n < 8) return;
	memcpy(&magic, buf, 4);
	memcpy(&ver, buf + 4, 4);
	if (magic != SAVE_MAGIC) return;
	if (ver == 2 && n >= sizeof(SaveData)) memcpy(&SD, buf, sizeof SD);
	else if (ver == 1 && n >= sizeof(SaveV1)) {
		static SaveV1 o;
		memcpy(&o, buf, sizeof o);
		SD.sound = o.sound; SD.stereo = o.stereo; SD.autoStart = o.autoStart;
		for (int m = 0; m < 4; m++)
			for (int d = 0; d < 3; d++) { SD.best[m][d] = o.best[m][d]; SD.won[m][d] = o.won[m][d]; }
		SD.hasResume = o.hasResume; SD.endless = o.endless;
		SD.map = o.map; SD.diff = o.diff; SD.wave = o.wave; SD.lives = o.lives; SD.coins = o.coins;
		memcpy(SD.tw, o.tw, sizeof SD.tw);
		// alte Fortschritte in XP umrechnen
		for (int m = 0; m < 4; m++) for (int d = 0; d < 3; d++) SD.xp += SD.best[m][d] * 40.0 * (1 + d * 0.25);
		SD.version = SAVE_VERSION;
	}
}
static void snapshot(void) {
	SaveData *s = &snap;
	s->hasResume = 1;
	s->endless = G.endless;
	s->map = G.map; s->diff = G.diff; s->wave = G.wave; s->lives = G.lives; s->coins = G.coins;
	for (int i = 0; i < MAX_TW; i++) {
		Tower *t = &G.tw[i];
		SaveTower *o = &s->tw[i];
		memset(o, 0, sizeof *o);
		o->alive = t->alive; o->type = t->type; o->a = t->a; o->b = t->b; o->target = t->target;
		o->x = t->x; o->y = t->y; o->spent = t->spent; o->pops = t->pops;
	}
}
static void commitSnapshot(void) {
	SD.hasResume = snap.hasResume;
	SD.endless = snap.endless;
	SD.map = snap.map; SD.diff = snap.diff; SD.wave = snap.wave; SD.lives = snap.lives; SD.coins = snap.coins;
	memcpy(SD.tw, snap.tw, sizeof SD.tw);
}
static void restoreResume(void) {
	td_new(&G, SD.map, SD.diff);
	G.wave = SD.wave; G.lives = SD.lives; G.coins = SD.coins; G.endless = SD.endless;
	for (int i = 0; i < MAX_TW; i++) {
		SaveTower *o = &SD.tw[i];
		Tower *t = &G.tw[i];
		memset(t, 0, sizeof *t);
		if (!o->alive || o->type >= NTOWER) continue;
		t->alive = 1; t->type = o->type; t->a = o->a; t->b = o->b; t->target = o->target;
		t->x = o->x; t->y = o->y; t->spent = o->spent; t->pops = o->pops; t->ang = -1.57f;
	}
}

static aptHookCookie hookCookie;
static void onAptHook(APT_HookType hook, void *param) {
	(void)param;
	if (hook == APTHOOK_ONSUSPEND || hook == APTHOOK_ONSLEEP || hook == APTHOOK_ONEXIT) saveWrite();
}

// ============================================================ Fortschritt
static int rank(void) { return rank_of(SD.xp); }
static bool towerUnlocked(int t) { return rank() >= TOWER_RANK[t]; }
static bool mapUnlocked(int m) { return rank() >= MAP_RANK[m]; }

// ============================================================ Farben & Formen
static inline u32 RGBA(u32 hex, float a) {
	if (a < 0) a = 0;
	if (a > 1) a = 1;
	return C2D_Color32((hex >> 16) & 255, (hex >> 8) & 255, hex & 255, (u8)(a * 255));
}
static inline u32 C(u32 hex) { return RGBA(hex, 1); }
static u32 hexMix(u32 a, u32 b, float t) {
	if (t < 0) t = 0;
	if (t > 1) t = 1;
	int r = ((a >> 16) & 255) * (1 - t) + ((b >> 16) & 255) * t;
	int g = ((a >> 8) & 255) * (1 - t) + ((b >> 8) & 255) * t;
	int bl = (a & 255) * (1 - t) + (b & 255) * t;
	return (r << 16) | (g << 8) | bl;
}
static u32 lighten(u32 h, float t) { return hexMix(h, 0xFFFFFF, t); }
static u32 darken(u32 h, float t) { return hexMix(h, 0x000000, t); }
static void rect(float x, float y, float w, float h, u32 c) { C2D_DrawRectSolid(x, y, 0, w, h, c); }
static void rectG(float x, float y, float w, float h, u32 ct, u32 cb) { C2D_DrawRectangle(x, y, 0, w, h, ct, ct, cb, cb); }
static void circ(float x, float y, float r, u32 c) { if (r > 0.3f) C2D_DrawCircleSolid(x, y, 0, r, c); }
static void line(float x0, float y0, float x1, float y1, float th, u32 c) { C2D_DrawLine(x0, y0, c, x1, y1, c, th, 0); }
static void ring(float x, float y, float r, float th, u32 c, int seg) {
	for (int k = 0; k < seg; k++) {
		float a0 = 2 * PI * k / seg, a1 = 2 * PI * (k + 1) / seg;
		line(x + cosf(a0) * r, y + sinf(a0) * r, x + cosf(a1) * r, y + sinf(a1) * r, th, c);
	}
}
static u32 lerpC(u32 a, u32 b, float t) {
	if (t < 0) t = 0;
	if (t > 1) t = 1;
	u32 r = 0;
	for (int s = 0; s < 32; s += 8) {
		float va = (a >> s) & 255, vb = (b >> s) & 255;
		r |= ((u32)(va + (vb - va) * t) & 255) << s;
	}
	return r;
}
// Abgerundetes Rechteck als Dreiecksfaecher mit vertikalem Verlauf.
// Keine Ueberlappung -> auch halbtransparent ohne dunkle Flecken.
static void rrectC(float x, float y, float w, float h, float r, u32 ct, u32 cb) {
	if (w <= 0 || h <= 0) return;
	if (r * 2 > h) r = h / 2;
	if (r * 2 > w) r = w / 2;
	if (r < 0) r = 0;
	float cx = x + w / 2, cy = y + h / 2;
	u32 cc = lerpC(ct, cb, 0.5f);
	const int SEG = 5;
	float px = 0, py = 0;
	bool first = true;
	for (int c = 0; c < 5; c++) {
		int q = c % 4;
		float ox = (q == 0 || q == 3) ? x + w - r : x + r;
		float oy = (q < 2) ? y + h - r : y + r;
		for (int k = 0; k <= SEG; k++) {
			if (c == 4 && k > 0) break;
			float a = (q * 90 + k * 90.0f / SEG) * PI / 180;
			float qx = ox + cosf(a) * r, qy = oy + sinf(a) * r;
			if (!first) C2D_DrawTriangle(cx, cy, cc, px, py, lerpC(ct, cb, (py - y) / h), qx, qy, lerpC(ct, cb, (qy - y) / h), 0);
			first = false;
			px = qx; py = qy;
		}
	}
}
static void panel(float x, float y, float w, float h, float r, u32 fill, u32 border, float a) {
	rrectC(x, y, w, h, r, RGBA(lighten(border, 0.15f), a), RGBA(darken(border, 0.35f), a));
	rrectC(x + 1.5f, y + 1.5f, w - 3, h - 3, r - 1.5f, RGBA(lighten(fill, 0.1f), a), RGBA(darken(fill, 0.2f), a));
}

// ============================================================ Sprites
static C2D_SpriteSheet sheetSpr, sheetBg[NMAP], sheetTitle, sheetMenu;
static C2D_Image SPR[NSPRITE];
static C2D_Image BG[NMAP], TITLEIMG, MENUIMG;

static C2D_Image loadBig(C2D_SpriteSheet *sh, const void *data, size_t size) {
	*sh = C2D_SpriteSheetLoadFromMem(data, size);
	C2D_Image im = C2D_SpriteSheetGetImage(*sh, 0);
	C3D_TexSetFilter(im.tex, GPU_LINEAR, GPU_LINEAR);
	return im;
}
static void gfxLoad(void) {
	sheetSpr = C2D_SpriteSheetLoadFromMem(sprites_t3x, sprites_t3x_size);
	for (int i = 0; i < NSPRITE; i++) SPR[i] = C2D_SpriteSheetGetImage(sheetSpr, i);
	C3D_TexSetFilter(SPR[0].tex, GPU_LINEAR, GPU_LINEAR);
	BG[0] = loadBig(&sheetBg[0], bg0_t3x, bg0_t3x_size);
	BG[1] = loadBig(&sheetBg[1], bg1_t3x, bg1_t3x_size);
	BG[2] = loadBig(&sheetBg[2], bg2_t3x, bg2_t3x_size);
	BG[3] = loadBig(&sheetBg[3], bg3_t3x, bg3_t3x_size);
	BG[4] = loadBig(&sheetBg[4], bg4_t3x, bg4_t3x_size);
	BG[5] = loadBig(&sheetBg[5], bg5_t3x, bg5_t3x_size);
	TITLEIMG = loadBig(&sheetTitle, title_t3x, title_t3x_size);
	MENUIMG = loadBig(&sheetMenu, menubg_t3x, menubg_t3x_size);
}
static void gfxFree(void) {
	C2D_SpriteSheetFree(sheetSpr);
	for (int i = 0; i < NMAP; i++) C2D_SpriteSheetFree(sheetBg[i]);
	C2D_SpriteSheetFree(sheetTitle);
	C2D_SpriteSheetFree(sheetMenu);
}
// Sprite mittig zeichnen; col/blend faerben (blend 0 = Originalfarbe), alpha multipliziert
static void spr(int id, float x, float y, float sx, float sy, float ang, u32 col, float blend, float alpha) {
	C2D_ImageTint t;
	C2D_PlainImageTint(&t, RGBA(col, alpha), blend);
	C2D_DrawImageAtRotated(SPR[id], x, y, 0, ang, &t, sx, sy);
}
static void sprN(int id, float x, float y, float s) { spr(id, x, y, s, s, 0, 0xFFFFFF, 0, 1); }
static void shadow(float x, float y, float w, float h, float a) { spr(SPR_GLOW, x, y, w / 32, h / 32, 0, 0x000000, 1, a); }

// ============================================================ Text
static C2D_Text mk(const char *s) {
	C2D_Text t;
	C2D_TextParse(&t, tbuf, s);
	C2D_TextOptimize(&t);
	return t;
}
// Text mit Schatten, optional verkleinert auf maxw
static void textS(float x, float y, float sz, u32 col, int align, float maxw, const char *s) {
	C2D_Text t = mk(s);
	float w, h;
	C2D_TextGetDimensions(&t, sz, sz, &w, &h);
	float s2 = sz;
	if (maxw > 0 && w > maxw) s2 = sz * maxw / w;
	float yy = y + (sz - s2) * FONT_H * 0.5f;
	C2D_DrawText(&t, C2D_WithColor | align, x + 1, yy + 1.2f, 0, s2, s2, RGBA(0x000000, ((col >> 24) & 255) / 255.0f * 0.75f));
	C2D_DrawText(&t, C2D_WithColor | align, x, yy, 0, s2, s2, col);
}
static void textSf(float x, float y, float sz, u32 col, int align, float maxw, const char *fmt, ...) {
	char b[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(b, sizeof b, fmt, ap);
	va_end(ap);
	textS(x, y, sz, col, align, maxw, b);
}
static void textWrap(float x, float y, float sz, u32 col, float w, const char *s) {
	C2D_Text t = mk(s);
	C2D_DrawText(&t, C2D_WithColor | C2D_WordWrap, x, y, 0, sz, sz, col, w);
}

#include "sound.inc"

// ============================================================ UI-Zustand
enum { ST_TITLE, ST_MAPS, ST_GAME, ST_HELP, ST_TOWERS };
static int state = ST_TITLE;
static int selMap = 0, selTowerInfo = 0;
static int sel = -1, placeType = -1;
static bool placeDrag;
static float ghostX = 200, ghostY = 120;
static float stripScroll, stripV;
static int ff = 0;
static float autoT, panelAnim;
static char bannerText[64], bannerSub[80];
static float bannerT;
static u32 bannerCol = C_GOLD;
static bool endShown;
static float helpScroll;
static float overT;
static double endXp;
enum { OV_NONE, OV_PAUSE, OV_WIN, OV_LOSE, OV_CONFIRM_MENU };
static int overlay;
static bool quitReq;
static float pressX = -100, pressY = -100, pressT;
static float shake;
static double xpCommitted;

typedef struct { float x, y, w, h; int act, p; } Hit;
#define NHIT 112
static Hit hits[NHIT];
static int nHits;
enum { A_NONE, A_SLOT, A_FIELD, A_PLAY, A_PAUSE, A_UPG, A_SELL, A_TARGET, A_CLOSE, A_OV, A_MENU, A_MAP, A_DIFF, A_BACK, A_TINFO };
static void addHit(float x, float y, float w, float h, int act, int p) {
	if (nHits < NHIT) hits[nHits++] = (Hit){x, y, w, h, act, p};
}
static Hit *hitAt(float x, float y) {
	for (int i = nHits - 1; i >= 0; i--) {
		Hit *h = &hits[i];
		if (x >= h->x && x < h->x + h->w && y >= h->y && y < h->y + h->h) return h;
	}
	return NULL;
}
static void banner(u32 col, const char *a, const char *fmt, ...) {
	snprintf(bannerText, sizeof bannerText, "%s", a);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(bannerSub, sizeof bannerSub, fmt, ap);
	va_end(ap);
	bannerT = 2.8f;
	bannerCol = col;
}

// Knopf mit Verlauf, Lippe, Glanz und Drueck-Effekt
static void button(float x, float y, float w, float h, u32 col, bool on, float sz, const char *label, int icon) {
	bool pressed = pressT > 0 && pressX >= x && pressX < x + w && pressY >= y && pressY < y + h;
	float s = pressed ? 0.93f : 1;
	float cx = x + w / 2, cy = y + h / 2;
	w *= s; h *= s;
	x = cx - w / 2; y = cy - h / 2;
	u32 base = on ? col : 0x4A5080;
	rrectC(x, y + 2.5f, w, h, 7, C(darken(base, 0.5f)), C(darken(base, 0.6f)));
	rrectC(x, y, w, h, 7, C(lighten(base, 0.25f)), C(darken(base, 0.12f)));
	rrectC(x + 3, y + 2, w - 6, h * 0.4f, 5, RGBA(0xFFFFFF, on ? 0.35f : 0.12f), RGBA(0xFFFFFF, on ? 0.06f : 0.02f));
	float lum = (((base >> 16) & 255) * 0.3f + ((base >> 8) & 255) * 0.59f + (base & 255) * 0.11f) / 255;
	u32 tc = !on ? 0xC0C4DC : lum > 0.62f ? 0x1A1A30 : 0xFFFFFF;
	float tx = cx;
	if (icon >= 0) {
		float isz = fminf(h * 0.62f, 18) / 16;
		if (!label[0]) { sprN(icon, cx, cy, isz * 1.2f); return; }
		C2D_Text t = mk(label);
		float tw, th;
		C2D_TextGetDimensions(&t, sz, sz, &tw, &th);
		float total = fminf(tw, w - 34) + 16 * isz + 5;
		sprN(icon, cx - total / 2 + 8 * isz, cy, isz);
		tx = cx + 8 * isz + 2.5f;
	}
	textS(tx, cy - FONT_H * sz / 2 + 1, sz, C(tc), C2D_AlignCenter, w - 12 - (icon >= 0 ? 22 : 0), label);
}

static void bar(float x, float y, float w, float h, float p, u32 c1, u32 c2) {
	if (p < 0) p = 0;
	if (p > 1) p = 1;
	rrectC(x, y, w, h, h / 2, C(0x0A0E28), C(0x1A2048));
	if (p > 0.02f) rrectC(x + 1, y + 1, (w - 2) * p, h - 2, (h - 2) / 2, C(lighten(c1, 0.25f)), C(c2));
}

// ============================================================ Spielfeld
static float FS, FOX, FOY;
static bool FTOP;
static int nAliveEn;
static inline float FX(float x, float depth) { return FOX + x * FS + (FTOP ? eyeSign * depth * slider3d : 0); }
static inline float FY(float y) { return FOY + y * FS; }

static void drawGround(void) { C2D_DrawImageAt(BG[G.map], FX(0, 4), FY(0), 0, NULL, FS, FS); }

// Umgebungs-Animationen je Thema
static void drawAmbient(void) {
	int th = MAPS[G.map].theme;
	for (int i = 0; i < 26; i++) {
		float s1 = fmodf(i * 0.6180339f, 1), s2 = fmodf(i * 0.3819660f + 0.13f, 1), z = 0.5f + 0.5f * fmodf(i * 0.7548f, 1);
		float x, y, a = 0.7f, sz;
		u32 col;
		switch (th) {
		case 0: x = fmodf(s1 * FW + T * 14 * z, FW + 20) - 10; y = fmodf(s2 * FH + T * 8 * z + sinf(T + i) * 6 + FH, FH); col = i % 3 ? 0x9AE060 : 0xF0D050; sz = 0.16f; break;
		case 1: x = s1 * FW; y = s2 * FH; a = fmaxf(0, sinf(T * 2 + i * 1.7f)); col = 0xFFFFFF; sz = 0.4f; break;
		case 2: x = s1 * FW + sinf(T * 0.5f + i) * 10; y = fmodf(s2 * FH - T * 6 * z + FH * 3, FH); col = i % 2 ? 0x8AE8FF : 0xD08AFF; sz = 0.2f; a = 0.5f + 0.4f * sinf(T * 3 + i); break;
		case 3: x = s1 * FW + sinf(T + i) * 5; y = fmodf(s2 * FH - T * 18 * z + FH * 3, FH); col = 0xFF8A2A; sz = 0.16f; a = 0.85f; break;
		case 4: x = fmodf(s1 * FW + sinf(T * 0.8f + i) * 12 + FW, FW); y = fmodf(s2 * FH + T * 16 * z, FH); col = 0xFFFFFF; sz = 0.13f + 0.08f * z; a = 0.9f; break;
		default: x = s1 * FW; y = fmodf(s2 * FH + T * 5, FH); col = i % 2 ? 0xFF9AD0 : 0xFFFFFF; sz = 0.4f; a = fmaxf(0, sinf(T * 2.5f + i * 2.1f)); break;
		}
		if (th == 1 || th == 5) spr(SPR_SPARK, FX(x, -1), FY(y), sz * FS, sz * FS, T, col, 1, a);
		else spr(SPR_GLOW, FX(x, -1), FY(y), sz * FS, sz * FS, 0, col, 1, a);
	}
}

static void drawTower(const Tower *t, float alpha, bool ghost) {
	float x = FX(t->x, 0), y = FY(t->y), s = FS;
	int tier = t->a > t->b ? t->a : t->b;
	shadow(x + 1.5f * s, y + 3 * s, 30 * s, 18 * s, 0.45f * alpha);
	if (tier >= 3) spr(SPR_RING, x, y, 31 * s / 32, 31 * s / 32, T, tier >= 4 ? C_GOLD : 0xC0D8FF, 1, (0.55f + 0.2f * sinf(T * 4)) * alpha);
	if (tier >= 4) spr(SPR_GLOW, x, y, 48 * s / 32, 48 * s / 32, 0, C_GOLD, 1, 0.25f * alpha);
	float ang = t->ang;
	if (t->type == T_BIENE || t->type == T_DACHS) ang = 0;
	if (t->type == T_SEEIGEL) ang = T * 0.5f;
	float sc = s * (t->flash > 0 ? 1.08f : 1);
	spr(SPR_TW0 + t->type, x, y, sc, sc, ang, 0xFFFFFF, 0, alpha);
	if (ghost) return;
	if (tier >= 4) spr(SPR_I_CROWN, x, y - 14 * s, 0.7f * s, 0.7f * s, 0, 0xFFFFFF, 0, alpha);
	for (int k = 0; k < t->a; k++) circ(x - 10 * s + k * 2.8f * s, y + 12.5f * s, 1.2f * s, RGBA(C_CYAN, alpha));
	for (int k = 0; k < t->b; k++) circ(x + 10 * s - k * 2.8f * s, y + 12.5f * s, 1.2f * s, RGBA(C_GOLD, alpha));
}

static void drawEnemy(const Enemy *e) {
	const EnemyDef *d = &EDEF[e->type];
	int id = SPR_EN0 + e->type;
	float r = d->radius, s = FS;
	float sq = sinf(e->wob) * 0.08f;
	float ex = FX(e->x, -1), ey = FY(e->y - r * 0.1f);
	bool geist = e->mods & M_GEIST;
	bool simple = e->type < E_KOENIG && ((!FTOP && nAliveEn > 90) || nAliveEn > 200);
	if (!simple) shadow(ex + 1 * s, ey + r * 0.75f * s, r * 2.3f * s, r * 0.9f * s, geist ? 0.15f : 0.35f);
	float cx = GOO_CENTER[e->type][0], cy = GOO_CENTER[e->type][1];
	bool flip = e->dirx < -0.3f;
	float sx = s * (1 + sq), sy = s * (1 - sq);
	C2D_ImageTint tint;
	u32 tc = 0xFFFFFF;
	float blend = 0, al = geist ? 0.45f : 1;
	if (geist) { tc = 0xC8D8FF; blend = 0.35f; }
	if (e->hitT > 0) { tc = 0xFFFFFF; blend = fminf(0.7f, e->hitT * 6); }
	C2D_PlainImageTint(&tint, RGBA(tc, al), blend);
	float drawX = flip ? ex + cx * sx : ex - cx * sx;
	C2D_DrawImageAt(SPR[id], drawX, ey - cy * sy, 0, &tint, flip ? -sx : sx, sy);
	if (e->mods & M_PANZER) spr(SPR_HELMET, ex, ey - r * 0.55f * s, r * 2.2f * s / 32, r * 2.2f * s / 32, 0, 0xFFFFFF, 0, al);
	if (e->mods & M_REGEN) spr(SPR_RING, ex, ey, r * 2.7f * s / 32, r * 2.5f * s / 32, 0, C_PINK, 1, 0.5f + 0.35f * sinf(T * 6 + e->id));
	if (e->freezeT > 0) spr(SPR_ICEBLOCK, ex, ey, r * 2.6f * s / 32, r * 2.3f * s / 32, 0, 0xFFFFFF, 0, 0.85f);
	if (!simple && (e->acidT > 0 || e->slowT > 0)) spr(SPR_P_SLIME, ex + r * 0.5f * s, ey + r * 0.45f * s, s * 0.6f, s * 0.6f, 0, 0xFFFFFF, 0, 0.9f);
	if (e->type >= E_KOENIG) {
		float hpMax = d->hp * (G.cur.hpMul > 0 ? G.cur.hpMul : 1);
		float p = e->hp / hpMax;
		float by = ey - cy * sy - 4 * s;
		u32 hc = p > 0.5f ? C_GREEN : p > 0.25f ? C_GOLD : C_RED;
		rrectC(ex - r * s, by, r * 2 * s, 3.5f * s, 1.5f * s, RGBA(0x000000, 0.7f), RGBA(0x000000, 0.7f));
		rrectC(ex - r * s + 0.5f, by + 0.5f, (r * 2 * s - 1) * p, 2.5f * s, 1.2f * s, C(lighten(hc, 0.2f)), C(hc));
	}
}

static void drawProj(const Proj *p) {
	float x = FX(p->x, -2), y = FY(p->y), s = FS;
	float ang = atan2f(p->vy, p->vx);
	switch (p->kind) {
	case P_SPIKE: spr(SPR_P_SPIKE, x, y, s, s, ang, 0xFFFFFF, 0, 1); break;
	case P_BLADE: spr(SPR_P_BLADE, x, y, s, s, ang, 0xFFFFFF, 0, 1); break;
	case P_SHARD: spr(SPR_P_SHARD, x, y, s, s, p->rot, 0xFFFFFF, 0, 1); break;
	case P_BOMB: spr(SPR_P_BOMB, x, y, s, s, p->rot * 0.3f, 0xFFFFFF, 0, 1); break;
	case P_MINIBOMB: spr(SPR_P_MINIBOMB, x, y, s, s, 0, 0xFFFFFF, 0, 1); break;
	case P_BOOMER:
		if (p->flags & PF_AP) spr(SPR_GLOW, x, y, 0.5f * s, 0.5f * s, 0, 0xFF8A3A, 1, 0.6f);
		spr(SPR_P_BOOMER, x, y, s, s, p->rot, 0xFFFFFF, 0, 1);
		break;
	case P_SLIME: spr(SPR_P_SLIME, x, y, s, s, 0, 0xFFFFFF, 0, 1); break;
	case P_ICE: spr(SPR_P_ICE, x, y, s, s, ang, 0xFFFFFF, 0, 1); break;
	case P_SHURIKEN: spr(SPR_P_SHURIKEN, x, y, s, s, p->rot, 0xFFFFFF, 0, 1); break;
	case P_MAGIC: spr(SPR_P_MAGIC, x, y, s, s, 0, 0xFFFFFF, 0, 1); break;
	case P_FIREBALL: spr(SPR_P_FIREBALL, x, y, s * 1.2f, s * 1.2f, p->rot, 0xFFFFFF, 0, 1); break;
	case P_PILE: spr(SPR_P_PILE, FX(p->x, 1), y, s, s, p->rot, (p->flags & PF_AP) ? 0xFF9A5A : 0xFFFFFF, (p->flags & PF_AP) ? 0.3f : 0, fminf(1, p->life)); break;
	case P_LASER: spr(SPR_P_LASER, x, y, s * (p->radius > 4 ? 1.4f : 1), s * (p->radius > 4 ? 1.8f : 1), ang, 0xFFFFFF, 0, 1); break;
	}
}

static void drawFx(const Fx *f, int idx) {
	float p = f->t / f->max, s = FS;
	float x = FX(f->x, -2), y = FY(f->y);
	switch (f->kind) {
	case FX_RING: spr(SPR_RING, x, y, f->r * (0.5f + p) * 2 * s / 32, f->r * (0.5f + p) * 2 * s / 32, 0, f->col, 1, 1 - p); break;
	case FX_POP: {
		float sc = f->r * (0.9f + p * 1.3f) * 2 * s / 24;
		spr(SPR_SPLAT, x, y, sc, sc, idx * 1.3f, f->col, 1, (1 - p) * 0.95f);
		break;
	}
	case FX_BOLT: {
		float x2 = FX(f->x2, -2), y2 = FY(f->y2);
		float lx = x, ly = y;
		srand(idx * 31 + (int)(f->t * 30));
		for (int k = 1; k <= 4; k++) {
			float q = k / 4.0f;
			float nx = x + (x2 - x) * q + (k < 4 ? (frnd() - 0.5f) * 8 * s : 0);
			float ny = y + (y2 - y) * q + (k < 4 ? (frnd() - 0.5f) * 8 * s : 0);
			line(lx, ly, nx, ny, 3.2f * s, RGBA(f->col, 0.5f * (1 - p)));
			line(lx, ly, nx, ny, 1.1f * s, RGBA(0xFFFFFF, 1 - p));
			lx = nx; ly = ny;
		}
		spr(SPR_GLOW, x2, y2, 0.5f * s, 0.5f * s, 0, f->col, 1, 1 - p);
		break;
	}
	case FX_TRACER:
		line(x, y, FX(f->x2, -2), FY(f->y2), 3 * s, RGBA(f->col, 0.4f * (1 - p)));
		line(x, y, FX(f->x2, -2), FY(f->y2), 1.2f * s, RGBA(0xFFFFFF, 1 - p));
		spr(SPR_GLOW, FX(f->x2, -2), FY(f->y2), 0.6f * s, 0.6f * s, 0, f->col, 1, 1 - p);
		break;
	case FX_BLAST:
		spr(SPR_GLOW, x, y, f->r * (1.2f + p) * 2 * s / 32, f->r * (1.2f + p) * 2 * s / 32, 0, f->col, 1, 0.85f * (1 - p));
		spr(SPR_GLOW, x, y, f->r * (0.6f + p * 0.5f) * 2 * s / 32, f->r * (0.6f + p * 0.5f) * 2 * s / 32, 0, 0xFFF4C0, 1, 0.9f * (1 - p));
		spr(SPR_RING, x, y, f->r * (0.4f + p * 1.2f) * 2 * s / 32, f->r * (0.4f + p * 1.2f) * 2 * s / 32, 0, 0xFFE0A0, 1, 0.8f * (1 - p));
		break;
	case FX_SPARK: spr(SPR_SPARK, x, y, 0.6f * s, 0.6f * s, p * 3, 0xFFFFFF, 0, 1 - p); break;
	case FX_FREEZE:
		spr(SPR_GLOW, x, y, f->r * (0.5f + p) * 2 * s / 32, f->r * (0.5f + p) * 2 * s / 32, 0, f->col, 1, 0.6f * (1 - p));
		spr(SPR_RING, x, y, f->r * (0.4f + p * 0.7f) * 2 * s / 32, f->r * (0.4f + p * 0.7f) * 2 * s / 32, 0, 0xFFFFFF, 1, 1 - p);
		break;
	case FX_FIRE:
		spr(SPR_RING, x, y, f->r * (0.6f + p * 0.5f) * 2 * s / 32, f->r * (0.6f + p * 0.5f) * 2 * s / 32, 0, f->col, 1, 1 - p);
		spr(SPR_GLOW, x, y, f->r * 2.2f * s / 32, f->r * 2.2f * s / 32, 0, f->col, 1, 0.35f * (1 - p));
		break;
	case FX_SHOCK: spr(SPR_RING, x, y, f->r * p * 2 * s / 32, f->r * p * 2 * s / 32, 0, f->col, 1, 0.9f * (1 - p)); break;
	case FX_COIN: spr(SPR_I_COIN, x, y - p * 16 * s, 0.7f * s, 0.7f * s, 0, 0xFFFFFF, 0, 1 - p * p); break;
	}
}

static void rangeCircle(float x, float y, float r, bool ok) {
	u32 c = ok ? 0xFFFFFF : C_RED;
	spr(SPR_GLOW, FX(x, 0), FY(y), r * 2.1f * FS / 32, r * 2.1f * FS / 32, 0, c, 1, 0.22f);
	ring(FX(x, 0), FY(y), r * FS, 1.4f, RGBA(c, 0.75f), 44);
}

static void drawField(float s, float ox, float oy, bool top) {
	FS = s; FOX = ox; FOY = oy; FTOP = top;
	drawGround();
	nAliveEn = td_enemies_alive(&G);
	if (sel >= 0 && G.tw[sel].alive) {
		TStats st;
		td_stats(&G, &G.tw[sel], &st);
		if (st.range < 900) rangeCircle(G.tw[sel].x, G.tw[sel].y, st.range, true);
	}
	for (int i = 0; i < MAX_PR; i++) if (G.pr[i].alive && G.pr[i].kind == P_PILE) drawProj(&G.pr[i]);
	for (int i = 0; i < MAX_TW; i++) if (G.tw[i].alive) drawTower(&G.tw[i], 1, false);
	for (int i = 0; i < MAX_EN; i++) if (G.en[i].alive && G.en[i].type < E_KOENIG) drawEnemy(&G.en[i]);
	for (int i = 0; i < MAX_EN; i++) if (G.en[i].alive && G.en[i].type >= E_KOENIG) drawEnemy(&G.en[i]);
	for (int i = 0; i < MAX_PR; i++) if (G.pr[i].alive && G.pr[i].kind != P_PILE) drawProj(&G.pr[i]);
	for (int i = 0; i < MAX_FX; i++) if (G.fx[i].alive) drawFx(&G.fx[i], i);
	for (int i = 0; i < MAX_JAR; i++) {
		Jar *j = &G.jar[i];
		if (!j->alive) continue;
		float bob = sinf(T * 4 + i) * 1.5f;
		float x = FX(j->x, -2), y = FY(j->y) + bob * s;
		spr(SPR_GLOW, x, y, 0.8f * s, 0.8f * s, 0, C_GOLD, 1, 0.5f + 0.2f * sinf(T * 6));
		sprN(SPR_I_JAR, x, y, 0.9f * s);
	}
	if (top) drawAmbient();
	if (placeType >= 0) {
		Tower gt = {0};
		gt.type = placeType; gt.x = ghostX; gt.y = ghostY; gt.ang = -1.57f; gt.alive = 1;
		TStats st;
		td_stats(&G, &gt, &st);
		bool ok = td_can_place(&G, placeType, ghostX, ghostY) && G.coins >= td_cost(&G, placeType);
		rangeCircle(ghostX, ghostY, st.range < 900 ? st.range : 20, ok);
		drawTower(&gt, 0.8f, true);
	}
}

// ============================================================ HUD-Bausteine
static void pill(float x, float y, float w, float h) { rrectC(x, y, w, h, h / 2, RGBA(0x0A0E2A, 0.8f), RGBA(0x161C48, 0.8f)); }

static void rankBadge(float x, float y, float sc) {
	spr(SPR_GLOW, x, y, 1.1f * sc, 1.1f * sc, 0, C_CYAN, 1, 0.35f + 0.1f * sinf(T * 2));
	sprN(SPR_I_STAR, x, y, 1.9f * sc);
	textSf(x, y - 7 * sc, 0.45f * sc, C(0xFFFFFF), C2D_AlignCenter, 0, "%d", rank());
}

static void xpBar(float x, float y, float w) {
	int r = rank();
	double a = rank_xp(r), b = rank_xp(r + 1);
	bar(x, y, w, 11, (float)((SD.xp - a) / (b - a)), C_CYAN, 0x2A6AD8);
	textSf(x + w / 2, y - 1, 0.32f, C(C_TEXT), C2D_AlignCenter, 0, "%.0f / %.0f XP", SD.xp - a, b - a);
}

static const char *TGNAME[NTARGET] = {"Erster", "Letzter", "Stärkster", "Nächster"};

static void statsLine(const TStats *s, char *b, int n) {
	switch (s->kind) {
	case K_FARM: snprintf(b, n, "%d Münzen pro Welle", (int)(s->income * (1 + s->jarBonus))); break;
	case K_BUFF: snprintf(b, n, "Tempo +%.0f%%  Reichweite %.0f", s->buffSpeed * 100, s->range); break;
	case K_FREEZE: snprintf(b, n, "Frost %.1f s  alle %.1f s", s->freeze, s->interval); break;
	case K_SLIME: snprintf(b, n, "Bremst %.0f%%  Säure %.0f", s->slow * 100, s->acid); break;
	case K_CHAIN: snprintf(b, n, "Schaden %.0f  Sprünge %d  %.1f/s", s->dmg, s->jumps, 1 / s->interval); break;
	case K_BOMB: snprintf(b, n, "Schaden %.0f  Radius %.0f  %.1f/s", s->dmg, s->blast, 1 / s->interval); break;
	case K_SNIPER: snprintf(b, n, "Schaden %.0f  %.1f/s  ganze Karte", s->dmg, 1 / s->interval); break;
	case K_PILE: snprintf(b, n, "Haufen: %d Stacheln, alle %.1f s", s->pierce, s->interval); break;
	default: snprintf(b, n, "Schaden %.0f  Durchschlag %d  %.1f/s", s->dmg, s->pierce, 1 / s->interval); break;
	}
}

static void towerCard(int type, const Tower *t, float x, float y, float w, float h) {
	x += -2 * eyeSign * slider3d;
	panel(x, y, w, h, 9, 0x10143A, TDEF[type].color, 0.93f);
	spr(SPR_GLOW, x + 22, y + 22, 1.2f, 1.2f, 0, TDEF[type].color, 1, 0.4f);
	sprN(SPR_TW0 + type, x + 22, y + 22, 1.25f);
	textS(x + 42, y + 6, 0.5f, C(C_TEXT), 0, w - 48, TDEF[type].name);
	Tower tmp = {0};
	bool isTmp = !t;
	if (!t) { tmp.type = type; tmp.alive = 1; t = &tmp; }
	TStats s;
	td_stats(&G, t, &s);
	char b[96];
	statsLine(&s, b, sizeof b);
	textS(x + 42, y + 24, 0.33f, C(C_CYAN), 0, w - 48, b);
	char fl[64] = "";
	if (s.camo) strcat(fl, "Sieht Geister  ");
	if (s.ap || s.strip || s.stripAura) strcat(fl, "Knackt Panzer");
	if (fl[0]) textS(x + 42, y + 35, 0.32f, C(C_GREEN), 0, w - 48, fl);
	if (isTmp) {
		textWrap(x + 8, y + 50, 0.36f, C(C_DIM), w - 16, TDEF[type].desc);
		sprN(SPR_I_COIN, x + 14, y + h - 12, 0.8f);
		textSf(x + 24, y + h - 20, 0.42f, C(C_GOLD), 0, 0, "%d", td_cost(&G, type));
		return;
	}
	float yy = y + 50;
	for (int p = 0; p < 2; p++) {
		int tier = p ? t->b : t->a;
		textS(x + 8, yy, 0.36f, C(p ? C_GOLD : C_CYAN), 0, 0, TDEF[type].pathName[p]);
		yy += 12;
		for (int k = 0; k < 4; k++) {
			bool have = k < tier;
			bool xpLock = !have && ((k == 2 && G.towerXp[type] < TIER3_XP) || (k == 3 && G.towerXp[type] < TIER4_XP));
			bool pathLock = !have && k >= 2 && (p ? t->a : t->b) > 2;
			u32 c = have ? C_TEXT : (xpLock || pathLock) ? 0x5A6290 : C_DIM;
			if (have) spr(SPR_GLOW, x + 11, yy + 5, 0.25f, 0.25f, 0, p ? C_GOLD : C_CYAN, 1, 1);
			else if (xpLock) sprN(SPR_I_LOCK, x + 11, yy + 5, 0.5f);
			textS(x + 18, yy, 0.32f, C(c), 0, w - 26, k == tier && !xpLock && !pathLock ? TDEF[type].upDesc[p][k] : TDEF[type].upName[p][k]);
			yy += 11;
		}
		yy += 3;
	}
	textSf(x + 8, y + h - 14, 0.32f, C(C_DIM), 0, w - 16, "Plopps: %.0f   Turm-XP: %.0f", t->pops, fminf(G.towerXp[type], 99999));
}

// ============================================================ Oben
static void drawTitleTop(void) {
	C2D_DrawImageAt(TITLEIMG, eyeSign * 3 * slider3d, 0, 0, NULL, 1, 1);
	for (int i = 0; i < 6; i++) {
		float sp = 18 + i * 5;
		float x = fmodf(i * 97 + T * sp, TW + 80) - 40;
		int ty = i % 5;
		float y = 206 - fabsf(sinf(T * 4 + i)) * 6;
		float sq = sinf(T * 8 + i) * 0.08f;
		float r = EDEF[ty].radius;
		shadow(x, 214, r * 2.6f, r, 0.3f);
		spr(SPR_EN0 + ty, x + eyeSign * -1 * slider3d, y - r * 0.3f, 1.15f * (1 + sq), 1.15f * (1 - sq), 0, 0xFFFFFF, 0, 1);
	}
	for (int i = 0; i < 10; i++) {
		float a = fmaxf(0, sinf(T * 2 + i * 1.9f));
		spr(SPR_SPARK, fmodf(i * 71.3f, 400), 20 + fmodf(i * 37.1f, 110), 0.7f, 0.7f, T, 0xFFFFFF, 0, a);
	}
	pill(TW - 106, 6, 100, 22);
	sprN(SPR_I_STAR, TW - 92, 17, 0.95f);
	textSf(TW - 78, 8, 0.46f, C(C_TEXT), 0, 0, "Rang %d", rank());
}

static bool mapCrowned(int m) { return SD.won[m][0] && SD.won[m][1] && SD.won[m][2]; }
static void medal(int d, float x, float y, float sc, bool won) {
	spr(SPR_I_MEDAL0 + d, x, y, sc * (won ? 1 : 0.85f), sc * (won ? 1 : 0.85f), 0, won ? 0xFFFFFF : 0x30344A, won ? 0 : 0.85f, won ? 1 : 0.6f);
}

static void drawMapsTop(void) {
	FS = 1; FOX = 0; FOY = 0; FTOP = true;
	drawGround();
	drawAmbient();
	bool lock = !mapUnlocked(selMap);
	if (lock) rect(0, 0, TW, SH, RGBA(0x000000, 0.55f));
	float dd = -2 * eyeSign * slider3d;
	panel(8 + dd, 8, 250, 54, 10, 0x10143A, C_CYAN, 0.9f);
	textS(18 + dd, 11, 0.62f, C(C_TEXT), 0, 200, MAPS[selMap].name);
	textS(18 + dd, 37, 0.38f, C(C_DIM), 0, 230, MAPS[selMap].desc);
	if (mapCrowned(selMap)) sprN(SPR_I_CROWN, 240 + dd, 22, 1.2f);
	panel(266 + dd, 8, 126, 54, 10, 0x10143A, C_GOLD, 0.9f);
	for (int d = 0; d < NDIFF; d++) {
		float x = 286 + d * 43 + dd;
		bool won = SD.won[selMap][d];
		medal(d, x, 26, 1.25f, won);
		textSf(x, 41, 0.32f, C(won ? C_TEXT : C_DIM), C2D_AlignCenter, 0, "W %d", SD.best[selMap][d]);
	}
	if (lock) {
		sprN(SPR_I_LOCK, 200, 120, 3);
		textSf(200, 150, 0.6f, C(C_TEXT), C2D_AlignCenter, 0, "Ab Rang %d", MAP_RANK[selMap]);
	}
}

static void drawTowersTop(void) {
	C2D_DrawImageAt(MENUIMG, 0, 0, 0, NULL, 1.25f, 1);
	int t = selTowerInfo;
	bool lock = !towerUnlocked(t);
	float dd = -3 * eyeSign * slider3d;
	spr(SPR_GLOW, 80 + dd, 110, 3.5f, 3.5f, 0, TDEF[t].color, 1, 0.45f + 0.1f * sinf(T * 2));
	spr(SPR_TW0 + t, 80 + dd, 110, 3.4f, 3.4f, -0.3f + sinf(T) * 0.15f, lock ? 0x10102A : 0xFFFFFF, lock ? 0.9f : 0, 1);
	if (lock) { sprN(SPR_I_LOCK, 80 + dd, 110, 2.5f); textSf(80 + dd, 168, 0.55f, C(C_TEXT), C2D_AlignCenter, 0, "Ab Rang %d", TOWER_RANK[t]); }
	panel(156, 10, 236, 220, 10, 0x10143A, TDEF[t].color, 0.92f);
	textS(166, 14, 0.6f, C(C_TEXT), 0, 216, TDEF[t].name);
	textWrap(166, 38, 0.36f, C(C_DIM), 218, TDEF[t].desc);
	float xpv = SD.towerXp[t];
	textSf(166, 68, 0.36f, C(C_CYAN), 0, 0, "Turm-XP: %.0f", xpv);
	bar(166, 82, 216, 8, xpv / TIER4_XP, C_CYAN, 0x2A6AD8);
	float m3 = 166 + 216.0f * TIER3_XP / TIER4_XP;
	rect(m3, 80, 1.5f, 12, C(C_GOLD));
	textS(m3, 92, 0.28f, C(C_GOLD), C2D_AlignCenter, 0, "Stufe 3");
	textS(380, 92, 0.28f, C(C_GOLD), C2D_AlignRight, 0, "Stufe 4");
	float yy = 102;
	for (int p = 0; p < 2; p++) {
		textS(166, yy, 0.4f, C(p ? C_GOLD : C_CYAN), 0, 0, TDEF[t].pathName[p]);
		yy += 13;
		for (int k = 0; k < 4; k++) {
			bool xpLock = (k == 2 && xpv < TIER3_XP) || (k == 3 && xpv < TIER4_XP);
			if (xpLock) sprN(SPR_I_LOCK, 172, yy + 6, 0.55f);
			else spr(SPR_GLOW, 172, yy + 6, 0.25f, 0.25f, 0, p ? C_GOLD : C_CYAN, 1, 1);
			textS(182, yy, 0.34f, C(xpLock ? 0x6A72A0 : C_TEXT), 0, 110, TDEF[t].upName[p][k]);
			textS(386, yy + 1, 0.29f, C(xpLock ? 0x5A6290 : C_DIM), C2D_AlignRight, 92, TDEF[t].upDesc[p][k]);
			yy += 11.5f;
		}
		yy += 2;
	}
}

static void drawGameTop(void) {
	float sx = shake > 0 ? (frnd() - 0.5f) * shake * 6 : 0, sy = shake > 0 ? (frnd() - 0.5f) * shake * 6 : 0;
	drawField(1, sx, sy, true);
	float dd = -2 * eyeSign * slider3d;
	pill(4 + dd, 4, 164, 22);
	sprN(SPR_I_COIN, 18 + dd, 15, 1.0f);
	textSf(29 + dd, 5, 0.52f, C(C_GOLD), 0, 70, "%.0f", floor(G.coins));
	sprN(SPR_I_HEART, 110 + dd, 15, 1.0f);
	textSf(121 + dd, 5, 0.52f, C(G.lives < 20 ? C_RED : C_TEXT), 0, 45, "%d", G.lives);
	pill(172 + dd, 4, 96, 22);
	if (G.endless) textSf(220 + dd, 6, 0.46f, C(C_TEXT), C2D_AlignCenter, 0, "Welle %d", G.wave);
	else textSf(220 + dd, 6, 0.46f, C(C_TEXT), C2D_AlignCenter, 0, "Welle %d/%d", G.wave, DIFF[G.diff].finalWave);
	if (overlay == OV_PAUSE) { pill(TW - 46 + dd, 4, 42, 22); sprN(SPR_I_PAUSE, TW - 25 + dd, 15, 0.8f); }
	else if (ff) { pill(TW - 46 + dd, 4, 42, 22); spr(SPR_I_FF, TW - 25 + dd, 15, 0.9f, 0.9f, 0, C_CYAN, 1, 1); }
	int boss = -1;
	for (int i = 0; i < MAX_EN; i++) if (G.en[i].alive && G.en[i].type >= E_KOENIG && (boss < 0 || G.en[i].type > G.en[boss].type)) boss = i;
	if (boss >= 0) {
		Enemy *e = &G.en[boss];
		float p = e->hp / (EDEF[e->type].hp * (G.cur.hpMul > 0 ? G.cur.hpMul : 1));
		pill(96 + dd, 29, 208, 18);
		sprN(SPR_I_SKULL, 108 + dd, 38, 0.75f);
		bar(120 + dd, 33, 176, 10, p, C_RED, 0x8A1A2A);
		textS(208 + dd, 31, 0.3f, C(C_TEXT), C2D_AlignCenter, 0, EDEF[e->type].name);
	}
	if (sel >= 0 && G.tw[sel].alive) towerCard(G.tw[sel].type, &G.tw[sel], 228, 30, 166, 184);
	else if (placeType >= 0) towerCard(placeType, NULL, 228, 30, 166, 130);
	if (bannerT > 0) {
		float a = fminf(1, fminf(bannerT * 3, (2.8f - bannerT) * 5));
		float sc = 0.8f + 0.2f * fminf(1, (2.8f - bannerT) * 6);
		float w = 290 * sc, h = 56 * sc;
		float x = 200 - w / 2 + dd * 1.5f, y = 96 - h / 2 + 10;
		rrectC(x, y, w, h, 12, RGBA(darken(bannerCol, 0.6f), 0.9f * a), RGBA(darken(bannerCol, 0.8f), 0.9f * a));
		rrectC(x + 6, y + 3, w - 12, 3, 1.5f, RGBA(bannerCol, a), RGBA(bannerCol, a));
		textS(200 + dd * 1.5f, y + 6 * sc, 0.75f * sc, RGBA(bannerCol, a), C2D_AlignCenter, w - 20, bannerText);
		textS(200 + dd * 1.5f, y + 33 * sc, 0.4f * sc, RGBA(C_TEXT, a), C2D_AlignCenter, w - 20, bannerSub);
	}
	if (overlay == OV_WIN || overlay == OV_LOSE) {
		rect(0, 0, TW, SH, RGBA(0x000000, 0.45f));
		float sc = 0.6f + 0.4f * fminf(1, overT * 3);
		if (overlay == OV_WIN) {
			for (int i = 0; i < 14; i++) {
				float a = i * PI / 7 + T * 0.4f;
				spr(SPR_SPARK, 200 + cosf(a) * 120, 80 + sinf(a) * 50, 1, 1, T * 2 + i, i % 2 ? C_GOLD : 0xFFFFFF, 1, 0.8f);
			}
			sprN(SPR_I_TROPHY, 200, 56, 3 * sc);
		} else sprN(SPR_I_SKULL, 200, 56, 2.8f * sc);
		textS(200 + dd * 2, 96, 1.1f * sc, C(overlay == OV_WIN ? C_GOLD : C_RED), C2D_AlignCenter, 0, overlay == OV_WIN ? "Gewonnen!" : "Durchgebrochen!");
		textSf(200, 140, 0.5f, C(C_TEXT), C2D_AlignCenter, 0, "Welle %d  ·  %s  ·  %s", G.wave, MAPS[G.map].name, DIFF[G.diff].name);
	}
}

// ============================================================ Unten
#define SLOT_W 46.0f
#define STRIP_W 262.0f
static int stripList[NTOWER], nStrip;
static void buildStrip(void) {
	nStrip = 0;
	for (int r = 1; r <= 99 && nStrip < NTOWER; r++)
		for (int t = 0; t < NTOWER; t++)
			if (TOWER_RANK[t] == r && towerUnlocked(t)) stripList[nStrip++] = t;
}

static void drawStrip(void) {
	rectG(0, 192, BW, 48, C(0x2A3A8A), C(0x141C4A));
	rect(0, 192, BW, 1.5f, C(0x6A80D0));
	buildStrip();
	float maxS = nStrip * SLOT_W - STRIP_W + 4;
	if (maxS < 0) maxS = 0;
	if (stripScroll < 0) stripScroll = 0;
	if (stripScroll > maxS) stripScroll = maxS;
	for (int k = 0; k < nStrip; k++) {
		int ty = stripList[k];
		float x = 3 + k * SLOT_W - stripScroll;
		if (x + SLOT_W < 0 || x > STRIP_W) continue;
		int c = td_cost(&G, ty);
		bool can = G.coins >= c;
		bool on = placeType == ty;
		panel(x, 195, SLOT_W - 4, 43, 7, on ? 0x3A4AA0 : 0x1E2660, on ? C_CYAN : can ? 0x5A70C0 : 0x2E3870, 1);
		if (can) spr(SPR_GLOW, x + 21, 210, 0.9f, 0.9f, 0, TDEF[ty].color, 1, 0.3f);
		spr(SPR_TW0 + ty, x + 21, 210, 1.05f, 1.05f, -0.4f, can ? 0xFFFFFF : 0x202040, can ? 0 : 0.5f, 1);
		sprN(SPR_I_COIN, x + 8, 230, 0.55f);
		textSf(x + 25, 224.5f, 0.34f, C(can ? C_GOLD : 0x8A8AAA), C2D_AlignCenter, SLOT_W - 18, "%d", c);
		addHit(fmaxf(0, x), 193, SLOT_W, 47, A_SLOT, ty);
	}
	rectG(STRIP_W, 192, BW - STRIP_W, 48, C(0x2A3A8A), C(0x141C4A));
	bool can = !G.waveActive && !G.lost && overlay == OV_NONE;
	float bx = STRIP_W + 4, by = 196, bw = BW - STRIP_W - 8, bh = 40;
	if (!G.waveActive) button(bx, by, bw, bh, C_GREEN, can, 0.4f, "", SPR_I_PLAY);
	else button(bx, by, bw, bh, ff ? C_CYAN : 0x3A4A90, true, 0.4f, "", SPR_I_FF);
	addHit(STRIP_W, 192, BW - STRIP_W, 48, A_PLAY, 0);
}

static void drawUpgradePanel(void) {
	if (sel < 0 || !G.tw[sel].alive) return;
	Tower *t = &G.tw[sel];
	float y0 = 240 - 114 * panelAnim;
	rectG(0, y0, BW, 114, RGBA(0x1A2058, 0.97f), RGBA(0x0C1030, 0.97f));
	rect(0, y0, BW, 2, C(TDEF[t->type].color));
	addHit(0, y0, BW, 114, A_NONE, 0);
	sprN(SPR_TW0 + t->type, 16, y0 + 14, 0.8f);
	textS(30, y0 + 4, 0.48f, C(C_TEXT), 0, 170, TDEF[t->type].name);
	button(BW - 32, y0 + 4, 26, 21, 0x6A3A7A, true, 0.45f, "X", -1);
	addHit(BW - 40, y0, 40, 28, A_CLOSE, 0);
	for (int p = 0; p < 2; p++) {
		float x = 6 + p * 157, y = y0 + 27, w = 151;
		int tier = p ? t->b : t->a;
		u32 pc = p ? C_GOLD : C_CYAN;
		panel(x, y, w, 54, 8, 0x141A48, darken(pc, 0.3f), 1);
		textS(x + 6, y + 2, 0.34f, C(pc), 0, 70, TDEF[t->type].pathName[p]);
		for (int k = 0; k < 4; k++) rrectC(x + w - 46 + k * 10.5f, y + 5, 8.5f, 6, 2, C(k < tier ? lighten(pc, 0.2f) : 0x2A3166), C(k < tier ? pc : 0x232A58));
		int lk = td_upgrade_lock(&G, sel, p);
		if (lk == UL_MAX) {
			sprN(SPR_I_CROWN, x + w / 2 - 22, y + 32, 1.2f);
			textS(x + w / 2 + 8, y + 22, 0.5f, C(C_GOLD), C2D_AlignCenter, 0, "MAX");
			continue;
		}
		textS(x + 6, y + 15, 0.4f, C(lk == UL_OK ? C_TEXT : 0x7A82B0), 0, w - 12, TDEF[t->type].upName[p][tier]);
		if (lk == UL_PATH) { textS(x + 6, y + 33, 0.33f, C(0x8A90B8), 0, w - 12, "Gesperrt: anderer Pfad"); continue; }
		if (lk == UL_XP) {
			float need = tier + 1 == 3 ? TIER3_XP : TIER4_XP;
			sprN(SPR_I_LOCK, x + 12, y + 40, 0.7f);
			bar(x + 22, y + 35, w - 30, 10, G.towerXp[t->type] / need, C_CYAN, 0x2A6AD8);
			textSf(x + 22 + (w - 30) / 2, y + 34, 0.28f, C(C_TEXT), C2D_AlignCenter, 0, "Turm-XP %.0f/%.0f", G.towerXp[t->type], need);
			continue;
		}
		int c = td_up_cost(&G, sel, p);
		char b[24];
		snprintf(b, sizeof b, "%d", c);
		button(x + 6, y + 31, w - 12, 20, pc, G.coins >= c, 0.4f, b, SPR_I_COIN);
		addHit(x, y, w, 54, A_UPG, p);
	}
	char sb[32];
	snprintf(sb, sizeof sb, "Verkaufen %d", td_sell_value(&G, sel));
	button(6, y0 + 86, 120, 23, C_RED, true, 0.4f, sb, -1);
	addHit(6, y0 + 84, 120, 28, A_SELL, 0);
	int k = TDEF[t->type].kind;
	if (k != K_FARM && k != K_BUFF && k != K_FREEZE && k != K_PILE) {
		char tb[32];
		snprintf(tb, sizeof tb, "Ziel: %s", TGNAME[t->target]);
		button(132, y0 + 86, 128, 23, 0x4A5AB0, true, 0.4f, tb, -1);
		addHit(132, y0 + 84, 128, 28, A_TARGET, 0);
	}
	textSf(312, y0 + 90, 0.32f, C(C_DIM), C2D_AlignRight, 0, "%.0f", t->pops);
}

static void drawOverlayBottom(void) {
	if (overlay == OV_NONE) return;
	nHits = 0;
	rect(0, 0, BW, SH, RGBA(0x04060F, 0.7f));
	const char *title = "";
	const char *opts[5] = {0};
	int icons[5] = {-1, -1, -1, -1, -1};
	int n = 0;
	u32 col = C_CYAN;
	switch (overlay) {
	case OV_PAUSE: title = "Pause"; opts[0] = "Weiter"; icons[0] = SPR_I_PLAY; opts[1] = SD.sound ? "Ton: an" : "Ton: aus"; opts[2] = SD.stereo ? "3D: an" : "3D: aus";
		opts[3] = SD.autoStart ? "Auto-Start: an" : "Auto-Start: aus"; opts[4] = "Hauptmenü"; n = 5; icons[1] = icons[2] = icons[3] = SPR_I_GEAR; break;
	case OV_WIN: title = "Gewonnen!"; opts[0] = "Endlos weiterspielen"; opts[1] = "Hauptmenü"; n = 2; col = C_GOLD; icons[0] = SPR_I_PLAY; break;
	case OV_LOSE: title = "Verloren"; opts[0] = "Nochmal"; opts[1] = "Hauptmenü"; n = 2; col = C_RED; icons[0] = SPR_I_PLAY; break;
	case OV_CONFIRM_MENU: title = "Zum Hauptmenü?"; opts[0] = "Ja (XP bleiben erhalten)"; opts[1] = "Abbrechen"; n = 2; break;
	}
	bool endScreen = overlay == OV_WIN || overlay == OV_LOSE;
	float h = 40 + n * 34 + (endScreen ? 50 : 0);
	float y = (SH - h) / 2;
	panel(30, y, 260, h, 12, 0x161C50, col, 1);
	textS(160, y + 6, 0.62f, C(col), C2D_AlignCenter, 0, title);
	float oy = y + 36;
	if (endScreen) {
		double shown = endXp * fmin(1, overT / 1.4);
		sprN(SPR_I_STAR, 52, oy + 10, 1.1f);
		textSf(66, oy + 2, 0.45f, C(C_CYAN), 0, 0, "+%.0f XP", shown);
		if (overlay == OV_WIN) sprN(SPR_I_MEDAL0 + G.diff, 262, oy + 10, 1.4f);
		int r = rank();
		double a = rank_xp(r), b = rank_xp(r + 1);
		bar(50, oy + 24, 220, 11, (float)((SD.xp - a) / (b - a)), C_CYAN, 0x2A6AD8);
		textSf(160, oy + 23, 0.3f, C(C_TEXT), C2D_AlignCenter, 0, "Rang %d", r);
		oy += 50;
	}
	for (int i = 0; i < n; i++) {
		button(46, oy + i * 34, 228, 28, i == 0 ? col : 0x3A4A90, true, 0.44f, opts[i], icons[i]);
		addHit(40, oy - 2 + i * 34, 240, 32, A_OV, i);
	}
}

static void drawGameBottom(void) {
	nHits = 0;
	drawField(BS, 0, 0, false);
	addHit(0, 0, BW, 192, A_FIELD, 0);
	pill(3, 3, 128, 19);
	sprN(SPR_I_COIN, 13, 12.5f, 0.8f);
	textSf(22, 4, 0.42f, C(C_GOLD), 0, 52, "%.0f", floor(G.coins));
	sprN(SPR_I_HEART, 84, 12.5f, 0.8f);
	textSf(93, 4, 0.42f, C(G.lives < 20 ? C_RED : C_TEXT), 0, 34, "%d", G.lives);
	pill(BW - 31, 3, 28, 19);
	sprN(SPR_I_PAUSE, BW - 17, 12.5f, 0.7f);
	addHit(BW - 42, 0, 42, 30, A_PAUSE, 0);
	if (!G.waveActive && G.wave == 0 && placeType < 0 && sel < 0)
		textS(160, 160, 0.42f, C(C_TEXT), C2D_AlignCenter, 0, "Ziehe einen Turm aufs Feld, dann Start");
	drawStrip();
	if (panelAnim > 0.01f) drawUpgradePanel();
	if (placeType >= 0 && !placeDrag) textS(160, 172, 0.38f, C(C_CYAN), C2D_AlignCenter, 0, "Tippe aufs Feld zum Platzieren");
	drawOverlayBottom();
}

static void menuBg(void) {
	C2D_DrawImageAt(MENUIMG, 0, 0, 0, NULL, 1, 1);
	for (int i = 0; i < 8; i++) {
		float x = fmodf(i * 53 + T * (6 + i), BW + 40) - 20, y = 30 + fmodf(i * 67.3f, 180) + sinf(T + i) * 6;
		spr(SPR_EN0 + (i % 5), x, y, 0.9f, 0.9f, 0, 0xFFFFFF, 0, 0.2f);
	}
}

static void drawTitleBottom(void) {
	nHits = 0;
	menuBg();
	panel(8, 6, 304, 42, 12, 0x161C50, C_CYAN, 0.94f);
	rankBadge(30, 27, 1);
	textSf(52, 8, 0.44f, C(C_TEXT), 0, 0, "Rang %d", rank());
	xpBar(52, 29, 250);
	int nextT = -1;
	for (int t = 0; t < NTOWER; t++) if (!towerUnlocked(t) && (nextT < 0 || TOWER_RANK[t] < TOWER_RANK[nextT])) nextT = t;
	if (nextT >= 0) {
		sprN(SPR_TW0 + nextT, 290, 16, 0.45f);
		textSf(280, 9, 0.3f, C(C_DIM), C2D_AlignRight, 170, "Rang %d: %s", TOWER_RANK[nextT], TDEF[nextT].name);
	}
	button(30, 56, 260, 44, C_GREEN, true, 0.62f, "Spielen", SPR_I_PLAY);
	addHit(30, 54, 260, 48, A_MENU, 0);
	char b[64];
	if (SD.hasResume && SD.map < NMAP) {
		snprintf(b, sizeof b, "Weiter: %s, Welle %d", MAPS[SD.map].name, (int)SD.wave + 1);
		button(30, 108, 260, 34, C_GOLD, true, 0.44f, b, SPR_I_FF);
		addHit(30, 106, 260, 38, A_MENU, 1);
	} else button(30, 108, 260, 34, C_GOLD, false, 0.44f, "Kein Spielstand zum Fortsetzen", -1);
	button(30, 152, 82, 36, 0x7A5AE0, true, 0.4f, "Türme", SPR_I_TROPHY);
	addHit(28, 150, 86, 40, A_MENU, 4);
	button(119, 152, 82, 36, 0x3A8AD0, true, 0.4f, "Hilfe", SPR_I_BOOK);
	addHit(117, 150, 86, 40, A_MENU, 2);
	button(208, 152, 82, 36, 0x4A5AA0, true, 0.4f, SD.sound ? "Ton an" : "Ton aus", SPR_I_GEAR);
	addHit(206, 150, 86, 40, A_MENU, 3);
	textS(160, 222, 0.32f, C(0x9AA4D8), C2D_AlignCenter, 0, "3DS Sammlung  ·  START: Beenden");
}

static void drawMapsBottom(void) {
	nHits = 0;
	menuBg();
	textS(160, 1, 0.46f, C(C_TEXT), C2D_AlignCenter, 0, "Karte wählen");
	for (int m = 0; m < NMAP; m++) {
		float x = 6 + (m % 2) * 156, y = 20 + (m / 2) * 60;
		bool on = m == selMap, lock = !mapUnlocked(m);
		panel(x, y, 152, 57, 8, on ? 0x2A3A8A : 0x161C50, on ? C_CYAN : mapCrowned(m) ? C_GOLD : 0x3A4A90, 1);
		C2D_ImageTint tn;
		C2D_PlainImageTint(&tn, lock ? C(0x101020) : C(0xFFFFFF), lock ? 0.7f : 0);
		C2D_DrawImageAt(BG[m], x + 4, y + 4, 0, &tn, 82.0f / 400, 49.0f / 240);
		if (lock) sprN(SPR_I_LOCK, x + 45, y + 28, 1.1f);
		textS(x + 90, y + 5, 0.36f, C(lock ? C_DIM : C_TEXT), 0, 58, MAPS[m].name);
		if (lock) textSf(x + 90, y + 24, 0.32f, C(C_DIM), 0, 58, "Rang %d", MAP_RANK[m]);
		else for (int d = 0; d < NDIFF; d++) medal(d, x + 98 + d * 14, y + 38, 0.8f, SD.won[m][d]);
		if (mapCrowned(m)) sprN(SPR_I_CROWN, x + 140, y + 10, 0.8f);
		addHit(x, y, 152, 57, A_MAP, m);
	}
	button(6, 204, 46, 32, 0x4A5AA0, true, 0.5f, "<", -1);
	addHit(0, 200, 56, 40, A_BACK, 0);
	static const u32 dc[3] = {C_GREEN, C_GOLD, C_RED};
	bool lock = !mapUnlocked(selMap);
	for (int d = 0; d < NDIFF; d++) {
		button(58 + d * 87, 204, 83, 32, dc[d], !lock, 0.42f, DIFF[d].name, SD.won[selMap][d] ? SPR_I_MEDAL0 + d : -1);
		if (!lock) addHit(56 + d * 87, 200, 87, 40, A_DIFF, d);
	}
}

static void drawTowersBottom(void) {
	nHits = 0;
	menuBg();
	textS(160, 1, 0.46f, C(C_TEXT), C2D_AlignCenter, 0, "Türme & Turm-XP");
	for (int t = 0; t < NTOWER; t++) {
		float x = 6 + (t % 5) * 62, y = 20 + (t / 5) * 60;
		bool lock = !towerUnlocked(t), on = t == selTowerInfo;
		panel(x, y, 58, 56, 8, on ? 0x2A3A8A : 0x161C50, on ? C_CYAN : 0x3A4A90, 1);
		spr(SPR_TW0 + t, x + 29, y + 22, 1.2f, 1.2f, -0.4f, lock ? 0x101024 : 0xFFFFFF, lock ? 0.85f : 0, 1);
		if (lock) { sprN(SPR_I_LOCK, x + 29, y + 22, 0.9f); textSf(x + 29, y + 38, 0.3f, C(C_DIM), C2D_AlignCenter, 0, "Rang %d", TOWER_RANK[t]); }
		else {
			bar(x + 5, y + 44, 48, 7, SD.towerXp[t] / TIER4_XP, C_CYAN, 0x2A6AD8);
			if (SD.towerXp[t] >= TIER4_XP) sprN(SPR_I_CROWN, x + 48, y + 8, 0.6f);
		}
		addHit(x, y, 58, 56, A_TINFO, t);
	}
	button(6, 204, 90, 32, 0x4A5AA0, true, 0.44f, "< Zurück", -1);
	addHit(0, 200, 100, 40, A_BACK, 0);
}

static const char *HELP =
	"ZIEL\n"
	"Halte den Goo auf, bevor er deine Basis\n"
	"erreicht. Große Goos kosten viele Leben!\n\n"
	"TÜRME\n"
	"Ziehe einen Turm aus der Leiste aufs\n"
	"Feld oder tippe ihn an und dann aufs\n"
	"Feld. Tippe einen Turm an für Upgrades.\n"
	"Jeder Turm hat 2 Pfade: einer bis Stufe\n"
	"4, der andere nur bis Stufe 2.\n\n"
	"RANG & XP\n"
	"Jedes Spiel bringt XP. Neue Ränge\n"
	"schalten Türme und Karten frei.\n\n"
	"TURM-XP\n"
	"Jeder Plopp eines Turms gibt diesem Turm-\n"
	"typ XP. Ab 1500 XP gibt es Stufe 3, ab\n"
	"6000 XP Stufe 4. Farmen lohnt sich!\n\n"
	"MEDAILLEN & KRONEN\n"
	"Bronze, Silber, Gold für Leicht, Normal,\n"
	"Schwer. Alle drei auf einer Karte: Krone!\n\n"
	"GOO-ARTEN\n"
	"Geister-Goo (durchsichtig) sehen nur\n"
	"Türme mit Sicht (Eule, Fuchs, Dachs ...).\n"
	"Helm-Goo braucht Panzerknacker.\n"
	"Rosa Ring: wächst nach!\n\n"
	"TASTEN\n"
	"A: Welle starten / Vorspulen\n"
	"B: Auswahl aufheben  L/R: Leiste\n"
	"START: Pause";

static void drawHelpBottom(void) {
	nHits = 0;
	menuBg();
	int lines = 1;
	for (const char *c = HELP; *c; c++) lines += *c == '\n';
	float maxS = lines * 0.4f * FONT_H - 190;
	if (helpScroll < 0) helpScroll = 0;
	if (helpScroll > maxS) helpScroll = maxS;
	textS(10, 36 - helpScroll, 0.4f, C(C_TEXT), 0, 0, HELP);
	rectG(0, 0, BW, 32, C(0x1A2058), RGBA(0x1A2058, 0.9f));
	button(6, 4, 80, 24, 0x4A5AA0, true, 0.4f, "< Zurück", -1);
	addHit(0, 0, 90, 32, A_BACK, 0);
	textS(200, 6, 0.5f, C(C_TEXT), C2D_AlignCenter, 0, "Hilfe");
}

// ============================================================ Spielablauf
static void commitXp(void) {
	int before = rank();
	double d = (G.xpGained - xpCommitted) * (1 + 0.25 * G.diff);
	xpCommitted = G.xpGained;
	SD.xp += d;
	for (int t = 0; t < NTOWER; t++) if (G.towerXp[t] < 1e8f) SD.towerXp[t] = G.towerXp[t];
	int after = rank();
	if (after > before) {
		char b[80] = "";
		for (int t = 0; t < NTOWER; t++)
			if (TOWER_RANK[t] > before && TOWER_RANK[t] <= after) snprintf(b, sizeof b, "Neuer Turm: %s!", TDEF[t].name);
		for (int m = 0; m < NMAP; m++)
			if (MAP_RANK[m] > before && MAP_RANK[m] <= after && !b[0]) snprintf(b, sizeof b, "Neue Karte: %s!", MAPS[m].name);
		char a[32];
		snprintf(a, sizeof a, "RANG %d!", after);
		banner(C_CYAN, a, "%s", b[0] ? b : "Weiter so!");
		play(S_WIN, 1.2f, 0.6f);
	}
}

static void startGame(int map, int diff) {
	td_new(&G, map, diff);
	for (int t = 0; t < NTOWER; t++) G.towerXp[t] = SD.towerXp[t];
	xpCommitted = 0;
	sel = -1; placeType = -1; ff = 0; overlay = OV_NONE; endShown = false;
	state = ST_GAME;
	stripScroll = 0;
	snapshot();
	commitSnapshot();
	SD.games += 1;
	saveWrite();
	banner(C_GOLD, MAPS[map].name, "Platziere Türme und starte die erste Welle!");
#ifdef DESKTOP_TEST
	if (getenv("TD_COINS")) G.coins = atof(getenv("TD_COINS"));
	if (getenv("TD_WAVE")) G.wave = atoi(getenv("TD_WAVE"));
#endif
}

static void recordBest(void) {
	if (G.wave > SD.best[G.map][G.diff]) SD.best[G.map][G.diff] = G.wave > 255 ? 255 : G.wave;
}

static void startWave(void) {
	if (td_start_wave(&G)) {
		play(S_WAVE, 1, 0.5f);
		char a[32], b[80] = "";
		snprintf(a, sizeof a, "Welle %d", G.wave);
		for (int i = 0; i < G.cur.ng; i++)
			if (G.cur.g[i].type >= E_KOENIG) snprintf(b, sizeof b, "Achtung: %s!", EDEF[G.cur.g[i].type].name);
		if (!b[0]) {
			if (G.wave == 20) snprintf(b, sizeof b, "Neu: Geister-Goo! Du brauchst Sicht.");
			else if (G.wave == 26) snprintf(b, sizeof b, "Neu: Helm-Goo! Knacke den Panzer.");
			else if (G.wave == 33) snprintf(b, sizeof b, "Neu: Nachwachsender Goo!");
		}
		if (b[0] || G.wave % 10 == 0 || G.wave == 1) banner(b[0] && strstr(b, "Achtung") ? C_RED : C_GOLD, a, "%s", b[0] ? b : "Los geht's!");
	}
}

static float popCd, shootCd;
static void gameEvents(float dt) {
	popCd -= dt;
	shootCd -= dt;
	if (G.evPop && popCd <= 0) { play(S_POP, 0.85f + frnd() * 0.4f, fminf(0.5f, 0.2f + G.evPop * 0.05f)); popCd = 0.05f; }
	if (G.evBossPop) { play(S_BOOM, 0.7f, 0.7f); shake = 0.6f; }
	if (G.evBoom) play(S_BOOM, 1 + frnd() * 0.2f, 0.4f);
	if (G.evZap) play(S_ZAP, 1 + frnd() * 0.2f, 0.35f);
	if (G.evFreeze) play(S_FREEZE, 1, 0.35f);
	if (G.evShoot && !G.evPop && shootCd <= 0) { play(S_SHOOT, 0.9f + frnd() * 0.3f, 0.18f); shootCd = 0.08f; }
	if (G.evCoin) play(S_COIN, 1, 0.45f);
	if (G.evLeak) { play(S_LEAK, 1, 0.5f); shake = fmaxf(shake, 0.3f); }
	if (G.evUnlock) {
		int ty = G.evUnlock - 1;
		banner(C_CYAN, "Stufe freigeschaltet!", "%s: Stufe %d ist jetzt verfügbar", TDEF[ty].name, G.unlockTier);
		play(S_UPGRADE, 1.2f, 0.6f);
		G.evUnlock = 0;
	}
	if (G.evWaveEnd) {
		recordBest();
		commitXp();
		if (G.won && !endShown) {
			endShown = true;
			overlay = OV_WIN;
			overT = 0;
			endXp = G.xpGained * (1 + 0.25 * G.diff);
			SD.won[G.map][G.diff] = 1;
			SD.hasResume = 0;
			play(S_WIN, 1, 0.7f);
			saveWrite();
		} else {
			if (G.wave % 5 == 0 && bannerT <= 0) banner(C_GREEN, "Welle geschafft!", "+%d Münzen Bonus", G.waveEndBonus);
			snapshot();
			commitSnapshot();
			saveWrite();
			autoT = 1.5f;
		}
	}
	if (G.lost && overlay != OV_LOSE) {
		commitXp();
		overlay = OV_LOSE;
		overT = 0;
		endXp = G.xpGained * (1 + 0.25 * G.diff);
		recordBest();
		SD.hasResume = 0;
		saveWrite();
		play(S_LOSE, 1, 0.7f);
	}
	G.evPop = G.evShoot = G.evBoom = G.evZap = G.evFreeze = G.evCoin = G.evLeak = G.evBossPop = G.evWaveEnd = G.evBlock = 0;
}

// ============================================================ Eingabe
static bool tDown, tDrag;
static float tx0, ty0, tx, ty, tLastX;
static int tSlot = -1;

static void ovAction(int i) {
	play(S_CLICK, 1, 0.4f);
	switch (overlay) {
	case OV_PAUSE:
		if (i == 0) overlay = OV_NONE;
		if (i == 1) SD.sound = !SD.sound;
		if (i == 2) { SD.stereo = !SD.stereo; gfxSet3D(SD.stereo); }
		if (i == 3) SD.autoStart = !SD.autoStart;
		if (i == 4) overlay = OV_CONFIRM_MENU;
		saveWrite();
		break;
	case OV_CONFIRM_MENU:
		if (i == 0) { commitXp(); saveWrite(); state = ST_TITLE; overlay = OV_NONE; }
		else overlay = OV_PAUSE;
		break;
	case OV_WIN:
		if (i == 0) { G.endless = true; G.won = false; overlay = OV_NONE; snapshot(); commitSnapshot(); saveWrite(); }
		else { state = ST_TITLE; overlay = OV_NONE; }
		break;
	case OV_LOSE:
		if (i == 0) startGame(G.map, G.diff);
		else { state = ST_TITLE; overlay = OV_NONE; }
		break;
	}
}

static void tryPlace(float fx, float fy) {
	if (placeType < 0) return;
	int i = td_place(&G, placeType, fx, fy);
	if (i >= 0) {
		play(S_PLACE, 1, 0.6f);
		sel = -1;
		placeType = -1;
	} else {
		play(S_ERR, 1, 0.4f);
		if (placeDrag) placeType = -1;
	}
}

static void act(Hit *h, float x, float y) {
	pressX = x; pressY = y; pressT = 0.15f;
	switch (h->act) {
	case A_PLAY:
		if (!G.waveActive) startWave();
		else { ff = !ff; play(S_CLICK, ff ? 1.3f : 1, 0.4f); }
		break;
	case A_PAUSE: overlay = OV_PAUSE; overT = 0; play(S_CLICK, 1, 0.4f); break;
	case A_UPG:
		if (sel >= 0 && td_upgrade(&G, sel, h->p)) play(S_UPGRADE, 1, 0.55f);
		else play(S_ERR, 1, 0.4f);
		break;
	case A_SELL:
		if (sel >= 0) { td_sell(&G, sel); play(S_COIN, 0.8f, 0.5f); sel = -1; }
		break;
	case A_TARGET:
		if (sel >= 0) { G.tw[sel].target = (G.tw[sel].target + 1) % NTARGET; play(S_CLICK, 1.1f, 0.4f); }
		break;
	case A_CLOSE: sel = -1; play(S_CLICK, 0.9f, 0.3f); break;
	case A_FIELD: {
		float fx = x / BS, fy = y / BS;
		if (td_collect_jar(&G, fx, fy, 16) > 0) { play(S_COIN, 1.1f, 0.5f); break; }
		if (placeType >= 0) { ghostX = fx; ghostY = fy; tryPlace(fx, fy); break; }
		int t = td_tower_at(&G, fx, fy);
		if (t >= 0) { sel = t; play(S_CLICK, 1.2f, 0.4f); }
		else sel = -1;
		break;
	}
	case A_OV: ovAction(h->p); break;
	case A_MENU:
		play(S_CLICK, 1, 0.4f);
		if (h->p == 0) {
			state = ST_MAPS;
			if (!mapUnlocked(selMap)) selMap = 0;
			td_new(&G, selMap, 1);
		}
		if (h->p == 1 && SD.hasResume && SD.map < NMAP) {
			restoreResume();
			for (int t = 0; t < NTOWER; t++) G.towerXp[t] = SD.towerXp[t];
			xpCommitted = 0;
			snapshot();
			sel = -1; placeType = -1; ff = 0; overlay = OV_NONE; endShown = false;
			state = ST_GAME;
			banner(C_GOLD, "Willkommen zurück!", "Weiter geht's mit der nächsten Welle.");
		}
		if (h->p == 2) { state = ST_HELP; helpScroll = 0; }
		if (h->p == 3) { SD.sound = !SD.sound; saveWrite(); }
		if (h->p == 4) state = ST_TOWERS;
		break;
	case A_MAP: selMap = h->p; td_new(&G, selMap, 1); play(S_CLICK, 1.1f, 0.4f); break;
	case A_DIFF: play(S_WAVE, 1, 0.5f); startGame(selMap, h->p); break;
	case A_BACK: state = ST_TITLE; play(S_CLICK, 0.9f, 0.4f); break;
	case A_TINFO: selTowerInfo = h->p; play(S_CLICK, 1.1f, 0.4f); break;
	}
}

static void handleInput(u32 kDown, u32 kHeld, u32 kUp, float dt) {
	touchPosition tp;
	hidTouchRead(&tp);
	bool inGame = state == ST_GAME && overlay == OV_NONE;
	if (kDown & KEY_TOUCH) {
		tDown = true; tDrag = false;
		tx0 = tx = tp.px; ty0 = ty = tp.py; tLastX = tx;
		tSlot = -1;
		pressX = tx; pressY = ty; pressT = 0.12f;
		Hit *h = hitAt(tx, ty);
		if (inGame && h && h->act == A_SLOT) tSlot = h->p;
		if (state == ST_HELP) tDrag = true;
	} else if (tDown && (kHeld & KEY_TOUCH)) {
		float nx = tp.px, ny = tp.py;
		if (state == ST_HELP) helpScroll -= ny - ty;
		if (inGame && tSlot >= 0 && !tDrag) {
			float dx = nx - tx0, dy = ny - ty0;
			if (dy < -8 && -dy > fabsf(dx) * 0.5f) { tDrag = true; placeType = tSlot; placeDrag = true; sel = -1; }
			else if (fabsf(dx) > 10 && fabsf(dx) > fabsf(dy)) { tDrag = true; tSlot = -2; }
		}
		if (inGame && tSlot == -2) {
			stripScroll -= nx - tLastX;
			if (dt > 0) stripV = -(nx - tLastX) / dt;
		}
		if (inGame && placeDrag && placeType >= 0) { ghostX = nx / BS; ghostY = (ny - 26) / BS; }
		tLastX = nx;
		tx = nx; ty = ny;
	}
	if (kUp & KEY_TOUCH) {
		if (inGame && placeDrag && placeType >= 0) {
			if (ty < 188) tryPlace(ghostX, ghostY);
			else placeType = -1;
			placeDrag = false;
		} else if (tDown && !tDrag) {
			Hit *h = hitAt(tx0, ty0);
			if (h) {
				if (h->act == A_SLOT) {
					if (placeType == h->p) placeType = -1;
					else { placeType = h->p; placeDrag = false; sel = -1; ghostX = 200; ghostY = 110; }
					play(S_CLICK, 1.1f, 0.35f);
				} else act(h, tx0, ty0);
			}
		}
		tDown = false;
		tDrag = false;
		tSlot = -1;
	}
	if (!(kHeld & KEY_TOUCH) && fabsf(stripV) > 1) {
		stripScroll += stripV * dt;
		stripV *= powf(0.02f, dt);
	}
	if (kDown & KEY_START) {
		if (state == ST_GAME) { overlay = overlay == OV_PAUSE ? OV_NONE : (overlay == OV_NONE ? OV_PAUSE : overlay); overT = 0; }
		else if (state == ST_TITLE) quitReq = true;
		else state = ST_TITLE;
	}
	if (state == ST_GAME && overlay == OV_NONE) {
		if (kDown & KEY_A) { if (!G.waveActive) startWave(); else ff = !ff; }
		if (kDown & KEY_B) { sel = -1; placeType = -1; }
		if (kDown & KEY_R) stripScroll += SLOT_W * 3;
		if (kDown & KEY_L) stripScroll -= SLOT_W * 3;
		if (placeType >= 0 && !placeDrag) {
			if (kHeld & KEY_LEFT) ghostX -= 90 * dt;
			if (kHeld & KEY_RIGHT) ghostX += 90 * dt;
			if (kHeld & KEY_UP) ghostY -= 90 * dt;
			if (kHeld & KEY_DOWN) ghostY += 90 * dt;
			if (kDown & KEY_X) tryPlace(ghostX, ghostY);
		}
	} else if (state == ST_GAME && (kDown & KEY_B) && overlay == OV_PAUSE) overlay = OV_NONE;
	if (state == ST_HELP) {
		if (kHeld & KEY_DOWN) helpScroll += 200 * dt;
		if (kHeld & KEY_UP) helpScroll -= 200 * dt;
		if (kDown & KEY_B) state = ST_TITLE;
	}
	if ((state == ST_MAPS || state == ST_TOWERS) && (kDown & KEY_B)) state = ST_TITLE;
}

// ============================================================ main
int main(void) {
	gfxInitDefault();
	osSetSpeedupEnable(true);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(16000);
	C2D_Prepare();
	C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
	tgtTop = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	tgtTopR = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	tgtBot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	tbuf = C2D_TextBufNew(8192);
	gfxLoad();
	srand((unsigned)osGetTime());
	td_init_tables();
	saveRead();
#ifdef DESKTOP_TEST
	if (getenv("TD_XP")) SD.xp = atof(getenv("TD_XP"));
	if (getenv("TD_TXP")) for (int t = 0; t < NTOWER; t++) SD.towerXp[t] = atof(getenv("TD_TXP"));
#endif
	gfxSet3D(SD.stereo);
	sndInit();
	td_new(&G, 0, 1);
	aptHook(&hookCookie, onAptHook, NULL);

	u64 last = osGetTime();
	float acc = 0;
	while (aptMainLoop() && !quitReq) {
		hidScanInput();
		u32 kDown = hidKeysDown(), kHeld = hidKeysHeld(), kUp = hidKeysUp();
		u64 now = osGetTime();
		float dt = now >= last ? (now - last) / 1000.0f : 0;
		last = now;
		if (dt > 0.1f) dt = 0.1f;
		T += dt;
		if (pressT > 0) pressT -= dt;
		overT += dt;
		if (shake > 0) shake -= dt * 2;
		handleInput(kDown, kHeld, kUp, dt);
		if (state == ST_GAME) {
			if (overlay == OV_NONE) {
				acc += dt * (ff ? 3 : 1);
				int steps = 0;
				while (acc >= STEP && steps < 8) { td_step(&G); acc -= STEP; steps++; }
				if (steps >= 8) acc = 0;
				if (!G.waveActive && SD.autoStart && G.wave > 0 && !G.won && !G.lost) {
					autoT -= dt;
					if (autoT <= 0) startWave();
				}
			}
			gameEvents(dt);
			if (sel >= 0 && !G.tw[sel].alive) sel = -1;
			float target = (sel >= 0) ? 1 : 0;
			panelAnim += (target - panelAnim) * fminf(1, dt * 14);
		}
		if (bannerT > 0) bannerT -= dt;

		slider3d = SD.stereo ? osGet3DSliderState() : 0;
		C2D_TextBufClear(tbuf);
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		for (int eye = 0; eye < 2; eye++) {
			if (eye == 1 && slider3d <= 0.01f) break;
			C3D_RenderTarget *tg = eye ? tgtTopR : tgtTop;
			C2D_TargetClear(tg, C(0x000000));
			C2D_SceneBegin(tg);
			eyeSign = eye ? 1 : -1;
			if (state == ST_TITLE || state == ST_HELP) drawTitleTop();
			else if (state == ST_MAPS) drawMapsTop();
			else if (state == ST_TOWERS) drawTowersTop();
			else drawGameTop();
		}
		eyeSign = 0;
		C2D_TargetClear(tgtBot, C(0x000000));
		C2D_SceneBegin(tgtBot);
		if (state == ST_TITLE) drawTitleBottom();
		else if (state == ST_MAPS) drawMapsBottom();
		else if (state == ST_HELP) drawHelpBottom();
		else if (state == ST_TOWERS) drawTowersBottom();
		else drawGameBottom();
		C3D_FrameEnd(0);
	}
	if (state == ST_GAME) commitXp();
	saveWrite();
	sndExit();
	gfxFree();
	C2D_TextBufDelete(tbuf);
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
