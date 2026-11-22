// generiert von art/build_all.py
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "seren_body_fmesh.h"
#include "seren_arm_fmesh.h"
#include "seren_arm_l_fmesh.h"
#include "seren_leg_l_fmesh.h"
#include "seren_leg_r_fmesh.h"
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
#include "e_archer_body_fmesh.h"
#include "e_archer_glow_fmesh.h"
#include "e_bomber_body_fmesh.h"
#include "e_bomber_glow_fmesh.h"
#include "e_brute_body_fmesh.h"
#include "e_brute_glow_fmesh.h"
#include "e_crawler_body_fmesh.h"
#include "e_crawler_glow_fmesh.h"
#include "e_geode_body_fmesh.h"
#include "e_geode_glow_fmesh.h"
#include "e_queen_body_fmesh.h"
#include "e_queen_glow_fmesh.h"
#include "e_shard_body_fmesh.h"
#include "e_shard_glow_fmesh.h"
#include "e_spider_body_fmesh.h"
#include "e_spider_glow_fmesh.h"
#include "e_wisp_body_fmesh.h"
#include "e_wisp_glow_fmesh.h"
#include "r_anvil_fmesh.h"
#include "r_chest_fmesh.h"
#include "r_coins_fmesh.h"
#include "r_column_fmesh.h"
#include "r_door_fmesh.h"
#include "r_fountain_fmesh.h"
#include "r_gem_fmesh.h"
#include "r_heart_fmesh.h"
#include "r_portal_fmesh.h"
#include "r_rock_fmesh.h"
#include "r_shrine_fmesh.h"
#include "r_water_fmesh.h"
#include "w_arena_fmesh.h"
#include "w_arena_cave_fmesh.h"
#include "w_arena_dark_fmesh.h"
#include "w_arena_glow_fmesh.h"
#include "w_brazier_fmesh.h"
#include "w_brazier_fire_fmesh.h"
#include "w_cave_fmesh.h"
#include "w_cave_glow_fmesh.h"
#include "w_dummy_base_fmesh.h"
#include "w_dummy_top_fmesh.h"
#include "w_dwall_fmesh.h"
#include "w_dwall_fire_fmesh.h"
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
enum PartKind { PK_BODY, PK_ARM, PK_HAIR, PK_SCARF, PK_CAPE, PK_GLOW, PK_FX, PK_WEAPON, PK_WEAPON_GLOW, PK_ORBIT, PK_ORBIT_GLOW, PK_PET, PK_ARM_L, PK_LEG_L, PK_LEG_R, PK_COUNT };
struct PartSrc { int ch, kind; const uint8_t *data; size_t size; };
static const PartSrc PART_SRC[] = {
	{0, PK_BODY, seren_body_fmesh, seren_body_fmesh_size},
	{0, PK_ARM, seren_arm_fmesh, seren_arm_fmesh_size},
	{0, PK_ARM_L, seren_arm_l_fmesh, seren_arm_l_fmesh_size},
	{0, PK_LEG_L, seren_leg_l_fmesh, seren_leg_l_fmesh_size},
	{0, PK_LEG_R, seren_leg_r_fmesh, seren_leg_r_fmesh_size},
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
enum WorldMesh { WM_E_ARCHER_BODY, WM_E_ARCHER_GLOW, WM_E_BOMBER_BODY, WM_E_BOMBER_GLOW, WM_E_BRUTE_BODY, WM_E_BRUTE_GLOW, WM_E_CRAWLER_BODY, WM_E_CRAWLER_GLOW, WM_E_GEODE_BODY, WM_E_GEODE_GLOW, WM_E_QUEEN_BODY, WM_E_QUEEN_GLOW, WM_E_SHARD_BODY, WM_E_SHARD_GLOW, WM_E_SPIDER_BODY, WM_E_SPIDER_GLOW, WM_E_WISP_BODY, WM_E_WISP_GLOW, WM_R_ANVIL, WM_R_CHEST, WM_R_COINS, WM_R_COLUMN, WM_R_DOOR, WM_R_FOUNTAIN, WM_R_GEM, WM_R_HEART, WM_R_PORTAL, WM_R_ROCK, WM_R_SHRINE, WM_R_WATER, WM_ARENA, WM_ARENA_CAVE, WM_ARENA_DARK, WM_ARENA_GLOW, WM_BRAZIER, WM_BRAZIER_FIRE, WM_CAVE, WM_CAVE_GLOW, WM_DUMMY_BASE, WM_DUMMY_TOP, WM_DWALL, WM_DWALL_FIRE, WM_ISLAND, WM_ISLAND_PLAIN, WM_PEDESTAL, WM_PEDESTAL_GLOW, WM_SHARD_A, WM_SHARD_B, WM_SHARD_C, WM_TOWER, WM_TOWER_CRYSTALS, WM_TOWER_GLOW, WM_COUNT };
static const PartSrc WORLD_SRC[] = {
	{0, 0, e_archer_body_fmesh, e_archer_body_fmesh_size},
	{1, 0, e_archer_glow_fmesh, e_archer_glow_fmesh_size},
	{2, 0, e_bomber_body_fmesh, e_bomber_body_fmesh_size},
	{3, 0, e_bomber_glow_fmesh, e_bomber_glow_fmesh_size},
	{4, 0, e_brute_body_fmesh, e_brute_body_fmesh_size},
	{5, 0, e_brute_glow_fmesh, e_brute_glow_fmesh_size},
	{6, 0, e_crawler_body_fmesh, e_crawler_body_fmesh_size},
	{7, 0, e_crawler_glow_fmesh, e_crawler_glow_fmesh_size},
	{8, 0, e_geode_body_fmesh, e_geode_body_fmesh_size},
	{9, 0, e_geode_glow_fmesh, e_geode_glow_fmesh_size},
	{10, 0, e_queen_body_fmesh, e_queen_body_fmesh_size},
	{11, 0, e_queen_glow_fmesh, e_queen_glow_fmesh_size},
	{12, 0, e_shard_body_fmesh, e_shard_body_fmesh_size},
	{13, 0, e_shard_glow_fmesh, e_shard_glow_fmesh_size},
	{14, 0, e_spider_body_fmesh, e_spider_body_fmesh_size},
	{15, 0, e_spider_glow_fmesh, e_spider_glow_fmesh_size},
	{16, 0, e_wisp_body_fmesh, e_wisp_body_fmesh_size},
	{17, 0, e_wisp_glow_fmesh, e_wisp_glow_fmesh_size},
	{18, 0, r_anvil_fmesh, r_anvil_fmesh_size},
	{19, 0, r_chest_fmesh, r_chest_fmesh_size},
	{20, 0, r_coins_fmesh, r_coins_fmesh_size},
	{21, 0, r_column_fmesh, r_column_fmesh_size},
	{22, 0, r_door_fmesh, r_door_fmesh_size},
	{23, 0, r_fountain_fmesh, r_fountain_fmesh_size},
	{24, 0, r_gem_fmesh, r_gem_fmesh_size},
	{25, 0, r_heart_fmesh, r_heart_fmesh_size},
	{26, 0, r_portal_fmesh, r_portal_fmesh_size},
	{27, 0, r_rock_fmesh, r_rock_fmesh_size},
	{28, 0, r_shrine_fmesh, r_shrine_fmesh_size},
	{29, 0, r_water_fmesh, r_water_fmesh_size},
	{30, 0, w_arena_fmesh, w_arena_fmesh_size},
	{31, 0, w_arena_cave_fmesh, w_arena_cave_fmesh_size},
	{32, 0, w_arena_dark_fmesh, w_arena_dark_fmesh_size},
	{33, 0, w_arena_glow_fmesh, w_arena_glow_fmesh_size},
	{34, 0, w_brazier_fmesh, w_brazier_fmesh_size},
	{35, 0, w_brazier_fire_fmesh, w_brazier_fire_fmesh_size},
	{36, 0, w_cave_fmesh, w_cave_fmesh_size},
	{37, 0, w_cave_glow_fmesh, w_cave_glow_fmesh_size},
	{38, 0, w_dummy_base_fmesh, w_dummy_base_fmesh_size},
	{39, 0, w_dummy_top_fmesh, w_dummy_top_fmesh_size},
	{40, 0, w_dwall_fmesh, w_dwall_fmesh_size},
	{41, 0, w_dwall_fire_fmesh, w_dwall_fire_fmesh_size},
	{42, 0, w_island_fmesh, w_island_fmesh_size},
	{43, 0, w_island_plain_fmesh, w_island_plain_fmesh_size},
	{44, 0, w_pedestal_fmesh, w_pedestal_fmesh_size},
	{45, 0, w_pedestal_glow_fmesh, w_pedestal_glow_fmesh_size},
	{46, 0, w_shard_a_fmesh, w_shard_a_fmesh_size},
	{47, 0, w_shard_b_fmesh, w_shard_b_fmesh_size},
	{48, 0, w_shard_c_fmesh, w_shard_c_fmesh_size},
	{49, 0, w_tower_fmesh, w_tower_fmesh_size},
	{50, 0, w_tower_crystals_fmesh, w_tower_crystals_fmesh_size},
	{51, 0, w_tower_glow_fmesh, w_tower_glow_fmesh_size},
};
