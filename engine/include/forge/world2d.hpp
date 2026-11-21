// FORGE – 2D-Welt: Kamera, Tilemap, Partikel, Physik (Box2D)
#pragma once
#include "forge/forge.hpp"

class b2World;
class b2Body;

namespace forge {

// ------------------------------------------------------------------ Kamera
struct Camera2D {
	Vec2 pos;                 // Weltposition in der Bildschirmmitte
	float zoom = 1;
	float shake = 0;          // 0..1, klingt automatisch ab
	Vec2 boundsMin{-1e9f, -1e9f}, boundsMax{1e9f, 1e9f};
	Vec2 shakeOff;
	void follow(Vec2 target, float dt, float stiffness = 6);
	void addShake(float s) { shake = shake > s ? shake : s; }
	void update(float dt, Rng &rng);
	// Welt → Bildschirm für den aktuellen Gfx-Kontext
	Vec2 toScreen(const Gfx &g, Vec2 w) const { return {(w.x - pos.x) * zoom + g.width * 0.5f + shakeOff.x, (w.y - pos.y) * zoom + g.height * 0.5f + shakeOff.y}; }
	Vec2 toWorld(const Gfx &g, Vec2 s) const { return {(s.x - g.width * 0.5f - shakeOff.x) / zoom + pos.x, (s.y - g.height * 0.5f - shakeOff.y) / zoom + pos.y}; }
	bool visible(const Gfx &g, Vec2 w, float r) const;
};

// ------------------------------------------------------------------ Tilemap
struct Tilemap {
	int w = 0, h = 0;
	float tile = 16;            // Pixel pro Kachel
	u8 *cells = nullptr;        // Kachel-ID (0 = leer)
	u8 solid[256] = {};         // welche IDs fest sind
	const Image *images = nullptr; // Bild je ID (images[id])
	void create(int w_, int h_, float tileSize);
	void destroy();
	u8 get(int x, int y) const { return (x < 0 || y < 0 || x >= w || y >= h) ? 0 : cells[y * w + x]; }
	void set(int x, int y, u8 v) { if (x >= 0 && y >= 0 && x < w && y < h) cells[y * w + x] = v; }
	bool solidAt(Vec2 p) const { return solid[get(int(p.x / tile), int(p.y / tile))]; }
	void draw(Gfx &g, const Camera2D &cam, float depth = 2) const;
};

// ------------------------------------------------------------------ Partikel
struct Particle { Vec2 p, v; float life, maxLife, size0, size1, rot, spin; Color c0, c1; const Image *img; float drag, gravity; };
struct EmitDesc {
	const Image *img = nullptr;
	int count = 10;
	float speedMin = 30, speedMax = 120, angle = 0, spread = 2 * PI;
	float lifeMin = 0.4f, lifeMax = 0.9f;
	float size0 = 1, size1 = 0;        // Skalierung über die Lebenszeit
	Color c0{0xFFFFFF}, c1{0xFFFFFF, 0};
	float gravity = 0, drag = 2, spin = 0;
};
struct Particles {
	static constexpr int MAX = 512;
	Particle p[MAX];
	int next = 0;
	Rng rng{99};
	void emit(Vec2 at, const EmitDesc &d);
	void update(float dt);
	void draw(Gfx &g, const Camera2D *cam = nullptr, float depth = -1) const;
};

// ------------------------------------------------------------------ Physik (Box2D, Einheiten in Pixeln)
namespace phys {
constexpr float PPM = 32; // Pixel pro Meter
enum BodyType { STATIC, KINEMATIC, DYNAMIC };
struct Contact { int a, b; bool begin; }; // Benutzer-IDs der beteiligten Körper

class World {
public:
	b2World *w = nullptr;
	void create(Vec2 gravityPx);
	void destroy();
	void step(float dt);
	b2Body *box(Vec2 center, Vec2 size, BodyType t, int userId, float density = 1, float friction = 0.4f, bool sensor = false, bool fixedRot = false);
	b2Body *circle(Vec2 center, float r, BodyType t, int userId, float density = 1, float friction = 0.4f, bool sensor = false);
	void remove(b2Body *b);
	// Feste Kacheln zu wenigen Rechtecken zusammenfassen und als statische Körper anlegen
	void addTilemap(const Tilemap &m, int userId = -1);
	// Ereignisse seit dem letzten step()
	int contacts(const Contact **out) const;
	// Strahl: true bei Treffer, Punkt/Normale in Pixeln
	bool raycast(Vec2 a, Vec2 b, Vec2 *hit, Vec2 *normal) const;
	void debugDraw(Gfx &g, const Camera2D &cam) const;
};
Vec2 pos(const b2Body *b);
float angle(const b2Body *b);
Vec2 vel(const b2Body *b);
void setVel(b2Body *b, Vec2 v);
void impulse(b2Body *b, Vec2 imp);
int userId(const b2Body *b);
} // namespace phys

} // namespace forge
