"""lowpoly – kleine Modellier-Bibliothek für FORGE (.fmesh, flach schattiert, Vertexfarben).

Alle Formen sind Meshes aus Dreiecken mit Farbe pro Fläche. Koordinaten: Meter, Y nach oben,
Figuren schauen nach +Z. Vorderseite = gegen den Uhrzeigersinn von außen gesehen.

    from lowpoly import *
    m = Mesh()
    m += prism(0.2, 0.15, 0.8, 8, BLUE).move(0, 0.9, 0)
    m.save("data/held.fmesh")
"""
import math, random, struct


QUALITY = 1.0   # Segment-Multiplikator (Rundungen); Figuren setzen ihn höher


def _seg(n):
    return max(3, int(round(n * QUALITY)))


# ------------------------------------------------------------------ Farben
def rgb(h, a=255):
    return ((h >> 16) & 255, (h >> 8) & 255, h & 255, a)


def mix(c0, c1, t):
    return tuple(int(c0[i] + (c1[i] - c0[i]) * t) for i in range(4))


def shade(c, k):
    return (max(0, min(255, int(c[0] * k))), max(0, min(255, int(c[1] * k))), max(0, min(255, int(c[2] * k))), c[3])


# ------------------------------------------------------------------ Matrizen (4x4, Zeilen, Spaltenvektoren)
def mat_id():
    return [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]


def mat_mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def T(x, y, z):
    m = mat_id(); m[0][3], m[1][3], m[2][3] = x, y, z; return m


def S(x, y=None, z=None):
    y = x if y is None else y; z = x if z is None else z
    m = mat_id(); m[0][0], m[1][1], m[2][2] = x, y, z; return m


def RX(a):
    c, s = math.cos(a), math.sin(a); m = mat_id(); m[1][1], m[1][2], m[2][1], m[2][2] = c, -s, s, c; return m


def RY(a):
    c, s = math.cos(a), math.sin(a); m = mat_id(); m[0][0], m[0][2], m[2][0], m[2][2] = c, s, -s, c; return m


def RZ(a):
    c, s = math.cos(a), math.sin(a); m = mat_id(); m[0][0], m[0][1], m[1][0], m[1][1] = c, -s, s, c; return m


def apply(m, p):
    return tuple(m[i][0] * p[0] + m[i][1] * p[1] + m[i][2] * p[2] + m[i][3] for i in range(3))


def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def norm(a):
    l = math.sqrt(dot(a, a))
    return (a[0] / l, a[1] / l, a[2] / l) if l > 1e-9 else (0.0, 1.0, 0.0)


