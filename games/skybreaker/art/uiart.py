"""SKYBREAKER – UI-Kunst: Obsidian-Hintergründe mit Goldrahmen, Medaillon-Fassungen, Lebensleiste, Kartenrahmen.
Fertig gemalte Bildschirme (320x240) für den unteren Bildschirm + Einzelteile für den oberen HUD."""
import math, os, random
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops

HERE = os.path.dirname(os.path.abspath(__file__))
SERIF = "/usr/share/fonts/truetype/crosextra/Caladea-Bold.ttf"
K = 3  # Überabtastung
GOLD = (226, 186, 104)
GOLD_D = (120, 86, 40)
GOLD_L = (255, 236, 180)


def obsidian(w, h, seed=1, tint=(20, 16, 34)):
    rng = np.random.default_rng(seed)
    base = np.zeros((h, w, 3))
    base[:] = tint
    yy = np.linspace(0, 1, h)[:, None]
    base *= (1.15 - 0.35 * yy)[..., None]
    n = rng.random((h // 8 + 2, w // 8 + 2))
    n = np.array(Image.fromarray((n * 255).astype(np.uint8)).resize((w, h), Image.BICUBIC), dtype=float) / 255
    base *= (0.85 + 0.3 * n)[..., None]
    fine = rng.random((h, w)) * 0.08
    base *= (0.96 + fine)[..., None]
    im = Image.fromarray(np.clip(base, 0, 255).astype(np.uint8)).convert("RGBA")
    # feine Adern
    d = ImageDraw.Draw(im)
    r = random.Random(seed)
    for _ in range(int(w * h / 4000)):
        x, y = r.uniform(0, w), r.uniform(0, h)
        pts = [(x, y)]
        for _ in range(5):
            x += r.uniform(-12, 12); y += r.uniform(-6, 6)
            pts.append((x, y))
        d.line(pts, fill=(70, 56, 100, 60), width=1)
    return im


def gold_rect(d, box, w=2, inner=True, r=0):
    x0, y0, x1, y1 = box
    if r:
        d.rounded_rectangle(box, radius=r, outline=GOLD_D + (255,), width=w + 2)
        d.rounded_rectangle(box, radius=r, outline=GOLD + (255,), width=w)
        if inner:
            d.rounded_rectangle((x0 + w + 3, y0 + w + 3, x1 - w - 3, y1 - w - 3), radius=max(1, r - 3), outline=GOLD_D + (160,), width=1)
    else:
        d.rectangle(box, outline=GOLD_D + (255,), width=w + 2)
        d.rectangle(box, outline=GOLD + (255,), width=w)
        d.line((x0 + w, y0 + w, x1 - w, y0 + w), fill=GOLD_L + (200,), width=1)
        if inner:
            d.rectangle((x0 + w + 3, y0 + w + 3, x1 - w - 3, y1 - w - 3), outline=GOLD_D + (150,), width=1)


def corner(d, x, y, s, flipx=False, flipy=False):
    fx = -1 if flipx else 1
    fy = -1 if flipy else 1
    d.polygon([(x, y), (x + fx * s, y), (x, y + fy * s)], fill=GOLD + (255,))
    d.polygon([(x + fx * 2, y + fy * 2), (x + fx * (s - 3), y + fy * 2), (x + fx * 2, y + fy * (s - 3))], fill=GOLD_D + (255,))
    d.ellipse([x + fx * s * 0.55 - 2, y + fy * s * 0.55 - 2, x + fx * s * 0.55 + 2, y + fy * s * 0.55 + 2], fill=GOLD_L + (255,))


def plate(im, box, r=6, fill=(14, 10, 26, 235), frame=True, corners=True):
    lay = Image.new("RGBA", im.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    x0, y0, x1, y1 = box
    # Schlagschatten
    sh = Image.new("L", im.size, 0)
    ImageDraw.Draw(sh).rounded_rectangle((x0 + 2 * K, y0 + 3 * K, x1 + 2 * K, y1 + 3 * K), radius=r, fill=160)
    im.alpha_composite(Image.merge("RGBA", [Image.new("L", im.size, 0)] * 3 + [sh.filter(ImageFilter.GaussianBlur(4 * K))]))
    d.rounded_rectangle(box, radius=r, fill=fill)
    # innerer Verlauf (oben heller)
    g = Image.new("L", im.size, 0)
    gd = ImageDraw.Draw(g)
    for i in range(int((y1 - y0) * 0.45)):
        gd.line((x0 + r, y0 + i, x1 - r, y0 + i), fill=int(20 * (1 - i / ((y1 - y0) * 0.45))))
    lay.alpha_composite(Image.merge("RGBA", [Image.new("L", im.size, 255)] * 3 + [g]))
    if frame:
        gold_rect(d, box, w=K, inner=True, r=r)
    if corners:
        s = 6 * K
        corner(d, x0 + K, y0 + K, s); corner(d, x1 - K, y0 + K, s, flipx=True)
        corner(d, x0 + K, y1 - K, s, flipy=True); corner(d, x1 - K, y1 - K, s, True, True)
    im.alpha_composite(lay)


def medallion(im, cx, cy, rad):
    lay = Image.new("RGBA", im.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    d.ellipse((cx - rad - 3 * K, cy - rad - 3 * K, cx + rad + 3 * K, cy + rad + 3 * K), fill=(0, 0, 0, 120))
    d.ellipse((cx - rad, cy - rad, cx + rad, cy + rad), fill=GOLD_D + (255,))
    d.ellipse((cx - rad + K, cy - rad + K, cx + rad - K, cy + rad - K), fill=GOLD + (255,))
    d.ellipse((cx - rad + 3 * K, cy - rad + 3 * K, cx + rad - 3 * K, cy + rad - 3 * K), fill=(10, 8, 20, 255))
    # innere Vertiefung mit Verlauf
    for i in range(8):
        rr = rad - 4 * K - i * K
        if rr <= 0:
            break
        c = 16 + i * 4
        d.ellipse((cx - rr, cy - rr + i * 0.5 * K, cx + rr, cy + rr + i * 0.5 * K), fill=(c, c - 4, c + 14, 255))
    for k in range(8):                                    # Nieten
        a = math.pi / 4 * k + math.pi / 8
        x, y = cx + math.cos(a) * (rad - 1.5 * K), cy + math.sin(a) * (rad - 1.5 * K)
        d.ellipse((x - K, y - K, x + K, y + K), fill=GOLD_L + (255,))
    im.alpha_composite(lay)


def label(im, x, y, text, size, col=GOLD, anchor="mm", spacing=1.0):
    d = ImageDraw.Draw(im)
    f = ImageFont.truetype(SERIF, int(size * K))
    d.text((x + K, y + K), text, font=f, fill=(0, 0, 0, 200), anchor=anchor)
    d.text((x, y), text, font=f, fill=col + (255,), anchor=anchor)


def finish(im):
    return im.resize((im.width // K, im.height // K), Image.LANCZOS)


def bot_run():
    W, H = 320 * K, 240 * K
    im = obsidian(W, H, 3)
    d = ImageDraw.Draw(im)
    # Kopfleiste
    plate(im, (2 * K, 1 * K, 318 * K, 20 * K), r=4 * K, fill=(10, 8, 20, 240), corners=False)
    # Lebensleiste: geschnitzter Rahmen
    plate(im, (6 * K, 23 * K, 314 * K, 47 * K), r=6 * K, fill=(26, 6, 14, 255))
    # Segen-Fassungen
    for i, name in enumerate(("Angriff", "Wirbel", "Splitter", "Ausweichen")):
        cx = (46 + i * 76) * K
        medallion(im, cx, 80 * K, 25 * K)
        label(im, cx, 113 * K, name, 8.5, GOLD)
    # Gaben-Leiste
    plate(im, (8 * K, 122 * K, 312 * K, 146 * K), r=5 * K, fill=(12, 10, 24, 235), corners=False)
    label(im, 30 * K, 134 * K, "Gaben", 8, GOLD)
    for i in range(7):
        cx, cy = (70 + i * 34) * K, 134 * K
        ImageDraw.Draw(im).ellipse((cx - 9 * K, cy - 9 * K, cx + 9 * K, cy + 9 * K), fill=(8, 6, 16, 255), outline=GOLD_D + (255,), width=K)
    # Ladungen
    for i, name in enumerate(("Ausweichen", "Splitter", "Wirbel")):
        x0 = (8 + i * 102) * K
        plate(im, (x0, 151 * K, x0 + 98 * K, 175 * K), r=5 * K, fill=(12, 10, 24, 235), corners=False)
        label(im, x0 + 8 * K, 163 * K, name, 7.5, (200, 190, 230), anchor="lm")
    return finish(im)


def bot_cards():
    W, H = 320 * K, 240 * K
    im = obsidian(W, H, 5)
    plate(im, (40 * K, 2 * K, 280 * K, 22 * K), r=5 * K, fill=(12, 8, 22, 240), corners=False)
    for i in range(3):
        x0 = (8 + i * 104) * K
        plate(im, (x0, 28 * K, x0 + 96 * K, 120 * K), r=8 * K, fill=(20, 16, 36, 240))
    plate(im, (6 * K, 126 * K, 314 * K, 212 * K), r=8 * K, fill=(14, 10, 26, 245))
    return finish(im)


def bot_menu():
    W, H = 320 * K, 240 * K
    im = obsidian(W, H, 9)
    plate(im, (40 * K, 2 * K, 280 * K, 22 * K), r=5 * K, fill=(12, 8, 22, 240), corners=False)
    plate(im, (6 * K, 28 * K, 314 * K, 196 * K), r=8 * K, fill=(14, 10, 26, 230))
    return finish(im)


def hp_fill():
    W, H = 64, 16
    yy = np.linspace(0, 1, H)[:, None] * np.ones((1, W))
    r = 210 - 60 * yy + 40 * np.exp(-((yy - 0.25) ** 2) / 0.01)
    g = 40 - 20 * yy + 60 * np.exp(-((yy - 0.25) ** 2) / 0.01)
    b = 60 - 30 * yy + 50 * np.exp(-((yy - 0.25) ** 2) / 0.01)
    a = np.full((H, W), 255.0)
    return Image.fromarray(np.dstack([np.clip(c, 0, 255) for c in (r, g, b, a)]).astype(np.uint8), "RGBA")


def top_hp():
    """kleiner verzierter Rahmen für die Lebensleiste oben links (130x20)"""
    W, H = 130 * K, 20 * K
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    plate(im, (2 * K, 2 * K, W - 2 * K, H - 2 * K), r=5 * K, fill=(26, 6, 14, 220), corners=False)
    return finish(im)


def boss_bar():
    W, H = 300 * K, 24 * K
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    plate(im, (2 * K, 2 * K, W - 2 * K, H - 2 * K), r=6 * K, fill=(30, 6, 10, 230))
    return finish(im)


def build(save):
    """save(img, name) -> Pfad; gibt Liste der Pfade (für ui.t3s) und bg-Liste zurück"""
    ui = [save(hp_fill(), "hp_fill"), save(top_hp(), "top_hp"), save(boss_bar(), "boss_bar")]
    bg = [save(bot_run(), "bot_run"), save(bot_cards(), "bot_cards"), save(bot_menu(), "bot_menu")]
    return ui, bg
