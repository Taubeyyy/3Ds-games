// Star Forge (Sternenschmiede) - 3DS Frontend (citro2d)
#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "game.h"
#include "expedition.h"

#define TW 400
#define BW 320
#define SH 240
#define FONT_H 30.0f
#define PI 3.14159265f

#define C_BG1 0x0A0E22
#define C_BG2 0x121838
#define C_PANEL 0x182046
#define C_PANEL2 0x232D5E
#define C_LINE 0x34407E
#define C_TEXT 0xEEF1FF
#define C_DIM 0x8E97C6
#define C_CYAN 0x4AD8FF
#define C_GOLD 0xFFD25A
#define C_GREEN 0x46E89A
#define C_RED 0xFF5A6E
#define C_PURPLE 0xB070FF
#define C_PINK 0xFF6AD5
#define C_GRAY 0x3A4270

#define SAVE_DIR "sdmc:/3ds/sternenschmiede"
#define SAVE_PATH SAVE_DIR "/save.dat"
#define SAVE_TMP SAVE_DIR "/save.tmp"
#define SAVE_BAK SAVE_DIR "/save.bak"

static Game G;
u32 __stacksize__ = 256 * 1024; // groesserer Stack (Standard: 32 KB)
static C3D_RenderTarget *tgtTop, *tgtTopR, *tgtBot;
static C2D_TextBuf tbuf;
static float T;           // laufende Zeit
static float eyeSign;     // -1 links, +1 rechts
static float slider3d;

// ============================================================ Farben
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

// ============================================================ Zeichnen
static inline float px(float x, float depth) { return x + eyeSign * depth * slider3d; }
static void rect(float x, float y, float w, float h, u32 c) { C2D_DrawRectSolid(x, y, 0, w, h, c); }
static void rectG(float x, float y, float w, float h, u32 ct, u32 cb) { C2D_DrawRectangle(x, y, 0, w, h, ct, ct, cb, cb); }
static void rectH(float x, float y, float w, float h, u32 cl, u32 cr) { C2D_DrawRectangle(x, y, 0, w, h, cl, cr, cl, cr); }
static void circ(float x, float y, float r, u32 c) { if (r > 0.3f) C2D_DrawCircleSolid(x, y, 0, r, c); }
static void circG(float x, float y, float r, u32 hex, float a) {
	C2D_DrawCircle(x, y, 0, r, RGBA(lighten(hex, 0.55f), a), RGBA(hex, a), RGBA(hex, a), RGBA(darken(hex, 0.5f), a));
}
static void tri(float x0, float y0, u32 c0, float x1, float y1, u32 c1, float x2, float y2, u32 c2) {
	C2D_DrawTriangle(x0, y0, c0, x1, y1, c1, x2, y2, c2, 0);
}
static void line(float x0, float y0, float x1, float y1, float th, u32 c) { C2D_DrawLine(x0, y0, c, x1, y1, c, th, 0); }
static void lineG(float x0, float y0, u32 c0, float x1, float y1, u32 c1, float th) { C2D_DrawLine(x0, y0, c0, x1, y1, c1, th, 0); }

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
// Panel mit Rand
static void panel(float x, float y, float w, float h, float r, u32 fill, u32 border) {
	rrect(x, y, w, h, r, C(border));
	rrect(x + 1, y + 1, w - 2, h - 2, r - 1, C(fill));
}

