"""SKYBREAKER – Welt-Modelle für Titelbild und Pantheon (Insel, Himmelsturm, Splitter, Sockel)."""
import math, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "tools"))
from lowpoly import *

PI = math.pi
GRASS, GRASS2, ROCK, ROCK_D = rgb(0x5E9A4A), rgb(0x86B85A), rgb(0x8A7E8E), rgb(0x3A3448)
STONE, STONE_D, VIOLET, GOLD = rgb(0xC8C0D8), rgb(0x7A7098), rgb(0x5A4A8A), rgb(0xE8C060)
CRYSTAL, WARM = rgb(0x9AF4FF), rgb(0xFFD49A)


def island(r=2.6, depth=3.2, seg=11, seed=3):
    """schwebende Insel: Grasdecke oben, Felskegel nach unten"""
    rnd = __import__("random").Random(seed)
    radii = [r * rnd.uniform(0.85, 1.1) for _ in range(seg)]
    m = Mesh()
    def P(k, rr, y): a = 2 * PI * k / seg; return (math.sin(a) * radii[k % seg] * rr, y, math.cos(a) * radii[k % seg] * rr)
    # Gras oben (Fächer) + Grasrand
    hin = [rnd.uniform(0.0, 0.08) for _ in range(seg)]
    def Q(k): p = P(k, 0.55, 0); return (p[0], hin[k % seg], p[2])
    for k in range(seg):
        m.tri((0, 0.06, 0), Q(k), Q(k + 1), GRASS if k % 2 else GRASS2)
        m.quad(Q(k), P(k, 1.0, 0), P(k + 1, 1.0, 0), Q(k + 1), GRASS2 if k % 2 else GRASS)
        m.quad(P(k + 1, 1.0, 0), P(k, 1.0, 0), P(k, 1.02, -0.18), P(k + 1, 1.02, -0.18), shade(GRASS, 0.8))
    rings = [(1.02, -0.18), (0.9, -0.7), (0.62, -1.5), (0.32, -2.4), (0.0, -depth)]
    for j in range(len(rings) - 1):
        (r0, y0), (r1, y1) = rings[j], rings[j + 1]
        col = mix(ROCK, ROCK_D, j / (len(rings) - 1))
        for k in range(seg):
            a, b = P(k, r0, y0), P(k + 1, r0, y0)
            c, d = P(k + 1, r1, y1), P(k, r1, y1)
            if r1 == 0:
                m.tri(b, a, (0, y1, 0), col)
            else:
                m.quad(b, a, d, c, col)
    m = outward(m, (0, -1.0, 0)).jitter(0.09, seed)
    return m.vary(0.08, seed)


def tuft(seed):
    m = Mesh()
    rnd = __import__("random").Random(seed)
    for i in range(5):
        a = rnd.uniform(0, 2 * PI)
        m += slab([(-0.03, 0), (0.03, 0), (0.0, rnd.uniform(0.12, 0.22))], 0.01, GRASS2).rz(rnd.uniform(-0.4, 0.4)).ry(a)
    return m


def ruin_column(h=1.4, broken=True, seed=1):
    m = prism(0.24, 0.24, 0.12, 8, STONE_D)
    m += lathe([(0.17, 0.12), (0.155, h * 0.5), (0.16, h)], 8, STONE)
    if broken:
        m = cut_top(m, h * 0.75, seed)
    else:
        m += prism(0.22, 0.24, 0.12, 8, STONE_D).move(0, h, 0)
    return m.vary(0.07, seed)


def cut_top(m, h, seed):
    import random
    r = random.Random(seed)
    return m.bend(lambda p: (p[0], min(p[1], h + r.uniform(-0.12, 0.12)) if p[1] > h - 0.1 else p[1], p[2]))


