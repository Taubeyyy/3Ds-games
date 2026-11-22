"""SKYBREAKER – Gegner und Lauf-Requisiten (Türbogen, Belohnungen, Hindernisse). Ausgabe: data/e_*.fmesh, data/r_*.fmesh"""
import math, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "tools"))
from lowpoly import *
from characters import flame, star, leaf, bolt

PI = math.pi


def splitterling():
    """kristalliner Wicht mit Klingenarmen"""
    STONE, CRY, CRY2, EYE = rgb(0x3A3448), rgb(0x7A5AC8), rgb(0x9A7AE8), rgb(0xFF5AA0)
    b = Mesh()
    for sx in (-1, 1):
        b += prism(0.07, 0.05, 0.4, 5, STONE).move(sx * 0.12, 0, 0)
        b += box(0.12, 0.06, 0.18, shade(STONE, 0.8)).move(sx * 0.12, 0, 0.03)
    b += box(0.32, 0.14, 0.2, STONE, top=0.8).move(0, 0.34, 0)
    b += octa(0.24, 0.32, CRY).scale(1, 1, 0.8).move(0, 0.72, 0)
    for i, (x, y, z, s) in enumerate([(-0.18, 0.86, -0.05, 0.12), (0.2, 0.9, -0.02, 0.1), (0.0, 0.98, -0.14, 0.14), (-0.1, 0.62, 0.12, 0.08)]):
        b += octa(s * 0.6, s * 1.4, CRY2 if i % 2 else CRY).rz(0.4 * (1 if x > 0 else -1)).move(x, y, z)
    b += octa(0.15, 0.17, CRY2).move(0, 1.12, 0.02)
    arms = Mesh()
    for sx in (-1, 1):
        arms += tube([(sx * 0.2, 0.86, 0), (sx * 0.34, 0.7, 0.08), (sx * 0.36, 0.52, 0.2)], [0.04, 0.035, 0.03], 5, STONE)
        arms += slab([(0, 0), (0.05, 0.05), (0.02, 0.4), (-0.02, 0.06)], 0.02, CRY2).rx(1.1).move(sx * 0.36, 0.52, 0.2)
    glow = Mesh()
    for sx in (-1, 1):
        glow += box(0.04, 0.025, 0.01, EYE).move(sx * 0.05, 1.13, 0.13)
    glow += octa(0.06, 0.1, rgb(0xFF8AD0)).move(0, 0.74, 0.17)
    return dict(body=(b + arms).vary(0.06, 81), glow=glow)


def krabbler():
    ROCK, MOSS, EYE = rgb(0x6A6070), rgb(0x6A8A4A), rgb(0xFFB040)
    b = sphere(0.28, 7, 4, ROCK, squash=0.55).scale(1, 1, 1.2).move(0, 0.24, 0)
    b = b.recolor(lambda c, p: mix(c, MOSS, 0.6) if p[1] > 0.33 else c)
    b += sphere(0.13, 6, 3, shade(ROCK, 0.85), squash=0.7).move(0, 0.2, 0.3)
    for sx in (-1, 1):
        for k in range(3):
            z = -0.15 + k * 0.15
            b += tube([(sx * 0.2, 0.2, z), (sx * 0.38, 0.22, z + 0.02), (sx * 0.44, 0.0, z + 0.04)], [0.03, 0.025, 0.012], 4, shade(ROCK, 0.7))
        b += slab([(0, 0), (0.08, 0.02), (0.1, 0.06), (0.02, 0.03)], 0.02, rgb(0xD8C8A8)).ry(-sx * 0.6).move(sx * 0.06, 0.16, 0.4)
    glow = box(0.03, 0.03, 0.01, EYE).move(-0.05, 0.25, 0.42) + box(0.03, 0.03, 0.01, EYE).move(0.05, 0.25, 0.42)
    return dict(body=b.jitter(0.012, 82).vary(0.07, 83), glow=glow)


def irrlicht():
    ROBE, ROBE2, ORB = rgb(0x2A6A78), rgb(0x123A48), rgb(0x8AF8FF)
    b = lathe([(0.0, -0.1), (0.25, 0.05), (0.2, 0.4), (0.14, 0.7), (0.0, 0.88)], 7, ROBE).move(0, 0.6, 0)
    b = b.recolor(lambda c, p: mix(ROBE2, ROBE, max(0, min(1, (p[1] - 0.5) / 0.6))))
    for i in range(7):
        a = 2 * PI * i / 7
        b += slab([(-0.05, 0), (0.05, 0), (0.0, -0.18)], 0.015, ROBE2).ry(a).move(math.sin(a) * 0.23, 0.56, math.cos(a) * 0.23)
    b += tube([(0.14, 1.1, 0.05), (0.3, 1.02, 0.18), (0.36, 0.94, 0.3)], [0.03, 0.025, 0.02], 4, ROBE2)
    b += prism(0.015, 0.015, 0.2, 4, rgb(0x3A3A40)).move(0.36, 0.74, 0.3)
    glow = ico(0.09, ORB).move(0.36, 0.7, 0.3)
    glow += box(0.035, 0.02, 0.01, ORB).move(-0.04, 1.28, 0.1) + box(0.035, 0.02, 0.01, ORB).move(0.04, 1.28, 0.1)
    return dict(body=b.vary(0.05, 84), glow=glow)


