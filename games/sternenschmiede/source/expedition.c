// Star Forge - Expedition (Action-Modus)
#include "expedition.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

const ShipUpDef SHIPUP[NSHIPUP] = {
	[SU_LASER] = {"Laser Power", "Laserschaden", 10, 1.45, 25},
	[SU_RATE] = {"Fire Rate", "Schussfrequenz", 15, 1.5, 15},
	[SU_RANGE] = {"Laser Range", "Laser-Reichweite", 12, 1.5, 10},
	[SU_THRUST] = {"Thrusters", "Geschwindigkeit", 10, 1.45, 12},
	[SU_HULL] = {"Hull Plating", "Rumpfstärke", 12, 1.45, 20},
	[SU_FUEL] = {"Fuel Tank", "Flugzeit", 10, 1.45, 15},
	[SU_CARGO] = {"Cargo Bay", "Laderaum", 10, 1.45, 20},
	[SU_MAGNET] = {"Tractor Beam", "Sammelradius", 8, 1.5, 10},
	[SU_TWIN] = {"Twin Lasers", "Zusätzliche Laserstrahlen", 150, 4, 3},
	[SU_SHIELD] = {"Nano Repair", "Rumpf repariert sich", 60, 2, 10},
};

static uint32_t rs;
static float rnd(void) {
	rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
	return (rs & 0xFFFFFF) / (float)0x1000000;
}
static float d2(float ax, float ay, float bx, float by) { float dx = ax - bx, dy = ay - by; return dx * dx + dy * dy; }

static void xfx(Exp *e, int kind, float x, float y, float x2, float y2, float max, float r, uint32_t col) {
	for (int i = 0; i < MAX_EFX; i++)
		if (!e->fx[i].alive) {
			e->fx[i] = (XFx){1, (uint8_t)kind, x, y, x2, y2, 0, max, r, col};
			return;
		}
}

double ship_up_cost(Game *g, int i) { return ceil(SHIPUP[i].cost * pow(SHIPUP[i].growth, g->s.ext.x.ship[i])); }
bool ship_up_buy(Game *g, int i) {
	uint8_t *l = &g->s.ext.x.ship[i];
	if (*l >= SHIPUP[i].max) return false;
	double c = ship_up_cost(g, i);
	if (g->s.ext.x.alloy < c) return false;
	g->s.ext.x.alloy -= c;
	(*l)++;
	return true;
}

static void calc_stats(Exp *e, Game *g) {
	uint8_t *s = g->s.ext.x.ship;
	e->dmg = 10 * powf(1.32f, s[SU_LASER]) * (1 + art_bonus(g, AK_LASER));
	e->rate = 0.4f * powf(0.92f, s[SU_RATE]);
	e->range = 70 + 7 * s[SU_RANGE];
	e->speed = 110 + 12 * s[SU_THRUST];
	e->maxHull = 100 * powf(1.25f, s[SU_HULL]);
	e->maxFuel = 60 + 10 * s[SU_FUEL];
	e->maxCargo = floorf(40 * powf(1.22f, s[SU_CARGO]) * (1 + art_bonus(g, AK_CARGO)));
	e->magnet = 28 + 9 * s[SU_MAGNET];
	e->beams = 1 + s[SU_TWIN];
	e->regen = 0.8f * s[SU_SHIELD];
}

void ship_up_desc(Game *g, int i, char *b, int n) {
	static Exp tmp; // gross -> nicht auf den Stack
	calc_stats(&tmp, g);
	switch (i) {
	case SU_LASER: snprintf(b, n, "Schaden %.0f", tmp.dmg); break;
	case SU_RATE: snprintf(b, n, "%.1f Schüsse/s", 1 / tmp.rate); break;
	case SU_RANGE: snprintf(b, n, "Reichweite %.0f", tmp.range); break;
	case SU_THRUST: snprintf(b, n, "Tempo %.0f", tmp.speed); break;
	case SU_HULL: snprintf(b, n, "Rumpf %.0f", tmp.maxHull); break;
	case SU_FUEL: snprintf(b, n, "Flugzeit %.0f s", tmp.maxFuel); break;
	case SU_CARGO: snprintf(b, n, "Laderaum %.0f", tmp.maxCargo); break;
	case SU_MAGNET: snprintf(b, n, "Sammelradius %.0f", tmp.magnet); break;
	case SU_TWIN: snprintf(b, n, "%d Laserstrahl%s", tmp.beams, tmp.beams > 1 ? "en" : ""); break;
	case SU_SHIELD: snprintf(b, n, "Repariert %.1f/s", tmp.regen); break;
	}
}

