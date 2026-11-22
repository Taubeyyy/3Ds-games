"""SKYBREAKER – Figuren (Low-Poly, flach schattiert). Erzeugt data/<figur>_<teil>.fmesh.
Teile: body (fest), cape/hair/scarf (schwingen), weapon (in der Hand), glow (leuchtet, unbeleuchtet), orbit (kreist).
Figuren stehen bei y=0 und schauen nach +Z."""
import math, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "tools"))
from lowpoly import *

PI = math.pi


# ------------------------------------------------------------------ Bausteine
def head(skin, hair, eye, y0=1.46, s=1.0, ears=False, face_z=1.0):
    h = Mesh()
    prof = [(0.0, 0), (0.06, 0.005), (0.095, 0.05), (0.108, 0.13), (0.1, 0.21), (0.072, 0.265), (0.0, 0.29)]
    h += lathe([(r * s, y0 + y * s) for r, y in prof], 8, skin).scale(1, 1, 0.96 * face_z)
    # Gesicht: Augen, Brauen, Nase, Mund
    for sx in (-1, 1):
        h += box(0.026 * s, 0.032 * s, 0.012 * s, eye).move(sx * 0.038 * s, y0 + 0.125 * s, 0.098 * s * face_z)
        h += box(0.008 * s, 0.009 * s, 0.006 * s, rgb(0xFFFFFF)).move(sx * 0.032 * s, y0 + 0.142 * s, 0.104 * s * face_z)
        if hair:
            h += box(0.04 * s, 0.008 * s, 0.01 * s, shade(hair, 0.8)).rz(sx * -0.18).move(sx * 0.04 * s, y0 + 0.165 * s, 0.1 * s * face_z)
        if ears:
            h += octa(0.02 * s, 0.035 * s, skin).move(sx * 0.105 * s, y0 + 0.12 * s, 0)
    h += box(0.018 * s, 0.03 * s, 0.02 * s, shade(skin, 0.92), top=0.6).move(0, y0 + 0.08 * s, 0.1 * s * face_z)
    h += box(0.034 * s, 0.007 * s, 0.008 * s, shade(skin, 0.7)).move(0, y0 + 0.05 * s, 0.096 * s * face_z)
    return h


def arm(side, sh, el, wr, sleeve, fore, hand, rs=1.0):
    """side: +1 rechts (x>0), -1 links"""
    m = Mesh()
    m += tube([sh, el], [0.062 * rs, 0.052 * rs], 6, sleeve)
    m += tube([el, wr], [0.05 * rs, 0.042 * rs], 6, fore)
    hp = (wr[0] + (wr[0] - el[0]) * 0.25, wr[1] - 0.05, wr[2] + (wr[2] - el[2]) * 0.25)
    m += ico(0.048 * rs, hand).scale(0.9, 1.15, 0.8).move(*hp)
    return m


def leg(x, hip_y, top, mid, boot, knee=None, r=1.0):
    m = Mesh()
    m += prism(0.068 * r, 0.088 * r, hip_y - 0.3, 6, top).move(x, 0.3, 0)
    m += prism(0.08 * r, 0.072 * r, 0.32, 6, boot).move(x, 0.0, 0)
    m += box(0.13 * r, 0.07, 0.17 * r, shade(boot, 0.9), top=0.8).move(x, 0, 0.05)
    m += prism(0.085 * r, 0.085 * r, 0.04, 6, mid).move(x, 0.3, 0)
    if knee:
        m += box(0.1 * r, 0.09, 0.03, knee, top=0.7).move(x, 0.42, 0.075 * r)
    return m


def cut(m, fn):
    """Dreiecke entfernen, für die fn(mittelpunkt) True ist"""
    return Mesh([t for t in m.tris if not fn(((t[0][0] + t[1][0] + t[2][0]) / 3, (t[0][1] + t[1][1] + t[2][1]) / 3, (t[0][2] + t[1][2] + t[2][2]) / 3))])


def flame(h, w, col, tip):
    pts = [(-w, 0), (-w * 0.6, h * 0.35), (-w * 0.85, h * 0.62), (-w * 0.25, h * 0.55), (0, h), (w * 0.3, h * 0.6), (w * 0.8, h * 0.7),
           (w * 0.55, h * 0.3), (w, 0)]
    m = slab(pts, w * 0.45, col)
    return m.recolor(lambda c, p: mix(col, tip, min(1, max(0, p[1] / h))))


def leaf(l, w, col):
    pts = [(0, 0), (w * 0.5, l * 0.3), (w * 0.4, l * 0.7), (0, l), (-w * 0.4, l * 0.7), (-w * 0.5, l * 0.3)]
    return slab(pts, 0.012, col)


def star(r, thick, col, n=5, inner=0.45):
    pts = []
    for i in range(n * 2):
        a = PI / 2 + PI * i / n
        rr = r if i % 2 == 0 else r * inner
        pts.append((math.cos(a) * rr, math.sin(a) * rr))
    return slab(pts, thick, col)


def bolt(h, w, col):
    pts = [(-w * 0.2, h), (w * 0.6, h * 0.55), (w * 0.05, h * 0.5), (w * 0.4, 0), (-w * 0.6, h * 0.48), (-w * 0.05, h * 0.52)]
    return slab(pts[::-1], w * 0.3, col)


