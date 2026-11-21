# Erzeugt icon.png (48x48), banner.png (256x128) und banner.wav fuer Glibberalarm
import math, random, struct, wave, os
from PIL import Image, ImageDraw, ImageFilter, ImageFont

D = os.path.dirname(os.path.abspath(__file__))
FB = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
FR = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"


def lt(c, t): return tuple(int(v + (255 - v) * t) for v in c[:3]) + (255,)
def dk(c, t): return tuple(int(v * (1 - t)) for v in c[:3]) + (255,)


def blob(img, cx, cy, r, col, look=(1, 0)):
    sh = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(sh).ellipse([cx - r * 0.9, cy + r * 0.55, cx + r * 0.9, cy + r * 0.95], fill=(0, 0, 0, 90))
    img.alpha_composite(sh.filter(ImageFilter.GaussianBlur(r * 0.12)))
    d = ImageDraw.Draw(img)
    d.ellipse([cx - r * 1.05, cy - r * 0.9, cx + r * 1.05, cy + r * 0.9], fill=dk(col, 0.25))
    d.ellipse([cx - r * 0.98, cy - r * 0.86, cx + r * 0.92, cy + r * 0.78], fill=col + (255,))
    d.ellipse([cx - r * 0.6, cy - r * 0.65, cx - r * 0.05, cy - r * 0.3], fill=(255, 255, 255, 120))
    for k in (-1, 1):
        ex, ey = cx + k * r * 0.34 + look[0] * r * 0.12, cy - r * 0.05
        d.ellipse([ex - r * 0.24, ey - r * 0.28, ex + r * 0.24, ey + r * 0.28], fill=(255, 255, 255, 255))
        px, py = ex + look[0] * r * 0.1, ey + look[1] * r * 0.1
        d.ellipse([px - r * 0.12, py - r * 0.14, px + r * 0.12, py + r * 0.14], fill=(16, 16, 26, 255))


def hedgehog(img, cx, cy, r):
    d = ImageDraw.Draw(img)
    for k in range(11):
        a = math.pi * 0.95 + k * math.pi * 0.11
        p1 = (cx + math.cos(a - 0.2) * r * 0.8, cy + math.sin(a - 0.2) * r * 0.8)
        p2 = (cx + math.cos(a) * r * 1.55, cy + math.sin(a) * r * 1.55)
        p3 = (cx + math.cos(a + 0.2) * r * 0.8, cy + math.sin(a + 0.2) * r * 0.8)
        d.polygon([p1, p2, p3], fill=(90, 58, 34, 255))
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(176, 128, 80, 255))
    d.ellipse([cx + r * 0.1, cy - r * 0.45, cx + r * 1.05, cy + r * 0.5], fill=(240, 216, 176, 255))
    d.ellipse([cx + r * 0.95, cy - r * 0.08, cx + r * 1.2, cy + r * 0.17], fill=(42, 26, 16, 255))
    d.ellipse([cx + r * 0.4, cy - r * 0.3, cx + r * 0.62, cy - r * 0.06], fill=(16, 16, 26, 255))


def grass(w, h):
    img = Image.new("RGBA", (w, h), (74, 154, 58, 255))
    d = ImageDraw.Draw(img)
    random.seed(3)
    for _ in range(40):
        x, y, rr = random.uniform(0, w), random.uniform(0, h), random.uniform(w * 0.05, w * 0.12)
        d.ellipse([x - rr, y - rr * 0.6, x + rr, y + rr * 0.6], fill=(62, 138, 50, 255))
    return img


# ---- Icon
S = 4
ic = grass(48 * S, 48 * S)
d = ImageDraw.Draw(ic)
d.line([(0, 34 * S), (48 * S, 30 * S)], fill=(168, 140, 90, 255), width=15 * S)
d.line([(0, 34 * S), (48 * S, 30 * S)], fill=(216, 192, 138, 255), width=11 * S)
blob(ic, 30 * S, 28 * S, 12 * S, (124, 232, 90), look=(-1, 0))
hedgehog(ic, 12 * S, 13 * S, 7 * S)
ic = ic.resize((48, 48), Image.LANCZOS)
ic.save(os.path.join(D, "icon.png"))

# ---- Banner
S = 3
W, H = 256 * S, 128 * S
bn = grass(W, H)
d = ImageDraw.Draw(bn)
d.line([(0, 104 * S), (256 * S, 96 * S)], fill=(168, 140, 90, 255), width=26 * S)
d.line([(0, 104 * S), (256 * S, 96 * S)], fill=(216, 192, 138, 255), width=20 * S)
blob(bn, 150 * S, 98 * S, 10 * S, (240, 208, 64), look=(-1, 0))
blob(bn, 186 * S, 96 * S, 14 * S, (224, 80, 140), look=(-1, 0))
blob(bn, 226 * S, 94 * S, 8 * S, (74, 200, 240), look=(-1, 0))
hedgehog(bn, 30 * S, 92 * S, 12 * S)
f1 = ImageFont.truetype(FB, 30 * S)
for off, c in (((3 * S, 3 * S), (20, 40, 10, 220)), ((0, 0), (255, 255, 255, 255))):
    d.text((128 * S + off[0], 30 * S + off[1]), "GLIBBER", font=f1, fill=c, anchor="mm")
for off, c in (((3 * S, 3 * S), (60, 30, 0, 220)), ((0, 0), (255, 210, 90, 255))):
    d.text((128 * S + off[0], 62 * S + off[1]), "ALARM", font=f1, fill=c, anchor="mm")
bn = bn.resize((256, 128), Image.LANCZOS)
bn.save(os.path.join(D, "banner.png"))

# ---- Sound: Blubber-Plopps
rate = 22050
n = int(rate * 1.4)
pops = [(0.0, 500), (0.18, 640), (0.36, 760), (0.6, 980)]
out = []
for i in range(n):
    t = i / rate
    v = 0.0
    for st, f in pops:
        if t >= st:
            tt = t - st
            v += math.sin(2 * math.pi * f * (1 - tt * 2) * tt) * math.exp(-tt * 18) * 0.5
    out.append(max(-1, min(1, v)))
with wave.open(os.path.join(D, "banner.wav"), "wb") as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
    w.writeframes(b"".join(struct.pack("<h", int(s * 30000)) for s in out))
print("ok")