# ------------------------------------------------------------------ Mesh
class Mesh:
    def __init__(self, tris=None):
        self.tris = tris or []  # (p0, p1, p2, farbe)

    def tri(self, a, b, c, col):
        self.tris.append((a, b, c, col)); return self

    def quad(self, a, b, c, d, col):
        self.tris.append((a, b, c, col)); self.tris.append((a, c, d, col)); return self

    def __iadd__(self, o):
        self.tris.extend(o.tris); return self

    def __add__(self, o):
        return Mesh(self.tris + o.tris)

    def xf(self, m):
        flip = _det3(m) < 0
        out = []
        for a, b, c, col in self.tris:
            A, B, C = apply(m, a), apply(m, b), apply(m, c)
            out.append((A, C, B, col) if flip else (A, B, C, col))
        return Mesh(out)

    def move(self, x, y, z): return self.xf(T(x, y, z))
    def scale(self, x, y=None, z=None): return self.xf(S(x, y, z))
    def rx(self, a): return self.xf(RX(a))
    def ry(self, a): return self.xf(RY(a))
    def rz(self, a): return self.xf(RZ(a))
    def mirror_x(self): return self.xf(S(-1, 1, 1))

    def recolor(self, fn):
        """fn(farbe, mittelpunkt) -> neue Farbe"""
        return Mesh([(a, b, c, fn(col, ((a[0] + b[0] + c[0]) / 3, (a[1] + b[1] + c[1]) / 3, (a[2] + b[2] + c[2]) / 3))) for a, b, c, col in self.tris])

    def vary(self, amount=0.06, seed=1):
        """leichte Helligkeitsstreuung pro Fläche (wirkt handgemacht)"""
        r = random.Random(seed)
        return Mesh([(a, b, c, shade(col, 1 + r.uniform(-amount, amount))) for a, b, c, col in self.tris])

    def jitter(self, amount, seed=1):
        """Eckpunkte zufällig verschieben (Felsen, Äste); gleiche Punkte bleiben verbunden"""
        r = random.Random(seed)
        off = {}
        def j(p):
            k = (round(p[0], 4), round(p[1], 4), round(p[2], 4))
            if k not in off:
                off[k] = (r.uniform(-amount, amount), r.uniform(-amount, amount), r.uniform(-amount, amount))
            o = off[k]
            return (p[0] + o[0], p[1] + o[1], p[2] + o[2])
        return Mesh([(j(a), j(b), j(c), col) for a, b, c, col in self.tris])

    def bend(self, fn):
        """fn(punkt) -> punkt, frei verformen"""
        return Mesh([(fn(a), fn(b), fn(c), col) for a, b, c, col in self.tris])

    def bounds(self):
        ps = [p for t in self.tris for p in t[:3]]
        return tuple(min(p[i] for p in ps) for i in range(3)), tuple(max(p[i] for p in ps) for i in range(3))

    def save(self, path, smooth=False, angle=48):
        """smooth: Auto-Glättung – Normalen werden nur über Flächen gemittelt, die weniger als 'angle' Grad abweichen"""
        verts, idx, look = [], [], {}
        acc = {}
        if smooth:
            for a, b, c, col in self.tris:
                n = cross(sub(b, a), sub(c, a))
                for p in (a, b, c):
                    acc.setdefault((round(p[0], 4), round(p[1], 4), round(p[2], 4)), []).append(n)
        cosA = math.cos(math.radians(angle))
        for a, b, c, col in self.tris:
            fn = norm(cross(sub(b, a), sub(c, a)))
            for p in (a, b, c):
                if smooth:
                    sx = sy = sz = 0.0
                    for n2 in acc[(round(p[0], 4), round(p[1], 4), round(p[2], 4))]:
                        if dot(norm(n2), fn) >= cosA:
                            sx += n2[0]; sy += n2[1]; sz += n2[2]
                    n = norm((sx, sy, sz)) if (sx or sy or sz) else fn
                else:
                    n = fn
                key = (round(p[0], 5), round(p[1], 5), round(p[2], 5), round(n[0], 3), round(n[1], 3), round(n[2], 3), col)
                if key not in look:
                    look[key] = len(verts); verts.append((p, n, col))
                idx.append(look[key])
        if len(verts) > 65535:
            raise SystemExit("%s: zu viele Vertices (%d)" % (path, len(verts)))
        with open(path, "wb") as f:
            f.write(struct.pack("<III", 0x48534D46, len(verts), len(idx)))
            for p, n, c in verts:
                f.write(struct.pack("<8f4B", p[0], p[1], p[2], 0.0, 0.0, n[0], n[1], n[2], *c))
            f.write(struct.pack("<%dH" % len(idx), *idx))
        return len(verts), len(idx) // 3


def _det3(m):
    return (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
            + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]))


