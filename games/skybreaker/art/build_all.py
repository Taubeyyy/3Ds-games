"""Alle SKYBREAKER-Assets erzeugen und source/parts_gen.hpp schreiben."""
import os, sys, subprocess
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
for script in ("gfx.py", "world.py", "music.py"):
    subprocess.check_call([sys.executable, os.path.join(HERE, script)], stdout=subprocess.DEVNULL)
import characters
data = os.path.join(HERE, "..", "data")
KINDS = ["body", "arm", "hair", "scarf", "cape", "glow", "fx", "weapon", "weapon_glow", "orbit", "orbit_glow", "pet"]
rows, incs = [], []
tri_total = 0
for ci, (name, fn) in enumerate(characters.ALL.items()):
    for part, m in fn().items():
        nv, nt = m.save(os.path.join(data, "%s_%s.fmesh" % (name, part)))
        tri_total += nt
        sym = "%s_%s_fmesh" % (name, part)
        incs.append('#include "%s.h"' % sym)
        rows.append("\t{%d, PK_%s, %s, %s_size}," % (ci, part.upper(), sym, sym))
world = sorted(f[:-6] for f in os.listdir(data) if f.startswith("w_") and f.endswith(".fmesh"))
with open(os.path.join(HERE, "..", "source", "parts_gen.hpp"), "w") as f:
    f.write("// generiert von art/build_all.py\n#pragma once\n#include <stdint.h>\n#include <stddef.h>\n" + "\n".join(incs) + "\n")
    f.write("\n".join('#include "%s_fmesh.h"' % w for w in world) + "\n")
    f.write("enum CharId { %s, CH_COUNT };\n" % ", ".join("CH_" + n.upper() for n in characters.ALL))
    f.write("enum PartKind { %s, PK_COUNT };\n" % ", ".join("PK_" + k.upper() for k in KINDS))
    f.write("struct PartSrc { int ch, kind; const uint8_t *data; size_t size; };\n")
    f.write("static const PartSrc PART_SRC[] = {\n" + "\n".join(rows) + "\n};\n")
    f.write("enum WorldMesh { %s, WM_COUNT };\n" % ", ".join("WM_" + w[2:].upper() for w in world))
    f.write("static const PartSrc WORLD_SRC[] = {\n" + "\n".join("\t{%d, 0, %s_fmesh, %s_fmesh_size}," % (i, w, w) for i, w in enumerate(world)) + "\n};\n")
print("Figuren:", tri_total, "Dreiecke, Welt:", len(world), "Modelle")
