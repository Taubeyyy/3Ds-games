# Erzeugt icon.png (48x48), banner.png (256x128) und banner.wav
import math, random, struct, wave, os
from PIL import Image, ImageDraw, ImageFilter, ImageFont

D = os.path.dirname(os.path.abspath(__file__))
random.seed(7)
FB = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
FR = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"


def space(w, h):
    img = Image.new("RGBA", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            t = y / h
            d = math.hypot(x - w * 0.35, y - h * 0.5) / max(w, h)
            r = int(8 + 18 * t + 30 * max(0, 0.5 - d))
            g = int(10 + 6 * t + 20 * max(0, 0.5 - d))
            b = int(30 + 34 * t + 70 * max(0, 0.5 - d))
            px[x, y] = (r, g, b, 255)
    neb = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    nd = ImageDraw.Draw(neb)
    for _ in range(5):
        cx, cy, r = random.uniform(0, w), random.uniform(0, h), random.uniform(w * 0.15, w * 0.35)
        col = random.choice([(176, 112, 255, 60), (58, 106, 255, 60), (255, 106, 213, 40)])
        nd.ellipse([cx - r, cy - r, cx + r, cy + r], fill=col)
    neb = neb.filter(ImageFilter.GaussianBlur(w * 0.08))
    img = Image.alpha_composite(img, neb)
    d = ImageDraw.Draw(img)
    for _ in range(int(w * h / 350)):
        x, y = random.uniform(0, w), random.uniform(0, h)
        s = random.random() ** 3 * 2.5 * (w / 256) + 0.6
        a = random.randint(120, 255)
        d.ellipse([x - s / 2, y - s / 2, x + s / 2, y + s / 2], fill=(255, 255, 255, a))
    return img


def crystal(img, cx, cy, s, col=(74, 216, 255)):
    glow = Image.new("RGBA", img.size, (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    for k in range(6, 0, -1):
        r = s * (0.6 + k * 0.28)
        gd.ellipse([cx - r, cy - r, cx + r, cy + r], fill=col + (int(22 + 6 * (6 - k)),))
    glow = glow.filter(ImageFilter.GaussianBlur(s * 0.25))
    img.alpha_composite(glow)
    d = ImageDraw.Draw(img)
    top, bot = (cx, cy - s * 1.2), (cx, cy + s * 1.2)
    l, r, m = (cx - s * 0.8, cy - s * 0.12), (cx + s * 0.8, cy - s * 0.12), (cx + s * 0.18, cy - s * 0.12)

    def lt(c, t):
        return tuple(int(v + (255 - v) * t) for v in c) + (255,)

    def dk(c, t):
        return tuple(int(v * (1 - t)) for v in c) + (255,)
    d.polygon([top, l, m], fill=lt(col, 0.7))
    d.polygon([top, m, r], fill=lt(col, 0.3))
    d.polygon([l, bot, m], fill=col + (255,))
    d.polygon([m, bot, r], fill=dk(col, 0.45))
    d.line([l, r], fill=(255, 255, 255, 120), width=max(1, int(s / 20)))
    d.line([top, m], fill=(255, 255, 255, 160), width=max(1, int(s / 24)))


def orbit(img, cx, cy, a, b, col, width, dots):
    d = ImageDraw.Draw(img)
    d.ellipse([cx - a, cy - b, cx + a, cy + b], outline=col + (110,), width=width)
    for t in dots:
        x, y = cx + a * math.cos(t), cy + b * math.sin(t)
        rr = width * 2.2
        d.ellipse([x - rr, y - rr, x + rr, y + rr], fill=col + (255,))


# ---- Icon (4x ueberabgetastet)
S = 4
ic = space(48 * S, 48 * S)
orbit(ic, 24 * S, 27 * S, 21 * S, 7 * S, (255, 210, 90), S, [0.6, 3.6])
crystal(ic, 24 * S, 24 * S, 14 * S)
ic = ic.resize((48, 48), Image.LANCZOS)
ic.save(os.path.join(D, "icon.png"))

# ---- Banner
S = 3
W, H = 256 * S, 128 * S
bn = space(W, H)
orbit(bn, 62 * S, 68 * S, 52 * S, 15 * S, (255, 210, 90), S, [0.4, 2.2, 4.4])
orbit(bn, 62 * S, 68 * S, 38 * S, 11 * S, (176, 112, 255), S, [1.2, 5.0])
crystal(bn, 62 * S, 62 * S, 30 * S)
d = ImageDraw.Draw(bn)
f1 = ImageFont.truetype(FB, 21 * S)
f2 = ImageFont.truetype(FR, 11 * S)
title = "Sternen-\nschmiede"
for off, c in (((2 * S, 2 * S), (10, 8, 30, 200)), ((0, 0), (255, 255, 255, 255))):
    d.multiline_text((120 * S + off[0], 30 * S + off[1]), title, font=f1, fill=c, spacing=2 * S)
d.text((121 * S, 88 * S), "Kosmisches Idle-Spiel", font=f2, fill=(74, 216, 255, 255))
bn = bn.resize((256, 128), Image.LANCZOS)
bn.save(os.path.join(D, "banner.png"))

# ---- Banner-Sound (kurzer Glockenklang)
rate = 22050
n = int(rate * 1.6)
notes = [(0.0, 660), (0.12, 880), (0.24, 990), (0.36, 1320)]
samples = []
for i in range(n):
    t = i / rate
    v = 0.0
    for st, f in notes:
        if t >= st:
            tt = t - st
            v += (math.sin(2 * math.pi * f * tt) + 0.3 * math.sin(4 * math.pi * f * tt)) * math.exp(-tt * 3.5) * 0.22
    samples.append(max(-1, min(1, v)))
with wave.open(os.path.join(D, "banner.wav"), "wb") as w:
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(rate)
    w.writeframes(b"".join(struct.pack("<h", int(s * 30000)) for s in samples))
print("ok")