def tower():
    """Himmelsturm: hoch, sich verjüngend, Galerien, oben abgebrochen"""
    m = Mesh()
    glow = Mesh()
    y = 0.0
    r = 5.0
    levels = 9
    for i in range(levels):
        h = 5.2 - i * 0.25
        r1 = r * 0.9
        col = mix(STONE, STONE_D, 0.15 + 0.05 * (i % 2))
        sec = lathe([(r, y), (r1, y + h)], 12, col, cap_bottom=False, cap_top=False)
        if i == levels - 1:
            sec = sec.bend(lambda p: (p[0], p[1] - (0 if p[1] < y + 1 else (1.5 + 1.4 * math.sin(math.atan2(p[0], p[2]) * 3)) ), p[2]))
        m += sec
        if i < levels - 1:
            m += lathe([(r1 * 1.14, y + h - 0.35), (r1 * 1.14, y + h)], 12, VIOLET, cap_bottom=True, cap_top=True)  # Galerie
            for k in range(6):                                                        # Fenster
                a = 2 * PI * k / 6 + i * 0.4
                rw = r - (r - r1) * 0.35 + 0.03
                glow += box(0.45, 1.0, 0.12, WARM).ry(a).move(math.sin(a) * rw, y + h * 0.35, math.cos(a) * rw)
        y += h
        r = r1
    # Fundament
    m += lathe([(7.5, -6), (6.5, -1.0), (5.2, 0.0)], 12, ROCK, cap_top=False).jitter(0.4, 9)
    # schwebende Trümmer über der Bruchkante
    for k in range(9):
        a = 2 * PI * k / 9
        rr = r * (0.8 + 0.5 * (k % 3) / 2)
        m += ico(0.6 + 0.35 * (k % 3), STONE).jitter(0.25, 20 + k).move(math.sin(a) * rr, y + 1.5 + (k % 4) * 1.6, math.cos(a) * rr)
    crystals = Mesh()
    for k in range(5):
        a = 2 * PI * k / 5 + 0.3
        crystals += octa(0.4, 1.1, CRYSTAL).rz(0.3).move(math.sin(a) * r * 1.5, y + 3 + k * 0.8, math.cos(a) * r * 1.5)
    crystals += octa(1.0, 2.6, CRYSTAL).move(0, y + 6.5, 0)
    return m.vary(0.05, 31), glow, crystals, y


def shard(seed, s=1.0):
    """kleiner schwebender Felssplitter mit Grashaube"""
    rnd = __import__("random").Random(seed)
    m = island(r=0.55 * s, depth=0.9 * s, seg=7, seed=seed)
    if rnd.random() < 0.5:
        m += ruin_column(0.6 * s, True, seed).scale(0.8 * s).move(0.1, 0, 0)
    return m


def pedestal():
    m = prism(0.95, 0.85, 0.18, 12, STONE_D)
    m += prism(0.8, 0.75, 0.12, 12, STONE).move(0, 0.18, 0)
    m += lathe([(0.75, 0.3), (0.78, 0.32)], 12, GOLD, cap_bottom=False)
    for k in range(12):
        a = 2 * PI * (k + 0.5) / 12
        m += box(0.08, 0.14, 0.04, GOLD).move(math.sin(a) * 0.9, 0.02, math.cos(a) * 0.9).ry(0)
    runes = Mesh()
    for k in range(8):
        a = 2 * PI * k / 8
        runes += box(0.12, 0.012, 0.04, rgb(0xFFFFFF)).ry(a).move(math.sin(a) * 0.62, 0.305, math.cos(a) * 0.62)
    return m.vary(0.05, 41), runes


def dummy():
    WOOD, WOOD_D, STRAW, CLOTH, RED, ROPE = rgb(0x8A5A34), rgb(0x5A3A22), rgb(0xD8B860), rgb(0xC8B898), rgb(0xD83A3A), rgb(0xB89A6A)
    base = Mesh()
    for a in (0, PI / 2):
        base += box(0.9, 0.1, 0.12, WOOD_D).ry(a)
    base += prism(0.06, 0.06, 0.55, 6, WOOD)
    top = Mesh()
    top += prism(0.055, 0.05, 1.0, 6, WOOD).move(0, 0.0, 0)
    top += lathe([(0.2, 0.25), (0.25, 0.45), (0.25, 0.7), (0.2, 0.88), (0.08, 0.95)], 8, STRAW).scale(1, 1, 0.8)
    for y in (0.35, 0.82):
        top += lathe([(0.255, y), (0.255, y + 0.04)], 8, ROPE, cap_bottom=False, cap_top=False).scale(1, 1, 0.8)
    top += prism(0.035, 0.035, 0.9, 6, WOOD).rz(PI / 2).move(0.45, 0.8, 0)
    for sx in (-1, 1):
        top += lathe([(0.0, 0), (0.07, 0.02), (0.08, 0.14), (0.0, 0.18)], 6, STRAW).rz(sx * PI / 2).move(sx * 0.44, 0.8, 0)
    top += sphere(0.16, 8, 6, CLOTH).move(0, 1.12, 0)
    top += box(0.05, 0.02, 0.01, rgb(0x2A1A10)).move(-0.05, 1.15, 0.155) + box(0.05, 0.02, 0.01, rgb(0x2A1A10)).move(0.05, 1.15, 0.155)
    tgt = ring(0.14, 0.025, 12, RED).rx(PI / 2).move(0, 0.58, 0.21) + ring(0.06, 0.02, 10, RED).rx(PI / 2).move(0, 0.58, 0.212)
    top += tgt
    return base.vary(0.06, 61), top.vary(0.06, 62)


