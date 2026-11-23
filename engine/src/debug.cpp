// FORGE debug – Profiler-Bereiche, Bildzeit-Statistik, Overlay, Log
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "forge/forge.hpp"

namespace forge {
namespace debug {

static Stats g_st;
struct Sec { const char *name; double acc; float shown; };
static Sec g_sec[16];
static int g_nsec;
static int g_stack[8];
static double g_t0[8];
static int g_depth;
static bool g_on;
static char g_log[12][72];
static int g_logN;
static float g_hist[64];
static int g_histI;

const Stats &stats() { return g_st; }
bool overlayOn() { return g_on; }
void toggle() { g_on = !g_on; }
void setCmdBuf(float u) { g_st.cmdBuf += (u - g_st.cmdBuf) * 0.2f; if (u > g_st.cmdBuf) g_st.cmdBuf = u; }

void begin(const char *name) {
	if (g_depth >= 8) return;
	int k = 0;
	while (k < g_nsec && strcmp(g_sec[k].name, name)) k++;
	if (k == g_nsec) {
		if (g_nsec >= 16) return;
		g_sec[g_nsec++] = {name, 0, 0};
	}
	g_stack[g_depth] = k;
	g_t0[g_depth++] = tickMs();
}
void end() {
	if (g_depth <= 0) return;
	g_depth--;
	g_sec[g_stack[g_depth]].acc += tickMs() - g_t0[g_depth];
}

void frameDone(float frameMs, float updMs, float drawMs, float gpuMs, int updates, u32 memFree, u32 linFree) {
	const float k = 0.1f; // gleitender Mittelwert
	g_st.frameMs += (frameMs - g_st.frameMs) * k;
	g_st.fps = g_st.frameMs > 0.1f ? 1000 / g_st.frameMs : 60;
	g_st.updateMs += (updMs - g_st.updateMs) * k;
	g_st.drawMs += (drawMs - g_st.drawMs) * k;
	g_st.gpuMs += (gpuMs - g_st.gpuMs) * k;
	g_st.updatesPerFrame = updates;
	g_st.memFree = memFree;
	g_st.linearFree = linFree;
	for (int i = 0; i < g_nsec; i++) {
		g_sec[i].shown += (float(g_sec[i].acc) - g_sec[i].shown) * k;
		g_sec[i].acc = 0;
	}
	g_hist[g_histI++ & 63] = frameMs;
}

void overlay(Gfx &g, float x, float y) {
	float h = 64 + g_nsec * 11;
	g.rrect(x, y, 170, h, 6, Color(0x05070F, 0.82f), Color(0x0A0E1E, 0.82f), -3);
	Color fc = g_st.fps > 55 ? Color(0x5AE07A) : g_st.fps > 28 ? Color(0xFFD25A) : Color(0xFF5A5A);
	g.textf(x + 6, y + 3, 0.42f, fc, ALIGN_LEFT, "%2.0f FPS", g_st.fps);
	g.textf(x + 62, y + 6, 0.32f, Color(0xA8B0D8), ALIGN_LEFT, "%.1f ms  x%d upd", g_st.frameMs, g_st.updatesPerFrame);
	g.textf(x + 6, y + 20, 0.32f, Color(0xD8E0F8), ALIGN_LEFT, "CPU upd %.1f  draw %.1f  GPU %.1f", g_st.updateMs, g_st.drawMs, g_st.gpuMs);
	if (g_st.memFree) g.textf(x + 6, y + 32, 0.32f, Color(0x8A98C8), ALIGN_LEFT, "RAM %u KB  lin %u KB  GPU %2.0f%%", g_st.memFree / 1024, g_st.linearFree / 1024, g_st.cmdBuf * 100);
	// Bildzeit-Verlauf
	for (int i = 0; i < 64; i++) {
		float v = g_hist[(g_histI + i) & 63];
		float bh = v / 33.3f * 14;
		bh = bh > 14 ? 14 : bh;
		g.rect(x + 6 + i * 2.4f, y + 58 - bh, 2, bh, v > 17.5f ? Color(0xFF8A5A) : Color(0x4AB8FF), -3);
	}
	g.rect(x + 6, y + 58 - 7, 154, 1, Color(0xFFFFFF, 0.3f), -3); // 16,7-ms-Linie
	for (int i = 0; i < g_nsec; i++) {
		float yy = y + 62 + i * 11;
		float w = g_sec[i].shown / 16.7f * 70;
		w = w > 70 ? 70 : w;
		g.text(x + 6, yy, 0.3f, Color(0xC8D0F0), g_sec[i].name, ALIGN_LEFT, 70, false, -3);
		g.rect(x + 80, yy + 3, w, 6, Color(0x5AE07A, 0.9f), -3);
		g.textf(x + 152, yy, 0.3f, Color(0xA8B0D8), ALIGN_RIGHT, "%.2f", g_sec[i].shown);
	}
}

void log(const char *fmt, ...) {
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
#ifndef __3DS__
	fprintf(stderr, "[log] %s\n", buf);
#endif
	if (g_logN == 12) { memmove(g_log[0], g_log[1], sizeof g_log[0] * 11); g_logN = 11; }
	snprintf(g_log[g_logN++], sizeof g_log[0], "%s", buf);
}
void drawLog(Gfx &g, float x, float y, int lines) {
	int from = g_logN > lines ? g_logN - lines : 0;
	for (int i = from; i < g_logN; i++) g.text(x, y + (i - from) * 11, 0.32f, Color(0xD8E0F8), g_log[i], ALIGN_LEFT, 0, true, -3);
}

} // namespace debug
} // namespace forge