static int pick_ore(int s) {
	float w[NORE];
	w[ORE_FERRITE] = fmaxf(1, 10 - s * 1.2f);
	w[ORE_COBALT] = 3 + s;
	w[ORE_LUMINITE] = s >= 2 ? s * 1.5f : 0;
	w[ORE_VOIDSTONE] = s >= 4 ? (s - 3) * 1.4f : 0;
	w[ORE_STARHEART] = s >= 6 ? (s - 5) * 0.9f : 0;
	float sum = 0;
	for (int i = 0; i < NORE; i++) sum += w[i];
	float r = rnd() * sum;
	for (int i = 0; i < NORE; i++) {
		if (r < w[i]) return i;
		r -= w[i];
	}
	return 0;
}

static int add_ast(Exp *e, float x, float y, float r, int ore) {
	for (int i = 0; i < MAX_AST; i++)
		if (!e->ast[i].alive) {
			Asteroid *a = &e->ast[i];
			memset(a, 0, sizeof *a);
			a->alive = 1; a->x = x; a->y = y; a->r = r; a->ore = ore;
			a->maxHp = a->hp = (r / 10) * (r / 10) * 14 * powf(1.55f, e->sector - 1) * (1 + ore * 0.35f);
			a->rot = rnd() * 6.28f;
			a->rotV = (rnd() - 0.5f) * 0.8f;
			for (int k = 0; k < 10; k++) a->shape[k] = 0.78f + rnd() * 0.32f;
			return i;
		}
	return -1;
}

static void gen_world(Exp *e) {
	memset(e->ast, 0, sizeof e->ast);
	memset(e->ch, 0, sizeof e->ch);
	memset(e->dr, 0, sizeof e->dr);
	memset(e->mn, 0, sizeof e->mn);
	memset(e->bl, 0, sizeof e->bl);
	int s = e->sector;
	e->x = WORLD / 2; e->y = WORLD - 80; e->vx = e->vy = 0; e->ang = -1.5708f;
	e->gateX = 150 + rnd() * (WORLD - 300);
	e->gateY = 90 + rnd() * 150;
	int n = 70 + 7 * s;
	if (n > MAX_AST - 30) n = MAX_AST - 30;
	for (int k = 0, tries = 0; k < n && tries < 4000; tries++) {
		float x = 40 + rnd() * (WORLD - 80), y = 40 + rnd() * (WORLD - 80);
		float r = 9 + rnd() * rnd() * 22;
		if (d2(x, y, e->x, e->y) < 120 * 120 || d2(x, y, e->gateX, e->gateY) < 70 * 70) continue;
		bool ok = true;
		for (int i = 0; i < MAX_AST && ok; i++)
			if (e->ast[i].alive && d2(x, y, e->ast[i].x, e->ast[i].y) < (r + e->ast[i].r + 14) * (r + e->ast[i].r + 14)) ok = false;
		if (!ok) continue;
		add_ast(e, x, y, r, pick_ore(s));
		k++;
	}
	// Relikt-Asteroid
	for (int tries = 0; tries < 200; tries++) {
		float x = 100 + rnd() * (WORLD - 200), y = 100 + rnd() * (WORLD - 400);
		bool ok = true;
		for (int i = 0; i < MAX_AST && ok; i++)
			if (e->ast[i].alive && d2(x, y, e->ast[i].x, e->ast[i].y) < (40 + e->ast[i].r) * (40 + e->ast[i].r)) ok = false;
		if (!ok) continue;
		int i = add_ast(e, x, y, 22, pick_ore(s));
		if (i >= 0) { e->ast[i].relic = 1; e->ast[i].hp = e->ast[i].maxHp *= 4; }
		break;
	}
	if (s >= 2)
		for (int k = 0; k < 4 + 2 * s && k < MAX_MINE; k++) {
			float x = 60 + rnd() * (WORLD - 120), y = 60 + rnd() * (WORLD - 200);
			if (d2(x, y, e->x, e->y) < 200 * 200) continue;
			e->mn[k] = (Mine){1, x, y, rnd() * 6};
		}
	if (s >= 3)
		for (int k = 0; k < 2 + s * 3 / 5 && k < MAX_DRONE; k++) {
			float x = 60 + rnd() * (WORLD - 120), y = 60 + rnd() * (WORLD - 300);
			Drone *d = &e->dr[k];
			d->alive = 1; d->x = x; d->y = y; d->maxHp = d->hp = 30 * powf(1.6f, s - 1); d->cd = 1 + rnd();
		}
}

