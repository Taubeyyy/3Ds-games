"""Vorschau-Renderer für lowpoly-Meshes (numpy, z-Puffer, flache Schattierung, Kantenlicht, 2x Kantenglättung).
Für Konzept-Blätter und schnelle Design-Iterationen ohne Konsole."""
import math
import numpy as np
from PIL import Image


def render(parts, w=400, h=520, yaw=0.0, pitch=0.12, dist=None, target=None, fov=28, bg=None,
           light=(0.45, 0.8, 0.55), light_col=(1.0, 0.96, 0.9), ambient=(0.32, 0.34, 0.44), rim_col=(0.55, 0.7, 1.0),
           ss=2, emissive=None, extra_lights=()):
    """parts: Liste von Meshes (lowpoly.Mesh) oder (Mesh, glow) – glow=True: unbeleuchtet (leuchtet).
    extra_lights: [(pos(x,y,z), farbe(r,g,b), reichweite)]"""
    W, H = w * ss, h * ss
    tris, cols, glow = [], [], []
    for p in parts:
        m, g = (p, False) if not isinstance(p, tuple) else p
        for a, b, c, col in m.tris:
            tris.append((a, b, c)); cols.append(col[:3]); glow.append(g)
    P = np.array(tris, dtype=np.float64)            # (n,3,3)
    C = np.array(cols, dtype=np.float64) / 255.0
    G = np.array(glow, dtype=bool)
    mn, mx = P.reshape(-1, 3).min(0), P.reshape(-1, 3).max(0)
    tgt = np.array(target if target is not None else (mn + mx) / 2)
    size = max(mx[1] - mn[1], (mx[0] - mn[0]) * h / w, 0.5)
    if dist is None:
        dist = size * 0.62 / math.tan(math.radians(fov / 2))
    eye = tgt + dist * np.array([math.sin(yaw) * math.cos(pitch), math.sin(pitch), math.cos(yaw) * math.cos(pitch)])
    f = tgt - eye; f /= np.linalg.norm(f)
    s = np.cross(f, [0, 1, 0]); s /= np.linalg.norm(s)
    u = np.cross(s, f)
    V = P - eye
    vx, vy, vz = V @ s, V @ u, V @ f                   # Kamera-Raum, vz = Abstand nach vorne
    fl = 1 / math.tan(math.radians(fov / 2))
    sx = (vx / vz) * fl * (H / 2) + W / 2
    sy = H / 2 - (vy / vz) * fl * (H / 2)
    # Normalen & Licht (Weltraum)
    N = np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0])
    N /= np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-9)
    L = np.array(light) / np.linalg.norm(light)
    ctr = P.mean(1)
    view = eye - ctr; view /= np.linalg.norm(view, axis=1, keepdims=True)
    lam = np.clip(N @ L, 0, 1)[:, None]
    rim = (np.clip(1 - np.sum(N * view, 1), 0, 1) ** 3)[:, None]
    hv = (L + view); hv /= np.linalg.norm(hv, axis=1, keepdims=True)
    spec = (np.clip(np.sum(N * hv, 1), 0, 1) ** 24)[:, None] * 0.25
    lit = np.array(ambient) + lam * np.array(light_col) + rim * np.array(rim_col) * 0.6
    for lp, lc, rr in extra_lights:
        d = np.array(lp) - ctr
        dl = np.linalg.norm(d, axis=1, keepdims=True)
        att = np.clip(1 - (dl / rr) ** 2, 0, 1) ** 2
        lit = lit + np.clip(np.sum(N * d / np.maximum(dl, 1e-6), 1, keepdims=True), 0, 1) * att * np.array(lc)
    shadec = np.where(G[:, None], C * 1.15, C * lit + spec)
    shadec = np.clip(shadec, 0, 1)
    # Rückseiten weg
    facing = np.sum(N * view, 1) > -1e-4
    img = np.zeros((H, W, 3)) if bg is None else np.array(bg.resize((W, H)).convert("RGB"), dtype=np.float64) / 255
    alpha = np.zeros((H, W))
    zb = np.full((H, W), np.inf)
    X = np.stack([sx, sy], -1)
    for i in np.argsort(-vz.mean(1)):
        if not facing[i] or (vz[i] <= 0.05).any():
            continue
        (x0, y0), (x1, y1), (x2, y2) = X[i]
        area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
        if abs(area) < 1e-9:
            continue
        bx0, bx1 = max(int(min(x0, x1, x2)), 0), min(int(max(x0, x1, x2)) + 1, W - 1)
        by0, by1 = max(int(min(y0, y1, y2)), 0), min(int(max(y0, y1, y2)) + 1, H - 1)
        if bx0 > bx1 or by0 > by1:
            continue
        gx, gy = np.meshgrid(np.arange(bx0, bx1 + 1) + 0.5, np.arange(by0, by1 + 1) + 0.5)
        w0 = ((x1 - gx) * (y2 - gy) - (x2 - gx) * (y1 - gy)) / area
        w1 = ((x2 - gx) * (y0 - gy) - (x0 - gx) * (y2 - gy)) / area
        w2 = 1 - w0 - w1
        inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
        z = w0 * vz[i, 0] + w1 * vz[i, 1] + w2 * vz[i, 2]
        sub = zb[by0:by1 + 1, bx0:bx1 + 1]
        m = inside & (z < sub)
        sub[m] = z[m]
        img[by0:by1 + 1, bx0:bx1 + 1][m] = shadec[i]
        alpha[by0:by1 + 1, bx0:bx1 + 1][m] = 1
    out = Image.fromarray((np.dstack([img, alpha]) * 255).astype(np.uint8), "RGBA")
    return out.resize((w, h), Image.LANCZOS)
