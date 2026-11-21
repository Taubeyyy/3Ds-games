"""SKYBREAKER – 2D-Grafiken: Logo, Himmel mit Riss, Hintergründe, Göttersiegel, Namen, Knöpfe, Partikel."""
import math, os, random, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "tools"))
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageChops, ImageFont
from artlib import logo_text, paste_center, tint, vgrad

GFX = os.path.join(HERE, "..", "gfx")
PNG = os.path.join(GFX, "png")
os.makedirs(PNG, exist_ok=True)
SERIF = "/usr/share/fonts/truetype/crosextra/Caladea-Bold.ttf"
SANS = "/usr/share/fonts/opentype/inter/Inter-ExtraBold.otf"

# Figuren: Name, Farbe (Element), Symbol
CHARS = [("seren", "SEREN", (138, 240, 255)), ("ignara", "IGNARA", (255, 110, 50)), ("thalor", "THALOR", (110, 180, 255)),
         ("nerys", "NERYS", (60, 220, 210)), ("vexa", "VEXA", (170, 100, 255)), ("korrin", "KORRIN", (235, 150, 70)),
         ("lyra", "LYRA", (255, 226, 140)), ("sylv", "SYLV", (130, 225, 90))]


def save(im, name):
    im.save(os.path.join(PNG, name + ".png"))
    return "png/%s.png" % name


def blur_glow(mask, r, col):
    return tint(mask.filter(ImageFilter.GaussianBlur(r)), col)


# ------------------------------------------------------------------ Himmelsriss (Pfade)
def crack_paths(seed, w, h, x0, y0, x1, y1, branches=6):
    r = random.Random(seed)
    paths = []
    def walk(ax, ay, bx, by, n, jit):
        pts = [(ax, ay)]
        for i in range(1, n):
            t = i / n
            pts.append((ax + (bx - ax) * t + r.uniform(-jit, jit), ay + (by - ay) * t + r.uniform(-jit, jit)))
        pts.append((bx, by))
        return pts
    main = walk(x0, y0, x1, y1, 14, 10)
    paths.append((main, 3.2))
    for _ in range(branches):
        k = r.randint(2, len(main) - 3)
        ax, ay = main[k]
        ang = math.atan2(y1 - y0, x1 - x0) + r.choice([-1, 1]) * r.uniform(0.6, 1.3)
        L = r.uniform(40, 110) * w / 400
        b = walk(ax, ay, ax + math.cos(ang) * L, ay + math.sin(ang) * L, 6, 5)
        paths.append((b, 1.6))
        if r.random() < 0.6:
            k2 = r.randint(1, len(b) - 2)
            ang2 = ang + r.uniform(-0.9, 0.9)
            c = walk(b[k2][0], b[k2][1], b[k2][0] + math.cos(ang2) * L * 0.5, b[k2][1] + math.sin(ang2) * L * 0.5, 4, 3)
            paths.append((c, 0.9))
    return paths


def draw_cracks(size, paths, scale=1.0):
    core = Image.new("L", size, 0)
    d = ImageDraw.Draw(core)
    for pts, wdt in paths:
        d.line([(x * scale, y * scale) for x, y in pts], fill=255, width=max(1, int(wdt * scale)), joint="curve")
    return core


