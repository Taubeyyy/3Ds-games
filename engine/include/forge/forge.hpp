// FORGE – 3DS-Engine (öffentliche API)
// Spiele binden nur diese Datei ein. Backends: platform/3ds (citro2d/3d) und platform/pc (Software, headless).
#pragma once
#include <cmath>
#include <cstdint>
#include <cstddef>

namespace forge {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s16 = int16_t;

constexpr float PI = 3.14159265f;
constexpr int TOP_W = 400, BOT_W = 320, SCR_H = 240;

// ------------------------------------------------------------------ Mathe
struct Vec2 {
	float x = 0, y = 0;
	constexpr Vec2() = default;
	constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}
	Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
	Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
	Vec2 operator*(float s) const { return {x * s, y * s}; }
	Vec2 operator/(float s) const { return {x / s, y / s}; }
	Vec2 &operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
	Vec2 &operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
	Vec2 &operator*=(float s) { x *= s; y *= s; return *this; }
	float len() const { return std::sqrt(x * x + y * y); }
	float len2() const { return x * x + y * y; }
	Vec2 norm() const { float l = len(); return l > 1e-6f ? Vec2{x / l, y / l} : Vec2{}; }
	float dot(Vec2 o) const { return x * o.x + y * o.y; }
	static Vec2 fromAngle(float a) { return {std::cos(a), std::sin(a)}; }
};
inline float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float approach(float v, float target, float rate, float dt) { return v + (target - v) * (1 - std::exp(-rate * dt)); }
namespace ease {
inline float outBack(float t) { float c = 1.70158f; t -= 1; return 1 + (c + 1) * t * t * t + c * t * t; }
inline float outCubic(float t) { t = 1 - t; return 1 - t * t * t; }
inline float inOutSine(float t) { return 0.5f - 0.5f * std::cos(PI * t); }
}

// Zufall (xorshift, deterministisch pro Seed)
struct Rng {
	u32 s = 2463534242u;
	explicit Rng(u32 seed = 1) : s(seed ? seed : 1) {}
	u32 next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
	float f() { return (next() & 0xFFFFFF) / float(0x1000000); }
	float range(float a, float b) { return a + (b - a) * f(); }
	int irange(int a, int b) { return a + int(next() % u32(b - a + 1)); }
};

// ------------------------------------------------------------------ Farben (Hex 0xRRGGBB + Alpha)
struct Color {
	u32 hex = 0xFFFFFF;
	float a = 1;
	constexpr Color() = default;
	constexpr Color(u32 h, float alpha = 1) : hex(h), a(alpha) {}
	Color alpha(float al) const { return {hex, a * al}; }
	Color mix(Color o, float t) const;
	Color lighten(float t) const { return mix(Color(0xFFFFFF, a), t); }
	Color darken(float t) const { return mix(Color(0x000000, a), t); }
};

// ------------------------------------------------------------------ Eingabe
enum Key : u32 {
	KEY_A = 1u << 0, KEY_B = 1u << 1, KEY_SELECT = 1u << 2, KEY_START = 1u << 3, KEY_DRIGHT = 1u << 4, KEY_DLEFT = 1u << 5,
	KEY_DUP = 1u << 6, KEY_DDOWN = 1u << 7, KEY_R = 1u << 8, KEY_L = 1u << 9, KEY_X = 1u << 10, KEY_Y = 1u << 11,
	KEY_ZL = 1u << 14, KEY_ZR = 1u << 15, KEY_TOUCH = 1u << 20
};
struct Input {
	u32 down = 0, held = 0, up = 0;
	Vec2 stick;              // Circle Pad, -1..1 (y nach unten positiv)
	bool touching = false;   // Finger auf dem Touchscreen
	bool tapped = false;     // gerade losgelassen ohne Ziehen
	Vec2 touch, touchStart, touchDelta;
	float touchTime = 0;     // Dauer der aktuellen Berührung
	bool dragging = false;
	bool pressed(u32 k) const { return down & k; }
	bool holding(u32 k) const { return held & k; }
};

// ------------------------------------------------------------------ Grafik
enum class Screen { Top, Bottom };
struct Image { void *h = nullptr; int w = 0, h_ = 0; bool valid() const { return h != nullptr; } int height() const { return h_; } };

// Eingebettetes Sheet aus gfx/<name>.t3s laden: sheet.loadMem(FORGE_SHEET(name))
#define FORGE_SHEET(name) name##_t3x, name##_t3x_size, #name

class SpriteSheet {
public:
	bool loadMem(const void *data, size_t size, const char *debugName);
	bool loadFile(const char *path); // z. B. "romfs:/gfx/player.t3x"
	Image get(int i) const;
	int count() const;
	void free();
	void *impl = nullptr;
};

enum Align { ALIGN_LEFT = 0, ALIGN_CENTER = 1, ALIGN_RIGHT = 2 };

// 2D-Zeichen-API (pro Bildschirm/Auge). Tiefe (depth) erzeugt Stereo-3D: >0 hinter dem Bildschirm, <0 davor.
class Gfx {
public:
	Screen screen = Screen::Top;
	int width = TOP_W, height = SCR_H;
	float eye = 0;        // -1 links, +1 rechts, 0 = kein Stereo
	float stereo = 0;     // Regler 0..1
	float ox = 0, oy = 0; // Kamera-Versatz (Bildschirm-Shake etc.)