void exp_start(Exp *e, Game *g, int sector) {
	memset(e, 0, sizeof *e);
	rs = (uint32_t)(g->s.playTime * 1000) * 2654435761u + sector * 97 + 1;
	if (!rs) rs = 1;
	e->sector = e->startSector = sector;
	e->lock = -1;
	calc_stats(e, g);
	e->hull = e->maxHull;
	e->fuel = e->maxFuel;
	gen_world(e);
}

static void spawn_chunk(Exp *e, float x, float y, int ore, int art) {
	for (int i = 0; i < MAX_CHUNK; i++)
		if (!e->ch[i].alive) {
			float a = rnd() * 6.28f, v = 30 + rnd() * 50;
			e->ch[i] = (Chunk){1, (uint8_t)ore, (int8_t)art, x, y, cosf(a) * v, sinf(a) * v, 0};
			return;
		}
}

static int unowned_artifact(Game *g, Exp *e) {
	int cand[NART], n = 0;
	for (int i = 0; i < NART; i++) {
		if (art_has(g, i)) continue;
		bool found = false;
		for (int k = 0; k < e->nArt; k++) if (e->artFound[k] == i) found = true;
		for (int k = 0; k < MAX_CHUNK; k++) if (e->ch[k].alive && e->ch[k].art == i) found = true;
		if (!found) cand[n++] = i;
	}
	return n ? cand[(int)(rnd() * n) % n] : -1;
}

static void hurt(Exp *e, float dmg) {
	e->hull -= dmg;
	e->hitFlash = 0.3f;
	e->evHurt++;
}

