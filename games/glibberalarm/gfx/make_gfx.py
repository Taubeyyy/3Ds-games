# Goo Alert - Grafik-Generator
# Erzeugt Sprites, Karten-Hintergruende, Titelbild, .t3s-Dateien und source/gfx_idx.h
# Nutzung: python3 gfx/make_gfx.py [desktop-ausgabeordner]
import json, math, os, random, struct, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageChops

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(ROOT, "..", "..", "tools"))
from artlib import radial, vgrad, logo_text, paste_center, tint, FONT_BLACK, FONT_XBOLD

SS = 4  # Supersampling fuer Sprites
PNG = os.path.join(HERE, "png")
os.makedirs(os.path.join(PNG, "spr"), exist_ok=True)
PATH_W = 11.0

# ======================================================================== Hilfen
def canvas(w, h):
    return Image.new("RGBA", (int(w * SS), int(h * SS)), (0, 0, 0, 0))


def P(v):
    return v * SS


def ball(img, cx, cy, rx, ry, col, light=0.55, dark=0.45, hl=True, ss=SS):
    """Schattierte Ellipse (Licht von oben links)"""
    w, h = int(rx * 2 * ss) + 2, int(ry * 2 * ss) + 2
    lt = tuple(min(255, int(c + (255 - c) * light)) for c in col)
    dk = tuple(int(c * (1 - dark)) for c in col)
    g = radial(w, h, w * 0.35, h * 0.3, max(w, h) * 0.85, [(0, lt + (255,)), (0.5, tuple(col) + (255,)), (1, dk + (255,))])
    m = Image.new("L", (w, h), 0)
    ImageDraw.Draw(m).ellipse([0, 0, w - 1, h - 1], fill=255)
    g.putalpha(m)
    img.alpha_composite(g, (int(cx * ss - w / 2), int(cy * ss - h / 2)))
    if hl:
        d = ImageDraw.Draw(img)
        d.ellipse([P(cx - rx * 0.55), P(cy - ry * 0.7), P(cx - rx * 0.05), P(cy - ry * 0.3)], fill=(255, 255, 255, 110))


def poly(img, pts, col):
    ImageDraw.Draw(img).polygon([(P(x), P(y)) for x, y in pts], fill=col)


def line(img, pts, col, w):
    ImageDraw.Draw(img).line([(P(x), P(y)) for x, y in pts], fill=col, width=max(1, int(P(w))), joint="curve")


def ell(img, cx, cy, rx, ry, col):
    ImageDraw.Draw(img).ellipse([P(cx - rx), P(cy - ry), P(cx + rx), P(cy + ry)], fill=col)


def finish(img, w, h, outline=(20, 14, 30), ow=1):
    """Herunterskalieren + dunkle Kontur"""
    small = img.resize((int(w), int(h)), Image.LANCZOS)
    if not outline:
        return small
    a = small.getchannel("A")
    dil = a.filter(ImageFilter.MaxFilter(ow * 2 + 1))
    out = Image.new("RGBA", small.size, outline + (0,))
    out.putalpha(dil.point(lambda v: min(255, int(v * 0.95))))
    out.alpha_composite(small)
    return out


def eyes(img, cx, cy, dx, size, look=(1, 0), spread=None, pupil=0.5):
    spread = spread if spread is not None else size * 1.2
    for k in (-1, 1):
        ex, ey = cx + dx, cy + k * spread
        ell(img, ex, ey, size, size, (255, 255, 255, 255))
        ell(img, ex + look[0] * size * 0.35, ey + look[1] * size * 0.35, size * pupil, size * pupil, (20, 16, 26, 255))
        ell(img, ex + look[0] * size * 0.35 - size * 0.18, ey - size * 0.2, size * 0.18, size * 0.18, (255, 255, 255, 255))


SPRITES = []  # (name, Image)


def add(name, im):
    SPRITES.append((name, im))


# ======================================================================== Tuerme (Blick nach rechts)
TS = 30  # Spritegroesse
C = TS / 2


def base_disc(img, col=(120, 130, 150), rim=(200, 210, 230)):
    ell(img, C + 0.6, C + 1.2, 12.6, 12.6, (0, 0, 0, 90))
    ball(img, C, C, 12.5, 12.5, col, light=0.3, dark=0.5, hl=False)
    ImageDraw.Draw(img).ellipse([P(C - 12.5), P(C - 12.5), P(C + 12.5), P(C + 12.5)], outline=rim + (255,), width=int(P(1.1)))


def t_igel():
    im = canvas(TS, TS)
    base_disc(im, (110, 140, 90), (180, 220, 140))
    for k in range(15):
        a = math.radians(105 + k * 10.7)
        tip = (C - 1 + math.cos(a) * 11.5, C + math.sin(a) * 11.5)
        b1 = (C - 1 + math.cos(a - 0.22) * 5.5, C + math.sin(a - 0.22) * 5.5)
        b2 = (C - 1 + math.cos(a + 0.22) * 5.5, C + math.sin(a + 0.22) * 5.5)
        poly(im, [b1, tip, b2], (70, 44, 26, 255) if k % 2 else (100, 66, 40, 255))
        poly(im, [((tip[0] * 3 + b1[0]) / 4, (tip[1] * 3 + b1[1]) / 4), tip, ((tip[0] * 3 + b2[0]) / 4, (tip[1] * 3 + b2[1]) / 4)], (230, 210, 180, 255))
    ball(im, C - 1, C, 7.5, 7, (150, 100, 60))
    ball(im, C + 4.5, C, 4.8, 4.3, (240, 214, 170), light=0.4, dark=0.25)
    ell(im, C + 9, C, 1.5, 1.5, (30, 18, 12, 255))
    eyes(im, C, C, 4.2, 1.5, spread=2.4, pupil=0.7)
    line(im, [(C + 3, C + 6.5), (C + 12, C + 6.5)], (60, 40, 25, 255), 1.1)
    return finish(im, TS, TS)


def t_krabbe():
    im = canvas(TS, TS)
    base_disc(im, (90, 100, 120), (170, 180, 200))
    for k in (-1, 1):
        for j in range(3):
            x = C - 3 + j * 2.5
            line(im, [(x, C + k * 4), (x - 2.5, C + k * 9.5)], (170, 40, 30, 255), 1.3)
    line(im, [(C, C), (C + 13, C)], (50, 52, 62, 255), 4.2)
    line(im, [(C + 10, C), (C + 13.5, C)], (90, 92, 104, 255), 4.8)
    ell(im, C + 13, C, 1.4, 1.4, (20, 20, 26, 255))
    for k in (-1, 1):
        ball(im, C + 6, C + k * 7.5, 3.4, 3.1, (220, 70, 50))
        poly(im, [(C + 7.5, C + k * 6.5), (C + 10.5, C + k * 6.8), (C + 8, C + k * 7.8)], (40, 10, 10, 255))
    ball(im, C - 1, C, 8, 6.5, (225, 75, 55))
    for k in (-1, 1):
        line(im, [(C + 3, C + k * 2), (C + 5.5, C + k * 2.8)], (150, 40, 30, 255), 1)
        ell(im, C + 6, C + k * 2.9, 1.6, 1.6, (255, 255, 255, 255))
        ell(im, C + 6.5, C + k * 2.9, 0.8, 0.8, (20, 16, 26, 255))
    return finish(im, TS, TS)


def t_pinguin():
    im = canvas(TS, TS)
    base_disc(im, (150, 200, 230), (230, 250, 255))
    for k in (-1, 1):
        ball(im, C - 1, C + k * 7, 3.5, 2, (40, 44, 58), hl=False)
    ball(im, C - 0.5, C, 8, 7.6, (44, 48, 64))
    ball(im, C + 2.5, C, 5.2, 5.6, (245, 248, 255), light=0.2, dark=0.15)
    poly(im, [(C + 6.5, C - 1.6), (C + 11.5, C), (C + 6.5, C + 1.6)], (255, 160, 40, 255))
    eyes(im, C, C, 4.6, 1.3, spread=2.6, pupil=0.75)
    # kleiner Eiskristall
    for a in range(3):
        aa = a * math.pi / 3
        line(im, [(C - 4 - math.cos(aa) * 2, C - math.sin(aa) * 2), (C - 4 + math.cos(aa) * 2, C + math.sin(aa) * 2)], (190, 240, 255, 255), 0.6)
    return finish(im, TS, TS)


