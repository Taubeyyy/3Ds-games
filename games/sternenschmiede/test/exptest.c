// Bot fliegt Expeditionen: Stabilitaet und Ausbeute pruefen
#include "../source/expedition.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static Game G;
static Exp E;
int main(void) {
	game_init_tables();
	game_new(&G);
	for (int lvl = 0; lvl <= 12; lvl += 4) {
		for (int i = 0; i < NSHIPUP; i++) G.s.ext.x.ship[i] = lvl > SHIPUP[i].max ? SHIPUP[i].max : lvl;
		for (int sec = 1; sec <= 12; sec += 3) {
			G.s.ext.x.sectorMax = 12;
			exp_start(&E, &G, sec);
			int steps = 0;
			while (!E.done && steps < 60 * 400) {
				// naechstes Ziel: Brocken > Asteroid > Tor
				float tx = E.gateX, ty = E.gateY, bd = 1e12f;
				for (int i = 0; i < MAX_CHUNK; i++) if (E.ch[i].alive && E.cargo < E.maxCargo) {
					float d = (E.ch[i].x - E.x) * (E.ch[i].x - E.x) + (E.ch[i].y - E.y) * (E.ch[i].y - E.y);
					if (d < bd) { bd = d; tx = E.ch[i].x; ty = E.ch[i].y; } }
				if (bd > 1e11f) for (int i = 0; i < MAX_AST; i++) if (E.ast[i].alive) {
					float d = (E.ast[i].x - E.x) * (E.ast[i].x - E.x) + (E.ast[i].y - E.y) * (E.ast[i].y - E.y);
					if (d < bd) { bd = d; tx = E.ast[i].x; ty = E.ast[i].y; } }
				if (E.cargo >= E.maxCargo) { tx = E.gateX; ty = E.gateY; }
				float dx = tx - E.x, dy = ty - E.y, l = sqrtf(dx * dx + dy * dy) + 1e-3f;
				float s = l > 50 ? 1 : 0.3f;
				exp_step(&E, &G, dx / l * s, dy / l * s, false, 1 / 60.0f);
				steps++;
			}
			double en, al;
			float cargo = E.cargo;
			int endSec = E.sector;
			bool dead = E.destroyed;
			exp_finish(&E, &G, &en, &al);
			printf("Upg %2d Sektor %2d->%2d: %4.0fs Fracht %3.0f/%3.0f %s Legierung %6.0f Art %d\n", lvl, sec, endSec, steps / 60.0,
				cargo, E.maxCargo, dead ? "ZERSTÖRT" : "ok      ", al, E.nArt);
		}
	}
	return 0;
}
