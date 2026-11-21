// Bot spielt Glibberalarm: prueft Balance und Stabilitaet
#include "../source/td.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TD G;
static float samples[2000][2];
static int nsamp;

static void sample_path(TD *g) {
	nsamp = 0;
	for (float d = 0; d < g->pathLen && nsamp < 2000; d += 4) {
		td_path_pos(g, d, &samples[nsamp][0], &samples[nsamp][1]);
		nsamp++;
	}
}

static int best_spot(TD *g, int type, float *bx, float *by) {
	TStats s;
	Tower tmp = {0};
	tmp.type = type;
	td_stats(g, &tmp, &s);
	float r = s.range > 300 ? 60 : s.range;
	int best = -1;
	for (int y = 10; y < FH - 8; y += 8)
		for (int x = 10; x < FW - 8; x += 8) {
			if (!td_can_place(g, type, x, y)) continue;
			int sc = 0;
			for (int k = 0; k < nsamp; k++) {
				float dx = samples[k][0] - x, dy = samples[k][1] - y;
				if (dx * dx + dy * dy <= r * r) sc += 1 + (k < nsamp / 2);
			}
			if (type == T_DACHS) {
				sc = 0;
				for (int i = 0; i < MAX_TW; i++)
					if (g->tw[i].alive && (g->tw[i].x - x) * (g->tw[i].x - x) + (g->tw[i].y - y) * (g->tw[i].y - y) < r * r) sc += 10;
			}
			if (type == T_BIENE) sc = 1000 - sc;
			if (sc > best) { best = sc; *bx = x; *by = y; }
		}
	return best;
}

static const int ORDER[] = {T_IGEL, T_IGEL, T_SCHNECKE, T_KRABBE, T_SEEIGEL, T_EULE, T_KAENGURU, T_AAL, T_BIENE, T_PINGUIN, T_DACHS, T_KRABBE, T_AAL, T_IGEL};
static const int MAINP[NTOWER] = {1, 0, 0, 0, 1, 0, 0, 0, 0, 0};

static int count(TD *g, int type) {
	int n = 0;
	for (int i = 0; i < MAX_TW; i++) n += g->tw[i].alive && g->tw[i].type == type;
	return n;
}

static void bot(TD *g, int *ord) {
	for (int guard = 0; guard < 50; guard++) {
		// Geister-Sicht sicherstellen
		if (g->wave >= 15) {
			int e = -1;
			for (int i = 0; i < MAX_TW; i++) if (g->tw[i].alive && g->tw[i].type == T_EULE) e = i;
			if (e < 0) {
				float x, y;
				if (best_spot(g, T_EULE, &x, &y) >= 0 && td_place(g, T_EULE, x, y) >= 0) continue;
				if (g->wave >= 18) return;
			} else if (g->tw[e].a < 2) { if (td_upgrade(g, e, 0)) continue; if (g->wave >= 18) return; }
		}
		int target = 3 + g->wave / 3;
		if (target > 34) target = 34;
		int towers = 0;
		for (int i = 0; i < MAX_TW; i++) towers += g->tw[i].alive;
		// Upgrade-Kandidat
		int bt = -1, bp = 0, bc = 1 << 30;
		for (int i = 0; i < MAX_TW; i++) {
			Tower *t = &g->tw[i];
			if (!t->alive) continue;
			for (int p = 0; p < 2; p++) {
				if (!td_can_upgrade(g, i, p)) continue;
				int c = td_up_cost(g, i, p);
				int tier = p ? t->b : t->a;
				if (p != MAINP[t->type] && tier >= 2) continue;
				if (c < bc) { bc = c; bt = i; bp = p; }
			}
		}
		bool wantTower = towers < target || bt < 0;
		if (wantTower) {
			int type = ORDER[*ord % (int)(sizeof ORDER / sizeof ORDER[0])];
			if (type == T_BIENE && count(g, T_BIENE) >= 3) { (*ord)++; continue; }
			if (g->coins < td_cost(g, type)) return;
			float x, y;
			if (best_spot(g, type, &x, &y) < 0) { (*ord)++; return; }
			if (td_place(g, type, x, y) >= 0) { (*ord)++; continue; }
			return;
		}
		if (g->coins < bc) return;
		td_upgrade(g, bt, bp);
	}
}

int main(int argc, char **argv) {
	td_init_tables();
	srand(1);
	int only = argc > 1 ? atoi(argv[1]) : -1;
	printf("Wellen-RBE: w10=%d w20=%d w30=%d w40=%d w50=%d w60=%d w80=%d\n", td_wave_rbe(10), td_wave_rbe(20), td_wave_rbe(30),
		td_wave_rbe(40), td_wave_rbe(50), td_wave_rbe(60), td_wave_rbe(80));
	for (int m = 0; m < NMAP; m++)
		for (int d = 0; d < NDIFF; d++) {
			if (only >= 0 && only != m) continue;
			TD *g = &G;
			td_new(g, m, d);
			sample_path(g);
			int ord = 0;
			long steps = 0;
			int maxEn = 0, maxPr = 0;
			while (!g->lost && !g->won && g->wave < 200) {
				bot(g, &ord);
				if (!g->waveActive) {
					if (getenv("VERB") && g->wave > 0) {
						int tw = 0; for (int i = 0; i < MAX_TW; i++) tw += g->tw[i].alive;
						printf("  w%2d leben %3d coins %5.0f tuerme %d\n", g->wave, g->lives, g->coins, tw);
					}
					td_start_wave(g);
				}
				td_step(g);
				td_collect_jar(g, 200, 120, 1000);
				steps++;
				int ne = td_enemies_alive(g), np = 0;
				for (int i = 0; i < MAX_PR; i++) np += g->pr[i].alive;
				if (ne > maxEn) maxEn = ne;
				if (np > maxPr) maxPr = np;
			}
			int towers = 0;
			for (int i = 0; i < MAX_TW; i++) towers += g->tw[i].alive;
			printf("%-14s %-6s: %s Welle %2d, Leben %3d, Türme %2d, Zeit %4.0f min, maxGegner %3d maxProj %3d\n", MAPS[m].name, DIFF[d].name,
				g->won ? "GEWONNEN" : "verloren", g->wave, g->lives, towers, steps / 60.0 / 60, maxEn, maxPr);
		}
	return 0;
}
