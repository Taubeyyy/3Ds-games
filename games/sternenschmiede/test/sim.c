// Balancing-Simulation: spielt einen halbaktiven Spieler durch.
#include "../source/game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void buy_best(Game *g) {
	for (int guard = 0; guard < 2000; guard++) {
		upg_buy_all(g);
		int best = -1;
		double bv = 1e300;
		for (int i = 0; i < NGEN; i++) {
			if (!gen_visible(g, i) && !(i > 0 && g->s.gens[i - 1] > 0)) continue;
			if (gen_locked(g, i)) continue;
			double c = gen_cost(g, i, 1);
			double d = GEN[i].prod * g->genMult[i] * g->globalMult + 1e-12;
			double v = c / d;
			if (v < bv) { bv = v; best = i; }
		}
		if (best < 0 || gen_cost(g, best, 1) > g->s.energy) break;
		gen_buy(g, best, 1);
	}
}

static void buy_perks(Game *g) {
	for (;;) {
		int best = -1; double bc = 1e300;
		for (int i = 0; i < NPERK; i++) {
			if (!perk_available(g, i) || g->s.perk[i] >= PERK[i].max) continue;
			double c = perk_cost(g, i);
			if (i == PK_LIGHT || i == PK_AUTOGEN) c *= 0.5; // Prioritaet
			if (c < bc) { bc = c; best = i; }
		}
		if (best < 0 || !perk_buy(g, best)) break;
	}
	for (;;) {
		int best = -1; double bc = 1e300;
		for (int i = 0; i < NSPERK; i++) {
			if (g->s.sperk[i] >= SPERK[i].max) continue;
			double c = sperk_cost(g, i);
			if (c < bc) { bc = c; best = i; }
		}
		if (best < 0 || !sperk_buy(g, best)) break;
	}
}

int main(int argc, char **argv) {
	double hours = argc > 1 ? atof(argv[1]) : 12;
	double cps = argc > 2 ? atof(argv[2]) : 2.5; // Klicks/s beim aktiven Spielen
	game_init_tables();
	static Game G;
	Game *g = &G;
	game_new(g);
	game_seed(12345);
	double dt = 0.5, t = 0, nextRep = 0, firstNova = -1, firstBang = -1;
	int nextChal = 0;
	while (t < hours * 3600) {
		// aktiv: erste Minute jeder 10 Minuten wird geklickt
		bool active = fmod(t, 600) < 120;
		if (active) for (int k = 0; k < (int)(cps * dt); k++) click(g, true);
		game_update(g, dt);
		GameEvent e;
		while (game_pop(g, &e)) if (e.type == EV_METEOR && active) { double x; meteor_collect(g, &x); }
		buy_best(g);
		// Supernova-Strategie
		double ng = nova_gain(g);
		if (ng >= fmax(3, g->s.dustTotal * 0.8) && g->s.runTime > 120) {
			if (firstNova < 0) firstNova = t;
			nova_do(g);
			buy_perks(g);
		}
		if (bang_ready(g) && bang_gain(g) >= fmax(10, g->s.singTotal * 0.5)) {
			if (firstBang < 0) firstBang = t;
			bang_do(g);
			buy_perks(g);
		}
		if (g->s.bangs > 0 && g->s.chalActive < 0 && nextChal < NCHAL && g->s.runEarned > CHAL[nextChal].goal) {
			chal_start(g, nextChal);
			printf("  [%5.2fh] Starte Pruefung %s\n", t / 3600, CHAL[nextChal].name);
			nextChal++;
		}
		if (g->s.chalActive >= 0 && g->s.chalTime > 7200) { printf("  [%5.2fh] Abbruch %s\n", t/3600, CHAL[g->s.chalActive].name); chal_abort(g); }
		while (game_pop(g, &e)) {
			if (e.type == EV_CHAL_DONE) printf("  [%5.2fh] Pruefung geschafft: %s\n", t / 3600, CHAL[e.id].name);
			if (e.type == EV_CHAL_FAIL) printf("  [%5.2fh] Pruefung verloren: %s\n", t / 3600, CHAL[e.id].name);
		}
		if (t >= nextRep) {
			int hi = 0;
			for (int i = 0; i < NGEN; i++) if (g->s.gens[i] > 0) hi = i;
			char a[24], b[24], c[24];
			fmt_num(g->eps, a, 24); fmt_num(g->s.dustTotal, b, 24); fmt_num(g->s.bangEarned, c, 24);
			printf("%5.2fh L%d/%d/%d/%d eps=%-9s gen=%2d upg=%3d ach=%3d nova=%3.0f dust=%-9s bang=%2.0f sing=%3.0f bE=%s\n",
				t / 3600, g->s.perk[PK_LIGHT], g->s.perk[PK_START], g->s.perk[PK_CLICK], g->s.perk[PK_NETWORK], a, hi, g->upgCount, ach_count(g), g->s.novas, b, g->s.bangs, g->s.singTotal, c);
			nextRep += argc > 3 ? atof(argv[3]) * 60 : (hours > 13 ? 3600 : 1800);
		}
		if (argc > 5 && t >= atof(argv[4]) * 3600) {
			g->s.lastSave = 0;
			FILE *f = fopen(argv[5], "wb");
			fwrite(&g->s, 1, sizeof g->s, f);
			fclose(f);
			return 0;
		}
		t += dt;
	}
	printf("Erste Supernova: %.1f min, Erster Urknall: %.2f h\n", firstNova / 60, firstBang / 3600);
	return 0;
}
