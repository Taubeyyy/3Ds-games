// SKYBREAKER – Raster-Räume: verschiedene Grundrisse, Wände, Tore, Fackeln, Kollision
#pragma once
#include "game.hpp"

constexpr int GW = 17, GH = 13;            // Zellen (1 m), Mitte bei (0,0)
enum Layout { LY_ROUND, LY_HALL, LY_CROSS, LY_L, LY_PILLARS, LY_DIAMOND, LY_TWIN, LY_COUNT };
extern const char *LAYOUT_NAME[LY_COUNT];

struct RoomGrid {
	u8 cell[GH][GW];                         // 1 = Boden
	int layout = LY_ROUND;
	Vec3 entry;                              // Startpunkt der Heldin
	Vec3 doorSlot[3];
	int nSlots = 0;
	Vec3 torch[10];
	int nTorch = 0;
	Vec3 pillar[6];
	int nPillar = 0;
	float minX = -4, maxX = 4, minZ = -4, maxZ = 4;
	bool floorAt(float x, float z) const {
		int i = int(floorf(x + GW * 0.5f)), j = int(floorf(z + GH * 0.5f));
		return i >= 0 && j >= 0 && i < GW && j < GH && cell[j][i];
	}
	Vec3 cellCenter(int i, int j) const { return Vec3(i - GW * 0.5f + 0.5f, 0.075f, j - GH * 0.5f + 0.5f); }
	// Kreis gegen Wände: verschiebt p aus blockierten Zellen
	void collide(Vec3 &p, float r) const;
	Vec3 randomFloor(Rng &rng, Vec3 awayFrom, float minDist) const;
};

// Baut den Raum: Grundriss + statisches Mesh (Boden + Wände) + Leuchtteile (Fackelflammen/Kristalle)
void buildRoom(RoomGrid &g, int layout, bool mirror, int biome, u32 seed, Mesh &mesh, Mesh &glow);
