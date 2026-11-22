// FORGE gfx3d – 3DS-Backend (citro3d, PICA200-Fragmentlicht)
#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <tex3ds.h>
#include <cstring>
#include "forge/gfx3d.hpp"
#include "lit_shbin.h"

namespace forge {

void Mesh::alloc(int nVerts, int nIdx) {
	free();
	nv = nVerts; ni = nIdx;
	v = (Vertex3D *)linearAlloc(sizeof(Vertex3D) * nv);
	idx = (u16 *)linearAlloc(sizeof(u16) * ni);
	memset(v, 0, sizeof(Vertex3D) * nv);
}
void Mesh::free() {
	if (v) linearFree(v);
	if (idx) linearFree(idx);
	v = nullptr; idx = nullptr; nv = ni = 0;
}
void Mesh::upload() {
	if (v) GSPGPU_FlushDataCache(v, sizeof(Vertex3D) * nv);
	if (idx) GSPGPU_FlushDataCache(idx, sizeof(u16) * ni);
}

bool Texture3D::loadMem(const void *data, size_t size, const char *) {
	C3D_Tex *t = new C3D_Tex;
	Tex3DS_Texture tx = Tex3DS_TextureImport(data, size, t, nullptr, false);
	if (!tx) { delete t; return false; }
	Tex3DS_TextureFree(tx);
	C3D_TexSetFilter(t, GPU_LINEAR, GPU_LINEAR);
	if (t->maxLevel > 0) C3D_TexSetFilterMipmap(t, GPU_LINEAR);
	C3D_TexSetWrap(t, GPU_REPEAT, GPU_REPEAT);
	impl = t; w = t->width; h = t->height;
	return true;
}
bool Texture3D::loadFile(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) return false;
	C3D_Tex *t = new C3D_Tex;
	Tex3DS_Texture tx = Tex3DS_TextureImportStdio(f, t, nullptr, false);
	fclose(f);
	if (!tx) { delete t; return false; }
	Tex3DS_TextureFree(tx);
	C3D_TexSetFilter(t, GPU_LINEAR, GPU_LINEAR);
	if (t->maxLevel > 0) C3D_TexSetFilterMipmap(t, GPU_LINEAR);
	C3D_TexSetWrap(t, GPU_REPEAT, GPU_REPEAT);
	impl = t; w = t->width; h = t->height;
	return true;
}
void Texture3D::free() { if (impl) { C3D_TexDelete((C3D_Tex *)impl); delete (C3D_Tex *)impl; impl = nullptr; } }