static void ellipseLine(float cx, float cy, float a, float b, u32 hex, float al, float th, int seg, int half, float depthBack, float depthFront) {
	for (int k = 0; k < seg; k++) {
		float t0 = 2 * PI * k / seg, t1 = 2 * PI * (k + 1) / seg;
		float s = sinf((t0 + t1) * 0.5f);
		if (half == 1 && s >= 0) continue;
		if (half == 2 && s < 0) continue;
		float d = s < 0 ? depthBack : depthFront;
		line(px(cx + a * cosf(t0), d), cy + b * sinf(t0), px(cx + a * cosf(t1), d), cy + b * sinf(t1), th, RGBA(hex, al * (s < 0 ? 0.55f : 1)));
	}
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
// Text, der bei Bedarf verkleinert wird, damit er in maxw passt
static void textFit(float x, float y, float sz, u32 col, int align, float maxw, const char *s) {
	C2D_Text t = mk(s);
	float w, h;
	C2D_TextGetDimensions(&t, sz, sz, &w, &h);
	float s2 = sz;
	if (w > maxw && w > 0) s2 = sz * maxw / w;
	float dy = (sz - s2) * FONT_H * 0.5f;
	C2D_DrawText(&t, C2D_WithColor | align, x, y + dy, 0, s2, s2, col);
}
static void textWrap(float x, float y, float sz, u32 col, float w, const char *s) {
	C2D_Text t = mk(s);
	C2D_DrawText(&t, C2D_WithColor | C2D_WordWrap, x, y, 0, sz, sz, col, w);
}
static void num(double v, char *b) { fmt_num(v, b, 32); }
static void textS(float x, float y, float sz, u32 col, int align, const char *s) {
	C2D_Text t = mk(s);
	C2D_DrawText(&t, C2D_WithColor | align, x + 1, y + 1, 0, sz, sz, RGBA(0x000000, 0.7f));
	C2D_DrawText(&t, C2D_WithColor | align, x, y, 0, sz, sz, col);
}

// ============================================================ Partikel & Effekte
typedef struct { float x, y, vx, vy, life, max, size; u32 col; u8 top; } Particle;
#define NPART 220
static Particle parts[NPART];
static int partNext;
static void spawnP(int top, float x, float y, float vx, float vy, float life, float size, u32 col) {
	Particle *p = &parts[partNext];
	partNext = (partNext + 1) % NPART;
	p->x = x; p->y = y; p->vx = vx; p->vy = vy; p->life = p->max = life; p->size = size; p->col = col; p->top = top;
}
static float frnd(void) { return (rand() & 0xFFFF) / 65535.0f; }
static void burst(int top, float x, float y, int n, float speed, u32 col) {
	for (int i = 0; i < n; i++) {
		float a = frnd() * 2 * PI, s = speed * (0.3f + frnd() * 0.7f);
		spawnP(top, x, y, cosf(a) * s, sinf(a) * s, 0.4f + frnd() * 0.5f, 1 + frnd() * 2.2f, frnd() < 0.3f ? 0xFFFFFF : col);
	}
}
static void updParts(float dt) {
	for (int i = 0; i < NPART; i++) {
		Particle *p = &parts[i];
		if (p->life <= 0) continue;
		p->life -= dt;
		p->x += p->vx * dt; p->y += p->vy * dt;
		p->vx *= 1 - 2.2f * dt; p->vy *= 1 - 2.2f * dt;
		p->vy += 30 * dt;
	}
}
static void drawParts(int top) {
	for (int i = 0; i < NPART; i++) {
		Particle *p = &parts[i];
		if (p->life <= 0 || p->top != top) continue;
		float a = p->life / p->max;
		float x = top ? px(p->x, -1) : p->x;
		circ(x, p->y, p->size * (0.5f + a * 0.5f), RGBA(p->col, a));
	}
}

typedef struct { float x, y, life; char s[40]; u32 col; } Floater;
#define NFLOAT 24
static Floater floats[NFLOAT];
static int floatNext;
static void spawnFloat(float x, float y, u32 col, const char *s) {
	Floater *f = &floats[floatNext];
	floatNext = (floatNext + 1) % NFLOAT;
	f->x = x; f->y = y; f->life = 1.0f; f->col = col;
	snprintf(f->s, sizeof f->s, "%s", s);
}

typedef struct { char title[48], msg[96]; u32 col; } Toast;
#define NTOAST 8
static Toast toasts[NTOAST];
static int toastHead, toastTail;
static float toastT;
static void toast(u32 col, const char *title, const char *fmt, ...) {
	int n = (toastHead + 1) % NTOAST;
	if (n == toastTail) toastTail = (toastTail + 1) % NTOAST;
	Toast *t = &toasts[toastHead];
	snprintf(t->title, sizeof t->title, "%s", title);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(t->msg, sizeof t->msg, fmt, ap);
	va_end(ap);
	t->col = col;
	toastHead = n;
}

static float crystalKick, coreKick, novaFlash, bangFlash, saveFlash;
static float meteorX0, meteorY0, meteorX1, meteorY1;

// ============================================================ Sound
typedef struct { s16 *data; u32 n; } Sample;
enum { SND_TAP, SND_BUY, SND_ERR, SND_ACH, SND_METEOR, SND_NOVA, SND_TAB, NSND };
static Sample snd[NSND];
static ndspWaveBuf wbuf[8];
static bool sndOk;
static int sndCh;
#define SRATE 22050

static void genSnd(int id, float dur, int kind) {
	u32 n = (u32)(dur * SRATE);
	s16 *d = (s16 *)linearAlloc(n * sizeof(s16));
	if (!d) return;
	float ph = 0, lp = 0;
	for (u32 i = 0; i < n; i++) {
		float t = (float)i / SRATE, p = t / dur, v = 0, f = 440;
		switch (kind) {
		case SND_TAP: f = 1500 - 500 * p; v = sinf(ph) * expf(-t * 45) + 0.3f * sinf(ph * 2.01f) * expf(-t * 70); break;
		case SND_TAB: f = 900; v = sinf(ph) * expf(-t * 60) * 0.6f; break;
		case SND_BUY: f = t < 0.055f ? 880 : 1320; v = (sinf(ph) + 0.25f * sinf(ph * 3)) * expf(-fmodf(t, 0.055f) * 30) * 0.8f; break;
		case SND_ERR: f = 160; v = (sinf(ph) > 0 ? 0.5f : -0.5f) * (1 - p); break;
		case SND_ACH: {
			static const float nt[5] = {660, 830, 990, 1320, 1320};
			int k = (int)(t / 0.09f);
			if (k > 4) k = 4;
			f = nt[k];
			v = (sinf(ph) + 0.3f * sinf(ph * 2)) * expf(-fmodf(t, 0.09f) * 12) * (1 - p * 0.6f) * 0.8f;
			break;
		}
		case SND_METEOR: f = 500 + 2200 * p; v = sinf(ph) * (1 - p) * 0.6f + ((rand() & 255) / 128.0f - 1) * 0.15f * (1 - p); break;
		case SND_NOVA: {
			float nz = (rand() & 1023) / 512.0f - 1;
			lp += (nz - lp) * (0.08f + 0.3f * (1 - p));
			f = 55 + 30 * (1 - p);
			v = (lp * 1.6f + sinf(ph) * 0.7f) * expf(-t * 2.2f) * fminf(1, t * 40);
			break;
		}
		}
		ph += 2 * PI * f / SRATE;
		if (ph > 2 * PI * 64) ph -= 2 * PI * 64;
		if (v > 1) v = 1;
		if (v < -1) v = -1;
		d[i] = (s16)(v * 26000);
	}
	DSP_FlushDataCache(d, n * sizeof(s16));
	snd[id].data = d;
	snd[id].n = n;
}

static void sndInit(void) {
	if (R_FAILED(ndspInit())) return;
	sndOk = true;
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	for (int ch = 0; ch < 8; ch++) {
		ndspChnReset(ch);
		ndspChnSetInterp(ch, NDSP_INTERP_LINEAR);
		ndspChnSetRate(ch, SRATE);
		ndspChnSetFormat(ch, NDSP_FORMAT_MONO_PCM16);
	}
	genSnd(SND_TAP, 0.08f, SND_TAP);
	genSnd(SND_TAB, 0.05f, SND_TAB);
	genSnd(SND_BUY, 0.13f, SND_BUY);
	genSnd(SND_ERR, 0.09f, SND_ERR);
	genSnd(SND_ACH, 0.55f, SND_ACH);
	genSnd(SND_METEOR, 0.35f, SND_METEOR);
	genSnd(SND_NOVA, 1.6f, SND_NOVA);
}
static void play(int id, float pitch, float vol) {
	if (!sndOk || !G.s.sound || !snd[id].data) return;
	int ch = sndCh;
	sndCh = (sndCh + 1) % 8;
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

// ============================================================ Speichern
static bool saveGame(void) {
	mkdir("sdmc:/3ds", 0777);
	mkdir(SAVE_DIR, 0777);
	G.s.lastSave = osGetTime();
	FILE *f = fopen(SAVE_TMP, "wb");
	if (!f) return false;
	size_t n = fwrite(&G.s, 1, sizeof(Save), f);
	fclose(f);
	if (n != sizeof(Save)) return false;
	remove(SAVE_BAK);
	rename(SAVE_PATH, SAVE_BAK);
	rename(SAVE_TMP, SAVE_PATH);
	saveFlash = 1.5f;
	return true;
}
static bool loadFile(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) return false;
	static Save s;
	memset(&s, 0, sizeof s);
	size_t n = fread(&s, 1, sizeof s, f);
	fclose(f);
	if (n != sizeof s || s.magic != SAVE_MAGIC || s.version > SAVE_VERSION) return false;
	G.s = s;
	return true;
}
static bool loadGame(void) { return loadFile(SAVE_PATH) || loadFile(SAVE_BAK); }

static aptHookCookie hookCookie;
static void onAptHook(APT_HookType hook, void *param) {
	(void)param;
	if (hook == APTHOOK_ONSUSPEND || hook == APTHOOK_ONSLEEP || hook == APTHOOK_ONEXIT) saveGame();
}

// ============================================================ UI-Zustand
enum { P_CORE, P_BUILD, P_UPG, P_HANGAR, P_STARS, P_MORE, P_ACH, P_STATS, P_BANG, P_CHAL, P_OPT, P_HELP, P_CONTRACTS, P_ARTS, NPAGE };
static int tab, page;
static float scrollY[NPAGE], scrollV[NPAGE], scrollMax[NPAGE];
static int upgSel = -1;

enum { A_NONE, A_TAB, A_CRYSTAL, A_METEOR, A_BUYMODE, A_GEN, A_UPGSEL, A_UPGBUY, A_UPGALL, A_NOVA, A_PERK,
	A_MENU, A_BACK, A_BANG, A_SPERK, A_CHAL, A_OPT, A_YES, A_NO, A_ACHSEL, A_RADAR, A_EXPRET, A_HSTART, A_HSEC, A_SHIPUP, A_CLAIM };
static void drawToast(void);
static void drawModal(void);
typedef struct { float x, y, w, h; int act, p; } Hit;
#define NHIT 160
static Hit hits[NHIT];
static int nHits;
static void addHit(float x, float y, float w, float h, int act, int p) {
	if (nHits >= NHIT) return;
	hits[nHits++] = (Hit){x, y, w, h, act, p};
}
// mit Clipping auf den sichtbaren Listenbereich
static void addHitC(float x, float y, float w, float h, int act, int p, float y0, float y1) {
	if (y < y0) { h -= y0 - y; y = y0; }
	if (y + h > y1) h = y1 - y;
	if (h > 2) addHit(x, y, w, h, act, p);
}
static Hit *hitAt(float x, float y) {
	for (int i = nHits - 1; i >= 0; i--) {
		Hit *h = &hits[i];
		if (x >= h->x && x < h->x + h->w && y >= h->y && y < h->y + h->h) return h;
	}
	return NULL;
}

enum { MD_NONE, MD_NOVA, MD_BANG, MD_CHAL, MD_CHALABORT, MD_RESET, MD_RESET2, MD_INFO, MD_QUIT };
static int modal, modalP;
static char modalTitle[48], modalMsg[200];
static void openModal(int kind, int p, const char *title, const char *fmt, ...) {
	modal = kind;
	modalP = p;
	snprintf(modalTitle, sizeof modalTitle, "%s", title);
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(modalMsg, sizeof modalMsg, fmt, ap);
	va_end(ap);
}
static bool quitReq;


// ============================================================ Spielaktionen
static void doClick(float x, float y) {
	double v = click(&G, true);
	char b[32], s[40];
	num(v, b);
	snprintf(s, sizeof s, "+%s", b);
	if (G.s.chalActive == CH_NOCLICK) snprintf(s, sizeof s, "Stille...");
	spawnFloat(x + (frnd() - 0.5f) * 16, y - 10, G.s.buffT[BUFF_CLICKRUSH] > 0 ? C_GOLD : C_TEXT, s);
	float heat = G.comboHeat / (G.comboMax > 0 ? G.comboMax : 1);
	u32 col = hexMix(C_CYAN, C_PINK, heat);
	burst(0, x, y, 7, 120, col);
	crystalKick = 1;
	coreKick = 1;
	play(SND_TAP, 0.9f + heat * 0.6f + frnd() * 0.08f, 0.55f);
}

static void buyGen(int i, float x, float y) {
	int mode = G.s.buyMode;
	int n = mode == 0 ? 1 : mode == 1 ? 10 : mode == 2 ? 100 : 0;
	int got = gen_buy(&G, i, n);
	if (got > 0) {
		char s[24];
		snprintf(s, sizeof s, "+%d", got);
		spawnFloat(x, y, GEN[i].color, s);
		burst(0, x, y + 8, 8, 90, GEN[i].color);
		play(SND_BUY, 1.0f + 0.02f * (i % 8), 0.5f);
	} else play(SND_ERR, 1, 0.4f);
}

static void doNova(void) {
	double gain = nova_gain(&G);
	nova_do(&G);
	novaFlash = 2.0f;
	upgSel = -1;
	char b[32];
	num(gain, b);
	toast(C_GOLD, "SUPERNOVA!", "+%s Stardust", b);
	play(SND_NOVA, 1, 0.9f);
	burst(1, 200, 136, 60, 260, C_GOLD);
}
static void doBang(void) {
	double gain = bang_gain(&G);
	bang_do(&G);
	bangFlash = 3.0f;
	upgSel = -1;
	char b[32];
	num(gain, b);
	toast(C_PURPLE, "BIG BANG!", "+%s Singularities. Ein neues Universum entsteht.", b);
	play(SND_NOVA, 0.7f, 1.0f);
	burst(1, 200, 136, 90, 320, C_PURPLE);
}

static void meteorSpawn(void) {
	int side = rand() & 1;
	meteorX0 = side ? -20 : BW + 20;
	meteorX1 = side ? BW + 20 : -20;
	meteorY0 = 50 + frnd() * 120;
	meteorY1 = 50 + frnd() * 120;
}
static void meteorPos(float *x, float *y) {
	float p = 1 - G.meteorLife / 13.0f;
	*x = meteorX0 + (meteorX1 - meteorX0) * p;
	*y = meteorY0 + (meteorY1 - meteorY0) * p + sinf(p * PI * 3) * 14;
}
static void meteorEffect(int e, double gain, float x, float y) {
	char b[32];
	if (e == 0) {
		num(gain, b);
		toast(C_GOLD, "Energiefund!", "+%s Energie", b);
		char s[40];
		snprintf(s, sizeof s, "+%s", b);
		spawnFloat(x, y, C_GOLD, s);
	} else if (e == 1) toast(C_PINK, "Sternenfieber!", "Produktion x7 für %.0f s", G.s.buffT[BUFF_FRENZY]);
	else if (e == 2) toast(C_CYAN, "Klickrausch!", "Klicks x77 für %.0f s", G.s.buffT[BUFF_CLICKRUSH]);
	burst(0, x, y, 30, 200, C_GOLD);
	play(SND_METEOR, 1, 0.7f);
}

// ============================================================ Sternenfeld (oben)
typedef struct { float x, y, z, ph, sz; } Star;
#define NSTAR 150
static Star stars[NSTAR];
static void initStars(void) {
	for (int i = 0; i < NSTAR; i++) {
		stars[i].x = frnd() * (TW + 40) - 20;
		stars[i].y = frnd() * SH;
		stars[i].z = frnd();
		stars[i].ph = frnd() * 6.28f;
		stars[i].sz = 0.6f + frnd() * frnd() * 2.2f;
	}
}

static u32 coreColor(void) {
	if (G.s.bangs > 0) return hexMix(0xC08AFF, 0xFFFFFF, fminf(1, G.s.bangs / 20.0f));
	return hexMix(0xFF9A3A, 0xBFEFFF, fminf(1, G.s.novas / 12.0f));
}

static void genIcon(int i, float cx, float cy, float r, float al) {
	u32 h = GEN[i].color;
	circ(cx, cy, r * 1.3f, RGBA(h, 0.16f * al));
	int v = i % 7;
	switch (v) {
	case 0: // Sonne
		for (int k = 0; k < 8; k++) {
			float a = k * PI / 4 + T * 0.5f;
			line(cx + cosf(a) * r * 0.5f, cy + sinf(a) * r * 0.5f, cx + cosf(a) * r * 1.1f, cy + sinf(a) * r * 1.1f, 1.6f, RGBA(h, al));
		}
		circG(cx, cy, r * 0.62f, h, al);
		break;
	case 1: // Ringplanet
		ellipseLine(cx, cy, r * 1.15f, r * 0.36f, lighten(h, 0.3f), al, 1.6f, 14, 1, 0, 0);
		circG(cx, cy, r * 0.65f, h, al);
		ellipseLine(cx, cy, r * 1.15f, r * 0.36f, lighten(h, 0.3f), al, 1.6f, 14, 2, 0, 0);
		break;
	case 2: { // Kristall
		float w = r * 0.75f, hh = r * 1.05f;
		tri(cx, cy - hh, RGBA(lighten(h, 0.6f), al), cx - w, cy, RGBA(h, al), cx, cy, RGBA(lighten(h, 0.3f), al));
		tri(cx, cy - hh, RGBA(lighten(h, 0.3f), al), cx, cy, RGBA(h, al), cx + w, cy, RGBA(darken(h, 0.2f), al));
		tri(cx - w, cy, RGBA(h, al), cx, cy + hh, RGBA(darken(h, 0.5f), al), cx, cy, RGBA(darken(h, 0.1f), al));
		tri(cx, cy, RGBA(darken(h, 0.2f), al), cx, cy + hh, RGBA(darken(h, 0.6f), al), cx + w, cy, RGBA(darken(h, 0.4f), al));
		break;
	}
	case 3: // Kern mit Monden
		circG(cx, cy, r * 0.55f, h, al);
		for (int k = 0; k < 3; k++) {
			float a = T * 1.5f + k * 2.094f;
			circ(cx + cosf(a) * r * 0.95f, cy + sinf(a) * r * 0.45f, r * 0.17f, RGBA(lighten(h, 0.5f), al));
		}
		break;
	case 4: // Sechseck
		for (int k = 0; k < 6; k++) {
			float a0 = k * PI / 3 + PI / 6, a1 = (k + 1) * PI / 3 + PI / 6;
			float sh = 0.35f + 0.5f * (0.5f + 0.5f * sinf(a0 + 1.2f));
			tri(cx, cy, RGBA(lighten(h, 0.4f), al), cx + cosf(a0) * r * 0.9f, cy + sinf(a0) * r * 0.9f, RGBA(darken(h, 1 - sh), al),
				cx + cosf(a1) * r * 0.9f, cy + sinf(a1) * r * 0.9f, RGBA(darken(h, 1 - sh), al));
		}
		break;
	case 5: { // Stern
		float a = T * 0.4f;
		for (int k = 0; k < 2; k++) {
			float o = a + k * PI / 3;
			tri(cx + cosf(o) * r, cy + sinf(o) * r, RGBA(h, al), cx + cosf(o + 2.094f) * r, cy + sinf(o + 2.094f) * r, RGBA(h, al),
				cx + cosf(o + 4.189f) * r, cy + sinf(o + 4.189f) * r, RGBA(lighten(h, 0.4f), al));
		}
		circ(cx, cy, r * 0.35f, RGBA(0xFFFFFF, al * 0.9f));
		break;
	}
	case 6: // Schwarzes Loch
		ellipseLine(cx, cy, r * 1.1f, r * 0.42f, h, al, 2.2f, 16, 1, 0, 0);
		circ(cx, cy, r * 0.62f, RGBA(lighten(h, 0.2f), al));
		circ(cx, cy, r * 0.48f, RGBA(0x05030A, al));
		ellipseLine(cx, cy, r * 1.1f, r * 0.42f, lighten(h, 0.3f), al, 2.2f, 16, 2, 0, 0);
		break;
	}
	if (i >= 7) {
		float tw = 0.5f + 0.5f * sinf(T * 3 + i);
		circ(cx + r * 0.75f, cy - r * 0.75f, 1.2f + tw, RGBA(0xFFFFFF, al * (0.5f + tw * 0.5f)));
	}
}

static void drawOrbitBodies(int i, float cx, float cy, float a, float b, int front) {
	double n = G.s.gens[i];
	int cnt = (int)fmin(10, 1 + floor(sqrt(n)));
	float w = 0.32f / (1 + i * 0.13f);
	u32 h = GEN[i].color;
	for (int k = 0; k < cnt; k++) {
		float th = T * w + k * 2 * PI / cnt + i * 1.7f;
		float s = sinf(th);
		if ((s >= 0) != front) continue;
		float d = 3 - 4 * s;
		float x = px(cx + a * cosf(th), d), y = cy + b * s;
		float r = 2.0f + 0.9f * s + (i % 4) * 0.25f;
		circ(x, y, r * 2.4f, RGBA(h, 0.18f));
		circ(x, y, r, RGBA(lighten(h, 0.3f), 1));
	}
}

static void drawTop(void) {
	rectG(0, 0, TW, SH, C(0x060818), C(0x170F3A));
	// Nebel
	for (int k = 0; k < 4; k++) {
		float nx = 80 + k * 90 + sinf(T * 0.05f + k * 2) * 30, ny = 70 + (k % 2) * 90 + cosf(T * 0.04f + k) * 20;
		u32 nc = k % 2 ? C_PURPLE : 0x3A6AFF;
		if (G.s.bangs > 0 && k == 2) nc = C_PINK;
		circ(px(nx, 10), ny, 120, RGBA(nc, 0.045f));
		circ(px(nx + 20, 10), ny + 10, 70, RGBA(nc, 0.05f));
	}
	// Sterne
	for (int i = 0; i < NSTAR; i++) {
		Star *s = &stars[i];
		float x = fmodf(s->x - T * (2 + s->z * 6) + 10000 * (TW + 40), TW + 40) - 20;
		float tw = 0.45f + 0.55f * (0.5f + 0.5f * sinf(T * (1 + s->z * 2) + s->ph));
		float d = 9 - s->z * 5;
		u32 col = s->ph > 5.2f ? 0xFFE8B0 : (s->ph < 0.8f ? 0xB8D0FF : 0xFFFFFF);
		x = px(x, d);
		if (s->sz > 2.0f) {
			line(x - 3, s->y, x + 3, s->y, 0.8f, RGBA(col, tw * 0.5f));
			line(x, s->y - 3, x, s->y + 3, 0.8f, RGBA(col, tw * 0.5f));
		}
		rect(x - s->sz * 0.5f, s->y - s->sz * 0.5f, s->sz, s->sz, RGBA(col, tw));
	}

	float cx = 200, cy = 138;
	// Umlaufbahnen (hinten)
	for (int i = 0; i < NGEN; i++) {
		if (G.s.gens[i] <= 0) continue;
		float a = 46 + i * 10.5f, b = a * 0.3f;
		ellipseLine(cx, cy, a, b, GEN[i].color, 0.22f, 1, 36, 1, 3, 1);
		drawOrbitBodies(i, cx, cy, a, b, 0);
	}
	// Kern
	u32 cc = coreColor();
	float r = 16 + fminf(22, (float)log10(1 + G.s.lifeEarned) * 0.9f);
	r *= 1 + 0.06f * coreKick + 0.03f * sinf(T * 1.7f);
	float cxx = px(cx, 2);
	for (int k = 0; k < 12; k++) {
		float an = k * PI / 6 + T * 0.15f;
		float len = r * (1.5f + 0.35f * sinf(T * 2 + k * 1.3f));
		lineG(cxx, cy, RGBA(cc, 0.35f), cxx + cosf(an) * len, cy + sinf(an) * len, RGBA(cc, 0), 2.2f);
	}
	circ(cxx, cy, r * 2.6f, RGBA(cc, 0.05f));
	circ(cxx, cy, r * 1.9f, RGBA(cc, 0.09f));
	circ(cxx, cy, r * 1.4f, RGBA(cc, 0.18f));
	C2D_DrawCircle(cxx, cy, 0, r, C(0xFFFFFF), C(lighten(cc, 0.5f)), C(lighten(cc, 0.3f)), C(cc));
	circ(cxx - r * 0.3f, cy - r * 0.3f, r * 0.35f, RGBA(0xFFFFFF, 0.6f));
	// Umlaufbahnen (vorne)
	for (int i = 0; i < NGEN; i++) {
		if (G.s.gens[i] <= 0) continue;
		float a = 46 + i * 10.5f, b = a * 0.3f;
		ellipseLine(cx, cy, a, b, GEN[i].color, 0.22f, 1, 36, 2, 3, 1);
		drawOrbitBodies(i, cx, cy, a, b, 1);
	}
	// Meteor oben
	if (G.meteorLife > 0) {
		float p = 1 - G.meteorLife / 13.0f;
		float mx = px(420 - p * 460, 1), my = 70 + sinf(p * 5) * 25;
		for (int k = 0; k < 10; k++) circ(mx + k * 4.5f, my - k * 1.2f, 4 - k * 0.35f, RGBA(C_GOLD, 0.5f - k * 0.045f));
		circ(mx, my, 4.5f, C(0xFFF3C0));
	}
	drawParts(1);

	// HUD
	char b[32], b2[32];
	num(floor(G.s.energy), b);
	textFit(px(200, -1), 4, 0.95f, C(C_TEXT), C2D_AlignCenter, 300, b);
	num(G.eps, b2);
	u32 ec = G.s.buffT[BUFF_FRENZY] > 0 ? C_PINK : C_CYAN;
	textf(px(200, -1), 34, 0.5f, C(ec), C2D_AlignCenter, "+%s Energie/s", b2);
	if (G.s.dustTotal > 0) {
		num(G.s.dust, b);
		tri(px(12, -1), 8, C(C_GOLD), px(18, -1), 20, C(C_GOLD), px(6, -1), 20, C(C_GOLD));
		tri(px(12, -1), 24, C(C_GOLD), px(18, -1), 12, C(C_GOLD), px(6, -1), 12, C(C_GOLD));
		text(px(24, -1), 7, 0.45f, C(C_GOLD), 0, b);
	}
	if (G.s.singTotal > 0) {
		num(G.s.sing, b);
		circ(px(TW - 14, -1), 15, 6, C(C_PURPLE));
		circ(px(TW - 14, -1), 15, 3, C(0x100820));
		text(px(TW - 24, -1), 7, 0.45f, C(C_PURPLE), C2D_AlignRight, b);
	}

	// Statusleiste
	rectG(0, 216, TW, 24, RGBA(0x0A0E22, 0.0f), RGBA(0x0A0E22, 0.85f));
	char st[96];
	u32 stc = C_DIM;
	if (G.s.chalActive >= 0) {
		const ChalDef *c = &CHAL[G.s.chalActive];
		float pr = chal_progress(&G);
		if (G.s.chalActive == CH_RUSH) {
			char tb[24];
			fmt_time(fmax(0, 900 - G.s.chalTime), tb, sizeof tb);
			snprintf(st, sizeof st, "Prüfung %s: %.0f%% (%s)", c->name, pr * 100, tb);
		} else snprintf(st, sizeof st, "Prüfung %s: %.0f%%", c->name, pr * 100);
		stc = C_RED;
		rect(8, 236, 384 * pr, 2, C(C_RED));
	} else if (bang_ready(&G)) {
		num(bang_gain(&G), b);
		snprintf(st, sizeof st, "Big Bang möglich: +%s Singularities", b);
		stc = C_PURPLE;
	} else if (nova_gain(&G) >= 1) {
		num(nova_gain(&G), b);
		snprintf(st, sizeof st, "Supernova bereit: +%s Stardust", b);
		stc = C_GOLD;
	} else {
		int nx = -1;
		for (int i = 0; i < NGEN; i++) if (G.s.gens[i] == 0 && !gen_locked(&G, i)) { nx = i; break; }
		if (G.s.clicksLife < 10) snprintf(st, sizeof st, "Tippe unten auf den Kristall!");
		else if (nx >= 0 && (nx == 0 || G.s.gens[nx - 1] > 0)) {
			num(gen_cost(&G, nx, 1), b);
			snprintf(st, sizeof st, "Nächstes Gebäude: %s (%s)", GEN[nx].name, b);
		} else if (G.s.novas < 1) {
			num(1e8, b);
			snprintf(st, sizeof st, "Erste Supernova ab %s Energie", b);
		} else snprintf(st, sizeof st, "Lauf: %.0f min", G.s.runTime / 60);
	}
	textFit(px(8, -0.5f), 220, 0.42f, C(stc), 0, 270, st);
	// Buffs
	float bx = TW - 8;
	if (G.s.buffT[BUFF_CLICKRUSH] > 0) {
		textf(px(bx, -0.5f), 220, 0.42f, C(C_CYAN), C2D_AlignRight, "Klick x77 %.0fs", G.s.buffT[BUFF_CLICKRUSH]);
		bx -= 96;
	}
	if (G.s.buffT[BUFF_FRENZY] > 0) textf(px(bx, -0.5f), 220, 0.42f, C(C_PINK), C2D_AlignRight, "Fieber x7 %.0fs", G.s.buffT[BUFF_FRENZY]);
	if (saveFlash > 0) text(px(TW - 8, -0.5f), 200, 0.38f, RGBA(C_GREEN, fminf(1, saveFlash)), C2D_AlignRight, "Gespeichert");

	drawToast();
	// Blitze
	if (novaFlash > 0) {
		float p = 1 - novaFlash / 2.0f;
		circ(px(cx, 2), cy, 20 + p * 420, RGBA(C_GOLD, (1 - p) * 0.35f));
		rect(0, 0, TW, SH, RGBA(0xFFFFFF, fmaxf(0, 0.9f - p * 2.5f)));
	}
	if (bangFlash > 0) {
		float p = 1 - bangFlash / 3.0f;
		circ(px(cx, 2), cy, p * 500, RGBA(C_PURPLE, (1 - p) * 0.5f));
		circ(px(cx, 2), cy, p * 300, RGBA(0xFFFFFF, (1 - p) * 0.4f));
		rect(0, 0, TW, SH, RGBA(0xFFFFFF, fmaxf(0, 1 - p * 2)));
	}
}

// ============================================================ Unten: Bausteine
static void button(float x, float y, float w, float h, u32 col, bool on, float sz, const char *label) {
	u32 base = on ? col : C_GRAY;
	rrect(x, y + 2, w, h, 6, C(darken(base, 0.55f)));
	rrect(x, y, w, h, 6, C(base));
	rect(x + 5, y + 2, w - 10, h * 0.42f, RGBA(0xFFFFFF, on ? 0.18f : 0.06f));
	textFit(x + w / 2, y + (h - FONT_H * sz) / 2 + 1, sz, C(on ? 0x10142A : C_DIM), C2D_AlignCenter, w - 8, label);
}

static void bar(float x, float y, float w, float h, float p, u32 c1, u32 c2) {
	if (p < 0) p = 0;
	if (p > 1) p = 1;
	rrect(x, y, w, h, h / 2, C(0x0C1030));
	if (p > 0.01f) {
		float fw = fmaxf(h, w * p);
		rectH(x + h / 2, y, fw - h, h, C(c1), C(hexMix(c1, c2, p)));
		circ(x + h / 2, y + h / 2, h / 2, C(c1));
		circ(x + fw - h / 2, y + h / 2, h / 2, C(hexMix(c1, c2, p)));
	}
}

static void starIcon(float x, float y, float r, u32 c) {
	tri(x, y - r, C(c), x + r * 0.87f, y + r * 0.5f, C(c), x - r * 0.87f, y + r * 0.5f, C(c));
	tri(x, y + r, C(c), x + r * 0.87f, y - r * 0.5f, C(c), x - r * 0.87f, y - r * 0.5f, C(c));
}

#define LIST_BOT 206.0f
static void maskBg(float y0, float y1) {
	rectG(0, y0, BW, y1 - y0, C(hexMix(C_BG1, C_BG2, y0 / SH)), C(hexMix(C_BG1, C_BG2, y1 / SH)));
}
static void scrollMaxSet(int pg, float contentH, float viewH) {
	scrollMax[pg] = contentH > viewH ? contentH - viewH : 0;
	if (scrollY[pg] > scrollMax[pg]) scrollY[pg] = scrollMax[pg];
}

// ============================================================ Seiten
static void pageCore(void) {
	float cx = 160, cy = 112;
	float heat = G.comboHeat / (G.comboMax > 0 ? G.comboMax : 1);
	u32 gc = hexMix(C_CYAN, C_PINK, heat);
	float s = 40 * (1 + 0.1f * crystalKick) + sinf(T * 2) * 1.2f;
	// Glanz
	for (int k = 0; k < 8; k++) {
		float an = k * PI / 4 + T * 0.3f;
		lineG(cx, cy, RGBA(gc, 0.25f + heat * 0.2f), cx + cosf(an) * s * 1.9f, cy + sinf(an) * s * 1.9f, RGBA(gc, 0), 3);
	}
	circ(cx, cy, s * 1.85f, RGBA(gc, 0.05f + heat * 0.05f));
	circ(cx, cy, s * 1.45f, RGBA(gc, 0.08f + heat * 0.06f));
	circ(cx, cy, s * 1.15f, RGBA(gc, 0.12f));
	// Funken auf Bahn (hinten)
	for (int k = 0; k < 6; k++) {
		float th = T * (1.1f + heat) + k * 1.047f;
		if (sinf(th) >= 0) continue;
		circ(cx + cosf(th) * s * 1.55f, cy + sinf(th) * s * 0.45f, 1.6f, RGBA(lighten(gc, 0.5f), 0.7f));
	}
	// Kristall
	float ox = sinf(T * 0.9f) * s * 0.28f;
	float top = cy - s * 1.2f, bot = cy + s * 1.2f, lx = cx - s * 0.8f, rx = cx + s * 0.8f, my = cy - s * 0.12f;
	u32 c0 = lighten(gc, 0.75f), c1 = lighten(gc, 0.35f), c2 = gc, c3 = darken(gc, 0.35f), c4 = darken(gc, 0.6f);
	tri(cx, top, C(c0), lx, my, C(c1), cx + ox, my, C(c0));
	tri(cx, top, C(c1), cx + ox, my, C(c0), rx, my, C(c2));
	tri(lx, my, C(c2), cx, bot, C(c4), cx + ox, my, C(c1));
	tri(cx + ox, my, C(c2), cx, bot, C(c4), rx, my, C(c3));
	line(lx, my, rx, my, 1.2f, RGBA(0xFFFFFF, 0.35f));
	line(cx, top, cx + ox, my, 1, RGBA(0xFFFFFF, 0.45f));
	float sp = 0.5f + 0.5f * sinf(T * 2.3f);
	tri(cx - s * 0.22f, cy - s * 0.55f, RGBA(0xFFFFFF, 0.25f + sp * 0.5f), cx - s * 0.1f, cy - s * 0.75f, RGBA(0xFFFFFF, 0),
		cx - s * 0.05f, cy - s * 0.4f, RGBA(0xFFFFFF, 0));
	// Funken vorne
	for (int k = 0; k < 6; k++) {
		float th = T * (1.1f + heat) + k * 1.047f;
		if (sinf(th) < 0) continue;
		circ(cx + cosf(th) * s * 1.55f, cy + sinf(th) * s * 0.45f, 2.2f, C(lighten(gc, 0.6f)));
	}
	addHit(cx - 80, cy - 70, 160, 140, A_CRYSTAL, 0);

	char b[32];
	num(G.clickVal * (1 + G.comboHeat), b);
	if (G.s.chalActive == CH_NOCLICK) text(cx, 36, 0.45f, C(C_RED), C2D_AlignCenter, "Prüfung: Klicks wirkungslos");
	else if (G.s.clicksLife < 15) text(cx, 36, 0.5f, RGBA(C_TEXT, 0.6f + 0.4f * sinf(T * 4)), C2D_AlignCenter, "Tippe auf den Kristall!");
	else textf(cx, 36, 0.45f, C(C_DIM), C2D_AlignCenter, "+%s pro Tipp", b);
	// Kombo
	float bw = 200;
	bar(cx - bw / 2, 182, bw, 9, heat, C_CYAN, C_PINK);
	textf(cx - bw / 2, 166, 0.4f, C(C_DIM), 0, "Kombo");
	textf(cx + bw / 2, 166, 0.4f, C(gc), C2D_AlignRight, "x%.1f", 1 + G.comboHeat);
	if (G.s.perk[PK_AUTOCLICK] && G.s.autoClickOn)
		textf(cx, 193, 0.36f, C(C_DIM), C2D_AlignCenter, "Geisterhand: %d Klicks/s", G.s.perk[PK_AUTOCLICK]);
}

static int nVisGens(void) {
	int n = 0;
	for (int i = 0; i < NGEN; i++) if (gen_visible(&G, i)) n = i + 1;
	return n;
}

static void pageBuild(void) {
	float y0 = 56, rowH = 48;
	int nv = nVisGens();
	int nrows = nv + (nv < NGEN ? 1 : 0);
	scrollMaxSet(P_BUILD, nrows * rowH + 4, LIST_BOT - y0);
	float sc = scrollY[P_BUILD];
	for (int i = 0; i < nrows; i++) {
		float y = y0 + i * rowH - sc;
		if (y + rowH < 30 || y > LIST_BOT) continue;
		if (i >= nv) {
			panel(6, y, 308, 44, 7, 0x10142E, 0x1C2350);
			text(48, y + 6, 0.5f, C(0x4A5280), 0, "???");
			textf(48, y + 25, 0.38f, C(0x4A5280), 0, "Kaufe zuerst %s.", GEN[i - 1].name);
			circ(25, y + 22, 13, C(0x1C2350));
			text(25, y + 13, 0.5f, C(0x4A5280), C2D_AlignCenter, "?");
			continue;
		}
		bool locked = gen_locked(&G, i);
		int mode = G.s.buyMode;
		int n = mode == 0 ? 1 : mode == 1 ? 10 : mode == 2 ? 100 : gen_max_afford(&G, i);
		if (n < 1) n = 1;
		double cost = gen_cost(&G, i, n);
		bool can = !locked && cost <= G.s.energy;
		u32 h = GEN[i].color;
		panel(6, y, 308, 44, 7, can ? 0x1D2654 : C_PANEL, can ? darken(h, 0.35f) : 0x262E60);
		rect(7, y + 8, 3, 28, C(h));
		genIcon(i, 28, y + 22, 12, locked ? 0.3f : 1);
		textFit(48, y + 4, 0.48f, C(locked ? C_DIM : C_TEXT), 0, 120, GEN[i].name);
		char b[32], b2[32], s[64];
		textf(222, y + 3, 0.6f, C(lighten(h, 0.3f)), C2D_AlignRight, "%.0f", G.s.gens[i]);
		if (locked) snprintf(s, sizeof s, "Gesperrt (Einsamkeit)");
		else {
			num(gen_unit_prod(&G, i), b);
			if (G.s.gens[i] > 0 && G.eps > 0) snprintf(s, sizeof s, "je %s/s  ·  %.0f%%", b, G.genEps[i] / G.eps * 100);
			else snprintf(s, sizeof s, "je %s/s", b);
		}
		textFit(48, y + 25, 0.38f, C(C_DIM), 0, 176, s);
		// Kaufknopf
		float bx = 230, by = y + 6;
		rrect(bx, by + 2, 78, 32, 6, C(can ? 0x1E7A50 : 0x1A1F44));
		rrect(bx, by, 78, 32, 6, C(can ? C_GREEN : 0x2A3166));
		num(cost, b2);
		textf(bx + 39, by + 1, 0.34f, C(can ? 0x0E3A24 : C_DIM), C2D_AlignCenter, "x%d", n);
		textFit(bx + 39, by + 13, 0.44f, C(can ? 0x08200F : C_DIM), C2D_AlignCenter, 72, b2);
		addHitC(6, y, 308, 44, A_GEN, i, y0, LIST_BOT);
	}
	maskBg(31, y0 - 1);
	static const char *ml[4] = {"x1", "x10", "x100", "Max"};
	text(8, 34, 0.5f, C(C_TEXT), 0, "Gebäude");
	for (int m = 0; m < 4; m++) {
		float x = 156 + m * 40;
		bool on = G.s.buyMode == m;
		rrect(x, 33, 37, 18, 5, C(on ? C_CYAN : C_PANEL2));
		text(x + 18.5f, 34.5f, 0.4f, C(on ? 0x10142A : C_DIM), C2D_AlignCenter, ml[m]);
		addHit(x, 30, 37, 24, A_BUYMODE, m);
	}
}

static int ulist[MAX_UPG], nAvail, nList;
static void buildUpgList(void) {
	nAvail = nList = 0;
	for (int i = 0; i < nUpg; i++)
		if (!upg_bought(&G, i) && upg_unlocked(&G, i)) ulist[nList++] = i;
	for (int a = 1; a < nList; a++) {
		int v = ulist[a];
		double c = upg_cost(&G, v);
		int b = a - 1;
		while (b >= 0 && upg_cost(&G, ulist[b]) > c) { ulist[b + 1] = ulist[b]; b--; }
		ulist[b + 1] = v;
	}
	nAvail = nList;
	// naechste gesperrte je Gruppe
	int added = 0;
	for (int i = 0; i < nUpg && added < 12; i++) {
		if (upg_bought(&G, i) || upg_unlocked(&G, i)) continue;
		UpgDef *u = &UPG[i];
		bool first = true;
		for (int j = 0; j < i; j++) {
			UpgDef *w = &UPG[j];
			if (w->type == u->type && w->gen == u->gen && !upg_bought(&G, j) && !upg_unlocked(&G, j)) { first = false; break; }
		}
		if (!first) continue;
		if ((u->type == UT_GEN || u->type == UT_SYNERGY) && G.s.gens[u->gen] <= 0) continue;
		if (u->type == UT_SYNERGY && G.s.gens[u->gen2] <= 0) continue;
		ulist[nList++] = i;
		added++;
	}
}

static void upgIcon(int i, float x, float y, float sz, float al) {
	UpgDef *u = &UPG[i];
	float cx = x + sz / 2, cy = y + sz / 2;
	switch (u->type) {
	case UT_GEN:
		genIcon(u->gen, cx, cy - 3, sz * 0.27f, al);
		{
			static const char *R[12] = {"I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII"};
			textf(cx, y + sz - 14, 0.34f, RGBA(C_TEXT, al), C2D_AlignCenter, "%s", R[u->tier % 12]);
		}
		break;
	case UT_SYNERGY:
		genIcon(u->gen, cx - 8, cy, sz * 0.18f, al);
		genIcon(u->gen2, cx + 8, cy, sz * 0.18f, al);
		line(cx - 4, cy, cx + 4, cy, 1.5f, RGBA(0xFFFFFF, al * 0.7f));
		break;
	case UT_GLOBAL:
		starIcon(cx, cy - 3, sz * 0.25f, C_GOLD);
		text(cx, y + sz - 15, 0.34f, RGBA(C_GOLD, al), C2D_AlignCenter, "x1,5");
		break;
	case UT_CLICK:
		tri(cx, cy - 12, RGBA(C_CYAN, al), cx - 8, cy, RGBA(lighten(C_CYAN, 0.4f), al), cx + 8, cy, RGBA(darken(C_CYAN, 0.3f), al));
		tri(cx - 8, cy, RGBA(C_CYAN, al), cx, cy + 12, RGBA(darken(C_CYAN, 0.5f), al), cx + 8, cy, RGBA(darken(C_CYAN, 0.2f), al));
		text(cx, y + sz - 15, 0.34f, RGBA(C_TEXT, al), C2D_AlignCenter, "x2");
		break;
	case UT_CLICKPCT:
		circ(cx, cy - 2, sz * 0.26f, RGBA(C_GREEN, al * 0.85f));
		text(cx, cy - 11, 0.5f, RGBA(0x0A2A18, al), C2D_AlignCenter, "%");
		break;
	case UT_METEOR:
		for (int k = 0; k < 6; k++) circ(cx + 6 - k * 3.5f, cy - 6 + k * 3.5f, 5 - k * 0.6f, RGBA(C_GOLD, al * (0.9f - k * 0.13f)));
		break;
	case UT_COMBO:
		for (int k = 0; k < 3; k++) rect(cx - 10 + k * 8, cy + 6 - k * 6, 5, 6 + k * 6, RGBA(hexMix(C_CYAN, C_PINK, k / 2.0f), al));
		break;
	}
}

static void pageUpg(void) {
	buildUpgList();
	if (upgSel >= 0 && upg_bought(&G, upgSel)) upgSel = -1;
	int affordable = 0;
	for (int k = 0; k < nAvail; k++) if (upg_cost(&G, ulist[k]) <= G.s.energy) affordable++;
	bool asc = G.s.chalActive == CH_ASCETIC;
	float y0 = 92, step = 51, sz = 46;
	int rows = (nList + 5) / 6;
	scrollMaxSet(P_UPG, rows * step + 4, LIST_BOT - y0);
	float sc = scrollY[P_UPG];
	for (int k = 0; k < nList; k++) {
		int i = ulist[k];
		float x = 7 + (k % 6) * 51, y = y0 + (k / 6) * step - sc;
		if (y + sz < 86 || y > LIST_BOT) continue;
		bool avail = k < nAvail;
		bool can = avail && !asc && upg_cost(&G, i) <= G.s.energy;
		u32 border = i == upgSel ? 0xFFFFFF : can ? C_CYAN : 0x2A3470;
		u32 fill = avail ? (can ? 0x1F2A60 : C_PANEL) : 0x0F1330;
		if (can) circ(x + sz / 2, y + sz / 2, sz * 0.62f, RGBA(C_CYAN, 0.06f + 0.04f * sinf(T * 3 + k)));
		panel(x, y, sz, sz, 7, fill, border);
		upgIcon(i, x, y, sz, avail ? 1 : 0.28f);
		if (!avail) {
			rrect(x + sz - 13, y + 3, 10, 9, 2, C(0x4A5280));
			circ(x + sz - 8, y + 4, 3, C(0x4A5280));
			circ(x + sz - 8, y + 4, 1.6f, C(fill));
		}
		addHitC(x, y, sz, sz, A_UPGSEL, i, y0 - 4, LIST_BOT);
	}
	maskBg(31, y0 - 3);
	// Infopanel
	panel(6, 32, 308, 54, 8, C_PANEL, 0x2A3470);
	if (upgSel >= 0) {
		bool unl = upg_unlocked(&G, upgSel);
		upgIcon(upgSel, 10, 36, 44, unl ? 1 : 0.4f);
		textFit(58, 35, 0.44f, C(C_TEXT), 0, 166, UPG[upgSel].name);
		char d[96];
		upg_desc(&G, upgSel, d, sizeof d);
		textWrap(58, 53, 0.36f, C(unl ? C_DIM : C_RED), 168, d);
		char b[32], s[48];
		num(upg_cost(&G, upgSel), b);
		bool can = unl && !asc && upg_cost(&G, upgSel) <= G.s.energy;
		snprintf(s, sizeof s, "%s", b);
		button(230, 36, 80, 24, C_GREEN, can, 0.44f, s);
		addHit(230, 34, 80, 28, A_UPGBUY, upgSel);
	} else {
		if (asc) textWrap(14, 38, 0.42f, C(C_RED), 210, "Prüfung Asket: keine Upgrades kaufbar.");
		else if (nAvail == 0 && G.upgCount > 0) textWrap(14, 38, 0.42f, C(C_GREEN), 210, "Alles gekauft! Neue Upgrades kommen mit mehr Gebäuden und Energie.");
		else if (nAvail == 0) textWrap(14, 38, 0.42f, C(C_DIM), 210, "Noch keine Upgrades. Kaufe Gebäude, um welche freizuschalten.");
		else {
			textf(14, 38, 0.46f, C(C_TEXT), 0, "%d verfügbar, %d bezahlbar", nAvail, affordable);
			text(14, 58, 0.38f, C(C_DIM), 0, "Tippe ein Upgrade an.");
		}
	}
	char al[24];
	snprintf(al, sizeof al, "Alle (%d)", affordable);
	button(230, 63, 80, 19, C_CYAN, affordable > 0 && !asc, 0.38f, al);
	addHit(230, 62, 80, 22, A_UPGALL, 0);

}

static double nextDustAt(void) {
	double pot = floor(pow(G.s.bangEarned / 1e8, 0.4) * dust_mult(&G));
	return 1e8 * pow((pot + 1) / dust_mult(&G), 1 / 0.4);
}

static void pageStars(void) {
	char b[32];
	float y0 = 100, rowH = 44;
	scrollMaxSet(P_STARS, NPERK * rowH + 4, LIST_BOT - y0);
	float sc = scrollY[P_STARS];
	for (int i = 0; i < NPERK; i++) {
		float y = y0 + i * rowH - sc;
		if (y + rowH < 96 || y > LIST_BOT) continue;
		const PerkDef *p = &PERK[i];
		bool av = perk_available(&G, i);
		int l = G.s.perk[i];
		bool maxed = l >= p->max;
		double c = perk_cost(&G, i);
		bool can = av && !maxed && c <= G.s.dust;
		panel(6, y, 308, 40, 7, can ? 0x2A2046 : C_PANEL, can ? 0x8A6A30 : 0x2A3166);
		textFit(12, y + 3, 0.44f, C(av ? C_TEXT : C_DIM), 0, 150, p->name);
		textf(226, y + 4, 0.36f, C(maxed ? C_GREEN : C_DIM), C2D_AlignRight, maxed ? "MAX" : "%d/%d", l, p->max);
		char d[96];
		if (!av) snprintf(d, sizeof d, "Benötigt %s Stufe %d.", PERK[p->needPerk].name, p->needLvl);
		else perk_desc(&G, i, d, sizeof d);
		textFit(12, y + 22, 0.35f, C(av ? C_DIM : 0x6A5070), 0, 218, d);
		if (!maxed) {
			num(c, b);
			button(234, y + 6, 74, 26, C_GOLD, can, 0.42f, b);
			addHitC(6, y, 308, 40, A_PERK, i, y0, LIST_BOT);
		}
	}
	maskBg(31, y0 - 2);
	{
	panel(6, 32, 308, 64, 8, 0x241A3A, 0x5A4470);
	char b2[32];
	starIcon(20, 46, 8, C_GOLD);
	num(G.s.dust, b);
	textFit(34, 35, 0.56f, C(C_GOLD), 0, 120, b);
	num(1 + 0.05 * G.s.dustTotal, b2);
	textf(34, 55, 0.36f, C(C_DIM), 0, "Bonus: Produktion x%s", b2);
	double gain = nova_gain(&G);
	num(gain, b);
	if (gain >= 1) textf(14, 72, 0.42f, C(C_TEXT), 0, "Supernova: +%s Staub", b);
	else {
		double nx = nextDustAt();
		double prog = log10(fmax(1, G.s.bangEarned)) / log10(fmax(10, nx));
		num(nx, b2);
		textf(14, 70, 0.36f, C(C_DIM), 0, "Nächster Staub bei %s", b2);
		bar(14, 85, 200, 6, (float)prog, 0x8A6A20, C_GOLD);
	}
	button(226, 40, 84, 36, 0xFF8A3A, gain >= 1, 0.46f, "SUPERNOVA");
	if (gain >= 1) addHit(226, 38, 84, 40, A_NOVA, 0);
	text(268, 79, 0.32f, C(C_DIM), C2D_AlignCenter, "setzt Lauf zurück");

	}
}

static void pageMore(void) {
	static const char *lbl[8] = {"Erfolge", "Statistik", "Big Bang", "Prüfungen", "Aufträge", "Artefakte", "Optionen", "Hilfe"};
	static const int pg[8] = {P_ACH, P_STATS, P_BANG, P_CHAL, P_CONTRACTS, P_ARTS, P_OPT, P_HELP};
	static const u32 col[8] = {C_GOLD, C_CYAN, C_PURPLE, C_RED, C_GREEN, C_GOLD, C_GREEN, C_DIM};
	for (int k = 0; k < 8; k++) {
		float x = 8 + (k % 2) * 156, y = 34 + (k / 2) * 43;
		bool locked = (k == 2 && G.s.bangs < 1 && !bang_ready(&G) && G.s.novas < 3) || (k == 3 && G.s.bangs < 1);
		panel(x, y, 148, 39, 8, locked ? 0x10142E : C_PANEL, locked ? 0x1C2350 : darken(col[k], 0.4f));
		rect(x + 10, y + 35, 128, 2, C(locked ? 0x1C2350 : col[k]));
		textFit(x + 74, y + 2, 0.48f, C(locked ? 0x4A5280 : C_TEXT), C2D_AlignCenter, 136, lbl[k]);
		char s[48] = "";
		if (k == 0) snprintf(s, sizeof s, "%d / %d", ach_count(&G), nAch);
		if (k == 1) { fmt_time(G.s.playTime, s, sizeof s); }
		if (k == 2) {
			if (locked) snprintf(s, sizeof s, "Nach 3 Supernovas");
			else if (bang_ready(&G)) snprintf(s, sizeof s, "BEREIT!");
			else { char b[32]; num(G.s.sing, b); snprintf(s, sizeof s, "%s Singularities", b); }
		}
		if (k == 3) {
			if (locked) snprintf(s, sizeof s, "Nach dem 1. Big Bang");
			else { int d = 0; for (int c = 0; c < NCHAL; c++) d += (G.s.chalDone >> c) & 1; snprintf(s, sizeof s, "%d / %d geschafft", d, NCHAL); }
		}
		if (k == 4) { int r = contracts_ready(&G); if (r) snprintf(s, sizeof s, "%d bereit zum Einlösen!", r); else snprintf(s, sizeof s, "3 aktive Aufträge"); }
		if (k == 5) snprintf(s, sizeof s, "%d / %d gefunden", art_count(&G), NART);
		if (k == 6) snprintf(s, sizeof s, "Ton, 3D, Automatik");
		if (k == 7) snprintf(s, sizeof s, "So funktioniert's");
		bool hl = (k == 2 && bang_ready(&G)) || (k == 4 && contracts_ready(&G));
		textFit(x + 74, y + 20, 0.35f, C(locked ? 0x4A5280 : (hl ? (k == 2 ? C_PURPLE : C_GREEN) : C_DIM)), C2D_AlignCenter, 136, s);
		if (!locked) addHit(x, y, 148, 39, A_MENU, pg[k]);
	}
}

static void subHeader(const char *title) {
	maskBg(31, 56);
	rrect(6, 33, 66, 20, 6, C(C_PANEL2));
	text(6 + 33, 35.5f, 0.4f, C(C_TEXT), C2D_AlignCenter, "< Zurück");
	addHit(0, 30, 80, 26, A_BACK, 0);
	textFit(196, 34, 0.5f, C(C_TEXT), C2D_AlignCenter, 230, title);
}

static void pageAch(void) {
	char t[48];
	snprintf(t, sizeof t, "Erfolge  %d/%d", ach_count(&G), nAch);
	float y0 = 58, rowH = 32;
	scrollMaxSet(P_ACH, nAch * rowH + 22, LIST_BOT - y0);
	float sc = scrollY[P_ACH];
	textf(160, y0 - sc, 0.36f, C(C_DIM), C2D_AlignCenter, "Jeder Erfolg: +%.1f%% Produktion", 1 + 0.5f * G.s.perk[PK_ACHBOOST]);
	for (int i = 0; i < nAch; i++) {
		float y = y0 + 18 + i * rowH - sc;
		if (y + rowH < 54 || y > LIST_BOT) continue;
		bool has = ach_has(&G, i);
		char nm[48], ds[96];
		ach_text(i, nm, sizeof nm, ds, sizeof ds);
		panel(6, y, 308, 29, 6, has ? 0x262448 : 0x10142E, has ? 0x6A5A30 : 0x1C2350);
		if (has) {
			circ(20, y + 14.5f, 8, C(C_GOLD));
			line(15, y + 15, 19, y + 19, 2, C(0x302000));
			line(19, y + 19, 25, y + 10, 2, C(0x302000));
		} else circ(20, y + 14.5f, 7, C(0x1C2350));
		textFit(34, y + 1, 0.4f, C(has ? C_TEXT : 0x5A6290), 0, 270, nm);
		textFit(34, y + 15, 0.33f, C(has ? C_DIM : 0x4A5280), 0, 270, ds);
	}
	subHeader(t);
}

static void statLine(float y, const char *k, const char *v) {
	text(14, y, 0.4f, C(C_DIM), 0, k);
	textFit(306, y, 0.4f, C(C_TEXT), C2D_AlignRight, 150, v);
}

static void pageStats(void) {
	float y = 58 - scrollY[P_STATS];
	char b[48];
	const float dy = 17;
	int n = 0;
#define SL(k, ...) do { snprintf(b, sizeof b, __VA_ARGS__); if (y > 40 && y < LIST_BOT) statLine(y, k, b); y += dy; n++; } while (0)
	char t[32];
	fmt_time(G.s.playTime, t, sizeof t); SL("Spielzeit", "%s", t);
	fmt_time(G.s.runTime, t, sizeof t); SL("Dieser Lauf", "%s", t);
	num(G.s.runEarned, t); SL("Energie (Lauf)", "%s", t);
	num(G.s.lifeEarned, t); SL("Energie (gesamt)", "%s", t);
	num(G.s.bestEps, t); SL("Beste Produktion", "%s/s", t);
	num(G.globalMult, t); SL("Gesamtmultiplikator", "x%s", t);
	num(G.s.clicksLife, t); SL("Klicks", "%s", t);
	num(G.clickVal, t); SL("Klickkraft", "%s", t);
	SL("Beste Kombo", "x%.1f", G.s.bestCombo);
	num(G.s.meteorsLife, t); SL("Meteore gefangen", "%s", t);
	num(G.totalGens, t); SL("Gebäude", "%s", t);
	SL("Upgrades", "%d / %d", G.upgCount, nUpg);
	SL("Erfolge", "%d / %d", ach_count(&G), nAch);
	SL("Supernovas", "%.0f", G.s.novas);
	num(G.s.dustTotal, t); SL("Stardust (Bonus)", "%s", t);
	SL("Big Bangs", "%.0f", G.s.bangs);
	num(G.s.singTotal, t); SL("Singularities", "%s", t);
	SL("Preisanstieg", "%.3f", G.costGrowth);
	SL("Offline-Effizienz", "%.0f%%", game_offline_eff(&G) * 100);
	num(G.s.ext.x.alloy, t); SL("Alloy", "%s", t);
	SL("Expeditionen", "%.0f", G.s.ext.x.expeditions);
	SL("Bester Sektor", "%d", G.s.ext.x.sectorMax);
	SL("Artefakte", "%d / %d", art_count(&G), NART);
	SL("Aufträge erfüllt", "%.0f", G.s.ext.x.contractsDone);
	num(refinery_mult(&G), t); SL("Raffinerie", "x%s", t);
	fmt_time(game_offline_cap(&G), t, sizeof t); SL("Offline-Limit", "%s", t);
#undef SL
	scrollMaxSet(P_STATS, n * dy + 6, LIST_BOT - 58);
	subHeader("Statistik");
}

static void pageBang(void) {
	char b[32];
	float y0 = 126, rowH = 42;
	scrollMaxSet(P_BANG, NSPERK * rowH + 4, LIST_BOT - y0);
	float sc = scrollY[P_BANG];
	for (int i = 0; i < NSPERK; i++) {
		float y = y0 + i * rowH - sc;
		if (y + rowH < 122 || y > LIST_BOT) continue;
		int l = G.s.sperk[i];
		bool maxed = l >= SPERK[i].max;
		double c = sperk_cost(&G, i);
		bool can = !maxed && c <= G.s.sing;
		panel(6, y, 308, 38, 7, can ? 0x261A4A : C_PANEL, can ? 0x7A4AC0 : 0x2A3166);
		textFit(12, y + 3, 0.44f, C(C_TEXT), 0, 150, SPERK[i].name);
		textf(226, y + 4, 0.36f, C(maxed ? C_GREEN : C_DIM), C2D_AlignRight, maxed ? "MAX" : "%d/%d", l, SPERK[i].max);
		char d[96];
		sperk_desc(&G, i, d, sizeof d);
		textFit(12, y + 21, 0.35f, C(C_DIM), 0, 218, d);
		if (!maxed) {
			num(c, b);
			button(234, y + 6, 74, 24, C_PURPLE, can, 0.42f, b);
			addHitC(6, y, 308, 38, A_SPERK, i, y0, LIST_BOT);
		}
	}
	maskBg(31, y0 - 2);
	{
	subHeader("Big Bang");
	panel(6, 58, 308, 64, 8, 0x1E1440, 0x5A3A90);
	char b2[32];
	circ(20, 72, 8, C(C_PURPLE));
	circ(20, 72, 4, C(0x100820));
	num(G.s.sing, b);
	textFit(34, 61, 0.54f, C(C_PURPLE), 0, 110, b);
	num(sqrt(1 + G.s.singTotal), b2);
	textf(34, 80, 0.35f, C(C_DIM), 0, "Produktion x%s, Staub x%.2f", b2, pow(1 + G.s.singTotal, 0.25));
	bool rdy = bang_ready(&G);
	if (rdy) {
		num(bang_gain(&G), b);
		textf(14, 98, 0.42f, C(C_TEXT), 0, "Big Bang: +%s Singularities", b);
	} else {
		num(bang_need(&G), b);
		textf(14, 96, 0.35f, C(C_DIM), 0, "Benötigt %s Energie seit Big Bang", b);
		bar(14, 111, 196, 6, (float)(log10(fmax(1, G.s.bangEarned)) / log10(bang_need(&G))), 0x5A3A90, C_PURPLE);
	}
	button(222, 66, 88, 36, C_PURPLE, rdy, 0.46f, "BIG BANG");
	if (rdy) addHit(222, 64, 88, 40, A_BANG, 0);
	textFit(266, 105, 0.32f, C(C_DIM), C2D_AlignCenter, 92, "setzt fast alles zurück");

	}
}

static void pageChal(void) {
	float y0 = 58, rowH = 64;
	scrollMaxSet(P_CHAL, NCHAL * rowH + 24, LIST_BOT - y0);
	float sc = scrollY[P_CHAL];
	textWrap(10, y0 - sc, 0.33f, C(C_DIM), 300, "Start löst eine Supernova aus. Erreiche das Ziel im Lauf, um die Belohnung dauerhaft zu erhalten.");
	for (int c = 0; c < NCHAL; c++) {
		float y = y0 + 24 + c * rowH - sc;
		if (y + rowH < 54 || y > LIST_BOT) continue;
		bool done = (G.s.chalDone >> c) & 1;
		bool act = G.s.chalActive == c;
		panel(6, y, 308, 60, 8, act ? 0x3A1A2A : C_PANEL, act ? C_RED : done ? 0x2A7A50 : 0x2A3166);
		textFit(12, y + 3, 0.46f, C(C_TEXT), 0, 200, CHAL[c].name);
		textFit(12, y + 21, 0.35f, C(C_DIM), 0, 210, CHAL[c].rule);
		char g[32];
		num(CHAL[c].goal, g);
		char rw[80];
		snprintf(rw, sizeof rw, "Ziel %s  ·  %s%s", g, CHAL[c].reward, G.s.sperk[SP_CHALX2] ? " (x2)" : "");
		textFit(12, y + 38, 0.35f, C(done ? C_GREEN : C_GOLD), 0, 216, rw);
		if (act) {
			bar(12, y + 53, 216, 4, (float)chal_progress(&G), C_RED, C_GOLD);
			button(234, y + 14, 74, 28, C_RED, true, 0.4f, "Abbrechen");
			addHitC(6, y, 308, 60, A_CHAL, c, y0, LIST_BOT);
		} else if (done) {
			text(271, y + 18, 0.5f, C(C_GREEN), C2D_AlignCenter, "Erledigt");
		} else {
			button(234, y + 14, 74, 28, C_GOLD, G.s.chalActive < 0, 0.44f, "Start");
			if (G.s.chalActive < 0) addHitC(6, y, 308, 60, A_CHAL, c, y0, LIST_BOT);
		}
	}
	subHeader("Prüfungen");
}

enum { O_SOUND, O_3D, O_SCI, O_AUTOGEN, O_AUTOUPG, O_AUTOCLICK, O_SAVE, O_RESET, NOPT };
static void pageOpt(void) {
	static const char *lbl[NOPT] = {"Ton", "3D-Effekt", "Zahlen wissenschaftlich", "Auto-Bau (Drohnenschwarm)",
		"Auto-Upgrades", "Geisterhand (Auto-Klick)", "Jetzt speichern", "Spielstand löschen"};
	float y = 58 - scrollY[P_OPT];
	int n = 0;
	for (int o = 0; o < NOPT; o++) {
		if (o == O_AUTOGEN && !G.s.perk[PK_AUTOGEN]) continue;
		if (o == O_AUTOUPG && !G.s.perk[PK_AUTOUPG]) continue;
		if (o == O_AUTOCLICK && !G.s.perk[PK_AUTOCLICK]) continue;
		n++;
		if (y + 30 > 54 && y < LIST_BOT) {
			panel(6, y, 308, 28, 6, C_PANEL, 0x2A3166);
			text(14, y + 6, 0.42f, C(o == O_RESET ? C_RED : C_TEXT), 0, lbl[o]);
			if (o < O_SAVE) {
				bool on = o == O_SOUND ? G.s.sound : o == O_3D ? G.s.stereo : o == O_SCI ? g_sci : o == O_AUTOGEN ? G.s.autoGen
					: o == O_AUTOUPG ? G.s.autoUpg : G.s.autoClickOn;
				float sx = 266, sy = y + 6;
				rrect(sx, sy, 40, 16, 8, C(on ? C_GREEN : 0x2A3166));
				circ(on ? sx + 32 : sx + 8, sy + 8, 6.5f, C(0xFFFFFF));
			} else text(300, y + 6, 0.42f, C(C_DIM), C2D_AlignRight, ">");
			addHitC(6, y, 308, 28, A_OPT, o, 56, LIST_BOT);
		}
		y += 32;
	}
	text(160, y + 6, 0.32f, C(0x4A5280), C2D_AlignCenter, "Star Forge v2.0 - START: Beenden");
	scrollMaxSet(P_OPT, n * 32 + 30, LIST_BOT - 58);
	subHeader("Optionen");
}

static const char *HELP =
	"ZIEL\n"
	"Sammle Energie und baue ein kosmisches\n"
	"Imperium auf. Tippe auf den Kristall.\n"
	"Schnelles Tippen baut eine Kombo auf.\n\n"
	"GEBÄUDE & UPGRADES\n"
	"Gebäude erzeugen Energie pro Sekunde.\n"
	"Ab 25, 50, 100 ... Stück verdoppelt\n"
	"sich ihre Leistung. Halte gedrückt, um\n"
	"mehrfach zu kaufen.\n\n"
	"EXPEDITION (HANGAR)\n"
	"Fliege mit deinem Schiff durch Asteroi-\n"
	"denfelder: Circle Pad lenkt, B boostet,\n"
	"der Laser feuert automatisch. Sammle\n"
	"Erz, weiche Minen und Drohnen aus und\n"
	"fliege durch Sprungtore in tiefere\n"
	"Sektoren. Erz bringt Energie und Alloy\n"
	"für Schiffs-Upgrades. Goldene Relikt-\n"
	"Asteroiden enthalten Artefakte.\n\n"
	"AUFTRÄGE\n"
	"Drei Aufträge gleichzeitig. Erfülle sie\n"
	"für Alloy und Energie.\n\n"
	"METEORE\n"
	"Fliegen ab und zu über den Bildschirm.\n"
	"Tippe sie für Energie oder Boni an!\n\n"
	"SUPERNOVA\n"
	"Setzt den Lauf zurück und gibt Star-\n"
	"dust: +5% Produktion pro Stück. Gib ihn\n"
	"für Konstellationen aus, ohne den Bonus\n"
	"zu verlieren.\n\n"
	"BIG BANG\n"
	"Setzt auch Stardust zurück, gibt aber\n"
	"Singularities: mächtige Dauer-Boni und\n"
	"Prüfungen mit festen Belohnungen.\n"
	"Artefakte, Alloy und Schiff bleiben!\n\n"
	"OFFLINE & STANDBY\n"
	"Klappst du den 3DS zu, läuft die\n"
	"Produktion weiter (anfangs 50%).\n\n"
	"TASTEN\n"
	"L/R: Reiter  A: Tippen  B: Zurück\n"
	"Steuerkreuz: Scrollen  START: Beenden";

static void pageHelp(void) {
	int lines = 1;
	for (const char *c = HELP; *c; c++) lines += *c == '\n';
	float lh = 0.38f * FONT_H;
	scrollMaxSet(P_HELP, lines * lh + 10, LIST_BOT - 58);
	text(12, 58 - scrollY[P_HELP], 0.38f, C(C_TEXT), 0, HELP);
	subHeader("Hilfe");
}

#include "ui_exp.inc"

// ============================================================ Kopf, Reiter, Meteor, Modal
static bool anyGenAffordable(void) {
	for (int i = 0; i < NGEN; i++)
		if (gen_visible(&G, i) && !gen_locked(&G, i) && gen_cost(&G, i, 1) <= G.s.energy) return true;
	return false;
}
static int upgAffordable(void) {
	if (G.s.chalActive == CH_ASCETIC) return 0;
	int n = 0;
	for (int i = 0; i < nUpg; i++)
		if (!upg_bought(&G, i) && upg_unlocked(&G, i) && upg_cost(&G, i) <= G.s.energy) n++;
	return n;
}
static bool perkAffordable(void) {
	if (nova_gain(&G) >= 1) return true;
	for (int i = 0; i < NPERK; i++)
		if (perk_available(&G, i) && G.s.perk[i] < PERK[i].max && perk_cost(&G, i) <= G.s.dust) return true;
	return false;
}

static void tabIcon(int k, float cx, float cy, u32 c) {
	switch (k) {
	case 0:
		tri(cx, cy - 8, C(lighten(c, 0.4f)), cx - 6, cy, C(c), cx + 6, cy, C(c));
		tri(cx - 6, cy, C(c), cx, cy + 8, C(darken(c, 0.4f)), cx + 6, cy, C(darken(c, 0.2f)));
		break;
	case 1:
		rect(cx - 8, cy - 2, 5, 9, C(c));
		rect(cx - 2, cy - 8, 5, 15, C(c));
		rect(cx + 4, cy - 4, 5, 11, C(c));
		break;
	case 2:
		tri(cx, cy - 9, C(c), cx - 7, cy - 1, C(c), cx + 7, cy - 1, C(c));
		rect(cx - 3, cy - 1, 6, 8, C(c));
		break;
	case 3:
		tri(cx + 9, cy, C(lighten(c, 0.3f)), cx - 6, cy - 6, C(c), cx - 6, cy + 6, C(darken(c, 0.3f)));
		tri(cx - 6, cy - 6, C(darken(c, 0.2f)), cx - 2, cy, C(c), cx - 8, cy, C(c));
		tri(cx - 6, cy + 6, C(darken(c, 0.2f)), cx - 2, cy, C(c), cx - 8, cy, C(c));
		break;
	case 4: starIcon(cx, cy, 8, c); break;
	case 5:
		for (int i = -1; i <= 1; i++) circ(cx + i * 6, cy, 2.4f, C(c));
		break;
	}
}

static void drawHeader(void) {
	rectG(0, 0, BW, 30, C(0x1A2250), C(0x111738));
	rect(0, 30, BW, 1, C(0x2E3A78));
	char b[32], b2[32];
	tri(14, 6, C(C_CYAN), 8, 15, C(lighten(C_CYAN, 0.4f)), 20, 15, C(C_CYAN));
	tri(8, 15, C(C_CYAN), 14, 24, C(darken(C_CYAN, 0.4f)), 20, 15, C(darken(C_CYAN, 0.2f)));
	num(floor(G.s.energy), b);
	textFit(26, 3, 0.62f, C(C_TEXT), 0, 170, b);
	num(G.eps, b2);
	textFit(312, 8, 0.42f, C(G.s.buffT[BUFF_FRENZY] > 0 ? C_PINK : C_CYAN), C2D_AlignRight, 120, b2);
	text(312, 19, 0.3f, C(C_DIM), C2D_AlignRight, "pro Sekunde");
}

static void drawTabs(void) {
	static const char *lbl[6] = {"Kern", "Bauen", "Upgrades", "Hangar", "Sterne", "Mehr"};
	const float TWD = BW / 6.0f;
	rectG(0, 207, BW, 33, C(0x141A40), C(0x0C1028));
	rect(0, 207, BW, 1, C(0x2E3A78));
	for (int k = 0; k < 6; k++) {
		float x = k * TWD;
		bool on = tab == k;
		if (on) {
			rectG(x + 2, 208, TWD - 4, 32, C(0x26306A), C(0x161C44));
			rect(x + 8, 208, TWD - 16, 2, C(C_CYAN));
		}
		tabIcon(k, x + TWD / 2, 218, on ? C_CYAN : C_DIM);
		textFit(x + TWD / 2, 227.5f, 0.34f, C(on ? C_TEXT : C_DIM), C2D_AlignCenter, TWD - 4, lbl[k]);
		bool badge = false;
		int cnt = 0;
		if (k == 1) badge = anyGenAffordable() && G.s.gens[0] < 1;
		if (k == 2) { cnt = upgAffordable(); badge = cnt > 0; }
		if (k == 3) badge = shipAffordable();
		if (k == 4) badge = perkAffordable();
		if (k == 5) { cnt = contracts_ready(&G); badge = bang_ready(&G) || cnt > 0; }
		if (badge && !on) {
			float pulse = 0.5f + 0.5f * sinf(T * 5);
			circ(x + TWD - 10, 213, 5 + pulse, C(C_RED));
			if (cnt > 0 && cnt < 100) textf(x + TWD - 10, 207.5f, 0.28f, C(0xFFFFFF), C2D_AlignCenter, "%d", cnt);
		}
		addHit(x, 207, TWD, 33, A_TAB, k);
	}
}

static void drawMeteor(void) {
	if (G.meteorLife <= 0) return;
	float x, y;
	meteorPos(&x, &y);
	float dir = meteorX1 > meteorX0 ? -1 : 1;
	for (int k = 1; k < 12; k++) circ(x + dir * k * 4, y - k * 1.0f, 9 - k * 0.65f, RGBA(hexMix(C_GOLD, 0xFF5A2A, k / 12.0f), 0.55f - k * 0.04f));
	float p = 0.5f + 0.5f * sinf(T * 8);
	circ(x, y, 22 + p * 4, RGBA(C_GOLD, 0.12f));
	circ(x, y, 14, RGBA(C_GOLD, 0.25f));
	C2D_DrawCircle(x, y, 0, 10, C(0xFFF6D0), C(C_GOLD), C(0xFFB040), C(0xC06A20));
	circ(x - 3, y - 3, 3, RGBA(0xFFFFFF, 0.8f));
	if (frnd() < 0.4f) spawnP(0, x, y, dir * 40 + (frnd() - 0.5f) * 30, (frnd() - 0.5f) * 30, 0.5f, 1.5f, C_GOLD);
	addHit(x - 26, y - 26, 52, 52, A_METEOR, 0);
}

static void drawModal(void) {
	if (!modal) return;
	nHits = 0;
	rect(0, 0, BW, SH, RGBA(0x04060F, 0.78f));
	float x = 18, y = 46, w = 284, h = 148;
	u32 bc = modal == MD_NOVA ? C_GOLD : modal == MD_BANG ? C_PURPLE : (modal == MD_RESET || modal == MD_RESET2 || modal == MD_CHALABORT) ? C_RED : C_CYAN;
	panel(x, y, w, h, 10, C_PANEL, bc);
	rect(x + 12, y, w - 24, 2, C(bc));
	textFit(BW / 2, y + 8, 0.6f, C(bc), C2D_AlignCenter, w - 20, modalTitle);
	textWrap(x + 14, y + 36, 0.4f, C(C_TEXT), w - 28, modalMsg);
	if (modal == MD_INFO) {
		button(BW / 2 - 50, y + h - 38, 100, 28, C_CYAN, true, 0.48f, "OK");
		addHit(0, 0, BW, SH, A_YES, 0);
	} else {
		button(x + 16, y + h - 38, 116, 28, bc, true, 0.48f, modal == MD_QUIT ? "Beenden" : "Ja");
		button(x + w - 132, y + h - 38, 116, 28, C_PANEL2, true, 0.48f, "Abbrechen");
		addHit(x + 10, y + h - 44, 128, 40, A_YES, 0);
		addHit(x + w - 138, y + h - 44, 128, 40, A_NO, 0);
	}
}

static void drawFloats(void) {
	for (int i = 0; i < NFLOAT; i++) {
		Floater *f = &floats[i];
		if (f->life <= 0) continue;
		float a = fminf(1, f->life * 2);
		textFit(f->x, f->y - (1 - f->life) * 30, 0.5f, RGBA(f->col, a), C2D_AlignCenter, 140, f->s);
	}
}

static void drawBottom(void) {
	nHits = 0;
	rectG(0, 0, BW, SH, C(C_BG1), C(C_BG2));
	// leichte Sterne im Hintergrund
	for (int i = 0; i < 40; i++) {
		Star *s = &stars[i];
		float x = fmodf(s->x * 0.8f + T * 1.5f * s->z, BW);
		rect(x, s->y, 1, 1, RGBA(0xFFFFFF, 0.15f + 0.15f * sinf(T + s->ph)));
	}
	switch (page) {
	case P_CORE: pageCore(); break;
	case P_BUILD: pageBuild(); break;
	case P_UPG: pageUpg(); break;
	case P_STARS: pageStars(); break;
	case P_MORE: pageMore(); break;
	case P_ACH: pageAch(); break;
	case P_STATS: pageStats(); break;
	case P_BANG: pageBang(); break;
	case P_CHAL: pageChal(); break;
	case P_OPT: pageOpt(); break;
	case P_HELP: pageHelp(); break;
	case P_HANGAR: pageHangar(); break;
	case P_CONTRACTS: pageContracts(); break;
	case P_ARTS: pageArts(); break;
	}
	// Scrollbalken
	if (scrollMax[page] > 0) {
		float top = page == P_UPG ? 92 : page == P_STARS ? 100 : page == P_BANG ? 126 : page == P_BUILD ? 56 : page == P_HANGAR ? 104 : 58;
		float vh = LIST_BOT - top;
		float th = fmaxf(16, vh * vh / (vh + scrollMax[page]));
		float ty = top + (vh - th) * (scrollY[page] / scrollMax[page]);
		rrect(316, ty, 3, th, 1.5f, RGBA(C_CYAN, 0.45f));
	}
	drawParts(0);
	drawFloats();
	drawHeader();
	drawTabs();
	drawMeteor();
	drawModal();
}

// ============================================================ Eingabe
static bool tDown, tDrag, tConsumed, tRepeated;
static float tx0, ty0, tx, ty, tLastY, tHold, tRepeatT, tScroll0;

static bool pageScrolls(void) { return scrollMax[page] > 0; }

static void setTab(int k) {
	if (k < 0) k = 5;
	if (k > 5) k = 0;
	if (tab == k && page != k) { page = k; return; }
	tab = k;
	page = k;
	play(SND_TAB, 1, 0.35f);
}

static void modalYes(void) {
	int m = modal;
	modal = MD_NONE;
	switch (m) {
	case MD_NOVA: doNova(); break;
	case MD_BANG: doBang(); break;
	case MD_CHAL:
		chal_start(&G, modalP);
		novaFlash = 2.0f;
		toast(C_RED, "Prüfung gestartet", "%s: %s", CHAL[modalP].name, CHAL[modalP].rule);
		play(SND_NOVA, 1.3f, 0.6f);
		break;
	case MD_CHALABORT: chal_abort(&G); toast(C_DIM, "Prüfung abgebrochen", "%s", CHAL[modalP].name); break;
	case MD_RESET:
		openModal(MD_RESET2, 0, "Wirklich?", "Letzte Warnung: Alles wird unwiderruflich gelöscht, auch Erfolge.");
		break;
	case MD_RESET2: {
		u8 snd0 = G.s.sound, st = G.s.stereo;
		game_new(&G);
		contract_fill(&G);
		G.s.sound = snd0;
		G.s.stereo = st;
		saveGame();
		tab = page = 0;
		toast(C_RED, "Neues Spiel", "Der Spielstand wurde gelöscht.");
		break;
	}
	case MD_QUIT: quitReq = true; break;
	default: break;
	}
}

static void act(Hit *h, float x, float y) {
	switch (h->act) {
	case A_TAB: setTab(h->p); break;
	case A_CRYSTAL: doClick(x, y); break;
	case A_METEOR: {
		double gain;
		float mx, my;
		meteorPos(&mx, &my);
		int e = meteor_collect(&G, &gain);
		if (e >= 0) meteorEffect(e, gain, mx, my);
		break;
	}
	case A_BUYMODE: G.s.buyMode = h->p; play(SND_TAB, 1.2f, 0.3f); break;
	case A_GEN: buyGen(h->p, 270, y); break;
	case A_UPGSEL:
		if (upgSel == h->p && upg_unlocked(&G, h->p)) {
			if (upg_buy(&G, h->p)) { play(SND_BUY, 1.2f, 0.55f); burst(0, x, y, 14, 120, C_CYAN); upgSel = -1; }
			else play(SND_ERR, 1, 0.4f);
		} else { upgSel = h->p; play(SND_TAB, 1.1f, 0.3f); }
		break;
	case A_UPGBUY:
		if (upg_buy(&G, h->p)) { play(SND_BUY, 1.2f, 0.55f); burst(0, 270, 48, 14, 120, C_CYAN); upgSel = -1; }
		else play(SND_ERR, 1, 0.4f);
		break;
	case A_UPGALL: {
		int n = upg_buy_all(&G);
		if (n > 0) {
			play(SND_BUY, 1.3f, 0.6f);
			burst(0, 270, 72, 20, 140, C_CYAN);
			char s[24];
			snprintf(s, sizeof s, "+%d Upgrades", n);
			spawnFloat(270, 80, C_CYAN, s);
		} else play(SND_ERR, 1, 0.4f);
		upgSel = -1;
		break;
	}
	case A_NOVA: {
		char b[32];
		num(nova_gain(&G), b);
		openModal(MD_NOVA, 0, "Supernova auslösen?", "Du erhältst +%s Stardust. Energie, Gebäude und Upgrades werden zurückgesetzt.", b);
		break;
	}
	case A_PERK:
		if (perk_buy(&G, h->p)) { play(SND_BUY, 0.8f, 0.6f); burst(0, 270, y, 16, 120, C_GOLD); }
		else play(SND_ERR, 1, 0.4f);
		break;
	case A_MENU: page = h->p; scrollY[page] = 0; play(SND_TAB, 1, 0.3f); break;
	case A_BACK: page = P_MORE; play(SND_TAB, 0.9f, 0.3f); break;
	case A_BANG: {
		char b[32];
		num(bang_gain(&G), b);
		openModal(MD_BANG, 0, "Big Bang auslösen?", "Du erhältst +%s Singularities. ALLES außer Erfolgen, Singularities und Prüfungen wird zurückgesetzt.", b);
		break;
	}
	case A_SPERK:
		if (sperk_buy(&G, h->p)) { play(SND_BUY, 0.7f, 0.6f); burst(0, 270, y, 16, 120, C_PURPLE); }
		else play(SND_ERR, 1, 0.4f);
		break;
	case A_CHAL:
		if (G.s.chalActive == h->p) openModal(MD_CHALABORT, h->p, "Prüfung abbrechen?", "Der Lauf geht normal weiter, aber ohne Belohnung.");
		else openModal(MD_CHAL, h->p, CHAL[h->p].name, "%s Startet eine Supernova. Belohnung: %s.", CHAL[h->p].rule, CHAL[h->p].reward);
		break;
	case A_OPT:
		switch (h->p) {
		case O_SOUND: G.s.sound = !G.s.sound; break;
		case O_3D: G.s.stereo = !G.s.stereo; gfxSet3D(G.s.stereo); break;
		case O_SCI: g_sci = !g_sci; G.s.sciNotation = g_sci; break;
		case O_AUTOGEN: G.s.autoGen = !G.s.autoGen; break;
		case O_AUTOUPG: G.s.autoUpg = !G.s.autoUpg; break;
		case O_AUTOCLICK: G.s.autoClickOn = !G.s.autoClickOn; break;
		case O_SAVE: if (saveGame()) toast(C_GREEN, "Gespeichert", "Spielstand auf SD-Karte gesichert."); else toast(C_RED, "Fehler", "Speichern fehlgeschlagen!"); break;
		case O_RESET: openModal(MD_RESET, 0, "Spielstand löschen?", "Dein gesamter Fortschritt geht verloren."); break;
		}
		play(SND_TAB, 1.1f, 0.35f);
		break;
	case A_YES: modalYes(); play(SND_TAB, 1, 0.35f); break;
	case A_HSTART: startExpedition(); break;
	case A_HSEC: expSector += h->p; play(SND_TAB, 1.1f, 0.3f); break;
	case A_SHIPUP:
		if (ship_up_buy(&G, h->p)) { play(SND_BUY, 1.1f, 0.55f); burst(0, 270, y, 14, 120, C_CYAN); }
		else play(SND_ERR, 1, 0.4f);
		break;
	case A_CLAIM: {
		double en, al;
		if (contract_claim(&G, h->p, &en, &al)) {
			char b[32], b2[32];
			num(en, b);
			num(al, b2);
			toast(C_GREEN, "Auftrag erfüllt!", "+%s Alloy, +%s Energie", b2, b);
			play(SND_ACH, 1.1f, 0.6f);
			burst(0, 270, y, 20, 140, C_GREEN);
		}
		break;
	}
	case A_NO: modal = MD_NONE; play(SND_TAB, 0.8f, 0.35f); break;
	}
}

static void handleInput(u32 kDown, u32 kHeld, u32 kUp, float dt) {
	if (inExp) { handleExpInput(kDown, kHeld); return; }
	touchPosition tp;
	hidTouchRead(&tp);
	if (kDown & KEY_TOUCH) {
		tDown = true; tDrag = false; tConsumed = false; tRepeated = false;
		tx0 = tx = tp.px; ty0 = ty = tLastY = tp.py;
		tHold = 0; tRepeatT = 0;
		tScroll0 = scrollY[page];
		scrollV[page] = 0;
		Hit *h = hitAt(tx, ty);
		if (h && (h->act == A_TAB || h->act == A_CRYSTAL || h->act == A_METEOR)) {
			act(h, tx, ty);
			tConsumed = true;
		}
	} else if (tDown && (kHeld & KEY_TOUCH)) {
		tx = tp.px; ty = tp.py;
		tHold += dt;
		if (!tConsumed && !tDrag && fabsf(ty - ty0) > 7 && pageScrolls() && ty0 > 30 && ty0 < 207) tDrag = true;
		if (tDrag) {
			float ns = tScroll0 - (ty - ty0);
			if (ns < -30) ns = -30 + (ns + 30) * 0.3f;
			if (ns > scrollMax[page] + 30) ns = scrollMax[page] + 30 + (ns - scrollMax[page] - 30) * 0.3f;
			scrollY[page] = ns;
			if (dt > 0) scrollV[page] = scrollV[page] * 0.5f + (tLastY - ty) / dt * 0.5f;
			tLastY = ty;
		} else if (!tConsumed && tHold > 0.45f) {
			Hit *h = hitAt(tx0, ty0);
			if (h && (h->act == A_GEN || h->act == A_PERK || h->act == A_SPERK)) {
				tRepeatT -= dt;
				if (tRepeatT <= 0) { act(h, tx0, ty0); tRepeatT = 0.09f; tRepeated = true; }
			}
		}
	}
	if (kUp & KEY_TOUCH) {
		if (tDown && !tConsumed && !tDrag && !tRepeated) {
			Hit *h = hitAt(tx0, ty0);
			if (h && h->act != A_TAB && h->act != A_CRYSTAL && h->act != A_METEOR) act(h, tx0, ty0);
		}
		tDown = false;
		tDrag = false;
	}
	// Tasten
	if (modal) {
		if (kDown & KEY_A) { modalYes(); }
		else if (kDown & KEY_B) { modal = MD_NONE; }
		return;
	}
	if (kDown & KEY_L) setTab(tab - 1);
	if (kDown & KEY_R) setTab(tab + 1);
	if ((kDown & KEY_A) && page == P_CORE) doClick(160, 112);
	if ((kDown & KEY_B) && page > P_MORE) page = P_MORE;
	if (kHeld & (KEY_DOWN | KEY_CPAD_DOWN)) scrollY[page] += 260 * dt;
	if (kHeld & (KEY_UP | KEY_CPAD_UP)) scrollY[page] -= 260 * dt;
	if (kDown & KEY_START) openModal(MD_QUIT, 0, "Beenden?", "Dein Fortschritt wird gespeichert. Klappst du den 3DS nur zu, läuft die Produktion weiter.");
}

static void updScroll(float dt) {
	for (int p = 0; p < NPAGE; p++) {
		if (tDrag && p == page) continue;
		scrollY[p] += scrollV[p] * dt;
		scrollV[p] *= powf(0.04f, dt);
		if (fabsf(scrollV[p]) < 5) scrollV[p] = 0;
		if (scrollY[p] < 0) { scrollY[p] += (0 - scrollY[p]) * fminf(1, dt * 14); if (scrollY[p] > -0.5f) scrollY[p] = 0; scrollV[p] = 0; }
		if (scrollY[p] > scrollMax[p]) { scrollY[p] += (scrollMax[p] - scrollY[p]) * fminf(1, dt * 14); scrollV[p] = 0; }
	}
}

// ============================================================ Hauptprogramm
static void handleEvents(void) {
	GameEvent e;
	while (game_pop(&G, &e)) {
		switch (e.type) {
		case EV_ACH: {
			char nm[48], ds[96];
			ach_text(e.id, nm, sizeof nm, ds, sizeof ds);
			toast(C_GOLD, "Erfolg freigeschaltet!", "%s", nm);
			play(SND_ACH, 1, 0.6f);
			break;
		}
		case EV_METEOR:
			meteorSpawn();
			play(SND_METEOR, 0.7f, 0.35f);
			break;
		case EV_METEOR_AUTO: {
			float x, y;
			meteorPos(&x, &y);
			meteorEffect(e.id, 0, x, y);
			break;
		}
		case EV_CHAL_DONE:
			toast(C_GREEN, "Prüfung gemeistert!", "%s - Belohnung: %s", CHAL[e.id].name, CHAL[e.id].reward);
			play(SND_ACH, 0.8f, 0.7f);
			break;
		case EV_CHAL_FAIL:
			toast(C_RED, "Prüfung verloren", "%s: Zeit abgelaufen.", CHAL[e.id].name);
			play(SND_ERR, 0.8f, 0.6f);
			break;
		}
	}
}

int main(void) {
	gfxInitDefault();
	osSetSpeedupEnable(true);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(12000);
	C2D_Prepare();
	C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
	tgtTop = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	tgtTopR = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	tgtBot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	tbuf = C2D_TextBufNew(16384);
	srand((unsigned)osGetTime());
	game_seed((u32)osGetTime());
	game_init_tables();
	game_new(&G);
	initStars();
	sndInit();
	bool loaded = loadGame();
	g_sci = G.s.sciNotation;
	contract_fill(&G);
#ifdef DESKTOP_TEST
	if (getenv("SF_ALLOY")) G.s.ext.x.alloy = atof(getenv("SF_ALLOY"));
	if (getenv("SF_ARTS")) G.s.ext.x.artifacts = (uint32_t)strtoul(getenv("SF_ARTS"), NULL, 0);
	if (getenv("SF_SECTOR")) G.s.ext.x.sectorMax = atoi(getenv("SF_SECTOR"));
#endif
	gfxSet3D(G.s.stereo);
	game_recalc(&G);
	if (loaded) {
		u64 now = osGetTime();
		if (G.s.lastSave > 0 && now > G.s.lastSave) {
			double away = (now - G.s.lastSave) / 1000.0;
			if (away > 60) {
				double gain = game_offline(&G, away);
				char b[32], t[32];
				num(gain, b);
				fmt_time(fmin(away, game_offline_cap(&G)), t, sizeof t);
				openModal(MD_INFO, 0, "Willkommen zurück!", "Du warst %s weg. Deine Schmiede hat %s Energie erzeugt (%.0f%% Effizienz).", t, b, game_offline_eff(&G) * 100);
			}
		}
	} else {
		toast(C_CYAN, "Willkommen, Schmied!", "Tippe auf den Kristall, um Energie zu sammeln.");
	}
	aptHook(&hookCookie, onAptHook, NULL);

	u64 last = osGetTime();
	float saveT = 0;
	while (aptMainLoop() && !quitReq) {
		hidScanInput();
		u32 kDown = hidKeysDown(), kHeld = hidKeysHeld(), kUp = hidKeysUp();
		u64 now = osGetTime();
		double dt = now >= last ? (now - last) / 1000.0 : 0;
		last = now;
		if (dt > 5) {
			double gain = game_offline(&G, dt);
			if (dt > 60 && gain > 0 && !modal) {
				char b[32], t[32];
				num(gain, b);
				fmt_time(fmin(dt, game_offline_cap(&G)), t, sizeof t);
				openModal(MD_INFO, 0, "Willkommen zurück!", "Standby: %s. Deine Schmiede hat %s Energie erzeugt.", t, b);
			}
			dt = 0;
		}
		game_update(&G, dt);
		float fdt = (float)fmin(dt, 0.05);
		T += fdt;
		handleEvents();
		handleInput(kDown, kHeld, kUp, fdt);
		if (inExp) {
			float ix, iy;
			bool boost;
			expInput(kHeld, &ix, &iy, &boost);
			if (!modal) exp_step(&EX, &G, ix, iy, boost, fdt);
			expEvents();
			if (expIntroT > 0) expIntroT -= fdt;
			float tcx = fminf(fmaxf(EX.x, 200), WORLD - 200), tcy = fminf(fmaxf(EX.y, 120), WORLD - 120);
			camX += (tcx - camX) * fminf(1, fdt * 8);
			camY += (tcy - camY) * fminf(1, fdt * 8);
			if (EX.done) endExpedition();
		}
		updScroll(fdt);
		updParts(fdt);
		for (int i = 0; i < NFLOAT; i++) if (floats[i].life > 0) floats[i].life -= fdt;
		crystalKick = fmaxf(0, crystalKick - fdt * 6);
		coreKick = fmaxf(0, coreKick - fdt * 3);
		novaFlash = fmaxf(0, novaFlash - fdt);
		bangFlash = fmaxf(0, bangFlash - fdt);
		saveFlash = fmaxf(0, saveFlash - fdt);
		if (toastTail != toastHead) {
			toastT += fdt;
			if (toastT > 3.0f) { toastT = 0; toastTail = (toastTail + 1) % NTOAST; }
		}
		saveT += fdt;
		if (saveT > 30) { saveT = 0; saveGame(); }

		slider3d = G.s.stereo ? osGet3DSliderState() : 0;
		C2D_TextBufClear(tbuf);
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(tgtTop, C(0x060818));
		C2D_SceneBegin(tgtTop);
		eyeSign = -1;
		if (inExp) drawExpTop(); else drawTop();
		if (slider3d > 0.01f) {
			C2D_TargetClear(tgtTopR, C(0x060818));
			C2D_SceneBegin(tgtTopR);
			eyeSign = 1;
			if (inExp) drawExpTop(); else drawTop();
		}
		eyeSign = 0;
		C2D_TargetClear(tgtBot, C(C_BG1));
		C2D_SceneBegin(tgtBot);
		if (inExp) drawExpBottom(); else drawBottom();
		C3D_FrameEnd(0);
	}
	saveGame();
	sndExit();
	C2D_TextBufDelete(tbuf);
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