# ------------------------------------------------------------------ Himmel (oben, 400x240)
def sky():
    W, H = 400, 240
    y = np.linspace(0, 1, H)[:, None]
    stops = [(0.0, (8, 10, 34)), (0.38, (34, 24, 84)), (0.66, (110, 50, 120)), (0.84, (224, 110, 110)), (1.0, (255, 182, 120))]
    img = np.zeros((H, W, 3))
    for (t0, c0), (t1, c1) in zip(stops, stops[1:]):
        m = (y >= t0) & (y <= t1)
        k = np.clip((y - t0) / (t1 - t0), 0, 1)
        for i in range(3):
            img[..., i] = np.where(m, c0[i] + (c1[i] - c0[i]) * k, img[..., i])
    im = Image.fromarray(img.astype(np.uint8)).convert("RGBA")
    r = random.Random(4)
    # Nebel-Schwaden
    neb = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    nd = ImageDraw.Draw(neb)
    for _ in range(26):
        x, yy = r.uniform(-40, W + 40), r.uniform(10, 150)
        rad = r.uniform(30, 80)
        col = r.choice([(120, 70, 200), (60, 90, 200), (200, 90, 170)])
        nd.ellipse([x - rad * 1.6, yy - rad * 0.6, x + rad * 1.6, yy + rad * 0.6], fill=col + (int(r.uniform(18, 40)),))
    im.alpha_composite(neb.filter(ImageFilter.GaussianBlur(18)))
    # Sterne
    sd = ImageDraw.Draw(im)
    for _ in range(170):
        x, yy = r.uniform(0, W), r.uniform(0, H * 0.7) ** 1.0
        a = int(255 * (1 - yy / (H * 0.75)) * r.uniform(0.3, 1))
        s = r.choice([0, 0, 0, 1])
        sd.ellipse([x - s * 0.5, yy - s * 0.5, x + s * 0.5 + 0.6, yy + s * 0.5 + 0.6], fill=(255, 250, 240, max(0, a)))
    # Riss im Himmel
    paths = crack_paths(9, W, H, 250, -10, 330, 150, branches=7)
    core = draw_cracks((W, H), paths)
    im.alpha_composite(blur_glow(core, 10, (120, 190, 255, 150)))
    im.alpha_composite(blur_glow(core, 4, (190, 230, 255, 200)))
    im.alpha_composite(tint(core, (255, 255, 255, 255)))
    # Wolkenband am Horizont
    cl = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    cd = ImageDraw.Draw(cl)
    for _ in range(40):
        x, yy = r.uniform(-30, W + 30), r.uniform(185, 235)
        rad = r.uniform(18, 40)
        cd.ellipse([x - rad * 1.8, yy - rad * 0.55, x + rad * 1.8, yy + rad * 0.55], fill=(255, 200, 170, 70))
    im.alpha_composite(cl.filter(ImageFilter.GaussianBlur(6)))
    crack_only = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    crack_only.alpha_composite(blur_glow(core, 8, (150, 210, 255, 220)))
    crack_only.alpha_composite(tint(core, (255, 255, 255, 255)))
    return im, crack_only


# ------------------------------------------------------------------ Unterer Hintergrund (320x240)
def bottom_bg():
    W, H = 320, 240
    im = Image.new("RGBA", (W, H))
    im.paste(vgrad(W, H, [(0, (14, 16, 44, 255)), (0.6, (24, 18, 54, 255)), (1, (40, 22, 60, 255))]))
    r = random.Random(7)
    d = ImageDraw.Draw(im)
    for _ in range(80):
        x, y = r.uniform(0, W), r.uniform(0, H)
        a = int(r.uniform(40, 160))
        d.point((x, y), fill=(255, 255, 255, a))
    # Turm-Silhouette
    sil = Image.new("L", (W, H), 0)
    sd = ImageDraw.Draw(sil)
    cx = 250
    sd.polygon([(cx - 26, H), (cx - 13, 60), (cx - 9, 40), (cx - 4, 52), (cx + 2, 34), (cx + 7, 48), (cx + 11, 44), (cx + 26, H)], fill=255)
    for k in range(5):
        sd.ellipse([cx - 4 + (k - 2) * 12, 18 + k * 5 % 11, cx + 2 + (k - 2) * 12, 23 + k * 5 % 11], fill=255)
    im.alpha_composite(blur_glow(sil, 6, (90, 120, 255, 90)))
    im.alpha_composite(tint(sil, (10, 10, 28, 235)))
    # Vignette
    v = Image.new("L", (W, H), 0)
    ImageDraw.Draw(v).rectangle([0, 0, W, H], fill=255)
    inner = Image.new("L", (W, H), 0)
    ImageDraw.Draw(inner).ellipse([-60, -50, W + 60, H + 50], fill=255)
    vig = ImageChops.subtract(v, inner.filter(ImageFilter.GaussianBlur(40)))
    im.alpha_composite(tint(vig, (0, 0, 0, 150)))
    return im


