// Software-Rasterizer + Eingabe-Skript fuer Screenshots am PC
#include "citro2d.h"
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#undef fopen
#undef mkdir
#undef remove
#undef rename

static float fbTop[240][400][3], fbBot[240][320][3], fbR[240][400][3];
static float (*cur)[3];
static int curW, curH;
static C3D_RenderTarget tTop = {0, 0}, tTopR = {0, 1}, tBot = {1, 0};

static unsigned char font[224][1 + 32 * 30];
static const char *saveDir = ".", *outDir = ".";

// ---------------- Script
typedef struct { int f; char cmd[16]; float a[5]; char s[64]; } Ev;
static Ev evs[512];
static int nEv, frame, quitAt = 1 << 30;
static u64 clockMs = 3900000000000ull;
static double speed = 1;
static u32 kDown, kHeld, kUp, kPrev;
static int tx, ty, touchOn;
static int stickX, stickY, stickUntil;
static struct { int on, f0, n; float x0, y0, x1, y1; } drag;
static char shotName[64];

static void loadScript(const char *p) {
	FILE *f = fopen(p, "r");
	if (!f) return;
	char line[256];
	while (fgets(line, sizeof line, f) && nEv < 512) {
		Ev *e = &evs[nEv];
		memset(e, 0, sizeof *e);
		int n = sscanf(line, "%d %15s", &e->f, e->cmd);
		if (n < 2 || line[0] == '#') continue;
		char *rest = strstr(line, e->cmd) + strlen(e->cmd);
		if (!strcmp(e->cmd, "shot") || !strcmp(e->cmd, "key")) sscanf(rest, "%63s", e->s);
		else sscanf(rest, "%f %f %f %f %f", &e->a[0], &e->a[1], &e->a[2], &e->a[3], &e->a[4]);
		nEv++;
	}
	fclose(f);
}

static u32 keyBit(const char *s) {
	if (!strcmp(s, "A")) return KEY_A; if (!strcmp(s, "B")) return KEY_B; if (!strcmp(s, "L")) return KEY_L;
	if (!strcmp(s, "R")) return KEY_R; if (!strcmp(s, "START")) return KEY_START; if (!strcmp(s, "DOWN")) return KEY_DOWN;
	if (!strcmp(s, "UP")) return KEY_UP;
	return 0;
}
static u32 pressKeys;

void hidScanInput(void) {
	frame++;
	u32 held = 0;
	pressKeys = 0;
	for (int i = 0; i < nEv; i++) {
		Ev *e = &evs[i];
		if (e->f != frame) continue;
		if (!strcmp(e->cmd, "tap")) { drag.on = 1; drag.f0 = frame; drag.n = 2; drag.x0 = drag.x1 = e->a[0]; drag.y0 = drag.y1 = e->a[1]; }
		else if (!strcmp(e->cmd, "hold")) { drag.on = 1; drag.f0 = frame; drag.n = (int)e->a[2]; drag.x0 = drag.x1 = e->a[0]; drag.y0 = drag.y1 = e->a[1]; }
		else if (!strcmp(e->cmd, "drag")) { drag.on = 1; drag.f0 = frame; drag.n = (int)e->a[4]; drag.x0 = e->a[0]; drag.y0 = e->a[1]; drag.x1 = e->a[2]; drag.y1 = e->a[3]; }
		else if (!strcmp(e->cmd, "key")) pressKeys |= keyBit(e->s);
		else if (!strcmp(e->cmd, "shot")) snprintf(shotName, sizeof shotName, "%s", e->s);
		else if (!strcmp(e->cmd, "time")) clockMs += (u64)(e->a[0] * 1000);
		else if (!strcmp(e->cmd, "speed")) speed = e->a[0];
		else if (!strcmp(e->cmd, "quit")) quitAt = frame;
		else if (!strcmp(e->cmd, "stick")) { stickX = (int)e->a[0]; stickY = (int)e->a[1]; stickUntil = frame + (int)e->a[2]; }
	}
	if (drag.on) {
		int k = frame - drag.f0;
		if (k < drag.n) {
			float p = drag.n > 1 ? (float)k / (drag.n - 1) : 0;
			tx = (int)(drag.x0 + (drag.x1 - drag.x0) * p);
			ty = (int)(drag.y0 + (drag.y1 - drag.y0) * p);
			held |= KEY_TOUCH;
		} else drag.on = 0;
	}
	held |= pressKeys;
	kDown = held & ~kPrev;
	kUp = kPrev & ~held;
	kHeld = held;
	kPrev = held;
	clockMs += (u64)(1000.0 / 60 * speed);
}
u32 hidKeysDown(void) { return kDown; }
u32 hidKeysHeld(void) { return kHeld; }
u32 hidKeysUp(void) { return kUp; }
void hidTouchRead(touchPosition *t) { t->px = (kHeld & KEY_TOUCH) ? tx : 0; t->py = (kHeld & KEY_TOUCH) ? ty : 0; }
u64 osGetTime(void) { return clockMs; }
bool aptMainLoop(void) { return frame < quitAt; }
void aptHook(aptHookCookie *c, aptHookFn f, void *p) { (void)c; (void)f; (void)p; }
void gfxInitDefault(void) {}
void gfxExit(void) {}
void gfxSet3D(bool b) { (void)b; }
void osSetSpeedupEnable(bool b) { (void)b; }
float osGet3DSliderState(void) { return 0; }
void *linearAlloc(size_t n) { return malloc(n); }
void linearFree(void *p) { free(p); }

