# Hilfen fuer Icons und HOME-Menue-Banner (Logos mit Verlauf, Kontur, Gluehen)
import math, random
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops

FONT_BLACK = "/usr/share/fonts/opentype/inter/InterDisplay-Black.otf"
FONT_XBOLD = "/usr/share/fonts/opentype/inter/Inter-ExtraBold.otf"


def _interp(stops, t):
    for i in range(len(stops) - 1):
        t0, c0 = stops[i]
        t1, c1 = stops[i + 1]
        if t <= t1:
            k = 0 if t1 == t0 else (t - t0) / (t1 - t0)
            return [c0[j] + (c1[j] - c0[j]) * k for j in range(4)]
    return list(stops[-1][1])


def vgrad(w, h, stops):
    """Vertikaler Verlauf, stops = [(t, (r,g,b,a)), ...]"""
    col = np.array([_interp(stops, y / max(1, h - 1)) for y in range(h)], dtype=np.float32)
    arr = np.repeat(col[:, None, :], w, axis=1)
    return Image.fromarray(arr.clip(0, 255).astype(np.uint8), "RGBA")


def radial(w, h, cx, cy, r, stops):
    yy, xx = np.mgrid[0:h, 0:w]
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2) / r
    d = d.clip(0, 1)
    lut = np.array([_interp(stops, i / 255) for i in range(256)], dtype=np.float32)
    arr = lut[(d * 255).astype(np.int32)]
    return Image.fromarray(arr.clip(0, 255).astype(np.uint8), "RGBA")


def tint(mask, color):
    layer = Image.new("RGBA", mask.size, color[:3] + (0,))
    a = mask if len(color) < 4 else ImageChops.multiply(mask, Image.new("L", mask.size, color[3]))
    layer.putalpha(a)
    return layer