# ------------------------------------------------------------------ Seren, die Sternenschmiedin (Heldin)
def seren():
    NAVY, NAVY_D, GOLD, BRONZE = rgb(0x26366A), rgb(0x1A2248), rgb(0xE0B050), rgb(0xA8743A)
    SKIN, HAIR, BOOT, LEATHER = rgb(0xEDBE9C), rgb(0xC0502C), rgb(0x4A3226), rgb(0x70482E)
    SCARF, STEEL, STAR, CAPE, EYE = rgb(0xEE6A3C), rgb(0xD0D8E8), rgb(0x8AF0FF), rgb(0x30479A), rgb(0x2A2440)
    b = Mesh()
    for x in (-0.1, 0.1):
        b += leg(x, 0.84, NAVY_D, BRONZE, BOOT, knee=BRONZE)
    b += prism(0.172, 0.168, 0.09, 8, LEATHER).move(0, 0.8, 0)                      # Gürtel
    b += box(0.07, 0.06, 0.03, GOLD).move(0, 0.815, 0.16)                            # Schnalle
    b += box(0.06, 0.1, 0.05, LEATHER).move(0.15, 0.74, 0.08)                         # Tasche
    skirt = lathe([(0.24, 0.6), (0.2, 0.72), (0.172, 0.86)], 8, NAVY, colors=[NAVY, NAVY], cap_bottom=False, cap_top=False)
    b += double_sided(cut(skirt, lambda p: p[2] > 0.12 and abs(p[0]) < 0.07))       # vorne offen
    b += lathe([(0.243, 0.6), (0.243, 0.625)], 8, GOLD, cap_bottom=False, cap_top=False).xf(S(1.0, 1, 1))
    torso = lathe([(0.17, 0.88), (0.19, 1.04), (0.205, 1.2), (0.18, 1.33), (0.09, 1.41)], 8, NAVY)
    b += torso.scale(1, 1, 0.74)
    b += box(0.05, 0.5, 0.025, LEATHER).rz(0.55).move(0.0, 0.92, 0.135)              # Schräggurt
    b += prism(0.045, 0.05, 0.1, 6, SKIN).move(0, 1.38, 0)                             # Hals
    b += lathe([(0.0, 1.27), (0.12, 1.29), (0.13, 1.36), (0.0, 1.42)], 6, BRONZE).scale(1, 1, 0.9).move(-0.2, 0, 0)  # Schulterplatte
    b += arm(-1, (-0.2, 1.32, 0), (-0.25, 1.07, -0.02), (-0.27, 0.84, 0.03), NAVY, BRONZE, SKIN)
    armm = arm(1, (0.2, 1.32, 0), (0.25, 1.07, 0.02), (0.27, 0.86, 0.09), NAVY, BRONZE, SKIN)  # Schwertarm, Drehpunkt Schulter
    b += head(SKIN, HAIR, EYE, y0=1.46)
    # Haare: Kappe ohne Gesicht, Pony, Seitensträhnen
    cap = lathe([(0.106, 1.53), (0.122, 1.6), (0.118, 1.68), (0.09, 1.74), (0.0, 1.775)], 8, HAIR)
    b += cut(cap, lambda p: p[2] > 0.04 and p[1] < 1.69)
    for i, (x, a) in enumerate([(-0.06, 0.35), (-0.02, 0.1), (0.03, -0.15), (0.07, -0.4)]):
        b += slab([(-0.03, 0), (0.03, 0), (0.0, -0.09)], 0.02, shade(HAIR, 1.05)).rz(a).move(x, 1.71, 0.095)
    for sx in (-1, 1):
        b += tube([(sx * 0.1, 1.68, 0.05), (sx * 0.115, 1.58, 0.06), (sx * 0.1, 1.5, 0.05)], [0.03, 0.025, 0.008], 5, HAIR)
    b += ring(0.095, 0.035, 8, SCARF).move(0, 1.4, 0)                                  # Schal um den Hals
    glow = octa(0.035, 0.055, STAR).move(0.07, 1.2, 0.152)                             # Sternsplitter auf der Brust
    glow += ring(0.046, 0.008, 8, GOLD).rx(PI / 2).move(0.07, 1.2, 0.15)
    hair = tube([(0, 1.71, -0.1), (0, 1.62, -0.17), (0, 1.48, -0.2), (0, 1.34, -0.19)], [0.055, 0.05, 0.035, 0.01], 6, HAIR)
    hair += ring(0.045, 0.012, 6, GOLD).move(0, 1.65, -0.14)
    scarf = double_sided(Mesh().quad((-0.05, 0, 0), (0.05, 0, 0), (0.07, -0.35, -0.05), (-0.02, -0.38, -0.05), SCARF)
                         .quad((-0.02, -0.38, -0.05), (0.07, -0.35, -0.05), (0.08, -0.52, -0.1), (0.0, -0.55, -0.12), shade(SCARF, 0.9)))
    scarf = scarf.move(-0.12, 1.38, -0.1)
    cape = cloth(0.44, 0.78, 4, 4, CAPE, curve=0.07, flare=0.55, col_bottom=shade(CAPE, 0.6)).rx(0.1).move(0, 1.36, -0.13)
    cape += Mesh().quad((-0.22, 1.36, -0.13), (0.22, 1.36, -0.13), (0.22, 1.37, -0.08), (-0.22, 1.37, -0.08), GOLD)
    # Waffe: Sternstahl-Klinge (Griff im Ursprung, Klinge entlang +Y)
    w = blade(0.78, 0.075, 0.02, STEEL, edge=rgb(0xF0F8FF)).move(0, 0.12, 0)
    w += box(0.2, 0.035, 0.05, GOLD, top=0.85).move(0, 0.09, 0)
    w += prism(0.018, 0.016, 0.11, 6, LEATHER).move(0, -0.02, 0)
    w += octa(0.028, 0.03, GOLD).move(0, -0.04, 0)
    wg = slab([(-0.008, 0.14), (0.008, 0.14), (0.004, 0.7), (-0.004, 0.7)], 0.024, STAR)
    return dict(body=b.vary(0.05, 1), arm=armm.vary(0.05, 5), hair=hair.vary(0.04, 2), scarf=scarf, cape=cape.vary(0.04, 3), glow=glow, weapon=w, weapon_glow=wg)


