// FORGE gfx3d – PC-Backend: Software-Rasterizer mit Tiefenpuffer, Gouraud-Licht, Nebel
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "forge/gfx3d.hpp"

namespace forge {
float (*pcFramebuffer(int *w, int *h))[3]; // aus platform/pc/backend.cpp

void Mesh::alloc(int nVerts, int nIdx) {
	free();
	nv = nVerts; ni = nIdx;
	v = (Vertex3D *)calloc(nv, sizeof(Vertex3D));
	idx = (u16 *)calloc(ni, sizeof(u16));
}
void Mesh::free() { ::free(v); ::free(idx); v = nullptr; idx = nullptr; nv = ni = 0; }
void Mesh::upload() {}

struct PcTex { int w, h; std::vector<unsigned char> px; };
static bool loadTexBin(Texture3D *t, const std::string &path) {
	FILE *f = fopen(path.c_str(), "rb");
	if (!f) { fprintf(stderr, "[forge] Textur fehlt: %s\n", path.c_str()); return false; }
	u32 n = 0;
	u16 wh[2];
	bool ok = fread(&n, 4, 1, f) == 1 && n > 0 && fread(wh, 2, 2, f) == 2;
	if (ok) {
		PcTex *pt = new PcTex;
		pt->w = wh[0]; pt->h = wh[1];
		pt->px.resize(size_t(pt->w) * pt->h * 4);
		if (fread(pt->px.data(), 4, size_t(pt->w) * pt->h, f)) {}
		t->impl = pt; t->w = pt->w; t->h = pt->h;
	}
	fclose(f);
	return ok;
}
bool Texture3D::loadMem(const void *, size_t, const char *name) {
	const char *d = getenv("FORGE_GFX");
	return loadTexBin(this, std::string(d ? d : ".") + "/" + name + ".bin");
}
bool Texture3D::loadFile(const char *path) {
	std::string p(path);
	const char *r = getenv("FORGE_ROMFS");
	if (p.rfind("romfs:/", 0) == 0) p = std::string(r ? r : "romfs") + "/" + p.substr(7);
	size_t dot = p.rfind('.');
	if (dot != std::string::npos) p = p.substr(0, dot) + ".bin";
	return loadTexBin(this, p);
}
void Texture3D::free() { delete (PcTex *)impl; impl = nullptr; }

namespace gfx3d {

static float (*g_fb)[3];
static int g_w, g_h;
static std::vector<float> g_depth;
static Mat4 g_view;
static float g_f, g_aspect, g_near;
static const Scene3D *g_scene;
static Camera3D g_cam;

void begin(Gfx &g, const Camera3D &cam, const Scene3D &scene) {
	if (scene.clear.a > 0) g.rect(0, 0, g.width, g.height, scene.clear);
	g_fb = pcFramebuffer(&g_w, &g_h);
	g_depth.assign(size_t(g_w) * g_h, 1e30f);
	g_view = Mat4::lookAt(cam.eye, cam.target, cam.up);
	g_f = 1 / tanf(cam.fov * PI / 360);
	g_aspect = float(g.width) / g.height;
	g_near = cam.znear;
	g_scene = &scene;
	g_cam = cam;
}

struct SV { float x, y, z, iw, u, v, r, g, b, a, fog; };
static void rasterTri(const SV &a, const SV &b, const SV &c, const PcTex *tex, const float fogc[3], bool add) {
	float minx = fmaxf(0, floorf(fminf(a.x, fminf(b.x, c.x)))), maxx = fminf(g_w - 1, ceilf(fmaxf(a.x, fmaxf(b.x, c.x))));
	float miny = fmaxf(0, floorf(fminf(a.y, fminf(b.y, c.y)))), maxy = fminf(g_h - 1, ceilf(fmaxf(a.y, fmaxf(b.y, c.y))));
	float area = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
	if (fabsf(area) < 1e-6f || area > 0) return; // Rueckseiten verwerfen (Bildschirm-y nach unten)
	for (int j = int(miny); j <= int(maxy); j++)
		for (int i = int(minx); i <= int(maxx); i++) {
			float px = i + 0.5f, py = j + 0.5f;
			float w0 = ((b.x - px) * (c.y - py) - (c.x - px) * (b.y - py)) / area;
			float w1 = ((c.x - px) * (a.y - py) - (a.x - px) * (c.y - py)) / area;
			float w2 = 1 - w0 - w1;
			if (w0 < 0 || w1 < 0 || w2 < 0) continue;
			float iw = a.iw * w0 + b.iw * w1 + c.iw * w2;
			float z = 1 / iw;
			float &d = g_depth[size_t(j) * g_w + i];
			if (z >= d) continue;
			auto P = [&](float qa, float qb, float qc) { return (qa * a.iw * w0 + qb * b.iw * w1 + qc * c.iw * w2) * z; };
			float r = P(a.r, b.r, c.r), gg = P(a.g, b.g, c.g), bb = P(a.b, b.b, c.b), al = P(a.a, b.a, c.a);
			if (tex) {
				float u = P(a.u, b.u, c.u), v = P(a.v, b.v, c.v);
				u -= floorf(u); v -= floorf(v);
				int tx = int(u * tex->w) % tex->w, ty = int((1 - v) * tex->h) % tex->h;
				if (ty < 0) ty += tex->h;
				const unsigned char *tp = &tex->px[(size_t(ty) * tex->w + tx) * 4];
				r *= tp[0] / 255.f; gg *= tp[1] / 255.f; bb *= tp[2] / 255.f; al *= tp[3] / 255.f;
			}
			float fog = P(a.fog, b.fog, c.fog);
			r = r * (1 - fog) + fogc[0] * fog; gg = gg * (1 - fog) + fogc[1] * fog; bb = bb * (1 - fog) + fogc[2] * fog;
			if (al < 0.01f) continue;
			float *p = g_fb[size_t(j) * g_w + i];
			if (add) { p[0] = fminf(1, p[0] + r * al); p[1] = fminf(1, p[1] + gg * al); p[2] = fminf(1, p[2] + bb * al); continue; }
			if (al > 0.99f) d = z;
			p[0] += (fminf(r, 1) - p[0]) * al; p[1] += (fminf(gg, 1) - p[1]) * al; p[2] += (fminf(bb, 1) - p[2]) * al;
		}
}

void draw(const Mesh &m, const Mat4 &model, const Material &mat) {
	if (!m.v || !m.ni) return;
	Mat4 mv = g_view * model;
	const Scene3D &sc = *g_scene;
	auto col = [](Color c, float out[3]) { out[0] = ((c.hex >> 16) & 255) / 255.f; out[1] = ((c.hex >> 8) & 255) / 255.f; out[2] = (c.hex & 255) / 255.f; };
	float amb[3], mc[3], em[3], fogc[3];
	col(sc.ambient, amb); col(mat.color, mc); col(mat.emission, em); col(sc.fogColor, fogc);
	std::vector<SV> sv(m.nv);
	std::vector<Vec3> vp(m.nv);
	for (int i = 0; i < m.nv; i++) {
		const Vertex3D &q = m.v[i];
		Vec3 p = mv.point(Vec3(q.pos[0], q.pos[1], q.pos[2]));
		Vec3 n = mv.dir(Vec3(q.nrm[0], q.nrm[1], q.nrm[2])).norm();
		vp[i] = p;
		float lr = 1, lg = 1, lb = 1, sr = 0, sg = 0, sb = 0;
		if (mat.lit) {
			lr = amb[0] + em[0]; lg = amb[1] + em[1]; lb = amb[2] + em[2];
			for (int k = 0; k < sc.nLights; k++) {
				const Light3D &L = sc.lights[k];
				float lc[3];
				col(L.color, lc);
				float att = L.intensity;
				Vec3 ld;
				if (L.point) {
					Vec3 dv = g_view.point(L.pos) - p;
					float t = 1 - dv.dot(dv) / (L.range * L.range);
					att *= t > 0 ? t * t : 0;
					ld = dv.norm();
				} else ld = g_view.dir(L.pos).norm();
				float nd = fmaxf(0, n.dot(ld)) * att;
				lr += lc[0] * nd; lg += lc[1] * nd; lb += lc[2] * nd;
				Vec3 h = (ld + (-p).norm()).norm();
				float sp = powf(fmaxf(0, n.dot(h)), mat.shininess) * mat.specular * att;
				sr += lc[0] * sp; sg += lc[1] * sp; sb += lc[2] * sp;
			}
		}
		float vc[4] = {q.col[0] / 255.f, q.col[1] / 255.f, q.col[2] / 255.f, q.col[3] / 255.f};
		SV &s = sv[i];
		s.r = vc[0] * mc[0] * lr + sr; s.g = vc[1] * mc[1] * lg + sg; s.b = vc[2] * mc[2] * lb + sb; s.a = vc[3] * mat.color.a;
		s.u = q.uv[0]; s.v = q.uv[1];
		float dist = -p.z;
		s.fog = sc.fog ? clampf((dist - sc.fogNear) / fmaxf(0.01f, sc.fogFar - sc.fogNear), 0, 1) : 0;
		if (-p.z < g_near) { s.iw = -1; continue; }
		s.iw = 1 / -p.z;
		s.x = (p.x * g_f / g_aspect * s.iw * 0.5f + 0.5f) * g_w;
		s.y = (-p.y * g_f * s.iw * 0.5f + 0.5f) * g_h;
		s.z = -p.z;
	}
	const PcTex *tex = mat.tex ? (const PcTex *)mat.tex->impl : nullptr;
	for (int i = 0; i + 2 < m.ni; i += 3) {
		const SV &a = sv[m.idx[i]], &b = sv[m.idx[i + 1]], &c = sv[m.idx[i + 2]];
		if (a.iw < 0 || b.iw < 0 || c.iw < 0) continue; // einfaches Near-Clipping
		rasterTri(a, b, c, tex, fogc, mat.additive);
	}
}

void end(Gfx &g) { (void)g; }

} // namespace gfx3d
} // namespace forge
