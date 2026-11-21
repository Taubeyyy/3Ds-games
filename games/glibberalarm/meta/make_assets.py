# Goo Alert: icon.png (48x48), banner.png (256x128), banner.wav
import math, os, struct, sys, wave
from PIL import Image, ImageDraw, ImageFilter
D = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(D, "..", "..", "..", "tools"))
from artlib import *


def meadow(w, h, horizon):
    img = vgrad(w, h, [(0, (120, 200, 255, 255)), (horizon / h * 0.95, (200, 240, 255, 255)), (horizon / h, (110, 200, 80, 255)), (1, (50, 130, 40, 255))])
    d = ImageDraw.Draw(img)
    rnd = __import__("random").Random(5)
    for _ in range(int(w * h / 4000)):
        x, y = rnd.uniform(0, w), rnd.uniform(horizon, h)
        r = rnd.uniform(0.02, 0.06) * w
        d.ellipse([x - r, y - r * 0.4, x + r, y + r * 0.4], fill=(70, 160, 55, 120))
    # Wolken
    lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    dl = ImageDraw.Draw(lay)
    for cx, cy, s in [(0.12, 0.12, 0.08), (0.55, 0.08, 0.06), (0.9, 0.16, 0.07)]:
        for k in range(4):
            dl.ellipse([(cx + k * s * 0.5 - s) * w, (cy - s * 0.5) * h, (cx + k * s * 0.5 + s * 0.3) * w, (cy + s * 0.9) * h], fill=(255, 255, 255, 200))
    img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(w * 0.004)))
    return img


def path(img, pts, wdt):
    d = ImageDraw.Draw(img)
    d.line(pts, fill=(160, 130, 80, 255), width=int(wdt * 1.25), joint="curve")
    d.line(pts, fill=(222, 196, 140, 255), width=int(wdt), joint="curve")


def hedgehog(img, cx, cy, r):
    d = ImageDraw.Draw(img)
    for k in range(11):
        a = math.pi * 0.95 + k * math.pi * 0.11
        d.polygon([(cx + math.cos(a - 0.2) * r * 0.8, cy + math.sin(a - 0.2) * r * 0.8), (cx + math.cos(a) * r * 1.6, cy + math.sin(a) * r * 1.6),
                   (cx + math.cos(a + 0.2) * r * 0.8, cy + math.sin(a + 0.2) * r * 0.8)], fill=(80, 50, 28, 255))
    b = radial(int(r * 2) + 2, int(r * 2) + 2, r * 0.7, r * 0.6, r * 1.5, [(0, (220, 170, 110, 255)), (1, (120, 80, 45, 255))])
    m = Image.new("L", b.size, 0)
    ImageDraw.Draw(m).ellipse([0, 0, r * 2, r * 2], fill=255)
    b.putalpha(m)
    img.alpha_composite(b, (int(cx - r), int(cy - r)))
    d.ellipse([cx + r * 0.1, cy - r * 0.45, cx + r * 1.08, cy + r * 0.5], fill=(240, 216, 176, 255))
    d.ellipse([cx + r * 0.95, cy - r * 0.08, cx + r * 1.22, cy + r * 0.18], fill=(42, 26, 16, 255))
    d.ellipse([cx + r * 0.38, cy - r * 0.32, cx + r * 0.64, cy - r * 0.04], fill=(16, 16, 26, 255))
    d.ellipse([cx + r * 0.45, cy - r * 0.28, cx + r * 0.52, cy - r * 0.2], fill=(255, 255, 255, 255))


def spike(img, x, y, ang, L):
    d = ImageDraw.Draw(img)
    c, s = math.cos(ang), math.sin(ang)
    d.line([(x - c * L, y - s * L), (x, y)], fill=(70, 45, 25, 255), width=max(2, int(L / 6)))
    lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(lay).line([(x - c * L * 2.2, y - s * L * 2.2), (x - c * L, y - s * L)], fill=(255, 255, 255, 120), width=max(2, int(L / 5)))
    img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(L * 0.15)))


def drips(img, mask, col, dark, seed, n, size):
    rnd = __import__("random").Random(seed)
    bbox = mask.getbbox()
    lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    px = mask.load()
    tries = 0
    while n > 0 and tries < 400:
        tries += 1
        x = int(rnd.uniform(bbox[0] + size, bbox[2] - size))
        y = bbox[3] - 1
        while y > bbox[1] and px[x, y] < 128: y -= 1
        if y <= bbox[1] + (bbox[3] - bbox[1]) * 0.6: continue
        L = rnd.uniform(1.2, 3.0) * size
        w = rnd.uniform(0.5, 0.8) * size
        d.rectangle([x - w / 2, y - size, x + w / 2, y + L], fill=dark)
        d.ellipse([x - w * 0.75, y + L - w * 0.6, x + w * 0.75, y + L + w * 0.9], fill=dark)
        d.rectangle([x - w / 2 + size * 0.15, y - size, x + w / 2 - size * 0.15, y + L], fill=col)
        d.ellipse([x - w * 0.55, y + L - w * 0.45, x + w * 0.55, y + L + w * 0.7], fill=col)
        n -= 1
    return lay


