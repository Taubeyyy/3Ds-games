# Glyphen-Atlas (Latin-1) fuer den Desktop-Shim, angenaehert an den 3DS-Systemfont
import sys
from PIL import Image, ImageDraw, ImageFont
f = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 22)
out = bytearray()
for cp in range(32, 256):
    ch = chr(cp)
    im = Image.new("L", (32, 30), 0)
    ImageDraw.Draw(im).text((0, 3), ch, font=f, fill=255)
    out.append(min(31, int(round(f.getlength(ch)))))
    out += im.tobytes()
open(sys.argv[1], "wb").write(out)
