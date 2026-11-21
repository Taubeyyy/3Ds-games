// Glibberalarm - 3DS Frontend (citro2d)
#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "td.h"

#define TW 400
#define BW 320
#define SH 240
#define FONT_H 30.0f
#define PI 3.14159265f
#define BS 0.8f // Massstab des Spielfelds unten

#define C_TEXT 0xEEF1FF
#define C_DIM 0x9AA2C8
#define C_PANEL 0x1A2046
#define C_PANEL2 0x262E60
#define C_CYAN 0x4AD8FF
#define C_GOLD 0xFFD25A
#define C_GREEN 0x5AE07A
#define C_RED 0xFF5A6E
#define C_PURPLE 0xB070FF
#define C_GRAY 0x3A4270
#define C_SLIME 0x7CE85A

#define SAVE_DIR "sdmc:/3ds/glibberalarm"
#define SAVE_PATH SAVE_DIR "/save.dat"
#define SAVE_TMP SAVE_DIR "/save.tmp"
#define SAVE_MAGIC 0x474C4942u
#define SAVE_VERSION 1

static TD G;
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
typedef struct {
	uint32_t magic, version;
	uint8_t sound, stereo, autoStart, pad;
	uint8_t best[NMAP][NDIFF], won[NMAP][NDIFF];
	uint8_t hasResume, endless, pad2[2];
	int32_t map, diff, wave, lives;
	double coins;
	SaveTower tw[MAX_TW];
	double totalPops;
	double reserved[32];
} SaveData;
static SaveData SD;
static SaveData snap; // Stand am letzten Wellenende

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
	static SaveData s;
	size_t n = fread(&s, 1, sizeof s, f);
	fclose(f);
	if (n == sizeof s && s.magic == SAVE_MAGIC && s.version <= SAVE_VERSION) SD = s;
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
		if (!o->alive) continue;
		t->alive = 1; t->type = o->type; t->a = o->a; t->b = o->b; t->target = o->target;
		t->x = o->x; t->y = o->y; t->spent = o->spent; t->pops = o->pops; t->ang = -1.57f;
	}
}

static aptHookCookie hookCookie;
static void onAptHook(APT_HookType hook, void *param) {
	(void)param;
	if (hook == APTHOOK_ONSUSPEND || hook == APTHOOK_ONSLEEP || hook == APTHOOK_ONEXIT) saveWrite();
}

// ============================================================ Farben & Zeichnen
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
static void circG(float x, float y, float r, u32 hex, float a) {
	if (r > 0.3f) C2D_DrawCircle(x, y, 0, r, RGBA(lighten(hex, 0.45f), a), RGBA(hex, a), RGBA(hex, a), RGBA(darken(hex, 0.4f), a));
}
static void ell(float cx, float cy, float w, float h, u32 c) { if (w > 0.5f && h > 0.5f) C2D_DrawEllipseSolid(cx - w / 2, cy - h / 2, 0, w, h, c); }
static void ellG(float cx, float cy, float w, float h, u32 hex, float a) {
	if (w > 0.5f && h > 0.5f)
		C2D_DrawEllipse(cx - w / 2, cy - h / 2, 0, w, h, RGBA(lighten(hex, 0.5f), a), RGBA(hex, a), RGBA(hex, a), RGBA(darken(hex, 0.35f), a));
}
static void tri(float x0, float y0, float x1, float y1, float x2, float y2, u32 c) { C2D_DrawTriangle(x0, y0, c, x1, y1, c, x2, y2, c, 0); }
static void line(float x0, float y0, float x1, float y1, float th, u32 c) { C2D_DrawLine(x0, y0, c, x1, y1, c, th, 0); }
static void ring(float x, float y, float r, float th, u32 c, int seg) {
	for (int k = 0; k < seg; k++) {
		float a0 = 2 * PI * k / seg, a1 = 2 * PI * (k + 1) / seg;
		line(x + cosf(a0) * r, y + sinf(a0) * r, x + cosf(a1) * r, y + sinf(a1) * r, th, c);
	}
}
static void rrect(float x, float y, float w, float h, float r, u32 c) {
	if (r * 2 > h) r = h / 2;
	if (r * 2 > w) r = w / 2;
	rect(x + r, y, w - 2 * r, h, c);
	rect(x, y + r, r, h - 2 * r, c);
	rect(x + w - r, y + r, r, h - 2 * r, c);
	circ(x + r, y + r, r, c);
	circ(x + w - r, y + r, r, c);
	circ(x + r, y + h - r, r, c);
	circ(x + w - r, y + h - r, r, c);
}
static void panel(float x, float y, float w, float h, float r, u32 fill, u32 border) {
	rrect(x, y, w, h, r, C(border));
	rrect(x + 1, y + 1, w - 2, h - 2, r - 1, C(fill));
}

