#pragma once
#include "3ds.h"
typedef struct C3D_RenderTarget { int scr, side; } C3D_RenderTarget;
typedef struct C2D_TextBuf_s *C2D_TextBuf;
typedef struct { const char *str; } C2D_Text;
#define C3D_DEFAULT_CMDBUF_SIZE 0
#define C3D_FRAME_SYNCDRAW 0
enum { GPU_ALWAYS, GPU_WRITE_COLOR };
enum { C2D_AlignLeft = 0, C2D_WithColor = 1, C2D_AlignRight = 4, C2D_AlignCenter = 8, C2D_WordWrap = 32 };
static inline u32 C2D_Color32(u8 r, u8 g, u8 b, u8 a) { return r | (g << 8) | (b << 16) | ((u32)a << 24); }
void C3D_Init(int); void C3D_Fini(void); void C3D_DepthTest(bool, int, int);
void C3D_FrameBegin(int); void C3D_FrameEnd(int);
void C2D_Init(int); void C2D_Prepare(void); void C2D_Fini(void);
C3D_RenderTarget *C2D_CreateScreenTarget(gfxScreen_t, gfx3dSide_t);
void C2D_TargetClear(C3D_RenderTarget *, u32); void C2D_SceneBegin(C3D_RenderTarget *);
void C2D_DrawRectSolid(float x, float y, float z, float w, float h, u32 c);
void C2D_DrawRectangle(float x, float y, float z, float w, float h, u32 c0, u32 c1, u32 c2, u32 c3);
void C2D_DrawCircleSolid(float x, float y, float z, float r, u32 c);
void C2D_DrawCircle(float x, float y, float z, float r, u32 c0, u32 c1, u32 c2, u32 c3);
void C2D_DrawTriangle(float x0, float y0, u32 c0, float x1, float y1, u32 c1, float x2, float y2, u32 c2, float d);
void C2D_DrawLine(float x0, float y0, u32 c0, float x1, float y1, u32 c1, float th, float d);
C2D_TextBuf C2D_TextBufNew(int); void C2D_TextBufClear(C2D_TextBuf); void C2D_TextBufDelete(C2D_TextBuf);
const char *C2D_TextParse(C2D_Text *, C2D_TextBuf, const char *);
static inline void C2D_TextOptimize(C2D_Text *t) { (void)t; }
void C2D_DrawText(const C2D_Text *, u32 flags, float x, float y, float z, float sx, float sy, ...);
void C2D_TextGetDimensions(const C2D_Text *, float sx, float sy, float *w, float *h);
