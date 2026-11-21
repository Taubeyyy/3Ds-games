# Erzeugt die Texturen der 3D-Demo (Steinboden, Säulenstein, Leuchtpunkt) und das Icon
import os, sys, random
D = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(D, "..", "..", "..", "..", "tools"))
from artlib import radial
from PIL import Image, ImageDraw, ImageFilter
P = os.path.join(D, "png"); os.makedirs(P, exist_ok=True)
rnd = random.Random(7)

def noise(w, h, amp, blur):
    im = Image.new("L", (w, h))
    im.putdata([128 + rnd.randint(-amp, amp) for _ in range(w * h)])
    return im.filter(ImageFilter.GaussianBlur(blur))

def stone_tiles(path, size, tiles, base, grout):
    s = 2; W = size * s
    im = Image.new("RGB", (W, W), grout)
    d = ImageDraw.Draw(im)
    t = W // tiles
    for j in range(tiles):
        for i in range(tiles):
            k = rnd.uniform(0.82, 1.12)
            c = tuple(min(255, int(v * k)) for v in base)
            x0, y0 = i * t + 3 * s, j * t + 3 * s
            d.rounded_rectangle([x0, y0, x0 + t - 6 * s, y0 + t - 6 * s], radius=4 * s, fill=c)
            # Kantenlicht oben/links, Schatten unten/rechts
            d.line([x0 + 3 * s, y0 + s, x0 + t - 9 * s, y0 + s], fill=tuple(min(255, v + 28) for v in c), width=s)
            d.line([x0 + 3 * s, y0 + t - 7 * s, x0 + t - 9 * s, y0 + t - 7 * s], fill=tuple(max(0, v - 30) for v in c), width=s)
            for _ in range(3):  # Risse
                cx, cy = x0 + rnd.randint(4, t - 12) * 1, y0 + rnd.randint(4, t - 12)
                pts = [(cx, cy)]
                for _ in range(4):
                    cx += rnd.randint(-5, 5) * s; cy += rnd.randint(-5, 5) * s
                    pts.append((cx, cy))
                d.line(pts, fill=tuple(max(0, v - 40) for v in c), width=1)
    n = noise(W, W, 40, 1.2)
    im = Image.merge("RGB", [Image.blend(ch, n, 0.18) for ch in im.split()])
    im.resize((size, size), Image.LANCZOS).save(path)

def bricks(path, w, h, base):
    s = 2; W, H = w * s, h * s
    im = Image.new("RGB", (W, H), (30, 26, 34))
    d = ImageDraw.Draw(im)
    bh = H // 8
    for r in range(8):
        off = (r % 2) * (W // 4)
        for c in range(-1, 3):
            k = rnd.uniform(0.8, 1.15)
            col = tuple(min(255, int(v * k)) for v in base)
            x0 = c * (W // 2) + off + 2 * s
            d.rectangle([x0, r * bh + 2 * s, x0 + W // 2 - 4 * s, r * bh + bh - 2 * s], fill=col)
    # Leuchtrune in der Mitte
    d.line([(W // 2, H * 0.3), (W // 2, H * 0.7)], fill=(120, 220, 255), width=3 * s)
    d.line([(W * 0.35, H * 0.42), (W * 0.65, H * 0.58)], fill=(120, 220, 255), width=3 * s)
    n = noise(W, H, 40, 1)
    im = Image.merge("RGB", [Image.blend(ch, n, 0.15) for ch in im.split()])
    im.resize((w, h), Image.LANCZOS).save(path)

stone_tiles(os.path.join(P, "floor.png"), 128, 4, (120, 110, 132), (36, 30, 44))
bricks(os.path.join(P, "pillar.png"), 64, 128, (150, 130, 120))
g = radial(32, 32, 16, 16, 16, [(0, (255, 255, 255, 255)), (0.4, (255, 255, 255, 140)), (1, (255, 255, 255, 0))])
g.save(os.path.join(P, "glow.png"))
open(os.path.join(D, "floor.t3s"), "w").write("-f rgba8 -m lanczos -z auto\npng/floor.png\n")
open(os.path.join(D, "pillar.t3s"), "w").write("-f rgba8 -m lanczos -z auto\npng/pillar.png\n")
open(os.path.join(D, "sprites.t3s"), "w").write("--atlas -f rgba8 -z auto\npng/glow.png\n")
# Icon 48x48
ic = radial(48, 48, 24, 20, 30, [(0, (255, 200, 120, 255)), (0.5, (200, 80, 60, 255)), (1, (40, 20, 60, 255))])
dd = ImageDraw.Draw(ic)
dd.polygon([(24, 8), (40, 18), (40, 32), (24, 42), (8, 32), (8, 18)], outline=(255, 240, 220), width=2)
dd.line([(24, 8), (24, 26), (40, 18)], fill=(255, 240, 220), width=2); dd.line([(24, 26), (8, 18)], fill=(255, 240, 220), width=2)
dd.line([(24, 26), (24, 42)], fill=(255, 240, 220), width=2)
ic.save(os.path.join(D, "..", "meta", "icon.png"))

# 3D-Modell als OBJ (Quelle) → data/halo.fmesh über tools/meshtool.py
import math, subprocess
M = os.path.join(D, "models"); os.makedirs(M, exist_ok=True)
open(os.path.join(M, "halo.mtl"), "w").write("newmtl gold\nKd 1.0 0.78 0.35\nnewmtl gem\nKd 0.45 0.95 1.0\n")
V, F = [], []
def vert(x, y, z):
    V.append((x, y, z)); return len(V)
R, r, NS, NR = 1.3, 0.07, 40, 8
ring = [[vert((R + r * math.cos(b)) * math.cos(a), r * math.sin(b), (R + r * math.cos(b)) * math.sin(a))
         for b in [2 * math.pi * j / NR for j in range(NR)]] for a in [2 * math.pi * i / NS for i in range(NS)]]
F.append("usemtl gold")
for i in range(NS):
    for j in range(NR):
        a, b = ring[i][j], ring[(i + 1) % NS][j]
        c, d = ring[(i + 1) % NS][(j + 1) % NR], ring[i][(j + 1) % NR]
        F.append("f %d %d %d %d" % (a, d, c, b))
F.append("usemtl gem"); F.append("s off")
for k in range(6):
    a = 2 * math.pi * k / 6
    cx, cz, s = R * math.cos(a), R * math.sin(a), 0.16
    top, bot = vert(cx, s * 1.6, cz), vert(cx, -s * 1.6, cz)
    eq = [vert(cx + s * math.cos(t), 0, cz + s * math.sin(t)) for t in [2 * math.pi * q / 4 for q in range(4)]]
    for q in range(4):
        e0, e1 = eq[q], eq[(q + 1) % 4]
        F.append("f %d %d %d" % (top, e1, e0))
        F.append("f %d %d %d" % (bot, e0, e1))
with open(os.path.join(M, "halo.obj"), "w") as f:
    f.write("mtllib halo.mtl\n" + "".join("v %.4f %.4f %.4f\n" % v for v in V) + "\n".join(F) + "\n")
os.makedirs(os.path.join(D, "..", "data"), exist_ok=True)
subprocess.check_call([sys.executable, os.path.join(D, "..", "..", "..", "tools", "meshtool.py"),
                       os.path.join(M, "halo.obj"), os.path.join(D, "..", "data", "halo.fmesh")])
