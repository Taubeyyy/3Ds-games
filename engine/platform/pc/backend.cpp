// FORGE – PC-Backend (headless): Software-Rasterizer, Skript-Eingabe, Screenshots (PPM)
// Umgebungsvariablen: FORGE_SCRIPT (Eingabe-Skript), FORGE_OUT (Screenshot-Ordner), FORGE_FONT (Glyphen-Atlas),
//                     FORGE_GFX (Ordner mit <name>.bin-Sheets), FORGE_ROMFS (RomFS-Ersatz), FORGE_SAVE (Spielstände)
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "forge/forge.hpp"

namespace forge {

// ------------------------------------------------------------------ Framebuffer
static float fbTop[SCR_H][TOP_W][3], fbBot[SCR_H][BOT_W][3];
static float (*cur)[3];
static int curW, curH;
static unsigned char g_font[224][1 + 32 * 30];

Color Color::mix(Color o, float t) const {
	t = clampf(t, 0, 1);
	int r = int(((hex >> 16) & 255) * (1 - t) + ((o.hex >> 16) & 255) * t);
	int g = int(((hex >> 8) & 255) * (1 - t) + ((o.hex >> 8) & 255) * t);
	int b = int((hex & 255) * (1 - t) + (o.hex & 255) * t);
	return Color(u32((r << 16) | (g << 8) | b), a + (o.a - a) * t);
}

struct RGBAf { float r, g, b, a; };
static RGBAf toF(Color c) { return {((c.hex >> 16) & 255) / 255.f, ((c.hex >> 8) & 255) / 255.f, (c.hex & 255) / 255.f, clampf(c.a, 0, 1)}; }
static RGBAf lerpF(RGBAf a, RGBAf b, float t) { return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t}; }
static void blend(int x, int y, RGBAf c, float cov) {
	if (x < 0 || y < 0 || x >= curW || y >= curH) return;
	float a = c.a * cov;
	if (a <= 0) return;
	float *p = cur[y * curW + x];
	p[0] += (c.r - p[0]) * a; p[1] += (c.g - p[1]) * a; p[2] += (c.b - p[2]) * a;
}
static void triF(float x0, float y0, RGBAf c0, float x1, float y1, RGBAf c1, float x2, float y2, RGBAf c2) {
	float minx = fminf(x0, fminf(x1, x2)), maxx = fmaxf(x0, fmaxf(x1, x2));
	float miny = fminf(y0, fminf(y1, y2)), maxy = fmaxf(y0, fmaxf(y1, y2));
	float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
	if (fabsf(area) < 1e-6f) return;
	for (int j = int(floorf(miny)); j <= int(ceilf(maxy)); j++)
		for (int i = int(floorf(minx)); i <= int(ceilf(maxx)); i++) {
			float px_ = i + 0.5f, py_ = j + 0.5f;
			float w0 = ((x1 - px_) * (y2 - py_) - (x2 - px_) * (y1 - py_)) / area;
			float w1 = ((x2 - px_) * (y0 - py_) - (x0 - px_) * (y2 - py_)) / area;
			float w2 = 1 - w0 - w1;
			if (w0 < 0 || w1 < 0 || w2 < 0) continue;
			RGBAf c = {c0.r * w0 + c1.r * w1 + c2.r * w2, c0.g * w0 + c1.g * w1 + c2.g * w2, c0.b * w0 + c1.b * w1 + c2.b * w2, c0.a * w0 + c1.a * w1 + c2.a * w2};
			blend(i, j, c, 1);
		}
}