def pantheon_bg():
    W, H = 400, 240
    im = Image.new("RGBA", (W, H))
    im.paste(vgrad(W, H, [(0, (6, 6, 16, 255)), (0.7, (14, 12, 28, 255)), (1, (22, 18, 38, 255))]))
    r = random.Random(12)
    d = ImageDraw.Draw(im)
    for _ in range(120):
        x, y = r.uniform(0, W), r.uniform(0, H * 0.8)
        d.point((x, y), fill=(255, 255, 255, int(r.uniform(30, 140))))
    # Säulenhalle als Silhouette
    sil = Image.new("L", (W, H), 0)
    sd = ImageDraw.Draw(sil)
    for x in (30, 95, 305, 370):
        sd.rectangle([x - 10, 40, x + 10, H], fill=255)
        sd.rectangle([x - 15, 34, x + 15, 42], fill=255)
    sd.rectangle([0, 26, W, 34], fill=255)
    im.alpha_composite(tint(sil, (0, 0, 0, 120)))
    return im


# ------------------------------------------------------------------ Logo
def logo():
    k = 2
    stops = [(0, (255, 255, 255, 255)), (0.45, (200, 232, 255, 255)), (0.62, (140, 190, 255, 255)), (1, (70, 110, 210, 255))]
    t, mask = logo_text("SKYBREAKER", 50 * k, stops, stroke=4 * k, stroke_col=(10, 14, 40), glow=9 * k, glow_col=(90, 180, 255, 170),
                        shadow=(0, 4 * k), font=SERIF, highlight=0.18, spacing=3 * k)
    W, H = t.size
    # Riss durch die Buchstaben: Pfad, rechte Hälfte leicht versetzt
    r = random.Random(3)
    pts = [(W * 0.56 + r.uniform(-6, 6) * k, y) for y in range(0, H + 1, 10 * k)]
    poly_r = [(W, 0), (W, H)] + [(x, y) for x, y in reversed(pts)]
    right = Image.new("L", (W, H), 0)
    ImageDraw.Draw(right).polygon(poly_r, fill=255)
    left = ImageChops.invert(right)
    lt = Image.new("RGBA", (W, H), (0, 0, 0, 0)); lt.paste(t, (0, 0), left)
    rt = Image.new("RGBA", (W, H), (0, 0, 0, 0)); rt.paste(t, (0, 0), right)
    out = Image.new("RGBA", (W + 8 * k, H + 8 * k), (0, 0, 0, 0))
    out.alpha_composite(lt, (2 * k, 4 * k))
    out.alpha_composite(rt, (6 * k, 0))
    line = Image.new("L", out.size, 0)
    ImageDraw.Draw(line).line([(x + 4 * k, y + 2 * k) for x, y in pts], fill=255, width=2 * k)
    line = ImageChops.multiply(line, out.getchannel("A").filter(ImageFilter.MaxFilter(9)))
    out.alpha_composite(blur_glow(line, 5 * k, (150, 220, 255, 255)))
    out.alpha_composite(tint(line, (255, 255, 255, 255)))
    out = out.crop(out.getbbox())
    return out.resize((out.width // k, out.height // k), Image.LANCZOS)


def subtitle(text, size=11, col=(240, 205, 120)):
    k = 3
    f = ImageFont.truetype(SERIF, size * k)
    spacing = int(size * k * 0.32)
    w = int(sum(f.getlength(c) for c in text) + spacing * (len(text) - 1)) + 40 * k
    h = int(size * k * 1.6)
    im = Image.new("RGBA", (w + 80 * k, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    x = 40 * k + 20 * k
    for c in text:
        d.text((x, size * k * 0.2), c, font=f, fill=col + (255,))
        x += f.getlength(c) + spacing
    cy = h // 2 + k
    for sx, ex in ((8 * k, 52 * k), (x + 8 * k - spacing, x + 52 * k - spacing)):
        d.line([(sx, cy), (ex, cy)], fill=col + (200,), width=k)
    for px in (6 * k, x + 54 * k - spacing):
        d.polygon([(px, cy - 3 * k), (px + 3 * k, cy), (px, cy + 3 * k), (px - 3 * k, cy)], fill=col + (255,))
    im = im.crop(im.getbbox())
    return im.resize((im.width // k, im.height // k), Image.LANCZOS)


# ------------------------------------------------------------------ Siegel (Medaillons 72x72)
def sigil(key, col):
    k = 4
    S = 72 * k
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    c = S / 2
    disc = Image.new("L", (S, S), 0)
    ImageDraw.Draw(disc).ellipse([4 * k, 4 * k, S - 4 * k, S - 4 * k], fill=255)
    # radialer Verlauf
    yy, xx = np.mgrid[0:S, 0:S]
    dist = np.sqrt((xx - c) ** 2 + (yy - c * 0.8) ** 2) / (S / 2)
    base = np.array([18, 16, 36]); edge = np.array(col) * 0.55
    rgbv = base + (edge - base) * np.clip(dist, 0, 1)[..., None] ** 1.4
    grad = Image.fromarray(np.dstack([rgbv, np.full((S, S), 255)]).astype(np.uint8), "RGBA")
    im.paste(grad, (0, 0), disc)
    d = ImageDraw.Draw(im)
    # Goldring
    d.ellipse([4 * k, 4 * k, S - 4 * k, S - 4 * k], outline=(232, 192, 96, 255), width=3 * k)
    d.ellipse([9 * k, 9 * k, S - 9 * k, S - 9 * k], outline=(140, 100, 40, 200), width=k)
    sym = Image.new("L", (S, S), 0)
    sd = ImageDraw.Draw(sym)
    def P(pts): return [(c + x * k, c + y * k) for x, y in pts]
    if key == "seren":
        pts = []
        for i in range(16):
            a = -math.pi / 2 + math.pi * i / 8
            rr = 22 if i % 4 == 0 else (12 if i % 2 == 0 else 6)
            pts.append((math.cos(a) * rr, math.sin(a) * rr))
        sd.polygon(P(pts), fill=255)
    elif key == "ignara":
        sd.polygon(P([(0, -22), (8, -8), (14, 2), (12, 12), (4, 18), (-6, 18), (-13, 10), (-12, -2), (-6, -6), (-6, 4), (0, -2), (-2, -12)]), fill=255)
        sd.polygon(P([(0, 0), (5, 8), (3, 15), (-3, 15), (-5, 8)]), fill=0)
    elif key == "thalor":
        sd.polygon(P([(4, -24), (-12, 2), (-1, 2), (-6, 24), (12, -4), (1, -4), (8, -24)]), fill=255)
    elif key == "nerys":
        for j, yo in enumerate((-9, 1, 11)):
            pts = [(x, yo + math.sin(x / 5.5 + j) * 4) for x in range(-20, 21, 2)]
            sd.line(P(pts), fill=255, width=4 * k, joint="curve")
    elif key == "vexa":
        sd.ellipse(P([(-18, -18), (18, 18)]), fill=255)
        sd.ellipse(P([(-10, -22), (24, 12)]), fill=0)
        sd.ellipse(P([(-4, -3), (4, 5)]), fill=255)
    elif key == "korrin":
        sd.polygon(P([(-20, -8), (16, -8), (22, -14), (22, -4), (10, 0), (6, 10), (12, 16), (-12, 16), (-6, 10), (-10, 0), (-20, -2)]), fill=255)
        sd.rectangle(P([(-3, -24), (3, -10)]), fill=255)
        sd.rectangle(P([(-11, -26), (11, -18)]), fill=255)
    elif key == "lyra":
        pts = []
        for i in range(10):
            a = -math.pi / 2 + math.pi * i / 5
            rr = 21 if i % 2 == 0 else 9
            pts.append((math.cos(a) * rr, math.sin(a) * rr))
        sd.polygon(P(pts), fill=255)
        for i in range(8):
            a = math.pi * i / 4 + math.pi / 8
            sd.line(P([(math.cos(a) * 23, math.sin(a) * 23), (math.cos(a) * 27, math.sin(a) * 27)]), fill=255, width=2 * k)
    elif key == "sylv":
        sd.polygon(P([(0, 22), (-12, 8), (-14, -6), (-6, -18), (0, -24), (6, -18), (14, -6), (12, 8)]), fill=255)
        sd.line(P([(0, 22), (0, -16)]), fill=0, width=2 * k)
        for yv in (-8, 0, 8):
            sd.line(P([(0, yv + 4), (-7, yv - 3)]), fill=0, width=int(1.5 * k))
            sd.line(P([(0, yv + 4), (7, yv - 3)]), fill=0, width=int(1.5 * k))
    im.alpha_composite(blur_glow(sym, 5 * k, col + (230,)))
    hi = tuple(min(255, int(v * 0.35 + 255 * 0.65)) for v in col)
    g = vgrad(S, S, [(0, (255, 255, 255, 255)), (1, hi + (255,))])
    g.putalpha(sym)
    im.alpha_composite(g)
    return im.resize((72, 72), Image.LANCZOS)


def name_logo(text, col):
    stops = [(0, (255, 255, 255, 255)), (0.55, tuple(min(255, int(v * 0.4 + 160)) for v in col) + (255,)), (1, col + (255,))]
    t, _ = logo_text(text, 30 * 2, stops, stroke=3 * 2, stroke_col=(8, 8, 20), glow=6 * 2, glow_col=col + (150,),
                     shadow=(0, 3 * 2), font=SERIF, highlight=0.15, spacing=6 * 2)
    t = t.crop(t.getbbox())
    return t.resize((t.width // 2, t.height // 2), Image.LANCZOS)


# ------------------------------------------------------------------ Knöpfe / Panels
def panel(w, h, top, bot, border, glow=None, ornament=True, r=10):
    k = 3
    W, H = w * k, h * k
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    mask = Image.new("L", (W, H), 0)
    ImageDraw.Draw(mask).rounded_rectangle([2 * k, 2 * k, W - 2 * k, H - 2 * k], radius=r * k, fill=255)
    if glow:
        im.alpha_composite(blur_glow(mask, 3 * k, glow))
    g = vgrad(W, H, [(0, top + (240,)), (1, bot + (240,))])
    im.paste(g, (0, 0), mask)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([2 * k, 2 * k, W - 2 * k, H - 2 * k], radius=r * k, outline=border + (255,), width=int(1.5 * k))
    d.rounded_rectangle([5 * k, 5 * k, W - 5 * k, H - 5 * k], radius=(r - 3) * k, outline=border + (70,), width=k)
    d.line([(14 * k, 6 * k), (W - 14 * k, 6 * k)], fill=(255, 255, 255, 40), width=k)
    if ornament:
        cy = H // 2
        for px in (2 * k, W - 2 * k):
            d.polygon([(px, cy - 6 * k), (px + 5 * k, cy), (px, cy + 6 * k), (px - 5 * k, cy)], fill=border + (255,))
    return im.resize((w, h), Image.LANCZOS)


def particles():
    out = {}
    g = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    yy, xx = np.mgrid[0:64, 0:64]
    d = np.sqrt((xx - 31.5) ** 2 + (yy - 31.5) ** 2) / 32
    a = np.clip(1 - d, 0, 1) ** 2.2
    out["glow"] = Image.fromarray(np.dstack([np.full((64, 64), 255)] * 3 + [a * 255]).astype(np.uint8), "RGBA")
    sp = Image.new("L", (32, 32), 0)
    sd = ImageDraw.Draw(sp)
    sd.polygon([(16, 0), (18, 14), (32, 16), (18, 18), (16, 32), (14, 18), (0, 16), (14, 14)], fill=255)
    spark = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    spark.alpha_composite(blur_glow(sp, 2, (255, 255, 255, 200)))
    spark.alpha_composite(tint(sp, (255, 255, 255, 255)))
    out["spark"] = spark
    em = Image.new("L", (16, 16), 0)
    ImageDraw.Draw(em).ellipse([5, 5, 11, 11], fill=255)
    ember = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    ember.alpha_composite(blur_glow(em, 2.5, (255, 255, 255, 255)))
    ember.alpha_composite(tint(em, (255, 255, 255, 255)))
    out["ember"] = ember
    bub = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    ImageDraw.Draw(bub).ellipse([2, 2, 13, 13], outline=(255, 255, 255, 230), width=1)
    ImageDraw.Draw(bub).point([(5, 5), (6, 5)], fill=(255, 255, 255, 255))
    out["bubble"] = bub
    lf = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    ImageDraw.Draw(lf).polygon([(8, 1), (13, 7), (8, 15), (3, 7)], fill=(255, 255, 255, 255))
    ImageDraw.Draw(lf).line([(8, 2), (8, 14)], fill=(200, 200, 200, 255))
    out["leaf"] = lf
    wi = Image.new("L", (32, 32), 0)
    ImageDraw.Draw(wi).ellipse([6, 6, 26, 26], fill=255)
    out["wisp"] = tint(wi.filter(ImageFilter.GaussianBlur(5)), (255, 255, 255, 255))
    sq = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    ImageDraw.Draw(sq).polygon([(8, 2), (14, 8), (8, 14), (2, 8)], fill=(255, 255, 255, 255))
    out["dust"] = sq
    return out


def cloud(seed, w=160, h=64):
    r = random.Random(seed)
    m = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(m)
    for _ in range(9):
        x, y = r.uniform(w * 0.2, w * 0.8), r.uniform(h * 0.45, h * 0.7)
        rr = r.uniform(12, 24)
        d.ellipse([x - rr * 1.4, y - rr, x + rr * 1.4, y + rr], fill=255)
    m = m.filter(ImageFilter.GaussianBlur(5))
    im = vgrad(w, h, [(0, (255, 246, 240, 255)), (1, (200, 170, 220, 255))])
    im.putalpha(m)
    return im


def lock_icon():
    im = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.arc([4, 1, 12, 10], 180, 360, fill=(220, 210, 190, 255), width=2)
    d.rectangle([3, 7, 13, 15], fill=(220, 190, 110, 255))
    d.rectangle([7, 10, 8, 12], fill=(60, 40, 20, 255))
    return im


def build():
    ui = []
    ui.append(save(logo(), "logo"))                                   # 0
    ui.append(save(subtitle("DER ZERBROCHENE HIMMEL"), "subtitle"))     # 1
    for key, name, col in CHARS:                                       # 2..9
        ui.append(save(sigil(key, col), "sigil_" + key))
    for key, name, col in CHARS:                                       # 10..17
        ui.append(save(name_logo(name, col), "name_" + key))
    p = particles()
    for k in ("glow", "spark", "ember", "bubble", "leaf", "wisp", "dust"):   # 18..24
        ui.append(save(p[k], "p_" + k))
    gold = (232, 192, 96)
    ui.append(save(panel(232, 46, (60, 44, 110), (26, 18, 52), gold, glow=(255, 200, 120, 120)), "btn_main"))  # 25
    ui.append(save(panel(232, 36, (34, 38, 76), (16, 18, 40), (150, 140, 190)), "btn"))                      # 26
    ui.append(save(panel(232, 36, (30, 30, 40), (18, 18, 26), (80, 80, 96)), "btn_off"))                     # 27
    ui.append(save(panel(304, 92, (24, 22, 48), (12, 12, 26), (120, 110, 160), ornament=False, r=8), "desc"))  # 28
    ui.append(save(lock_icon(), "lock"))                               # 29
    for i in range(3):                                                 # 30..32
        ui.append(save(cloud(70 + i), "cloud%d" % i))
    sel = Image.new("RGBA", (84, 84), (0, 0, 0, 0))
    m = Image.new("L", (84, 84), 0)
    ImageDraw.Draw(m).ellipse([4, 4, 80, 80], outline=255, width=3)
    sel.alpha_composite(blur_glow(m, 3, (255, 230, 160, 255)))
    sel.alpha_composite(tint(m, (255, 240, 200, 255)))
    ui.append(save(sel, "select"))                                     # 33
    ui.append(save(panel(64, 28, (34, 38, 76), (16, 18, 40), (150, 140, 190), ornament=False, r=8), "btn_small"))  # 34
    open(os.path.join(GFX, "ui.t3s"), "w").write("--atlas -f rgba8 -z auto\n" + "\n".join(ui) + "\n")
    s, crack = sky()
    bg = [save(s, "sky"), save(crack, "crack"), save(bottom_bg(), "bottom"), save(pantheon_bg(), "pantheon")]
    open(os.path.join(GFX, "bg.t3s"), "w").write("--atlas -f rgba8 -z auto\n" + "\n".join(bg) + "\n")
    # Enum für den Code
    names = [os.path.splitext(os.path.basename(x))[0].upper() for x in ui]
    with open(os.path.join(HERE, "..", "source", "gfx_ids.hpp"), "w") as f:
        f.write("// generiert von art/gfx.py\n#pragma once\nenum UiImg {\n")
        for i, n in enumerate(names):
            f.write("\tUI_%s = %d,\n" % (n, i))
        f.write("\tUI_COUNT = %d\n};\nenum BgImg { BG_SKY, BG_CRACK, BG_BOTTOM, BG_PANTHEON, BG_COUNT };\n" % len(names))


if __name__ == "__main__":
    os.makedirs(os.path.join(HERE, "..", "source"), exist_ok=True)
    build()
    print("ok")