def gluetkaefer():
    BASALT, LAVA, LAVA2 = rgb(0x2E2428), rgb(0xFF6A1A), rgb(0xFFC040)
    b = sphere(0.3, 8, 4, BASALT, squash=0.6).scale(1, 1, 1.25).move(0, 0.26, 0)
    b += sphere(0.12, 6, 3, shade(BASALT, 1.2), squash=0.8).move(0, 0.22, 0.34)
    for sx in (-1, 1):
        for k in range(3):
            z = -0.18 + k * 0.16
            b += tube([(sx * 0.22, 0.2, z), (sx * 0.4, 0.2, z), (sx * 0.45, 0.0, z + 0.03)], [0.03, 0.025, 0.012], 4, BASALT)
    glow = Mesh()
    for i in range(5):
        a = -0.8 + 0.4 * i
        glow += box(0.03, 0.02, 0.36, LAVA if i % 2 else LAVA2).ry(a * 0.3).move(math.sin(a) * 0.18, 0.43, -0.02 + abs(a) * 0.05)
    glow += box(0.035, 0.03, 0.01, LAVA2).move(-0.05, 0.27, 0.45) + box(0.035, 0.03, 0.01, LAVA2).move(0.05, 0.27, 0.45)
    return dict(body=b.vary(0.06, 85), glow=glow)


def waechter():
    STONE, STONE2, BRONZE, VISOR, MOSS = rgb(0x8A8494), rgb(0x6A6474), rgb(0xA8743A), rgb(0xFFD25A), rgb(0x6A8A4A)
    b = Mesh()
    for sx in (-1, 1):
        b += box(0.24, 0.62, 0.26, STONE2, top=0.85).move(sx * 0.2, 0, 0)
        b += box(0.28, 0.1, 0.34, shade(STONE2, 0.8)).move(sx * 0.2, 0, 0.03)
    b += box(0.66, 0.2, 0.42, BRONZE).move(0, 0.6, 0)
    b += box(0.78, 0.66, 0.5, STONE, top=1.12).move(0, 0.78, 0)
    b += box(0.4, 0.3, 0.06, MOSS).move(0.1, 1.0, 0.24)
    b += box(0.36, 0.32, 0.36, STONE2, top=0.85).move(0, 1.44, 0.02)
    for sx in (-1, 1):
        b += box(0.34, 0.3, 0.4, STONE, top=0.8).move(sx * 0.52, 1.18, 0)
        b += tube([(sx * 0.5, 1.2, 0), (sx * 0.56, 0.86, 0.06), (sx * 0.56, 0.6, 0.16)], [0.11, 0.1, 0.09], 6, STONE2)
    shield = box(0.5, 0.72, 0.08, BRONZE, top=0.9).move(0, -0.36, 0).recolor(lambda c, p: mix(c, STONE, 0.3) if abs(p[0]) < 0.06 else c)
    b += shield.move(-0.66, 0.78, 0.26)
    maul = prism(0.035, 0.035, 0.9, 5, rgb(0x5A3A26)).move(0, -0.45, 0) + box(0.36, 0.24, 0.24, STONE2).move(0, 0.4, 0)
    glow = box(0.24, 0.035, 0.02, VISOR).move(0, 1.58, 0.2)
    b += maul.rx(0.5).move(0.58, 0.62, 0.22)
    return dict(body=b.vary(0.05, 86), glow=glow)


# ------------------------------------------------------------------ Requisiten
def door():
    STONE, STONE2, GOLD = rgb(0xB0A8C0), rgb(0x8A8098), rgb(0xE8C060)
    m = Mesh()
    for sx in (-1, 1):
        m += box(0.34, 2.2, 0.4, STONE, top=0.9).move(sx * 0.85, 0, 0)
        m += box(0.44, 0.18, 0.5, STONE2).move(sx * 0.85, 0, 0)
    seg = 7
    for i in range(seg):
        a0, a1 = PI * i / seg, PI * (i + 1) / seg
        r0, r1 = 0.68, 1.02
        p = [(math.cos(a0) * r1, math.sin(a0) * r1), (math.cos(a1) * r1, math.sin(a1) * r1), (math.cos(a1) * r0, math.sin(a1) * r0), (math.cos(a0) * r0, math.sin(a0) * r0)]
        m += slab(p[::-1], 0.4, STONE if i % 2 else STONE2).move(0, 2.1, 0)
    m += octa(0.1, 0.16, GOLD).move(0, 3.22, 0.05)
    portal = Mesh()
    pts = [(-0.68, 0.0)] + [(math.cos(PI - PI * i / 10) * 0.68, 2.1 + math.sin(PI - PI * i / 10) * 0.68) for i in range(11)] + [(0.68, 0.0)]
    portal += slab(pts[::-1], 0.04, rgb(0xFFFFFF))
    return m.vary(0.05, 91), portal


