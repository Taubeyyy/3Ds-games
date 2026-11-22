"""SKYBREAKER – Umgebungstexturen (prozedural, kachelbar): Dungeon-Boden/-Wand, Grotten-Boden/-Wand, Vignette.
Ausgabe: gfx/png/tex_*.png + gfx/tex_*.t3s (einzelne Texturen mit Mipmaps)."""
import math, os, random
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
GFX = os.path.join(HERE, "..", "gfx")
PNG = os.path.join(GFX, "png")
os.makedirs(PNG, exist_ok=True)
N = 256


def noise(size, scale, seed, octaves=4):
    """kachelbares Wertrauschen (0..1)"""
    rng = np.random.default_rng(seed)
    out = np.zeros((size, size))
    amp, tot = 1.0, 0.0
    for o in range(octaves):
        cells = max(2, int(scale * 2 ** o))
        g = rng.random((cells, cells))
        yy, xx = np.mgrid[0:size, 0:size] / size * cells
        x0, y0 = np.floor(xx).astype(int), np.floor(yy).astype(int)
        fx, fy = xx - x0, yy - y0
        fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
        x1, y1 = (x0 + 1) % cells, (y0 + 1) % cells
        x0, y0 = x0 % cells, y0 % cells
        v = (g[y0, x0] * (1 - fx) + g[y0, x1] * fx) * (1 - fy) + (g[y1, x0] * (1 - fx) + g[y1, x1] * fx) * fy
        out += v * amp
        tot += amp
        amp *= 0.5
    return out / tot


def shade_tiles(size, rects, base_cols, grout, seed, edge=3, crack=True, moss=None):
    """Platten mit Kantenlicht/-schatten, Fugen, Rauschen, Rissen"""
    rnd = random.Random(seed)
    img = np.zeros((size, size, 3))
    img[:] = grout
    h = np.zeros((size, size))
    n1 = noise(size, 4, seed)
    n2 = noise(size, 16, seed + 1, 3)
    for (x0, y0, x1, y1) in rects:
        col = np.array(rnd.choice(base_cols), dtype=float) * rnd.uniform(0.88, 1.1)
        for y in range(y0, y1):
            for x in range(x0, x1):
                xx, yy = x % size, y % size
                dx0, dx1, dy0, dy1 = x - x0, x1 - 1 - x, y - y0, y1 - 1 - y
                d = min(dx0, dx1, dy0, dy1)
                if d < 1:
                    continue
                k = 1.0
                if d < edge:
                    if dy0 == d or dx0 == d:
                        k = 1.18
                    else:
                        k = 0.72
                img[yy, xx] = col * k
                h[yy, xx] = 1
    # Oberflächenrauschen
    img *= (0.82 + 0.3 * n1[..., None]) * (0.92 + 0.16 * n2[..., None])
    im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    if crack:
        for _ in range(9):
            x, y = rnd.uniform(0, size), rnd.uniform(0, size)
            pts = [(x, y)]
            for _ in range(rnd.randint(3, 6)):
                x += rnd.uniform(-9, 9); y += rnd.uniform(-9, 9)
                pts.append((x, y))
            d.line(pts, fill=(72, 66, 78), width=1)
    if moss is not None:
        m = noise(size, 3, seed + 7)
        mask = np.clip((m - 0.62) * 5, 0, 1)[..., None]
        arr = np.array(im, dtype=float)
        arr = arr * (1 - mask * 0.7) + np.array(moss) * mask * 0.7
        im = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8))
    return im


