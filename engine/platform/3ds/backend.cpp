// FORGE – 3DS-Backend (citro2d, hid, ndsp, sdmc)
#include <3ds.h>
#include <citro2d.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include "forge/forge.hpp"

u32 __stacksize__ = 256 * 1024; // grosser Stack fuer alle FORGE-Spiele

namespace forge {

static C2D_TextBuf g_tbuf;
static const Config *g_cfg;
static App *g_app;

// ------------------------------------------------------------------ Farben
Color Color::mix(Color o, float t) const {
	t = clampf(t, 0, 1);
	int r = int(((hex >> 16) & 255) * (1 - t) + ((o.hex >> 16) & 255) * t);
	int g = int(((hex >> 8) & 255) * (1 - t) + ((o.hex >> 8) & 255) * t);
	int b = int((hex & 255) * (1 - t) + (o.hex & 255) * t);
	return Color(u32((r << 16) | (g << 8) | b), a + (o.a - a) * t);
}
static inline u32 cc(Color c) {
	float a = clampf(c.a, 0, 1);
	return C2D_Color32((c.hex >> 16) & 255, (c.hex >> 8) & 255, c.hex & 255, u8(a * 255));
}
static inline u32 lerpC(u32 a, u32 b, float t) {
	t = clampf(t, 0, 1);
	u32 r = 0;
	for (int s = 0; s < 32; s += 8) {
		float va = (a >> s) & 255, vb = (b >> s) & 255;
		r |= (u32(va + (vb - va) * t) & 255) << s;
	}
	return r;
}

// ------------------------------------------------------------------ Bilder
struct SheetImpl { C2D_SpriteSheet sheet; int n; C2D_Image *imgs; };
static bool finishSheet(SpriteSheet *s, C2D_SpriteSheet sh) {
	if (!sh) return false;
	SheetImpl *im = new SheetImpl;
	im->sheet = sh;
	im->n = int(C2D_SpriteSheetCount(sh));
	im->imgs = new C2D_Image[im->n];
	for (int i = 0; i < im->n; i++) {
		im->imgs[i] = C2D_SpriteSheetGetImage(sh, i);
		C3D_TexSetFilter(im->imgs[i].tex, GPU_LINEAR, GPU_LINEAR);
	}
	s->impl = im;
	return true;
}
bool SpriteSheet::loadMem(const void *data, size_t size, const char *) { return finishSheet(this, C2D_SpriteSheetLoadFromMem(data, size)); }
bool SpriteSheet::loadFile(const char *path) { return finishSheet(this, C2D_SpriteSheetLoad(path)); }
Image SpriteSheet::get(int i) const {
	Image r;
	SheetImpl *im = (SheetImpl *)impl;
	if (!im || i < 0 || i >= im->n) return r;
	r.h = &im->imgs[i];
	r.w = im->imgs[i].subtex->width;
	r.h_ = im->imgs[i].subtex->height;
	return r;
}
int SpriteSheet::count() const { return impl ? ((SheetImpl *)impl)->n : 0; }
void SpriteSheet::free() {
	SheetImpl *im = (SheetImpl *)impl;
	if (!im) return;
	C2D_SpriteSheetFree(im->sheet);
	delete[] im->imgs;
	delete im;
	impl = nullptr;
}

// ------------------------------------------------------------------ Zeichnen
void Gfx::rect(float x, float y, float w, float h, Color c, float d) { C2D_DrawRectSolid(px(x, d), py(y), 0, w, h, cc(c)); }
void Gfx::rectGrad(float x, float y, float w, float h, Color t, Color b, float d) {
	C2D_DrawRectangle(px(x, d), py(y), 0, w, h, cc(t), cc(t), cc(b), cc(b));
}
void Gfx::rrect(float x, float y, float w, float h, float r, Color top, Color bottom, float d) {
	if (w <= 0 || h <= 0) return;
	x = px(x, d); y = py(y);
	if (r * 2 > h) r = h / 2;
	if (r * 2 > w) r = w / 2;
	if (r < 0) r = 0;
	u32 ct = cc(top), cb = cc(bottom);
	float cx = x + w / 2, cy = y + h / 2;
	u32 cm = lerpC(ct, cb, 0.5f);
	const int SEG = 5;
	float qx0 = 0, qy0 = 0;
	bool first = true;
	for (int c = 0; c < 5; c++) {
		int q = c % 4;
		float ox = (q == 0 || q == 3) ? x + w - r : x + r;
		float oy = (q < 2) ? y + h - r : y + r;
		for (int k = 0; k <= SEG; k++) {
			if (c == 4 && k > 0) break;
			float a = (q * 90 + k * 90.0f / SEG) * PI / 180;
			float qx = ox + cosf(a) * r, qy = oy + sinf(a) * r;
			if (!first) C2D_DrawTriangle(cx, cy, cm, qx0, qy0, lerpC(ct, cb, (qy0 - y) / h), qx, qy, lerpC(ct, cb, (qy - y) / h), 0);
			first = false;
			qx0 = qx; qy0 = qy;
		}
	}
}
void Gfx::circle(float x, float y, float r, Color c, float d) { if (r > 0.3f) C2D_DrawCircleSolid(px(x, d), py(y), 0, r, cc(c)); }
void Gfx::ring(float x, float y, float r, float th, Color c, int seg, float d) {
	u32 col = cc(c);
	x = px(x, d); y = py(y);
	for (int k = 0; k < seg; k++) {
		float a0 = 2 * PI * k / seg, a1 = 2 * PI * (k + 1) / seg;
		C2D_DrawLine(x + cosf(a0) * r, y + sinf(a0) * r, col, x + cosf(a1) * r, y + sinf(a1) * r, col, th, 0);
	}
}
void Gfx::line(float x0, float y0, float x1, float y1, float th, Color c, float d) {
	u32 col = cc(c);
	C2D_DrawLine(px(x0, d), py(y0), col, px(x1, d), py(y1), col, th, 0);
}
void Gfx::tri(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color ccl, float d) {
	C2D_DrawTriangle(px(a.x, d), py(a.y), cc(ca), px(b.x, d), py(b.y), cc(cb), px(c.x, d), py(c.y), cc(ccl), 0);
}
void Gfx::image(const Image &img, float x, float y, float sx, float sy, float angle, Color tint, float blend, float d) {
	if (!img.h) return;
	C2D_ImageTint t;
	C2D_PlainImageTint(&t, cc(tint), blend);
	C2D_DrawImageAtRotated(*(C2D_Image *)img.h, px(x, d), py(y), 0, angle, &t, sx, sy);
}
void Gfx::imageAt(const Image &img, float x, float y, float sx, float sy, Color tint, float blend, float d) {
	if (!img.h) return;
	C2D_ImageTint t;
	C2D_PlainImageTint(&t, cc(tint), blend);
	C2D_DrawImageAt(*(C2D_Image *)img.h, px(x, d), py(y), 0, &t, sx, sy);
}
static C2D_Text mkText(const char *s) {
	C2D_Text t;
	C2D_TextParse(&t, g_tbuf, s);
	C2D_TextOptimize(&t);
	return t;
}
static int alignFlag(int a) { return a == ALIGN_CENTER ? C2D_AlignCenter : a == ALIGN_RIGHT ? C2D_AlignRight : C2D_AlignLeft; }
void Gfx::text(float x, float y, float size, Color c, const char *s, int align, float maxw, bool shadow, float d) {
	C2D_Text t = mkText(s);
	float s2 = size;
	if (maxw > 0) {
		float w, h;
		C2D_TextGetDimensions(&t, size, size, &w, &h);
		if (w > maxw && w > 0) s2 = size * maxw / w;
	}
	float yy = py(y) + (size - s2) * 15;
	float xx = px(x, d);
	if (shadow) C2D_DrawText(&t, C2D_WithColor | alignFlag(align), xx + 1, yy + 1.2f, 0, s2, s2, cc(Color(0, c.a * 0.75f)));
	C2D_DrawText(&t, C2D_WithColor | alignFlag(align), xx, yy, 0, s2, s2, cc(c));
}
void Gfx::textf(float x, float y, float size, Color c, int align, const char *fmt, ...) {
	char b[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(b, sizeof b, fmt, ap);
	va_end(ap);
	text(x, y, size, c, b, align);
}
float Gfx::textWidth(const char *s, float size) {
	C2D_Text t = mkText(s);
	float w, h;
	C2D_TextGetDimensions(&t, size, size, &w, &h);
	return w;
}
void Gfx::textWrap(float x, float y, float size, Color c, float w, const char *s) {
	C2D_Text t = mkText(s);
	C2D_DrawText(&t, C2D_WithColor | C2D_WordWrap, px(x, 0), py(y), 0, size, size, cc(c), w);
}

// ------------------------------------------------------------------ Audio
static bool g_sndOk;
static ndspWaveBuf g_wb[24];
static int g_ch;
static float g_master = 1;
namespace audio {
bool available() { return g_sndOk; }
Sound make(u32 samples, u32 rate) {
	Sound s;
	s.data = (s16 *)linearAlloc(samples * sizeof(s16));
	s.samples = s.data ? samples : 0;
	s.rate = rate;
	if (s.data) memset(s.data, 0, samples * sizeof(s16));
	return s;
}
void commit(Sound &s) { if (s.data) DSP_FlushDataCache(s.data, s.samples * sizeof(s16)); }
void play(const Sound &s, float pitch, float vol, float pan) {
	if (!g_sndOk || !s.data) return;
	int ch = g_ch;
	g_ch = (g_ch + 1) % 20; // Kanaele 0..19 fuer Effekte, 20..23 fuer Musik
	ndspChnWaveBufClear(ch);
	ndspChnSetRate(ch, s.rate * pitch);
	float mix[12] = {0};
	float v = vol * g_master;
	mix[0] = v * clampf(1 - pan, 0, 1);
	mix[1] = v * clampf(1 + pan, 0, 1);
	ndspChnSetMix(ch, mix);
	memset(&g_wb[ch], 0, sizeof g_wb[ch]);
	g_wb[ch].data_vaddr = s.data;
	g_wb[ch].nsamples = s.samples;
	ndspChnWaveBufAdd(ch, &g_wb[ch]);
}
void setMasterVolume(float v) { g_master = v; }
static const int MUSIC_CH = 20;
void music(const Sound &s, float vol) {
	if (!g_sndOk || !s.data) return;
	ndspChnWaveBufClear(MUSIC_CH);
	ndspChnSetRate(MUSIC_CH, s.rate);
	memset(&g_wb[MUSIC_CH], 0, sizeof g_wb[MUSIC_CH]);
	g_wb[MUSIC_CH].data_vaddr = s.data;
	g_wb[MUSIC_CH].nsamples = s.samples;
	g_wb[MUSIC_CH].looping = true;
	musicVolume(vol);
	ndspChnWaveBufAdd(MUSIC_CH, &g_wb[MUSIC_CH]);
}
void musicVolume(float vol) {
	if (!g_sndOk) return;
	float mix[12] = {0};
	mix[0] = mix[1] = clampf(vol, 0, 1) * g_master;
	ndspChnSetMix(MUSIC_CH, mix);
}
void stopMusic() { if (g_sndOk) ndspChnWaveBufClear(MUSIC_CH); }
} // namespace audio

// ------------------------------------------------------------------ Speichern
static void savePath(char *out, size_t n, const char *name, const char *ext) {
	snprintf(out, n, "sdmc:/3ds/%s/%s.%s", g_cfg->name, name, ext);
}
struct SaveHeader { u32 magic, version, size, sum; };
static u32 checksum(const void *d, size_t n) {
	const u8 *p = (const u8 *)d;
	u32 h = 2166136261u;
	for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 16777619u;
	return h;
}
namespace save {
bool write(const char *name, const void *data, size_t size, u32 version) {
	char dir[96], tmp[128], fin[128], bak[128];
	mkdir("sdmc:/3ds", 0777);
	snprintf(dir, sizeof dir, "sdmc:/3ds/%s", g_cfg->name);
	mkdir(dir, 0777);
	savePath(tmp, sizeof tmp, name, "tmp");
	savePath(fin, sizeof fin, name, "dat");
	savePath(bak, sizeof bak, name, "bak");
	FILE *f = fopen(tmp, "wb");
	if (!f) return false;
	SaveHeader h = {0x45475246u, version, u32(size), checksum(data, size)};
	bool ok = fwrite(&h, sizeof h, 1, f) == 1 && fwrite(data, 1, size, f) == size;
	fclose(f);
	if (!ok) return false;
	remove(bak);
	rename(fin, bak);
	return rename(tmp, fin) == 0;
}
static u32 readOne(const char *path, void *data, size_t size) {
	FILE *f = fopen(path, "rb");
	if (!f) return 0;
	SaveHeader h;
	u32 v = 0;
	if (fread(&h, sizeof h, 1, f) == 1 && h.magic == 0x45475246u) {
		void *buf = malloc(h.size);
		if (buf && fread(buf, 1, h.size, f) == h.size && checksum(buf, h.size) == h.sum) {
			memcpy(data, buf, h.size < size ? h.size : size);
			v = h.version;
		}
		::free(buf);
	}
	fclose(f);
	return v;
}
u32 read(const char *name, void *data, size_t size) {
	char fin[128], bak[128];
	savePath(fin, sizeof fin, name, "dat");
	savePath(bak, sizeof bak, name, "bak");
	u32 v = readOne(fin, data, size);
	return v ? v : readOne(bak, data, size);
}
} // namespace save

u64 nowMs() { return osGetTime(); }
double tickMs() { return svcGetSystemTick() / double(CPU_TICKS_PER_MSEC); }

// ------------------------------------------------------------------ Eingabe
static Input g_in;
static void readInput(float dt) {
	hidScanInput();
	Input &in = g_in;
	in.down = hidKeysDown();
	in.held = hidKeysHeld();
	in.up = hidKeysUp();
	circlePosition cp;
	hidCircleRead(&cp);
	in.stick = Vec2(cp.dx / 150.0f, -cp.dy / 150.0f);
	if (in.stick.len() < 0.15f) in.stick = Vec2();
	if (in.stick.len() > 1) in.stick = in.stick.norm();
	touchPosition tp;
	hidTouchRead(&tp);
	in.tapped = false;
	if (in.down & KEY_TOUCH) {
		in.touching = true;
		in.touch = in.touchStart = Vec2(tp.px, tp.py);
		in.touchDelta = Vec2();
		in.touchTime = 0;
		in.dragging = false;
	} else if ((in.held & KEY_TOUCH) && in.touching) {
		Vec2 p(tp.px, tp.py);
		in.touchDelta = p - in.touch;
		in.touch = p;
		in.touchTime += dt;
		if ((p - in.touchStart).len() > 8) in.dragging = true;
	}
	if (in.up & KEY_TOUCH) {
		in.tapped = in.touching && !in.dragging;
		in.touching = false;
		in.touchDelta = Vec2();
	}
}

static aptHookCookie g_hook;
static void onApt(APT_HookType h, void *) {
	if (g_app && (h == APTHOOK_ONSUSPEND || h == APTHOOK_ONSLEEP || h == APTHOOK_ONEXIT)) g_app->suspend();
}

// ------------------------------------------------------------------ Hauptschleife
int run(App &app, const Config &cfg) {
	g_cfg = &cfg;
	g_app = &app;
	gfxInitDefault();
	osSetSpeedupEnable(true);
	if (cfg.romfs) romfsInit();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 2); // Platz für viele 3D-Zeichenaufrufe pro Bild
	C2D_Init(cfg.maxObjects);
	C2D_Prepare();
	C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
	// Eigene Ziele mit 24-Bit-Tiefenpuffer (für gfx3d), Ausgabe wie citro2d
	const u32 xfer = GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
		GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);
	auto mk = [&](gfxScreen_t s, gfx3dSide_t side, int w) {
		C3D_RenderTarget *t = C3D_RenderTargetCreate(240, w, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
		C3D_RenderTargetSetOutput(t, s, side, xfer);
		return t;
	};
	C3D_RenderTarget *top = mk(GFX_TOP, GFX_LEFT, 400);
	C3D_RenderTarget *topR = mk(GFX_TOP, GFX_RIGHT, 400);
	C3D_RenderTarget *bot = mk(GFX_BOTTOM, GFX_LEFT, 320);
	g_tbuf = C2D_TextBufNew(8192);
	if (R_SUCCEEDED(ndspInit())) {
		g_sndOk = true;
		ndspSetOutputMode(NDSP_OUTPUT_STEREO);
		for (int ch = 0; ch < 24; ch++) {
			ndspChnReset(ch);
			ndspChnSetInterp(ch, NDSP_INTERP_LINEAR);
			ndspChnSetFormat(ch, NDSP_FORMAT_MONO_PCM16);
		}
	}
	gfxSet3D(cfg.stereo);
	aptHook(&g_hook, onApt, nullptr);
	app.init();
	const float STEP = 1.0f / 60;
	u64 last = osGetTime();
	float acc = 0;
	Gfx g;
	while (aptMainLoop() && !app.quit) {
		u64 now = osGetTime();
		float dt = now >= last ? (now - last) / 1000.0f : 0;
		last = now;
		if (dt > 0.1f) dt = 0.1f;
		acc += dt;
		readInput(dt);
		const u32 combo = KEY_L | KEY_R | KEY_SELECT;
		if (cfg.debugCombo && (g_in.held & combo) == combo && (g_in.down & combo)) debug::toggle();
		double t0 = tickMs();
		int steps = 0;
		while (acc >= STEP && steps < 4) {
			app.time += STEP;
			app.update(STEP, g_in);
			g_in.down = g_in.up = 0;
			g_in.tapped = false;
			acc -= STEP;
			steps++;
		}
		if (steps >= 4) acc = 0;
		double t1 = tickMs();
		app.stereoSlider = cfg.stereo ? osGet3DSliderState() : 0;
		C2D_TextBufClear(g_tbuf);
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		for (int e = 0; e < 2; e++) {
			if (e == 1 && app.stereoSlider <= 0.01f) break;
			C3D_RenderTarget *t = e ? topR : top;
			C2D_Flush();
			C3D_RenderTargetClear(t, C3D_CLEAR_ALL, 0x000000FF, 0);
			C2D_SceneBegin(t);
			g = Gfx();
			g.screen = Screen::Top; g.width = TOP_W; g.height = SCR_H;
			g.eye = app.stereoSlider > 0.01f ? (e ? 1.f : -1.f) : 0;
			g.stereo = app.stereoSlider;
			app.draw(g);
			if (debug::overlayOn()) debug::overlay(g);
		}
		C2D_Flush();
		C3D_RenderTargetClear(bot, C3D_CLEAR_ALL, 0x000000FF, 0);
		C2D_SceneBegin(bot);
		g = Gfx();
		g.screen = Screen::Bottom; g.width = BOT_W; g.height = SCR_H;
		app.draw(g);
		double t2 = tickMs();
		C3D_FrameEnd(0);
		debug::frameDone(dt * 1000, float(t1 - t0), float(t2 - t1), C3D_GetDrawingTime(), steps, u32(osGetMemRegionFree(MEMREGION_APPLICATION)), u32(linearSpaceFree()));
	}
	app.suspend();
	if (g_sndOk) ndspExit();
	C2D_TextBufDelete(g_tbuf);
	C2D_Fini();
	C3D_Fini();
	if (cfg.romfs) romfsExit();
	gfxExit();
	return 0;
}

} // namespace forge