# ------------------------------------------------------------------ Seren als Schatten-Kriegerin
def schatten():
    CLOTH, CLOTH2, DARK = rgb(0x3A3050), rgb(0x2A223C), rgb(0x1C1626)
    LEATHER, METAL, TRIM = rgb(0x3A2E3A), rgb(0x5E5C70), rgb(0x8A5AC8)
    MASK, EYE, SHADOW, BLADE = rgb(0x1A1622), rgb(0x8AF0FF), rgb(0x9A6AFF), rgb(0x4A4660)
    b = Mesh()
    # Hüfte/Gürtel, Torso, Kopf mit Kapuze und Maske
    b += prism(0.17, 0.165, 0.09, 8, LEATHER).move(0, 0.8, 0)
    b += box(0.06, 0.06, 0.03, TRIM).move(0, 0.815, 0.16)
    skirt = lathe([(0.23, 0.56), (0.19, 0.7), (0.17, 0.86)], 8, CLOTH2, cap_bottom=False, cap_top=False)
    b += double_sided(cut(skirt, lambda p: p[2] > 0.1 and abs(p[0]) < 0.08))
    b += lathe([(0.17, 0.88), (0.19, 1.04), (0.2, 1.2), (0.17, 1.33), (0.09, 1.41)], 8, CLOTH).scale(1, 1, 0.74)
    b += box(0.05, 0.52, 0.025, LEATHER).rz(-0.55).move(0.0, 0.92, 0.135)       # Gurt
    for sx in (-1, 1):                                                            # Schulterplatten
        b += lathe([(0.0, 1.28), (0.11, 1.3), (0.12, 1.36), (0.0, 1.41)], 6, METAL).scale(1, 1, 0.9).move(sx * 0.2, 0, 0)
        b += slab([(-0.05, 0), (0.05, 0), (0.0, 0.1)], 0.02, TRIM).move(sx * 0.21, 1.38, 0.08)
    b += prism(0.05, 0.05, 0.1, 6, DARK).move(0, 1.38, 0)
    b += lathe([(0.0, 1.46), (0.095, 1.5), (0.105, 1.6), (0.095, 1.68), (0.0, 1.74)], 8, MASK).scale(1, 1, 0.96)
    hood = lathe([(0.13, 1.42), (0.15, 1.55), (0.14, 1.68), (0.1, 1.79), (0.03, 1.86), (0.0, 1.88)], 8, CLOTH)
    b += cut(hood, lambda p: p[2] > 0.06 and 1.47 < p[1] < 1.74)
    b += slab([(-0.03, 0), (0.03, 0), (0.0, -0.12)], 0.03, CLOTH).rx(0.3).move(0, 1.88, -0.06)  # Kapuzenspitze
    b += box(0.2, 0.08, 0.03, CLOTH2, top=0.8).move(0, 1.47, 0.1)                 # Tuch vor dem Mund
    glow = Mesh()
    for sx in (-1, 1):
        glow += box(0.036, 0.016, 0.01, EYE, top=0.6).rz(sx * 0.2).move(sx * 0.038, 1.6, 0.1)
    rune = []
    for i in range(8):
        a = PI / 2 + PI * i / 4
        rr = 0.05 if i % 2 == 0 else 0.022
        rune.append((math.cos(a) * rr, math.sin(a) * rr))
    glow += slab(rune, 0.01, SHADOW).move(0.0, 1.2, 0.155)
    # Beine einzeln (Drehpunkt Hüfte)
    def legm(x):
        l = Mesh()
        l += prism(0.068, 0.085, 0.54, 6, DARK).move(x, 0.3, 0)
        l += prism(0.078, 0.07, 0.32, 6, LEATHER).move(x, 0.0, 0)
        l += box(0.12, 0.07, 0.17, shade(LEATHER, 0.85), top=0.8).move(x, 0, 0.05)
        l += box(0.09, 0.08, 0.03, METAL, top=0.7).move(x, 0.42, 0.072)
        l += prism(0.082, 0.082, 0.035, 6, TRIM).move(x, 0.3, 0)
        return l
    # linker Arm einzeln (Drehpunkt Schulter)
    arml = arm(-1, (-0.2, 1.32, 0), (-0.25, 1.07, -0.02), (-0.27, 0.84, 0.03), CLOTH, METAL, DARK)
    armr = arm(1, (0.2, 1.32, 0), (0.25, 1.07, 0.02), (0.27, 0.86, 0.09), CLOTH, METAL, DARK)
    # zerfetzter Schattenumhang + Schal
    cape = cloth(0.46, 0.92, 5, 4, CLOTH2, curve=0.07, flare=0.6, col_bottom=DARK).rx(0.1).move(0, 1.37, -0.13)
    cape = cape.bend(lambda p: (p[0], p[1] + (0.1 * abs(math.sin(p[0] * 24)) if p[1] < 0.65 else 0), p[2]))
    cape += Mesh().quad((-0.22, 1.37, -0.13), (0.22, 1.37, -0.13), (0.22, 1.38, -0.08), (-0.22, 1.38, -0.08), TRIM)
    scarf = double_sided(Mesh().quad((-0.05, 0, 0), (0.05, 0, 0), (0.06, -0.3, -0.06), (-0.02, -0.34, -0.06), CLOTH)
                         .quad((-0.02, -0.34, -0.06), (0.06, -0.3, -0.06), (0.04, -0.5, -0.12), (0.0, -0.56, -0.14), DARK)).move(0.1, 1.42, -0.1)
    # Schattenklinge (Griff im Ursprung)
    w = blade(0.8, 0.07, 0.02, BLADE, edge=rgb(0x2A2638)).move(0, 0.12, 0)
    w += box(0.18, 0.035, 0.05, DARK, top=0.7).move(0, 0.09, 0)
    w += slab([(-0.09, 0), (-0.05, 0.05), (0.0, 0.0)], 0.03, TRIM).move(0, 0.1, 0) + slab([(0.0, 0.0), (0.05, 0.05), (0.09, 0)], 0.03, TRIM).move(0, 0.1, 0)
    w += prism(0.018, 0.016, 0.11, 6, LEATHER).move(0, -0.02, 0)
    w += octa(0.026, 0.03, TRIM).move(0, -0.04, 0)
    wg = slab([(-0.01, 0.13), (0.01, 0.13), (0.006, 0.72), (0.0, 0.84), (-0.006, 0.72)], 0.026, SHADOW)
    return dict(body=b.vary(0.05, 1), arm=armr.vary(0.05, 5), arm_l=arml.vary(0.05, 6), leg_l=legm(-0.1).vary(0.05, 7), leg_r=legm(0.1).vary(0.05, 8),
                scarf=scarf, cape=cape.vary(0.04, 3), glow=glow, weapon=w, weapon_glow=wg)