// ============================================================ Text
static C2D_Text mk(const char *s) {
	C2D_Text t;
	C2D_TextParse(&t, tbuf, s);
	C2D_TextOptimize(&t);
	return t;
}
static void text(float x, float y, float sz, u32 col, int align, const char *s) {
	C2D_Text t = mk(s);
	C2D_DrawText(&t, C2D_WithColor | align, x, y, 0, sz, sz, col);
}
static void textf(float x, float y, float sz, u32 col, int align, const char *fmt, ...) {
	char b[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(b, sizeof b, fmt, ap);
	va_end(ap);
	text(x, y, sz, col, align, b);
}
static void textFit(float x, float y, float sz, u32 col, int align, float maxw, const char *s) {
	C2D_Text t = mk(s);
	float w, h;
	C2D_TextGetDimensions(&t, sz, sz, &w, &h);
	float s2 = sz;
	if (w > maxw && w > 0) s2 = sz * maxw / w;
	C2D_DrawText(&t, C2D_WithColor | align, x, y + (sz - s2) * FONT_H * 0.5f, 0, s2, s2, col);
}
static void textWrap(float x, float y, float sz, u32 col, float w, const char *s) {
	C2D_Text t = mk(s);
	C2D_DrawText(&t, C2D_WithColor | C2D_WordWrap, x, y, 0, sz, sz, col, w);
}
// Text mit Schatten (fuer HUD ueber dem Spielfeld)
static void textS(float x, float y, float sz, u32 col, int align, const char *s) {
	C2D_Text t = mk(s);
	C2D_DrawText(&t, C2D_WithColor | align, x + 1, y + 1, 0, sz, sz, RGBA(0x000000, 0.7f));
	C2D_DrawText(&t, C2D_WithColor | align, x, y, 0, sz, sz, col);
}

// ============================================================ Sound
enum { S_POP, S_SHOOT, S_BOOM, S_ZAP, S_FREEZE, S_COIN, S_PLACE, S_UPGRADE, S_WAVE, S_LEAK, S_WIN, S_LOSE, S_CLICK, S_ERR, NSND };
typedef struct { s16 *data; u32 n; } Sample;
static Sample snd[NSND];
static ndspWaveBuf wbuf[12];
static bool sndOk;
static int sndCh;
#define SRATE 22050
#define NCH 12

static void genSnd(int id, float dur) {
	u32 n = (u32)(dur * SRATE);
	s16 *d = (s16 *)linearAlloc(n * sizeof(s16));
	if (!d) return;
	float ph = 0, lp = 0;
	for (u32 i = 0; i < n; i++) {
		float t = (float)i / SRATE, p = t / dur, v = 0, f = 440;
		float nz = (rand() & 1023) / 512.0f - 1;
		switch (id) {
		case S_POP: f = 700 - 500 * p; v = sinf(ph) * (1 - p) * (1 - p) + nz * 0.25f * expf(-t * 120); break;
		case S_SHOOT: f = 1800 - 900 * p; v = sinf(ph) * expf(-t * 70) * 0.35f + nz * 0.15f * expf(-t * 90); break;
		case S_BOOM: lp += (nz - lp) * 0.15f; f = 60; v = (lp * 1.8f + sinf(ph) * 0.6f) * expf(-t * 7); break;
		case S_ZAP: f = 220 + 80 * sinf(t * 300); v = ((sinf(ph) > 0 ? 0.5f : -0.5f) + nz * 0.3f) * (1 - p) * 0.6f; break;
		case S_FREEZE: f = 2400 + 600 * sinf(t * 40); v = sinf(ph) * (1 - p) * 0.3f + nz * 0.1f * (1 - p); break;
		case S_COIN: f = t < 0.06f ? 1320 : 1760; v = (sinf(ph) + 0.3f * sinf(ph * 2)) * expf(-fmodf(t, 0.06f) * 25) * 0.6f; break;
		case S_PLACE: f = 180 - 80 * p; v = sinf(ph) * expf(-t * 25) + nz * 0.2f * expf(-t * 60); break;
		case S_UPGRADE: {
			static const float nt[4] = {660, 880, 1100, 1320};
			int k = (int)(t / 0.06f);
			if (k > 3) k = 3;
			f = nt[k];
			v = sinf(ph) * expf(-fmodf(t, 0.06f) * 20) * 0.6f;
			break;
		}
		case S_WAVE: f = t < 0.15f ? 392 : 523; v = (sinf(ph) + 0.4f * sinf(ph * 2) + 0.2f * sinf(ph * 3)) * fminf(1, t * 30) * (1 - p) * 0.45f; break;
		case S_LEAK: f = 140 - 60 * p; v = (sinf(ph) > 0 ? 0.5f : -0.5f) * (1 - p) * 0.6f; break;
		case S_WIN: {
			static const float nt[6] = {523, 659, 784, 1047, 784, 1047};
			int k = (int)(t / 0.13f);
			if (k > 5) k = 5;
			f = nt[k];
			v = (sinf(ph) + 0.3f * sinf(ph * 2)) * (k == 5 ? 1 - (t - 0.65f) / (dur - 0.65f) : 0.8f) * 0.55f;
			break;
		}
		case S_LOSE: f = 300 * powf(0.5f, p * 1.5f); v = (sinf(ph) + 0.4f * sinf(ph * 1.5f)) * (1 - p) * 0.5f; break;
		case S_CLICK: f = 1000; v = sinf(ph) * expf(-t * 80) * 0.5f; break;
		case S_ERR: f = 150; v = (sinf(ph) > 0 ? 0.4f : -0.4f) * (1 - p); break;
		}
		ph += 2 * PI * f / SRATE;
		if (ph > 2 * PI * 64) ph -= 2 * PI * 64;
		if (v > 1) v = 1;
		if (v < -1) v = -1;
		d[i] = (s16)(v * 24000);
	}
	DSP_FlushDataCache(d, n * sizeof(s16));
	snd[id].data = d;
	snd[id].n = n;
}
static void sndInit(void) {
	if (R_FAILED(ndspInit())) return;
	sndOk = true;
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	for (int ch = 0; ch < NCH; ch++) {
		ndspChnReset(ch);
		ndspChnSetInterp(ch, NDSP_INTERP_LINEAR);
		ndspChnSetRate(ch, SRATE);
		ndspChnSetFormat(ch, NDSP_FORMAT_MONO_PCM16);
	}
	static const float dur[NSND] = {0.09f, 0.05f, 0.45f, 0.14f, 0.3f, 0.16f, 0.12f, 0.28f, 0.45f, 0.25f, 1.1f, 1.0f, 0.04f, 0.1f};
	for (int i = 0; i < NSND; i++) genSnd(i, dur[i]);
}
static void play(int id, float pitch, float vol) {
	if (!sndOk || !SD.sound || !snd[id].data) return;
	int ch = sndCh;
	sndCh = (sndCh + 1) % NCH;
	ndspChnWaveBufClear(ch);
	ndspChnSetRate(ch, SRATE * pitch);
	float mix[12] = {0};
	mix[0] = mix[1] = vol;
	ndspChnSetMix(ch, mix);
	memset(&wbuf[ch], 0, sizeof wbuf[ch]);
	wbuf[ch].data_vaddr = snd[id].data;
	wbuf[ch].nsamples = snd[id].n;
	ndspChnWaveBufAdd(ch, &wbuf[ch]);
}
static void sndExit(void) {
	if (!sndOk) return;
	ndspExit();
	for (int i = 0; i < NSND; i++) if (snd[i].data) linearFree(snd[i].data);
}
static float frnd(void) { return (rand() & 0xFFFF) / 65535.0f; }

// ============================================================ Zustand UI
enum { ST_TITLE, ST_MAPS, ST_GAME, ST_HELP };
static int state = ST_TITLE;
static int selMap = 0;
static int sel = -1;        // ausgewaehlter Turm
static int placeType = -1;  // Turm, der platziert wird
static bool placeDrag;      // per Ziehen aus der Leiste
static float ghostX = 200, ghostY = 120;
static float stripScroll, stripV;
static int ff = 0;          // Vorspulen
static float autoT;
static float panelAnim;
static char bannerText[64], bannerSub[64];
static float bannerT;
static bool endShown;
static float helpScroll;
enum { OV_NONE, OV_PAUSE, OV_WIN, OV_LOSE, OV_CONFIRM_MENU };
static int overlay;
static bool quitReq;

typedef struct { float x, y, w, h; int act, p; } Hit;
#define NHIT 96
static Hit hits[NHIT];
static int nHits;
enum { A_NONE, A_SLOT, A_FIELD, A_PLAY, A_PAUSE, A_UPG, A_SELL, A_TARGET, A_CLOSE, A_OV, A_MENU, A_MAP, A_DIFF, A_BACK, A_OPT };
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

static void banner(const char *a, const char *b) {
	snprintf(bannerText, sizeof bannerText, "%s", a);
	snprintf(bannerSub, sizeof bannerSub, "%s", b);
	bannerT = 2.6f;
}

// ============================================================ Deko je Karte
typedef struct { float x, y, s; int kind; } Deco;
#define NDECO 40
static Deco deco[NDECO];
static int nDeco;
static void makeDeco(int map) {
	srand(1234 + map * 77);
	nDeco = 0;
	TD tmp;
	td_new(&tmp, map, 0);
	for (int tries = 0; tries < 600 && nDeco < NDECO; tries++) {
		float x = 10 + frnd() * (FW - 20), y = 10 + frnd() * (FH - 20);
		if (!td_can_place(&tmp, 0, x, y)) continue;
		bool ok = true;
		for (int i = 0; i < nDeco; i++) if ((deco[i].x - x) * (deco[i].x - x) + (deco[i].y - y) * (deco[i].y - y) < 22 * 22) ok = false;
		if (!ok) continue;
		deco[nDeco++] = (Deco){x, y, 0.7f + frnd() * 0.6f, rand() % 3};
	}
	srand((unsigned)osGetTime());
}

// ============================================================ Spielfeld zeichnen
static float FS, FOX, FOY;   // aktueller Massstab/Versatz
static int nAliveEn;         // fuer Detailstufe
static bool FTOP;
static inline float FX(float x, float depth) { return FOX + x * FS + (FTOP ? eyeSign * depth * slider3d : 0); }
static inline float FY(float y) { return FOY + y * FS; }

static void drawGround(void) {
	const MapDef *m = &MAPS[G.map];
	rect(FX(0, 4), FY(0), FW * FS, FH * FS, C(m->ground1));
	// Flecken
	for (int i = 0; i < 18; i++) {
		float x = fmodf(i * 97.3f, FW), y = fmodf(i * 53.7f + 20, FH);
		ell(FX(x, 4), FY(y), (30 + (i % 5) * 9) * FS, (18 + (i % 3) * 6) * FS, C(m->ground2));
	}
	if (m->theme == 1) { // Meer unten rechts
		rectG(FX(0, 4), FY(214), FW * FS, 26 * FS, C(0x6AC8E8), C(0x3A90C8));
		for (int i = 0; i < 8; i++) {
			float x = fmodf(i * 61 + T * 8, FW);
			line(FX(x, 4), FY(222 + (i % 3) * 5), FX(x + 14, 4), FY(222 + (i % 3) * 5), 1.2f * FS, RGBA(0xFFFFFF, 0.5f));
		}
	}
	if (m->theme == 3) { // Lavaglut
		for (int i = 0; i < 6; i++) {
			float x = fmodf(i * 71.0f + 30, FW), y = fmodf(i * 43.0f + 15, FH);
			float p = 0.5f + 0.5f * sinf(T * 2 + i);
			ell(FX(x, 4), FY(y), 26 * FS, 14 * FS, RGBA(0xFF5A1A, 0.25f + p * 0.2f));
			ell(FX(x, 4), FY(y), 14 * FS, 7 * FS, RGBA(0xFFC04A, 0.35f + p * 0.3f));
		}
	}
	// Deko
	for (int i = 0; i < nDeco; i++) {
		Deco *d = &deco[i];
		float x = FX(d->x, 3), y = FY(d->y), s = d->s * FS;
		switch (m->theme) {
		case 0:
			if (d->kind == 0) { ell(x + 2 * s, y + 6 * s, 14 * s, 5 * s, RGBA(0, 0.25f)); circG(x, y, 7 * s, m->deco, 1); circ(x - 2 * s, y - 2 * s, 2.5f * s, RGBA(0xFFFFFF, 0.15f)); }
			else if (d->kind == 1) { for (int k = 0; k < 5; k++) circ(x + cosf(k * 1.26f) * 2.5f * s, y + sinf(k * 1.26f) * 2.5f * s, 2 * s, C(0xFFE0F0)); circ(x, y, 1.6f * s, C(0xFFD25A)); }
			else { ell(x, y, 10 * s, 6 * s, C(0x8A8A7A)); ell(x - 1, y - 1.5f * s, 7 * s, 3 * s, C(0xA8A898)); }
			break;
		case 1:
			if (d->kind == 0) { line(x, y + 6 * s, x + 2 * s, y - 8 * s, 2 * s, C(0x8A6A3A)); for (int k = 0; k < 4; k++) line(x + 2 * s, y - 8 * s, x + 2 * s + cosf(k * 1.1f - 2.6f) * 9 * s, y - 8 * s + sinf(k * 1.1f - 2.6f) * 6 * s + 3 * s, 2 * s, C(0x3A9A4A)); }
			else if (d->kind == 1) { ell(x, y, 8 * s, 6 * s, C(0xF0A0B0)); line(x - 3 * s, y, x + 3 * s, y, 0.8f * s, C(0xC07080)); }
			else { circ(x, y, 3 * s, C(0xFFF0D0)); }
			break;
		case 2:
			if (d->kind != 2) {
				float h = 10 * s;
				tri(x - 4 * s, y + 3 * s, x, y - h, x + 4 * s, y + 3 * s, RGBA(m->deco, 0.9f));
				tri(x - 1 * s, y + 3 * s, x + 4 * s, y - h * 0.6f, x + 7 * s, y + 3 * s, RGBA(lighten(m->deco, 0.3f), 0.8f));
			} else ell(x, y, 12 * s, 6 * s, C(0x2A2A50));
			break;
		case 3:
			if (d->kind == 0) { ell(x, y, 12 * s, 8 * s, C(0x3A2A24)); ell(x - 1, y - 1.5f * s, 8 * s, 4 * s, C(0x4A3A32)); }
			else if (d->kind == 1) { circ(x, y, 2 * s, RGBA(0xFF8A3A, 0.6f + 0.4f * sinf(T * 3 + i))); }
			else { line(x - 4 * s, y, x + 4 * s, y - 2 * s, 1.5f * s, C(0x2A1A16)); }
			break;
		}
	}
}

static void drawPath(void) {
	const MapDef *m = &MAPS[G.map];
	float w = PATH_W * 2 * FS;
	for (int pass = 0; pass < 2; pass++) {
		u32 c = pass == 0 ? C(m->path2) : C(m->path1);
		float ww = pass == 0 ? w + 4 * FS : w;
		for (int i = 0; i + 1 < m->n; i++)
			line(FX(m->pts[i][0], 3), FY(m->pts[i][1]), FX(m->pts[i + 1][0], 3), FY(m->pts[i + 1][1]), ww, c);
		for (int i = 0; i < m->n; i++) circ(FX(m->pts[i][0], 3), FY(m->pts[i][1]), ww / 2, c);
	}
	// Markierungen
	for (float d = 10; FTOP && d < G.pathLen; d += 26) {
		float x, y, x2, y2;
		td_path_pos(&G, d, &x, &y);
		td_path_pos(&G, d + 5, &x2, &y2);
		line(FX(x, 3), FY(y), FX(x2, 3), FY(y2), 2 * FS, RGBA(darken(m->path1, 0.15f), 0.6f));
	}
	// Basis am Ende
	float ex = m->pts[m->n - 1][0], ey = m->pts[m->n - 1][1];
	if (ex > FW) ex = FW - 12;
	if (ey > FH) ey = FH - 12;
	if (ex < 0) ex = 12;
	float bx = FX(ex, 2), by = FY(ey);
	circ(bx, by, 13 * FS, RGBA(0x000000, 0.35f));
	circ(bx, by, 10 * FS, C(0xFFE0A0));
	tri(bx - 10 * FS, by - 3 * FS, bx, by - 14 * FS, bx + 10 * FS, by - 3 * FS, C(0xE05A4A));
	rect(bx - 2.5f * FS, by, 5 * FS, 7 * FS, C(0x8A5A3A));
}

static void drawTower(const Tower *t, float alpha, bool ghost) {
	float x = FX(t->x, 0), y = FY(t->y), s = FS;
	const TowerDef *d = &TDEF[t->type];
	u32 col = d->color;
	float a = t->ang;
	float ca = cosf(a), sa = sinf(a);
	int tier = t->a > t->b ? t->a : t->b;
	ell(x + 1.5f * s, y + 7 * s, 18 * s, 6 * s, RGBA(0x000000, 0.3f * alpha));
	if (tier >= 3) ring(x, y + 1 * s, 11 * s, 1.5f * s, RGBA(tier >= 4 ? C_GOLD : 0xC0C8E0, 0.8f * alpha), 16);
	if (tier >= 4) circ(x, y, 14 * s, RGBA(C_GOLD, (0.12f + 0.06f * sinf(T * 4)) * alpha));
	float br = 8 * s * (1 + (t->flash > 0 ? 0.1f : 0));
	switch (t->type) {
	case T_IGEL:
		for (int k = 0; k < 9; k++) {
			float an = PI * 0.15f + k * PI * 0.19f + a + PI / 2;
			tri(x + cosf(an - 0.25f) * br * 0.8f, y + sinf(an - 0.25f) * br * 0.8f, x + cosf(an) * br * 1.45f, y + sinf(an) * br * 1.45f,
				x + cosf(an + 0.25f) * br * 0.8f, y + sinf(an + 0.25f) * br * 0.8f, RGBA(0x5A3A22, alpha));
		}
		circG(x, y, br, col, alpha);
		circ(x + ca * br * 0.45f, y + sa * br * 0.45f, br * 0.55f, RGBA(0xF0D8B0, alpha));
		circ(x + ca * br * 0.95f, y + sa * br * 0.95f, 1.6f * s, RGBA(0x2A1A10, alpha));
		break;
	case T_KRABBE:
		line(x, y, x + ca * br * 1.5f, y + sa * br * 1.5f, 4 * s, RGBA(0x3A3A44, alpha));
		circG(x - sa * br * 1.0f, y + ca * br * 1.0f, br * 0.45f, darken(col, 0.1f), alpha);
		circG(x + sa * br * 1.0f, y - ca * br * 1.0f, br * 0.45f, darken(col, 0.1f), alpha);
		ellG(x, y, br * 2.2f, br * 1.7f, col, alpha);
		break;
	case T_PINGUIN:
		circG(x, y, br, 0x2A2E3A, alpha);
		circ(x + ca * br * 0.3f, y + sa * br * 0.3f, br * 0.65f, RGBA(0xF4F8FF, alpha));
		tri(x + ca * br * 0.7f - sa * 2 * s, y + sa * br * 0.7f + ca * 2 * s, x + ca * br * 1.35f, y + sa * br * 1.35f,
			x + ca * br * 0.7f + sa * 2 * s, y + sa * br * 0.7f - ca * 2 * s, RGBA(0xFF9A2A, alpha));
		break;
	case T_EULE:
		tri(x - br * 0.8f, y - br * 0.4f, x - br * 0.6f, y - br * 1.35f, x - br * 0.15f, y - br * 0.8f, RGBA(darken(col, 0.2f), alpha));
		tri(x + br * 0.8f, y - br * 0.4f, x + br * 0.6f, y - br * 1.35f, x + br * 0.15f, y - br * 0.8f, RGBA(darken(col, 0.2f), alpha));
		circG(x, y, br, col, alpha);
		for (int k = -1; k <= 1; k += 2) {
			float ex = x + k * br * 0.38f, ey = y - br * 0.15f;
			circ(ex, ey, br * 0.36f, RGBA(0xFFD25A, alpha));
			circ(ex + ca * 1.2f * s, ey + sa * 1.2f * s, br * 0.18f, RGBA(0x10101A, alpha));
		}
		tri(x - 1.5f * s, y + br * 0.15f, x + 1.5f * s, y + br * 0.15f, x, y + br * 0.5f, RGBA(0xFF9A2A, alpha));
		break;
	case T_SEEIGEL: {
		float rot = T * 0.6f;
		for (int k = 0; k < 12; k++) {
			float an = rot + k * PI / 6;
			line(x + cosf(an) * br * 0.6f, y + sinf(an) * br * 0.6f, x + cosf(an) * br * 1.5f, y + sinf(an) * br * 1.5f, 1.6f * s, RGBA(lighten(col, 0.2f), alpha));
		}
		circG(x, y, br * 0.9f, col, alpha);
		break;
	}
	case T_KAENGURU:
		ell(x - br * 0.45f, y - br * 0.95f, br * 0.5f, br * 1.0f, RGBA(darken(col, 0.15f), alpha));
		ell(x + br * 0.45f, y - br * 0.95f, br * 0.5f, br * 1.0f, RGBA(darken(col, 0.15f), alpha));
		circG(x, y, br, col, alpha);
		circ(x + ca * br * 0.6f, y + sa * br * 0.6f, br * 0.35f, RGBA(0xF0D8B0, alpha));
		{
			float bx = x + ca * br * 1.2f, by = y + sa * br * 1.2f;
			line(bx - sa * 4 * s, by + ca * 4 * s, bx + ca * 3 * s, by + sa * 3 * s, 2 * s, RGBA(0xC08040, alpha));
			line(bx + ca * 3 * s, by + sa * 3 * s, bx + sa * 4 * s, by - ca * 4 * s, 2 * s, RGBA(0xC08040, alpha));
		}
		break;
	case T_AAL:
		ring(x, y, br * 0.75f, 3.2f * s, RGBA(col, alpha), 14);
		circG(x + ca * br * 0.75f, y + sa * br * 0.75f, br * 0.5f, lighten(col, 0.2f), alpha);
		circ(x + ca * br * 0.9f, y + sa * br * 0.9f, 1.2f * s, RGBA(0x10101A, alpha));
		if (t->flash > 0 || ghost) circ(x, y, br * 0.4f, RGBA(0xFFFFFF, 0.8f * alpha));
		break;
	case T_BIENE:
		for (int k = 0; k < 6; k++) {
			float a0 = k * PI / 3, a1 = (k + 1) * PI / 3;
			tri(x, y, x + cosf(a0) * br * 1.05f, y + sinf(a0) * br * 1.05f, x + cosf(a1) * br * 1.05f, y + sinf(a1) * br * 1.05f, RGBA(k % 2 ? col : darken(col, 0.15f), alpha));
		}
		rect(x - br * 0.7f, y - 1 * s, br * 1.4f, 2 * s, RGBA(0x4A3010, alpha));
		circ(x, y + br * 0.35f, 2 * s, RGBA(0x4A3010, alpha));
		for (int k = 0; k < 3; k++) {
			float an = T * 3 + k * 2.1f;
			float bx = x + cosf(an) * br * 1.5f, by = y + sinf(an) * br * 0.9f;
			circ(bx, by, 1.6f * s, RGBA(0xFFE04A, alpha));
			circ(bx, by - 1 * s, 1.0f * s, RGBA(0xFFFFFF, 0.7f * alpha));
		}
		break;
	case T_DACHS:
		circ(x + ca * br * 0.9f, y + sa * br * 0.9f, br * 0.55f, RGBA(0xC04A3A, alpha));
		circ(x + ca * br * 0.9f, y + sa * br * 0.9f, br * 0.35f, RGBA(0xF0E0C0, alpha));
		circG(x, y, br, col, alpha);
		line(x - ca * br * 0.8f, y - sa * br * 0.8f, x + ca * br * 0.5f, y + sa * br * 0.5f, 2.5f * s, RGBA(0xFFFFFF, alpha));
		if (t->flash > 0) ring(x, y, br * 1.6f, 1.2f * s, RGBA(0xFFFFFF, 0.5f), 14);
		break;
	case T_SCHNECKE:
		ell(x + ca * br * 0.2f, y + 3 * s, br * 2.1f, br * 0.8f, RGBA(lighten(col, 0.2f), alpha));
		circG(x - ca * br * 0.25f, y - 1.5f * s, br * 0.8f, 0xB07A40, alpha);
		ring(x - ca * br * 0.25f, y - 1.5f * s, br * 0.45f, 1.2f * s, RGBA(0x6A4A20, alpha), 10);
		circ(x + ca * br * 0.95f, y + sa * br * 0.6f + 1 * s, 1.8f * s, RGBA(lighten(col, 0.1f), alpha));
		break;
	}
	// Augen fuer die meisten Tiere
	if (t->type == T_IGEL || t->type == T_KRABBE || t->type == T_PINGUIN || t->type == T_KAENGURU || t->type == T_DACHS || t->type == T_SEEIGEL) {
		float ex = x + ca * br * 0.3f, ey = y + sa * br * 0.3f;
		for (int k = -1; k <= 1; k += 2) {
			float px2 = ex - sa * k * br * 0.32f, py2 = ey + ca * k * br * 0.32f;
			circ(px2, py2, br * 0.22f, RGBA(0xFFFFFF, alpha));
			circ(px2 + ca * 0.8f * s, py2 + sa * 0.8f * s, br * 0.11f, RGBA(0x10101A, alpha));
		}
	}
	// Stufenpunkte
	if (!ghost && (t->a || t->b)) {
		for (int k = 0; k < t->a; k++) circ(x - 9 * s + k * 2.6f * s, y + 10 * s, 1.0f * s, RGBA(C_CYAN, alpha));
		for (int k = 0; k < t->b; k++) circ(x + 9 * s - k * 2.6f * s, y + 10 * s, 1.0f * s, RGBA(C_GOLD, alpha));
	}
}

static void drawEnemy(const Enemy *e) {
	const EnemyDef *d = &EDEF[e->type];
	float r = d->radius;
	float sq = sinf(e->wob) * 0.09f;
	float x = FX(e->x, -1), y = FY(e->y - r * 0.15f), s = FS;
	float w = r * 2 * (1 + sq) * s, h = r * 2 * (1 - sq) * s;
	bool geist = e->mods & M_GEIST;
	float al = geist ? 0.45f : 1;
	u32 col = d->color;
	if (geist) col = hexMix(col, 0xC0D0FF, 0.4f);
	bool simple = e->type < E_KOENIG && ((!FTOP && nAliveEn > 70) || nAliveEn > 170);
	if (!simple) ell(x + 1.5f * s, y + h * 0.42f, w * 0.9f, h * 0.35f, RGBA(0x000000, 0.25f * al));
	ellG(x, y, w, h, col, al);
	if (!simple) ell(x - w * 0.2f, y - h * 0.22f, w * 0.3f, h * 0.2f, RGBA(0xFFFFFF, 0.45f * al));
	// Augen in Laufrichtung
	float dx = e->dirx, dy = e->diry;
	for (int k = -1; k <= 1 && !simple; k += 2) {
		float ex = x + dx * w * 0.18f + k * w * 0.17f, ey = y + dy * h * 0.12f - h * 0.05f;
		circ(ex, ey, r * 0.24f * s, RGBA(0xFFFFFF, al));
		circ(ex + dx * r * 0.09f * s, ey + dy * r * 0.09f * s, r * 0.12f * s, RGBA(0x10101A, al));
	}
	if (e->mods & M_PANZER) {
		C2D_DrawEllipse(x - w * 0.52f, y - h * 0.55f, 0, w * 1.04f, h * 0.6f, RGBA(0xE0E4EC, al), RGBA(0xA0A8B8, al), RGBA(0x8A92A2, al), RGBA(0x6A7282, al));
		for (int k = -1; k <= 1; k++) circ(x + k * w * 0.25f, y - h * 0.3f, 0.9f * s, RGBA(0x5A6272, al));
	}
	if (e->mods & M_REGEN) {
		float p = 0.5f + 0.5f * sinf(T * 6 + e->id);
		ring(x, y, r * 1.15f * s, 1.0f * s, RGBA(0xFF7AB0, 0.4f + p * 0.4f), 12);
	}
	if (e->freezeT > 0) {
		rrect(x - w * 0.6f, y - h * 0.6f, w * 1.2f, h * 1.2f, 2 * s, RGBA(0xBFEFFF, 0.45f));
	}
	if (e->acidT > 0) {
		circ(x + w * 0.25f, y + h * 0.3f, r * 0.25f * s, RGBA(C_SLIME, 0.8f));
		circ(x - w * 0.3f, y + h * 0.1f, r * 0.18f * s, RGBA(C_SLIME, 0.8f));
	} else if (e->slowT > 0) {
		circ(x + w * 0.2f, y + h * 0.35f, r * 0.2f * s, RGBA(0xC8E07A, 0.7f));
	}
	if (e->hitT > 0) ell(x, y, w, h, RGBA(0xFFFFFF, e->hitT * 3));
	if (e->type >= E_KOENIG) {
		float cy = y - h * 0.5f;
		for (int k = -1; k <= 1; k++) tri(x + k * r * 0.35f * s - r * 0.18f * s, cy, x + k * r * 0.35f * s, cy - r * 0.45f * s, x + k * r * 0.35f * s + r * 0.18f * s, cy, C(C_GOLD));
		rect(x - r * 0.55f * s, cy - 1 * s, r * 1.1f * s, 2.2f * s, C(C_GOLD));
		float hpMax = d->hp * (G.cur.hpMul > 0 ? G.cur.hpMul : 1);
		float p = e->hp / hpMax;
		rect(x - r * s, y - h * 0.5f - r * 0.65f * s, r * 2 * s, 2.5f * s, RGBA(0x000000, 0.6f));
		rect(x - r * s, y - h * 0.5f - r * 0.65f * s, r * 2 * s * p, 2.5f * s, C(p > 0.5f ? C_GREEN : p > 0.25f ? C_GOLD : C_RED));
	}
}

static void drawProj(const Proj *p) {
	float x = FX(p->x, -2), y = FY(p->y), s = FS;
	float sp = sqrtf(p->vx * p->vx + p->vy * p->vy) + 1e-4f;
	float dx = p->vx / sp, dy = p->vy / sp;
	switch (p->kind) {
	case P_SPIKE: line(x - dx * 4 * s, y - dy * 4 * s, x + dx * 3 * s, y + dy * 3 * s, 1.8f * s, C(0x4A3020)); break;
	case P_BLADE: line(x - dx * 5 * s, y - dy * 5 * s, x + dx * 4 * s, y + dy * 4 * s, 2.6f * s, C(0xE0E8F8)); break;
	case P_SHARD: circ(x, y, 1.3f * s, C(0x6A6A70)); break;
	case P_BOMB: circ(x, y, 3.5f * s, C(0x22222A)); circ(x + 1.5f * s, y - 2.5f * s, 1.2f * s, C(frnd() < 0.5f ? 0xFFD25A : 0xFF6A2A)); break;
	case P_MINIBOMB: circ(x, y, 2.2f * s, C(0x33333A)); break;
	case P_BOOMER: {
		float a = p->rot;
		line(x, y, x + cosf(a) * 5 * s, y + sinf(a) * 5 * s, 2 * s, C(0xC08040));
		line(x, y, x + cosf(a + 1.9f) * 5 * s, y + sinf(a + 1.9f) * 5 * s, 2 * s, C(0xC08040));
		if (p->flags & PF_AP) circ(x, y, 3 * s, RGBA(0xFF8A3A, 0.4f));
		break;
	}
	case P_SLIME: circ(x, y, 3.2f * s, C(C_SLIME)); circ(x - 1 * s, y - 1 * s, 1.2f * s, RGBA(0xFFFFFF, 0.5f)); break;
	case P_ICE: tri(x - dy * 2 * s, y + dx * 2 * s, x + dx * 5 * s, y + dy * 5 * s, x + dy * 2 * s, y - dx * 2 * s, C(0xCFF4FF)); break;
	}
}

static void drawFx(const Fx *f, int idx) {
	float p = f->t / f->max, s = FS;
	float x = FX(f->x, -2), y = FY(f->y);
	switch (f->kind) {
	case FX_RING: ring(x, y, f->r * (0.4f + p * 0.8f) * s, 1.5f * s, RGBA(f->col, 1 - p), 16); break;
	case FX_POP:
		for (int k = 0; k < 6; k++) {
			float a = k * 1.047f + idx;
			float rr = f->r * (0.3f + p * 1.6f) * s;
			circ(x + cosf(a) * rr, y + sinf(a) * rr, f->r * 0.35f * (1 - p) * s, RGBA(f->col, 1 - p));
		}
		break;
	case FX_BOLT: {
		float x2 = FX(f->x2, -2), y2 = FY(f->y2);
		float lx = x, ly = y;
		srand(idx * 31 + (int)(f->t * 30));
		for (int k = 1; k <= 4; k++) {
			float q = k / 4.0f;
			float nx = x + (x2 - x) * q + (k < 4 ? (frnd() - 0.5f) * 8 * s : 0);
			float ny = y + (y2 - y) * q + (k < 4 ? (frnd() - 0.5f) * 8 * s : 0);
			line(lx, ly, nx, ny, 2.2f * s, RGBA(f->col, 1 - p));
			line(lx, ly, nx, ny, 0.8f * s, RGBA(0xFFFFFF, 1 - p));
			lx = nx; ly = ny;
		}
		break;
	}
	case FX_TRACER: line(x, y, FX(f->x2, -2), FY(f->y2), f->r * s, RGBA(f->col, 1 - p)); break;
	case FX_BLAST:
		circ(x, y, f->r * (0.5f + p * 0.6f) * s, RGBA(f->col, 0.55f * (1 - p)));
		circ(x, y, f->r * (0.3f + p * 0.4f) * s, RGBA(0xFFF0A0, 0.6f * (1 - p)));
		break;
	case FX_SPARK:
		line(x - 3 * s, y, x + 3 * s, y, 1 * s, RGBA(0xFFFFFF, 1 - p));
		line(x, y - 3 * s, x, y + 3 * s, 1 * s, RGBA(0xFFFFFF, 1 - p));
		break;
	case FX_FREEZE: circ(x, y, f->r * (0.3f + p * 0.7f) * s, RGBA(f->col, 0.4f * (1 - p))); ring(x, y, f->r * (0.3f + p * 0.7f) * s, 1.5f * s, RGBA(0xFFFFFF, 0.7f * (1 - p)), 20); break;
	case FX_FIRE: ring(x, y, f->r * (0.5f + p * 0.5f) * s, 4 * s, RGBA(f->col, 0.8f * (1 - p)), 22); break;
	case FX_SHOCK: ring(x, y, f->r * p * s, 2 * s, RGBA(f->col, 0.8f * (1 - p)), 22); break;
	case FX_COIN:
		circ(x, y - p * 14 * s, 3.5f * s, RGBA(C_GOLD, 1 - p));
		circ(x - 1 * s, y - p * 14 * s - 1 * s, 1.2f * s, RGBA(0xFFFFFF, 0.8f * (1 - p)));
		break;
	}
}

static void rangeCircle(float x, float y, float r, bool ok) {
	u32 c = ok ? 0xFFFFFF : C_RED;
	circ(FX(x, 0), FY(y), r * FS, RGBA(c, 0.12f));
	ring(FX(x, 0), FY(y), r * FS, 1.2f, RGBA(c, 0.6f), 40);
}

static void drawField(float s, float ox, float oy, bool top) {
	FS = s; FOX = ox; FOY = oy; FTOP = top;
	drawGround();
	drawPath();
	// Reichweite der Auswahl
	if (sel >= 0 && G.tw[sel].alive) {
		TStats st;
		td_stats(&G, &G.tw[sel], &st);
		if (st.range < 900) rangeCircle(G.tw[sel].x, G.tw[sel].y, st.range, true);
	}
	for (int i = 0; i < MAX_TW; i++) if (G.tw[i].alive) drawTower(&G.tw[i], 1, false);
	nAliveEn = td_enemies_alive(&G);
	for (int i = 0; i < MAX_EN; i++) if (G.en[i].alive && G.en[i].type < E_KOENIG) drawEnemy(&G.en[i]);
	for (int i = 0; i < MAX_EN; i++) if (G.en[i].alive && G.en[i].type >= E_KOENIG) drawEnemy(&G.en[i]);
	for (int i = 0; i < MAX_PR; i++) if (G.pr[i].alive) drawProj(&G.pr[i]);
	for (int i = 0; i < MAX_FX; i++) if (G.fx[i].alive) drawFx(&G.fx[i], i);
	// Honig
	for (int i = 0; i < MAX_JAR; i++) {
		Jar *j = &G.jar[i];
		if (!j->alive) continue;
		float bob = sinf(T * 4 + i) * 1.5f;
		float x = FX(j->x, -2), y = FY(j->y) + bob * s;
		circ(x, y, 9 * s, RGBA(C_GOLD, 0.2f + 0.1f * sinf(T * 6)));
		rrect(x - 4 * s, y - 4 * s, 8 * s, 9 * s, 2 * s, C(0xF0A020));
		rect(x - 4.5f * s, y - 6 * s, 9 * s, 2.5f * s, C(0xC8D0E0));
		circ(x - 1.5f * s, y - 1 * s, 1.2f * s, RGBA(0xFFFFFF, 0.6f));
	}
	// Platzier-Geist
	if (placeType >= 0) {
		Tower gt = {0};
		gt.type = placeType; gt.x = ghostX; gt.y = ghostY; gt.ang = -1.57f; gt.alive = 1;
		TStats st;
		td_stats(&G, &gt, &st);
		bool ok = td_can_place(&G, placeType, ghostX, ghostY) && G.coins >= td_cost(&G, placeType);
		rangeCircle(ghostX, ghostY, st.range < 900 ? st.range : 20, ok);
		drawTower(&gt, 0.75f, true);
	}
}

// ============================================================ Oben
static void hudPill(float x, float y, float w, u32 col) {
	rrect(x, y, w, 20, 10, RGBA(0x0A0E22, 0.65f));
	(void)col;
}
static void coinIcon(float x, float y, float r) {
	circ(x, y, r, C(0xC89A2A));
	circ(x, y, r * 0.8f, C(C_GOLD));
	circ(x - r * 0.3f, y - r * 0.3f, r * 0.25f, RGBA(0xFFFFFF, 0.7f));
}
static void heartIcon(float x, float y, float r) {
	circ(x - r * 0.45f, y - r * 0.2f, r * 0.55f, C(C_RED));
	circ(x + r * 0.45f, y - r * 0.2f, r * 0.55f, C(C_RED));
	tri(x - r * 0.98f, y, x + r * 0.98f, y, x, y + r * 0.95f, C(C_RED));
}

static void statsLine(const TStats *s, int type, char *b, int n) {
	switch (s->kind) {
	case K_FARM: snprintf(b, n, "%d Münzen pro Welle", (int)(s->income * (1 + s->jarBonus))); break;
	case K_BUFF: snprintf(b, n, "Tempo +%.0f%%  Reichweite %.0f", s->buffSpeed * 100, s->range); break;
	case K_FREEZE: snprintf(b, n, "Frost %.1f s  alle %.1f s  Reichw. %.0f", s->freeze, s->interval, s->range); break;
	case K_SLIME: snprintf(b, n, "Bremst %.0f%%  Säure %.0f  Reichw. %.0f", s->slow * 100, s->acid, s->range); break;
	case K_CHAIN: snprintf(b, n, "Schaden %.0f  Sprünge %d  %.1f/s", s->dmg, s->jumps, 1 / s->interval); break;
	case K_BOMB: snprintf(b, n, "Schaden %.0f  Radius %.0f  %.1f/s", s->dmg, s->blast, 1 / s->interval); break;
	case K_SNIPER: snprintf(b, n, "Schaden %.0f  %.1f/s  ganze Karte", s->dmg, 1 / s->interval); break;
	default: snprintf(b, n, "Schaden %.0f  Durchschlag %d  %.1f/s", s->dmg, s->pierce, 1 / s->interval); break;
	}
	(void)type;
}

static void towerCard(int type, const Tower *t) {
	float x = 226, y = 26, w = 168, h = 186;
	rrect(x + eyeSign * -2 * slider3d, y, w, h, 8, RGBA(0x0C1030, 0.88f));
	float ox = x + eyeSign * -2 * slider3d;
	rect(ox + 8, y, w - 16, 2, C(TDEF[type].color));
	textFit(ox + 8, y + 5, 0.55f, C(C_TEXT), 0, w - 16, TDEF[type].name);
	Tower tmp = {0};
	if (!t) { tmp.type = type; tmp.alive = 1; t = &tmp; }
	TStats s;
	td_stats(&G, t, &s);
	char b[96];
	statsLine(&s, type, b, sizeof b);
	textFit(ox + 8, y + 26, 0.36f, C(C_CYAN), 0, w - 16, b);
	char fl[64] = "";
	if (s.camo) strcat(fl, "Sieht Geister  ");
	if (s.ap || s.strip || s.stripAura) strcat(fl, "Knackt Panzer");
	if (fl[0]) textFit(ox + 8, y + 39, 0.34f, C(C_GREEN), 0, w - 16, fl);
	if (t == &tmp) {
		textWrap(ox + 8, y + 54, 0.38f, C(C_DIM), w - 16, TDEF[type].desc);
		textf(ox + 8, y + h - 20, 0.42f, C(C_GOLD), 0, "Preis: %d", td_cost(&G, type));
		return;
	}
	float yy = y + 54;
	for (int p = 0; p < 2; p++) {
		int tier = p ? t->b : t->a;
		textf(ox + 8, yy, 0.38f, C(p ? C_GOLD : C_CYAN), 0, "%s", TDEF[type].pathName[p]);
		yy += 13;
		for (int k = 0; k < 4; k++) {
			bool have = k < tier;
			bool locked = !have && k >= 2 && (p ? t->a : t->b) > 2;
			u32 c = have ? C_TEXT : locked ? 0x4A5280 : C_DIM;
			textFit(ox + 14, yy, 0.34f, C(c), 0, w - 22, have ? TDEF[type].upName[p][k] : (k == tier ? TDEF[type].upDesc[p][k] : TDEF[type].upName[p][k]));
			if (have) circ(ox + 10, yy + 5, 2, C(p ? C_GOLD : C_CYAN));
			yy += 11.5f;
		}
		yy += 4;
	}
	char pb[48];
	snprintf(pb, sizeof pb, "Plopps: %.0f", t->pops);
	text(ox + 8, y + h - 16, 0.34f, C(C_DIM), 0, pb);
}

static void drawTitleTop(void) {
	rectG(0, 0, TW, SH, C(0x1A1050), C(0x3A1A5A));
	for (int i = 0; i < 9; i++) {
		float x = fmodf(i * 53 + T * (20 + i * 4), TW + 60) - 30;
		float y = 200 - fabsf(sinf(T * 2 + i)) * 30 - (i % 3) * 8;
		Enemy e = {0};
		e.alive = 1; e.type = i % 5; e.x = x; e.y = y; e.wob = T * 6 + i; e.dirx = 1; e.mods = i == 7 ? M_GEIST : i == 4 ? M_PANZER : 0;
		FS = 1.6f; FOX = 0; FOY = 0; FTOP = true;
		e.x = x / 1.6f; e.y = y / 1.6f;
		drawEnemy(&e);
	}
	float d = -3 * eyeSign * slider3d;
	const char *t1 = "GLIBBER", *t2 = "ALARM";
	textS(200 + d, 36 + sinf(T * 2) * 3, 1.5f, C(C_SLIME), C2D_AlignCenter, t1);
	textS(200 + d, 82 + sinf(T * 2 + 1) * 3, 1.5f, C(C_GOLD), C2D_AlignCenter, t2);
	text(200, 134, 0.5f, C(C_TEXT), C2D_AlignCenter, "Tower Defense gegen Glibber");
}

static void drawMapsTop(void) {
	drawField(1, 0, 0, true);
	rrect(10, 10, 220, 52, 8, RGBA(0x0C1030, 0.85f));
	text(18, 13, 0.62f, C(C_TEXT), 0, MAPS[selMap].name);
	text(18, 38, 0.4f, C(C_DIM), 0, MAPS[selMap].desc);
	for (int d = 0; d < NDIFF; d++) {
		float x = 250 + d * 50;
		rrect(x, 14, 44, 44, 6, RGBA(0x0C1030, 0.85f));
		text(x + 22, 16, 0.32f, C(C_DIM), C2D_AlignCenter, DIFF[d].name);
		if (SD.won[selMap][d]) {
			circ(x + 22, 40, 9, C(d == 0 ? 0xC87A3A : d == 1 ? 0xC8D0E0 : C_GOLD));
			line(x + 17, 40, x + 21, 44, 2, C(0x10142A));
			line(x + 21, 44, x + 28, 35, 2, C(0x10142A));
		} else textf(x + 22, 33, 0.42f, C(C_DIM), C2D_AlignCenter, "%d", SD.best[selMap][d]);
	}
}

static void drawGameTop(void) {
	drawField(1, 0, 0, true);
	float dd = -2 * eyeSign * slider3d;
	// HUD
	hudPill(4 + dd, 4, 150, 0);
	coinIcon(16 + dd, 14, 6);
	char b[48];
	snprintf(b, sizeof b, "%.0f", floor(G.coins));
	text(26 + dd, 6, 0.5f, C(C_GOLD), 0, b);
	heartIcon(98 + dd, 14, 6);
	textf(108 + dd, 6, 0.5f, C(G.lives < 20 ? C_RED : C_TEXT), 0, "%d", G.lives);
	int fw = DIFF[G.diff].finalWave;
	hudPill(158 + dd, 4, 84, 0);
	if (G.endless) textf(200 + dd, 6, 0.45f, C(C_TEXT), C2D_AlignCenter, "Welle %d", G.wave);
	else textf(200 + dd, 6, 0.45f, C(C_TEXT), C2D_AlignCenter, "Welle %d/%d", G.wave, fw);
	if (overlay == OV_PAUSE) { hudPill(TW - 70 + dd, 4, 66, 0); text(TW - 37 + dd, 6, 0.45f, C(C_GOLD), C2D_AlignCenter, "Pause"); }
	else if (ff) { hudPill(TW - 50 + dd, 4, 46, 0); text(TW - 27 + dd, 6, 0.45f, C(C_CYAN), C2D_AlignCenter, ">>"); }
	// Boss-Leiste
	int boss = -1;
	for (int i = 0; i < MAX_EN; i++) if (G.en[i].alive && G.en[i].type >= E_KOENIG && (boss < 0 || G.en[i].type > G.en[boss].type)) boss = i;
	if (boss >= 0) {
		Enemy *e = &G.en[boss];
		float p = e->hp / (EDEF[e->type].hp * (G.cur.hpMul > 0 ? G.cur.hpMul : 1));
		rrect(100 + dd, 28, 200, 16, 6, RGBA(0x0A0E22, 0.75f));
		rect(104 + dd, 37, 192, 4, C(0x301018));
		rect(104 + dd, 37, 192 * p, 4, C(C_RED));
		textFit(200 + dd, 26, 0.3f, C(C_TEXT), C2D_AlignCenter, 190, EDEF[e->type].name);
	}
	// Karte
	if (sel >= 0 && G.tw[sel].alive) towerCard(G.tw[sel].type, &G.tw[sel]);
	else if (placeType >= 0) towerCard(placeType, NULL);
	// Banner
	if (bannerT > 0) {
		float a = fminf(1, fminf(bannerT * 3, (2.6f - bannerT) * 5));
		float y = 92;
		rrect(60 + dd * 1.5f, y, 280, 50, 10, RGBA(0x0C1030, 0.85f * a));
		textFit(200 + dd * 1.5f, y + 4, 0.7f, RGBA(C_GOLD, a), C2D_AlignCenter, 260, bannerText);
		textFit(200 + dd * 1.5f, y + 30, 0.4f, RGBA(C_TEXT, a), C2D_AlignCenter, 260, bannerSub);
	}
	if (overlay == OV_WIN || overlay == OV_LOSE) {
		rect(0, 0, TW, SH, RGBA(0x000000, 0.45f));
		textS(200 + dd * 2, 80, 1.1f, C(overlay == OV_WIN ? C_GOLD : C_RED), C2D_AlignCenter, overlay == OV_WIN ? "Gewonnen!" : "Durchgebrochen!");
		textf(200, 120, 0.55f, C(C_TEXT), C2D_AlignCenter, "Welle %d  ·  %s  ·  %s", G.wave, MAPS[G.map].name, DIFF[G.diff].name);
	}
}

// ============================================================ Unten
static void button(float x, float y, float w, float h, u32 col, bool on, float sz, const char *label) {
	u32 base = on ? col : C_GRAY;
	rrect(x, y + 2, w, h, 6, C(darken(base, 0.5f)));
	rrect(x, y, w, h, 6, C(base));
	rect(x + 5, y + 2, w - 10, h * 0.4f, RGBA(0xFFFFFF, on ? 0.18f : 0.05f));
	float lum = (((base >> 16) & 255) * 0.3f + ((base >> 8) & 255) * 0.59f + (base & 255) * 0.11f) / 255;
	u32 tc = !on ? C_DIM : lum > 0.45f ? 0x10142A : C_TEXT;
	textFit(x + w / 2, y + (h - FONT_H * sz) / 2 + 1, sz, C(tc), C2D_AlignCenter, w - 8, label);
}

#define SLOT_W 46.0f
#define STRIP_W 262.0f
static void drawStrip(void) {
	rectG(0, 192, BW, 48, C(0x1A2250), C(0x0E1230));
	rect(0, 192, BW, 1, C(0x34407E));
	float maxS = NTOWER * SLOT_W - STRIP_W + 4;
	if (stripScroll < 0) stripScroll = 0;
	if (stripScroll > maxS) stripScroll = maxS;
	for (int k = 0; k < NTOWER; k++) {
		float x = 3 + k * SLOT_W - stripScroll;
		if (x + SLOT_W < 0 || x > STRIP_W) continue;
		int c = td_cost(&G, k);
		bool can = G.coins >= c;
		bool on = placeType == k;
		panel(x, 195, SLOT_W - 4, 43, 6, on ? 0x2E3A7A : C_PANEL, on ? C_CYAN : can ? 0x3A4680 : 0x262C58);
		Tower t = {0};
		t.type = k; t.alive = 1; t.ang = 1.2f;
		FS = 1.05f; FOX = x + (SLOT_W - 4) / 2 - 0; FOY = 210; FTOP = false;
		t.x = 0; t.y = 0;
		drawTower(&t, can ? 1 : 0.45f, true);
		char b[16];
		snprintf(b, sizeof b, "%d", c);
		textFit(x + (SLOT_W - 4) / 2, 224, 0.34f, C(can ? C_GOLD : 0x6A6A8A), C2D_AlignCenter, SLOT_W - 8, b);
		addHit(fmaxf(0, x), 193, SLOT_W, 47, A_SLOT, k);
	}
	// Verdecken rechts + Play-Knopf
	rectG(STRIP_W, 192, BW - STRIP_W, 48, C(0x1A2250), C(0x0E1230));
	bool can = !G.waveActive && !G.lost && overlay == OV_NONE;
	float bx = STRIP_W + 4, by = 196, bw = BW - STRIP_W - 8, bh = 40;
	if (!G.waveActive) {
		button(bx, by, bw, bh, C_GREEN, can, 0.4f, "");
		tri(bx + bw / 2 - 7, by + 9, bx + bw / 2 - 7, by + 31, bx + bw / 2 + 10, by + 20, C(can ? 0x10142A : C_DIM));
	} else {
		button(bx, by, bw, bh, ff ? C_CYAN : C_PANEL2, true, 0.4f, "");
		u32 ic = ff ? 0x10142A : C_TEXT;
		tri(bx + 10, by + 11, bx + 10, by + 29, bx + 24, by + 20, C(ic));
		tri(bx + 24, by + 11, bx + 24, by + 29, bx + 38, by + 20, C(ic));
	}
	addHit(STRIP_W, 192, BW - STRIP_W, 48, A_PLAY, 0);
}

static const char *TGNAME[NTARGET] = {"Erster", "Letzter", "Stärkster", "Nächster"};

static void drawUpgradePanel(void) {
	if (sel < 0 || !G.tw[sel].alive) return;
	Tower *t = &G.tw[sel];
	float y0 = 240 - 112 * panelAnim;
	rect(0, y0, BW, 112, RGBA(0x0C1030, 0.95f));
	rect(0, y0, BW, 2, C(TDEF[t->type].color));
	addHit(0, y0, BW, 112, A_NONE, 0);
	textFit(8, y0 + 4, 0.48f, C(C_TEXT), 0, 180, TDEF[t->type].name);
	button(BW - 30, y0 + 4, 24, 20, C_PANEL2, true, 0.45f, "X");
	addHit(BW - 36, y0, 36, 28, A_CLOSE, 0);
	for (int p = 0; p < 2; p++) {
		float x = 6 + p * 157, y = y0 + 26, w = 151;
		int tier = p ? t->b : t->a;
		u32 pc = p ? C_GOLD : C_CYAN;
		panel(x, y, w, 52, 6, C_PANEL, darken(pc, 0.45f));
		text(x + 6, y + 2, 0.34f, C(pc), 0, TDEF[t->type].pathName[p]);
		for (int k = 0; k < 4; k++) {
			float px2 = x + w - 44 + k * 10;
			rrect(px2, y + 5, 8, 6, 2, C(k < tier ? pc : 0x2A3166));
		}
		if (tier >= 4) {
			text(x + w / 2, y + 22, 0.48f, C(C_GREEN), C2D_AlignCenter, "MAX");
			continue;
		}
		bool can = td_can_upgrade(&G, sel, p);
		textFit(x + 6, y + 15, 0.4f, C(can ? C_TEXT : 0x4A5280), 0, w - 12, TDEF[t->type].upName[p][tier]);
		if (!can) {
			text(x + 6, y + 32, 0.34f, C(0x6A72A0), 0, "Gesperrt (anderer Pfad)");
			continue;
		}
		int c = td_up_cost(&G, sel, p);
		char b[24];
		snprintf(b, sizeof b, "%d", c);
		bool afford = G.coins >= c;
		button(x + 6, y + 29, w - 12, 19, pc, afford, 0.4f, b);
		addHit(x, y, w, 52, A_UPG, p);
	}
	char sb[32];
	snprintf(sb, sizeof sb, "Verkaufen %d", td_sell_value(&G, sel));
	button(6, y0 + 84, 120, 22, C_RED, true, 0.4f, sb);
	addHit(6, y0 + 82, 120, 26, A_SELL, 0);
	int k = TDEF[t->type].kind;
	if (k != K_FARM && k != K_BUFF && k != K_FREEZE) {
		char tb[32];
		snprintf(tb, sizeof tb, "Ziel: %s", TGNAME[t->target]);
		button(132, y0 + 84, 120, 22, C_PANEL2, true, 0.4f, tb);
		addHit(132, y0 + 82, 120, 26, A_TARGET, 0);
	}
	textf(312, y0 + 88, 0.34f, C(C_DIM), C2D_AlignRight, "%.0f", t->pops);
}

static void drawOverlayBottom(void) {
	if (overlay == OV_NONE) return;
	nHits = 0;
	rect(0, 0, BW, SH, RGBA(0x04060F, 0.75f));
	const char *title = "";
	const char *opts[5] = {0};
	int n = 0;
	u32 col = C_CYAN;
	switch (overlay) {
	case OV_PAUSE: title = "Pause"; opts[0] = "Weiter"; opts[1] = SD.sound ? "Ton: an" : "Ton: aus"; opts[2] = SD.stereo ? "3D: an" : "3D: aus";
		opts[3] = SD.autoStart ? "Auto-Start: an" : "Auto-Start: aus"; opts[4] = "Hauptmenü"; n = 5; break;
	case OV_WIN: title = "Gewonnen!"; opts[0] = "Endlos weiterspielen"; opts[1] = "Hauptmenü"; n = 2; col = C_GOLD; break;
	case OV_LOSE: title = "Verloren"; opts[0] = "Nochmal"; opts[1] = "Hauptmenü"; n = 2; col = C_RED; break;
	case OV_CONFIRM_MENU: title = "Zum Hauptmenü?"; opts[0] = "Ja (Stand der letzten Welle)"; opts[1] = "Abbrechen"; n = 2; break;
	}
	float h = 40 + n * 34;
	float y = (SH - h) / 2;
	panel(30, y, 260, h, 10, C_PANEL, col);
	text(160, y + 6, 0.6f, C(col), C2D_AlignCenter, title);
	for (int i = 0; i < n; i++) {
		button(46, y + 36 + i * 34, 228, 28, i == 0 ? col : C_PANEL2, true, 0.44f, opts[i]);
		addHit(40, y + 34 + i * 34, 240, 32, A_OV, i);
	}
}

static void drawGameBottom(void) {
	nHits = 0;
	drawField(BS, 0, 0, false);
	addHit(0, 0, BW, 192, A_FIELD, 0);
	// Info-Pillen
	rrect(3, 3, 118, 18, 9, RGBA(0x0A0E22, 0.7f));
	coinIcon(13, 12, 5);
	char b[32];
	snprintf(b, sizeof b, "%.0f", floor(G.coins));
	text(21, 4, 0.42f, C(C_GOLD), 0, b);
	heartIcon(80, 12, 5);
	textf(88, 4, 0.42f, C(G.lives < 20 ? C_RED : C_TEXT), 0, "%d", G.lives);
	rrect(BW - 30, 3, 27, 18, 9, RGBA(0x0A0E22, 0.7f));
	rect(BW - 21, 7, 3, 10, C(C_TEXT));
	rect(BW - 15, 7, 3, 10, C(C_TEXT));
	addHit(BW - 40, 0, 40, 28, A_PAUSE, 0);
	if (!G.waveActive && G.wave == 0 && placeType < 0 && sel < 0)
		textS(160, 160, 0.42f, C(C_TEXT), C2D_AlignCenter, "Ziehe einen Turm aufs Feld, dann Start");
	drawStrip();
	if (panelAnim > 0.01f) drawUpgradePanel();
	if (placeType >= 0 && !placeDrag) textS(160, 172, 0.38f, C(C_CYAN), C2D_AlignCenter, "Tippe aufs Feld zum Platzieren");
	drawOverlayBottom();
}

static void drawTitleBottom(void) {
	nHits = 0;
	rectG(0, 0, BW, SH, C(0x141A40), C(0x0A0E22));
	const char *lbl[4] = {"Spielen", "Fortsetzen", "Hilfe", "Ton"};
	for (int i = 0; i < 4; i++) {
		bool on = i != 1 || SD.hasResume;
		char b[48];
		if (i == 1 && SD.hasResume) snprintf(b, sizeof b, "Fortsetzen: %s, Welle %d", MAPS[SD.map].name, (int)SD.wave + 1);
		else if (i == 3) snprintf(b, sizeof b, "Ton: %s", SD.sound ? "an" : "aus");
		else snprintf(b, sizeof b, "%s", lbl[i]);
		button(40, 28 + i * 48, 240, 38, i == 0 ? C_GREEN : i == 1 ? C_GOLD : C_PANEL2, on, 0.5f, b);
		if (on) addHit(40, 26 + i * 48, 240, 44, A_MENU, i);
	}
	text(160, 222, 0.34f, C(0x5A6290), C2D_AlignCenter, "3DS Sammlung · START: Beenden");
}

static void drawMapsBottom(void) {
	nHits = 0;
	rectG(0, 0, BW, SH, C(0x141A40), C(0x0A0E22));
	text(160, 4, 0.45f, C(C_TEXT), C2D_AlignCenter, "Karte wählen");
	for (int m = 0; m < NMAP; m++) {
		float x = 6 + (m % 2) * 156, y = 24 + (m / 2) * 86;
		bool on = m == selMap;
		panel(x, y, 152, 82, 8, C_PANEL, on ? C_CYAN : 0x2A3166);
		int keep = G.map;
		G.map = m;
		float s = 0.33f;
		FS = s; FOX = x + 76 - FW * s / 2; FOY = y + 4; FTOP = false;
		rect(FOX, FOY, FW * s, FH * s, C(MAPS[m].ground1));
		const MapDef *md = &MAPS[m];
		for (int i = 0; i + 1 < md->n; i++) line(FX(md->pts[i][0], 0), FY(md->pts[i][1]), FX(md->pts[i + 1][0], 0), FY(md->pts[i + 1][1]), 5, C(md->path1));
		G.map = keep;
		rect(x + 2, y + 62, 148, 18, RGBA(0x0C1030, 0.85f));
		textFit(x + 76, y + 63, 0.38f, C(C_TEXT), C2D_AlignCenter, 140, md->name);
		for (int d = 0; d < NDIFF; d++)
			if (SD.won[m][d]) circ(x + 136 - (2 - d) * 10, y + 10, 4, C(d == 0 ? 0xC87A3A : d == 1 ? 0xC8D0E0 : C_GOLD));
		addHit(x, y, 152, 82, A_MAP, m);
	}
	button(6, 202, 50, 32, C_PANEL2, true, 0.45f, "<");
	addHit(0, 198, 60, 42, A_BACK, 0);
	static const u32 dc[3] = {C_GREEN, C_GOLD, C_RED};
	for (int d = 0; d < NDIFF; d++) {
		char b[32];
		snprintf(b, sizeof b, "%s (%d)", DIFF[d].name, DIFF[d].finalWave);
		button(62 + d * 86, 202, 82, 32, dc[d], true, 0.4f, b);
		addHit(60 + d * 86, 198, 86, 42, A_DIFF, d);
	}
}

static const char *HELP =
	"ZIEL\n"
	"Halte die Glibber auf, bevor sie deine\n"
	"Basis erreichen. Jeder Glibber kostet\n"
	"Leben (große Glibber viele!).\n\n"
	"TÜRME\n"
	"Ziehe einen Turm aus der Leiste aufs\n"
	"Feld oder tippe ihn an und dann aufs\n"
	"Feld. Tippe einen Turm an für Upgrades.\n"
	"Jeder Turm hat 2 Pfade: einer bis Stufe\n"
	"4, der andere nur bis Stufe 2.\n\n"
	"GLIBBER\n"
	"Glibber zerplatzen in kleinere Glibber.\n"
	"Geister-Glibber (durchsichtig) sehen\n"
	"nur Eule (Nachtsicht), Pinguin und\n"
	"Türme beim Dachs (Lauscher).\n"
	"Panzer-Glibber brauchen Bomben, Säure,\n"
	"Blitze oder Upgrades, die Panzer\n"
	"knacken. Rosa Ring: wächst nach!\n\n"
	"HONIG\n"
	"Bienen erzeugen Honiggläser. Tippe sie\n"
	"an, sonst werden sie bald eingesammelt.\n\n"
	"TASTEN\n"
	"A: Welle starten / Vorspulen\n"
	"B: Auswahl aufheben  L/R: Leiste\n"
	"START: Pause\n\n"
	"Gespeichert wird nach jeder Welle.";

static void drawHelpBottom(void) {
	nHits = 0;
	rectG(0, 0, BW, SH, C(0x141A40), C(0x0A0E22));
	int lines = 1;
	for (const char *c = HELP; *c; c++) lines += *c == '\n';
	float maxS = lines * 0.4f * FONT_H - 190;
	if (helpScroll < 0) helpScroll = 0;
	if (helpScroll > maxS) helpScroll = maxS;
	text(10, 34 - helpScroll, 0.4f, C(C_TEXT), 0, HELP);
	rect(0, 0, BW, 30, C(0x141A40));
	button(6, 4, 70, 22, C_PANEL2, true, 0.4f, "< Zurück");
	addHit(0, 0, 90, 30, A_BACK, 0);
	text(200, 6, 0.48f, C(C_TEXT), C2D_AlignCenter, "Hilfe");
}

// ============================================================ Spielablauf
static void startGame(int map, int diff) {
	td_new(&G, map, diff);
	makeDeco(map);
	sel = -1; placeType = -1; ff = 0; overlay = OV_NONE; endShown = false;
	state = ST_GAME;
	snapshot();
	snap.hasResume = 1;
	commitSnapshot();
	saveWrite();
	banner(MAPS[map].name, "Platziere Türme und starte die erste Welle!");
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
		char a[32], b[64] = "";
		snprintf(a, sizeof a, "Welle %d", G.wave);
		for (int i = 0; i < G.cur.ng; i++)
			if (G.cur.g[i].type >= E_KOENIG) snprintf(b, sizeof b, "Achtung: %s!", EDEF[G.cur.g[i].type].name);
		if (!b[0]) {
			if (G.wave == 20) snprintf(b, sizeof b, "Neu: Geister-Glibber! Du brauchst Sicht.");
			else if (G.wave == 26) snprintf(b, sizeof b, "Neu: Panzer-Glibber!");
			else if (G.wave == 33) snprintf(b, sizeof b, "Neu: Nachwachsende Glibber!");
		}
		if (b[0] || G.wave % 10 == 0 || G.wave == 1) banner(a, b[0] ? b : "Los geht's!");
	}
}

