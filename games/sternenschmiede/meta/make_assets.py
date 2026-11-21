# Star Forge: icon.png (48x48), banner.png (256x128), banner.wav
import math, os, struct, sys, wave
from PIL import Image, ImageDraw, ImageFilter
D = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(D, "..", "..", "..", "tools"))
from artlib import *


def space(w, h, seed):
    img = radial(w, h, w * 0.3, h * 0.45, max(w, h) * 0.9, [(0, (40, 30, 90, 255)), (0.5, (14, 14, 44, 255)), (1, (4, 6, 18, 255))])
    nebula(img, [((176, 112, 255, 90), 3), ((58, 106, 255, 90), 3), ((255, 106, 213, 50), 2)], seed)
    stars(img, int(w * h / 900 / (w / 256)), seed)
    return img


def ship(img, x, y, s, ang):
    c, sn = math.cos(ang), math.sin(ang)
    def P(a, b): return (x + a * s * c - b * s * sn, y + a * s * sn + b * s * c)
    # Triebwerksspur
    lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(lay).polygon([P(-6, -3), P(-60, 0), P(-6, 3)], fill=(120, 220, 255, 150))
    img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(s * 1.5)))
    d = ImageDraw.Draw(img)
    d.polygon([P(-6, -3.5), P(-16, 0), P(-6, 3.5)], fill=(255, 200, 90, 255))
    d.polygon([P(-7, -10), P(4, -3), P(-3, 0)], fill=(60, 110, 220, 255))
    d.polygon([P(-7, 10), P(4, 3), P(-3, 0)], fill=(50, 90, 200, 255))
    d.polygon([P(12, 0), P(-7, -5), P(-7, 5)], fill=(225, 235, 255, 255))
    d.polygon([P(12, 0), P(-7, 0), P(-7, 5)], fill=(170, 185, 220, 255))
    cx, cy = P(3, 0)
    d.ellipse([cx - 2.4 * s, cy - 2.4 * s, cx + 2.4 * s, cy + 2.4 * s], fill=(138, 232, 255, 255))


def orbit(img, cx, cy, a, b, col, width, planets):
    lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    d.ellipse([cx - a, cy - b, cx + a, cy + b], outline=col + (150,), width=width)
    img.alpha_composite(lay)
    for t, r, pc in planets:
        x, y = cx + a * math.cos(t), cy + b * math.sin(t)
        glow_dot(img, x, y, r * 2, pc, 120)
        p = radial(int(r * 2) + 2, int(r * 2) + 2, r * 0.7, r * 0.6, r * 1.6, [(0, tuple(min(255, v + 80) for v in pc) + (255,)), (1, tuple(v // 3 for v in pc) + (255,))])
        m = Image.new("L", p.size, 0)
        ImageDraw.Draw(m).ellipse([0, 0, r * 2, r * 2], fill=255)
        p.putalpha(m)
        img.alpha_composite(p, (int(x - r), int(y - r)))


# ---- Icon (8x)
S = 8
ic = space(48 * S, 48 * S, 3)
orbit(ic, 24 * S, 26 * S, 21 * S, 7 * S, (255, 210, 90), S, [(0.5, 2.2 * S, (255, 170, 80)), (3.5, 1.6 * S, (180, 120, 255))])
crystal(ic, 24 * S, 23 * S, 12.5 * S)
ship(ic, 37 * S, 38 * S, 0.55 * S, -0.7)
save_small(ic, os.path.join(D, "icon.png"), (48, 48))

# ---- Banner (4x)
S = 4
W, H = 256 * S, 128 * S
bn = space(W, H, 7)
orbit(bn, 58 * S, 66 * S, 50 * S, 15 * S, (255, 210, 90), S, [(0.35, 4 * S, (255, 160, 80)), (2.4, 3 * S, (120, 200, 255)), (4.3, 2.5 * S, (200, 130, 255))])
crystal(bn, 58 * S, 60 * S, 27 * S)
orbit(bn, 58 * S, 66 * S, 50 * S, 15 * S, (255, 210, 90), 0, [(0.35, 4 * S, (255, 160, 80))])
ship(bn, 228 * S, 104 * S, 1.1 * S, -0.35)
t1, _ = logo_text("STAR", 31 * S, [(0, (255, 255, 255, 255)), (0.55, (150, 230, 255, 255)), (1, (60, 120, 255, 255))],
                  stroke=3 * S, stroke_col=(10, 16, 50), glow=6 * S, glow_col=(74, 216, 255, 170), shadow=(2 * S, 3 * S), spacing=1 * S)
t2, _ = logo_text("FORGE", 31 * S, [(0, (255, 245, 200, 255)), (0.55, (255, 200, 90, 255)), (1, (230, 110, 40, 255))],
                  stroke=3 * S, stroke_col=(40, 16, 6), glow=6 * S, glow_col=(255, 160, 60, 150), shadow=(2 * S, 3 * S), spacing=1 * S)
paste_center(bn, t1, 178 * S, 33 * S)
paste_center(bn, t2, 178 * S, 68 * S)
sub, _ = logo_text("IDLE  ·  EXPEDITION", 9 * S, [(0, (220, 230, 255, 255)), (1, (160, 180, 230, 255))], stroke=1 * S,
                   stroke_col=(10, 16, 50), font=FONT_XBOLD, highlight=0, spacing=1 * S)
paste_center(bn, sub, 178 * S, 96 * S)
glow_dot(bn, 118 * S, 18 * S, 5 * S, (255, 255, 255), 230)
save_small(bn, os.path.join(D, "banner.png"), (256, 128))

# ---- Sound
rate, n = 22050, int(22050 * 1.6)
notes = [(0.0, 660), (0.12, 880), (0.24, 990), (0.36, 1320)]
out = []
for i in range(n):
    t = i / rate
    v = sum((math.sin(2 * math.pi * f * (t - st)) + 0.3 * math.sin(4 * math.pi * f * (t - st))) * math.exp(-(t - st) * 3.5) * 0.22 for st, f in notes if t >= st)
    out.append(max(-1, min(1, v)))
with wave.open(os.path.join(D, "banner.wav"), "wb") as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
    w.writeframes(b"".join(struct.pack("<h", int(s * 30000)) for s in out))
print("ok")
