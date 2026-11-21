#!/usr/bin/env python3
"""FORGE meshtool – wandelt Wavefront-OBJ (+MTL) in .fmesh für Mesh::loadMem/loadFile.

Nutzung: meshtool.py eingabe.obj ausgabe.fmesh [--scale S] [--center] [--flat] [--color RRGGBB]
("s off" im OBJ = flache Normalen für die folgenden Flächen)

.fmesh-Format (little endian):
  u32 'FMSH' (0x48534D46), u32 Vertexanzahl, u32 Indexanzahl
  je Vertex: f32 pos[3], f32 uv[2], f32 nrm[3], u8 rgba[4]   (36 Byte, = Vertex3D)
  u16 Indizes (Dreiecke, gegen den Uhrzeigersinn = Vorderseite)
Farben: Kd aus der MTL-Datei je usemtl wird zur Vertexfarbe (gut für Low-Poly-Modelle ohne Textur).
"""
import argparse, math, os, struct, sys


def load_mtl(path):
    mats, cur = {}, None
    if not os.path.exists(path):
        return mats
    for line in open(path, encoding="utf-8", errors="ignore"):
        p = line.split()
        if not p:
            continue
        if p[0] == "newmtl":
            cur = p[1]
            mats[cur] = (1.0, 1.0, 1.0, 1.0)
        elif p[0] == "Kd" and cur:
            r, g, b = map(float, p[1:4])
            mats[cur] = (r, g, b, mats[cur][3])
        elif p[0] in ("d", "Tr") and cur:
            a = float(p[1])
            if p[0] == "Tr":
                a = 1 - a
            mats[cur] = mats[cur][:3] + (a,)
    return mats


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("obj")
    ap.add_argument("out")
    ap.add_argument("--scale", type=float, default=1.0)
    ap.add_argument("--center", action="store_true", help="Modell mittig auf den Boden (y=0) setzen")
    ap.add_argument("--flat", action="store_true", help="flache Normalen erzwingen")
    ap.add_argument("--color", default="FFFFFF", help="Farbe ohne Material")
    a = ap.parse_args()

    base = os.path.dirname(os.path.abspath(a.obj))
    dc = int(a.color, 16)
    default = ((dc >> 16 & 255) / 255, (dc >> 8 & 255) / 255, (dc & 255) / 255, 1.0)
    mats, col, flat = {}, default, a.flat
    P, T, N = [], [], []
    tris = []  # je Ecke: (pi, ti, ni, farbe)
    for line in open(a.obj, encoding="utf-8", errors="ignore"):
        p = line.split()
        if not p:
            continue
        if p[0] == "v":
            P.append(tuple(float(x) * a.scale for x in p[1:4]))
        elif p[0] == "vt":
            T.append((float(p[1]), float(p[2]) if len(p) > 2 else 0.0))
        elif p[0] == "vn":
            N.append(tuple(float(x) for x in p[1:4]))
        elif p[0] == "mtllib":
            mats.update(load_mtl(os.path.join(base, " ".join(p[1:]))))
        elif p[0] == "s":  # Glättungsgruppen: "s off" = flache Normalen
            flat = a.flat or p[1] in ("off", "0")
        elif p[0] == "usemtl":
            col = mats.get(p[1], default)
        elif p[0] == "f":
            corners = []
            for c in p[1:]:
                ix = c.split("/")
                pi = int(ix[0]); pi = pi - 1 if pi > 0 else len(P) + pi
                ti = int(ix[1]) if len(ix) > 1 and ix[1] else 0
                ti = (ti - 1 if ti > 0 else len(T) + ti) if ti else -1
                ni = int(ix[2]) if len(ix) > 2 and ix[2] else 0
                ni = (ni - 1 if ni > 0 else len(N) + ni) if ni else -1
                corners.append((pi, ti, ni, col))
            for k in range(1, len(corners) - 1):  # Fächer-Triangulierung
                tris.append((corners[0], corners[k], corners[k + 1], flat))

    if a.center and P:
        mn = [min(v[i] for v in P) for i in range(3)]
        mx = [max(v[i] for v in P) for i in range(3)]
        off = ((mn[0] + mx[0]) / 2, mn[1], (mn[2] + mx[2]) / 2)
        P = [(v[0] - off[0], v[1] - off[1], v[2] - off[2]) for v in P]

    def sub(u, v): return (u[0] - v[0], u[1] - v[1], u[2] - v[2])
    def cross(u, v): return (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
    def norm(u):
        l = math.sqrt(u[0] ** 2 + u[1] ** 2 + u[2] ** 2)
        return (u[0] / l, u[1] / l, u[2] / l) if l > 1e-9 else (0.0, 1.0, 0.0)

    # Glatte Normalen je Positionsindex, falls keine angegeben
    smooth = {}
    if not N:
        for t in tris:
            if t[3]:
                continue
            fn = cross(sub(P[t[1][0]], P[t[0][0]]), sub(P[t[2][0]], P[t[0][0]]))
            for c in t[:3]:
                s = smooth.get(c[0], (0.0, 0.0, 0.0))
                smooth[c[0]] = (s[0] + fn[0], s[1] + fn[1], s[2] + fn[2])

    verts, index, lookup = [], [], {}
    for t in tris:
        fn = norm(cross(sub(P[t[1][0]], P[t[0][0]]), sub(P[t[2][0]], P[t[0][0]])))
        for c in t[:3]:
            pi, ti, ni, cc = c
            if t[3]:
                n = fn
            elif ni >= 0 and N:
                n = norm(N[ni])
            else:
                n = norm(smooth[pi])
            uv = T[ti] if ti >= 0 and T else (0.0, 0.0)
            rgba = tuple(max(0, min(255, int(round(x * 255)))) for x in cc)
            key = (P[pi], uv, tuple(round(x, 4) for x in n), rgba)
            if key not in lookup:
                lookup[key] = len(verts)
                verts.append(key)
            index.append(lookup[key])
    if len(verts) > 65535:
        sys.exit("Zu viele Vertices (%d > 65535) – Modell aufteilen" % len(verts))
    with open(a.out, "wb") as f:
        f.write(struct.pack("<III", 0x48534D46, len(verts), len(index)))
        for p, uv, n, c in verts:
            f.write(struct.pack("<8f4B", p[0], p[1], p[2], uv[0], uv[1], n[0], n[1], n[2], *c))
        f.write(struct.pack("<%dH" % len(index), *index))
    print("%s: %d Vertices, %d Dreiecke, %d Byte" % (os.path.basename(a.out), len(verts), len(index) // 3, os.path.getsize(a.out)))


if __name__ == "__main__":
    main()
