# Banner (aus einem PC-Screenshot), Banner-Sound und Website-Bilder für eine Engine-Demo
# Nutzung: demo_meta.py <screenshot.ppm> <demo-ordner> <id> "LOGO TEXT"
import os, sys, math, struct, wave
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
D = os.path.join(os.path.abspath(sys.argv[2]), "meta")
DID, TXT = sys.argv[3], sys.argv[4]
sys.path.insert(0, os.path.join(ROOT, "tools"))
from artlib import logo_text, paste_center
from PIL import Image, ImageFilter, ImageEnhance
shot = Image.open(sys.argv[1]).convert("RGB").crop((0, 0, 400, 240))
b = shot.resize((256, 154), Image.LANCZOS).crop((0, 13, 256, 141))
b = ImageEnhance.Brightness(b).enhance(0.8).convert("RGBA")
logo = logo_text(TXT, 34 if len(TXT) < 10 else 28, [(0, (255, 255, 255, 255)), (1, (150, 230, 255, 255))], stroke=3, stroke_col=(20, 16, 48), glow=3, glow_col=(90, 200, 255, 140))[0]
paste_center(b, logo, 128, 100)
b.convert("RGB").save(os.path.join(D, "banner.png"))
docs = os.path.join(ROOT, "docs", "img")
big = shot.resize((400, 240), Image.LANCZOS).convert("RGBA")
paste_center(big, logo_text(TXT, 52 if len(TXT) < 10 else 44, [(0, (255, 255, 255, 255)), (1, (150, 230, 255, 255))], stroke=4, stroke_col=(20, 16, 48), glow=4, glow_col=(90, 200, 255, 140))[0], 200, 196)
big.convert("RGB").save(os.path.join(docs, DID + "-banner.png"))
Image.open(os.path.join(D, "icon.png")).save(os.path.join(docs, DID + "-icon.png"))
# Kurzer Glocken-Akkord als Banner-Ton
sr, n = 22050, int(22050 * 1.6)
fr = [523.25, 659.25, 783.99, 1046.5]
data = []
for i in range(n):
    t = i / sr
    v = sum(math.sin(2 * math.pi * f * t + k) * math.exp(-t * (2.2 + k)) * (1 if t > k * 0.07 else 0) for k, f in enumerate(fr)) * 0.22
    data.append(int(max(-1, min(1, v)) * 32000))
with wave.open(os.path.join(D, "banner.wav"), "wb") as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr)
    w.writeframes(struct.pack("<%dh" % n, *data))