static void mapPath(const char *p, char *out, int n) {
	const char *s = strrchr(p, '/');
	snprintf(out, n, "%s/%s", saveDir, s ? s + 1 : p);
}
FILE *shim_fopen(const char *p, const char *m) { char b[512]; mapPath(p, b, sizeof b); return fopen(b, m); }
int shim_mkdir(const char *p, int m) { (void)p; (void)m; return 0; }
int shim_remove(const char *p) { char b[512]; mapPath(p, b, sizeof b); return remove(b); }
int shim_rename(const char *a, const char *b) { char x[512], y[512]; mapPath(a, x, sizeof x); mapPath(b, y, sizeof y); return rename(x, y); }

// ---------------- Rasterizer
static void blend(int x, int y, u32 c, float cov) {
	if (x < 0 || y < 0 || x >= curW || y >= curH) return;
	float a = ((c >> 24) & 255) / 255.0f * cov;
	if (a <= 0) return;
	float *p = cur[y * curW + x];
	p[0] += (((c) & 255) / 255.0f - p[0]) * a;
	p[1] += (((c >> 8) & 255) / 255.0f - p[1]) * a;
	p[2] += (((c >> 16) & 255) / 255.0f - p[2]) * a;
}
static u32 lerpc(u32 a, u32 b, float t) {
	u32 r = 0;
	for (int s = 0; s < 32; s += 8) {
		float va = (a >> s) & 255, vb = (b >> s) & 255;
		r |= ((u32)(va + (vb - va) * t) & 255) << s;
	}
	return r;
}
static u32 bil(u32 c0, u32 c1, u32 c2, u32 c3, float u, float v) { return lerpc(lerpc(c0, c1, u), lerpc(c2, c3, u), v); }

