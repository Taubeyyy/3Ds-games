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


def kristallspinne():
    BODY, CRY, EYE = rgb(0x2A2C44), rgb(0x8AE8FF), rgb(0xFF5AD0)
    b = sphere(0.24, 7, 4, BODY, squash=0.7).scale(1, 1, 1.3).move(0, 0.34, -0.05)
    b += sphere(0.15, 6, 4, BODY).move(0, 0.32, 0.28)
    for sx in (-1, 1):
        for k in range(4):
            z = -0.2 + k * 0.14
            b += tube([(sx * 0.15, 0.36, z), (sx * 0.4, 0.55, z + 0.05), (sx * 0.55, 0.0, z + 0.1)], [0.025, 0.02, 0.01], 4, BODY)
    glow = Mesh()
    for i, (x, z) in enumerate([(0, -0.12), (0.1, 0.0), (-0.1, 0.02), (0, 0.1)]):
        glow += octa(0.05, 0.16 + 0.04 * (i % 2), CRY).move(x, 0.5, z - 0.1)
    for sx in (-1, 1):
        glow += box(0.03, 0.03, 0.01, EYE).move(sx * 0.05, 0.36, 0.42)
    return dict(body=b.vary(0.06, 87), glow=glow)


def schuetze():
    ROBE, ROBE2, CRY = rgb(0x3A2E5A), rgb(0x241C3A), rgb(0xB08AFF)
    b = lathe([(0.24, 0), (0.2, 0.5), (0.16, 0.9), (0.1, 1.2), (0.0, 1.32)], 7, ROBE).recolor(lambda c, p: mix(ROBE2, ROBE, max(0, min(1, p[1] / 1.0))))
    b += sphere(0.13, 6, 4, ROBE2).move(0, 1.3, 0.02)
    b += tube([(0.14, 1.05, 0.05), (0.3, 1.0, 0.25), (0.32, 1.0, 0.45)], [0.035, 0.03, 0.025], 5, ROBE)
    bow = tube([(0, -0.4, 0), (0.06, -0.2, 0), (0.08, 0, 0), (0.06, 0.2, 0), (0, 0.4, 0)], [0.012, 0.02, 0.024, 0.02, 0.012], 4, rgb(0x6A5AA0))
    b += bow.ry(PI / 2).move(0.32, 1.0, 0.48)
    glow = box(0.12, 0.02, 0.02, CRY).move(0, 1.32, 0.13) + octa(0.05, 0.1, CRY).move(0, 0.85, 0.2)
    glow += box(0.006, 0.8, 0.006, CRY).move(0.32, 0.6, 0.48)
    return dict(body=b.vary(0.05, 88), glow=glow)


def geode():
    ROCK, CRY = rgb(0x4A4A62), rgb(0xD08AFF)
    b = ico(0.5, ROCK).jitter(0.07, 89).scale(1, 0.85, 1).move(0, 0.45, 0)
    for sx in (-1, 1):
        b += ico(0.16, shade(ROCK, 0.85)).jitter(0.03, 90 + sx).move(sx * 0.3, 0.12, 0.15)
    glow = Mesh()
    for i in range(6):
        a = 2 * PI * i / 6
        glow += octa(0.07, 0.2, CRY).rx(0.6).ry(a).move(math.sin(a) * 0.32, 0.62, math.cos(a) * 0.32)
    glow += box(0.18, 0.04, 0.02, rgb(0xFFE0FF)).move(0, 0.55, 0.47)
    return dict(body=b.vary(0.07, 91), glow=glow)


def koenigin():
    CRY, CRY2, DARK, GOLD = rgb(0x8AE8FF), rgb(0xC8A0FF), rgb(0x1E1A34), rgb(0xE8C060)
    b = lathe([(0.0, 0.0), (0.55, 0.35), (0.45, 1.1), (0.3, 1.6), (0.38, 1.9), (0.0, 2.1)], 9, DARK)
    b += sphere(0.22, 8, 5, rgb(0xC8D8F0)).move(0, 2.3, 0.03)
    for sx in (-1, 1):
        b += tube([(sx * 0.32, 1.8, 0), (sx * 0.7, 1.6, 0.2), (sx * 0.95, 1.85, 0.45)], [0.07, 0.06, 0.04], 5, DARK)
    glow = Mesh()
    for i in range(7):                                                   # Kristallkrone
        a = -0.9 + 0.3 * i
        glow += octa(0.05, 0.22 - abs(a) * 0.08, CRY if i % 2 else CRY2).rz(-a * 0.5).move(math.sin(a) * 0.2, 2.55 - abs(a) * 0.05, math.cos(a) * 0.12)
    for i in range(10):                                                  # Kristallrock
        a = 2 * PI * i / 10
        glow += octa(0.12, 0.45, CRY2 if i % 2 else CRY).rx(0.5).ry(a).move(math.sin(a) * 0.5, 0.35, math.cos(a) * 0.5)
    for sx in (-1, 1):
        glow += octa(0.1, 0.25, CRY).move(sx * 0.98, 1.95, 0.5)
    glow += box(0.14, 0.03, 0.02, rgb(0xFFFFFF)).move(0, 2.32, 0.24)
    return dict(body=b.vary(0.05, 92), glow=glow)


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


