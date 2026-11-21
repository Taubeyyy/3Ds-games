"""Banner, Banner-Ton und Website-Bilder aus einem PC-Screenshot des Titelbilds."""
import os, sys, wave, struct
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
shot = Image.open(sys.argv[1]).convert("RGB").crop((0, 0, 400, 240))
b = shot.resize((256, 154), Image.LANCZOS).crop((0, 0, 256, 128))
b.save(os.path.join(ROOT, "meta", "banner.png"))
docs = os.path.join(ROOT, "..", "..", "docs", "img")
shot.save(os.path.join(docs, "skybreaker-banner.png"))
Image.open(os.path.join(ROOT, "gfx", "png", "sigil_seren.png")).resize((48, 48), Image.LANCZOS).save(os.path.join(docs, "skybreaker-icon.png"))
with wave.open(os.path.join(ROOT, "data", "theme.wav")) as w:
    sr = w.getframerate()
    d = np.frombuffer(w.readframes(int(sr * 3.0)), dtype=np.int16).astype(np.float64)
d *= np.minimum(1, np.linspace(3.0, 0, len(d)) / 0.8)
with wave.open(os.path.join(ROOT, "meta", "banner.wav"), "wb") as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr)
    w.writeframes(d.astype(np.int16).tobytes())
print("ok")
