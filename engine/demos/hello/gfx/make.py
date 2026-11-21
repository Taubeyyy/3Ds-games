# Erzeugt die Sprites der Hallo-Demo
import os, sys
D = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(D, "..", "..", "..", "..", "tools"))
from artlib import radial, glow_dot
from PIL import Image, ImageDraw
def orb(col, path):
    s = 4
    im = radial(32 * s, 32 * s, 12 * s, 11 * s, 20 * s, [(0, (255, 255, 255, 255)), (0.35, col + (255,)), (1, tuple(c // 3 for c in col) + (255,))])
    m = Image.new("L", im.size, 0); ImageDraw.Draw(m).ellipse([2 * s, 2 * s, 30 * s, 30 * s], fill=255); im.putalpha(m)
    im.resize((32, 32), Image.LANCZOS).save(path)
os.makedirs(os.path.join(D, "png"), exist_ok=True)
orb((90, 200, 255), os.path.join(D, "png", "orb_blue.png"))
orb((255, 120, 200), os.path.join(D, "png", "orb_pink.png"))
g = radial(32, 32, 16, 16, 16, [(0, (255, 255, 255, 255)), (0.4, (255, 255, 255, 140)), (1, (255, 255, 255, 0))])
g.save(os.path.join(D, "png", "glow.png"))
open(os.path.join(D, "sprites.t3s"), "w").write("--atlas -f rgba8 -z auto\npng/orb_blue.png\npng/orb_pink.png\npng/glow.png\n")