def dagger():
    BLADE, DARK, TRIM, GLOW = rgb(0x4A4660), rgb(0x1C1626), rgb(0x8A5AC8), rgb(0x9A6AFF)
    w = blade(0.42, 0.06, 0.018, BLADE, edge=rgb(0x2A2638)).move(0, 0.08, 0)
    w += box(0.12, 0.03, 0.04, DARK, top=0.7).move(0, 0.06, 0)
    w += prism(0.016, 0.014, 0.09, 6, rgb(0x3A2E36)).move(0, -0.03, 0)
    w += octa(0.02, 0.025, TRIM).move(0, -0.05, 0)
    g = slab([(-0.008, 0.1), (0.008, 0.1), (0.0, 0.46)], 0.024, GLOW)
    return w, g


def scythe():
    WOOD, DARK, BLADE, GLOW = rgb(0x2E2436), rgb(0x1C1626), rgb(0x5A5670), rgb(0x8AE8FF)
    w = prism(0.022, 0.022, 1.7, 6, WOOD).move(0, -0.55, 0)
    w += box(0.07, 0.08, 0.07, DARK).move(0, 1.1, 0)
    pts = [(0.0, 0.0), (0.12, 0.06), (0.42, 0.04), (0.7, -0.08), (0.86, -0.26), (0.62, -0.12), (0.36, -0.06), (0.1, -0.06)]
    w += slab(pts, 0.025, BLADE).move(0.02, 1.13, 0)
    g = slab([(0.12, 0.0), (0.42, -0.02), (0.7, -0.12), (0.82, -0.24), (0.66, -0.1), (0.4, -0.05)], 0.03, GLOW).move(0.02, 1.13, 0)
    g += octa(0.03, 0.05, GLOW).move(0, -0.6, 0)
    return w, g


def altar():
    STONE, STONE2, GOLD = rgb(0x6A6474), rgb(0x4E4858), rgb(0xE8C060)
    m = prism(0.6, 0.55, 0.2, 8, STONE2)
    m += prism(0.3, 0.22, 0.9, 8, STONE).move(0, 0.2, 0)
    m += lathe([(0.2, 1.1), (0.45, 1.15), (0.48, 1.22), (0.3, 1.25)], 8, STONE2, cap_top=True)
    m += lathe([(0.49, 1.18), (0.49, 1.21)], 8, GOLD, cap_bottom=False, cap_top=False)
    g = Mesh()
    pts = []
    for i in range(16):
        a = PI / 2 + PI * i / 8
        rr = 0.24 if i % 2 == 0 else 0.09
        pts.append((math.cos(a) * rr, math.sin(a) * rr))
    g += slab(pts, 0.05, rgb(0xFFE8A0)).move(0, 1.75, 0)
    g += ring(0.5, 0.012, 20, rgb(0x8AF0FF)).rx(PI / 2).move(0, 1.75, 0)
    for i in range(6):
        a = 2 * PI * i / 6
        g += octa(0.025, 0.04, rgb(0xFFFFFF)).move(math.cos(a) * 0.5, 1.75 + math.sin(a) * 0.5, 0)
    return m.vary(0.05, 141), g


def rack():
    WOOD, WOOD2, IRON = rgb(0x5A3A26), rgb(0x3E2818), rgb(0x4A4458)
    m = Mesh()
    for sx in (-1, 1):
        m += box(0.1, 1.5, 0.1, WOOD).move(sx * 0.75, 0, 0)
        m += box(0.3, 0.06, 0.3, WOOD2).move(sx * 0.75, 0, 0)
    for y in (0.5, 1.3):
        m += box(1.6, 0.07, 0.08, WOOD2).move(0, y, 0)
    for i, x in enumerate((-0.45, 0.0, 0.45)):
        m += box(0.05, 0.12, 0.1, IRON).move(x, 1.24, 0.05)
    return m.vary(0.06, 142)