def arena(R=4.7, dark=False):
    import random
    rnd = random.Random(8)
    m = Mesh()
    glow = Mesh()
    TILE = [rgb(0x9A90A8), rgb(0x887E96), rgb(0xA89EB2), rgb(0x7C7288), rgb(0x948A9C)]
    MOSS = rgb(0x7A9A6A)
    if dark:
        TILE = [rgb(0x6E6670), rgb(0x5E5862), rgb(0x76706E), rgb(0x56505A), rgb(0x68625E)]
        MOSS = rgb(0x4E6A3E)
    rings = [(0.95, 1.9, 9), (1.9, 2.85, 15), (2.85, 3.8, 21), (3.8, R, 27)]
    for r0, r1, n in rings:
        off = rnd.uniform(0, 1)
        for k in range(n):
            a0 = 2 * PI * (k + off) / n + 0.035 / r1 * 3
            a1 = 2 * PI * (k + 1 + off) / n - 0.035 / r1 * 3
            g0, g1 = r0 + 0.04, r1 - 0.04
            h = 0.07 + rnd.uniform(-0.012, 0.012)
            col = rnd.choice(TILE)
            if rnd.random() < 0.12:
                col = mix(col, MOSS, 0.5)
            pts = []
            for i in range(3):
                a = a0 + (a1 - a0) * i / 2
                pts.append((math.sin(a) * g1, math.cos(a) * g1))
            for i in range(3):
                a = a1 + (a0 - a1) * i / 2
                pts.append((math.sin(a) * g0, math.cos(a) * g0))
            cx = sum(p[0] for p in pts) / 6; cz = sum(p[1] for p in pts) / 6
            for i in range(6):
                p0, p1 = pts[i], pts[(i + 1) % 6]
                m.tri((cx, h + 0.005, cz), (p1[0], h, p1[1]), (p0[0], h, p0[1]), col)
                m.quad((p0[0], h, p0[1]), (p1[0], h, p1[1]), (p1[0], 0.0, p1[1]), (p0[0], 0.0, p0[1]), shade(col, 0.72))
    # Mittelscheibe mit Sternintarsie
    m += prism(0.95, 0.95, 0.075, 16, rgb(0x8A82A0))
    star = []
    for i in range(16):
        a = PI * i / 8
        rr = 0.82 if i % 2 == 0 else 0.32
        star.append((math.sin(a) * rr, math.cos(a) * rr))
    for i in range(16):
        p0, p1 = star[i], star[(i + 1) % 16]
        glow.tri((0, 0.081, 0), (p0[0], 0.081, p0[1]), (p1[0], 0.081, p1[1]), rgb(0x8AF0FF) if i % 2 == 0 else rgb(0x5AC8E8))
    m += lathe([(0.97, 0.0), (0.97, 0.08)], 16, GOLD, cap_bottom=False, cap_top=False)
    # Fugen (dunkle Grundscheibe) + Randsteine
    m += prism(R + 0.02, R + 0.02, 0.03, 30, rgb(0x2A2438))
    for k in range(30):
        a0, a1 = 2 * PI * k / 30 + 0.02, 2 * PI * (k + 1) / 30 - 0.02
        cols = rgb(0x8A8098) if k % 2 else rgb(0x7A7088)
        pts = [(math.sin(a0) * (R + 0.02), math.cos(a0) * (R + 0.02)), (math.sin(a1) * (R + 0.02), math.cos(a1) * (R + 0.02)),
               (math.sin(a1) * (R + 0.32), math.cos(a1) * (R + 0.32)), (math.sin(a0) * (R + 0.32), math.cos(a0) * (R + 0.32))]
        h = 0.2
        m.quad(*[(x, h, z) for x, z in reversed(pts)], cols) if False else m.quad((pts[3][0], h, pts[3][1]), (pts[2][0], h, pts[2][1]), (pts[1][0], h, pts[1][1]), (pts[0][0], h, pts[0][1]), cols)
        for i in range(4):
            p0, p1 = pts[i], pts[(i + 1) % 4]
            m.quad((p1[0], h, p1[1]), (p0[0], h, p0[1]), (p0[0], 0, p0[1]), (p1[0], 0, p1[1]), shade(cols, 0.75))
    return outward_top(m), glow


