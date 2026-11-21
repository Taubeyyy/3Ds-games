# Wandelt .t3s-Listen in Desktop-Sheets (<name>.bin) für das PC-Backend um.
# Nutzung: t3s2bin.py <ausgabeordner> <datei.t3s>...
import os, struct, sys
from PIL import Image
out = sys.argv[1]
os.makedirs(out, exist_ok=True)
for t3s in sys.argv[2:]:
    base = os.path.dirname(os.path.abspath(t3s))
    pngs = [l.strip() for l in open(t3s) if l.strip() and not l.strip().startswith("-") and not l.strip().startswith("#")]
    name = os.path.splitext(os.path.basename(t3s))[0]
    with open(os.path.join(out, name + ".bin"), "wb") as f:
        f.write(struct.pack("<I", len(pngs)))
        for p in pngs:
            im = Image.open(os.path.join(base, p)).convert("RGBA")
            f.write(struct.pack("<HH", im.width, im.height))
            f.write(im.tobytes())
