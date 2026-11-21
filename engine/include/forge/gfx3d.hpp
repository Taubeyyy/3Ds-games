// FORGE – 3D-Grafik (Modul gfx3d: FORGE_MODULES += gfx3d)
// 3DS: eigener PICA200-Shader + Hardware-Fragmentlicht (max. 8 Lichter), Nebel, Stereo.
// PC: Software-Rasterizer mit Tiefenpuffer (Gouraud-Licht) für Tests/Screenshots.
#pragma once
#include "forge/forge.hpp"

namespace forge {

struct Vec3 {
	float x = 0, y = 0, z = 0;
	constexpr Vec3() = default;
	constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
	Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
	Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
	Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
	Vec3 operator-() const { return {-x, -y, -z}; }
	Vec3 &operator+=(Vec3 o) { x += o.x; y += o.y; z += o.z; return *this; }
	float dot(Vec3 o) const { return x * o.x + y * o.y + z * o.z; }
	Vec3 cross(Vec3 o) const { return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x}; }
	float len() const { return std::sqrt(dot(*this)); }
	Vec3 norm() const { float l = len(); return l > 1e-6f ? *this * (1 / l) : Vec3(); }
};

// 4x4-Matrix, Zeilen-Haupt-Ordnung, Spaltenvektoren (p' = M * p)
struct Mat4 {
	float m[4][4];
	static Mat4 identity();
	static Mat4 translate(Vec3 t);
	static Mat4 scale(Vec3 s);
	static Mat4 rotX(float a);
	static Mat4 rotY(float a);
	static Mat4 rotZ(float a);
	static Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up);
	Mat4 operator*(const Mat4 &o) const;
	Vec3 point(Vec3 p) const;   // mit Translation
	Vec3 dir(Vec3 d) const;     // ohne Translation
};

struct Vertex3D {
	float pos[3];
	float uv[2];
	float nrm[3];
	u8 col[4];
};

class Mesh {
public:
	Vertex3D *v = nullptr;
	u16 *idx = nullptr;
	int nv = 0, ni = 0;
	void alloc(int nVerts, int nIdx);   // 3DS: linearer Speicher
	void free();
	void upload();                      // nach Änderungen aufrufen
	void computeNormals();
	// Prozedurale Grundkörper (Farbe pro Vertex)
	static Mesh cube(Vec3 size, Color c);
	static Mesh sphere(float r, int seg, int rings, Color c);
	static Mesh plane(Vec2 size, int div, Color c, float uvScale = 1);
	static Mesh cylinder(float r, float h, int seg, Color c);
	// Gelände: (nx+1)*(nz+1) Höhen, zentriert um den Ursprung; colors optional (0xRRGGBB je Punkt)
	static Mesh grid(int nx, int nz, float cell, const float *heights, const u32 *colors, float uvScale = 1);
	bool loadMem(const void *data, size_t size); // eingebettete .fmesh (Ordner data/ → <name>_fmesh.h)
	bool loadFile(const char *path);    // .fmesh aus tools/meshtool.py (auch romfs:/…)
};

struct Texture3D {
	void *impl = nullptr;
	int w = 0, h = 0;
	bool loadMem(const void *data, size_t size, const char *debugName); // einzelnes Bild (.t3s ohne Atlas)
	bool loadFile(const char *path);
	void free();
};

struct Material {
	Color color{0xFFFFFF};
	Color emission{0x000000};
	float specular = 0.3f;     // 0..1
	float shininess = 20;      // Glanz-Exponent
	bool lit = true;
	const Texture3D *tex = nullptr;
};

struct Light3D {
	bool point = false;        // false = gerichtet
	Vec3 pos{0, -1, 0};        // Richtung (gerichtet: Licht kommt AUS dieser Richtung) oder Position
	Color color{0xFFFFFF};
	float intensity = 1;
	float range = 20;          // nur Punktlicht
};

struct Camera3D {
	Vec3 eye{0, 2, 6}, target{0, 0, 0}, up{0, 1, 0};
	float fov = 55;            // Grad (vertikal)
	float znear = 0.1f, zfar = 200;
	float focus = 6;           // Stereo-Fokusabstand (Objekte dort liegen auf der Bildschirmebene)
};

struct Scene3D {
	Color ambient{0x303040};
	Light3D lights[8];
	int nLights = 0;
	bool fog = false;
	Color fogColor{0x8090B0};
	float fogNear = 10, fogFar = 60;
	Color clear{0x000000, 0};  // Alpha 0 = Hintergrund nicht löschen (2D-Hintergrund bleibt)
};

// Zeichnen: zwischen begin/end beliebig viele Meshes. 2D darf davor/danach gezeichnet werden.
namespace gfx3d {
void begin(Gfx &g, const Camera3D &cam, const Scene3D &scene);
void draw(const Mesh &m, const Mat4 &model, const Material &mat);
void end(Gfx &g);
// Welt → Bildschirm (für 2D-Beschriftungen/Glow über 3D-Objekten). depth2d: passende 2D-Stereotiefe
// für Gfx-Aufrufe, damit 2D-Elemente im 3D-Bild an der richtigen Stelle schweben.
bool project(const Gfx &g, const Camera3D &cam, Vec3 p, Vec2 *out, float *depth2d = nullptr);
} // namespace gfx3d

} // namespace forge