// ------------------------------------------------------------------ Bilder
struct PcImg { int w, h; std::vector<unsigned char> px; };
struct PcSheet { std::vector<PcImg> imgs; };
static bool loadBin(SpriteSheet *s, const std::string &path) {
	FILE *f = fopen(path.c_str(), "rb");
	if (!f) { fprintf(stderr, "[forge] Sheet fehlt: %s\n", path.c_str()); return false; }
	PcSheet *sh = new PcSheet;
	u32 n = 0;
	if (fread(&n, 4, 1, f) != 1) n = 0;
	sh->imgs.resize(n);
	for (u32 i = 0; i < n; i++) {
		u16 wh[2];
		if (fread(wh, 2, 2, f) != 2) break;
		sh->imgs[i].w = wh[0]; sh->imgs[i].h = wh[1];
		sh->imgs[i].px.resize(size_t(wh[0]) * wh[1] * 4);
		if (fread(sh->imgs[i].px.data(), 4, size_t(wh[0]) * wh[1], f)) {}
	}
	fclose(f);
	s->impl = sh;
	return true;
}
static std::string envOr(const char *k, const char *d) { const char *v = getenv(k); return v ? v : d; }
bool SpriteSheet::loadMem(const void *, size_t, const char *name) { return loadBin(this, envOr("FORGE_GFX", ".") + "/" + name + ".bin"); }
bool SpriteSheet::loadFile(const char *path) {
	std::string p(path);
	if (p.rfind("romfs:/", 0) == 0) p = envOr("FORGE_ROMFS", "romfs") + "/" + p.substr(7);
	size_t dot = p.rfind('.');
	if (dot != std::string::npos) p = p.substr(0, dot) + ".bin";
	return loadBin(this, p);
}
Image SpriteSheet::get(int i) const {
	Image r;
	PcSheet *sh = (PcSheet *)impl;
	if (!sh || i < 0 || i >= int(sh->imgs.size())) return r;
	r.h = &sh->imgs[i];
	r.w = sh->imgs[i].w;
	r.h_ = sh->imgs[i].h;
	return r;
}
int SpriteSheet::count() const { return impl ? int(((PcSheet *)impl)->imgs.size()) : 0; }
void SpriteSheet::free() { delete (PcSheet *)impl; impl = nullptr; }

static void drawImg(const PcImg *im, float cx, float cy, float ang, Color tint, float bl, float sx, float sy) {
	if (fabsf(sx) < 1e-4f || fabsf(sy) < 1e-4f) return;
	int w = im->w, h = im->h;
	float ca = cosf(ang), sa = sinf(ang);
	float hw = w * fabsf(sx) / 2, hh = h * fabsf(sy) / 2;
	float ex = fabsf(ca) * hw + fabsf(sa) * hh, ey = fabsf(sa) * hw + fabsf(ca) * hh;
	RGBAf t = toF(tint);
	for (int j = int(floorf(cy - ey)); j <= int(ceilf(cy + ey)); j++)
		for (int i = int(floorf(cx - ex)); i <= int(ceilf(cx + ex)); i++) {
			if (i < 0 || j < 0 || i >= curW || j >= curH) continue;
			float dx = i + 0.5f - cx, dy = j + 0.5f - cy;
			float lx = ca * dx + sa * dy, ly = -sa * dx + ca * dy;
			float u = lx / sx + w / 2.0f - 0.5f, v = ly / sy + h / 2.0f - 0.5f;
			if (u < -0.5f || v < -0.5f || u > w - 0.5f || v > h - 0.5f) continue;
			int u0 = int(floorf(u)), v0 = int(floorf(v));
			float fu = u - u0, fv = v - v0, acc[4] = {0, 0, 0, 0};
			for (int q = 0; q < 4; q++) {
				int uu = u0 + (q & 1), vv = v0 + (q >> 1);
				uu = uu < 0 ? 0 : uu >= w ? w - 1 : uu;
				vv = vv < 0 ? 0 : vv >= h ? h - 1 : vv;
				float wt = ((q & 1) ? fu : 1 - fu) * ((q >> 1) ? fv : 1 - fv);
				const unsigned char *pp = &im->px[(size_t(vv) * w + uu) * 4];
				for (int c = 0; c < 4; c++) acc[c] += pp[c] / 255.0f * wt;
			}
			RGBAf c = {acc[0] + (t.r - acc[0]) * bl, acc[1] + (t.g - acc[1]) * bl, acc[2] + (t.b - acc[2]) * bl, acc[3] * t.a};
			blend(i, j, c, 1);
		}
}

