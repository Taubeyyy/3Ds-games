// FORGE – UI-Toolkit, Szenen-Stapel, Synthesizer (plattformunabhängig)
#include "forge/ui.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace forge {
namespace ui {

static Theme g_theme;
Theme &theme() { return g_theme; }

struct Region { int id; float x, y, w, h; bool scroll; };
static Region g_reg[160], g_regPrev[160];
static int g_nReg, g_nPrev;
static int g_pressed = -1, g_clicked = -1, g_last = -1;
static float g_holdT;
static bool g_drawingBottom;
static float g_pressX = -100, g_pressY = -100, g_pressAnim;

void beginFrame(Gfx &g) {
	g_drawingBottom = g.screen == Screen::Bottom;
	if (g_drawingBottom) g_nReg = 0;
}
static void addRegion(int id, float x, float y, float w, float h, bool scroll) {
	if (!g_drawingBottom || g_nReg >= 160) return;
	g_reg[g_nReg++] = {id, x, y, w, h, scroll};
}
void hit(int id, float x, float y, float w, float h) { addRegion(id, x, y, w, h, false); }
static const Region *findRegion(Vec2 p) {
	for (int i = g_nPrev - 1; i >= 0; i--) {
		const Region &r = g_regPrev[i];
		if (p.x >= r.x && p.x < r.x + r.w && p.y >= r.y && p.y < r.y + r.h) return &r;
	}
	return nullptr;
}
void process(const Input &in, float dt) {
	memcpy(g_regPrev, g_reg, sizeof(Region) * g_nReg);
	g_nPrev = g_nReg;
	g_clicked = -1;
	if (g_pressAnim > 0) g_pressAnim -= dt;
	if (in.down & KEY_TOUCH) {
		const Region *r = findRegion(in.touch);
		g_pressed = r ? r->id : -1;
		g_holdT = 0;
		g_pressX = in.touch.x; g_pressY = in.touch.y; g_pressAnim = 0.12f;
	} else if (in.touching && g_pressed >= 0) {
		g_holdT += dt;
		const Region *r = findRegion(in.touchStart);
		if (in.dragging && !(r && r->scroll)) g_pressed = -1; // Ziehen bricht Tippen ab
	}
	if (in.tapped && g_pressed >= 0) {
		const Region *r = findRegion(in.touch);
		if (r && r->id == g_pressed && !r->scroll) { g_clicked = g_pressed; g_last = g_pressed; }
	}
	if (!in.touching) g_pressed = -1;
}
bool clicked(int id) { return g_clicked == id; }
bool held(int id) { return g_pressed == id; }
float holdTime(int id) { return g_pressed == id ? g_holdT : 0; }
int lastClicked() { return g_last; }

void panel(Gfx &g, float x, float y, float w, float h, Color fill, Color border, float a) {
	g.rrect(x, y, w, h, g_theme.radius + 2, border.lighten(0.15f).alpha(a), border.darken(0.35f).alpha(a));
	g.rrect(x + 1.5f, y + 1.5f, w - 3, h - 3, g_theme.radius + 0.5f, fill.lighten(0.1f).alpha(a), fill.darken(0.2f).alpha(a));
}

bool button(Gfx &g, int id, float x, float y, float w, float h, const char *label, Color col, bool enabled, const Image *icon, float ts) {
	bool pressed = held(id) && enabled;
	float s = pressed ? 0.93f : 1;
	float cx = x + w / 2, cy = y + h / 2;
	float W = w * s, H = h * s;
	float X = cx - W / 2, Y = cy - H / 2;
	Color base = enabled ? col : Color(0x4A5080);
	g.rrect(X, Y + 2.5f, W, H, g_theme.radius, base.darken(0.5f), base.darken(0.6f));
	g.rrect(X, Y, W, H, g_theme.radius, base.lighten(0.25f), base.darken(0.12f));
	g.rrect(X + 3, Y + 2, W - 6, H * 0.4f, g_theme.radius - 2, Color(0xFFFFFF, enabled ? 0.33f : 0.1f), Color(0xFFFFFF, enabled ? 0.05f : 0.02f));
	float lum = (((base.hex >> 16) & 255) * 0.3f + ((base.hex >> 8) & 255) * 0.59f + (base.hex & 255) * 0.11f) / 255;
	Color tc = !enabled ? Color(0xC0C4DC) : lum > 0.62f ? Color(0x1A1A30) : Color(0xFFFFFF);
	float tx = cx;
	if (icon && icon->valid()) {
		float isz = fminf(H * 0.62f, 20) / fmaxf(icon->w, icon->height());
		if (!label || !label[0]) g.image(*icon, cx, cy, isz, isz);
		else {
			float tw = fminf(g.textWidth(label, ts), W - 34);
			float iw = icon->w * isz;
			float total = tw + iw + 5;
			g.image(*icon, cx - total / 2 + iw / 2, cy, isz, isz);
			tx = cx + iw / 2 + 2.5f;
		}
	}
	if (label && label[0]) g.text(tx, cy - 15 * ts + 1, ts, tc, label, ALIGN_CENTER, W - 12 - (icon ? 22 : 0));
	addRegion(id, x, y, w, h, false);
	return enabled && clicked(id);
}

bool toggle(Gfx &g, int id, float x, float y, const char *label, bool on, float w) {
	panel(g, x, y, w, 28, g_theme.panel, g_theme.border);
	g.text(x + 10, y + 6, 0.42f, g_theme.text, label, ALIGN_LEFT, w - 70);
	float sx = x + w - 52, sy = y + 6;
	g.rrect(sx, sy, 42, 16, 8, on ? g_theme.good : Color(0x2A3166), on ? g_theme.good.darken(0.2f) : Color(0x1E2450));
	g.circle(on ? sx + 34 : sx + 8, sy + 8, 6.5f, Color(0xFFFFFF));
	addRegion(id, x, y, w, 28, false);
	return clicked(id);
}

void bar(Gfx &g, float x, float y, float w, float h, float p, Color c1, Color c2) {
	p = clampf(p, 0, 1);
	g.rrect(x, y, w, h, h / 2, Color(0x0A0E28), Color(0x1A2048));
	if (p > 0.02f) g.rrect(x + 1, y + 1, (w - 2) * p, h - 2, (h - 2) / 2, c1.lighten(0.25f), c2);
}

void scrollArea(Scroll &s, int id, float x, float y, float w, float h) {
	s.top = y;
	s.bottom = y + h;
	s.left = x;
	s.right = x + w;
	addRegion(id, x, y, w, h, true);
}
void scrollUpdate(Scroll &s, int id, const Input &in, float dt) {
	(void)id;
	bool inside = in.touchStart.x >= s.left && in.touchStart.x < s.right && in.touchStart.y >= s.top && in.touchStart.y < s.bottom;
	if (!in.touching) s.grabbing = false;
	else if (in.dragging && inside && fabsf(in.touch.y - in.touchStart.y) > fabsf(in.touch.x - in.touchStart.x) * 0.7f) s.grabbing = true;
	if (s.grabbing) {
		s.pos -= in.touchDelta.y;
		if (dt > 0) s.vel = s.vel * 0.5f - in.touchDelta.y / dt * 0.5f;
	} else {
		s.pos += s.vel * dt;
		s.vel *= powf(0.04f, dt);
		if (fabsf(s.vel) < 4) s.vel = 0;
		if (s.pos < 0) { s.pos += -s.pos * fminf(1, dt * 14); s.vel = 0; if (s.pos > -0.5f) s.pos = 0; }
		if (s.pos > s.max) { s.pos += (s.max - s.pos) * fminf(1, dt * 14); s.vel = 0; }
	}
	if (in.holding(KEY_DDOWN)) s.pos += 240 * dt;
	if (in.holding(KEY_DUP)) s.pos -= 240 * dt;
}

struct Toast { char title[48], msg[96]; Color col; };
static Toast g_toasts[8];
static int g_tHead, g_tTail;
static float g_tTime;
void toast(const char *title, const char *msg, Color col) {
	int n = (g_tHead + 1) % 8;
	if (n == g_tTail) g_tTail = (g_tTail + 1) % 8;
	snprintf(g_toasts[g_tHead].title, 48, "%s", title);
	snprintf(g_toasts[g_tHead].msg, 96, "%s", msg);
	g_toasts[g_tHead].col = col;
	g_tHead = n;
}
void updateToasts(float dt) {
	if (g_tTail == g_tHead) return;
	g_tTime += dt;
	if (g_tTime > 2.8f) { g_tTime = 0; g_tTail = (g_tTail + 1) % 8; }
}
void drawToasts(Gfx &g, float y) {
	if (g_tTail == g_tHead) return;
	const Toast &t = g_toasts[g_tTail];
	float a = fminf(1, fminf(g_tTime * 5, (2.8f - g_tTime) * 5));
	float w = g.width - 60;
	float x = 30;
	float yy = y + (1 - a) * 16;
	g.rrect(x, yy, w, 40, 10, t.col.darken(0.65f).alpha(0.92f * a), t.col.darken(0.8f).alpha(0.92f * a), -2);
	g.rect(x + 8, yy, w - 16, 2.5f, t.col.alpha(a), -2);
	g.text(g.width / 2.0f, yy + 3, 0.5f, t.col.alpha(a), t.title, ALIGN_CENTER, w - 16, true, -2);
	g.text(g.width / 2.0f, yy + 21, 0.38f, Color(0xFFFFFF, a), t.msg, ALIGN_CENTER, w - 16, true, -2);
}

} // namespace ui

