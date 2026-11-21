# FORGE – Technik-Notizen (Recherche)

## Bibliotheken im devkitPro-Image (portlibs/3ds)
- Physik: **Box2D 2.4** (C++: b2World, b2Body, b2Fixture), **Bullet** (C++)
- Audio: libopusfile/opus, libvorbisidec (tremor), libxmp, libmodplug, mpg123, FLAC
- Sonstiges: freetype, libpng, libjpeg/turbojpeg, lua5.1, physfs, zstd, lz4, zlib, minizip, tinyxml2, jansson
- → Engine in C++17 (ohne Exceptions/RTTI), damit Box2D/Bullet direkt nutzbar sind

## Grafik (citro3d)
- Shader: PICA200-Assembler `.v.pica` → picasso → `*_shbin.h`; `DVLB_ParseFile`, `shaderProgramInit/SetVsh`, `C3D_BindProgram`,
  `shaderInstanceGetUniformLocation`, `C3D_FVUnifMtx4x4`
- Vertexformat: `AttrInfo_AddLoader(attr, reg, GPU_FLOAT, n)`, `BufInfo_Add(buf, ptr, stride, nattr, 0x3210)`, `C3D_DrawArrays/DrawElements`
- Render-Targets: `C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8)`, `C3D_RenderTargetSetOutput(t, GFX_TOP, GFX_LEFT, flags)`,
  `C3D_FrameDrawOn(t)`; Render-to-Texture über Textur-Ziel (composite_scene-Beispiel)
- Stereo: `Mtx_PerspStereoTilt(&proj, fov, C3D_AspectRatioTop, near, far, iod, focal, false)`, je Auge eigenes Target
- **Fragment-Licht** (Hardware, max 8 Lichter): `C3D_LightEnvInit/Bind/Material`, `C3D_LightInit/Color/Position`,
  LUTs (`LightLut_Phong`), **Normal-Mapping**: `C3D_LightEnvBumpMode(GPU_BUMP_AS_BUMP)`, `BumpSel(1)`;
  Vertex-Shader gibt `normalquat` + `view` aus (Quaternion aus N/T, siehe normal_mapping-Beispiel)
  → Idee: normal-gemappte 2D-Sprites mit dynamischen Lichtern
- Nebel: `FogLut_Exp`, `C3D_FogGasMode(GPU_FOG, GPU_PLAIN_DENSITY,false)`, `C3D_FogColor`, `C3D_FogLutBind`
- Blending: additiv `C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE,...)`
- TexEnv-Kombinierer (6 Stufen): `C3D_TexEnvSrc/Func`, Primary/Secondary Fragment Color aus Licht
- Prozedurale Texturen in Hardware: `C3D_ProcTex*` (Rauschen, Holz, Marmor)
- Immediate Mode: `C3D_ImmDrawBegin/SendAttrib/End` (für Debug)
- Mipmaps: `C3D_TexSetFilterMipmap`, tex3ds `-m lanczos`

## Texturen (tex3ds)
- Formate: rgba8, rgb8, rgba5551, rgb565, rgba4, la8, la4, l8, a8, **etc1, etc1a4, auto-etc1**
- `-m <filter>` Mipmaps, `-q high` ETC1-Qualität, `-a` Atlas, `-b edge` Rand gegen Kantenflimmern, `-s` Skybox, `-c` Cubemap, `-z auto`
- Laden: `Tex3DS_TextureImport(data,size,&tex,NULL,false)` bzw. `...ImportStdio(FILE*)` von RomFS

## RomFS
- Makefile: `ROMFS := romfs` → 3dsx/CIA enthalten den Ordner; im Code `romfsInit()`, Pfade `romfs:/...`
- CIA: RSF braucht `RomFs: RootPath: $(APP_ROMFS)` (in tools/cia.rsf wieder aktivieren, wenn genutzt)

## Audio
- Streaming (opus-decoding-Beispiel): eigener Thread, 3 WaveBufs à 120 ms, `ndspSetCallback` + `LightEvent` zum Nachfüllen,
  `op_open_file`, `op_read_stereo`, 48 kHz
- Effekte: kurze PCM16-Puffer im linearen Speicher, Kanäle rotieren (wie in den bisherigen Spielen)

## Lehren aus den bisherigen Spielen
- Große Strukturen nie auf den Stack (32 KB Standard) → statisch/Arena; zusätzlich `__stacksize__`
- Halbtransparente Rundrechtecke als Dreiecksfächer (keine überlappenden Kreise)
- Desktop-Shim + Skript-Eingabe + PPM-Screenshots = unverzichtbar zum Testen ohne Hardware