// ------------------------------------------------------------------ Zeichnen
void Gfx::rect(float x, float y, float w, float h, Color c, float d) { rectGrad(x, y, w, h, c, c, d); }
void Gfx::rectGrad(float x, float y, float w, float h, Color t, Color b, float d) {
	if (w <= 0 || h <= 0) return;
	x = px(x, d); y = py(y);
	RGBAf ct = toF(t), cb = toF(b);
	for (int j = int(floorf(y)); j < int(ceilf(y + h)); j++)
		for (int i = int(floorf(x)); i < int(ceilf(x + w)); i++) {
			float cx = i + 0.5f, cy = j + 0.5f;
			if (cx < x || cx >= x + w || cy < y || cy >= y + h) continue;
			blend(i, j, lerpF(ct, cb, (cy - y) / h), 1);
		}
}
void Gfx::rrect(float x, float y, float w, float h, float r, Color top, Color bottom, float d) {
	if (w <= 0 || h <= 0) return;
	x = px(x, d); y = py(y);
	if (r * 2 > h) r = h / 2;
	if (r * 2 > w) r = w / 2;
	RGBAf ct = toF(top), cb = toF(bottom);
	for (int j = int(floorf(y)); j < int(ceilf(y + h)); j++)
		for (int i = int(floorf(x)); i < int(ceilf(x + w)); i++) {
			float cx = i + 0.5f, cy = j + 0.5f;
			float qx = fmaxf(fmaxf(x + r - cx, cx - (x + w - r)), 0), qy = fmaxf(fmaxf(y + r - cy, cy - (y + h - r)), 0);
			float dist = sqrtf(qx * qx + qy * qy) - r;
			float cov = clampf(0.5f - dist, 0, 1);
			if (cx < x || cx >= x + w || cy < y || cy >= y + h) cov = 0;
			if (cov > 0) blend(i, j, lerpF(ct, cb, (cy - y) / h), cov);
		}
}
void Gfx::circle(float x, float y, float r, Color c, float d) {
	x = px(x, d); y = py(y);
	RGBAf cf = toF(c);
	for (int j = int(floorf(y - r)); j <= int(ceilf(y + r)); j++)
		for (int i = int(floorf(x - r)); i <= int(ceilf(x + r)); i++) {
			float dx = i + 0.5f - x, dy = j + 0.5f - y;
			float cov = clampf(r - sqrtf(dx * dx + dy * dy) + 0.5f, 0, 1);
			if (cov > 0) blend(i, j, cf, cov);
		}
}
void Gfx::line(float x0, float y0, float x1, float y1, float th, Color c, float d) {
	x0 = px(x0, d); x1 = px(x1, d); y0 = py(y0); y1 = py(y1);
	float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
	if (l < 1e-4f) return;
	float nx = -dy / l * th / 2, ny = dx / l * th / 2;
	RGBAf cf = toF(c);
	triF(x0 + nx, y0 + ny, cf, x1 + nx, y1 + ny, cf, x0 - nx, y0 - ny, cf);
	triF(x1 + nx, y1 + ny, cf, x1 - nx, y1 - ny, cf, x0 - nx, y0 - ny, cf);
}
void Gfx::ring(float x, float y, float r, float th, Color c, int seg, float d) {
	for (int k = 0; k < seg; k++) {
		float a0 = 2 * PI * k / seg, a1 = 2 * PI * (k + 1) / seg;
		line(x + cosf(a0) * r, y + sinf(a0) * r, x + cosf(a1) * r, y + sinf(a1) * r, th, c, d);
	}
}
void Gfx::tri(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color ccl, float d) {
	triF(px(a.x, d), py(a.y), toF(ca), px(b.x, d), py(b.y), toF(cb), px(c.x, d), py(c.y), toF(ccl));
}
void Gfx::image(const Image &img, float x, float y, float sx, float sy, float angle, Color tint, float bl, float d) {
	if (img.h) drawImg((const PcImg *)img.h, px(x, d), py(y), angle, tint, bl, sx, sy);
}
void Gfx::imageAt(const Image &img, float x, float y, float sx, float sy, Color tint, float bl, float d) {
	if (img.h) drawImg((const PcImg *)img.h, px(x, d) + img.w * sx / 2, py(y) + img.h_ * sy / 2, 0, tint, bl, sx, sy);
}