def t_eule():
    im = canvas(TS, TS)
    base_disc(im, (140, 100, 70), (210, 170, 120))
    line(im, [(C, C + 1), (C + 14, C + 1)], (50, 46, 56, 255), 2)
    ell(im, C + 6, C + 1, 1.6, 1.6, (90, 200, 255, 255))
    for k in (-1, 1):
        poly(im, [(C - 3, C + k * 5), (C - 6, C + k * 9.5), (C - 0.5, C + k * 7)], (90, 70, 120, 255))
    ball(im, C - 0.5, C, 8.2, 8, (140, 110, 180))
    for j in range(4):
        ImageDraw.Draw(im).arc([P(C - 6 + j * 1.5), P(C - 5), P(C - 1 + j * 1.5), P(C + 5)], 120, 240, fill=(100, 80, 140, 255), width=int(P(0.5)))
    for k in (-1, 1):
        ell(im, C + 3.2, C + k * 3.2, 2.9, 2.9, (255, 210, 80, 255))
        ell(im, C + 3.8, C + k * 3.2, 1.4, 1.4, (20, 16, 26, 255))
        ell(im, C + 3.4, C + k * 3.2 - 0.6, 0.45, 0.45, (255, 255, 255, 255))
    poly(im, [(C + 5.5, C - 1), (C + 8.5, C), (C + 5.5, C + 1)], (255, 160, 50, 255))
    return finish(im, TS, TS)


def t_seeigel():
    im = canvas(TS, TS)
    base_disc(im, (200, 180, 130), (240, 225, 180))
    for k in range(18):
        a = k * math.pi * 2 / 18
        L = 12.5 if k % 2 else 10.5
        tip = (C + math.cos(a) * L, C + math.sin(a) * L)
        b1 = (C + math.cos(a - 0.17) * 5, C + math.sin(a - 0.17) * 5)
        b2 = (C + math.cos(a + 0.17) * 5, C + math.sin(a + 0.17) * 5)
        poly(im, [b1, tip, b2], (150, 70, 200, 255) if k % 2 else (230, 110, 210, 255))
    ball(im, C, C, 6.8, 6.8, (110, 60, 170))
    eyes(im, C, C, 2.2, 1.6, spread=2.3, pupil=0.7)
    return finish(im, TS, TS)


def t_kaenguru():
    im = canvas(TS, TS)
    base_disc(im, (110, 150, 90), (170, 210, 140))
    for k in (-1, 1):
        ball(im, C - 5, C + k * 4.5, 3.8, 1.8, (190, 130, 80), hl=False)
        ell(im, C - 5, C + k * 4.5, 2.4, 0.9, (240, 180, 170, 255))
    ball(im, C - 1, C, 7.5, 6.8, (210, 150, 90))
    ball(im, C + 4.5, C, 4, 3.4, (240, 210, 170), light=0.3, dark=0.2)
    ell(im, C + 8, C, 1.2, 1.2, (40, 26, 20, 255))
    eyes(im, C, C, 2.5, 1.3, spread=2.6)
    line(im, [(C + 5, C + 8.5), (C + 9.5, C + 6), (C + 5, C + 3.8)], (150, 90, 40, 255), 1.6)
    return finish(im, TS, TS)


def t_aal():
    im = canvas(TS, TS)
    base_disc(im, (60, 120, 190), (140, 200, 255))
    d = ImageDraw.Draw(im)
    for r, w, col in [(7.5, 3.6, (180, 150, 30)), (7.5, 2.6, (250, 220, 70))]:
        d.arc([P(C - r - 1), P(C - r), P(C + r - 1), P(C + r)], 20, 340, fill=col + (255,), width=int(P(w)))
    ball(im, C + 6, C - 2.5, 3.4, 3, (255, 225, 80))
    ell(im, C + 7.3, C - 3.2, 1, 1, (20, 16, 26, 255))
    for k in range(3):
        a = k * 2.1 + 0.4
        x, y = C - 1 + math.cos(a) * 3, C + math.sin(a) * 3
        line(im, [(x - 1, y - 1.2), (x + 0.6, y), (x - 0.6, y + 0.4), (x + 1, y + 1.5)], (160, 240, 255, 255), 0.5)
    return finish(im, TS, TS)


def t_biene():
    im = canvas(TS, TS)
    base_disc(im, (110, 160, 80), (170, 220, 120))
    pts = [(C + math.cos(k * math.pi / 3 + math.pi / 6) * 10, C + math.sin(k * math.pi / 3 + math.pi / 6) * 10) for k in range(6)]
    poly(im, [(x + 0.6, y + 0.8) for x, y in pts], (0, 0, 0, 90))
    poly(im, pts, (230, 170, 40, 255))
    for j in range(4):
        y = C - 6 + j * 4
        line(im, [(C - 8.5, y), (C + 8.5, y)], (190, 120, 20, 255), 0.9)
    inner = [(C + math.cos(k * math.pi / 3 + math.pi / 6) * 6, C + math.sin(k * math.pi / 3 + math.pi / 6) * 6) for k in range(6)]
    poly(im, inner, (255, 205, 70, 255))
    ell(im, C, C + 1.5, 2, 1.6, (70, 40, 10, 255))
    ell(im, C - 2.5, C - 3, 1.6, 1, (255, 255, 255, 120))
    return finish(im, TS, TS)


def t_dachs():
    im = canvas(TS, TS)
    base_disc(im, (120, 110, 100), (190, 180, 170))
    ball(im, C - 1.5, C, 8, 7.2, (110, 115, 125))
    poly(im, [(C - 9, C - 1.2), (C + 7, C - 1), (C + 7, C + 1), (C - 9, C + 1.2)], (245, 245, 250, 255))
    for k in (-1, 1):
        ell(im, C + 3.5, C + k * 3, 2.2, 1.6, (30, 30, 36, 255))
        ell(im, C + 4, C + k * 3, 0.8, 0.8, (255, 255, 255, 255))
    ell(im, C + 7.5, C, 1.2, 1.2, (20, 20, 20, 255))
    ball(im, C + 9.5, C + 6.5, 3.6, 3.6, (210, 60, 50), light=0.2)
    ell(im, C + 9.5, C + 6.5, 2.6, 2.6, (245, 230, 200, 255))
    line(im, [(C + 5, C + 2), (C + 9, C + 5.5)], (180, 140, 90, 255), 0.8)
    return finish(im, TS, TS)


def t_schnecke():
    im = canvas(TS, TS)
    base_disc(im, (90, 110, 90), (150, 190, 140))
    ball(im, C + 1.5, C + 1, 9.5, 4.6, (140, 220, 90), light=0.4, dark=0.35)
    for k in (-1, 1):
        line(im, [(C + 8, C + k * 1), (C + 11, C + k * 3)], (120, 200, 80, 255), 0.8)
        ell(im, C + 11, C + k * 3, 1.1, 1.1, (20, 16, 26, 255))
    ball(im, C - 2.5, C - 0.5, 6.5, 6.5, (200, 130, 60))
    d = ImageDraw.Draw(im)
    for k in range(4):
        r = 5.5 - k * 1.3
        d.arc([P(C - 2.5 - r), P(C - 0.5 - r), P(C - 2.5 + r), P(C - 0.5 + r)], k * 90, k * 90 + 270, fill=(120, 70, 30, 255), width=int(P(0.7)))
    ell(im, C + 4, C + 5, 1.3, 1.3, (150, 255, 100, 200))
    return finish(im, TS, TS)


def t_fuchs():
    im = canvas(TS, TS)
    base_disc(im, (70, 60, 80), (150, 120, 170))
    poly(im, [(C - 6, C - 2), (C - 13, C - 6), (C - 11, C), (C - 13, C + 6), (C - 6, C + 2)], (240, 130, 50, 255))
    poly(im, [(C - 11, C - 4.5), (C - 13, C - 6), (C - 12, C - 3)], (255, 250, 240, 255))
    for k in (-1, 1):
        poly(im, [(C - 1, C + k * 3), (C - 4, C + k * 8.5), (C + 1.5, C + k * 5)], (220, 110, 40, 255))
    ball(im, C - 0.5, C, 7.2, 6.8, (245, 135, 50))
    poly(im, [(C + 2, C - 4), (C + 10, C), (C + 2, C + 4)], (255, 245, 235, 255))
    ell(im, C + 9.6, C, 1, 1, (30, 20, 20, 255))
    line(im, [(C - 4, C - 5.5), (C + 3, C - 6.5)], (40, 40, 60, 255), 1.6)  # Stirnband
    for k in (-1, 1):
        ell(im, C + 3.3, C + k * 2.2, 1.1, 0.8, (20, 16, 26, 255))
    # Shuriken in der Hand
    sx, sy = C + 6, C + 7.5
    for a in range(4):
        aa = a * math.pi / 2 + 0.3
        poly(im, [(sx + math.cos(aa) * 3.2, sy + math.sin(aa) * 3.2), (sx + math.cos(aa + 0.8) * 1, sy + math.sin(aa + 0.8) * 1), (sx, sy)], (200, 210, 225, 255))
    return finish(im, TS, TS)


