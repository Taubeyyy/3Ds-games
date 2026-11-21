# Texturen der Physik-Demo: Gras-Detail (Graustufen, Farbe kommt aus den Vertices), Holzkiste, Stein, Leuchtpunkt
import os, sys, random
D = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(D, "..", "..", "..", "..", "tools"))
from artlib import radial
from PIL import Image, ImageDraw, ImageFilter
P = os.path.join(D, "png"); os.makedirs(P, exist_ok=True)
rnd = random.Random(11)

def noise(w, h, amp, blur):
    im = Image.new("L", (w, h)); im.putdata([128 + rnd.randint(-amp, amp) for _ in range(w * h)])
    return im.filter(ImageFilter.GaussianBlur(blur))

# Gras-Detail: hell mit feinen Halmen, kachelbar
g = Image.new("L", (128, 128), 215)
d = ImageDraw.Draw(g)
for _ in range(900):
    x, y = rnd.randint(0, 127), rnd.randint(0, 127)
    l = rnd.randint(2, 5); v = rnd.randint(150, 255)
    for ox in (-128, 0, 128):
        for oy in (-128, 0, 128):
            d.line([(x + ox, y + oy), (x + ox + rnd.randint(-1, 1), y + oy - l)], fill=v)
g = Image.blend(g, noise(128, 128, 50, 2), 0.35)
Image.merge("RGB", [g, g, g]).save(os.path.join(P, "grass.png"))

# Holzkiste
s = 2; W = 64 * s
c = Image.new("RGB", (W, W), (120, 78, 40)); d = ImageDraw.Draw(c)
for i in range(4):
    k = rnd.uniform(0.85, 1.1)
    d.rectangle([0, i * W // 4 + s, W, (i + 1) * W // 4 - s], fill=(int(176 * k), int(118 * k), int(62 * k)))
fr = (92, 58, 30)
d.rectangle([0, 0, W - 1, W - 1], outline=fr, width=7 * s)
d.line([(6 * s, 6 * s), (W - 6 * s, W - 6 * s)], fill=fr, width=6 * s)
for x, y in [(4, 4), (60, 4), (4, 60), (60, 60)]:
    d.ellipse([x * s - 2 * s, y * s - 2 * s, x * s + 2 * s, y * s + 2 * s], fill=(200, 200, 190))
c = Image.merge("RGB", [Image.blend(ch, noise(W, W, 40, 1), 0.15) for ch in c.split()])
c.resize((64, 64), Image.LANCZOS).save(os.path.join(P, "crate.png"))

# Steinblöcke (Treppe, Rampe)
st = Image.new("RGB", (64 * s, 64 * s), (40, 36, 46)); d = ImageDraw.Draw(st)
for r in range(4):
    off = (r % 2) * 16 * s
    for cc in range(-1, 3):
        k = rnd.uniform(0.85, 1.1)
        x0 = cc * 32 * s + off + s
        d.rectangle([x0, r * 16 * s + s, x0 + 30 * s, r * 16 * s + 15 * s], fill=(int(150 * k), int(145 * k), int(160 * k)))
st = Image.merge("RGB", [Image.blend(ch, noise(64 * s, 64 * s, 40, 1), 0.2) for ch in st.split()])
st.resize((64, 64), Image.LANCZOS).save(os.path.join(P, "stone.png"))

radial(32, 32, 16, 16, 16, [(0, (255, 255, 255, 255)), (0.4, (255, 255, 255, 140)), (1, (255, 255, 255, 0))]).save(os.path.join(P, "glow.png"))
for n in ("grass", "crate", "stone"):
    open(os.path.join(D, n + ".t3s"), "w").write("-f rgba8 -m lanczos -z auto\npng/%s.png\n" % n)
open(os.path.join(D, "sprites.t3s"), "w").write("--atlas -f rgba8 -z auto\npng/glow.png\n")
# Icon
ic = radial(48, 48, 24, 18, 32, [(0, (170, 255, 150, 255)), (0.6, (60, 150, 70, 255)), (1, (20, 40, 30, 255))])
dd = ImageDraw.Draw(ic)
dd.rectangle([10, 22, 26, 38], fill=(176, 118, 62), outline=(92, 58, 30), width=2)
dd.ellipse([26, 10, 40, 24], fill=(255, 210, 90), outline=(120, 80, 20), width=2)
ic.save(os.path.join(D, "..", "meta", "icon.png"))