static int nextCp(const unsigned char **p) {
	const unsigned char *s = *p;
	int c = *s++;
	if (c >= 0xC0 && c < 0xE0) c = ((c & 31) << 6) | (*s++ & 63);
	else if (c >= 0xE0 && c < 0xF0) { c = ((c & 15) << 12) | ((s[0] & 63) << 6) | (s[1] & 63); s += 2; }
	*p = s;
	return c;
}
static float adv(int cp) { return (cp >= 32 && cp < 256) ? g_font[cp - 32][0] : 12; }
static float lineW(const char *s, float sc) {
	float w = 0;
	const unsigned char *p = (const unsigned char *)s;
	while (*p && *p != '\n') w += adv(nextCp(&p)) * sc;
	return w;
}
static void glyph(int cp, float x, float y, float s, RGBAf col) {
	if (cp < 32 || cp >= 256) cp = '?';
	unsigned char *g = g_font[cp - 32] + 1;
	int W = int(ceilf(32 * s)), H = int(ceilf(30 * s));
	for (int j = 0; j < H; j++)
		for (int i = 0; i < W; i++) {
			int sx = int(i / s), sy = int(j / s);
			if (sx >= 32 || sy >= 30) continue;
			float a = g[sy * 32 + sx] / 255.0f;
			if (a > 0) blend(int(x + i), int(y + j), col, a);
		}
}
static void drawStr(const char *s, float x, float y, float sc, RGBAf col, int align, float wrap) {
	const unsigned char *p = (const unsigned char *)s;
	float yy = y;
	while (*p) {
		float lw = lineW((const char *)p, sc);
		float xx = align == ALIGN_RIGHT ? x - lw : align == ALIGN_CENTER ? x - lw / 2 : x;
		float start = xx;
		while (*p && *p != '\n') {
			if (wrap > 0 && *p == ' ') {
				const unsigned char *q = p + 1;
				float ww = adv(' ') * sc;
				while (*q && *q != ' ' && *q != '\n') ww += adv(nextCp(&q)) * sc;
				if (xx + ww - start > wrap) { p++; xx = start; yy += 30 * sc; continue; }
			}
			int cp = nextCp(&p);
			glyph(cp, xx, yy, sc, col);
			xx += adv(cp) * sc;
		}
		if (*p == '\n') p++;
		yy += 30 * sc;
	}
}
void Gfx::text(float x, float y, float size, Color c, const char *s, int align, float maxw, bool shadow, float d) {
	float s2 = size;
	if (maxw > 0) {
		float w = lineW(s, size);
		if (w > maxw && w > 0) s2 = size * maxw / w;
	}
	float yy = py(y) + (size - s2) * 15, xx = px(x, d);
	if (shadow) drawStr(s, xx + 1, yy + 1.2f, s2, toF(Color(0, c.a * 0.75f)), align, 0);
	drawStr(s, xx, yy, s2, toF(c), align, 0);
}
void Gfx::textf(float x, float y, float size, Color c, int align, const char *fmt, ...) {
	char b[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(b, sizeof b, fmt, ap);
	va_end(ap);
	text(x, y, size, c, b, align);
}
float Gfx::textWidth(const char *s, float size) { return lineW(s, size); }
void Gfx::textWrap(float x, float y, float size, Color c, float w, const char *s) { drawStr(s, px(x, 0), py(y), size, toF(c), ALIGN_LEFT, w); }

// ------------------------------------------------------------------ Audio (stumm)
namespace audio {
bool available() { return false; }
Sound make(u32 samples, u32 rate) { Sound s; s.data = (s16 *)calloc(samples, sizeof(s16)); s.samples = samples; s.rate = rate; return s; }
void commit(Sound &) {}
void play(const Sound &, float, float, float) {}
void setMasterVolume(float) {}
} // namespace audio

// ------------------------------------------------------------------ Speichern
namespace save {
struct SaveHeader { u32 magic, version, size, sum; };
static u32 checksum(const void *d, size_t n) {
	const u8 *p = (const u8 *)d;
	u32 h = 2166136261u;
	for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 16777619u;
	return h;
}
bool write(const char *name, const void *data, size_t size, u32 version) {
	std::string p = envOr("FORGE_SAVE", ".") + "/" + name + ".dat";
	FILE *f = fopen(p.c_str(), "wb");
	if (!f) return false;
	SaveHeader h = {0x45475246u, version, u32(size), checksum(data, size)};
	fwrite(&h, sizeof h, 1, f);
	fwrite(data, 1, size, f);
	fclose(f);
	return true;
}
u32 read(const char *name, void *data, size_t size) {
	std::string p = envOr("FORGE_SAVE", ".") + "/" + name + ".dat";
	FILE *f = fopen(p.c_str(), "rb");
	if (!f) return 0;
	SaveHeader h;
	u32 v = 0;
	if (fread(&h, sizeof h, 1, f) == 1 && h.magic == 0x45475246u) {
		std::vector<unsigned char> buf(h.size);
		if (fread(buf.data(), 1, h.size, f) == h.size && checksum(buf.data(), h.size) == h.sum) {
			memcpy(data, buf.data(), h.size < size ? h.size : size);
			v = h.version;
		}
	}
	fclose(f);
	return v;
}
} // namespace save

// ------------------------------------------------------------------ Skript-Eingabe
struct Ev { int f; char cmd[16]; float a[5]; char s[64]; };
static std::vector<Ev> g_evs;
static int g_frame, g_quitAt = 1 << 30;
static u64 g_clock = 3900000000000ull;
static double g_speed = 1;
static u32 g_prevHeld;
static struct { bool on; int f0, n; float x0, y0, x1, y1; } g_drag;
static float g_stickX, g_stickY;
static int g_stickUntil;
static std::string g_shot;
u64 nowMs() { return g_clock; }

static void loadScript() {
	const char *p = getenv("FORGE_SCRIPT");
	if (!p) return;
	FILE *f = fopen(p, "r");
	if (!f) return;
	char line[256];
	while (fgets(line, sizeof line, f)) {
		Ev e{};
		if (line[0] == '#' || sscanf(line, "%d %15s", &e.f, e.cmd) < 2) continue;
		char *rest = strstr(line, e.cmd) + strlen(e.cmd);
		if (!strcmp(e.cmd, "shot") || !strcmp(e.cmd, "key")) sscanf(rest, "%63s", e.s);
		else sscanf(rest, "%f %f %f %f %f", &e.a[0], &e.a[1], &e.a[2], &e.a[3], &e.a[4]);
		g_evs.push_back(e);
	}
	fclose(f);
}
static u32 keyBit(const char *s) {
	struct { const char *n; u32 k; } t[] = {{"A", KEY_A}, {"B", KEY_B}, {"X", KEY_X}, {"Y", KEY_Y}, {"L", KEY_L}, {"R", KEY_R}, {"START", KEY_START},
		{"SELECT", KEY_SELECT}, {"UP", KEY_DUP}, {"DOWN", KEY_DDOWN}, {"LEFT", KEY_DLEFT}, {"RIGHT", KEY_DRIGHT}};
	for (auto &e : t) if (!strcmp(s, e.n)) return e.k;
	return 0;
}
static Input g_in;
static void readInput(float dt) {
	g_frame++;
	u32 held = 0, press = 0;
	for (auto &e : g_evs) {
		if (e.f != g_frame) continue;
		if (!strcmp(e.cmd, "tap")) g_drag = {true, g_frame, 3, e.a[0], e.a[1], e.a[0], e.a[1]};
		else if (!strcmp(e.cmd, "hold")) g_drag = {true, g_frame, int(e.a[2]), e.a[0], e.a[1], e.a[0], e.a[1]};
		else if (!strcmp(e.cmd, "drag")) g_drag = {true, g_frame, int(e.a[4]), e.a[0], e.a[1], e.a[2], e.a[3]};
		else if (!strcmp(e.cmd, "key")) press |= keyBit(e.s);
		else if (!strcmp(e.cmd, "stick")) { g_stickX = e.a[0]; g_stickY = e.a[1]; g_stickUntil = g_frame + int(e.a[2]); }
		else if (!strcmp(e.cmd, "shot")) g_shot = e.s;
		else if (!strcmp(e.cmd, "time")) g_clock += u64(e.a[0] * 1000);
		else if (!strcmp(e.cmd, "speed")) g_speed = e.a[0];
		else if (!strcmp(e.cmd, "quit")) g_quitAt = g_frame;
	}
	Vec2 tp;
	if (g_drag.on) {
		int k = g_frame - g_drag.f0;
		if (k < g_drag.n) {
			float p = g_drag.n > 1 ? float(k) / (g_drag.n - 1) : 0;
			tp = Vec2(g_drag.x0 + (g_drag.x1 - g_drag.x0) * p, g_drag.y0 + (g_drag.y1 - g_drag.y0) * p);
			held |= KEY_TOUCH;
		} else g_drag.on = false;
	}
	held |= press;
	Input &in = g_in;
	in.down = held & ~g_prevHeld;
	in.up = g_prevHeld & ~held;
	in.held = held;
	g_prevHeld = held;
	in.stick = g_frame < g_stickUntil ? Vec2(g_stickX, g_stickY) : Vec2();
	in.tapped = false;
	if (in.down & KEY_TOUCH) { in.touching = true; in.touch = in.touchStart = tp; in.touchDelta = Vec2(); in.touchTime = 0; in.dragging = false; }
	else if ((held & KEY_TOUCH) && in.touching) {
		in.touchDelta = tp - in.touch;
		in.touch = tp;
		in.touchTime += dt;
		if ((tp - in.touchStart).len() > 8) in.dragging = true;
	}
	if (in.up & KEY_TOUCH) { in.tapped = in.touching && !in.dragging; in.touching = false; in.touchDelta = Vec2(); }
	g_clock += u64(1000.0 / 60 * g_speed);
}

static void writeShot(const std::string &name) {
	std::string p = envOr("FORGE_OUT", ".") + "/" + name + ".ppm";
	FILE *o = fopen(p.c_str(), "wb");
	if (!o) return;
	fprintf(o, "P6 400 480 255\n");
	static const float black[3] = {0, 0, 0};
	for (int y = 0; y < 480; y++)
		for (int x = 0; x < 400; x++) {
			const float *c = y < 240 ? fbTop[y][x] : (x >= 40 && x < 360) ? fbBot[y - 240][x - 40] : black;
			for (int k = 0; k < 3; k++) fputc(int(clampf(c[k], 0, 1) * 255), o);
		}
	fclose(o);
}

int run(App &app, const Config &cfg) {
	(void)cfg;
	FILE *f = fopen(envOr("FORGE_FONT", "font.bin").c_str(), "rb");
	if (f) { if (fread(g_font, 1, sizeof g_font, f)) {} fclose(f); }
	loadScript();
	app.init();
	const float STEP = 1.0f / 60;
	Gfx g;
	while (!app.quit && g_frame < g_quitAt) {
		float dt = float(STEP * g_speed);
		if (dt > 0.1f) dt = 0.1f;
		readInput(dt);
		int steps = int(g_speed + 0.5);
		if (steps < 1) steps = 1;
		for (int s = 0; s < steps; s++) {
			app.time += STEP;
			app.update(STEP, g_in);
			g_in.down = g_in.up = 0;
			g_in.tapped = false;
		}
		cur = &fbTop[0][0]; curW = TOP_W; curH = SCR_H;
		memset(fbTop, 0, sizeof fbTop);
		g = Gfx(); g.screen = Screen::Top; g.width = TOP_W;
		app.draw(g);
		cur = &fbBot[0][0]; curW = BOT_W; curH = SCR_H;
		memset(fbBot, 0, sizeof fbBot);
		g = Gfx(); g.screen = Screen::Bottom; g.width = BOT_W;
		app.draw(g);
		if (!g_shot.empty()) { writeShot(g_shot); g_shot.clear(); }
	}
	app.suspend();
	return 0;
}

} // namespace forge