def t_frosch():
    im = canvas(TS, TS)
    base_disc(im, (70, 80, 140), (150, 160, 230))
    for k in (-1, 1):
        ball(im, C - 4, C + k * 6.5, 3.6, 2.2, (60, 170, 70), hl=False)
    ball(im, C, C, 8, 7.4, (90, 200, 90))
    for k in (-1, 1):
        ball(im, C + 3.5, C + k * 3.4, 2.6, 2.6, (110, 220, 100), hl=False)
        ell(im, C + 4, C + k * 3.4, 1.4, 1.4, (255, 255, 255, 255))
        ell(im, C + 4.4, C + k * 3.4, 0.8, 0.8, (20, 16, 26, 255))
    # Zaubererhut
    poly(im, [(C - 9, C - 4.5), (C - 2, C - 1), (C - 9, C + 4.5)], (80, 60, 200, 255))
    poly(im, [(C - 10.5, C - 6), (C - 7.5, C - 6), (C - 7.5, C + 6), (C - 10.5, C + 6)], (60, 40, 170, 255))
    ell(im, C - 6, C, 0.9, 0.9, (255, 230, 90, 255))
    # Zauberstab
    line(im, [(C + 3, C + 7), (C + 11, C + 6)], (120, 80, 40, 255), 1)
    ell(im, C + 11.5, C + 6, 1.6, 1.6, (180, 240, 255, 255))
    return finish(im, TS, TS)


def t_maulwurf():
    im = canvas(TS, TS)
    base_disc(im, (120, 90, 60), (180, 140, 100))
    # Erdhuegel
    ball(im, C, C, 10, 9.5, (130, 95, 60), light=0.25, dark=0.4, hl=False)
    for k in range(5):
        a = k * 1.3 + 0.4
        ell(im, C + math.cos(a) * 6, C + math.sin(a) * 6, 1.3, 1, (100, 70, 45, 255))
    ball(im, C + 1, C, 6.2, 6, (90, 80, 90))
    ball(im, C + 5.5, C, 2.6, 2.2, (240, 150, 170), light=0.3, dark=0.2)
    for k in (-1, 1):
        ell(im, C + 2.8, C + k * 2.3, 0.7, 0.7, (20, 16, 26, 255))
        ball(im, C + 4, C + k * 5.5, 2, 1.6, (240, 190, 170), hl=False)
    ImageDraw.Draw(im).ellipse([P(C - 2), P(C - 6.8), P(C + 4), P(C - 3.5)], fill=(240, 200, 60, 255))  # Helm
    ell(im, C + 1, C - 5.3, 1, 0.8, (255, 255, 200, 255))
    return finish(im, TS, TS)


def t_katze():
    im = canvas(TS, TS)
    base_disc(im, (40, 50, 80), (120, 220, 255))
    for k in (-1, 1):
        poly(im, [(C - 3, C + k * 2.5), (C - 1, C + k * 8.5), (C + 2, C + k * 3)], (200, 205, 220, 255))
        poly(im, [(C - 1.8, C + k * 3.5), (C - 0.8, C + k * 7), (C + 0.8, C + k * 3.6)], (255, 170, 190, 255))
    ball(im, C, C, 7.6, 7.2, (215, 220, 235))
    # Cape
    poly(im, [(C - 4, C - 6.5), (C - 12, C - 8), (C - 12, C + 8), (C - 4, C + 6.5)], (210, 40, 60, 255))
    ball(im, C, C, 6.8, 6.4, (225, 228, 240))
    for k in (-1, 1):
        ell(im, C + 3.2, C + k * 2.6, 1.8, 1.5, (120, 220, 255, 255))
        ell(im, C + 3.6, C + k * 2.6, 0.8, 1.2, (20, 16, 26, 255))
    poly(im, [(C + 5.6, C - 0.8), (C + 6.8, C), (C + 5.6, C + 0.8)], (255, 150, 170, 255))
    line(im, [(C + 2, C + 7), (C + 12, C + 7)], (90, 100, 120, 255), 1.8)  # Laserarm
    ell(im, C + 12.3, C + 7, 1.3, 1.3, (255, 90, 110, 255))
    return finish(im, TS, TS)


TOWERS = [t_igel, t_krabbe, t_pinguin, t_eule, t_seeigel, t_kaenguru, t_aal, t_biene, t_dachs, t_schnecke, t_fuchs, t_frosch, t_maulwurf, t_katze]

# ======================================================================== Goo
GOO = [  # (radius, farbe, extra)
    (4.5, (124, 232, 90), ""), (6, (74, 200, 240), ""), (7.5, (240, 208, 64), ""), (8.5, (240, 138, 48), ""),
    (11, (224, 80, 140), "big"), (17, (154, 90, 240), "king"), (22, (70, 70, 100), "elder"), (27, (200, 42, 42), "titan"),
]


def goo_sprite(r, col, extra):
    pad = 2
    top = r * 0.9 if extra in ("king", "elder", "titan") else 0
    w, h = r * 2.3 + pad * 2, r * 2.0 + pad * 2 + top
    im = canvas(w, h)
    cx, cy = w / 2, pad + top + r * 1.0
    ell(im, cx + r * 0.08, cy + r * 0.62, r * 0.95, r * 0.28, (0, 0, 0, 60))
    ball(im, cx, cy, r * 1.12, r * 0.95, col, light=0.55, dark=0.5, hl=False)
    d = ImageDraw.Draw(im)
    # Glanz + Blaeschen
    d.ellipse([P(cx - r * 0.75), P(cy - r * 0.75), P(cx - r * 0.1), P(cy - r * 0.35)], fill=(255, 255, 255, 150))
    d.ellipse([P(cx + r * 0.25), P(cy - r * 0.65), P(cx + r * 0.45), P(cy - r * 0.45)], fill=(255, 255, 255, 130))
    if r >= 8:
        for k in range(3):
            bx, by = cx - r * 0.3 + k * r * 0.35, cy + r * 0.45 - (k % 2) * r * 0.15
            d.ellipse([P(bx - r * 0.07), P(by - r * 0.07), P(bx + r * 0.07), P(by + r * 0.07)], outline=(255, 255, 255, 120), width=int(P(0.4)))
    eye = max(1.3, r * 0.27)
    glow = extra in ("elder",)
    for k in (-1, 1):
        ex, ey = cx + r * 0.2 + k * r * 0.36, cy - r * 0.1
        ell(im, ex, ey, eye, eye * 1.1, (255, 240, 120, 255) if glow else (255, 255, 255, 255))
        ell(im, ex + eye * 0.35, ey + eye * 0.1, eye * 0.55, eye * 0.6, (20, 16, 26, 255))
        ell(im, ex + eye * 0.15, ey - eye * 0.3, eye * 0.18, eye * 0.18, (255, 255, 255, 255))
        if extra in ("king", "elder", "titan", "big"):
            line(im, [(ex + k * eye * 1.1, ey - eye * 1.5), (ex - k * eye * 0.6, ey - eye * 0.9)], (30, 16, 20, 255), max(0.6, r * 0.07))
    ImageDraw.Draw(im).arc([P(cx + r * 0.05), P(cy + r * 0.12), P(cx + r * 0.55), P(cy + r * 0.45)], 200, 340, fill=(30, 16, 20, 255), width=int(P(max(0.5, r * 0.06))))
    if extra in ("king", "elder", "titan"):
        cw, ch = r * 1.0, r * 0.7
        by = cy - r * 0.8
        gold = (255, 200, 60, 255) if extra != "elder" else (210, 210, 230, 255)
        pts = [(cx - cw / 2, by), (cx - cw / 2, by - ch * 0.6), (cx - cw / 4, by - ch * 0.25), (cx, by - ch), (cx + cw / 4, by - ch * 0.25),
               (cx + cw / 2, by - ch * 0.6), (cx + cw / 2, by)]
        poly(im, pts, gold)
        for gx, gc in [(cx - cw / 4, (255, 60, 80)), (cx, (90, 200, 255)), (cx + cw / 4, (90, 230, 120))]:
            ell(im, gx, by - ch * 0.12, r * 0.06, r * 0.06, gc + (255,))
    if extra == "titan":
        for k in (-1, 1):
            poly(im, [(cx + k * r * 0.6, cy - r * 0.6), (cx + k * r * 1.05, cy - r * 1.25), (cx + k * r * 0.85, cy - r * 0.5)], (240, 230, 210, 255))
    return finish(im, w, h), (cx, cy)