def logo_text(text, size, fill_stops, stroke=0, stroke_col=(0, 0, 0), glow=0, glow_col=(255, 255, 255, 160),
              shadow=(0, 0), font=FONT_BLACK, highlight=0.22, spacing=0):
    f = ImageFont.truetype(font, size)
    pad = stroke + glow * 2 + max(abs(shadow[0]), abs(shadow[1])) + 4
    # Breite inkl. Zeichenabstand
    widths = [f.getlength(ch) for ch in text]
    tw = int(sum(widths) + spacing * (len(text) - 1))
    asc, desc = f.getmetrics()
    W, H = tw + pad * 2, asc + desc + pad * 2
    m_fill = Image.new("L", (W, H), 0)
    m_stroke = Image.new("L", (W, H), 0)
    d1, d2 = ImageDraw.Draw(m_fill), ImageDraw.Draw(m_stroke)
    x = pad
    for ch, cw in zip(text, widths):
        d1.text((x, pad), ch, font=f, fill=255)
        if stroke:
            d2.text((x, pad), ch, font=f, fill=255, stroke_width=stroke, stroke_fill=255)
        x += cw + spacing
    if not stroke:
        m_stroke = m_fill.copy()
    out = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    if shadow != (0, 0):
        sh = Image.new("L", (W, H), 0)
        sh.paste(m_stroke, shadow)
        out.alpha_composite(tint(sh.filter(ImageFilter.GaussianBlur(max(1, size // 30))), (0, 0, 0, 170)))
    if glow:
        out.alpha_composite(tint(m_stroke.filter(ImageFilter.GaussianBlur(glow)), glow_col))
    if stroke:
        out.alpha_composite(tint(m_stroke, tuple(stroke_col) + (255,)))
    # Verlauf nur im Glyphenbereich
    bbox = m_fill.getbbox() or (0, 0, W, H)
    g = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    g.paste(vgrad(bbox[2] - bbox[0], bbox[3] - bbox[1], fill_stops), (bbox[0], bbox[1]))
    g.putalpha(ImageChops.multiply(g.getchannel("A"), m_fill))
    out.alpha_composite(g)
    if highlight > 0:
        hl = Image.new("L", (W, H), 0)
        top = bbox[1] + (bbox[3] - bbox[1]) * 0.08
        mid = bbox[1] + (bbox[3] - bbox[1]) * 0.45
        ImageDraw.Draw(hl).rectangle([0, top, W, mid], fill=int(255 * highlight))
        hl = ImageChops.multiply(hl, m_fill.filter(ImageFilter.MinFilter(3)))
        out.alpha_composite(tint(hl, (255, 255, 255)))
    return out, m_fill


def paste_center(base, img, cx, cy):
    base.alpha_composite(img, (int(cx - img.width / 2), int(cy - img.height / 2)))


def glow_dot(img, x, y, r, col, a=200):
    lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(lay).ellipse([x - r, y - r, x + r, y + r], fill=col + (a,))
    img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(r * 0.6)))


def stars(img, n, seed=1, maxr=1.6):
    rnd = random.Random(seed)
    d = ImageDraw.Draw(img)
    w, h = img.size
    for _ in range(n):
        x, y = rnd.uniform(0, w), rnd.uniform(0, h)
        r = (rnd.random() ** 5) * maxr * w / 256 + 0.35 * w / 256
        a = rnd.randint(120, 255)
        d.ellipse([x - r, y - r, x + r, y + r], fill=(255, 255, 255, a))
        if rnd.random() < 0.035:
            L = r * 4
            d.line([x - L, y, x + L, y], fill=(255, 255, 255, a // 2), width=max(1, int(r / 2)))
            d.line([x, y - L, x, y + L], fill=(255, 255, 255, a // 2), width=max(1, int(r / 2)))


def nebula(img, blobs, seed=2):
    rnd = random.Random(seed)
    w, h = img.size
    lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    for col, n in blobs:
        for _ in range(n):
            x, y = rnd.uniform(-0.1, 1.1) * w, rnd.uniform(-0.1, 1.1) * h
            r = rnd.uniform(0.15, 0.35) * w
            d.ellipse([x - r, y - r * 0.7, x + r, y + r * 0.7], fill=col)
    img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(w * 0.07)))


def crystal(img, cx, cy, s, col=(74, 216, 255), glow=True):
    if glow:
        lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
        d = ImageDraw.Draw(lay)
        for k in range(8):
            a = k * math.pi / 4 + 0.2
            d.polygon([(cx, cy), (cx + math.cos(a - 0.08) * s * 3, cy + math.sin(a - 0.08) * s * 3),
                       (cx + math.cos(a + 0.08) * s * 3, cy + math.sin(a + 0.08) * s * 3)], fill=col + (70,))
        img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(s * 0.15)))
        glow_dot(img, cx, cy, s * 1.4, col, 150)
    d = ImageDraw.Draw(img)
    def lt(t): return tuple(int(v + (255 - v) * t) for v in col) + (255,)
    def dk(t): return tuple(int(v * (1 - t)) for v in col) + (255,)
    top, bot = (cx, cy - s * 1.25), (cx, cy + s * 1.25)
    l, r = (cx - s * 0.82, cy - s * 0.12), (cx + s * 0.82, cy - s * 0.12)
    m = (cx + s * 0.2, cy - s * 0.12)
    d.polygon([top, l, m], fill=lt(0.75))
    d.polygon([top, m, r], fill=lt(0.35))
    d.polygon([l, bot, m], fill=col + (255,))
    d.polygon([m, bot, r], fill=dk(0.45))
    d.line([l, r], fill=(255, 255, 255, 140), width=max(1, int(s / 18)))
    d.line([top, m, bot], fill=(255, 255, 255, 90), width=max(1, int(s / 24)))
    d.polygon([(cx - s * 0.35, cy - s * 0.55), (cx - s * 0.18, cy - s * 0.85), (cx - s * 0.1, cy - s * 0.4)], fill=(255, 255, 255, 200))


def blob(img, cx, cy, r, col, look=(1, 0), angry=False, squish=1.0):
    """Glaenzender Glibber mit Augen"""
    w, h = r * 2.1, r * 1.8 * squish
    sh = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(sh).ellipse([cx - w * 0.45, cy + h * 0.35, cx + w * 0.45, cy + h * 0.55], fill=(0, 0, 0, 110))
    img.alpha_composite(sh.filter(ImageFilter.GaussianBlur(r * 0.12)))
    body = radial(int(w) + 2, int(h) + 2, w * 0.35, h * 0.3, w * 0.85,
                  [(0, tuple(min(255, c + 90) for c in col) + (255,)), (0.55, col + (255,)), (1, tuple(int(c * 0.45) for c in col) + (255,))])
    m = Image.new("L", body.size, 0)
    ImageDraw.Draw(m).ellipse([0, 0, w, h], fill=255)
    body.putalpha(m)
    rim = Image.new("RGBA", body.size, (0, 0, 0, 0))
    ImageDraw.Draw(rim).ellipse([0, 0, w, h], outline=tuple(int(c * 0.35) for c in col) + (255,), width=max(1, int(r * 0.08)))
    body.alpha_composite(rim)
    img.alpha_composite(body, (int(cx - w / 2), int(cy - h / 2)))
    d = ImageDraw.Draw(img)
    d.ellipse([cx - w * 0.3, cy - h * 0.38, cx - w * 0.02, cy - h * 0.2], fill=(255, 255, 255, 170))
    d.ellipse([cx + w * 0.12, cy - h * 0.36, cx + w * 0.2, cy - h * 0.28], fill=(255, 255, 255, 140))
    for k in (-1, 1):
        ex, ey = cx + k * r * 0.36 + look[0] * r * 0.1, cy - r * 0.02
        d.ellipse([ex - r * 0.26, ey - r * 0.3, ex + r * 0.26, ey + r * 0.3], fill=(255, 255, 255, 255))
        px, py = ex + look[0] * r * 0.11, ey + look[1] * r * 0.1 + r * 0.04
        d.ellipse([px - r * 0.13, py - r * 0.15, px + r * 0.13, py + r * 0.15], fill=(16, 16, 26, 255))
        d.ellipse([px - r * 0.07, py - r * 0.1, px - r * 0.01, py - r * 0.04], fill=(255, 255, 255, 255))
        if angry:
            d.line([(ex + k * r * 0.32, ey - r * 0.5), (ex - k * r * 0.22, ey - r * 0.3)], fill=(30, 20, 20, 255), width=max(1, int(r * 0.11)))
    d.arc([cx - r * 0.22, cy + r * 0.18, cx + r * 0.22, cy + r * 0.42], 200, 340, fill=(30, 20, 20, 255), width=max(1, int(r * 0.07)))


def save_small(big, path, size):
    big.resize(size, Image.LANCZOS).save(path)
