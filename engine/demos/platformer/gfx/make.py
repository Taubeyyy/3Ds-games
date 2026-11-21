# Sprites fuer das Plattformer-Demo (4x ueberabgetastet)
import os, sys, math
D = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(D, "..", "..", "..", "..", "tools"))
from artlib import radial
from PIL import Image, ImageDraw, ImageFilter
S = 4
def C(w, h): return Image.new("RGBA", (w * S, h * S), (0, 0, 0, 0))
def fin(im, w, h, outline=True):
    im = im.resize((w, h), Image.LANCZOS)
    if not outline: return im
    a = im.getchannel("A").filter(ImageFilter.MaxFilter(3))
    o = Image.new("RGBA", im.size, (24, 16, 30, 0)); o.putalpha(a); o.alpha_composite(im); return o
def tile(kind):
    im = C(16, 16); d = ImageDraw.Draw(im)
    if kind == "grass":
        d.rectangle([0, 0, 64, 64], fill=(150, 100, 60, 255))
        for i in range(10): d.ellipse([(i * 7) % 60, 20 + (i * 13) % 40, (i * 7) % 60 + 6, 26 + (i * 13) % 40], fill=(125, 82, 48, 255))
        d.rectangle([0, 0, 64, 18], fill=(90, 190, 70, 255)); d.rectangle([0, 0, 64, 6], fill=(140, 225, 100, 255))
        for i in range(8): d.polygon([(i * 8, 18), (i * 8 + 4, 26), (i * 8 + 8, 18)], fill=(90, 190, 70, 255))
    elif kind == "dirt":
        d.rectangle([0, 0, 64, 64], fill=(150, 100, 60, 255))
        for i in range(12): d.ellipse([(i * 11) % 58, (i * 17) % 58, (i * 11) % 58 + 6, (i * 17) % 58 + 5], fill=(125, 82, 48, 255))
    elif kind == "stone":
        d.rectangle([0, 0, 64, 64], fill=(120, 125, 140, 255))
        for y in range(0, 64, 16):
            off = 16 if (y // 16) % 2 else 0
            for x in range(-16, 64, 32):
                d.rectangle([x + off + 1, y + 1, x + off + 30, y + 14], fill=(145, 150, 165, 255)); d.line([x + off + 2, y + 2, x + off + 29, y + 2], fill=(175, 180, 195, 255), width=2)
    return im.resize((16, 16), Image.LANCZOS)
def crate():
    im = C(16, 16); d = ImageDraw.Draw(im)
    d.rectangle([2, 2, 62, 62], fill=(190, 130, 60, 255), outline=(110, 70, 30, 255), width=6)
    d.line([8, 8, 56, 56], fill=(140, 90, 40, 255), width=6); d.line([56, 8, 8, 56], fill=(140, 90, 40, 255), width=6)
    return fin(im, 16, 16)
def coin():
    im = C(12, 12); d = ImageDraw.Draw(im)
    d.ellipse([2, 2, 46, 46], fill=(220, 150, 30, 255)); d.ellipse([8, 8, 40, 40], fill=(255, 210, 70, 255)); d.rectangle([22, 14, 26, 34], fill=(215, 150, 30, 255)); d.ellipse([12, 11, 18, 17], fill=(255, 250, 220, 255))
    return fin(im, 12, 12)
def hero():
    im = C(16, 20); d = ImageDraw.Draw(im)
    d.rounded_rectangle([12, 30, 52, 72], radius=14, fill=(70, 120, 230, 255))
    d.ellipse([10, 4, 54, 46], fill=(255, 214, 170, 255))
    d.pieslice([8, 0, 56, 36], 180, 360, fill=(240, 80, 70, 255)); d.rectangle([8, 16, 60, 20], fill=(220, 60, 50, 255))
    d.ellipse([34, 20, 40, 28], fill=(30, 24, 40, 255)); d.ellipse([22, 20, 28, 28], fill=(30, 24, 40, 255))
    d.rectangle([16, 70, 26, 78], fill=(60, 50, 70, 255)); d.rectangle([38, 70, 48, 78], fill=(60, 50, 70, 255))
    return fin(im, 16, 20)
os.makedirs(os.path.join(D, "png"), exist_ok=True)
out = [("t_grass", tile("grass")), ("t_dirt", tile("dirt")), ("t_stone", tile("stone")), ("crate", crate()), ("coin", coin()), ("hero", hero()),
       ("glow", radial(32, 32, 16, 16, 16, [(0, (255, 255, 255, 255)), (0.4, (255, 255, 255, 140)), (1, (255, 255, 255, 0))]))]
for n, im in out: im.save(os.path.join(D, "png", n + ".png"))
open(os.path.join(D, "sprites.t3s"), "w").write("--atlas -f rgba8 -z auto\n" + "".join("png/%s.png\n" % n for n, _ in out))
