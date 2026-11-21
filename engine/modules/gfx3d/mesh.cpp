// FORGE gfx3d – plattformunabhängig: Matrizen, Grundkörper, Normalen, .fmesh-Laden
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "forge/gfx3d.hpp"

namespace forge {

Mat4 Mat4::identity() { Mat4 r{}; for (int i = 0; i < 4; i++) r.m[i][i] = 1; return r; }
Mat4 Mat4::translate(Vec3 t) { Mat4 r = identity(); r.m[0][3] = t.x; r.m[1][3] = t.y; r.m[2][3] = t.z; return r; }
Mat4 Mat4::scale(Vec3 s) { Mat4 r = identity(); r.m[0][0] = s.x; r.m[1][1] = s.y; r.m[2][2] = s.z; return r; }
Mat4 Mat4::rotX(float a) { Mat4 r = identity(); float c = cosf(a), s = sinf(a); r.m[1][1] = c; r.m[1][2] = -s; r.m[2][1] = s; r.m[2][2] = c; return r; }
Mat4 Mat4::rotY(float a) { Mat4 r = identity(); float c = cosf(a), s = sinf(a); r.m[0][0] = c; r.m[0][2] = s; r.m[2][0] = -s; r.m[2][2] = c; return r; }
Mat4 Mat4::rotZ(float a) { Mat4 r = identity(); float c = cosf(a), s = sinf(a); r.m[0][0] = c; r.m[0][1] = -s; r.m[1][0] = s; r.m[1][1] = c; return r; }
Mat4 Mat4::lookAt(Vec3 eye, Vec3 target, Vec3 up) {
	Vec3 f = (target - eye).norm(), s = f.cross(up).norm(), u = s.cross(f);
	Mat4 r = identity();
	r.m[0][0] = s.x; r.m[0][1] = s.y; r.m[0][2] = s.z; r.m[0][3] = -s.dot(eye);
	r.m[1][0] = u.x; r.m[1][1] = u.y; r.m[1][2] = u.z; r.m[1][3] = -u.dot(eye);
	r.m[2][0] = -f.x; r.m[2][1] = -f.y; r.m[2][2] = -f.z; r.m[2][3] = f.dot(eye);
	return r;
}
Mat4 Mat4::operator*(const Mat4 &o) const {
	Mat4 r{};
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++) r.m[i][j] += m[i][k] * o.m[k][j];
	return r;
}
Vec3 Mat4::point(Vec3 p) const {
	return {m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[0][3], m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[1][3],
		m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z + m[2][3]};
}
Vec3 Mat4::dir(Vec3 d) const {
	return {m[0][0] * d.x + m[0][1] * d.y + m[0][2] * d.z, m[1][0] * d.x + m[1][1] * d.y + m[1][2] * d.z, m[2][0] * d.x + m[2][1] * d.y + m[2][2] * d.z};
}

static void setV(Vertex3D &v, Vec3 p, Vec2 uv, Vec3 n, Color c) {
	v.pos[0] = p.x; v.pos[1] = p.y; v.pos[2] = p.z;
	v.uv[0] = uv.x; v.uv[1] = uv.y;
	v.nrm[0] = n.x; v.nrm[1] = n.y; v.nrm[2] = n.z;
	v.col[0] = (c.hex >> 16) & 255; v.col[1] = (c.hex >> 8) & 255; v.col[2] = c.hex & 255; v.col[3] = u8(clampf(c.a, 0, 1) * 255);
}