namespace gfx3d {

static DVLB_s *g_dvlb;
static shaderProgram_s g_prog;
static int g_uProj, g_uMV;
static C3D_LightEnv g_lenv;
static C3D_Light g_lights[8];
static C3D_LightLut g_lutPhong;
static C3D_LightLutDA g_da[8];
static float g_daRange[8];
static float attnFn(float d, float range, float) { float t = 1 - d * d / (range * range); return t > 0 ? t * t : 0; }
static float g_lutShine = -1;
static bool g_lutBound;
static C3D_FogLut g_fog;
static C3D_Mtx g_proj;
static Mat4 g_view;
static const Scene3D *g_scene;
static bool g_init;

// Linearer Nebel zwischen fogNear und fogFar (gleiche Formel wie das PC-Backend)
static void fogLinear(C3D_FogLut *lut, float fn, float ff, float zn, float zf) {
	float data[256];
	for (int i = 0; i <= 128; i++) {
		float x = FogLut_CalcZ(i / 128.0f, zn, zf);
		float val = 1 - clampf((x - fn) / fmaxf(0.01f, ff - fn), 0, 1);
		if (i < 128) data[i] = val;
		if (i > 0) data[i + 127] = val - data[i - 1];
	}
	FogLut_FromArray(lut, data);
}
static float g_fogKey[4] = {-1, -1, -1, -1};

static void toC3D(const Mat4 &m, C3D_Mtx *o) {
	for (int i = 0; i < 4; i++) { o->r[i].x = m.m[i][0]; o->r[i].y = m.m[i][1]; o->r[i].z = m.m[i][2]; o->r[i].w = m.m[i][3]; }
}
static void rgb(Color c, float k, float out[3]) {
	out[0] = ((c.hex >> 16) & 255) / 255.f * k; out[1] = ((c.hex >> 8) & 255) / 255.f * k; out[2] = (c.hex & 255) / 255.f * k;
}

void begin(Gfx &g, const Camera3D &cam, const Scene3D &scene) {
	if (!g_init) {
		g_dvlb = DVLB_ParseFile((u32 *)lit_shbin, lit_shbin_size);
		shaderProgramInit(&g_prog);
		shaderProgramSetVsh(&g_prog, &g_dvlb->DVLE[0]);
		g_uProj = shaderInstanceGetUniformLocation(g_prog.vertexShader, "projection");
		g_uMV = shaderInstanceGetUniformLocation(g_prog.vertexShader, "modelView");
		g_init = true;
	}
	if (scene.clear.a > 0) g.rect(0, 0, g.width, g.height, scene.clear);
	C2D_Flush();
	g_scene = &scene;
	C3D_BindProgram(&g_prog);
	C3D_AttrInfo *ai = C3D_GetAttrInfo();
	AttrInfo_Init(ai);
	AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 3);
	AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2);
	AttrInfo_AddLoader(ai, 2, GPU_FLOAT, 3);
	AttrInfo_AddLoader(ai, 3, GPU_UNSIGNED_BYTE, 4);
	C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
	C3D_CullFace(GPU_CULL_BACK_CCW);
	C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
	bool top = g.screen == Screen::Top;
	float iod = g.eye * g.stereo / 3;
	Mtx_PerspStereoTilt(&g_proj, C3D_AngleFromDegrees(cam.fov), top ? C3D_AspectRatioTop : C3D_AspectRatioBot, cam.znear, cam.zfar, iod, cam.focus, false);
	g_view = Mat4::lookAt(cam.eye, cam.target, cam.up);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_uProj, &g_proj);
	// Licht
	C3D_LightEnvInit(&g_lenv);
	g_lutBound = false;
	C3D_LightEnvAmbient(&g_lenv, 1, 1, 1); // Umgebungslicht kommt aus material.ambient (= Szene)
	C3D_LightEnvBind(&g_lenv);
	for (int i = 0; i < scene.nLights && i < 8; i++) {
		const Light3D &L = scene.lights[i];
		C3D_LightInit(&g_lights[i], &g_lenv);
		float c[3];
		rgb(L.color, L.intensity, c);
		C3D_LightColor(&g_lights[i], c[0], c[1], c[2]);
		Vec3 p = L.point ? g_view.point(L.pos) : g_view.dir(L.pos);
		C3D_FVec v = FVec4_New(p.x, p.y, p.z, L.point ? 1.0f : 0.0f);
		C3D_LightPosition(&g_lights[i], &v);
		if (L.point) {
			if (g_daRange[i] != L.range) { LightLutDA_Create(&g_da[i], attnFn, 0, L.range, L.range, 0); g_daRange[i] = L.range; }
			C3D_LightDistAttn(&g_lights[i], &g_da[i]);
			C3D_LightDistAttnEnable(&g_lights[i], true);
		} else C3D_LightDistAttnEnable(&g_lights[i], false);
	}
	// Nebel
	if (scene.fog) {
		float key[4] = {scene.fogNear, scene.fogFar, cam.znear, cam.zfar};
		if (memcmp(key, g_fogKey, sizeof key)) { fogLinear(&g_fog, key[0], key[1], key[2], key[3]); memcpy(g_fogKey, key, sizeof key); }
		C3D_FogGasMode(GPU_FOG, GPU_PLAIN_DENSITY, false);
		C3D_FogColor((scene.fogColor.hex & 0xFF) << 16 | (scene.fogColor.hex & 0xFF00) | (scene.fogColor.hex >> 16 & 0xFF));
		C3D_FogLutBind(&g_fog);
	} else C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
}