static float popCd, shootCd;
static void finishWaveEvents(void) {
	popCd -= 1 / 60.0f;
	shootCd -= 1 / 60.0f;
	if (G.evPop && popCd <= 0) { play(S_POP, 0.85f + frnd() * 0.4f, fminf(0.5f, 0.2f + G.evPop * 0.05f)); popCd = 0.05f; }
	if (G.evBossPop) play(S_BOOM, 0.7f, 0.7f);
	if (G.evBoom) play(S_BOOM, 1 + frnd() * 0.2f, 0.4f);
	if (G.evZap) play(S_ZAP, 1 + frnd() * 0.2f, 0.35f);
	if (G.evFreeze) play(S_FREEZE, 1, 0.35f);
	if (G.evShoot && !G.evPop && shootCd <= 0) { play(S_SHOOT, 0.9f + frnd() * 0.3f, 0.18f); shootCd = 0.08f; }
	if (G.evCoin) play(S_COIN, 1, 0.45f);
	if (G.evLeak) play(S_LEAK, 1, 0.5f);
	if (G.evWaveEnd) {
		recordBest();
		char b[48];
		snprintf(b, sizeof b, "+%d Münzen Bonus", G.waveEndBonus);
		if (G.won && !endShown) {
			endShown = true;
			overlay = OV_WIN;
			SD.won[G.map][G.diff] = 1;
			SD.hasResume = 0;
			play(S_WIN, 1, 0.7f);
			saveWrite();
		} else {
			if (G.wave % 5 == 0) banner("Welle geschafft!", b);
			snapshot();
			commitSnapshot();
			saveWrite();
			autoT = 1.5f;
		}
	}
	if (G.lost && overlay != OV_LOSE) {
		overlay = OV_LOSE;
		recordBest();
		SD.hasResume = 0;
		saveWrite();
		play(S_LOSE, 1, 0.7f);
	}
	G.evPop = G.evShoot = G.evBoom = G.evZap = G.evFreeze = G.evCoin = G.evLeak = G.evBossPop = G.evWaveEnd = G.evBlock = 0;
}