void exp_step(Exp *e, Game *g, float ix, float iy, bool boost, float dt) {
	if (e->done || dt <= 0) return;
	e->time += dt;
	for (int i = 0; i < MAX_EFX; i++)
		if (e->fx[i].alive && (e->fx[i].t += dt) >= e->fx[i].max) e->fx[i].alive = 0;
	if (e->hitFlash > 0) e->hitFlash -= dt;
	// Steuerung
	float il = sqrtf(ix * ix + iy * iy);
	if (il < 0.2f && e->autopilot) {
		float dx = e->apX - e->x, dy = e->apY - e->y, l = sqrtf(dx * dx + dy * dy);
		if (l < 18) e->autopilot = false;
		else { ix = dx / l; iy = dy / l; il = 1; }
	} else if (il >= 0.2f) e->autopilot = false;
	e->boosting = boost && il >= 0.2f && e->fuel > 1;
	float maxV = e->speed * (e->boosting ? 1.8f : 1);
	e->thrust = 0;
	if (il >= 0.2f) {
		if (il > 1) { ix /= il; iy /= il; il = 1; }
		float ta = atan2f(iy, ix);
		float da = remainderf(ta - e->ang, 6.2832f);
		e->ang += da * fminf(1, dt * 10);
		e->vx += ix * maxV * 3 * dt;
		e->vy += iy * maxV * 3 * dt;
		e->thrust = il;
	}
	float v = sqrtf(e->vx * e->vx + e->vy * e->vy);
	if (v > maxV) { e->vx *= maxV / v; e->vy *= maxV / v; v = maxV; }
	float damp = powf(0.25f, dt);
	if (il < 0.2f) { e->vx *= damp; e->vy *= damp; }
	e->x += e->vx * dt;
	e->y += e->vy * dt;
	if (e->x < 20) { e->x = 20; e->vx = fabsf(e->vx) * 0.5f; }
	if (e->y < 20) { e->y = 20; e->vy = fabsf(e->vy) * 0.5f; }
	if (e->x > WORLD - 20) { e->x = WORLD - 20; e->vx = -fabsf(e->vx) * 0.5f; }
	if (e->y > WORLD - 20) { e->y = WORLD - 20; e->vy = -fabsf(e->vy) * 0.5f; }
	e->fuel -= dt * (e->boosting ? 3 : 1);
	e->hull = fminf(e->maxHull, e->hull + e->regen * dt);

	// Asteroiden: Drehung & Kollision
	for (int i = 0; i < MAX_AST; i++) {
		Asteroid *a = &e->ast[i];
		if (!a->alive) continue;
		a->rot += a->rotV * dt;
		if (a->hitT > 0) a->hitT -= dt;
		float rr = a->r + 7;
		float dd = d2(e->x, e->y, a->x, a->y);
		if (dd < rr * rr && dd > 0.01f) {
			float l = sqrtf(dd), nx = (e->x - a->x) / l, ny = (e->y - a->y) / l;
			e->x = a->x + nx * rr;
			e->y = a->y + ny * rr;
			float vn = e->vx * nx + e->vy * ny;
			if (vn < 0) {
				if (-vn > 70) { hurt(e, (-vn - 70) * 0.15f * powf(1.2f, e->sector - 1)); xfx(e, XF_SPARK, e->x - nx * 7, e->y - ny * 7, 0, 0, 0.3f, 6, 0xFFD080); }
				e->vx -= 1.6f * vn * nx;
				e->vy -= 1.6f * vn * ny;
			}
		}
	}
	// Minen
	for (int i = 0; i < MAX_MINE; i++) {
		Mine *m = &e->mn[i];
		if (!m->alive) continue;
		m->t += dt;
		if (d2(e->x, e->y, m->x, m->y) < 14 * 14) {
			m->alive = 0;
			hurt(e, 20 * powf(1.25f, e->sector - 1));
			xfx(e, XF_BOOM, m->x, m->y, 0, 0, 0.5f, 34, 0xFF6A3A);
			e->evBoom++;
		}
	}
	// Drohnen
	for (int i = 0; i < MAX_DRONE; i++) {
		Drone *d = &e->dr[i];
		if (!d->alive) continue;
		if (d->hitT > 0) d->hitT -= dt;
		float dx = e->x - d->x, dy = e->y - d->y, l = sqrtf(dx * dx + dy * dy) + 0.01f;
		if (l < 240) {
			d->ang = atan2f(dy, dx);
			float want = l > 70 ? 1 : -0.6f;
			float sp = 70 + 5 * e->sector;
			d->vx += (dx / l * want * sp - d->vx) * fminf(1, dt * 2);
			d->vy += (dy / l * want * sp - d->vy) * fminf(1, dt * 2);
			d->cd -= dt;
			if (d->cd <= 0) {
				d->cd = 1.6f;
				for (int b = 0; b < MAX_BUL; b++)
					if (!e->bl[b].alive) { e->bl[b] = (Bullet){1, d->x, d->y, dx / l * 170, dy / l * 170, 0}; break; }
			}
		} else { d->vx *= 0.98f; d->vy *= 0.98f; }
		d->x += d->vx * dt;
		d->y += d->vy * dt;
	}
	for (int i = 0; i < MAX_BUL; i++) {
		Bullet *b = &e->bl[i];
		if (!b->alive) continue;
		b->t += dt;
		b->x += b->vx * dt;
		b->y += b->vy * dt;
		if (b->t > 2.5f) { b->alive = 0; continue; }
		if (d2(b->x, b->y, e->x, e->y) < 9 * 9) {
			b->alive = 0;
			hurt(e, 6 * powf(1.25f, e->sector - 1));
			xfx(e, XF_SPARK, b->x, b->y, 0, 0, 0.25f, 5, 0xFF5A6E);
		}
	}
	// Laser: Drohnen zuerst, dann naechste Asteroiden
	e->laserCd -= dt;
	if (e->laserCd <= 0) {
		int used[8], nu = 0;
		float R2 = e->range * e->range;
		for (int b = 0; b < e->beams; b++) {
			int best = -1, kind = 0;
			float bd = R2;
			for (int i = 0; i < MAX_DRONE; i++) {
				if (!e->dr[i].alive) continue;
				bool u = false;
				for (int k = 0; k < nu; k++) if (used[k] == 1000 + i) u = true;
				float dd = d2(e->x, e->y, e->dr[i].x, e->dr[i].y);
				if (!u && dd < bd) { bd = dd; best = i; kind = 1; }
			}
			// am bisherigen Ziel bleiben
			if (best < 0 && b == 0 && e->lock >= 0 && e->lock < MAX_AST && e->ast[e->lock].alive) {
				Asteroid *a = &e->ast[e->lock];
				if (d2(e->x, e->y, a->x, a->y) - a->r * a->r < R2) { best = e->lock; kind = 0; }
			}
			if (best < 0) {
				for (int i = 0; i < MAX_AST; i++) {
					Asteroid *a = &e->ast[i];
					if (!a->alive) continue;
					bool u = false;
					for (int k = 0; k < nu; k++) if (used[k] == i) u = true;
					float dd = d2(e->x, e->y, a->x, a->y) - a->r * a->r;
					if (!u && dd < bd) { bd = dd; best = i; kind = 0; }
				}
			}
			if (best < 0) break;
			used[nu++] = kind ? 1000 + best : best;
			if (b == 0 && !kind) e->lock = best;
			e->evShoot++;
			if (kind) {
				Drone *d = &e->dr[best];
				d->hp -= e->dmg;
				d->hitT = 0.1f;
				xfx(e, XF_BEAM, e->x, e->y, d->x, d->y, 0.12f, 2, 0xFF6A8A);
				if (d->hp <= 0) {
					d->alive = 0;
					xfx(e, XF_BOOM, d->x, d->y, 0, 0, 0.5f, 26, 0xFF8A3A);
					e->evBoom++;
					int ore = e->sector >= 6 ? ORE_VOIDSTONE : e->sector >= 3 ? ORE_LUMINITE : ORE_COBALT;
					spawn_chunk(e, d->x, d->y, ore, -1);
					spawn_chunk(e, d->x, d->y, ore, -1);
				}
			} else {
				Asteroid *a = &e->ast[best];
				a->hp -= e->dmg;
				a->hitT = 0.1f;
				float ang = atan2f(e->y - a->y, e->x - a->x);
				float hx = a->x + cosf(ang) * a->r * 0.8f, hy = a->y + sinf(ang) * a->r * 0.8f;
				xfx(e, XF_BEAM, e->x, e->y, hx, hy, 0.12f, 2, ORE[a->ore].color);
				xfx(e, XF_SPARK, hx, hy, 0, 0, 0.2f, 4, ORE[a->ore].color);
				e->evHit++;
				if (a->hp <= 0) {
					Asteroid old = *a;
					a->alive = 0;
					int n = 1 + (int)(old.r / 7);
					for (int k = 0; k < n; k++) spawn_chunk(e, old.x, old.y, old.ore, -1);
					if (old.relic || rnd() < 0.003f) {
						int art = unowned_artifact(g, e);
						if (art >= 0) spawn_chunk(e, old.x, old.y, old.ore, art);
					}
					if (old.r > 18) {
						for (int k = 0; k < 2; k++) {
							float an = rnd() * 6.28f;
							add_ast(e, old.x + cosf(an) * old.r * 0.5f, old.y + sinf(an) * old.r * 0.5f, old.r * 0.5f, old.ore);
						}
					}
					xfx(e, XF_BOOM, old.x, old.y, 0, 0, 0.45f, old.r * 1.3f, 0xC8B8A0);
					e->evBreak++;
				}
			}
		}
		e->laserCd = e->rate;
	}
	// Erzbrocken einsammeln
	float M2 = e->magnet * e->magnet;
	bool full = e->cargo >= e->maxCargo;
	for (int i = 0; i < MAX_CHUNK; i++) {
		Chunk *c = &e->ch[i];
		if (!c->alive) continue;
		c->t += dt;
		float dd = d2(c->x, c->y, e->x, e->y);
		bool canTake = c->art >= 0 || !full;
		if (dd < M2 && canTake && c->t > 0.3f) {
			float l = sqrtf(dd) + 0.01f;
			float pull = 260 * (1 - l / e->magnet) + 60;
			c->vx += (e->x - c->x) / l * pull * dt * 6;
			c->vy += (e->y - c->y) / l * pull * dt * 6;
		}
		c->vx *= powf(0.3f, dt);
		c->vy *= powf(0.3f, dt);
		c->x += c->vx * dt;
		c->y += c->vy * dt;
		if (dd < 11 * 11 && c->t > 0.3f) {
			if (c->art >= 0) {
				if (e->nArt < 6) e->artFound[e->nArt++] = c->art;
				c->alive = 0;
				e->evArt++;
				xfx(e, XF_RING, e->x, e->y, 0, 0, 0.8f, 40, 0xFFD25A);
			} else if (!full) {
				e->ore[c->ore] += 1;
				e->cargo += 1;
				full = e->cargo >= e->maxCargo;
				c->alive = 0;
				e->evPickup++;
				xfx(e, XF_PICK, e->x, e->y, 0, 0, 0.25f, 8, ORE[c->ore].color);
			}
		}
	}
	if (full) e->evFull++;
	// Sprungtor
	if (d2(e->x, e->y, e->gateX, e->gateY) < 26 * 26) {
		if (e->sector < MAX_SECTOR) {
			e->sector++;
			if (e->sector > g->s.ext.x.sectorMax) g->s.ext.x.sectorMax = e->sector;
			gen_world(e);
			e->fuel = fminf(e->maxFuel, e->fuel + e->maxFuel * 0.25f);
			e->evGate++;
		}
	}
	if (e->hull <= 0) { e->hull = 0; e->destroyed = true; e->done = true; }
	if (e->fuel <= 0) { e->fuel = 0; e->done = true; }
}

void exp_finish(Exp *e, Game *g, double *energy, double *alloy) {
	double en = 0, al = 0;
	float keep = e->destroyed ? 0.5f : 1;
	double base = fmax(g->epsBase, 3) * (1 + 0.15 * (e->sector - 1));
	for (int i = 0; i < NORE; i++) {
		double n = floor(e->ore[i] * keep);
		e->ore[i] = n;
		en += n * ORE[i].energySec * base;
		al += n * ORE[i].alloy;
		g->s.ext.x.oreTotal[i] += n;
	}
	g->s.ext.x.oreValueTotal += al;
	g->s.ext.x.alloy += al;
	g->s.ext.x.expeditions += 1;
	for (int k = 0; k < e->nArt; k++) g->s.ext.x.artifacts |= 1u << e->artFound[k];
	if (e->sector > g->s.ext.x.sectorMax) g->s.ext.x.sectorMax = e->sector;
	g->s.energy += en;
	g->s.runEarned += en;
	g->s.lifeEarned += en;
	g->s.bangEarned += en;
	game_recalc(g);
	*energy = en;
	*alloy = al;
}