// ------------------------------------------------------------------ Szenen
void SceneStack::push(Scene *s, Transition t) { pending = s; op = 1; tr = t; outPhase = t != Transition::None && n > 0; this->t = 0; if (!outPhase) apply(); }
void SceneStack::pop(Transition t) { pending = nullptr; op = 2; tr = t; outPhase = t != Transition::None; this->t = 0; if (!outPhase) apply(); }
void SceneStack::replace(Scene *s, Transition t) { pending = s; op = 3; tr = t; outPhase = t != Transition::None && n > 0; this->t = 0; if (!outPhase) apply(); }
void SceneStack::apply() {
	if (op == 1 && n < 16) { stack[n++] = pending; pending->enter(); }
	else if (op == 2 && n > 0) { stack[--n]->exit(); if (n) stack[n - 1]->enter(); }
	else if (op == 3) { if (n) stack[n - 1]->exit(); else n = 1; stack[n - 1] = pending; pending->enter(); }
	op = 0;
	outPhase = false;
	t = tr == Transition::None ? 0 : 1; // Einblende-Phase
}
void SceneStack::update(float dt, const Input &in) {
	const float D = 0.22f;
	if (outPhase) {
		t += dt / D;
		if (t >= 1) apply();
		return; // waehrend des Ausblendens keine Eingabe
	}
	if (t > 0) t = fmaxf(0, t - dt / D);
	if (n) stack[n - 1]->update(dt, in);
}
void SceneStack::draw(Gfx &g) {
	if (!n) return;
	float k = ease::inOutSine(clampf(t, 0, 1));
	if (tr == Transition::SlideLeft) g.ox = -k * g.width * (outPhase ? 1 : -1) * 0.3f;
	if (tr == Transition::SlideRight) g.ox = k * g.width * (outPhase ? 1 : -1) * 0.3f;
	stack[n - 1]->draw(g);
	g.ox = 0;
	if (k > 0.001f) g.rect(0, 0, g.width, g.height, Color(0x000000, k));
}