# ======================================================================== Overlays, Geschosse, Effekte, Icons
def helmet():
    im = canvas(32, 20)
    ball(im, 16, 14, 15, 11, (170, 178, 195), light=0.6, dark=0.5)
    ImageDraw.Draw(im).rectangle([0, P(14), P(32), P(20)], fill=(0, 0, 0, 0))
    line(im, [(2, 13.5), (30, 13.5)], (110, 118, 135, 255), 1.6)
    for x in (8, 16, 24):
        ell(im, x, 8 if x == 16 else 10, 1.1, 1.1, (90, 98, 115, 255))
    return finish(im, 32, 20)


def ice_block():
    im = canvas(32, 32)
    ImageDraw.Draw(im).rounded_rectangle([P(1), P(1), P(31), P(31)], radius=P(5), fill=(190, 235, 255, 140), outline=(240, 252, 255, 230), width=int(P(1.2)))
    line(im, [(6, 6), (13, 6)], (255, 255, 255, 200), 1.4)
    line(im, [(6, 6), (6, 12)], (255, 255, 255, 200), 1.4)
    line(im, [(20, 24), (26, 18)], (255, 255, 255, 140), 1)
    return finish(im, 32, 32, outline=None)


def glow():
    w = 32
    g = radial(w, w, w / 2, w / 2, w / 2, [(0, (255, 255, 255, 255)), (0.35, (255, 255, 255, 160)), (1, (255, 255, 255, 0))])
    return g


def ring():
    im = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    ImageDraw.Draw(im).ellipse([6, 6, 58, 58], outline=(255, 255, 255, 255), width=5)
    im = im.filter(ImageFilter.GaussianBlur(1.6))
    return im.resize((32, 32), Image.LANCZOS)


def splat():
    im = canvas(24, 24)
    rnd = random.Random(4)
    ell(im, 12, 12, 5, 5, (255, 255, 255, 255))
    for k in range(9):
        a = k * 0.7 + rnd.random() * 0.4
        L = 6 + rnd.random() * 5
        x, y = 12 + math.cos(a) * L, 12 + math.sin(a) * L
        r = 1 + rnd.random() * 1.8
        line(im, [(12 + math.cos(a) * 3, 12 + math.sin(a) * 3), (x, y)], (255, 255, 255, 255), r * 0.9)
        ell(im, x, y, r, r, (255, 255, 255, 255))
    return finish(im, 24, 24, outline=None)


def spark():
    im = canvas(16, 16)
    poly(im, [(8, 0), (9.3, 6.7), (16, 8), (9.3, 9.3), (8, 16), (6.7, 9.3), (0, 8), (6.7, 6.7)], (255, 255, 255, 255))
    return finish(im, 16, 16, outline=None)


def proj(name):
    if name == "spike":
        im = canvas(12, 5); poly(im, [(0, 1.5), (9, 1.5), (12, 2.5), (9, 3.5), (0, 3.5)], (90, 60, 35, 255)); poly(im, [(8, 1.6), (12, 2.5), (8, 3.4)], (235, 215, 185, 255)); return finish(im, 12, 5)
    if name == "blade":
        im = canvas(14, 6); poly(im, [(0, 2), (10, 1), (14, 3), (10, 5), (0, 4)], (225, 232, 248, 255)); line(im, [(1, 3), (11, 3)], (160, 170, 190, 255), 0.6); return finish(im, 14, 6)
    if name == "bomb":
        im = canvas(11, 11); ball(im, 5, 6, 4.3, 4.3, (50, 50, 62)); line(im, [(6.5, 2.5), (8.5, 1)], (150, 110, 60, 255), 0.8); ell(im, 9, 1.2, 1.2, 1.2, (255, 200, 60, 255)); return finish(im, 11, 11)
    if name == "minibomb":
        im = canvas(7, 7); ball(im, 3.5, 3.5, 2.8, 2.8, (60, 60, 72)); return finish(im, 7, 7)
    if name == "shard":
        im = canvas(5, 5); poly(im, [(0, 2.5), (2.5, 0), (5, 2.5), (2.5, 5)], (120, 120, 130, 255)); return finish(im, 5, 5)
    if name == "boomer":
        im = canvas(13, 13); line(im, [(2, 2.5), (6.5, 6.5), (2, 10.5)], (190, 120, 50, 255), 2.4); line(im, [(2, 2.5), (6.5, 6.5)], (230, 170, 90, 255), 1); return finish(im, 13, 13)
    if name == "slime":
        im = canvas(10, 10); ball(im, 5, 5, 4, 3.6, (130, 230, 80)); return finish(im, 10, 10)
    if name == "ice":
        im = canvas(12, 7); poly(im, [(0, 3.5), (4, 0.5), (12, 3.5), (4, 6.5)], (200, 245, 255, 255)); line(im, [(3, 3.5), (10, 3.5)], (255, 255, 255, 255), 0.6); return finish(im, 12, 7)
    if name == "shuriken":
        im = canvas(11, 11)
        for a in range(4):
            aa = a * math.pi / 2
            poly(im, [(5.5 + math.cos(aa) * 5.3, 5.5 + math.sin(aa) * 5.3), (5.5 + math.cos(aa + 0.9) * 1.6, 5.5 + math.sin(aa + 0.9) * 1.6), (5.5, 5.5)], (210, 218, 232, 255))
        ell(im, 5.5, 5.5, 1.1, 1.1, (60, 60, 70, 255)); return finish(im, 11, 11)
    if name == "magic":
        im = canvas(12, 12); g = glow().resize((P(12), P(12))); im.alpha_composite(tint(g.getchannel("A"), (140, 220, 255))); ell(im, 6, 6, 2.2, 2.2, (255, 255, 255, 255)); return finish(im, 12, 12, outline=None)
    if name == "fireball":
        im = canvas(14, 14); g = glow().resize((P(14), P(14))); im.alpha_composite(tint(g.getchannel("A"), (255, 120, 30))); ball(im, 7, 7, 3.6, 3.6, (255, 190, 60), light=0.7); return finish(im, 14, 14, outline=None)
    if name == "pile":
        im = canvas(14, 14)
        for k in range(8):
            a = k * math.pi / 4 + 0.2
            poly(im, [(7 + math.cos(a - 0.35) * 2.5, 7 + math.sin(a - 0.35) * 2.5), (7 + math.cos(a) * 6.5, 7 + math.sin(a) * 6.5), (7 + math.cos(a + 0.35) * 2.5, 7 + math.sin(a + 0.35) * 2.5)], (150, 155, 170, 255))
        ball(im, 7, 7, 3, 3, (110, 115, 130)); return finish(im, 14, 14)
    if name == "laser":
        im = canvas(14, 5); g = Image.new("RGBA", (P(14), P(5)), (0, 0, 0, 0)); ImageDraw.Draw(g).rounded_rectangle([0, P(0.5), P(14), P(4.5)], radius=P(2), fill=(255, 80, 120, 200)); im.alpha_composite(g.filter(ImageFilter.GaussianBlur(P(0.6))))
        ImageDraw.Draw(im).rounded_rectangle([P(1), P(1.8), P(13), P(3.2)], radius=P(0.7), fill=(255, 230, 240, 255)); return finish(im, 14, 5, outline=None)