# ------------------------------------------------------------------ Götter (Maßstab ~1,3 × Mensch)
def torso(col, y0, y1, w=0.19, depth=0.74, chest=1.0):
    prof = [(w * 0.9, y0), (w * chest, y0 + (y1 - y0) * 0.55), (w * 0.95 * chest, y0 + (y1 - y0) * 0.85), (w * 0.45, y1)]
    return lathe(prof, 8, col).scale(1, 1, depth)


def robe(cols, y_top=0.9, r_top=0.17, r_bot=0.4, rings=4, y_bot=0.0, open_front=False):
    prof = []
    for k in range(rings + 1):
        t = k / rings
        prof.append((r_bot + (r_top - r_bot) * (t ** 0.8), y_bot + (y_top - y_bot) * t))
    m = lathe(prof, 10, cols[0], colors=[cols[min(k, len(cols) - 1)] for k in range(rings)], cap_bottom=False, cap_top=False)
    return double_sided(m) if open_front else m


def ignara():
    CRIM, EMBER, GOLD, SKIN = rgb(0x9A1E24), rgb(0xE0502A), rgb(0xF0C050), rgb(0xB87A58)
    FIRE0, FIRE1, EYE = rgb(0xFF5A1E), rgb(0xFFE070), rgb(0xFFB020)
    b = Mesh()
    b += robe([shade(CRIM, 0.7), CRIM, CRIM, shade(CRIM, 1.1)], y_top=0.92, r_top=0.16, r_bot=0.38)
    b += lathe([(0.385, 0.06), (0.385, 0.1)], 10, GOLD, cap_bottom=False, cap_top=False)
    b += prism(0.16, 0.16, 0.06, 8, GOLD).move(0, 0.9, 0)
    b += torso(CRIM, 0.92, 1.42, w=0.18)
    b += box(0.2, 0.32, 0.02, EMBER, top=0.4).move(0, 1.02, 0.135).recolor(lambda c, p: mix(EMBER, GOLD, (p[1] - 1.02) / 0.32))
    b += ring(0.13, 0.018, 10, GOLD).move(0, 1.38, 0)                                   # Halsreif
    b += prism(0.045, 0.05, 0.1, 6, SKIN).move(0, 1.4, 0)
    b += arm(-1, (-0.19, 1.34, 0), (-0.26, 1.1, 0.0), (-0.3, 0.88, 0.05), SKIN, SKIN, SKIN)
    b += arm(1, (0.19, 1.34, 0), (0.33, 1.2, 0.1), (0.36, 1.42, 0.2), SKIN, SKIN, SKIN)  # Hand erhoben
    for x, y, z in ((-0.28, 1.0, 0.03), (0.35, 1.32, 0.16)):
        b += ring(0.05, 0.012, 8, GOLD).move(x, y, z)
    b += head(SKIN, rgb(0x5A1A10), EYE, y0=1.48)
    b += lathe([(0.112, 1.64), (0.115, 1.68)], 10, GOLD, cap_bottom=False, cap_top=False)  # Stirnreif
    b += octa(0.02, 0.03, rgb(0xFF3030)).move(0, 1.67, 0.115)
    fx = Mesh()
    for i in range(9):                                                                   # Flammenhaar
        a = -1.0 + 2.0 * i / 8
        f = flame(0.32 + 0.1 * math.cos(a * 2), 0.07, FIRE0, FIRE1).rx(-0.5).rz(a * 0.6).move(math.sin(a) * 0.07, 1.68, -0.03 - abs(a) * 0.02)
        fx += f
    for i in range(12):                                                                  # brennender Saum
        a = 2 * PI * i / 12
        fx += flame(0.16, 0.06, FIRE0, FIRE1).ry(a).move(math.sin(a) * 0.37, 0.0, math.cos(a) * 0.37)
    orbit = ico(0.08, FIRE1).move(0.37, 1.58, 0.22) + ico(0.12, shade(FIRE0, 1.1)).scale(1, 1.3, 1).move(0.37, 1.6, 0.22)
    return dict(body=b.vary(0.05, 11), fx=fx, glow=orbit)


