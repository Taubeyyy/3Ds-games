// Minimaler libctru-Ersatz fuer Desktop-Tests (Screenshots)
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int16_t s16; typedef int32_t s32; typedef int32_t Result;
#define R_FAILED(r) ((r) < 0)
enum { KEY_A = 1, KEY_B = 2, KEY_SELECT = 4, KEY_START = 8, KEY_RIGHT = 16, KEY_LEFT = 32, KEY_UP = 64, KEY_DOWN = 128,
	KEY_R = 256, KEY_L = 512, KEY_X = 1024, KEY_Y = 2048, KEY_TOUCH = 1 << 20, KEY_CPAD_UP = 1 << 30, KEY_CPAD_DOWN = 1u << 31 };
typedef struct { u16 px, py; } touchPosition;
typedef enum { GFX_TOP, GFX_BOTTOM } gfxScreen_t;
typedef enum { GFX_LEFT, GFX_RIGHT } gfx3dSide_t;
typedef enum { APTHOOK_ONSUSPEND, APTHOOK_ONRESTORE, APTHOOK_ONSLEEP, APTHOOK_ONWAKEUP, APTHOOK_ONEXIT } APT_HookType;
typedef void (*aptHookFn)(APT_HookType, void *);
typedef struct { int x; } aptHookCookie;
void gfxInitDefault(void); void gfxExit(void); void gfxSet3D(bool);
void osSetSpeedupEnable(bool); u64 osGetTime(void); float osGet3DSliderState(void);
bool aptMainLoop(void); void aptHook(aptHookCookie *, aptHookFn, void *);
void hidScanInput(void); u32 hidKeysDown(void); u32 hidKeysHeld(void); u32 hidKeysUp(void); void hidTouchRead(touchPosition *);
typedef struct { void *data_vaddr; u32 nsamples; } ndspWaveBuf;
enum { NDSP_OUTPUT_STEREO, NDSP_INTERP_LINEAR, NDSP_FORMAT_MONO_PCM16 };
static inline Result ndspInit(void) { return -1; }
static inline void ndspExit(void) {}
static inline void ndspSetOutputMode(int m) { (void)m; }
static inline void ndspChnReset(int c) { (void)c; }
static inline void ndspChnSetInterp(int c, int m) { (void)c; (void)m; }
static inline void ndspChnSetRate(int c, float r) { (void)c; (void)r; }
static inline void ndspChnSetFormat(int c, int f) { (void)c; (void)f; }
static inline void ndspChnWaveBufClear(int c) { (void)c; }
static inline void ndspChnSetMix(int c, float *m) { (void)c; (void)m; }
static inline void ndspChnWaveBufAdd(int c, ndspWaveBuf *w) { (void)c; (void)w; }
void *linearAlloc(size_t); void linearFree(void *);
static inline void DSP_FlushDataCache(void *p, u32 n) { (void)p; (void)n; }
FILE *shim_fopen(const char *, const char *); int shim_mkdir(const char *, int); int shim_remove(const char *); int shim_rename(const char *, const char *);
#define fopen shim_fopen
#define mkdir shim_mkdir
#define remove shim_remove
#define rename shim_rename
typedef struct { s16 dx, dy; } circlePosition;
void hidCircleRead(circlePosition *);