def icon(name):
    s = 16
    im = canvas(s, s)
    if name == "coin":
        ball(im, 8, 8, 7, 7, (220, 160, 30), light=0.4)
        ell(im, 8, 8, 5, 5, (255, 205, 60, 255))
        line(im, [(8, 5), (8, 11)], (210, 150, 30, 255), 1.4)
        ell(im, 5.5, 5.5, 1.3, 1.3, (255, 255, 230, 255))
    elif name == "heart":
        ell(im, 5, 6, 4, 4, (240, 60, 90, 255)); ell(im, 11, 6, 4, 4, (240, 60, 90, 255))
        poly(im, [(1.3, 7.5), (14.7, 7.5), (8, 15)], (240, 60, 90, 255)); ell(im, 4.5, 5, 1.6, 1.2, (255, 200, 210, 255))
    elif name == "jar":
        ImageDraw.Draw(im).rounded_rectangle([P(3), P(4), P(13), P(15)], radius=P(2.5), fill=(240, 160, 30, 255))
        ImageDraw.Draw(im).rectangle([P(2.5), P(2), P(13.5), P(4.5)], fill=(200, 210, 225, 255))
        ell(im, 6, 8, 1.2, 2.5, (255, 230, 160, 255))
    elif name == "star":
        pts = []
        for k in range(10):
            a = -math.pi / 2 + k * math.pi / 5
            r = 7.5 if k % 2 == 0 else 3.3
            pts.append((8 + math.cos(a) * r, 8.5 + math.sin(a) * r))
        poly(im, pts, (120, 200, 255, 255)); ell(im, 6.5, 6.5, 1.2, 1.2, (255, 255, 255, 255))
    elif name == "crown":
        poly(im, [(1.5, 13), (1.5, 5), (5, 8.5), (8, 2.5), (11, 8.5), (14.5, 5), (14.5, 13)], (255, 200, 50, 255))
        line(im, [(1.5, 12), (14.5, 12)], (220, 150, 30, 255), 1.5)
        ell(im, 8, 9.5, 1.2, 1.2, (255, 70, 90, 255)); ell(im, 4.5, 10, 0.9, 0.9, (90, 200, 255, 255)); ell(im, 11.5, 10, 0.9, 0.9, (90, 230, 120, 255))
    elif name.startswith("medal"):
        col = {"medal0": (205, 127, 50), "medal1": (200, 210, 225), "medal2": (255, 200, 50)}[name]
        poly(im, [(5, 0), (8, 6), (11, 0)], (220, 60, 70, 255))
        ball(im, 8, 10, 5.5, 5.5, col, light=0.5)
        ell(im, 8, 10, 3.6, 3.6, tuple(min(255, c + 30) for c in col) + (255,))
    elif name == "lock":
        ImageDraw.Draw(im).rounded_rectangle([P(3), P(7), P(13), P(15)], radius=P(1.5), fill=(150, 156, 175, 255))
        ImageDraw.Draw(im).arc([P(4.5), P(1.5), P(11.5), P(10)], 180, 360, fill=(120, 126, 145, 255), width=int(P(1.6)))
        ell(im, 8, 11, 1.2, 1.2, (60, 64, 80, 255))
    elif name == "skull":
        ell(im, 8, 7, 6, 5.5, (240, 240, 245, 255)); ell(im, 5.8, 7, 1.6, 1.8, (30, 30, 40, 255)); ell(im, 10.2, 7, 1.6, 1.8, (30, 30, 40, 255))
        ImageDraw.Draw(im).rectangle([P(5), P(11), P(11), P(14)], fill=(240, 240, 245, 255))
    elif name == "play":
        poly(im, [(4, 2), (14, 8), (4, 14)], (255, 255, 255, 255))
    elif name == "ff":
        poly(im, [(1, 3), (8, 8), (1, 13)], (255, 255, 255, 255)); poly(im, [(8, 3), (15, 8), (8, 13)], (255, 255, 255, 255))
    elif name == "pause":
        ImageDraw.Draw(im).rectangle([P(3.5), P(2.5), P(6.5), P(13.5)], fill=(255, 255, 255, 255)); ImageDraw.Draw(im).rectangle([P(9.5), P(2.5), P(12.5), P(13.5)], fill=(255, 255, 255, 255))
    elif name == "gear":
        for k in range(8):
            a = k * math.pi / 4
            poly(im, [(8 + math.cos(a - 0.25) * 5, 8 + math.sin(a - 0.25) * 5), (8 + math.cos(a - 0.18) * 7.5, 8 + math.sin(a - 0.18) * 7.5),
                      (8 + math.cos(a + 0.18) * 7.5, 8 + math.sin(a + 0.18) * 7.5), (8 + math.cos(a + 0.25) * 5, 8 + math.sin(a + 0.25) * 5)], (200, 205, 220, 255))
        ell(im, 8, 8, 5.2, 5.2, (200, 205, 220, 255)); ell(im, 8, 8, 2, 2, (0, 0, 0, 0))
        ImageDraw.Draw(im).ellipse([P(6), P(6), P(10), P(10)], fill=(0, 0, 0, 0))
    elif name == "trophy":
        poly(im, [(3, 2), (13, 2), (12, 8), (8, 10), (4, 8)], (255, 200, 50, 255))
        ImageDraw.Draw(im).rectangle([P(7), P(9), P(9), P(12)], fill=(220, 160, 30, 255)); ImageDraw.Draw(im).rectangle([P(4.5), P(12), P(11.5), P(14.5)], fill=(160, 110, 60, 255))
        ell(im, 6, 4.5, 1, 1.4, (255, 250, 200, 255))
    elif name == "book":
        ImageDraw.Draw(im).rounded_rectangle([P(2), P(2), P(14), P(14)], radius=P(1.5), fill=(90, 150, 240, 255))
        ImageDraw.Draw(im).rectangle([P(4), P(4), P(12), P(6)], fill=(255, 255, 255, 220))
    return finish(im, s, s, outline=(20, 14, 30) if name not in ("play", "ff", "pause") else None)