def thalor():
    STEEL, BLUE, SLATE, GOLD = rgb(0xB8C4DA), rgb(0x2A5AB0), rgb(0x34405E), rgb(0xE8C860)
    SKIN, BEARD, BOLT, EYE = rgb(0xD8A888), rgb(0xD8DCE8), rgb(0xFFF07A), rgb(0x60D0FF)
    b = Mesh()
    for x in (-0.13, 0.13):
        b += leg(x, 0.86, SLATE, STEEL, STEEL, knee=STEEL, r=1.25)
    for i in range(10):                                                                  # Plattenrock
        a = 2 * PI * i / 10
        b += box(0.09, 0.26, 0.02, BLUE if i % 2 else STEEL, top=1.1).rx(0.12).move(0, -0.26, 0.2).ry(a).move(0, 0.92, 0)
    b += prism(0.21, 0.21, 0.08, 10, GOLD).move(0, 0.86, 0)
    b += torso(STEEL, 0.92, 1.46, w=0.25, chest=1.08).recolor(lambda c, p: BLUE if abs(p[0]) < 0.06 else c)
    b += prism(0.06, 0.065, 0.1, 6, SKIN).move(0, 1.42, 0)
    for sx in (-1, 1):                                                                   # Schulterpanzer
        b += lathe([(0.0, 1.3), (0.15, 1.33), (0.16, 1.4), (0.0, 1.47)], 6, STEEL).scale(1.1, 1, 1).move(sx * 0.29, 0, 0)
        b += bolt(0.14, 0.09, GOLD).move(sx * 0.29, 1.34, 0.12)
    b += arm(-1, (-0.27, 1.36, 0), (-0.34, 1.1, -0.02), (-0.36, 0.87, 0.05), SKIN, STEEL, SKIN, rs=1.3)
    b += arm(1, (0.27, 1.36, 0), (0.36, 1.12, 0.06), (0.36, 0.98, 0.25), SKIN, STEEL, SKIN, rs=1.3)
    b += head(SKIN, BEARD, EYE, y0=1.5, s=1.05)
    b += lathe([(0.0, 1.47), (0.09, 1.5), (0.1, 1.57), (0.0, 1.6)], 6, BEARD).scale(1, 1, 0.6).move(0, -0.06, 0.07)  # Bart
    b += tube([(0, 1.5, 0.1), (0, 1.38, 0.13), (0, 1.28, 0.1)], [0.06, 0.04, 0.01], 5, BEARD)
    cap = lathe([(0.112, 1.6), (0.126, 1.67), (0.11, 1.75), (0.0, 1.81)], 8, BEARD)
    b += cut(cap, lambda p: p[2] > 0.05 and p[1] < 1.73)
    b += lathe([(0.118, 1.69), (0.122, 1.74)], 10, GOLD, cap_bottom=False, cap_top=False)
    cape = cloth(0.6, 1.25, 5, 5, SLATE, curve=0.1, flare=0.6, col_bottom=rgb(0x1A2034)).move(0, 1.44, -0.2)
    cape = cape.bend(lambda p: (p[0], p[1] + (0.06 * math.sin(p[0] * 30) if p[1] < 0.3 else 0), p[2]))
    fx = Mesh()
    for i in range(5):                                                                   # Blitzkrone
        a = -0.8 + 0.4 * i
        fx += bolt(0.22 - abs(a) * 0.08, 0.1, BOLT).rz(-a * 0.5).move(math.sin(a) * 0.1, 1.74, math.cos(a) * 0.06)
    w = prism(0.022, 0.022, 2.1, 6, rgb(0x5A4A3A)).move(0, -0.9, 0)                    # Speerschaft
    w += prism(0.035, 0.035, 0.06, 6, GOLD).move(0, 1.12, 0)
    wg = bolt(0.42, 0.2, BOLT).move(0, 1.16, 0)
    hold = lambda m: m.rx(0.15).move(0.37, 0.92, 0.26)
    return dict(body=b.vary(0.05, 12), cape=cape.vary(0.04, 13), fx=fx, weapon=hold(w), weapon_glow=hold(wg))


def nerys():
    TEAL, DEEP, FOAM, PEARL = rgb(0x1E9AA8), rgb(0x123C78), rgb(0xBFF4F0), rgb(0xF4ECE0)
    SKIN, HAIR, WATER, EYE, CORAL = rgb(0xE6C2B0), rgb(0x2AB8B0), rgb(0x7AE8FF), rgb(0x1A6A90), rgb(0xFF8A7A)
    b = Mesh()
    b += robe([DEEP, shade(DEEP, 1.2), TEAL, TEAL], y_top=0.94, r_top=0.15, r_bot=0.36, rings=4)
    for i in range(14):                                                                  # Wellensaum
        a = 2 * PI * i / 14
        wave = slab([(-0.08, 0), (0.08, 0), (0.06, 0.08), (0.0, 0.13), (-0.03, 0.07)], 0.02, FOAM)
        b += wave.ry(a).move(math.sin(a) * 0.36, 0.0, math.cos(a) * 0.36)
    b += prism(0.15, 0.15, 0.05, 8, PEARL).move(0, 0.92, 0)
    b += torso(TEAL, 0.94, 1.42, w=0.17)
    for i in range(7):                                                                   # Perlenkette
        a = -0.9 + 0.3 * i
        b += ico(0.018, PEARL).move(math.sin(a) * 0.13, 1.37 - math.cos(a * 1.2) * 0.04, math.cos(a) * 0.1)
    b += prism(0.045, 0.05, 0.1, 6, SKIN).move(0, 1.4, 0)
    b += arm(-1, (-0.18, 1.34, 0), (-0.27, 1.12, 0.04), (-0.32, 0.95, 0.15), TEAL, SKIN, SKIN)
    b += arm(1, (0.18, 1.34, 0), (0.27, 1.1, 0.02), (0.3, 0.9, 0.1), TEAL, SKIN, SKIN)
    b += head(SKIN, HAIR, EYE, y0=1.48)
    cap = lathe([(0.108, 1.55), (0.124, 1.63), (0.12, 1.71), (0.09, 1.77), (0.0, 1.8)], 8, HAIR)
    b += cut(cap, lambda p: p[2] > 0.04 and p[1] < 1.71)
    for i in range(5):                                                                   # Muschelkrone
        a = -0.7 + 0.35 * i
        fan = slab([(-0.05, 0), (0.05, 0), (0.07, 0.1), (0.0, 0.14), (-0.07, 0.1)], 0.015, PEARL if i % 2 else CORAL)
        b += fan.rx(-0.2).rz(-a * 0.6).move(math.sin(a) * 0.1, 1.72, math.cos(a) * 0.07)
    hair = Mesh()
    for i in range(6):                                                                   # langes Wellenhaar
        x = -0.1 + 0.04 * i
        pts = [(x, 1.72, -0.06), (x * 1.3, 1.5, -0.14), (x * 1.5 + 0.03 * math.sin(i), 1.25, -0.16), (x * 1.4, 1.0, -0.12 - 0.02 * i % 3)]
        hair += tube(pts, [0.04, 0.045, 0.035, 0.005], 5, HAIR if i % 2 else shade(HAIR, 0.85))
    w = tube([(0, -0.9, 0), (0, 0.3, 0), (0.03, 0.9, 0), (0.12, 1.08, 0), (0.08, 1.2, 0), (-0.02, 1.15, 0)],
             [0.02, 0.022, 0.022, 0.018, 0.014, 0.008], 6, rgb(0xD8E8F0))
    wg = ico(0.08, WATER).move(0.04, 1.05, 0)
    hold = lambda m: m.rz(-0.08).move(-0.33, 0.95, 0.2)
    orbit = Mesh()
    for i in range(10):
        a = 2 * PI * i / 10
        orbit += ico(0.035 + 0.01 * (i % 3), WATER).move(math.sin(a) * 0.45, 0.6 + 0.08 * math.sin(a * 3), math.cos(a) * 0.45)
    return dict(body=b.vary(0.05, 14), hair=hair.vary(0.04, 15), weapon=hold(w), weapon_glow=hold(wg), orbit_glow=orbit)