def floor_dungeon():
    rects = []
    s = N // 4
    for ty in range(4):
        for tx in range(4):
            x0, y0 = tx * s, ty * s
            k = (tx * 7 + ty * 3) % 5
            if k == 1:
                rects += [(x0, y0, x0 + s, y0 + s // 2), (x0, y0 + s // 2, x0 + s, y0 + s)]
            elif k == 3:
                rects += [(x0, y0, x0 + s // 2, y0 + s), (x0 + s // 2, y0, x0 + s, y0 + s)]
            else:
                rects += [(x0, y0, x0 + s, y0 + s)]
    return shade_tiles(N, rects, [(128, 118, 128), (112, 104, 116), (136, 126, 122), (102, 96, 108)], (36, 30, 40), 11, moss=(70, 98, 60))


def wall_dungeon():
    rects = []
    rows = 8
    bh = N // rows
    for r in range(rows):
        off = (r % 2) * (N // 4)
        for c in range(-1, 5):
            x0 = c * (N // 4) + off
            rects.append((x0, r * bh, x0 + N // 4, (r + 1) * bh))
    im = shade_tiles(N, [(max(0, a), b, min(N, c), d) for a, b, c, d in rects] + [(a + N, b, c + N, d) for a, b, c, d in rects if a < 0],
                     [(104, 96, 110), (92, 84, 98), (114, 106, 112), (86, 80, 94)], (30, 26, 34), 12, edge=3, crack=True)
    return im


def floor_cave():
    rnd = random.Random(21)
    base = noise(N, 3, 31) * 0.6 + noise(N, 10, 32, 3) * 0.4
    col0, col1 = np.array([46, 52, 82]), np.array([86, 96, 140])
    img = col0 + (col1 - col0) * base[..., None]
    im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    for _ in range(30):                                       # Felsplatten-Kanten
        x, y = rnd.uniform(0, N), rnd.uniform(0, N)
        pts = [(x, y)]
        for _ in range(4):
            x += rnd.uniform(-14, 14); y += rnd.uniform(-14, 14)
            pts.append((x, y))
        d.line(pts, fill=(40, 44, 70), width=1)
    for _ in range(40):                                       # kleine Kristallsplitter
        x, y = rnd.uniform(0, N), rnd.uniform(0, N)
        c = rnd.choice([(140, 230, 255), (190, 150, 255)])
        d.polygon([(x, y - 2), (x + 1.2, y), (x, y + 2), (x - 1.2, y)], fill=c)
    return im


def wall_cave():
    base = noise(N, 2, 41) * 0.5 + noise(N, 6, 42) * 0.35 + noise(N, 18, 43, 2) * 0.15
    col0, col1 = np.array([26, 28, 48]), np.array([96, 100, 146])
    img = col0 + (col1 - col0) * np.clip(base * 1.2 - 0.1, 0, 1)[..., None]
    # horizontale Schichtung
    yy = np.mgrid[0:N, 0:N][0]
    img *= (0.9 + 0.1 * np.sin(yy / N * 2 * np.pi * 5 + base * 6))[..., None]
    return Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))


def vignette():
    W, H = 128, 64
    yy, xx = np.mgrid[0:H, 0:W]
    dx, dy = (xx - W / 2 + 0.5) / (W / 2), (yy - H / 2 + 0.5) / (H / 2)
    d = np.sqrt(dx * dx * 0.9 + dy * dy)
    a = np.clip((d - 0.55) / 0.6, 0, 1) ** 1.6 * 200
    arr = np.zeros((H, W, 4), dtype=np.uint8)
    arr[..., 3] = a.astype(np.uint8)
    return Image.fromarray(arr, "RGBA")


def build():
    out = {"tex_floor_d": floor_dungeon(), "tex_wall_d": wall_dungeon(), "tex_floor_c": floor_cave(), "tex_wall_c": wall_cave()}
    from PIL import ImageEnhance
    for name, im in out.items():
        im = ImageEnhance.Brightness(im.convert("RGB")).enhance(1.25)
        im.convert("RGB").save(os.path.join(PNG, name + ".png"))
        open(os.path.join(GFX, name + ".t3s"), "w").write("-f rgb565 -m lanczos -z auto\npng/%s.png\n" % name)
    vignette().save(os.path.join(PNG, "vignette.png"))


if __name__ == "__main__":
    build()
    print("Texturen ok")
