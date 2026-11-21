// FORGE – UI-Toolkit (beide Bildschirme, Touch + Tasten)
// Muster: In draw() werden Widgets gezeichnet und ihre Trefferflächen registriert,
// in update() fragt das Spiel ui::clicked(id) ab (Treffer aus dem letzten Bild).
#pragma once
#include "forge/forge.hpp"

namespace forge {
namespace ui {

struct Theme {
	Color text{0xF4F6FF}, dim{0xA8B0D8}, panel{0x161C50}, border{0x4A5AB0};
	Color accent{0x4AD8FF}, good{0x5AE07A}, warn{0xFFD25A}, bad{0xFF5A6E};
	float radius = 7;
};
Theme &theme();

// Pro Bild: vor dem Zeichnen des unteren Bildschirms aufrufen
void beginFrame(Gfx &g);
// In update(): Eingabe verarbeiten (Tippen, Halten, Ziehen)
void process(const Input &in, float dt);

bool clicked(int id);           // wurde in diesem Schritt angetippt
bool held(int id);              // Finger liegt darauf
float holdTime(int id);         // wie lange gedrückt (für Wiederholung)
int lastClicked();

// Widgets
void panel(Gfx &g, float x, float y, float w, float h, Color fill, Color border, float alpha = 1);
bool button(Gfx &g, int id, float x, float y, float w, float h, const char *label, Color col, bool enabled = true,
	const Image *icon = nullptr, float textSize = 0.45f);
bool toggle(Gfx &g, int id, float x, float y, const char *label, bool on, float w = 300);
void bar(Gfx &g, float x, float y, float w, float h, float p, Color c1, Color c2);
void hit(int id, float x, float y, float w, float h); // nur Trefferfläche

// Scroll-Liste mit Trägheit: Zustand pro Liste
struct Scroll {
	float pos = 0, vel = 0, max = 0;
	float top = 0, bottom = 240, left = 0, right = 320;
	bool grabbing = false;
	void setContent(float contentH) { max = contentH > bottom - top ? contentH - (bottom - top) : 0; }
};
// Registriert den Listenbereich als Zieh-Fläche; in update() Scroll::update aufrufen
void scrollArea(Scroll &s, int id, float x, float y, float w, float h);
void scrollUpdate(Scroll &s, int id, const Input &in, float dt);

// Toasts (kurze Meldungen) und Dialoge
void toast(const char *title, const char *msg, Color col);
void drawToasts(Gfx &g, float y);
void updateToasts(float dt);

} // namespace ui

// ------------------------------------------------------------------ Szenen
class Scene {
public:
	virtual ~Scene() = default;
	virtual void enter() {}
	virtual void exit() {}
	virtual void update(float dt, const Input &in) { (void)dt; (void)in; }
	virtual void draw(Gfx &g) { (void)g; }
};

enum class Transition { None, Fade, SlideLeft, SlideRight };
class SceneStack {
public:
	void push(Scene *s, Transition t = Transition::Fade);
	void pop(Transition t = Transition::Fade);
	void replace(Scene *s, Transition t = Transition::Fade);
	void update(float dt, const Input &in);
	void draw(Gfx &g);
	Scene *top() const { return n ? stack[n - 1] : nullptr; }
private:
	Scene *stack[16] = {};
	int n = 0;
	Scene *pending = nullptr;
	int op = 0; // 1 push, 2 pop, 3 replace
	Transition tr = Transition::None;
	float t = 0;       // Fortschritt der Überblendung
	bool outPhase = false;
	void apply();
};

// ------------------------------------------------------------------ Synthesizer für Soundeffekte
namespace synth {
enum Wave { SINE, SQUARE, SAW, TRIANGLE, NOISE };
struct Params {
	Wave wave = SINE;
	float freq = 440, freqEnd = 440;   // Tonhöhen-Verlauf
	float dur = 0.2f;
	float attack = 0.005f, decay = 8;  // Hüllkurve (exp. Abfall)
	float vibrato = 0, vibratoRate = 0;
	float noiseMix = 0;                // Rauschen beimischen
	float lowpass = 1;                 // 0..1 (1 = aus)
	float volume = 0.8f;
	float arpStep = 0;                 // Arpeggio: Halbtöne pro Stufe
	float arpTime = 0;                 // Dauer pro Stufe
};
Sound make(const Params &p, u32 rate = 22050);
// Fertige Klänge
Sound click();
Sound coin();
Sound hit();
Sound explosion();
Sound powerup();
Sound laser();
Sound pickup();
Sound error();
Sound whoosh();
} // namespace synth

} // namespace forge