# ------------------------------------------------------------------ Grundformen
def lathe(profile, seg, col, phase=0.0, cap_bottom=True, cap_top=True, colors=None):
    """Drehkörper um Y. profile: [(radius, y), ...] von unten nach oben. colors: Farbe je Ring-Abschnitt."""
    seg = _seg(seg)
    m = Mesh()
    ang = [phase + 2 * math.pi * i / seg for i in range(seg)]
    def P(r, y, a): return (r * math.sin(a), y, r * math.cos(a))
    for k in range(len(profile) - 1):
        (r0, y0), (r1, y1) = profile[k], profile[k + 1]
        c = colors[k] if colors else col
        for i in range(seg):
            a0, a1 = ang[i], ang[(i + 1) % seg]
            p00, p01, p10, p11 = P(r0, y0, a0), P(r0, y0, a1), P(r1, y1, a0), P(r1, y1, a1)
            if r0 < 1e-6:
                m.tri(p00, p01, p11, c) if False else m.tri(p00, p11, p10, c)
            elif r1 < 1e-6:
                m.tri(p00, p01, p10, c)
            else:
                m.quad(p00, p01, p11, p10, c)
    r, y = profile[0]
    if cap_bottom and r > 1e-6:
        cb = colors[0] if colors else col
        for i in range(seg):
            m.tri((0, y, 0), P(r, y, ang[(i + 1) % seg]), P(r, y, ang[i]), cb)
    r, y = profile[-1]
    if cap_top and r > 1e-6:
        ct = colors[-1] if colors else col
        for i in range(seg):
            m.tri((0, y, 0), P(r, y, ang[i]), P(r, y, ang[(i + 1) % seg]), ct)
    return m


def prism(r0, r1, h, seg, col, phase=None):
    """Kegelstumpf von y=0 bis y=h"""
    return lathe([(r0, 0), (r1, h)], seg, col, phase=math.pi / seg if phase is None else phase)


def box(sx, sy, sz, col, top=1.0, topz=None):
    """Quader, Boden bei y=0, mittig in x/z. top/topz: Verjüngung der Oberseite."""
    hx, hz = sx / 2, sz / 2
    tx, tz = hx * top, hz * (top if topz is None else topz)
    b = [(-hx, 0, -hz), (hx, 0, -hz), (hx, 0, hz), (-hx, 0, hz)]
    t = [(-tx, sy, -tz), (tx, sy, -tz), (tx, sy, tz), (-tx, sy, tz)]
    m = Mesh()
    m.quad(b[0], b[1], b[2], b[3], col)          # unten (zeigt nach -y)
    m.quad(t[3], t[2], t[1], t[0], col)          # oben
    for i in range(4):
        j = (i + 1) % 4
        m.quad(b[j], b[i], t[i], t[j], col)
    return m


def sphere(r, seg, rings, col, squash=1.0):
    rings = _seg(rings)
    prof = [(r * math.sin(math.pi * k / rings), -r * math.cos(math.pi * k / rings) * squash) for k in range(rings + 1)]
    return lathe(prof, seg, col, cap_bottom=False, cap_top=False)


def ico(r, col):
    t = (1 + 5 ** 0.5) / 2
    v = [norm(p) for p in [(-1, t, 0), (1, t, 0), (-1, -t, 0), (1, -t, 0), (0, -1, t), (0, 1, t), (0, -1, -t), (0, 1, -t),
                           (t, 0, -1), (t, 0, 1), (-t, 0, -1), (-t, 0, 1)]]
    f = [(0, 11, 5), (0, 5, 1), (0, 1, 7), (0, 7, 10), (0, 10, 11), (1, 5, 9), (5, 11, 4), (11, 10, 2), (10, 7, 6), (7, 1, 8),
         (3, 9, 4), (3, 4, 2), (3, 2, 6), (3, 6, 8), (3, 8, 9), (4, 9, 5), (2, 4, 11), (6, 2, 10), (8, 6, 7), (9, 8, 1)]
    m = Mesh()
    for a, b, c in f:
        m.tri(tuple(x * r for x in v[a]), tuple(x * r for x in v[b]), tuple(x * r for x in v[c]), col)
    return m


