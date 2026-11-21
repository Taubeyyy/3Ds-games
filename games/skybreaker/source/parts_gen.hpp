// generiert von art/build_all.py
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "seren_body_fmesh.h"
#include "seren_arm_fmesh.h"
#include "seren_hair_fmesh.h"
#include "seren_scarf_fmesh.h"
#include "seren_cape_fmesh.h"
#include "seren_glow_fmesh.h"
#include "seren_weapon_fmesh.h"
#include "seren_weapon_glow_fmesh.h"
#include "ignara_body_fmesh.h"
#include "ignara_fx_fmesh.h"
#include "ignara_glow_fmesh.h"
#include "thalor_body_fmesh.h"
#include "thalor_cape_fmesh.h"
#include "thalor_fx_fmesh.h"
#include "thalor_weapon_fmesh.h"
#include "thalor_weapon_glow_fmesh.h"
#include "nerys_body_fmesh.h"
#include "nerys_hair_fmesh.h"
#include "nerys_weapon_fmesh.h"
#include "nerys_weapon_glow_fmesh.h"
#include "nerys_orbit_glow_fmesh.h"
#include "vexa_body_fmesh.h"
#include "vexa_cape_fmesh.h"
#include "vexa_glow_fmesh.h"
#include "vexa_weapon_fmesh.h"
#include "vexa_orbit_fmesh.h"
#include "korrin_body_fmesh.h"
#include "korrin_glow_fmesh.h"
#include "korrin_weapon_fmesh.h"
#include "korrin_weapon_glow_fmesh.h"
#include "lyra_body_fmesh.h"
#include "lyra_hair_fmesh.h"
#include "lyra_glow_fmesh.h"
#include "lyra_weapon_fmesh.h"
#include "lyra_weapon_glow_fmesh.h"
#include "lyra_orbit_glow_fmesh.h"
#include "sylv_body_fmesh.h"
#include "sylv_cape_fmesh.h"
#include "sylv_glow_fmesh.h"
#include "sylv_weapon_fmesh.h"
#include "sylv_weapon_glow_fmesh.h"
#include "sylv_pet_fmesh.h"
#include "w_arena_fmesh.h"
#include "w_arena_glow_fmesh.h"
#include "w_brazier_fmesh.h"
#include "w_brazier_fire_fmesh.h"
#include "w_dummy_base_fmesh.h"
#include "w_dummy_top_fmesh.h"
#include "w_island_fmesh.h"
#include "w_island_plain_fmesh.h"
#include "w_pedestal_fmesh.h"
#include "w_pedestal_glow_fmesh.h"
#include "w_shard_a_fmesh.h"
#include "w_shard_b_fmesh.h"
#include "w_shard_c_fmesh.h"
#include "w_tower_fmesh.h"
#include "w_tower_crystals_fmesh.h"
#include "w_tower_glow_fmesh.h"
enum CharId { CH_SEREN, CH_IGNARA, CH_THALOR, CH_NERYS, CH_VEXA, CH_KORRIN, CH_LYRA, CH_SYLV, CH_COUNT };
enum PartKind { PK_BODY, PK_ARM, PK_HAIR, PK_SCARF, PK_CAPE, PK_GLOW, PK_FX, PK_WEAPON, PK_WEAPON_GLOW, PK_ORBIT, PK_ORBIT_GLOW, PK_PET, PK_COUNT };
struct PartSrc { int ch, kind; const uint8_t *data; size_t size; };
static const PartSrc PART_SRC[] = {
	{0, PK_BODY, seren_body_fmesh, seren_body_fmesh_size},
	{0, PK_ARM, seren_arm_fmesh, seren_arm_fmesh_size},
	{0, PK_HAIR, seren_hair_fmesh, seren_hair_fmesh_size},
	{0, PK_SCARF, seren_scarf_fmesh, seren_scarf_fmesh_size},
	{0, PK_CAPE, seren_cape_fmesh, seren_cape_fmesh_size},
	{0, PK_GLOW, seren_glow_fmesh, seren_glow_fmesh_size},
	{0, PK_WEAPON, seren_weapon_fmesh, seren_weapon_fmesh_size},
	{0, PK_WEAPON_GLOW, seren_weapon_glow_fmesh, seren_weapon_glow_fmesh_size},
	{1, PK_BODY, ignara_body_fmesh, ignara_body_fmesh_size},
	{1, PK_FX, ignara_fx_fmesh, ignara_fx_fmesh_size},
	{1, PK_GLOW, ignara_glow_fmesh, ignara_glow_fmesh_size},
	{2, PK_BODY, thalor_body_fmesh, thalor_body_fmesh_size},
	{2, PK_CAPE, thalor_cape_fmesh, thalor_cape_fmesh_size},
	{2, PK_FX, thalor_fx_fmesh, thalor_fx_fmesh_size},
	{2, PK_WEAPON, thalor_weapon_fmesh, thalor_weapon_fmesh_size},
	{2, PK_WEAPON_GLOW, thalor_weapon_glow_fmesh, thalor_weapon_glow_fmesh_size},
	{3, PK_BODY, nerys_body_fmesh, nerys_body_fmesh_size},
	{3, PK_HAIR, nerys_hair_fmesh, nerys_hair_fmesh_size},
	{3, PK_WEAPON, nerys_weapon_fmesh, nerys_weapon_fmesh_size},
	{3, PK_WEAPON_GLOW, nerys_weapon_glow_fmesh, nerys_weapon_glow_fmesh_size},
	{3, PK_ORBIT_GLOW, nerys_orbit_glow_fmesh, nerys_orbit_glow_fmesh_size},
	{4, PK_BODY, vexa_body_fmesh, vexa_body_fmesh_size},
	{4, PK_CAPE, vexa_cape_fmesh, vexa_cape_fmesh_size},
	{4, PK_GLOW, vexa_glow_fmesh, vexa_glow_fmesh_size},
	{4, PK_WEAPON, vexa_weapon_fmesh, vexa_weapon_fmesh_size},
	{4, PK_ORBIT, vexa_orbit_fmesh, vexa_orbit_fmesh_size},
	{5, PK_BODY, korrin_body_fmesh, korrin_body_fmesh_size},
	{5, PK_GLOW, korrin_glow_fmesh, korrin_glow_fmesh_size},
	{5, PK_WEAPON, korrin_weapon_fmesh, korrin_weapon_fmesh_size},
	{5, PK_WEAPON_GLOW, korrin_weapon_glow_fmesh, korrin_weapon_glow_fmesh_size},
	{6, PK_BODY, lyra_body_fmesh, lyra_body_fmesh_size},
	{6, PK_HAIR, lyra_hair_fmesh, lyra_hair_fmesh_size},
	{6, PK_GLOW, lyra_glow_fmesh, lyra_glow_fmesh_size},
	{6, PK_WEAPON, lyra_weapon_fmesh, lyra_weapon_fmesh_size},
	{6, PK_WEAPON_GLOW, lyra_weapon_glow_fmesh, lyra_weapon_glow_fmesh_size},
	{6, PK_ORBIT_GLOW, lyra_orbit_glow_fmesh, lyra_orbit_glow_fmesh_size},
	{7, PK_BODY, sylv_body_fmesh, sylv_body_fmesh_size},
	{7, PK_CAPE, sylv_cape_fmesh, sylv_cape_fmesh_size},
	{7, PK_GLOW, sylv_glow_fmesh, sylv_glow_fmesh_size},
	{7, PK_WEAPON, sylv_weapon_fmesh, sylv_weapon_fmesh_size},
	{7, PK_WEAPON_GLOW, sylv_weapon_glow_fmesh, sylv_weapon_glow_fmesh_size},
	{7, PK_PET, sylv_pet_fmesh, sylv_pet_fmesh_size},
};
enum WorldMesh { WM_ARENA, WM_ARENA_GLOW, WM_BRAZIER, WM_BRAZIER_FIRE, WM_DUMMY_BASE, WM_DUMMY_TOP, WM_ISLAND, WM_ISLAND_PLAIN, WM_PEDESTAL, WM_PEDESTAL_GLOW, WM_SHARD_A, WM_SHARD_B, WM_SHARD_C, WM_TOWER, WM_TOWER_CRYSTALS, WM_TOWER_GLOW, WM_COUNT };
static const PartSrc WORLD_SRC[] = {
	{0, 0, w_arena_fmesh, w_arena_fmesh_size},
	{1, 0, w_arena_glow_fmesh, w_arena_glow_fmesh_size},
	{2, 0, w_brazier_fmesh, w_brazier_fmesh_size},
	{3, 0, w_brazier_fire_fmesh, w_brazier_fire_fmesh_size},
	{4, 0, w_dummy_base_fmesh, w_dummy_base_fmesh_size},
	{5, 0, w_dummy_top_fmesh, w_dummy_top_fmesh_size},
	{6, 0, w_island_fmesh, w_island_fmesh_size},
	{7, 0, w_island_plain_fmesh, w_island_plain_fmesh_size},
	{8, 0, w_pedestal_fmesh, w_pedestal_fmesh_size},
	{9, 0, w_pedestal_glow_fmesh, w_pedestal_glow_fmesh_size},
	{10, 0, w_shard_a_fmesh, w_shard_a_fmesh_size},
	{11, 0, w_shard_b_fmesh, w_shard_b_fmesh_size},
	{12, 0, w_shard_c_fmesh, w_shard_c_fmesh_size},
	{13, 0, w_tower_fmesh, w_tower_fmesh_size},
	{14, 0, w_tower_crystals_fmesh, w_tower_crystals_fmesh_size},
	{15, 0, w_tower_glow_fmesh, w_tower_glow_fmesh_size},
};