def vexa():
    VIOLET, DARK, MASK, SILVER = rgb(0x3A2260), rgb(0x1A1228), rgb(0xEDE6F0), rgb(0xA8A0C8)
    GLOWV, SKIN = rgb(0xC070FF), rgb(0xC8B0D0)
    b = Mesh()
    for x in (-0.08, 0.08):
        b += leg(x, 0.84, DARK, VIOLET, DARK, r=0.85)
    b += prism(0.15, 0.15, 0.06, 8, SILVER).move(0, 0.84, 0)
    b += torso(DARK, 0.88, 1.4, w=0.16)
    b += arm(-1, (-0.17, 1.32, 0), (-0.25, 1.1, 0.06), (-0.3, 0.92, 0.18), DARK, VIOLET, SKIN, rs=0.9)
    b += arm(1, (0.17, 1.32, 0), (0.26, 1.08, -0.02), (0.31, 0.85, 0.02), DARK, VIOLET, SKIN, rs=0.9)
    # Kapuze + Maske
    hood = lathe([(0.15, 1.36), (0.16, 1.5), (0.15, 1.66), (0.1, 1.78), (0.03, 1.86), (0.0, 1.88)], 8, VIOLET)
    b += cut(hood, lambda p: p[2] > 0.07 and 1.42 < p[1] < 1.72)
    b += lathe([(0.0, 1.44), (0.09, 1.47), (0.11, 1.56), (0.1, 1.66), (0.0, 1.7)], 8, rgb(0x0A0610)).scale(1, 1, 0.9)  # Schatten im Gesicht
    b += slab([(-0.08, 0.0), (0.08, 0.0), (0.09, 0.14), (0.0, 0.2), (-0.09, 0.14)], 0.02, MASK).move(0, 1.48, 0.1)
    cloak = cloth(0.52, 1.25, 5, 6, VIOLET, curve=0.12, flare=0.7, col_bottom=DARK).move(0, 1.42, -0.15)
    cloak = cloak.bend(lambda p: (p[0], p[1] + (0.09 * abs(math.sin(p[0] * 22)) if p[1] < 0.35 else 0), p[2]))
    glow = Mesh()
    for sx in (-1, 1):
        glow += box(0.03, 0.014, 0.01, GLOWV, top=0.5).rz(sx * 0.25).move(sx * 0.035, 1.58, 0.112)
    w = Mesh()
    for side, (x, y, z), rot in ((1, (0.31, 0.83, 0.04), 2.6), (-1, (-0.3, 0.9, 0.2), 0.4)):
        dag = slab([(0, 0), (0.05, 0.12), (0.06, 0.3), (0.0, 0.42), (0.03, 0.28), (0.02, 0.12)], 0.015, SILVER)
        dag += box(0.08, 0.02, 0.03, VIOLET).move(0.01, -0.01, 0) + prism(0.012, 0.012, 0.08, 5, DARK).move(0.01, -0.09, 0)
        w += dag.rz(-0.2 * side).rx(rot).move(x, y, z)
    orbit = Mesh()
    for i in range(6):
        a = 2 * PI * i / 6
        orbit += octa(0.04, 0.12, rgb(0x6A3AA0)).rz(0.4).move(math.sin(a) * 0.5, 0.4 + 0.3 * (i % 3), math.cos(a) * 0.5)
    return dict(body=b.vary(0.05, 16), cape=cloak.vary(0.04, 17), glow=glow, weapon=w, orbit=orbit)


