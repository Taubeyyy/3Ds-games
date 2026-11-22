// SKYBREAKER – Raster-Räume
#include <cstring>
#include "rooms.hpp"

const char *LAYOUT_NAME[LY_COUNT] = {"Rundsaal", "Lange Halle", "Kreuzgang", "Winkelsaal", "Säulenhalle", "Rautensaal", "Doppelkammer"};

static void fill(RoomGrid &g, int i0, int j0, int i1, int j1) {
	for (int j = j0; j <= j1; j++)
		for (int i = i0; i <= i1; i++)
			if (i > 0 && j > 0 && i < GW - 1 && j < GH - 1) g.cell[j][i] = 1;
}

void RoomGrid::collide(Vec3 &p, float r) const {
	int ci = int(floorf(p.x + GW * 0.5f)), cj = int(floorf(p.z + GH * 0.5f));
	for (int j = cj - 1; j <= cj + 1; j++)
		for (int i = ci - 1; i <= ci + 1; i++) {
			bool blocked = i < 0 || j < 0 || i >= GW || j >= GH || !cell[j][i];
			if (!blocked) continue;
			float x0 = i - GW * 0.5f, z0 = j - GH * 0.5f;
			float cx = clampf(p.x, x0, x0 + 1), cz = clampf(p.z, z0, z0 + 1);
			float dx = p.x - cx, dz = p.z - cz;
			float d = sqrtf(dx * dx + dz * dz);
			if (d < r) {
				if (d < 1e-4f) { dx = p.x - (x0 + 0.5f); dz = p.z - (z0 + 0.5f); d = sqrtf(dx * dx + dz * dz) + 1e-4f; p.x = x0 + 0.5f + dx / d * (0.5f + r); p.z = z0 + 0.5f + dz / d * (0.5f + r); }
				else { p.x = cx + dx / d * r; p.z = cz + dz / d * r; }
			}
		}
}

Vec3 RoomGrid::randomFloor(Rng &rng, Vec3 away, float minDist) const {
	Vec3 best;
	for (int k = 0; k < 40; k++) {
		int i = rng.irange(1, GW - 2), j = rng.irange(1, GH - 2);
		if (!cell[j][i]) continue;
		Vec3 p = cellCenter(i, j) + Vec3(rng.range(-0.3f, 0.3f), 0, rng.range(-0.3f, 0.3f));
		best = p;
		if (Vec3(p.x - away.x, 0, p.z - away.z).len() >= minDist) return p;
	}
	return best;
}

static Color pick(Rng &r, const u32 *c, int n) { return Color(c[r.irange(0, n - 1)]); }

// Quader mit Welt-UVs (Textur kachelt alle 2 m), Seiten nur wo nötig
static void boxFaces(MeshBuilder &b, Vec3 c, Vec3 sz, Color col, bool top = true) {
	float x0 = c.x - sz.x / 2, x1 = c.x + sz.x / 2, y0 = c.y, y1 = c.y + sz.y, z0 = c.z - sz.z / 2, z1 = c.z + sz.z / 2;
	Color dk = col.mix(Color(0x000000), 0.18f);                 // unten etwas dunkler (Umgebungsverdeckung)
	b.quad(Vec3(x0, y0, z1), Vec3(x1, y0, z1), Vec3(x1, y1, z1), Vec3(x0, y1, z1), dk, dk, col, col, Vec2(x0 / 2, -y0 / 2), Vec2(x1 / 2, -y1 / 2));
	b.quad(Vec3(x1, y0, z0), Vec3(x0, y0, z0), Vec3(x0, y1, z0), Vec3(x1, y1, z0), dk, dk, col, col, Vec2(-x1 / 2, -y0 / 2), Vec2(-x0 / 2, -y1 / 2));
	b.quad(Vec3(x1, y0, z1), Vec3(x1, y0, z0), Vec3(x1, y1, z0), Vec3(x1, y1, z1), dk, dk, col, col, Vec2(-z1 / 2, -y0 / 2), Vec2(-z0 / 2, -y1 / 2));
	b.quad(Vec3(x0, y0, z0), Vec3(x0, y0, z1), Vec3(x0, y1, z1), Vec3(x0, y1, z0), dk, dk, col, col, Vec2(z0 / 2, -y0 / 2), Vec2(z1 / 2, -y1 / 2));
	if (top) { Color tc = col.mix(Color(0x000000), 0.25f); b.quad(Vec3(x0, y1, z0), Vec3(x0, y1, z1), Vec3(x1, y1, z1), Vec3(x1, y1, z0), tc, tc, tc, tc, Vec2(x0 / 2, z0 / 2), Vec2(x1 / 2, z1 / 2)); }
}