def octa(r, h, col):
    """Kristall/Edelstein: Doppelpyramide"""
    m = Mesh()
    e = [(r, 0, 0), (0, 0, r), (-r, 0, 0), (0, 0, -r)]
    for i in range(4):
        a, b = e[i], e[(i + 1) % 4]
        m.tri((0, h, 0), b, a, col)
        m.tri((0, -h, 0), a, b, col)
    return m


def slab(poly, thick, col, side=None):
    """2D-Umriss in der XY-Ebene (gegen den Uhrzeigersinn) mit Dicke in Z – Klingen, Flügel, Flammen, Blätter."""
    m = Mesh()
    hz = thick / 2
    n = len(poly)
    cx = sum(p[0] for p in poly) / n; cy = sum(p[1] for p in poly) / n
    side = side or shade(col, 0.8)
    for i in range(n):
        a, b = poly[i], poly[(i + 1) % n]
        m.tri((cx, cy, hz), (a[0], a[1], hz), (b[0], b[1], hz), col)
        m.tri((cx, cy, -hz), (b[0], b[1], -hz), (a[0], a[1], -hz), col)
        m.quad((a[0], a[1], -hz), (b[0], b[1], -hz), (b[0], b[1], hz), (a[0], a[1], hz), side)
    return m


def blade(length, width, thick, col, edge=None, tip=0.25):
    """spitze Klinge entlang +Y mit Mittelgrat (rautenförmiger Querschnitt)"""
    m = Mesh()
    edge = edge or col
    w = width / 2
    t = thick / 2
    L = length * (1 - tip)
    pts = [(-w, 0), (w, 0), (w, L), (0, length), (-w, L)]
    # zwei Hälften mit Grat in der Mitte
    for s in (1, -1):
        ridge0, ridge1 = (0, 0, s * t), (0, L, s * t)
        tipp = (0, length, 0)
        a0, a1 = (-w, 0, 0), (-w, L, 0)
        b0, b1 = (w, 0, 0), (w, L, 0)
        if s > 0:
            m.quad(a0, ridge0, ridge1, a1, col); m.quad(ridge0, b0, b1, ridge1, col)
            m.tri(a1, ridge1, tipp, edge); m.tri(ridge1, b1, tipp, edge)
        else:
            m.quad(ridge0, a0, a1, ridge1, col); m.quad(b0, ridge0, ridge1, b1, col)
            m.tri(ridge1, a1, tipp, edge); m.tri(b1, ridge1, tipp, edge)
    return orient(m, lambda c: (0, c[1], 0))


def tube(points, radii, seg, col, colors=None):
    """Röhre entlang einer Punktkette (Haarsträhnen, Ranken, Hörner, Schals)"""
    seg = _seg(seg)
    m = Mesh()
    rings = []
    for k, p in enumerate(points):
        d = norm(sub(points[min(k + 1, len(points) - 1)], points[max(k - 1, 0)]))
        up = (0, 1, 0) if abs(d[1]) < 0.9 else (1, 0, 0)
        u = norm(cross(d, up)); v = cross(u, d)
        r = radii[k] if isinstance(radii, (list, tuple)) else radii
        rings.append([(p[0] + (u[0] * math.cos(a) + v[0] * math.sin(a)) * r, p[1] + (u[1] * math.cos(a) + v[1] * math.sin(a)) * r,
                       p[2] + (u[2] * math.cos(a) + v[2] * math.sin(a)) * r) for a in [2 * math.pi * i / seg for i in range(seg)]])
    for k in range(len(rings) - 1):
        c = colors[k] if colors else col
        for i in range(seg):
            j = (i + 1) % seg
            a, b, cc, d = rings[k][i], rings[k][j], rings[k + 1][j], rings[k + 1][i]
            m.quad(a, b, cc, d, c)
    # Enden zu
    for k, sgn in ((0, -1), (len(rings) - 1, 1)):
        r = radii[k] if isinstance(radii, (list, tuple)) else radii
        if r <= 1e-6:
            continue
        ctr = points[k]
        c = colors[min(k, len(colors) - 1)] if colors else col
        for i in range(seg):
            j = (i + 1) % seg
            if sgn > 0:
                m.tri(ctr, rings[k][i], rings[k][j], c)
            else:
                m.tri(ctr, rings[k][j], rings[k][i], c)
    return _orient_tube(m, points)