// ============================================================ Eingabe
static bool tDown, tDrag, tConsumed;
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
		if (i == 0) { saveWrite(); state = ST_TITLE; overlay = OV_NONE; }
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
	switch (h->act) {
	case A_PLAY:
		if (!G.waveActive) startWave();
		else { ff = !ff; play(S_CLICK, ff ? 1.3f : 1, 0.4f); }
		break;
	case A_PAUSE: overlay = OV_PAUSE; play(S_CLICK, 1, 0.4f); break;
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
		if (h->p == 0) { state = ST_MAPS; td_new(&G, selMap, 1); makeDeco(selMap); }
		if (h->p == 1 && SD.hasResume) {
			restoreResume();
			makeDeco(G.map);
			snapshot();
			sel = -1; placeType = -1; ff = 0; overlay = OV_NONE; endShown = false;
			state = ST_GAME;
			banner("Willkommen zurück!", "Weiter geht's mit der nächsten Welle.");
		}
		if (h->p == 2) { state = ST_HELP; helpScroll = 0; }
		if (h->p == 3) { SD.sound = !SD.sound; saveWrite(); }
		break;
	case A_MAP: selMap = h->p; td_new(&G, selMap, 1); makeDeco(selMap); play(S_CLICK, 1.1f, 0.4f); break;
	case A_DIFF: play(S_WAVE, 1, 0.5f); startGame(selMap, h->p); break;
	case A_BACK: state = ST_TITLE; play(S_CLICK, 0.9f, 0.4f); break;
	}
}