# ======================================================================== Karten-Hintergruende
def noise(w, h, scales, seed):
    rng = np.random.RandomState(seed)
    acc = np.zeros((h, w), np.float32)
    tot = 0
    for sc, wt in scales:
        n = rng.rand(max(2, h // sc + 2), max(2, w // sc + 2)).astype(np.float32)
        im = Image.fromarray((n * 255).astype(np.uint8)).resize((w + sc * 2, h + sc * 2), Image.BICUBIC).crop((sc, sc, sc + w, sc + h))
        acc += np.asarray(im, np.float32) / 255 * wt
        tot += wt
    return acc / tot


def colorize(n, c1, c2):
    n = n[..., None]
    arr = np.array(c1, np.float32) * (1 - n) + np.array(c2, np.float32) * n
    a = np.full(n.shape, 255, np.float32)
    return Image.fromarray(np.concatenate([arr, a], 2).clip(0, 255).astype(np.uint8), "RGBA")


def path_mask(w, h, pts, width, k):
    m = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(m)
    sp = [(x * k, y * k) for x, y in pts]
    d.line(sp, fill=255, width=int(width * k), joint="curve")
    for x, y in sp:
        r = width * k / 2
        d.ellipse([x - r, y - r, x + r, y + r], fill=255)
    return m


def dist_to_path(x, y, pts):
    best = 1e9
    for (ax, ay), (bx, by) in zip(pts, pts[1:]):
        dx, dy = bx - ax, by - ay
        l2 = dx * dx + dy * dy
        t = 0 if l2 == 0 else max(0, min(1, ((x - ax) * dx + (y - ay) * dy) / l2))
        best = min(best, math.hypot(x - ax - dx * t, y - ay - dy * t))
    return best


def shadowed(base, draw_fn, k, off=(3, 4), blur=3, alpha=90):
    """zeichnet Objekt mit weichem Schatten"""
    lay = Image.new("RGBA", base.size, (0, 0, 0, 0))
    draw_fn(lay)
    sh = Image.new("RGBA", base.size, (0, 0, 0, 0))
    a = lay.getchannel("A")
    sh2 = Image.new("L", base.size, 0)
    sh2.paste(a, (int(off[0] * k), int(off[1] * k)))
    sh.putalpha(sh2.point(lambda v: v * alpha // 255).filter(ImageFilter.GaussianBlur(blur * k)))
    base.alpha_composite(sh)
    base.alpha_composite(lay)


def tree(d, x, y, r, k, pal):
    dark, mid, light = pal
    for (ox, oy, rr, c) in [(0, 0, 1.0, dark), (-0.25, -0.2, 0.75, mid), (0.22, -0.1, 0.62, mid), (-0.2, -0.38, 0.4, light)]:
        cx, cy, R = (x + ox * r) * k, (y + oy * r) * k, r * rr * k
        d.ellipse([cx - R, cy - R, cx + R, cy + R], fill=c)


def make_bg(mi, mp):
    k = 2
    W, H = 400 * k, 240 * k
    th = mp["theme"]
    pts = mp["pts"]
    rnd = random.Random(100 + mi)
    pal = {
        0: dict(g1=(70, 150, 55), g2=(110, 190, 70), p1=(215, 185, 130), p2=(185, 150, 95), edge=(120, 95, 60)),
        1: dict(g1=(225, 200, 140), g2=(245, 225, 170), p1=(170, 115, 70), p2=(140, 90, 55), edge=(100, 65, 40)),
        2: dict(g1=(42, 38, 70), g2=(70, 60, 105), p1=(130, 120, 160), p2=(100, 92, 130), edge=(60, 55, 85)),
        3: dict(g1=(45, 32, 30), g2=(78, 55, 48), p1=(95, 85, 80), p2=(70, 62, 58), edge=(255, 110, 40)),
        4: dict(g1=(215, 232, 248), g2=(245, 250, 255), p1=(160, 190, 215), p2=(135, 165, 195), edge=(100, 130, 165)),
        5: dict(g1=(255, 190, 220), g2=(255, 220, 238), p1=(130, 80, 50), p2=(105, 62, 38), edge=(255, 255, 255)),
    }[th]
    n = noise(W, H, [(4, 1), (16, 1.2), (60, 1.5)], 7 + mi)
    img = colorize(n, pal["g1"], pal["g2"])
    d = ImageDraw.Draw(img)
    # Themen-Untergrund
    if th == 0:
        for _ in range(500):
            x, y = rnd.uniform(0, W), rnd.uniform(0, H)
            c = (60 + rnd.randint(0, 30), 140 + rnd.randint(0, 40), 50, 255)
            d.line([(x, y), (x - 2 * k, y - 5 * k)], fill=c, width=k)
            d.line([(x, y), (x + 2 * k, y - 4 * k)], fill=c, width=k)
    if th == 1:
        sea = vgrad(W, int(40 * k), [(0, (120, 220, 235, 255)), (1, (40, 140, 200, 255))])
        img.alpha_composite(sea, (0, int(206 * k)))
        for j in range(4):
            yy = 206 * k + j * 8 * k
            for x in range(0, W, 24 * k):
                d.arc([x + (j % 2) * 12 * k, yy - 4 * k, x + 24 * k + (j % 2) * 12 * k, yy + 4 * k], 200, 340, fill=(255, 255, 255, 200), width=k)
    if th == 2:
        for _ in range(60):
            x, y = rnd.uniform(0, W), rnd.uniform(0, H)
            d.ellipse([x - 9 * k, y - 4 * k, x + 9 * k, y + 4 * k], fill=(32, 28, 55, 120))
    if th == 3:
        lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
        dl = ImageDraw.Draw(lay)
        for _ in range(14):
            x, y = rnd.uniform(0, W), rnd.uniform(0, H)
            pts2 = [(x, y)]
            for j in range(5):
                x += rnd.uniform(-30, 30) * k; y += rnd.uniform(-30, 30) * k
                pts2.append((x, y))
            dl.line(pts2, fill=(255, 120, 30, 255), width=3 * k)
        img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(4 * k)))
        img.alpha_composite(lay)
    if th == 4:
        for _ in range(8):
            x, y = rnd.uniform(0, W), rnd.uniform(0, H)
            r = rnd.uniform(25, 45) * k
            d.ellipse([x - r, y - r * 0.6, x + r, y + r * 0.6], fill=(190, 225, 245, 160))
            d.line([(x - r * 0.5, y), (x + r * 0.3, y - r * 0.2)], fill=(255, 255, 255, 200), width=k)
    if th == 5:
        for _ in range(500):
            x, y = rnd.uniform(0, W), rnd.uniform(0, H)
            c = rnd.choice([(255, 255, 255), (120, 220, 255), (255, 230, 90), (140, 240, 160), (255, 120, 160)])
            a = rnd.uniform(0, 3.14)
            d.line([(x, y), (x + math.cos(a) * 4 * k, y + math.sin(a) * 4 * k)], fill=c + (255,), width=k * 2)
    # Weg
    m_edge = path_mask(W, H, pts, PATH_W * 2 + 5, k)
    m_path = path_mask(W, H, pts, PATH_W * 2, k)
    sh = Image.new("RGBA", img.size, (0, 0, 0, 0))
    sh.putalpha(m_edge.filter(ImageFilter.GaussianBlur(4 * k)).point(lambda v: v * 110 // 255))
    img.alpha_composite(sh, (int(2 * k), int(3 * k)))
    edge = Image.new("RGBA", img.size, pal["edge"] + (255,))
    edge.putalpha(m_edge)
    img.alpha_composite(edge)
    pn = noise(W, H, [(3, 1), (10, 1)], 31 + mi)
    ptex = colorize(pn, pal["p2"], pal["p1"])
    if th == 1:  # Holzsteg
        dp = ImageDraw.Draw(ptex)
        for (ax, ay), (bx, by) in zip(pts, pts[1:]):
            L = math.hypot(bx - ax, by - ay)
            nx, ny = -(by - ay) / L, (bx - ax) / L
            for t in np.arange(0, L, 6):
                x, y = ax + (bx - ax) * t / L, ay + (by - ay) * t / L
                dp.line([((x + nx * 12) * k, (y + ny * 12) * k), ((x - nx * 12) * k, (y - ny * 12) * k)], fill=(95, 60, 35, 255), width=k)
    if th == 2:  # Pflaster
        dp = ImageDraw.Draw(ptex)
        for _ in range(900):
            x, y = rnd.uniform(0, W), rnd.uniform(0, H)
            dp.ellipse([x - 4 * k, y - 3 * k, x + 4 * k, y + 3 * k], outline=(80, 74, 105, 255), width=k)
    ptex.putalpha(m_path)
    img.alpha_composite(ptex)
    hl = Image.new("RGBA", img.size, (255, 255, 255, 0))
    hl.putalpha(path_mask(W, H, pts, PATH_W * 0.9, k).filter(ImageFilter.GaussianBlur(3 * k)).point(lambda v: v * 50 // 255))
    img.alpha_composite(hl)
    if th == 3:  # gluehende Wegraender
        gl = Image.new("RGBA", img.size, (255, 120, 40, 0))
        gl.putalpha(ImageChops.subtract(m_edge, m_path).filter(ImageFilter.GaussianBlur(2 * k)))
        img.alpha_composite(gl)
    if th == 5:  # Zuckerguss-Rand
        ic = Image.new("RGBA", img.size, (255, 255, 255, 255))
        ic.putalpha(ImageChops.subtract(m_edge, m_path))
        img.alpha_composite(ic)
    # Deko mit Schatten
    placed = []
    def free(x, y, r):
        if dist_to_path(x, y, pts) < PATH_W + r + 4:
            return False
        return all(math.hypot(x - a, y - b) > r + rr + 3 for a, b, rr in placed)
    for _ in range(900):
        x, y = rnd.uniform(6, 394), rnd.uniform(6, 234)
        r = rnd.uniform(4, 11)
        if th == 1 and y > 200:
            continue
        if not free(x, y, r):
            continue
        if len(placed) > 46:
            break
        placed.append((x, y, r))
        kind = rnd.random()
        def draw(lay, x=x, y=y, r=r, kind=kind):
            dl = ImageDraw.Draw(lay)
            if th == 0:
                if kind < 0.45: tree(dl, x, y, r, k, [(40, 110, 40, 255), (70, 150, 55, 255), (130, 200, 90, 255)])
                elif kind < 0.7:
                    for j in range(5):
                        a = j * 1.256
                        dl.ellipse([(x + math.cos(a) * 1.8 - 1.4) * k, (y + math.sin(a) * 1.8 - 1.4) * k, (x + math.cos(a) * 1.8 + 1.4) * k, (y + math.sin(a) * 1.8 + 1.4) * k], fill=rnd.choice([(255, 240, 250, 255), (255, 210, 90, 255), (190, 160, 255, 255)]))
                    dl.ellipse([(x - 1) * k, (y - 1) * k, (x + 1) * k, (y + 1) * k], fill=(255, 200, 60, 255))
                else:
                    dl.ellipse([(x - r * 0.6) * k, (y - r * 0.4) * k, (x + r * 0.6) * k, (y + r * 0.4) * k], fill=(140, 140, 130, 255))
                    dl.ellipse([(x - r * 0.4) * k, (y - r * 0.4) * k, (x + r * 0.1) * k, (y - r * 0.05) * k], fill=(185, 185, 175, 255))
            elif th == 1:
                if kind < 0.35:
                    for j in range(6):
                        a = j * 1.05 + 0.3
                        dl.line([(x * k, y * k), ((x + math.cos(a) * r * 1.2) * k, (y + math.sin(a) * r * 1.2) * k)], fill=(60, 160, 70, 255), width=int(2.2 * k))
                    dl.ellipse([(x - 1.8) * k, (y - 1.8) * k, (x + 1.8) * k, (y + 1.8) * k], fill=(140, 95, 50, 255))
                elif kind < 0.6:
                    for j in range(5):
                        a = j * 1.256 - 1.57
                        dl.polygon([(x * k, y * k), ((x + math.cos(a - 0.3) * 1.5) * k, (y + math.sin(a - 0.3) * 1.5) * k), ((x + math.cos(a) * 4.5) * k, (y + math.sin(a) * 4.5) * k), ((x + math.cos(a + 0.3) * 1.5) * k, (y + math.sin(a + 0.3) * 1.5) * k)], fill=(255, 130, 90, 255))
                else:
                    dl.pieslice([(x - 4) * k, (y - 4) * k, (x + 4) * k, (y + 4) * k], 180, 360, fill=(255, 230, 235, 255))
                    for j in range(4):
                        dl.line([(x * k, y * k), ((x - 3.5 + j * 2.3) * k, (y - 2.5) * k)], fill=(230, 180, 190, 255), width=k)
            elif th == 2:
                if kind < 0.55:
                    for j in range(3):
                        h2 = r * (1.3 - j * 0.25)
                        ox = (j - 1) * r * 0.45
                        c = rnd.choice([(120, 220, 255), (200, 130, 255), (255, 140, 220)])
                        dl.polygon([((x + ox - 2) * k, y * k), ((x + ox) * k, (y - h2) * k), ((x + ox + 2) * k, y * k)], fill=c + (255,))
                        dl.polygon([((x + ox) * k, y * k), ((x + ox) * k, (y - h2) * k), ((x + ox + 2) * k, y * k)], fill=tuple(int(v * 0.7) for v in c) + (255,))
                else:
                    dl.ellipse([(x - r * 0.7) * k, (y - r * 0.45) * k, (x + r * 0.7) * k, (y + r * 0.45) * k], fill=(55, 50, 85, 255))
                    dl.ellipse([(x - r * 0.5) * k, (y - r * 0.45) * k, (x + r * 0.1) * k, (y - r * 0.05) * k], fill=(80, 72, 115, 255))
            elif th == 3:
                if kind < 0.6:
                    dl.polygon([((x - r * 0.8) * k, (y + r * 0.3) * k), ((x - r * 0.3) * k, (y - r * 0.6) * k), ((x + r * 0.5) * k, (y - r * 0.5) * k), ((x + r * 0.8) * k, (y + r * 0.35) * k)], fill=(35, 26, 24, 255))
                    dl.polygon([((x - r * 0.3) * k, (y - r * 0.6) * k), ((x + r * 0.5) * k, (y - r * 0.5) * k), ((x + r * 0.2) * k, (y - r * 0.1) * k)], fill=(70, 55, 50, 255))
                else:
                    dl.ellipse([(x - r * 0.6) * k, (y - r * 0.35) * k, (x + r * 0.6) * k, (y + r * 0.35) * k], fill=(255, 110, 30, 255))
                    dl.ellipse([(x - r * 0.35) * k, (y - r * 0.2) * k, (x + r * 0.35) * k, (y + r * 0.2) * k], fill=(255, 210, 90, 255))
            elif th == 4:
                if kind < 0.5:
                    for j in range(3):
                        w2, yy = r * (0.9 - j * 0.25), y - j * r * 0.45
                        dl.polygon([((x - w2) * k, yy * k), (x * k, (yy - r * 0.7) * k), ((x + w2) * k, yy * k)], fill=(40, 100, 70, 255))
                        dl.polygon([((x - w2 * 0.8) * k, (yy - r * 0.15) * k), (x * k, (yy - r * 0.7) * k), ((x + w2 * 0.2) * k, (yy - r * 0.3) * k)], fill=(250, 252, 255, 255))
                elif kind < 0.65:
                    dl.ellipse([(x - 3) * k, (y - 3) * k, (x + 3) * k, (y + 3) * k], fill=(250, 252, 255, 255))
                    dl.ellipse([(x - 2) * k, (y - 7) * k, (x + 2) * k, (y - 3) * k], fill=(250, 252, 255, 255))
                    dl.polygon([(x * k, (y - 5.3) * k), ((x + 2.5) * k, (y - 5) * k), (x * k, (y - 4.6) * k)], fill=(255, 140, 40, 255))
                else:
                    dl.ellipse([(x - r * 0.6) * k, (y - r * 0.35) * k, (x + r * 0.6) * k, (y + r * 0.35) * k], fill=(170, 185, 205, 255))
                    dl.ellipse([(x - r * 0.5) * k, (y - r * 0.4) * k, (x + r * 0.3) * k, (y - r * 0.05) * k], fill=(250, 252, 255, 255))
            elif th == 5:
                if kind < 0.35:
                    dl.line([(x * k, y * k), (x * k, (y + r) * k)], fill=(250, 250, 250, 255), width=int(1.3 * k))
                    c = rnd.choice([(255, 90, 140), (120, 200, 255), (255, 200, 60), (140, 230, 120)])
                    dl.ellipse([(x - r * 0.55) * k, (y - r * 0.55) * k, (x + r * 0.55) * k, (y + r * 0.55) * k], fill=c + (255,))
                    dl.arc([(x - r * 0.35) * k, (y - r * 0.35) * k, (x + r * 0.35) * k, (y + r * 0.35) * k], 0, 270, fill=(255, 255, 255, 255), width=k)
                elif kind < 0.7:
                    c = rnd.choice([(255, 90, 140), (120, 200, 255), (255, 200, 60), (140, 230, 120), (200, 130, 255)])
                    dl.pieslice([(x - r * 0.55) * k, (y - r * 0.6) * k, (x + r * 0.55) * k, (y + r * 0.5) * k], 180, 360, fill=c + (255,))
                    dl.rectangle([(x - r * 0.55) * k, (y - r * 0.06) * k, (x + r * 0.55) * k, (y + r * 0.2) * k], fill=c + (255,))
                    dl.ellipse([(x - r * 0.3) * k, (y - r * 0.45) * k, (x - r * 0.05) * k, (y - r * 0.25) * k], fill=(255, 255, 255, 170))
                else:
                    dl.line([(x * k, (y + r * 0.6) * k), (x * k, (y - r * 0.3) * k)], fill=(255, 255, 255, 255), width=int(2 * k))
                    dl.arc([(x - r * 0.45) * k, (y - r * 0.75) * k, (x + r * 0.05) * k, (y - r * 0.05) * k], 180, 360, fill=(255, 255, 255, 255), width=int(2 * k))
                    for j in range(3):
                        dl.line([((x - 1) * k, (y - r * 0.1 + j * 3) * k), ((x + 1) * k, (y - r * 0.1 + j * 3 + 1.5) * k)], fill=(230, 50, 70, 255), width=int(1.2 * k))
        shadowed(img, draw, k, off=(2, 3), blur=2, alpha=90)
    # Start (Goo-Rohr) und Ziel (Haus)
    sx, sy = pts[0]
    ex, ey = pts[-1]
    def clampv(v, lo, hi): return max(lo, min(hi, v))
    sx2, sy2 = clampv(sx, 6, 394), clampv(sy, 6, 234)
    lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    dl = ImageDraw.Draw(lay)
    dl.ellipse([(sx2 - 13) * k, (sy2 - 13) * k, (sx2 + 13) * k, (sy2 + 13) * k], fill=(70, 160, 60, 200))
    dl.ellipse([(sx2 - 9) * k, (sy2 - 9) * k, (sx2 + 9) * k, (sy2 + 9) * k], fill=(124, 232, 90, 255))
    dl.ellipse([(sx2 - 5) * k, (sy2 - 6) * k, (sx2 + 1) * k, (sy2 - 2) * k], fill=(220, 255, 200, 220))
    img.alpha_composite(lay)
    ex2, ey2 = clampv(ex, 14, 386), clampv(ey, 14, 226)
    def house(lay):
        dl = ImageDraw.Draw(lay)
        if th == 2:
            dl.ellipse([(ex2 - 14) * k, (ey2 - 10) * k, (ex2 + 14) * k, (ey2 + 10) * k], fill=(10, 8, 20, 255))
            dl.ellipse([(ex2 - 10) * k, (ey2 - 6) * k, (ex2 + 10) * k, (ey2 + 6) * k], fill=(40, 20, 70, 255))
            return
        wall = {0: (255, 235, 200), 1: (255, 245, 220), 3: (110, 90, 85), 4: (240, 248, 255), 5: (255, 230, 240)}[th]
        roof = {0: (220, 80, 60), 1: (60, 150, 210), 3: (60, 40, 36), 4: (140, 190, 230), 5: (255, 110, 170)}[th]
        dl.rectangle([(ex2 - 10) * k, (ey2 - 4) * k, (ex2 + 10) * k, (ey2 + 10) * k], fill=wall + (255,))
        dl.polygon([((ex2 - 13) * k, (ey2 - 3) * k), (ex2 * k, (ey2 - 15) * k), ((ex2 + 13) * k, (ey2 - 3) * k)], fill=roof + (255,))
        dl.rectangle([(ex2 - 3) * k, (ey2 + 2) * k, (ex2 + 3) * k, (ey2 + 10) * k], fill=(120, 75, 45, 255))
        dl.rectangle([(ex2 + 5) * k, (ey2 - 1) * k, (ex2 + 8.5) * k, (ey2 + 2.5) * k], fill=(140, 210, 255, 255))
    shadowed(img, house, k, off=(3, 3), blur=2, alpha=110)
    # Vignette
    vig = radial(W, H, W / 2, H / 2, math.hypot(W, H) / 2, [(0, (0, 0, 0, 0)), (0.7, (0, 0, 0, 0)), (1, (0, 0, 0, 90))])
    img.alpha_composite(vig)
    return img.resize((400, 240), Image.LANCZOS).convert("RGB")


# ======================================================================== Titel- und Menuehintergrund
def make_title(sprites):
    k = 2
    W, H = 400 * k, 240 * k
    img = vgrad(W, H, [(0, (110, 190, 255, 255)), (0.55, (200, 235, 255, 255)), (0.56, (100, 190, 70, 255)), (1, (50, 125, 40, 255))])
    d = ImageDraw.Draw(img)
    rnd = random.Random(9)
    for _ in range(4):
        cx, cy = rnd.uniform(0, W), rnd.uniform(10, 70) * k
        for j in range(4):
            d.ellipse([cx + j * 18 * k - 25 * k, cy - 12 * k, cx + j * 18 * k + 10 * k, cy + 14 * k], fill=(255, 255, 255, 230))
    for _ in range(300):
        x, y = rnd.uniform(0, W), rnd.uniform(140 * k, H)
        d.line([(x, y), (x - 2 * k, y - 5 * k)], fill=(60, 140, 50, 255), width=k)
    pm = path_mask(W, H, [(-20, 205), (120, 196), (260, 212), (420, 200)], 26, k)
    pt = Image.new("RGBA", img.size, (220, 190, 135, 255))
    pt.putalpha(pm)
    img.alpha_composite(pt)
    for name, x, y, sc in [("tw0", 40, 172, 2.2), ("tw1", 352, 168, 2.0), ("en2", 120, 200, 2.6), ("en4", 200, 200, 2.4),
                           ("en1", 270, 206, 2.6), ("en0", 312, 204, 2.8), ("en5", 160, 160, 1.3)]:
        im = sprites[name]
        im = im.resize((int(im.width * sc * k), int(im.height * sc * k)), Image.LANCZOS)
        if name.startswith("tw"):
            im = im.rotate(-90 if name == "tw1" else 0, expand=True) if name == "tw1" else im
        img.alpha_composite(im, (int(x * k - im.width / 2), int(y * k - im.height / 2)))
    t1, m1 = logo_text("GOO", 62 * k, [(0, (230, 255, 170, 255)), (0.5, (124, 232, 90, 255)), (1, (40, 150, 40, 255))],
                       stroke=6 * k, stroke_col=(16, 50, 16), shadow=(3 * k, 5 * k), spacing=2 * k)
    t2, _ = logo_text("ALERT", 50 * k, [(0, (255, 250, 200, 255)), (0.5, (255, 210, 70, 255)), (1, (240, 120, 30, 255))],
                      stroke=6 * k, stroke_col=(70, 25, 5), shadow=(3 * k, 5 * k), spacing=2 * k)
    paste_center(img, t1, 108 * k, 58 * k)
    paste_center(img, t2, 282 * k, 74 * k)
    return img.resize((400, 240), Image.LANCZOS).convert("RGB")


def make_menubg():
    W, H = 320, 240
    img = vgrad(W, H, [(0, (40, 30, 90, 255)), (1, (20, 60, 90, 255))])
    lay = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    rnd = random.Random(3)
    for _ in range(26):
        x, y, r = rnd.uniform(0, W), rnd.uniform(0, H), rnd.uniform(8, 30)
        c = rnd.choice([(124, 232, 90), (74, 200, 240), (240, 208, 64), (224, 80, 140), (154, 90, 240)])
        d.ellipse([x - r, y - r * 0.85, x + r, y + r * 0.85], fill=c + (40,))
        d.ellipse([x - r * 0.6, y - r * 0.6, x - r * 0.1, y - r * 0.3], fill=(255, 255, 255, 30))
    img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(1.5)))
    return img.convert("RGB")


# ======================================================================== Ausgabe
def main():
    desk = sys.argv[1] if len(sys.argv) > 1 else None
    maps = json.load(open(os.path.join(HERE, "maps.json")))
    named = {}
    for i, f in enumerate(TOWERS):
        im = f(); add("tw%d" % i, im); named["tw%d" % i] = im
    goo_centers = []
    for i, (r, col, ex) in enumerate(GOO):
        im, c = goo_sprite(r, col, ex); add("en%d" % i, im); named["en%d" % i] = im; goo_centers.append(c)
    for name, f in [("helmet", helmet), ("iceblock", ice_block), ("glow", glow), ("ring", ring), ("splat", splat), ("spark", spark)]:
        add(name, f())
    for p in ["spike", "blade", "bomb", "minibomb", "shard", "boomer", "slime", "ice", "shuriken", "magic", "fireball", "pile", "laser"]:
        add("p_" + p, proj(p))
    for n in ["coin", "heart", "jar", "star", "crown", "medal0", "medal1", "medal2", "lock", "skull", "play", "ff", "pause", "gear", "trophy", "book"]:
        add("i_" + n, icon(n))
    # PNGs + t3s
    for name, im in SPRITES:
        im.save(os.path.join(PNG, "spr", name + ".png"))
    with open(os.path.join(HERE, "sprites.t3s"), "w") as f:
        f.write("--atlas -f rgba8 -z auto\n")
        for name, _ in SPRITES:
            f.write("png/spr/%s.png\n" % name)
    bigs = []
    for i, mp in enumerate(maps):
        im = make_bg(i, mp)
        im.save(os.path.join(PNG, "bg%d.png" % i))
        bigs.append(("bg%d" % i, im))
    tim = make_title(named)
    tim.save(os.path.join(PNG, "title.png")); bigs.append(("title", tim))
    mim = make_menubg()
    mim.save(os.path.join(PNG, "menubg.png")); bigs.append(("menubg", mim))
    for name, _ in bigs:
        with open(os.path.join(HERE, name + ".t3s"), "w") as f:
            f.write("--atlas -f rgb565 -z auto\npng/%s.png\n" % name)
    # Index-Header
    with open(os.path.join(ROOT, "source", "gfx_idx.h"), "w") as f:
        f.write("// automatisch erzeugt von gfx/make_gfx.py\n#pragma once\nenum {\n")
        for i, (name, im) in enumerate(SPRITES):
            f.write("\tSPR_%s = %d,\n" % (name.upper(), i))
        f.write("\tNSPRITE = %d\n};\n" % len(SPRITES))
        f.write("// Mittelpunkt des Goo-Koerpers im Sprite (x, y)\nstatic const float GOO_CENTER[8][2] = {%s};\n" %
                ", ".join("{%.2ff, %.2ff}" % c for c in goo_centers))
        f.write("#define NBIGSHEET %d\n" % len(bigs))
    # Desktop-Dateien fuer den Test-Shim
    if desk:
        os.makedirs(desk, exist_ok=True)
        def dump(path, ims):
            with open(path, "wb") as f:
                f.write(struct.pack("<I", len(ims)))
                for im in ims:
                    im = im.convert("RGBA")
                    f.write(struct.pack("<HH", im.width, im.height))
                    f.write(im.tobytes())
        dump(os.path.join(desk, "sprites.bin"), [im for _, im in SPRITES])
        for name, im in bigs:
            dump(os.path.join(desk, name + ".bin"), [im])
    print("Sprites:", len(SPRITES), "Grossbilder:", len(bigs))


if __name__ == "__main__":
    main()
