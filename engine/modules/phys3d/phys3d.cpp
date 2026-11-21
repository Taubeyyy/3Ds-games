// FORGE phys3d – Bullet-2.87-Anbindung + kinematischer Charakter-Controller
#include <algorithm>
#include <vector>
#include <btBulletDynamicsCommon.h>
#include "forge/phys3d.hpp"

namespace forge {
namespace phys3d {

static inline btVector3 bv(Vec3 v) { return btVector3(v.x, v.y, v.z); }
static inline Vec3 fv(const btVector3 &v) { return Vec3(v.x(), v.y(), v.z()); }

struct PairRec { const void *a, *b; int ia, ib; };
struct BodyData { btTriangleMesh *tm = nullptr; };

struct WorldImpl {
	btDefaultCollisionConfiguration cfg;
	btCollisionDispatcher disp;
	btDbvtBroadphase bp;
	btSequentialImpulseConstraintSolver solver;
	btDiscreteDynamicsWorld world;
	std::vector<Contact> events;
	std::vector<PairRec> prev, cur;
	static btDefaultCollisionConstructionInfo info() {
		btDefaultCollisionConstructionInfo i;
		i.m_defaultMaxPersistentManifoldPoolSize = 1024; // weniger Speicher auf dem 3DS
		i.m_defaultMaxCollisionAlgorithmPoolSize = 1024;
		return i;
	}
	WorldImpl() : cfg(info()), disp(&cfg), world(&disp, &bp, &solver, &cfg) {}
};

void World::create(Vec3 gravity) {
	destroy();
	impl = new WorldImpl;
	bt = &impl->world;
	bt->setGravity(bv(gravity));
}

void World::destroy() {
	if (!impl) return;
	for (int i = bt->getNumCollisionObjects() - 1; i >= 0; i--) remove(btRigidBody::upcast(bt->getCollisionObjectArray()[i]));
	delete impl;
	impl = nullptr;
	bt = nullptr;
}

static bool pairLess(const PairRec &x, const PairRec &y) { return x.a != y.a ? x.a < y.a : x.b < y.b; }

void World::step(float dt) {
	if (!impl) return;
	bt->stepSimulation(dt, 4, 1.0f / 60);
	impl->events.clear();
	impl->cur.clear();
	int n = impl->disp.getNumManifolds();
	for (int i = 0; i < n; i++) {
		btPersistentManifold *m = impl->disp.getManifoldByIndexInternal(i);
		if (m->getNumContacts() == 0) continue;
		const btCollisionObject *a = m->getBody0(), *b = m->getBody1();
		if (a > b) std::swap(a, b);
		PairRec r{a, b, a->getUserIndex(), b->getUserIndex()};
		impl->cur.push_back(r);
		if (!std::binary_search(impl->prev.begin(), impl->prev.end(), r, pairLess)) {
			const btManifoldPoint &pt = m->getContactPoint(0);
			impl->events.push_back({r.ia, r.ib, true, fv(pt.getPositionWorldOnA()), pt.getAppliedImpulse()});
		}
	}
	std::sort(impl->cur.begin(), impl->cur.end(), pairLess);
	for (const PairRec &r : impl->prev)
		if (!std::binary_search(impl->cur.begin(), impl->cur.end(), r, pairLess)) impl->events.push_back({r.ia, r.ib, false, Vec3(), 0});
	std::swap(impl->prev, impl->cur);
}

static Body *addBody(World &w, btCollisionShape *shape, Vec3 c, BodyType t, int id, float mass, float fr, float rest) {
	btScalar m = t == DYNAMIC ? mass : 0;
	btVector3 inertia(0, 0, 0);
	if (m > 0) shape->calculateLocalInertia(m, inertia);
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(bv(c));
	btRigidBody::btRigidBodyConstructionInfo ci(m, new btDefaultMotionState(tr), shape, inertia);
	ci.m_friction = fr;
	ci.m_restitution = rest;
	ci.m_rollingFriction = t == DYNAMIC ? 0.02f : 0;
	btRigidBody *b = new btRigidBody(ci);
	b->setUserIndex(id);
	b->setUserPointer(new BodyData);
	if (t == KINEMATIC) {
		b->setCollisionFlags(b->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		b->setActivationState(DISABLE_DEACTIVATION);
	}
	w.bt->addRigidBody(b);
	return b;
}

Body *World::box(Vec3 c, Vec3 h, BodyType t, int id, float mass, float fr, float rest) { return addBody(*this, new btBoxShape(bv(h)), c, t, id, mass, fr, rest); }
Body *World::sphere(Vec3 c, float r, BodyType t, int id, float mass, float fr, float rest) { return addBody(*this, new btSphereShape(r), c, t, id, mass, fr, rest); }
Body *World::capsule(Vec3 c, float r, float h, BodyType t, int id, float mass, float fr) {
	return addBody(*this, new btCapsuleShape(r, std::max(0.01f, h - 2 * r)), c, t, id, mass, fr, 0);
}
Body *World::cylinder(Vec3 c, float r, float h, BodyType t, int id, float mass, float fr) {
	return addBody(*this, new btCylinderShape(btVector3(r, h * 0.5f, r)), c, t, id, mass, fr, 0.1f);
}

Body *World::mesh(const Mesh &m, const Mat4 &xf, int id, float fr) {
	btTriangleMesh *tm = new btTriangleMesh(true, false);
	for (int i = 0; i + 2 < m.ni; i += 3) {
		btVector3 p[3];
		for (int k = 0; k < 3; k++) {
			const Vertex3D &v = m.v[m.idx[i + k]];
			p[k] = bv(xf.point(Vec3(v.pos[0], v.pos[1], v.pos[2])));
		}
		tm->addTriangle(p[0], p[1], p[2], false);
	}
	Body *b = addBody(*this, new btBvhTriangleMeshShape(tm, true), Vec3(), STATIC, id, 0, fr, 0);
	((BodyData *)b->getUserPointer())->tm = tm;
	return b;
}

void World::remove(Body *b) {
	if (!b || !impl) return;
	bt->removeRigidBody(b);
	BodyData *d = (BodyData *)b->getUserPointer();
	delete b->getMotionState();
	delete b->getCollisionShape();
	if (d) { delete d->tm; delete d; }
	delete b;
}

// ------------------------------------------------------------------ Abfragen
struct RayCB : btCollisionWorld::ClosestRayResultCallback {
	int ignore;
	RayCB(const btVector3 &a, const btVector3 &b, int ig) : ClosestRayResultCallback(a, b), ignore(ig) {}
	bool needsCollision(btBroadphaseProxy *p) const override {
		if (((btCollisionObject *)p->m_clientObject)->getUserIndex() == ignore) return false;
		return ClosestRayResultCallback::needsCollision(p);
	}
};
bool World::raycast(Vec3 from, Vec3 to, Hit *out, int ignore) const {
	if (!impl) return false;
	RayCB cb(bv(from), bv(to), ignore);
	bt->rayTest(bv(from), bv(to), cb);
	if (!cb.hasHit()) return false;
	if (out) {
		out->pos = fv(cb.m_hitPointWorld);
		out->normal = fv(cb.m_hitNormalWorld).norm();
		out->frac = cb.m_closestHitFraction;
		out->body = (Body *)btRigidBody::upcast(cb.m_collisionObject);
		out->userId = cb.m_collisionObject->getUserIndex();
	}
	return true;
}

struct SweepCB : btCollisionWorld::ClosestConvexResultCallback {
	int ignore;
	SweepCB(const btVector3 &a, const btVector3 &b, int ig) : ClosestConvexResultCallback(a, b), ignore(ig) {}
	bool needsCollision(btBroadphaseProxy *p) const override {
		if (((btCollisionObject *)p->m_clientObject)->getUserIndex() == ignore) return false;
		return ClosestConvexResultCallback::needsCollision(p);
	}
};
static bool sweepShape(const btDiscreteDynamicsWorld *bt, const btConvexShape *s, Vec3 from, Vec3 to, Hit *out, int ignore) {
	if ((to - from).len() < 1e-5f) return false;
	btTransform a, b;
	a.setIdentity(); b.setIdentity();
	a.setOrigin(bv(from)); b.setOrigin(bv(to));
	SweepCB cb(a.getOrigin(), b.getOrigin(), ignore);
	bt->convexSweepTest(s, a, b, cb, 0.001f);
	if (!cb.hasHit()) return false;
	out->pos = fv(cb.m_hitPointWorld);
	out->normal = fv(cb.m_hitNormalWorld).norm();
	out->frac = cb.m_closestHitFraction;
	out->body = (Body *)btRigidBody::upcast(cb.m_hitCollisionObject);
	out->userId = cb.m_hitCollisionObject->getUserIndex();
	return true;
}
bool World::sphereCast(Vec3 from, Vec3 to, float r, Hit *out, int ignore) const {
	if (!impl) return false;
	btSphereShape s(r);
	Hit h;
	if (!sweepShape(bt, &s, from, to, &h, ignore)) return false;
	if (out) *out = h;
	return true;
}

struct OverlapCB : btCollisionWorld::ContactResultCallback {
	const btCollisionObject *self;
	Body **out;
	int max, n = 0;
	btScalar addSingleResult(btManifoldPoint &cp, const btCollisionObjectWrapper *a, int, int, const btCollisionObjectWrapper *b, int, int) override {
		if (cp.getDistance() > 0.01f) return 0;
		const btCollisionObject *o = a->getCollisionObject() == self ? b->getCollisionObject() : a->getCollisionObject();
		Body *rb = (Body *)btRigidBody::upcast(o);
		if (!rb) return 0;
		for (int i = 0; i < n; i++) if (out[i] == rb) return 0;
		if (n < max) out[n++] = rb;
		return 0;
	}
};
int World::overlapSphere(Vec3 c, float r, Body **out, int max) const {
	if (!impl) return 0;
	btSphereShape s(r);
	btCollisionObject o;
	o.setCollisionShape(&s);
	btTransform t;
	t.setIdentity();
	t.setOrigin(bv(c));
	o.setWorldTransform(t);
	OverlapCB cb;
	cb.self = &o; cb.out = out; cb.max = max;
	bt->contactTest(&o, cb);
	return cb.n;
}

void World::explode(Vec3 c, float r, float strength) {
	if (!impl) return;
	for (int i = 0; i < bt->getNumCollisionObjects(); i++) {
		btRigidBody *b = btRigidBody::upcast(bt->getCollisionObjectArray()[i]);
		if (!b || b->getInvMass() == 0) continue;
		Vec3 d = fv(b->getCenterOfMassPosition()) - c;
		float l = d.len();
		if (l > r) continue;
		Vec3 dir = l > 1e-3f ? d * (1 / l) : Vec3(0, 1, 0);
		dir = (dir + Vec3(0, 0.4f, 0)).norm();
		b->activate();
		b->applyCentralImpulse(bv(dir * (strength * (1 - l / r) / b->getInvMass())));
	}
}

int World::contacts(const Contact **out) const {
	if (!impl) { *out = nullptr; return 0; }
	*out = impl->events.data();
	return int(impl->events.size());
}
int World::bodyCount() const { return impl ? bt->getNumCollisionObjects() : 0; }

void World::debugDraw(Gfx &g, const Camera3D &cam) const {
	if (!impl) return;
	static const int E[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
	for (int i = 0; i < bt->getNumCollisionObjects(); i++) {
		const btCollisionObject *o = bt->getCollisionObjectArray()[i];
		const btRigidBody *b = btRigidBody::upcast(o);
		if (o->getCollisionShape()->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE) continue; // Gelände nicht umranden
		Vec3 corner[8];
		const btTransform &t = o->getWorldTransform();
		if (o->getCollisionShape()->getShapeType() == BOX_SHAPE_PROXYTYPE) {
			btVector3 h = ((const btBoxShape *)o->getCollisionShape())->getHalfExtentsWithMargin();
			for (int k = 0; k < 8; k++) corner[k] = fv(t * btVector3(k & 1 ? h.x() : -h.x(), k & 2 ? h.y() : -h.y(), k & 4 ? h.z() : -h.z()));
		} else {
			btVector3 mn, mx;
			o->getCollisionShape()->getAabb(t, mn, mx);
			for (int k = 0; k < 8; k++) corner[k] = Vec3(k & 1 ? mx.x() : mn.x(), k & 2 ? mx.y() : mn.y(), k & 4 ? mx.z() : mn.z());
		}
		Color c = !b || b->isStaticObject() ? Color(0x9AA0B8, 0.6f) : b->isKinematicObject() ? Color(0x4AB8FF) : b->isActive() ? Color(0x5AE07A) : Color(0x3A8050);
		Vec2 s[8];
		bool ok[8];
		for (int k = 0; k < 8; k++) ok[k] = gfx3d::project(g, cam, corner[k], &s[k]);
		for (auto &e : E)
			if (ok[e[0]] && ok[e[1]]) g.line(s[e[0]].x, s[e[0]].y, s[e[1]].x, s[e[1]].y, 1, c, 0);
	}
}

// ------------------------------------------------------------------ Körper-Hilfen
Vec3 pos(const Body *b) { return fv(b->getWorldTransform().getOrigin()); }
Mat4 transform(const Body *b) {
	btTransform t;
	if (b->getMotionState()) b->getMotionState()->getWorldTransform(t);
	else t = b->getWorldTransform();
	const btMatrix3x3 &r = t.getBasis();
	Mat4 m = Mat4::identity();
	for (int i = 0; i < 3; i++) {
		m.m[i][0] = r[i].x(); m.m[i][1] = r[i].y(); m.m[i][2] = r[i].z();
	}
	m.m[0][3] = t.getOrigin().x(); m.m[1][3] = t.getOrigin().y(); m.m[2][3] = t.getOrigin().z();
	return m;
}
Vec3 vel(const Body *b) { return fv(b->getLinearVelocity()); }
void setVel(Body *b, Vec3 v) { b->activate(); b->setLinearVelocity(bv(v)); }
void setAngVel(Body *b, Vec3 w) { b->activate(); b->setAngularVelocity(bv(w)); }
void impulse(Body *b, Vec3 imp, Vec3 at) { b->activate(); b->applyImpulse(bv(imp), bv(at)); }
void setPos(Body *b, Vec3 p, float yaw) {
	btTransform t;
	t.setIdentity();
	t.setOrigin(bv(p));
	t.setRotation(btQuaternion(btVector3(0, 1, 0), yaw));
	b->setWorldTransform(t);
	if (b->getMotionState()) b->getMotionState()->setWorldTransform(t);
	b->activate();
}
int userId(const Body *b) { return b->getUserIndex(); }
bool isDynamic(const Body *b) { return b->getInvMass() > 0; }

// ------------------------------------------------------------------ Charakter
struct DepenCB : btCollisionWorld::ContactResultCallback {
	const btCollisionObject *self;
	btVector3 push{0, 0, 0};
	btScalar addSingleResult(btManifoldPoint &cp, const btCollisionObjectWrapper *a, int, int, const btCollisionObjectWrapper *, int, int) override {
		float d = cp.getDistance();
		if (d >= -0.002f) return 0;
		btVector3 n = cp.m_normalWorldOnB * (a->getCollisionObject() == self ? 1.f : -1.f);
		btVector3 p = n * -d;
		// größte Verschiebung je Achse sammeln (verhindert Doppelzählung bei mehreren Punkten)
		for (int k = 0; k < 3; k++)
			if (fabsf(p[k]) > fabsf(push[k])) push[k] = p[k];
		return 0;
	}
};

void Character::update(World &w, Vec3 wish, bool jumpPressed, float dt) {
	if (!w.impl) return;
	const float skin = 0.01f;
	float cylH = std::max(0.01f, height - 2 * radius);
	btCapsuleShape shape(radius - skin, cylH);
	auto center = [&](Vec3 foot) { return foot + Vec3(0, height * 0.5f, 0); };
	Vec3 p = pos;

	// 1) Aus Geometrie herausdrücken (bewegte Plattformen, Spawn in Wand)
	for (int it = 0; it < 3; it++) {
		btCollisionObject o;
		btCapsuleShape s2(radius, cylH);
		o.setCollisionShape(&s2);
		btTransform t;
		t.setIdentity();
		t.setOrigin(bv(center(p)));
		o.setWorldTransform(t);
		DepenCB cb;
		cb.self = &o;
		w.bt->contactTest(&o, cb);
		if (cb.push.length2() < 1e-8f) break;
		p += fv(cb.push);
	}

	// 2) Waagerechte Geschwindigkeit an Wunsch annähern
	Vec3 hv(vel.x, 0, vel.z);
	Vec3 dv = Vec3(wish.x, 0, wish.z) - hv;
	float maxd = (grounded ? accel : airAccel) * dt;
	if (dv.len() > maxd) dv = dv.norm() * maxd;
	hv += dv;

	// 3) Sprung mit Coyote-Zeit und Puffer
	coyote = grounded ? 0.1f : coyote - dt;
	jumpBuffer = jumpPressed ? 0.12f : jumpBuffer - dt;
	bool jumped = false;
	if (jumpBuffer > 0 && coyote > 0) {
		vel.y = jumpSpeed;
		jumped = true;
		grounded = false;
		coyote = jumpBuffer = 0;
	}
	if (!grounded) vel.y = std::max(vel.y - gravity * dt, -maxFall);
	else if (!jumped) vel.y = 0;

	// 4) Stufe: erst anheben
	float upDone = 0;
	Hit h;
	if (grounded && !jumped) {
		float up = stepHeight;
		if (sweepShape(w.bt, &shape, center(p), center(p + Vec3(0, up, 0)), &h, -999)) up = std::max(0.f, up * h.frac - skin);
		p.y += up;
		upDone = up;
	}

	// 5) Waagerecht bewegen und an Wänden gleiten
	Vec3 d = hv * dt;
	for (int it = 0; it < 4 && d.len() > 1e-5f; it++) {
		if (!sweepShape(w.bt, &shape, center(p), center(p + d), &h, -999)) { p += d; break; }
		float len = d.len();
		float move = std::max(0.f, h.frac * len - skin);
		p += d * (move / len);
		if (h.body && isDynamic(h.body)) { // Kisten schieben
			Vec3 dir = Vec3(d.x, 0, d.z).norm();
			impulse(h.body, dir * (pushForce * dt / std::max(0.05f, h.body->getInvMass())) * 0.2f);
		}
		Vec3 n(h.normal.x, 0, h.normal.z);
		if (n.len() < 1e-4f) break;
		n = n.norm();
		Vec3 rest = d * (1 - move / len);
		d = rest - n * rest.dot(n);
		float into = hv.dot(n);
		if (into < 0) hv = hv - n * into;
	}

	// 6) Senkrecht: Fallen/Steigen, Stufe zurück, am Boden haften
	float cosMax = cosf(maxSlopeDeg * PI / 180);
	float dyFree = vel.y * dt - upDone;
	float snap = (grounded && !jumped) ? stepHeight : 0;
	float dy = dyFree - snap;
	bool wasGrounded = grounded;
	grounded = false;
	if (fabsf(dy) > 1e-6f && sweepShape(w.bt, &shape, center(p), center(p + Vec3(0, dy, 0)), &h, -999)) {
		float move = std::max(0.f, h.frac * fabsf(dy) - skin);
		if (dy < 0) {
			if (h.normal.y >= cosMax) {
				p.y -= move;
				grounded = true;
				groundNormal = h.normal;
				groundUserId = h.userId;
				vel.y = 0;
			} else {
				// zu steil: abrutschen
				p.y -= move;
				Vec3 n(h.normal.x, 0, h.normal.z);
				hv += n.norm() * (gravity * dt);
			}
		} else {
			p.y += move; // Decke
			vel.y = std::min(vel.y, 0.f);
		}
	} else {
		p.y += dyFree; // kein Boden in Reichweite: frei fallen (ohne Haft-Weg)
		if (wasGrounded && !jumped) vel.y = 0;
	}
	pos = p;
	vel = Vec3(hv.x, vel.y, hv.z);
}

} // namespace phys3d
} // namespace forge