def korrin():
    SKIN, BEARD, LEATHER, STONE = rgb(0x9A6440), rgb(0xD0782A), rgb(0x6A4028), rgb(0x7A7480)
    IRON, LAVA, LAVA2, EYE, BRASS = rgb(0x4A4650), rgb(0xFF6A1A), rgb(0xFFC040), rgb(0xFFB040), rgb(0xC89A40)
    b = Mesh()
    for x in (-0.16, 0.16):
        b += leg(x, 0.7, LEATHER, BRASS, IRON, knee=IRON, r=1.5)
    b += prism(0.29, 0.28, 0.12, 10, LEATHER).move(0, 0.66, 0)
    b += box(0.16, 0.12, 0.05, BRASS).move(0, 0.66, 0.27)
    b += lathe([(0.29, 0.74), (0.33, 0.95), (0.33, 1.15), (0.27, 1.3), (0.12, 1.38)], 10, SKIN).scale(1.1, 1, 0.82)
    b += box(0.42, 0.72, 0.03, LEATHER, top=0.8).move(0, 0.42, 0.25)                  # Schürze
    b += arm(-1, (-0.33, 1.25, 0), (-0.45, 1.0, 0.0), (-0.46, 0.76, 0.08), SKIN, LEATHER, SKIN, rs=1.9)
    b += arm(1, (0.33, 1.25, 0), (0.47, 1.05, 0.08), (0.42, 0.9, 0.3), SKIN, LEATHER, SKIN, rs=1.9)
    for sx in (-1, 1):                                                                   # Felsschultern
        b += ico(0.17, STONE).jitter(0.03, 5 + sx).scale(1.1, 0.8, 1).move(sx * 0.36, 1.32, 0)
        b += ico(0.09, shade(STONE, 0.9)).jitter(0.02, 7 + sx).move(sx * 0.45, 1.42, -0.06)
    b += head(SKIN, BEARD, EYE, y0=1.32, s=1.15)
    b += lathe([(0.0, 1.26), (0.11, 1.32), (0.12, 1.42), (0.0, 1.47)], 7, BEARD).scale(1.1, 1, 0.7).move(0, -0.05, 0.08)
    for x in (-0.05, 0.05):                                                              # Zopfbart
        b += tube([(x, 1.3, 0.13), (x * 1.2, 1.12, 0.17), (x, 0.96, 0.18)], [0.035, 0.03, 0.012], 5, BEARD)
        b += ring(0.03, 0.01, 6, BRASS).move(x * 1.15, 1.1, 0.17)
    glow = Mesh()
    for i in range(3):                                                                   # Runen auf dem Kopf
        glow += box(0.015, 0.06, 0.01, LAVA2).rx(-0.5).move(-0.05 + 0.05 * i, 1.6, 0.06)
    for sx in (-1, 1):
        glow += Mesh().tri((sx * 0.3, 1.25, 0.11), (sx * 0.36, 1.35, 0.13), (sx * 0.42, 1.26, 0.12), LAVA)
    hammer = prism(0.03, 0.03, 1.15, 6, rgb(0x5A3A26)).move(0, -0.2, 0)
    head_ = box(0.42, 0.24, 0.24, IRON, top=0.92).move(0, 0.9, 0)
    hammer += head_ + box(0.44, 0.04, 0.26, BRASS).move(0, 1.0, 0)
    hg = box(0.43, 0.02, 0.245, LAVA).move(0, 0.95, 0) + box(0.02, 0.25, 0.25, LAVA2).move(0, 0.895, 0)
    hold = lambda m: m.rx(-0.25).rz(0.15).move(0.44, 0.7, 0.32)
    return dict(body=b.vary(0.06, 18), glow=glow, weapon=hold(hammer), weapon_glow=hold(hg))


def lyra():
    WHITE, PALE, GOLD, SILVER = rgb(0xF4F2FF), rgb(0xD8D4F0), rgb(0xF4CC60), rgb(0xE8ECF8)
    SKIN, STAR, EYE = rgb(0xF2D6C6), rgb(0xFFF2B0), rgb(0x8A70D0)
    b = Mesh()
    b += robe([PALE, WHITE, WHITE, WHITE], y_top=0.98, r_top=0.15, r_bot=0.3, rings=4, y_bot=0.18)
    b += lathe([(0.3, 0.18), (0.08, 0.05), (0.0, 0.02)], 10, PALE, cap_bottom=False, cap_top=False)  # schwebend, spitz
    b += lathe([(0.302, 0.2), (0.302, 0.24)], 10, GOLD, cap_bottom=False, cap_top=False)
    b += prism(0.15, 0.15, 0.05, 8, GOLD).move(0, 0.96, 0)
    b += torso(WHITE, 0.98, 1.44, w=0.165)
    b += star(0.05, 0.02, GOLD).move(0, 1.22, 0.13)
    b += prism(0.045, 0.05, 0.1, 6, SKIN).move(0, 1.42, 0)
    for sx in (-1, 1):                                                                   # weite Ärmel
        b += lathe([(0.05, 0.0), (0.09, -0.2), (0.12, -0.42)], 6, WHITE, cap_bottom=False, cap_top=False).rz(sx * 0.35).move(sx * 0.19, 1.36, 0)
    b += arm(-1, (-0.19, 1.36, 0), (-0.3, 1.18, 0.1), (-0.32, 1.3, 0.25), WHITE, SKIN, SKIN)
    b += arm(1, (0.19, 1.36, 0), (0.3, 1.12, 0.05), (0.3, 1.0, 0.2), WHITE, SKIN, SKIN)
    b += head(SKIN, SILVER, EYE, y0=1.5)
    cap = lathe([(0.108, 1.57), (0.122, 1.65), (0.118, 1.73), (0.09, 1.79), (0.0, 1.82)], 8, SILVER)
    b += cut(cap, lambda p: p[2] > 0.04 and p[1] < 1.73)
    hair = Mesh()
    for i in range(5):
        x = -0.09 + 0.045 * i
        hair += tube([(x, 1.74, -0.06), (x * 1.4, 1.45, -0.14), (x * 1.6, 1.12, -0.12), (x * 1.5, 0.85, -0.08)],
                     [0.04, 0.045, 0.035, 0.005], 5, SILVER if i % 2 else shade(SILVER, 0.88))
    glow = ring(0.3, 0.02, 20, GOLD).rx(PI / 2).move(0, 1.66, -0.2)                   # Heiligenschein
    for i in range(8):
        a = 2 * PI * i / 8
        glow += star(0.06, 0.02, STAR, n=4).move(math.cos(a) * 0.3, 1.66 + math.sin(a) * 0.3, -0.2)
    w = prism(0.018, 0.018, 1.7, 6, GOLD).move(0, -0.75, 0) + ring(0.08, 0.012, 10, GOLD).rx(PI / 2).move(0, 1.03, 0)
    wg = star(0.12, 0.04, STAR).move(0, 1.03, 0) + ico(0.04, rgb(0xFFFFFF)).move(0, 1.03, 0)
    hold = lambda m: m.rz(0.05).move(0.31, 0.92, 0.22)
    orbit = Mesh()
    for i in range(3):
        a = 2 * PI * i / 3
        orbit += octa(0.035, 0.06, STAR).move(math.sin(a) * 0.55, 1.0 + 0.2 * i, math.cos(a) * 0.55)
    return dict(body=b.vary(0.04, 19), hair=hair.vary(0.03, 20), glow=glow, weapon=hold(w), weapon_glow=hold(wg), orbit_glow=orbit)