def hubdecor(R=5.25):
    import random
    rnd = random.Random(55)
    m = Mesh()
    RUG1, RUG2, GOLD = rgb(0x4A2A5A), rgb(0x2E1A3E), rgb(0xC8A050)
    rug = Mesh()
    for k in range(24):                                                  # runder Teppich
        a0, a1 = 2 * PI * k / 24, 2 * PI * (k + 1) / 24
        for ri, (r0, r1, c) in enumerate([(0.0, 1.4, RUG2), (1.4, 1.55, GOLD), (1.55, 2.4, RUG1), (2.4, 2.52, GOLD)]):
            p = [(math.sin(a) * r, math.cos(a) * r) for a, r in ((a0, r0), (a1, r0), (a1, r1), (a0, r1))]
            rug.quad((p[0][0], 0.09, p[0][1]), (p[3][0], 0.09, p[3][1]), (p[2][0], 0.09, p[2][1]), (p[1][0], 0.09, p[1][1]), shade(c, 1 + 0.06 * ((k + ri) % 2)))
    m += rug
    for k, am in enumerate((PI - 1.15, PI + 1.15)):                      # Bücherregale an der Wand
        x, z = math.sin(am) * (R - 0.4), math.cos(am) * (R - 0.4)
        sh = box(1.4, 2.0, 0.4, rgb(0x3E2818)).ry(am)
        for row in range(4):
            for b in range(8):
                c = rnd.choice([rgb(0x8A3A3A), rgb(0x3A5A8A), rgb(0x4A7A4A), rgb(0x8A7A3A), rgb(0x6A3A7A)])
                hb = rnd.uniform(0.28, 0.4)
                sh += box(0.12, hb, 0.26, c).move(-0.55 + b * 0.157, 0.12 + row * 0.47, 0.06).ry(am)
        m += sh.move(x, 0, z)
    for am in (PI - 0.42, PI + 0.42, PI - 1.9, PI + 1.9):                # Banner
        x, z = math.sin(am) * (R - 0.08), math.cos(am) * (R - 0.08)
        ban = Mesh().quad((-0.35, 3.0, 0), (0.35, 3.0, 0), (0.35, 1.3, 0), (-0.35, 1.3, 0), rgb(0x3A1E5A))
        ban.tri((-0.35, 1.3, 0), (0.35, 1.3, 0), (0.0, 1.0, 0), rgb(0x3A1E5A))
        ban = double_sided(ban)
        st = []
        for i in range(8):
            a = PI / 2 + PI * i / 4
            rr = 0.16 if i % 2 == 0 else 0.06
            st.append((math.cos(a) * rr, math.sin(a) * rr + 2.2))
        ban += slab(st, 0.01, GOLD).move(0, 0, 0.01)
        m += ban.ry(am + PI).move(x, 0, z)
    for i in range(5):                                                   # Kisten und Fässer
        am = PI + rnd.uniform(-2.3, 2.3)
        if abs(am - PI) < 0.6:
            continue
        x, z = math.sin(am) * (R - 0.55), math.cos(am) * (R - 0.55)
        if rnd.random() < 0.5:
            m += box(0.5, 0.5, 0.5, rgb(0x6A4A2E)).ry(rnd.uniform(0, 1)).move(x, 0.07, z)
        else:
            m += lathe([(0.2, 0), (0.24, 0.3), (0.2, 0.6)], 8, rgb(0x5A3A24)).move(x, 0.07, z)
    return m.vary(0.04, 143)


def build(out):
    parts = {}
    for name, fn in (("shard", splitterling), ("crawler", krabbler), ("wisp", irrlicht), ("bomber", gluetkaefer), ("brute", waechter),
                     ("spider", kristallspinne), ("archer", schuetze), ("geode", geode), ("queen", koenigin)):
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
    parts["r_dagger"], parts["r_dagger_glow"] = dagger()
    parts["r_scythe"], parts["r_scythe_glow"] = scythe()
    parts["r_altar"], parts["r_altar_glow"] = altar()
    parts["r_rack"] = rack()
    parts["r_hubdecor"] = hubdecor()
    tris = 0
    for k, m in parts.items():
        nv, nt = m.save(os.path.join(out, "%s.fmesh" % k))
        tris += nt
    print("Gegner/Requisiten:", len(parts), "Modelle,", tris, "Dreiecke")
    return list(parts.keys())


if __name__ == "__main__":
    build(os.path.join(HERE, "..", "data"))