def outward_top(m):
    """Seitenflächen nach außen/innen korrekt drehen: Normale weg von der Kachelmitte"""
    out = []
    for a, b, c, col in m.tris:
        n = cross(sub(b, a), sub(c, a))
        if abs(n[1]) > 1e-6 and abs(n[1]) > 0.5 * math.sqrt(dot(n, n)):
            out.append((a, c, b, col) if n[1] < 0 else (a, b, c, col))   # Deckflächen nach oben
        else:
            out.append((a, b, c, col))
    return Mesh(out)


def brazier():
    IRON, STONE2 = rgb(0x4A4458), rgb(0x8A8098)
    m = prism(0.32, 0.28, 0.14, 8, STONE2)
    m += prism(0.09, 0.07, 0.8, 6, IRON).move(0, 0.14, 0)
    m += lathe([(0.06, 0.9), (0.3, 1.0), (0.36, 1.14), (0.33, 1.16)], 8, IRON, cap_top=False)
    m += prism(0.3, 0.3, 0.02, 8, rgb(0x2A1810)).move(0, 1.1, 0)
    fire = Mesh()
    for i in range(7):
        a = 2 * PI * i / 7
        from_c = rgb(0xFF5A1E); to_c = rgb(0xFFE070)
        h = 0.35 + 0.15 * (i % 3) / 2
        pts = [(-0.08, 0), (0.0, h), (0.08, 0)]
        f = slab(pts, 0.04, from_c).recolor(lambda c, p, h=h: mix(from_c, to_c, max(0, min(1, p[1] / h))))
        fire += f.ry(a).move(math.sin(a) * 0.12, 1.1, math.cos(a) * 0.12)
    fire += octa(0.12, 0.3, rgb(0xFFF0A0)).move(0, 1.18, 0)
    return m.vary(0.06, 71), fire


