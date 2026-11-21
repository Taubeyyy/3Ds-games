// FORGE – Kamera, Tilemap, Partikel
#include "forge/world2d.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace forge {

void Camera2D::follow(Vec2 t, float dt, float k) {
	pos.x = approach(pos.x, t.x, k, dt);
	pos.y = approach(pos.y, t.y, k, dt);
}
void Camera2D::update(float dt, Rng &rng) {
	pos.x = clampf(pos.x, boundsMin.x, boundsMax.x);
	pos.y = clampf(pos.y, boundsMin.y, boundsMax.y);
	if (shake > 0) {
		float s = shake * shake * 7;
		shakeOff = Vec2(rng.range(-s, s), rng.range(-s, s));
		shake = fmaxf(0, shake - dt * 1.8f);
	} else shakeOff = Vec2();
}
bool Camera2D::visible(const Gfx &g, Vec2 w, float r) const {
	Vec2 s = toScreen(g, w);
	float rr = r * zoom;
	return s.x > -rr && s.y > -rr && s.x < g.width + rr && s.y < g.height + rr;
}

void Tilemap::create(int w_, int h_, float ts) {
	destroy();
	w = w_; h = h_; tile = ts;
	cells = (u8 *)calloc(size_t(w) * h, 1);
}
void Tilemap::destroy() { free(cells); cells = nullptr; w = h = 0; }
void Tilemap::draw(Gfx &g, const Camera2D &cam, float depth) const {
	if (!images) return;
	Vec2 a = cam.toWorld(g, Vec2(0, 0)), b = cam.toWorld(g, Vec2(float(g.width), float(g.height)));
	int x0 = int(floorf(a.x / tile)) - 1, y0 = int(floorf(a.y / tile)) - 1, x1 = int(b.x / tile) + 1, y1 = int(b.y / tile) + 1;
	for (int y = y0; y <= y1; y++)
		for (int x = x0; x <= x1; x++) {
			u8 id = get(x, y);
			if (!id || !images[id].valid()) continue;
			Vec2 s = cam.toScreen(g, Vec2((x + 0.5f) * tile, (y + 0.5f) * tile));
			float sc = tile * cam.zoom / images[id].w;
			g.image(images[id], s.x, s.y, sc * 1.01f, sc * 1.01f, 0, Color(0xFFFFFF), 0, depth);
		}
}

void Particles::emit(Vec2 at, const EmitDesc &d) {
	for (int i = 0; i < d.count; i++) {
		Particle &q = p[next];
		next = (next + 1) % MAX;
		float a = d.angle + rng.range(-d.spread / 2, d.spread / 2);
		float sp = rng.range(d.speedMin, d.speedMax);
		q.p = at;
		q.v = Vec2::fromAngle(a) * sp;
		q.life = q.maxLife = rng.range(d.lifeMin, d.lifeMax);
		q.size0 = d.size0; q.size1 = d.size1;
		q.c0 = d.c0; q.c1 = d.c1;
		q.img = d.img;
		q.rot = rng.range(0, 2 * PI);
		q.spin = rng.range(-d.spin, d.spin);
		q.drag = d.drag; q.gravity = d.gravity;
	}
}
void Particles::update(float dt) {
	for (auto &q : p) {
		if (q.life <= 0) continue;
		q.life -= dt;
		q.v *= expf(-q.drag * dt);
		q.v.y += q.gravity * dt;
		q.p += q.v * dt;
		q.rot += q.spin * dt;
	}
}
void Particles::draw(Gfx &g, const Camera2D *cam, float depth) const {
	for (auto &q : p) {
		if (q.life <= 0 || !q.img) continue;
		float t = 1 - q.life / q.maxLife;
		Vec2 s = cam ? cam->toScreen(g, q.p) : q.p;
		float sc = lerpf(q.size0, q.size1, t) * (cam ? cam->zoom : 1);
		Color c = q.c0.mix(q.c1, t);
		g.image(*q.img, s.x, s.y, sc, sc, q.rot, c, 1, depth);
	}
}

} // namespace forge