GREEN = (124, 232, 90)
# ---- Icon (8x)
S = 8
ic = radial(48 * S, 48 * S, 24 * S, 20 * S, 34 * S, [(0, (255, 240, 150, 255)), (0.6, (255, 190, 70, 255)), (1, (230, 110, 40, 255))])
lay = Image.new("RGBA", ic.size, (0, 0, 0, 0))
dl = ImageDraw.Draw(lay)
for k in range(12):
    a = k * math.pi / 6
    dl.polygon([(24 * S, 20 * S), (24 * S + math.cos(a - 0.12) * 60 * S, 20 * S + math.sin(a - 0.12) * 60 * S),
                (24 * S + math.cos(a + 0.12) * 60 * S, 20 * S + math.sin(a + 0.12) * 60 * S)], fill=(255, 255, 255, 45))
ic.alpha_composite(lay)
ImageDraw.Draw(ic).ellipse([-10 * S, 38 * S, 58 * S, 60 * S], fill=(90, 180, 60, 255))
blob(ic, 26 * S, 29 * S, 15 * S, GREEN, look=(-1, 0), angry=True)
spike(ic, 13 * S, 19 * S, 0.5, 5 * S)
spike(ic, 9 * S, 27 * S, 0.2, 4 * S)
save_small(ic, os.path.join(D, "icon.png"), (48, 48))

# ---- Banner (4x)
S = 4
W, H = 256 * S, 128 * S
bn = meadow(W, H, 62 * S)
path(bn, [(-10 * S, 112 * S), (90 * S, 108 * S), (170 * S, 116 * S), (270 * S, 106 * S)], 20 * S)
hedgehog(bn, 30 * S, 100 * S, 11 * S)
spike(bn, 62 * S, 98 * S, 0.0, 6 * S)
blob(bn, 108 * S, 104 * S, 9 * S, (240, 208, 64), look=(-1, 0))
blob(bn, 148 * S, 101 * S, 13 * S, (224, 80, 140), look=(-1, 0), angry=True)
blob(bn, 190 * S, 106 * S, 8 * S, (74, 200, 240), look=(-1, 0))
blob(bn, 228 * S, 100 * S, 11 * S, GREEN, look=(-1, 0), angry=True)
t1, m1 = logo_text("GOO", 38 * S, [(0, (230, 255, 170, 255)), (0.5, (124, 232, 90, 255)), (1, (40, 150, 40, 255))],
                   stroke=4 * S, stroke_col=(16, 50, 16), glow=0, shadow=(2 * S, 3 * S), spacing=1 * S)
t1.alpha_composite(drips(t1, m1, (110, 220, 80, 255), (16, 50, 16, 255), 3, 3, 3.2 * S))
t2, m2 = logo_text("ALERT", 30 * S, [(0, (255, 250, 200, 255)), (0.5, (255, 210, 70, 255)), (1, (240, 120, 30, 255))],
                   stroke=4 * S, stroke_col=(70, 25, 5), glow=0, shadow=(2 * S, 3 * S), spacing=1 * S)
paste_center(bn, t1, 80 * S, 28 * S)
paste_center(bn, t2, 186 * S, 32 * S)
rib, _ = logo_text("TOWER DEFENSE", 9 * S, [(0, (255, 255, 255, 255)), (1, (255, 230, 230, 255))], stroke=0, font=FONT_XBOLD, highlight=0, spacing=1 * S)
d = ImageDraw.Draw(bn)
d.polygon([(70 * S, 55 * S), (190 * S, 55 * S), (196 * S, 61 * S), (190 * S, 67 * S), (70 * S, 67 * S), (64 * S, 61 * S)], fill=(200, 40, 50, 255))
d.polygon([(70 * S, 55 * S), (190 * S, 55 * S), (190 * S, 58 * S), (70 * S, 58 * S)], fill=(240, 90, 90, 255))
paste_center(bn, rib, 130 * S, 61 * S)
save_small(bn, os.path.join(D, "banner.png"), (256, 128))

# ---- Sound: Blubber-Plopps
rate = 22050
n = int(rate * 1.4)
pops = [(0.0, 500), (0.18, 640), (0.36, 760), (0.6, 980)]
out = []
for i in range(n):
    t = i / rate
    v = sum(math.sin(2 * math.pi * f * (1 - (t - st) * 2) * (t - st)) * math.exp(-(t - st) * 18) * 0.5 for st, f in pops if t >= st)
    out.append(max(-1, min(1, v)))
with wave.open(os.path.join(D, "banner.wav"), "wb") as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
    w.writeframes(b"".join(struct.pack("<h", int(s * 30000)) for s in out))
print("ok")