def _orient_tube(m, pts):
    """Wicklung so drehen, dass Normalen von der Mittellinie weg zeigen"""
    out = []
    for a, b, c, col in m.tris:
        ctr = ((a[0] + b[0] + c[0]) / 3, (a[1] + b[1] + c[1]) / 3, (a[2] + b[2] + c[2]) / 3)
        best = min(pts, key=lambda p: dot(sub(ctr, p), sub(ctr, p)))
        n = cross(sub(b, a), sub(c, a))
        out.append((a, c, b, col) if dot(n, sub(ctr, best)) < 0 else (a, b, c, col))
    return Mesh(out)


def ring(r, thick, seg, col, tseg=4):
    """Torus in der XZ-Ebene (Heiligenschein, Armreif, Wasserring)"""
    seg = _seg(seg)
    m = Mesh()
    for i in range(seg):
        for j in range(tseg):
            def P(ii, jj):
                a, b = 2 * math.pi * ii / seg, 2 * math.pi * jj / tseg
                rr = r + thick * math.cos(b)
                return (rr * math.cos(a), thick * math.sin(b), rr * math.sin(a))
            m.quad(P(i, j), P(i, j + 1), P(i + 1, j + 1), P(i + 1, j), col)
    return orient(m, lambda c: (r * c[0] / max(1e-6, math.hypot(c[0], c[2])), 0, r * c[2] / max(1e-6, math.hypot(c[0], c[2]))))


def orient(m, center_fn):
    """Wicklung so drehen, dass jede Fläche von center_fn(flächenmitte) weg zeigt"""
    out = []
    for a, b, c, col in m.tris:
        ctr = ((a[0] + b[0] + c[0]) / 3, (a[1] + b[1] + c[1]) / 3, (a[2] + b[2] + c[2]) / 3)
        n = cross(sub(b, a), sub(c, a))
        out.append((a, c, b, col) if dot(n, sub(ctr, center_fn(ctr))) < 0 else (a, b, c, col))
    return Mesh(out)


def outward(m, center=(0, 0, 0)):
    """Wicklung aller Dreiecke nach außen (vom Punkt weg) ausrichten"""
    out = []
    for a, b, c, col in m.tris:
        ctr = ((a[0] + b[0] + c[0]) / 3, (a[1] + b[1] + c[1]) / 3, (a[2] + b[2] + c[2]) / 3)
        n = cross(sub(b, a), sub(c, a))
        out.append((a, c, b, col) if dot(n, sub(ctr, center)) < 0 else (a, b, c, col))
    return Mesh(out)


def double_sided(m):
    """Flächen von beiden Seiten sichtbar (Umhänge, Blätter, Bänder)"""
    return Mesh(m.tris + [(a, c, b, shade(col, 0.82)) for a, b, c, col in m.tris])


def cloth(width, length, cols, rows, col, curve=0.0, flare=0.0, col_bottom=None):
    """Tuch (Umhang/Schal) von y=0 nach unten, leicht gewölbt nach -z, beidseitig"""
    m = Mesh()
    def P(i, j):
        u = i / cols - 0.5; v = j / rows
        w = width * (1 + flare * v)
        return (u * w, -v * length, -curve * (1 - (2 * u) ** 2) - 0.12 * v * v * length * 0.5)
    for j in range(rows):
        c = mix(col, col_bottom, j / max(1, rows - 1)) if col_bottom else col
        for i in range(cols):
            m.quad(P(i, j), P(i, j + 1), P(i + 1, j + 1), P(i + 1, j), c)
    return double_sided(m)