	float px(float x, float depth) const { return x + ox + eye * depth * stereo; }
	float py(float y) const { return y + oy; }

	void rect(float x, float y, float w, float h, Color c, float depth = 0);
	void rectGrad(float x, float y, float w, float h, Color top, Color bottom, float depth = 0);
	void rrect(float x, float y, float w, float h, float r, Color top, Color bottom, float depth = 0);
	void circle(float x, float y, float r, Color c, float depth = 0);
	void ring(float x, float y, float r, float thick, Color c, int seg = 32, float depth = 0);
	void line(float x0, float y0, float x1, float y1, float thick, Color c, float depth = 0);
	void tri(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc, float depth = 0);
	// Bild mittig, mit Skalierung, Drehung, Tönung (blend 0 = Original, 1 = volle Farbe) und Alpha
	void image(const Image &img, float x, float y, float sx = 1, float sy = 1, float angle = 0, Color tint = Color(0xFFFFFF),
		float blend = 0, float depth = 0);
	// Bild an linker oberer Ecke (für Hintergründe)
	void imageAt(const Image &img, float x, float y, float sx = 1, float sy = 1, Color tint = Color(0xFFFFFF), float blend = 0, float depth = 0);
	// Text (Systemschrift). size 1.0 ≈ 30 px Zeilenhöhe. maxw > 0 verkleinert bei Bedarf.
	void text(float x, float y, float size, Color c, const char *s, int align = ALIGN_LEFT, float maxw = 0, bool shadow = true, float depth = 0);
	void textf(float x, float y, float size, Color c, int align, const char *fmt, ...);
	float textWidth(const char *s, float size);
	void textWrap(float x, float y, float size, Color c, float w, const char *s);
};

// ------------------------------------------------------------------ Audio
// Kurze Effekte aus PCM16-Mono-Puffern (z. B. prozedural erzeugt), Musik per Streaming (später).
struct Sound { s16 *data = nullptr; u32 samples = 0; u32 rate = 22050; };
namespace audio {
bool available();
Sound make(u32 samples, u32 rate = 22050);          // Puffer anlegen (linearer Speicher auf dem 3DS)
void commit(Sound &s);                               // nach dem Befüllen aufrufen
void play(const Sound &s, float pitch = 1, float vol = 1, float pan = 0);
void setMasterVolume(float v);
}

// ------------------------------------------------------------------ Speichern
namespace save {
// Atomar schreiben (tmp + umbenennen, Backup). name: z. B. "goo" → sdmc:/3ds/<app>/goo.dat
bool write(const char *name, const void *data, size_t size, u32 version);
// Liest maximal size Bytes; gibt gelesene Version zurück (0 = nicht vorhanden/kaputt)
u32 read(const char *name, void *data, size_t size);
}

// ------------------------------------------------------------------ App
struct Config {
	const char *name = "forge";   // Ordnername für Spielstände
	bool stereo = true;           // 3D-Effekt erlauben
	int maxObjects = 16000;       // Zeichenbudget pro Bild
	bool romfs = false;
	bool debugCombo = true;       // L+R+SELECT blendet die Profiler-Anzeige ein/aus
};

class App {
public:
	virtual ~App() = default;
	virtual void init() {}
	virtual void update(float dt, const Input &in) { (void)dt; (void)in; } // feste 60 Hz
	virtual void draw(Gfx &g) { (void)g; }                                 // pro Bildschirm/Auge aufgerufen
	virtual void suspend() {}                                              // HOME/Standby/Beenden: speichern!
	bool quit = false;
	float time = 0;      // Gesamtzeit
	float stereoSlider = 0;
};

int run(App &app, const Config &cfg);

// Zeit seit 1900 in Millisekunden (für Offline-Fortschritt)
u64 nowMs();
double tickMs(); // hochauflösende Zeit in Millisekunden (für Messungen)

// ------------------------------------------------------------------ Debug / Profiler
namespace debug {
struct Stats {
	float fps = 60, frameMs = 16.7f, updateMs = 0, drawMs = 0, gpuMs = 0;
	u32 memFree = 0, linearFree = 0; // Bytes (nur 3DS)
	int updatesPerFrame = 1;
};
const Stats &stats();
void begin(const char *name);   // Messbereich (verschachtelbar, max. 16 Namen)
void end();
struct Scope { explicit Scope(const char *n) { begin(n); } ~Scope() { end(); } };
void overlay(Gfx &g, float x = 4, float y = 30);  // FPS, Zeiten, Speicher, Bereiche als Balken
void log(const char *fmt, ...);
void drawLog(Gfx &g, float x, float y, int lines = 8);
bool overlayOn();                // per L+R+SELECT umgeschaltet (Config::debugCombo)
// intern (Backends)
void frameDone(float frameMs, float updMs, float drawMs, float gpuMs, int updates, u32 memFree, u32 linFree);
void toggle();
} // namespace debug
#define FORGE_PROFILE(name) ::forge::debug::Scope FORGE_CAT(_forge_prof_, __LINE__)(name)
#define FORGE_CAT2(a, b) a##b
#define FORGE_CAT(a, b) FORGE_CAT2(a, b)

} // namespace forge