def sylv():
    BARK, MOSS, LEAF1, LEAF2 = rgb(0x5A3E2A), rgb(0x6A7A44), rgb(0x3E8A3A), rgb(0x8AC04A)
    SKIN, EYE, SPIRIT, ANT = rgb(0x8A9A6A), rgb(0xB0FF70), rgb(0x9AFFC0), rgb(0xD8C8A8)
    b = Mesh()
    for x in (-0.09, 0.09):
        b += leg(x, 0.88, BARK, MOSS, BARK, r=0.9)
    for i in range(16):                                                                  # Blätterrock
        a = 2 * PI * i / 16
        b += leaf(0.34, 0.12, LEAF1 if i % 2 else LEAF2).rx(PI + 0.15).move(0, 0.94, 0.18).ry(a)
    b += prism(0.16, 0.16, 0.06, 8, BARK).move(0, 0.88, 0)
    b += torso(MOSS, 0.92, 1.44, w=0.17)
    b += prism(0.045, 0.05, 0.1, 6, SKIN).move(0, 1.42, 0)
    b += arm(-1, (-0.18, 1.35, 0), (-0.3, 1.14, 0.08), (-0.38, 1.02, 0.24), SKIN, BARK, SKIN)
    b += arm(1, (0.18, 1.35, 0), (0.26, 1.1, -0.02), (0.29, 0.88, 0.04), SKIN, BARK, SKIN)
    b += head(SKIN, BARK, EYE, y0=1.5, ears=True)
    cap = lathe([(0.108, 1.57), (0.12, 1.65), (0.116, 1.73), (0.09, 1.79), (0.0, 1.81)], 8, rgb(0x3A2A1E))
    b += cut(cap, lambda p: p[2] > 0.04 and p[1] < 1.73)
    for sx in (-1, 1):                                                                   # Geweih
        main = [(sx * 0.07, 1.76, 0), (sx * 0.15, 1.9, -0.02), (sx * 0.22, 2.05, 0.0), (sx * 0.3, 2.18, 0.03)]
        b += tube(main, [0.025, 0.02, 0.016, 0.006], 5, ANT)
        b += tube([(sx * 0.15, 1.9, -0.02), (sx * 0.12, 2.02, 0.06), (sx * 0.1, 2.1, 0.1)], [0.014, 0.01, 0.004], 4, ANT)
        b += tube([(sx * 0.22, 2.05, 0.0), (sx * 0.3, 2.08, -0.06), (sx * 0.37, 2.1, -0.1)], [0.013, 0.01, 0.004], 4, ANT)
    mantle = Mesh()
    for i in range(14):                                                                  # Blättermantel
        a = PI * 0.25 + PI * 1.5 * i / 13
        mantle += leaf(0.5 + 0.1 * math.sin(i), 0.16, LEAF2 if i % 3 else LEAF1).rx(PI + 0.25).move(0, 1.4, 0.16).ry(a)
    glow = Mesh()
    for sx in (-1, 1):
        glow += box(0.028, 0.02, 0.01, EYE).move(sx * 0.038, 1.625, 0.106)
    # Bogen aus lebendem Holz
    pts = [(0, -0.55, 0.0), (0.06, -0.3, 0), (0.08, 0, 0), (0.06, 0.3, 0), (0, 0.55, 0)]
    w = tube(pts, [0.01, 0.018, 0.022, 0.018, 0.01], 5, BARK)
    w += leaf(0.08, 0.04, LEAF2).move(0.06, 0.32, 0.01) + leaf(0.07, 0.04, LEAF1).rz(PI).move(0.06, -0.3, 0.01)
    wg = box(0.006, 1.1, 0.006, SPIRIT).move(0, -0.55, 0)
    hold = lambda m: m.ry(PI / 2).rz(0.1).move(-0.4, 1.02, 0.26)
    # Geistwolf
    wolf = Mesh()
    wolf += box(0.2, 0.18, 0.5, SPIRIT, top=0.9).move(0, 0.32, 0)
    wolf += box(0.16, 0.14, 0.18, SPIRIT, top=0.85).move(0, 0.46, 0.3)
    wolf += box(0.09, 0.07, 0.12, SPIRIT, top=0.7).move(0, 0.47, 0.44)
    for sx in (-1, 1):
        wolf += octa(0.03, 0.05, SPIRIT).move(sx * 0.05, 0.62, 0.28)
        for z in (-0.17, 0.17):
            wolf += prism(0.028, 0.022, 0.34, 5, SPIRIT).move(sx * 0.07, 0.0, z)
    wolf += tube([(0, 0.4, -0.25), (0, 0.48, -0.42), (0, 0.42, -0.58)], [0.05, 0.04, 0.0], 5, SPIRIT)
    wolf = wolf.ry(-0.5).move(0.62, 0, 0.25)
    return dict(body=b.vary(0.05, 21), cape=mantle.vary(0.06, 22), glow=glow, weapon=hold(w), weapon_glow=hold(wg), pet=wolf)


def scaled(fn, k):
    def g():
        return {p: m.scale(k) for p, m in fn().items()}
    return g


ALL = {"seren": schatten, "ignara": scaled(ignara, 1.32), "thalor": scaled(thalor, 1.32), "nerys": scaled(nerys, 1.3),
       "vexa": scaled(vexa, 1.28), "korrin": scaled(korrin, 1.38), "lyra": scaled(lyra, 1.3), "sylv": scaled(sylv, 1.3)}


if __name__ == "__main__":
    out = os.path.join(HERE, "..", "data")
    os.makedirs(out, exist_ok=True)
    total = 0
    for name, fn in ALL.items():
        for part, m in fn().items():
            nv, nt = m.save(os.path.join(out, "%s_%s.fmesh" % (name, part)))
            total += nt
            print("%-22s %5d Vertices %5d Dreiecke" % (name + "_" + part, nv, nt))
    print("gesamt", total, "Dreiecke")