void Mesh::computeNormals() {
	for (int i = 0; i < nv; i++) v[i].nrm[0] = v[i].nrm[1] = v[i].nrm[2] = 0;
	for (int i = 0; i + 2 < ni; i += 3) {
		Vertex3D *a = &v[idx[i]], *b = &v[idx[i + 1]], *c = &v[idx[i + 2]];
		Vec3 pa(a->pos[0], a->pos[1], a->pos[2]), pb(b->pos[0], b->pos[1], b->pos[2]), pc(c->pos[0], c->pos[1], c->pos[2]);
		Vec3 n = (pb - pa).cross(pc - pa);
		for (Vertex3D *q : {a, b, c}) { q->nrm[0] += n.x; q->nrm[1] += n.y; q->nrm[2] += n.z; }
	}
	for (int i = 0; i < nv; i++) {
		Vec3 n = Vec3(v[i].nrm[0], v[i].nrm[1], v[i].nrm[2]).norm();
		v[i].nrm[0] = n.x; v[i].nrm[1] = n.y; v[i].nrm[2] = n.z;
	}
}

Mesh Mesh::cube(Vec3 s, Color c) {
	Mesh m;
	m.alloc(24, 36);
	static const float N[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
	Vec3 h = s * 0.5f;
	for (int f = 0; f < 6; f++) {
		Vec3 n(N[f][0], N[f][1], N[f][2]);
		Vec3 a = fabsf(n.y) > 0.5f ? Vec3(1, 0, 0) : Vec3(0, 1, 0);
		Vec3 u = a.cross(n), w = n.cross(u);
		for (int k = 0; k < 4; k++) {
			float su = (k == 1 || k == 2) ? 1.f : -1.f, sw = (k >= 2) ? 1.f : -1.f;
			Vec3 p = n + u * su + w * sw;
			setV(m.v[f * 4 + k], Vec3(p.x * h.x, p.y * h.y, p.z * h.z), Vec2((su + 1) / 2, (sw + 1) / 2), n, c);
		}
		u16 b = u16(f * 4);
		u16 q[6] = {b, u16(b + 1), u16(b + 2), b, u16(b + 2), u16(b + 3)};
		// Wicklung gegen den Uhrzeigersinn von aussen
		Vec3 p0(m.v[b].pos[0], m.v[b].pos[1], m.v[b].pos[2]), p1(m.v[b + 1].pos[0], m.v[b + 1].pos[1], m.v[b + 1].pos[2]),
			p2(m.v[b + 2].pos[0], m.v[b + 2].pos[1], m.v[b + 2].pos[2]);
		if ((p1 - p0).cross(p2 - p0).dot(n) < 0) { std::swap(q[1], q[2]); std::swap(q[4], q[5]); }
		memcpy(&m.idx[f * 6], q, sizeof q);
	}
	m.upload();
	return m;
}
Mesh Mesh::sphere(float r, int seg, int rings, Color c) {
	Mesh m;
	m.alloc((seg + 1) * (rings + 1), seg * rings * 6);
	int k = 0;
	for (int j = 0; j <= rings; j++)
		for (int i = 0; i <= seg; i++) {
			float th = PI * j / rings, ph = 2 * PI * i / seg;
			Vec3 n(sinf(th) * cosf(ph), cosf(th), sinf(th) * sinf(ph));
			setV(m.v[k++], n * r, Vec2(float(i) / seg, float(j) / rings), n, c);
		}
	k = 0;
	for (int j = 0; j < rings; j++)
		for (int i = 0; i < seg; i++) {
			u16 a = u16(j * (seg + 1) + i), b = u16(a + seg + 1);
			u16 q[6] = {a, u16(a + 1), b, u16(a + 1), u16(b + 1), b};
			memcpy(&m.idx[k], q, sizeof q);
			k += 6;
		}
	m.upload();
	return m;
}
Mesh Mesh::plane(Vec2 s, int div, Color c, float uvs) {
	Mesh m;
	m.alloc((div + 1) * (div + 1), div * div * 6);
	int k = 0;
	for (int j = 0; j <= div; j++)
		for (int i = 0; i <= div; i++) {
			float u = float(i) / div, w = float(j) / div;
			setV(m.v[k++], Vec3((u - 0.5f) * s.x, 0, (w - 0.5f) * s.y), Vec2(u * uvs, w * uvs), Vec3(0, 1, 0), c);
		}
	k = 0;
	for (int j = 0; j < div; j++)
		for (int i = 0; i < div; i++) {
			u16 a = u16(j * (div + 1) + i), b = u16(a + div + 1);
			u16 q[6] = {a, b, u16(a + 1), u16(a + 1), b, u16(b + 1)};
			memcpy(&m.idx[k], q, sizeof q);
			k += 6;
		}
	m.upload();
	return m;
}
Mesh Mesh::cylinder(float r, float h, int seg, Color c) {
	Mesh m;
	m.alloc((seg + 1) * 2 + 2 * (seg + 2), seg * 6 + seg * 6);
	int k = 0;
	for (int i = 0; i <= seg; i++) {
		float a = 2 * PI * i / seg;
		Vec3 n(cosf(a), 0, sinf(a));
		setV(m.v[k++], Vec3(n.x * r, 0, n.z * r), Vec2(float(i) / seg, 1), n, c);
		setV(m.v[k++], Vec3(n.x * r, h, n.z * r), Vec2(float(i) / seg, 0), n, c);
	}
	int capB = k;
	for (int cap = 0; cap < 2; cap++) {
		float y = cap ? h : 0;
		Vec3 n(0, cap ? 1.f : -1.f, 0);
		setV(m.v[k++], Vec3(0, y, 0), Vec2(0.5f, 0.5f), n, c);
		for (int i = 0; i <= seg; i++) {
			float a = 2 * PI * i / seg;
			setV(m.v[k++], Vec3(cosf(a) * r, y, sinf(a) * r), Vec2(0.5f + cosf(a) * 0.5f, 0.5f + sinf(a) * 0.5f), n, c);
		}
	}
	int q = 0;
	for (int i = 0; i < seg; i++) {
		u16 a = u16(i * 2);
		u16 t[6] = {a, u16(a + 1), u16(a + 2), u16(a + 1), u16(a + 3), u16(a + 2)};
		memcpy(&m.idx[q], t, sizeof t);
		q += 6;
	}
	for (int cap = 0; cap < 2; cap++) {
		u16 c0 = u16(capB + cap * (seg + 2));
		for (int i = 0; i < seg; i++) {
			if (cap) { m.idx[q++] = c0; m.idx[q++] = u16(c0 + 2 + i); m.idx[q++] = u16(c0 + 1 + i); }
			else { m.idx[q++] = c0; m.idx[q++] = u16(c0 + 1 + i); m.idx[q++] = u16(c0 + 2 + i); }
		}
	}
	m.ni = q;
	m.upload();
	return m;
}
bool Mesh::loadFile(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) return false;
	u32 hdr[3];
	bool ok = fread(hdr, 4, 3, f) == 3 && hdr[0] == 0x48534D46u; // 'FMSH'
	if (ok) {
		alloc(int(hdr[1]), int(hdr[2]));
		ok = fread(v, sizeof(Vertex3D), nv, f) == size_t(nv) && fread(idx, 2, ni, f) == size_t(ni);
		if (ok) upload();
	}
	fclose(f);
	return ok;
}

namespace gfx3d {
bool project(const Gfx &g, const Camera3D &cam, Vec3 p, Vec2 *out, float *depth2d) {
	Vec3 v = Mat4::lookAt(cam.eye, cam.target, cam.up).point(p);
	float d = -v.z;
	if (d < cam.znear) return false;
	float ta = tanf(cam.fov * PI / 360) * float(g.width) / g.height;
	out->x = (v.x / d / ta * 0.5f + 0.5f) * g.width;
	out->y = (-v.y / d / tanf(cam.fov * PI / 360) * 0.5f + 0.5f) * g.height;
	// Off-Axis-Stereo (wie Mtx_PerspStereoTilt mit iod = Regler/3): Versatz = iod/2 * (1/Fokus - 1/d) / ta * Breite/2
	if (depth2d) *depth2d = (1.0f / 6) * (1 / cam.focus - 1 / d) / ta * g.width * 0.5f;
	return true;
}
} // namespace gfx3d

} // namespace forge