def column():
    STONE, STONE2 = rgb(0xB8B0C8), rgb(0x8A8098)
    m = prism(0.34, 0.34, 0.16, 8, STONE2)
    m += lathe([(0.24, 0.16), (0.22, 1.2), (0.23, 2.0)], 8, STONE)
    m = m.bend(lambda p: (p[0], min(p[1], 1.7 + 0.25 * math.sin(p[0] * 9 + p[2] * 7)) if p[1] > 1.5 else p[1], p[2]))
    m += ico(0.16, STONE2).jitter(0.04, 3).move(0.4, 0.1, 0.2)
    return m.vary(0.07, 92)


def rock():
    return ico(0.5, rgb(0x7A7088)).jitter(0.12, 5).scale(1.1, 0.75, 1).move(0, 0.25, 0).vary(0.08, 93)


def chest():
    WOOD, GOLD = rgb(0x8A5A34), rgb(0xE8C060)
    m = box(0.7, 0.36, 0.46, WOOD)
    m += box(0.74, 0.06, 0.5, GOLD).move(0, 0.34, 0)
    m += lathe([(0.23, 0), (0.23, 0.7)], 6, WOOD, cap_bottom=True).rz(PI / 2).scale(1, 0.6, 1).move(-0.35, 0.36, 0)
    m += box(0.1, 0.12, 0.04, GOLD).move(0, 0.3, 0.24)
    return m.vary(0.05, 94)


def heart():
    m = Mesh()
    pts = []
    for i in range(16):
        t = 2 * PI * i / 16
        pts.append((0.16 * math.sin(t) ** 3 * 1.6, 0.13 * math.cos(t) - 0.05 * math.cos(2 * t) - 0.02 * math.cos(3 * t)))
    return slab(pts[::-1], 0.12, rgb(0xFF4A6A))


def anvil():
    IRON, STONE = rgb(0x4A4458), rgb(0x8A8098)
    m = box(0.5, 0.3, 0.4, STONE, top=0.8)
    m += box(0.22, 0.25, 0.2, IRON).move(0, 0.3, 0)
    m += box(0.62, 0.14, 0.26, IRON).move(0.04, 0.55, 0)
    m += box(0.2, 0.08, 0.14, IRON, top=0.3).rz(-PI / 2).move(0.38, 0.62, 0)
    return m.vary(0.05, 95)


def fountain():
    STONE, WATER = rgb(0xB0A8C0), rgb(0x7AE8FF)
    m = lathe([(0.9, 0), (0.9, 0.3), (0.8, 0.32), (0.75, 0.1)], 12, STONE, cap_top=False)
    m += prism(0.12, 0.1, 0.9, 8, STONE)
    m += lathe([(0.0, 0.9), (0.4, 0.95), (0.42, 1.05)], 10, STONE, cap_top=False)
    w = prism(0.76, 0.76, 0.02, 12, WATER).move(0, 0.24, 0) + prism(0.38, 0.38, 0.02, 10, WATER).move(0, 1.0, 0)
    return m.vary(0.05, 96), w


def shrine():
    STONE, GOLD = rgb(0x9A90A8), rgb(0xE8C060)
    m = prism(0.5, 0.45, 0.2, 6, STONE)
    m += box(0.36, 1.1, 0.3, STONE, top=0.8).move(0, 0.2, 0)
    m += octa(0.1, 0.14, GOLD).move(0, 1.42, 0)
    return m.vary(0.06, 97)


def splitter_gem():
    return octa(0.14, 0.26, rgb(0x8AF0FF))


def coins():
    m = Mesh()
    for i, (x, z, h) in enumerate([(0, 0, 3), (0.14, 0.05, 2), (-0.1, 0.1, 2), (0.04, -0.13, 1)]):
        for k in range(h):
            m += prism(0.07, 0.07, 0.025, 8, rgb(0xFFD25A) if (i + k) % 2 else rgb(0xE8B040)).move(x, k * 0.028, z)
    return m


def build(out):
    parts = {}
    for name, fn in (("shard", splitterling), ("crawler", krabbler), ("wisp", irrlicht), ("bomber", gluetkaefer), ("brute", waechter)):
        for k, m in fn().items():
            parts["e_%s_%s" % (name, k)] = m
    parts["r_door"], parts["r_portal"] = door()
    parts["r_column"] = column()
    parts["r_rock"] = rock()
    parts["r_chest"] = chest()
    parts["r_heart"] = heart()
    parts["r_anvil"] = anvil()
    parts["r_fountain"], parts["r_water"] = fountain()
    parts["r_shrine"] = shrine()
    parts["r_gem"] = splitter_gem()
    parts["r_coins"] = coins()
    tris = 0
    for k, m in parts.items():
        nv, nt = m.save(os.path.join(out, "%s.fmesh" % k))
        tris += nt
    print("Gegner/Requisiten:", len(parts), "Modelle,", tris, "Dreiecke")
    return list(parts.keys())


if __name__ == "__main__":
    build(os.path.join(HERE, "..", "data"))