static void handleInput(u32 kDown, u32 kHeld, u32 kUp, float dt) {
	touchPosition tp;
	hidTouchRead(&tp);
	bool inGame = state == ST_GAME && overlay == OV_NONE;
	if (kDown & KEY_TOUCH) {
		tDown = true; tDrag = false; tConsumed = false;
		tx0 = tx = tp.px; ty0 = ty = tp.py; tLastX = tx;
		tSlot = -1;
		Hit *h = hitAt(tx, ty);
		if (inGame && h && h->act == A_SLOT) tSlot = h->p;
		if (state == ST_HELP) tDrag = true;
	} else if (tDown && (kHeld & KEY_TOUCH)) {
		float nx = tp.px, ny = tp.py;
		if (state == ST_HELP) helpScroll -= ny - ty;
		if (inGame && tSlot >= 0 && !tDrag) {
			float dx = nx - tx0, dy = ny - ty0;
			if (dy < -8 && -dy > fabsf(dx) * 0.5f) {
				// nach oben aus der Leiste aufs Feld gezogen
				tDrag = true;
				placeType = tSlot;
				placeDrag = true;
				sel = -1;
			} else if (fabsf(dx) > 10 && fabsf(dx) > fabsf(dy)) {
				tDrag = true;
				tSlot = -2; // Leiste scrollen
			}
		}
		if (inGame && tSlot == -2) {
			stripScroll -= nx - tLastX;
			if (dt > 0) stripV = -(nx - tLastX) / dt;
		}
		if (inGame && placeDrag && placeType >= 0) {
			ghostX = nx / BS;
			ghostY = (ny - 26) / BS; // etwas ueber dem Finger
		}
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
	// Tasten
	if (kDown & KEY_START) {
		if (state == ST_GAME) overlay = overlay == OV_PAUSE ? OV_NONE : (overlay == OV_NONE ? OV_PAUSE : overlay);
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
	if (state == ST_MAPS && (kDown & KEY_B)) state = ST_TITLE;
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
	srand((unsigned)osGetTime());
	td_init_tables();
	saveRead();
	gfxSet3D(SD.stereo);
	sndInit();
	td_new(&G, 0, 1);
	makeDeco(0);
	aptHook(&hookCookie, onAptHook, NULL);

	u64 last = osGetTime();
	float acc = 0;
	while (aptMainLoop() && !quitReq) {
		hidScanInput();
		u32 kDown = hidKeysDown(), kHeld = hidKeysHeld(), kUp = hidKeysUp();
		u64 now = osGetTime();
		float dt = now >= last ? (now - last) / 1000.0f : 0;
		last = now;
		if (dt > 0.1f) dt = 0.1f; // Standby/HOME: Spiel pausiert einfach
		T += dt;
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
			finishWaveEvents();
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
			else drawGameTop();
		}
		eyeSign = 0;
		C2D_TargetClear(tgtBot, C(0x000000));
		C2D_SceneBegin(tgtBot);
		if (state == ST_TITLE) drawTitleBottom();
		else if (state == ST_MAPS) drawMapsBottom();
		else if (state == ST_HELP) drawHelpBottom();
		else drawGameBottom();
		C3D_FrameEnd(0);
	}
	saveWrite();
	sndExit();
	C2D_TextBufDelete(tbuf);
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
