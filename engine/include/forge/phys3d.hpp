// FORGE – 3D-Physik (Modul phys3d: FORGE_MODULES += gfx3d phys3d), Bullet 2.87 darunter.
// Einheiten: Meter, Sekunden. Y zeigt nach oben.
#pragma once
#include "forge/gfx3d.hpp"

class btRigidBody;
class btDiscreteDynamicsWorld;

namespace forge {
namespace phys3d {

enum BodyType { STATIC, KINEMATIC, DYNAMIC };
using Body = btRigidBody;

struct Hit {
	Vec3 pos, normal;
	float frac = 1;    // 0..1 entlang des Strahls/Wegs
	int userId = -1;
	Body *body = nullptr;
};
struct Contact { int a, b; bool begin; Vec3 pos; float impulse; };

struct WorldImpl;
class World {
public:
	WorldImpl *impl = nullptr;
	btDiscreteDynamicsWorld *bt = nullptr; // direkter Bullet-Zugriff für Spezialfälle

	void create(Vec3 gravity = Vec3(0, -20, 0));
	void destroy();
	void step(float dt);                   // feste Unterschritte (1/60 s)

	Body *box(Vec3 center, Vec3 halfExt, BodyType t, int userId, float mass = 1, float friction = 0.6f, float restitution = 0.1f);
	Body *sphere(Vec3 center, float r, BodyType t, int userId, float mass = 1, float friction = 0.6f, float restitution = 0.2f);
	Body *capsule(Vec3 center, float r, float height, BodyType t, int userId, float mass = 1, float friction = 0.6f);
	Body *cylinder(Vec3 center, float r, float height, BodyType t, int userId, float mass = 1, float friction = 0.6f);
	// Statisches Dreiecksnetz (Level-Geometrie, Gelände). Das Mesh muss so lange leben wie der Körper.
	Body *mesh(const Mesh &m, const Mat4 &xf, int userId, float friction = 0.8f);
	void remove(Body *b);

	bool raycast(Vec3 from, Vec3 to, Hit *out, int ignoreUserId = -999) const;
	// Kugel entlang eines Wegs schieben (z. B. Projektile, Dash-Prüfung)
	bool sphereCast(Vec3 from, Vec3 to, float r, Hit *out, int ignoreUserId = -999) const;
	// Alle Körper, die eine Kugel berühren (Nahkampf-Treffer, Explosionen). Gibt Anzahl zurück.
	int overlapSphere(Vec3 c, float r, Body **out, int max) const;
	// Explosion: Impuls auf alle dynamischen Körper im Radius
	void explode(Vec3 c, float r, float strength);

	int contacts(const Contact **out) const; // Ereignisse seit dem letzten step()
	int bodyCount() const;
	// Umrisse aller Körper als 2D-Linien über die 3D-Szene (nach gfx3d::end aufrufen)
	void debugDraw(Gfx &g, const Camera3D &cam) const;
};

Vec3 pos(const Body *b);
Mat4 transform(const Body *b);            // für gfx3d::draw
Vec3 vel(const Body *b);
void setVel(Body *b, Vec3 v);
void setAngVel(Body *b, Vec3 w);
void impulse(Body *b, Vec3 imp, Vec3 at = Vec3());
void setPos(Body *b, Vec3 p, float yaw = 0); // teleportieren / kinematische Körper bewegen
int userId(const Body *b);
bool isDynamic(const Body *b);

// ------------------------------------------------------------------ Figur (kinematische Kapsel)
// Bewegen & Gleiten an Wänden, Stufen hochsteigen, an Boden haften, Hänge bis maxSlope,
// schiebt dynamische Körper weg. pos = Fußpunkt.
struct Character {
	Vec3 pos;
	Vec3 vel;                 // aktuelle Geschwindigkeit (y = vertikal)
	float radius = 0.35f;
	float height = 1.6f;      // Gesamthöhe
	float stepHeight = 0.35f;
	float maxSlopeDeg = 50;
	float accel = 60, airAccel = 20, gravity = 25, jumpSpeed = 9, maxFall = 30;
	float pushForce = 4;
	bool grounded = false;
	Vec3 groundNormal{0, 1, 0};
	int groundUserId = -1;
	float coyote = 0, jumpBuffer = 0;
	// wish = gewünschte waagerechte Geschwindigkeit (m/s), jump = Sprungtaste gedrückt (Flanke)
	void update(World &w, Vec3 wish, bool jumpPressed, float dt);
	void teleport(Vec3 p) { pos = p; vel = Vec3(); }
};

} // namespace phys3d
} // namespace forge