void C2D_DrawRectangle(float x, float y, float z, float w, float h, u32 c0, u32 c1, u32 c2, u32 c3) {
	(void)z;
	if (w <= 0 || h <= 0) return;
	for (int j = (int)floorf(y); j < (int)ceilf(y + h); j++)
		for (int i = (int)floorf(x); i < (int)ceilf(x + w); i++) {
			float cx = i + 0.5f, cy = j + 0.5f;
			if (cx < x || cx >= x + w || cy < y || cy >= y + h) continue;
			blend(i, j, bil(c0, c1, c2, c3, (cx - x) / w, (cy - y) / h), 1);
		}
}
void C2D_DrawRectSolid(float x, float y, float z, float w, float h, u32 c) { C2D_DrawRectangle(x, y, z, w, h, c, c, c, c); }
void C2D_DrawCircle(float x, float y, float z, float r, u32 c0, u32 c1, u32 c2, u32 c3) {
	(void)z;
	for (int j = (int)floorf(y - r); j <= (int)ceilf(y + r); j++)
		for (int i = (int)floorf(x - r); i <= (int)ceilf(x + r); i++) {
			float dx = i + 0.5f - x, dy = j + 0.5f - y, d = sqrtf(dx * dx + dy * dy);
			float cov = r - d + 0.5f;
			if (cov <= 0) continue;
			if (cov > 1) cov = 1;
			blend(i, j, bil(c0, c1, c2, c3, (dx + r) / (2 * r), (dy + r) / (2 * r)), cov);
		}
}
void C2D_DrawCircleSolid(float x, float y, float z, float r, u32 c) { C2D_DrawCircle(x, y, z, r, c, c, c, c); }
void C2D_DrawTriangle(float x0, float y0, u32 c0, float x1, float y1, u32 c1, float x2, float y2, u32 c2, float d) {
	(void)d;
	float minx = fminf(x0, fminf(x1, x2)), maxx = fmaxf(x0, fmaxf(x1, x2));
	float miny = fminf(y0, fminf(y1, y2)), maxy = fmaxf(y0, fmaxf(y1, y2));
	float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
	if (fabsf(area) < 1e-6f) return;
	for (int j = (int)floorf(miny); j <= (int)ceilf(maxy); j++)
		for (int i = (int)floorf(minx); i <= (int)ceilf(maxx); i++) {
			float px = i + 0.5f, py = j + 0.5f;
			float w0 = ((x1 - px) * (y2 - py) - (x2 - px) * (y1 - py)) / area;
			float w1 = ((x2 - px) * (y0 - py) - (x0 - px) * (y2 - py)) / area;
			float w2 = 1 - w0 - w1;
			if (w0 < 0 || w1 < 0 || w2 < 0) continue;
			u32 c = lerpc(lerpc(c0, c1, w1 / (w0 + w1 + 1e-9f)), c2, w2);
			blend(i, j, c, 1);
		}
}
void C2D_DrawLine(float x0, float y0, u32 c0, float x1, float y1, u32 c1, float th, float d) {
	float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
	if (l < 1e-4f) return;
	float nx = -dy / l * th / 2, ny = dx / l * th / 2;
	C2D_DrawTriangle(x0 + nx, y0 + ny, c0, x1 + nx, y1 + ny, c1, x0 - nx, y0 - ny, c0, d);
	C2D_DrawTriangle(x1 + nx, y1 + ny, c1, x1 - nx, y1 - ny, c1, x0 - nx, y0 - ny, c0, d);
}

// ---------------- Text
struct C2D_TextBuf_s { int x; };
C2D_TextBuf C2D_TextBufNew(int n) { (void)n; return calloc(1, sizeof(struct C2D_TextBuf_s)); }
void C2D_TextBufClear(C2D_TextBuf b) { (void)b; }
void C2D_TextBufDelete(C2D_TextBuf b) { free(b); }
const char *C2D_TextParse(C2D_Text *t, C2D_TextBuf b, const char *s) { (void)b; t->str = strdup(s); return s + strlen(s); }

