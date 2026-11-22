"""Alle SKYBREAKER-Assets erzeugen und source/parts_gen.hpp schreiben."""
import os, sys, subprocess
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
for script in ("textures.py", "gfx.py", "world.py", "enemies.py", "music.py", "motifs.py"):
    subprocess.check_call([sys.executable, os.path.join(HERE, script)], stdout=subprocess.DEVNULL)
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "tools"))
import lowpoly
lowpoly.QUALITY = 1.6                      # rundere Figuren
import characters
data = os.path.join(HERE, "..", "data")
GLOWY = ("glow", "fx", "weapon_glow", "orbit_glow", "pet")


def ao(m, height):
    """eingebackene Verdunkelung nach unten (Bodennähe) – gibt Tiefe"""
    def f(c, p):
        k = 0.58 + 0.42 * min(1.0, max(0.0, p[1] / (0.8 * height)))
        return (int(c[0] * k), int(c[1] * k), int(c[2] * k), c[3])
    return m.recolor(f)
KINDS = ["body", "arm", "hair", "scarf", "cape", "glow", "fx", "weapon", "weapon_glow", "orbit", "orbit_glow", "pet", "arm_l", "leg_l", "leg_r"]
rows, incs = [], []
tri_total = 0
for ci, (name, fn) in enumerate(characters.ALL.items()):
    parts = fn()
    height = parts["body"].bounds()[1][1]
    for part, m in parts.items():
        if part not in GLOWY:
            m = ao(m, height)
        nv, nt = m.save(os.path.join(data, "%s_%s.fmesh" % (name, part)), smooth=part not in GLOWY)
        tri_total += nt
        sym = "%s_%s_fmesh" % (name, part)
        incs.append('#include "%s.h"' % sym)
        rows.append("\t{%d, PK_%s, %s, %s_size}," % (ci, part.upper(), sym, sym))
# Umriss-Hüllen für die Heldin (nach außen versetzte, umgedrehte Kopie -> dunkler Rand)
def hull(m, d=0.016):
    import math as _m
    acc = {}
    for a, b, c, col in m.tris:
        n = lowpoly.cross(lowpoly.sub(b, a), lowpoly.sub(c, a))
        for p in (a, b, c):
            k = (round(p[0], 4), round(p[1], 4), round(p[2], 4))
            s0 = acc.get(k, (0, 0, 0)); acc[k] = (s0[0] + n[0], s0[1] + n[1], s0[2] + n[2])
    def off(p):
        n = lowpoly.norm(acc[(round(p[0], 4), round(p[1], 4), round(p[2], 4))])
        return (p[0] + n[0] * d, p[1] + n[1] * d, p[2] + n[2] * d)
    return lowpoly.Mesh([(off(a), off(c), off(b), (12, 8, 20, 255)) for a, b, c, col in m.tris])

hull_rows, hull_incs = [], []
seren = characters.ALL["seren"]()
for part in ("body", "arm", "arm_l", "leg_l", "leg_r"):
    hull(seren[part]).save(os.path.join(data, "hull_%s.fmesh" % part))
    hull_incs.append('#include "hull_%s_fmesh.h"' % part)
    hull_rows.append("\t{0, PK_%s, hull_%s_fmesh, hull_%s_fmesh_size}," % (part.upper(), part, part))

world = sorted(f[:-6] for f in os.listdir(data) if f[:2] in ("w_", "e_", "r_") and f.endswith(".fmesh"))
with open(os.path.join(HERE, "..", "source", "parts_gen.hpp"), "w") as f:
    f.write("// generiert von art/build_all.py\n#pragma once\n#include <stdint.h>\n#include <stddef.h>\n" + "\n".join(incs) + "\n")
    f.write("\n".join('#include "%s_fmesh.h"' % w for w in world) + "\n")
    f.write("enum CharId { %s, CH_COUNT };\n" % ", ".join("CH_" + n.upper() for n in characters.ALL))
    f.write("enum PartKind { %s, PK_COUNT };\n" % ", ".join("PK_" + k.upper() for k in KINDS))
    f.write("struct PartSrc { int ch, kind; const uint8_t *data; size_t size; };\n")
    f.write("static const PartSrc PART_SRC[] = {\n" + "\n".join(rows) + "\n};\n")
    f.write("enum WorldMesh { %s, WM_COUNT };\n" % ", ".join("WM_" + (w[2:] if w.startswith("w_") else w).upper() for w in world))
    f.write("\n".join(hull_incs) + "\n")
    f.write("static const PartSrc HULL_SRC[] = {\n" + "\n".join(hull_rows) + "\n};\n")
    f.write("static const PartSrc WORLD_SRC[] = {\n" + "\n".join("\t{%d, 0, %s_fmesh, %s_fmesh_size}," % (i, w, w) for i, w in enumerate(world)) + "\n};\n")
print("Figuren:", tri_total, "Dreiecke, Welt:", len(world), "Modelle")