void buildRoom(RoomGrid &g, int layout, bool mirror, int biome, u32 seed, Mesh &floorMesh, Mesh &wallMesh, Mesh &glow) {
	memset(g.cell, 0, sizeof g.cell);
	g.layout = layout;
	g.nSlots = g.nTorch = g.nPillar = 0;
	Rng r(seed | 1);
	const int cx = GW / 2, cz = GH / 2;
	switch (layout) {
	case LY_ROUND:
		for (int j = 0; j < GH; j++)
			for (int i = 0; i < GW; i++) { float dx = i - cx, dz = j - cz; if (dx * dx + dz * dz <= 4.7f * 4.7f) fill(g, i, j, i, j); }
		break;
	case LY_HALL: fill(g, 2, 3, 14, 9); break;
	case LY_CROSS: fill(g, 2, 5, 14, 7); fill(g, 6, 1, 10, 11); fill(g, 5, 4, 11, 8); break;
	case LY_L: fill(g, 3, 3, 13, 9); for (int j = 3; j <= 5; j++) for (int i = 10; i <= 13; i++) g.cell[j][i] = 0; break;
	case LY_PILLARS:
		fill(g, 2, 2, 14, 10);
		for (int k = 0; k < 4; k++) g.pillar[g.nPillar++] = g.cellCenter(k % 2 ? 11 : 5, k / 2 ? 8 : 4);
		break;
	case LY_DIAMOND:
		for (int j = 2; j < GH - 1; j++)
			for (int i = 0; i < GW; i++) if (abs(i - cx) + abs(j - cz) <= 7) fill(g, i, j, i, j);
		break;
	case LY_TWIN: fill(g, 2, 3, 7, 9); fill(g, 9, 3, 14, 9); fill(g, 7, 5, 9, 7); break;
	}
	if (mirror)
		for (int j = 0; j < GH; j++)
			for (int i = 0; i < GW / 2; i++) { u8 t = g.cell[j][i]; g.cell[j][i] = g.cell[j][GW - 1 - i]; g.cell[j][GW - 1 - i] = t; }
	if (mirror) for (int k = 0; k < g.nPillar; k++) g.pillar[k].x = -g.pillar[k].x;
	// Grenzen, Eingang (vorne mittig)
	g.minX = g.minZ = 1e9f; g.maxX = g.maxZ = -1e9f;
	int bestEntry = -1, entryJ = -1;
	for (int j = 0; j < GH; j++)
		for (int i = 0; i < GW; i++) {
			if (!g.cell[j][i]) continue;
			Vec3 c = g.cellCenter(i, j);
			g.minX = fminf(g.minX, c.x); g.maxX = fmaxf(g.maxX, c.x); g.minZ = fminf(g.minZ, c.z); g.maxZ = fmaxf(g.maxZ, c.z);
			if (j > entryJ || (j == entryJ && abs(i - cx) < abs(bestEntry - cx))) { entryJ = j; bestEntry = i; }
		}
	g.entry = g.cellCenter(bestEntry, entryJ) + Vec3(0, 0, -0.4f);
	// Tor-Plätze: flache Stücke der Rückwand, mindestens 3 Zellen breit
	int jmin[GW];
	for (int i = 0; i < GW; i++) { jmin[i] = -1; for (int j = 0; j < GH; j++) if (g.cell[j][i]) { jmin[i] = j; break; } }
	int cand[GW], nc = 0;
	for (int i = 1; i < GW - 1; i++)
		if (jmin[i] > 0 && jmin[i - 1] == jmin[i] && jmin[i + 1] == jmin[i]) cand[nc++] = i;
	int chosen[3], nch = 0;
	if (nc) {
		int order[3] = {cand[nc / 2], cand[0], cand[nc - 1]};
		for (int k = 0; k < 3 && nch < 3; k++) {
			bool ok = true;
			for (int q = 0; q < nch; q++) if (abs(order[k] - chosen[q]) < 4) ok = false;
			if (ok) chosen[nch++] = order[k];
		}
		for (int q = 0; q < nc && nch < 3; q++) {                    // auffüllen
			bool ok = true;
			for (int w = 0; w < nch; w++) if (abs(cand[q] - chosen[w]) < 4) ok = false;
			if (ok) chosen[nch++] = cand[q];
		}
	}
	for (int a = 0; a < nch; a++) for (int b = a + 1; b < nch; b++) if (chosen[b] < chosen[a]) { int t = chosen[a]; chosen[a] = chosen[b]; chosen[b] = t; }
	for (int k = 0; k < nch; k++) {
		Vec3 c = g.cellCenter(chosen[k], jmin[chosen[k]]);
		g.doorSlot[g.nSlots++] = Vec3(c.x, 0.075f, c.z - 0.42f);
	}
	// --- Mesh: Boden (Textur 4 m), Wände (Textur 2 m), Leuchtteile
	MeshBuilder fb, wb, gb;
	Mesh cube = Mesh::cube(Vec3(1, 1, 1), Color(0xFFFFFF));
	for (int j = 0; j < GH; j++)
		for (int i = 0; i < GW; i++) {
			float x0 = i - GW * 0.5f, z0 = j - GH * 0.5f;
			if (g.cell[j][i]) {
				float k = r.range(0.9f, 1.04f);
				Color c = Color(0xFFFFFF).mix(Color(0x000000), 1 - k);
				// dunkler am Rand (Wandschatten)
				bool nearWall = false;
				for (int dj = -1; dj <= 1; dj++) for (int di = -1; di <= 1; di++) { int ii = i + di, jj = j + dj; if (ii >= 0 && jj >= 0 && ii < GW && jj < GH && !g.cell[jj][ii]) nearWall = true; }
				if (nearWall) c = c.mix(Color(0x000000), 0.28f);
				fb.quad(Vec3(x0, 0.075f, z0), Vec3(x0, 0.075f, z0 + 1), Vec3(x0 + 1, 0.075f, z0 + 1), Vec3(x0 + 1, 0.075f, z0), c, c, c, c,
				        Vec2(x0 / 4, z0 / 4), Vec2((x0 + 1) / 4, (z0 + 1) / 4));
				continue;
			}
			bool adj = false, floorNorth = false;
			for (int dj = -1; dj <= 1; dj++)
				for (int di = -1; di <= 1; di++) {
					int ii = i + di, jj = j + dj;
					if (ii < 0 || jj < 0 || ii >= GW || jj >= GH || !g.cell[jj][ii]) continue;
					adj = true;
					if (dj < 0) floorNorth = true;
				}
			if (!adj) continue;
			float h = floorNorth ? 0.45f : (j >= cz + 1 ? 1.0f : 2.6f + r.range(0, 0.6f));
			Color wc = Color(0xFFFFFF).mix(Color(0x000000), r.range(0.0f, 0.15f));
			Vec3 c(x0 + 0.5f, 0, z0 + 0.5f);
			if (biome == 1) {
				boxFaces(wb, c + Vec3(r.range(-0.06f, 0.06f), 0, r.range(-0.06f, 0.06f)), Vec3(1.1f, h, 1.1f), wc);
				wb.add(cube, Mat4::translate(c + Vec3(0, h + 0.1f, 0)) * Mat4::rotY(r.range(0, 1.5f)) * Mat4::rotX(0.25f) * Mat4::scale(Vec3(0.9f, 0.35f, 0.9f)), wc.mix(Color(0x000000), 0.2f));
				if (h > 2 && r.f() < 0.5f && j + 1 < GH && g.cell[j + 1][i]) {
					static const u32 CRY[4] = {0x7AE8FF, 0xB08AFF, 0x5AC8FF, 0xD8A0FF};
					for (int k = 0; k < 3; k++) {
						float hh = r.range(0.4f, 1.0f);
						Vec3 cp = c + Vec3(r.range(-0.4f, 0.4f), 0.1f + hh * 0.5f, 0.6f + r.range(0, 0.08f));
						gb.add(cube, Mat4::translate(cp) * Mat4::rotZ(r.range(-0.4f, 0.4f)) * Mat4::rotY(0.78f) * Mat4::scale(Vec3(0.14f, hh, 0.14f)), Color(CRY[r.irange(0, 3)]));
					}
				}
			} else {
				boxFaces(wb, c, Vec3(1.0f, h, 1.0f), wc);
				if (h > 2) {
					boxFaces(wb, c + Vec3(0, h, 0), Vec3(1.1f, 0.14f, 1.1f), wc.mix(Color(0x000000), 0.15f));
					if (i % 4 == 2) boxFaces(wb, c + Vec3(0, 0, 0.5f), Vec3(0.34f, h, 0.18f), wc.mix(Color(0xFFFFFF), 0.1f));   // Strebepfeiler
				}
				if (h > 2 && j + 1 < GH && g.cell[j + 1][i] && (i % 3 == 0) && g.nTorch < 10) {
					Vec3 tp = c + Vec3(0, 1.9f, 0.62f);
					wb.add(cube, Mat4::translate(tp + Vec3(0, -0.15f, -0.05f)) * Mat4::scale(Vec3(0.08f, 0.3f, 0.1f)), Color(0x5A5060));
					wb.add(cube, Mat4::translate(tp) * Mat4::scale(Vec3(0.18f, 0.06f, 0.18f)), Color(0x4A4050));
					gb.add(cube, Mat4::translate(tp + Vec3(0, 0.14f, 0)) * Mat4::rotY(0.78f) * Mat4::scale(Vec3(0.12f, 0.24f, 0.12f)), Color(0xFF7A2A));
					gb.add(cube, Mat4::translate(tp + Vec3(0, 0.2f, 0)) * Mat4::rotY(0.3f) * Mat4::scale(Vec3(0.07f, 0.18f, 0.07f)), Color(0xFFE070));
					g.torch[g.nTorch++] = tp + Vec3(0, 0.25f, 0.1f);
				}
			}
		}
	if (biome == 1)                                                    // Kristall-"Fackeln": Lichtpunkte an Rückwänden
		for (int i = 2; i < GW - 2 && g.nTorch < 8; i += 4) {
			int j = jmin[i];
			if (j <= 0) continue;
			g.torch[g.nTorch++] = g.cellCenter(i, j) + Vec3(0, 1.2f, -0.3f);
		}
	fb.build(&floorMesh);
	wb.build(&wallMesh);
	gb.build(&glow);
	fb.free();
	wb.free();
	gb.free();
	cube.free();
}