static int nextCp(const unsigned char **p) {
	const unsigned char *s = *p;
	int c = *s++;
	if (c >= 0xC0 && c < 0xE0) c = ((c & 31) << 6) | (*s++ & 63);
	else if (c >= 0xE0 && c < 0xF0) { c = ((c & 15) << 12) | ((s[0] & 63) << 6) | (s[1] & 63); s += 2; }
	*p = s;
	return c;
}
static float adv(int cp) { return (cp >= 32 && cp < 256) ? font[cp - 32][0] : 12; }
static float lineWidth(const char *s, float sx) {
	float w = 0;
	const unsigned char *p = (const unsigned char *)s;
	while (*p && *p != '\n') w += adv(nextCp(&p)) * sx;
	return w;
}
static void glyph(int cp, float x, float y, float s, u32 col) {
	if (cp < 32 || cp >= 256) cp = '?';
	unsigned char *g = font[cp - 32] + 1;
	int W = (int)ceilf(32 * s), H = (int)ceilf(30 * s);
	for (int j = 0; j < H; j++)
		for (int i = 0; i < W; i++) {
			int sx = (int)(i / s), sy = (int)(j / s);
			if (sx >= 32 || sy >= 30) continue;
			float a = g[sy * 32 + sx] / 255.0f;
			if (a > 0) blend((int)(x + i), (int)(y + j), col, a);
		}
}
void C2D_TextGetDimensions(const C2D_Text *t, float sx, float sy, float *w, float *h) {
	float mw = 0;
	int lines = 1;
	const char *s = t->str;
	for (;;) {
		float lw = lineWidth(s, sx);
		if (lw > mw) mw = lw;
		const char *nl = strchr(s, '\n');
		if (!nl) break;
		s = nl + 1;
		lines++;
	}
	if (w) *w = mw;
	if (h) *h = lines * 30 * sy;
}
void C2D_DrawText(const C2D_Text *t, u32 flags, float x, float y, float z, float sx, float sy, ...) {
	(void)z;
	va_list ap;
	va_start(ap, sy);
	u32 col = 0xFF000000;
	float wrap = 1e9f;
	if (flags & C2D_WithColor) col = va_arg(ap, u32);
	if (flags & C2D_WordWrap) wrap = (float)va_arg(ap, double);
	va_end(ap);
	const unsigned char *p = (const unsigned char *)t->str;
	float yy = y;
	while (*p) {
		float lw = lineWidth((const char *)p, sx);
		float xx = x;
		if (flags & C2D_AlignRight) xx = x - lw;
		else if (flags & C2D_AlignCenter) xx = x - lw / 2;
		float start = xx;
		while (*p && *p != '\n') {
			if ((flags & C2D_WordWrap) && *p == ' ') {
				const unsigned char *q = p + 1;
				float ww = adv(' ') * sx;
				while (*q && *q != ' ' && *q != '\n') ww += adv(nextCp(&q)) * sx;
				if (xx + ww - start > wrap) { p++; xx = start; yy += 30 * sy; continue; }
			}
			int cp = nextCp(&p);
			glyph(cp, xx, yy, sx, col);
			xx += adv(cp) * sx;
		}
		if (*p == '\n') p++;
		yy += 30 * sy;
	}
}

// ---------------- Frames
void C3D_Init(int n) { (void)n; }
void C3D_Fini(void) {}
void C3D_DepthTest(bool a, int b, int c) { (void)a; (void)b; (void)c; }
void C2D_Init(int n) {
	(void)n;
	FILE *f = fopen(getenv("SHIM_FONT"), "rb");
	if (f) { if (fread(font, 1, sizeof font, f)) {} fclose(f); }
	loadScript(getenv("SHIM_SCRIPT"));
	if (getenv("SHIM_SAVE")) saveDir = getenv("SHIM_SAVE");
	if (getenv("SHIM_OUT")) outDir = getenv("SHIM_OUT");
}
void C2D_Prepare(void) {}
void C2D_Fini(void) {}
C3D_RenderTarget *C2D_CreateScreenTarget(gfxScreen_t s, gfx3dSide_t side) { return s == GFX_BOTTOM ? &tBot : side ? &tTopR : &tTop; }
void C2D_SceneBegin(C3D_RenderTarget *t) {
	if (t->scr) { cur = &fbBot[0][0]; curW = 320; }
	else { cur = t->side ? &fbR[0][0] : &fbTop[0][0]; curW = 400; }
	curH = 240;
}
void C2D_TargetClear(C3D_RenderTarget *t, u32 c) {
	C2D_SceneBegin(t);
	for (int i = 0; i < curW * curH; i++) { cur[i][0] = (c & 255) / 255.0f; cur[i][1] = ((c >> 8) & 255) / 255.0f; cur[i][2] = ((c >> 16) & 255) / 255.0f; }
}
void C3D_FrameBegin(int f) { (void)f; }
void C3D_FrameEnd(int f) {
	(void)f;
	if (!shotName[0]) return;
	char p[512];
	snprintf(p, sizeof p, "%s/%s.ppm", outDir, shotName);
	FILE *o = fopen(p, "wb");
	fprintf(o, "P6 400 480 255\n");
	for (int y = 0; y < 480; y++)
		for (int x = 0; x < 400; x++) {
			float *c = NULL;
			static float black[3] = {0, 0, 0};
			if (y < 240) c = fbTop[y][x];
			else c = (x >= 40 && x < 360) ? fbBot[y - 240][x - 40] : black;
			for (int k = 0; k < 3; k++) fputc((int)(fminf(1, fmaxf(0, c[k])) * 255), o);
		}
	fclose(o);
	shotName[0] = 0;
}
void hidCircleRead(circlePosition *c) { c->dx = frame < stickUntil ? stickX : 0; c->dy = frame < stickUntil ? stickY : 0; }
