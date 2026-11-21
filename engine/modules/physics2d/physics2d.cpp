// FORGE – 2D-Physik über Box2D 2.4 (optional: FORGE_MODULES += physics2d)
#include <box2d/box2d.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include "forge/world2d.hpp"

namespace forge {
namespace phys {

static inline b2Vec2 toM(Vec2 p) { return b2Vec2(p.x / PPM, p.y / PPM); }
static inline Vec2 toPx(b2Vec2 p) { return Vec2(p.x * PPM, p.y * PPM); }

struct Listener : b2ContactListener {
	std::vector<Contact> events;
	static int uid(b2Fixture *f) { return int(f->GetBody()->GetUserData().pointer) - 1; }
	void BeginContact(b2Contact *c) override { events.push_back({uid(c->GetFixtureA()), uid(c->GetFixtureB()), true}); }
	void EndContact(b2Contact *c) override { events.push_back({uid(c->GetFixtureA()), uid(c->GetFixtureB()), false}); }
};
static Listener g_listener;
static std::vector<Contact> g_frameEvents;

void World::create(Vec2 g) {
	destroy();
	w = new b2World(toM(g));
	w->SetContactListener(&g_listener);
}
void World::destroy() { delete w; w = nullptr; }
void World::step(float dt) {
	g_listener.events.clear();
	w->Step(dt, 8, 3);
	g_frameEvents = g_listener.events;
}
static b2Body *makeBody(b2World *w, Vec2 c, BodyType t, int uid, bool fixedRot) {
	b2BodyDef bd;
	bd.type = t == STATIC ? b2_staticBody : t == KINEMATIC ? b2_kinematicBody : b2_dynamicBody;
	bd.position = toM(c);
	bd.fixedRotation = fixedRot;
	bd.userData.pointer = uintptr_t(uid + 1);
	return w->CreateBody(&bd);
}
b2Body *World::box(Vec2 c, Vec2 size, BodyType t, int uid, float density, float friction, bool sensor, bool fixedRot) {
	b2Body *b = makeBody(w, c, t, uid, fixedRot);
	b2PolygonShape sh;
	sh.SetAsBox(size.x / 2 / PPM, size.y / 2 / PPM);
	b2FixtureDef fd;
	fd.shape = &sh; fd.density = density; fd.friction = friction; fd.isSensor = sensor;
	b->CreateFixture(&fd);
	return b;
}
b2Body *World::circle(Vec2 c, float r, BodyType t, int uid, float density, float friction, bool sensor) {
	b2Body *b = makeBody(w, c, t, uid, false);
	b2CircleShape sh;
	sh.m_radius = r / PPM;
	b2FixtureDef fd;
	fd.shape = &sh; fd.density = density; fd.friction = friction; fd.isSensor = sensor;
	b->CreateFixture(&fd);
	return b;
}
void World::remove(b2Body *b) { if (b) w->DestroyBody(b); }
void World::addTilemap(const Tilemap &m, int uid) {
	std::vector<u8> used(size_t(m.w) * m.h, 0);
	for (int y = 0; y < m.h; y++)
		for (int x = 0; x < m.w; x++) {
			if (used[y * m.w + x] || !m.solid[m.get(x, y)]) continue;
			int x1 = x;
			while (x1 + 1 < m.w && !used[y * m.w + x1 + 1] && m.solid[m.get(x1 + 1, y)]) x1++;
			int y1 = y;
			for (;;) {
				if (y1 + 1 >= m.h) break;
				bool ok = true;
				for (int k = x; k <= x1; k++) if (used[(y1 + 1) * m.w + k] || !m.solid[m.get(k, y1 + 1)]) { ok = false; break; }
				if (!ok) break;
				y1++;
			}
			for (int yy = y; yy <= y1; yy++) for (int xx = x; xx <= x1; xx++) used[yy * m.w + xx] = 1;
			Vec2 size((x1 - x + 1) * m.tile, (y1 - y + 1) * m.tile);
			box(Vec2(x * m.tile + size.x / 2, y * m.tile + size.y / 2), size, STATIC, uid, 0, 0.5f);
		}
}
int World::contacts(const Contact **out) const { *out = g_frameEvents.data(); return int(g_frameEvents.size()); }
struct RayCb : b2RayCastCallback {
	bool hit = false; b2Vec2 p, n;
	float ReportFixture(b2Fixture *f, const b2Vec2 &pt, const b2Vec2 &nrm, float frac) override {
		if (f->IsSensor()) return -1;
		hit = true; p = pt; n = nrm;
		return frac;
	}
};
bool World::raycast(Vec2 a, Vec2 b, Vec2 *hit, Vec2 *normal) const {
	RayCb cb;
	w->RayCast(&cb, toM(a), toM(b));
	if (cb.hit) { if (hit) *hit = toPx(cb.p); if (normal) *normal = Vec2(cb.n.x, cb.n.y); }
	return cb.hit;
}
void World::debugDraw(Gfx &g, const Camera2D &cam) const {
	for (b2Body *b = w->GetBodyList(); b; b = b->GetNext())
		for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext()) {
			Color c = b->GetType() == b2_staticBody ? Color(0x5AE07A, 0.8f) : Color(0xFFD25A, 0.9f);
			if (f->GetType() == b2Shape::e_polygon) {
				b2PolygonShape *ps = (b2PolygonShape *)f->GetShape();
				for (int i = 0; i < ps->m_count; i++) {
					Vec2 p0 = cam.toScreen(g, toPx(b->GetWorldPoint(ps->m_vertices[i])));
					Vec2 p1 = cam.toScreen(g, toPx(b->GetWorldPoint(ps->m_vertices[(i + 1) % ps->m_count])));
					g.line(p0.x, p0.y, p1.x, p1.y, 1, c);
				}
			} else if (f->GetType() == b2Shape::e_circle) {
				Vec2 s = cam.toScreen(g, toPx(b->GetPosition()));
				g.ring(s.x, s.y, f->GetShape()->m_radius * PPM * cam.zoom, 1, c, 20);
			}
		}
}
Vec2 pos(const b2Body *b) { return toPx(b->GetPosition()); }
float angle(const b2Body *b) { return b->GetAngle(); }
Vec2 vel(const b2Body *b) { b2Vec2 v = b->GetLinearVelocity(); return Vec2(v.x * PPM, v.y * PPM); }
void setVel(b2Body *b, Vec2 v) { b->SetLinearVelocity(b2Vec2(v.x / PPM, v.y / PPM)); }
void impulse(b2Body *b, Vec2 imp) { b->ApplyLinearImpulseToCenter(b2Vec2(imp.x / PPM, imp.y / PPM), true); }
int userId(const b2Body *b) { return int(const_cast<b2Body *>(b)->GetUserData().pointer) - 1; }

} // namespace phys
} // namespace forge