// ------------------------------------------------------------------ Synthesizer
namespace synth {
Sound make(const Params &p, u32 rate) {
	u32 n = u32(p.dur * rate);
	Sound s = audio::make(n, rate);
	if (!s.data) return s;
	float ph = 0, lp = 0;
	u32 seed = 12345;
	for (u32 i = 0; i < n; i++) {
		float t = float(i) / rate, q = t / p.dur;
		float f = p.freq + (p.freqEnd - p.freq) * q;
		if (p.arpTime > 0) f *= powf(2, floorf(t / p.arpTime) * p.arpStep / 12);
		if (p.vibrato > 0) f *= 1 + p.vibrato * sinf(t * p.vibratoRate * 2 * PI);
		ph += 2 * PI * f / rate;
		if (ph > 2 * PI * 64) ph -= 2 * PI * 64;
		seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
		float nz = (seed & 0xFFFF) / 32768.0f - 1;
		float v = 0;
		float fr = fmodf(ph / (2 * PI), 1);
		switch (p.wave) {
		case SINE: v = sinf(ph); break;
		case SQUARE: v = fr < 0.5f ? 0.6f : -0.6f; break;
		case SAW: v = (fr * 2 - 1) * 0.6f; break;
		case TRIANGLE: v = (fr < 0.5f ? fr * 4 - 1 : 3 - fr * 4); break;
		case NOISE: v = nz; break;
		}
		v = v * (1 - p.noiseMix) + nz * p.noiseMix;
		lp += (v - lp) * p.lowpass;
		float env = fminf(1, t / fmaxf(p.attack, 1e-4f)) * expf(-t * p.decay) * (1 - q * 0.2f);
		float out = clampf(lp * env * p.volume, -1, 1);
		s.data[i] = s16(out * 30000);
	}
	audio::commit(s);
	return s;
}
Sound click() { Params p; p.freq = 1200; p.freqEnd = 900; p.dur = 0.05f; p.decay = 70; p.volume = 0.5f; return make(p); }
Sound coin() { Params p; p.wave = SQUARE; p.freq = 988; p.freqEnd = 988; p.arpStep = 5; p.arpTime = 0.06f; p.dur = 0.22f; p.decay = 10; p.lowpass = 0.4f; p.volume = 0.55f; return make(p); }
Sound hit() { Params p; p.wave = NOISE; p.freq = 200; p.dur = 0.12f; p.decay = 30; p.lowpass = 0.35f; p.volume = 0.9f; return make(p); }
Sound explosion() { Params p; p.wave = NOISE; p.dur = 0.8f; p.decay = 4.5f; p.lowpass = 0.08f; p.volume = 1; return make(p); }
Sound powerup() { Params p; p.wave = TRIANGLE; p.freq = 440; p.freqEnd = 880; p.arpStep = 4; p.arpTime = 0.05f; p.dur = 0.45f; p.decay = 3; p.volume = 0.6f; return make(p); }
Sound laser() { Params p; p.wave = SAW; p.freq = 1800; p.freqEnd = 300; p.dur = 0.18f; p.decay = 12; p.lowpass = 0.5f; p.volume = 0.45f; return make(p); }
Sound pickup() { Params p; p.freq = 660; p.freqEnd = 1320; p.dur = 0.12f; p.decay = 16; p.volume = 0.5f; return make(p); }
Sound error() { Params p; p.wave = SQUARE; p.freq = 160; p.freqEnd = 120; p.dur = 0.16f; p.decay = 10; p.lowpass = 0.3f; p.volume = 0.5f; return make(p); }
Sound whoosh() { Params p; p.wave = NOISE; p.dur = 0.3f; p.attack = 0.08f; p.decay = 6; p.lowpass = 0.15f; p.volume = 0.6f; return make(p); }
} // namespace synth

} // namespace forge