def dungeon_wall(R=5.25):
    """Rundsaal: hohe Wand hinten, niedrige Brüstung vorne (Kamera), Strebepfeiler, Wandfackeln"""
    import random
    rnd = random.Random(21)
    m = Mesh()
    fl = Mesh()
    STONE = [rgb(0x5A5460), rgb(0x4E4856), rgb(0x625C66), rgb(0x48424E)]
    seg = 28
    for k in range(seg):
        a0, a1 = 2 * PI * k / seg, 2 * PI * (k + 1) / seg
        am = (a0 + a1) / 2
        front = math.cos(am) > 0.35          # Richtung +z = Kamera
        h = 0.55 if front else 3.4
        rows = 2 if front else 7
        for r in range(rows):
            y0, y1 = h * r / rows, h * (r + 1) / rows
            off = (r % 2) * 0.5
            b0, b1 = a0 + (a1 - a0) * off * 0.0, a1
            col = rnd.choice(STONE)
            p = [(math.sin(a0) * R, math.cos(a0) * R), (math.sin(a1) * R, math.cos(a1) * R)]
            q = [(math.sin(a0) * (R + 0.5), math.cos(a0) * (R + 0.5)), (math.sin(a1) * (R + 0.5), math.cos(a1) * (R + 0.5))]
            inset = 0.02 if (r + k) % 2 else 0
            m.quad((p[1][0], y0, p[1][1]), (p[0][0], y0, p[0][1]), (p[0][0], y1 - inset, p[0][1]), (p[1][0], y1 - inset, p[1][1]), col)
            if r == rows - 1:
                m.quad((p[1][0], y1, p[1][1]), (p[0][0], y1, p[0][1]), (q[0][0], y1, q[0][1]), (q[1][0], y1, q[1][1]), shade(col, 1.1))
        if not front and k % 3 == 0:                                       # Strebepfeiler + Fackel
            x, z = math.sin(a0) * (R - 0.12), math.cos(a0) * (R - 0.12)
            m += box(0.5, 3.6, 0.36, rgb(0x6A6470), top=0.85).ry(a0).move(x, 0, z)
            tx, tz = math.sin(a0) * (R - 0.42), math.cos(a0) * (R - 0.42)
            m += box(0.08, 0.32, 0.08, rgb(0x3A3038)).move(tx, 1.75, tz)
            m += lathe([(0.04, 2.05), (0.11, 2.14), (0.12, 2.2)], 6, rgb(0x2E2830), cap_top=False).move(tx, 0, tz)
            for i in range(5):
                aa = 2 * PI * i / 5
                f = slab([(-0.05, 0), (0.0, 0.26 + 0.06 * (i % 2)), (0.05, 0)], 0.03, rgb(0xFF5A1E))
                f = f.recolor(lambda c, pp: mix(rgb(0xFF5A1E), rgb(0xFFE070), max(0, min(1, pp[1] / 0.3))))
                fl += f.ry(aa).move(tx + math.sin(aa) * 0.05, 2.18, tz + math.cos(aa) * 0.05)
        if not front and k % 3 == 1 and rnd.random() < 0.7:               # Kette / Fass / Knochen
            x, z = math.sin(am) * (R - 0.35), math.cos(am) * (R - 0.35)
            if rnd.random() < 0.5:
                m += lathe([(0.16, 0), (0.2, 0.25), (0.16, 0.5)], 7, rgb(0x6A4A2E)).move(x, 0, z)
                m += lathe([(0.205, 0.12), (0.205, 0.15)], 7, rgb(0x3A3A40), cap_bottom=False, cap_top=False).move(x, 0, z)
            else:
                for i in range(4):
                    m += tube([(0, 0, 0), (0.18, 0.02, 0.05)], [0.02, 0.02], 4, rgb(0xD8D0B8)).ry(rnd.uniform(0, 6)).move(x + rnd.uniform(-0.2, 0.2), 0.02, z + rnd.uniform(-0.2, 0.2))
                m += sphere(0.07, 6, 4, rgb(0xE0D8C0)).move(x, 0.07, z)
    # Bogen-Nischen oben (Gewölbe-Andeutung)
    for k in range(0, seg, 4):
        am = 2 * PI * (k + 1.5) / seg
        if math.cos(am) > 0.2:
            continue
        x, z = math.sin(am) * (R - 0.02), math.cos(am) * (R - 0.02)
        m += slab([(-0.35, 0), (0.35, 0), (0.35, 0.9), (0.25, 1.2), (0, 1.32), (-0.25, 1.2), (-0.35, 0.9)], 0.04, rgb(0x241E2A)).ry(am).move(x, 1.0, z)
    return m.vary(0.06, 22), fl


def build(out):
    isl = island()
    for i, (x, z) in enumerate([(-1.4, 0.6), (-0.6, 1.5), (1.2, 1.2), (1.7, -0.5), (0.4, -1.3), (-1.6, -0.8)]):
        isl += tuft(i).move(x, 0.0, z)
    isl += ruin_column(1.6, True, 4).move(-1.55, -0.02, -0.9)
    isl += ruin_column(1.0, True, 6).rz(1.35).move(-0.9, 0.12, -1.6)
    isl += box(0.5, 0.25, 0.4, STONE_D).ry(0.4).move(1.4, -0.05, -1.0).vary(0.05, 2)
    parts = {
        "island": isl,
        "shard_a": shard(51, 1.0), "shard_b": shard(52, 1.4), "shard_c": shard(53, 0.8),
    }
    t, tg, tc, top = tower()
    parts["tower"] = t
    parts["tower_glow"] = tg
    parts["tower_crystals"] = tc
    parts["dummy_base"], parts["dummy_top"] = dummy()
    parts["arena"], parts["arena_glow"] = arena()
    parts["arena_dark"], _ = arena(dark=True)
    parts["dwall"], parts["dwall_fire"] = dungeon_wall()
    parts["brazier"], parts["brazier_fire"] = brazier()
    parts["island_plain"] = island(r=2.7, depth=3.4, seg=16, seed=12)
    p, runes = pedestal()
    parts["pedestal"] = p
    parts["pedestal_glow"] = runes
    for k, m in parts.items():
        nv, nt = m.save(os.path.join(out, "w_%s.fmesh" % k))
        print("w_%-18s %5d Vertices %5d Dreiecke" % (k, nv, nt))
    return top


if __name__ == "__main__":
    out = os.path.join(HERE, "..", "data")
    os.makedirs(out, exist_ok=True)
    print("Turmhöhe", build(out))