void draw(const Mesh &m, const Mat4 &model, const Material &mat) {
	if (!m.v || !m.ni) return;
	C3D_Mtx mv;
	toC3D(g_view * model, &mv);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, g_uMV, &mv);
	// Material
	if (mat.lit) {
		// LUT nur bei Änderung neu hochladen (sonst läuft der Befehlspuffer bei vielen Objekten voll)
		if (g_lutShine != mat.shininess) { LightLut_Phong(&g_lutPhong, mat.shininess); g_lutShine = mat.shininess; g_lutBound = false; }
		if (!g_lutBound) { C3D_LightEnvLut(&g_lenv, GPU_LUT_D0, GPU_LUTINPUT_NH, false, &g_lutPhong); g_lutBound = true; }
		C3D_Material cm;
		memset(&cm, 0, sizeof cm);
		rgb(g_scene->ambient, 1, cm.ambient);
		cm.diffuse[0] = cm.diffuse[1] = cm.diffuse[2] = 1;
		cm.specular0[0] = cm.specular0[1] = cm.specular0[2] = mat.specular;
		rgb(mat.emission, 1, cm.emission);
		C3D_LightEnvMaterial(&g_lenv, &cm);
		C3D_LightEnvBind(&g_lenv);
	} else C3D_LightEnvBind(nullptr);
	// Kombinierer: Vertexfarbe * Materialfarbe (* Textur) (* Licht) (+ Glanz)
	C3D_TexEnv *e = C3D_GetTexEnv(0);
	C3D_TexEnvInit(e);
	C3D_TexEnvSrc(e, C3D_Both, GPU_PRIMARY_COLOR, GPU_CONSTANT, GPU_PRIMARY_COLOR);
	C3D_TexEnvFunc(e, C3D_Both, GPU_MODULATE);
	u32 mc = (u32(clampf(mat.color.a, 0, 1) * 255) << 24) | ((mat.color.hex & 0xFF) << 16) | (mat.color.hex & 0xFF00) | ((mat.color.hex >> 16) & 0xFF);
	C3D_TexEnvColor(e, mc);
	e = C3D_GetTexEnv(1);
	C3D_TexEnvInit(e);
	if (mat.tex && mat.tex->impl) {
		C3D_TexBind(0, (C3D_Tex *)mat.tex->impl);
		C3D_TexEnvSrc(e, C3D_Both, GPU_PREVIOUS, GPU_TEXTURE0, GPU_PREVIOUS);
		C3D_TexEnvFunc(e, C3D_Both, GPU_MODULATE);
	}
	e = C3D_GetTexEnv(2);
	C3D_TexEnvInit(e);
	if (mat.lit) {
		C3D_TexEnvSrc(e, C3D_RGB, GPU_PREVIOUS, GPU_FRAGMENT_PRIMARY_COLOR, GPU_PREVIOUS);
		C3D_TexEnvFunc(e, C3D_RGB, GPU_MODULATE);
	}
	e = C3D_GetTexEnv(3);
	C3D_TexEnvInit(e);
	if (mat.lit && mat.specular > 0) {
		C3D_TexEnvSrc(e, C3D_RGB, GPU_PREVIOUS, GPU_FRAGMENT_SECONDARY_COLOR, GPU_PREVIOUS);
		C3D_TexEnvFunc(e, C3D_RGB, GPU_ADD);
	}
	C3D_BufInfo *bi = C3D_GetBufInfo();
	BufInfo_Init(bi);
	BufInfo_Add(bi, m.v, sizeof(Vertex3D), 4, 0x3210);
	if (mat.additive) {
		C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE, GPU_SRC_ALPHA, GPU_ONE);
		C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_COLOR);
	}
	C3D_DrawElements(GPU_TRIANGLES, m.ni, C3D_UNSIGNED_SHORT, m.idx);
	if (mat.additive) {
		C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
		C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
	}
}

void end(Gfx &g) {
	(void)g;
	for (int i = 1; i < 6; i++) C3D_TexEnvInit(C3D_GetTexEnv(i));
	C3D_LightEnvBind(nullptr);
	C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
	C3D_CullFace(GPU_CULL_NONE);
	C2D_Prepare();
	C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
}

} // namespace gfx3d
} // namespace forge
