#!/bin/bash
# Baut ein FORGE-Spiel für den PC (headless) und führt optional ein Eingabe-Skript aus.
# Nutzung: engine/tools/pc.sh <spielordner> [skript] [ausgabeordner]
set -e
ENG="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$(cd "$1" && pwd)"
NAME="$(basename "$GAME")"
B="${FORGE_PCBUILD:-/tmp/forge-pc}/$NAME"
mkdir -p "$B/gen" "$B/gfx" "$B/save" "$B/shots"
[ -f "$B/font.bin" ] || python3 "$ENG/tools/mkfont.py" "$B/font.bin"
# Platzhalter für eingebettete Sheets: <name>_t3x.h
GEN="$B/gen/sheets.cpp"; echo "// generiert" > "$GEN"
for t in "$GAME"/gfx/*.t3s; do
	[ -f "$t" ] || continue
	n="$(basename "$t" .t3s)"
	printf '#pragma once\nextern const unsigned char %s_t3x[];\nextern const unsigned int %s_t3x_size;\n' "$n" "$n" > "$B/gen/${n}_t3x.h"
	printf 'extern const unsigned char %s_t3x[] = "%s";\nextern const unsigned int %s_t3x_size = sizeof("%s");\n' "$n" "$n" "$n" "$n" >> "$GEN"
	python3 "$ENG/tools/t3s2bin.py" "$B/gfx" "$t"
done
# Eingebettete Daten (data/*): <name>_<ext>.h + Byte-Arrays
for d in "$GAME"/data/*.*; do
	[ -f "$d" ] || continue
	python3 - "$d" "$B/gen" "$GEN" <<'PY'
import os, sys
path, gen, out = sys.argv[1:4]
sym = os.path.basename(path).replace(".", "_").replace("-", "_")
data = open(path, "rb").read()
open(os.path.join(gen, sym + ".h"), "w").write("#pragma once\n#include <stdint.h>\nextern const uint8_t %s[];\nextern const uint8_t %s_end[];\nextern const uint32_t %s_size;\n" % (sym, sym, sym))
with open(out, "a") as f:
    f.write("#include <stdint.h>\nextern const uint8_t %s[] = {%s};\nextern const uint8_t %s_end[] = {0};\nextern const uint32_t %s_size = %d;\n" % (sym, ",".join(str(b) for b in data) or "0", sym, sym, len(data)))
PY
done
# Optionale Module aus dem Spiel-Makefile (FORGE_MODULES := ...)
MODS="$(grep -E '^FORGE_MODULES' "$GAME/Makefile" 2>/dev/null | sed 's/.*=//')"
MODSRC=""; MODINC=""; MODLIB=""
for m in $MODS; do
	MODSRC="$MODSRC $(ls "$ENG"/modules/$m/*.cpp "$ENG"/modules/$m/pc/*.cpp 2>/dev/null || true)"
	if [ "$m" = physics2d ]; then
		T=/tmp/forge-pc/third/box2d
		if [ ! -f $T/build-pc/libbox2d.a ]; then
			mkdir -p /tmp/forge-pc/third && (cd /tmp/forge-pc/third && [ -d box2d ] || git clone -q --depth 1 --branch v2.4.1 https://github.com/erincatto/box2d.git)
			(cd $T && mkdir -p build-pc && g++ -std=c++11 -O2 -w -Iinclude -Isrc -c $(cd $T && find src -name "*.cpp") && ar rcs build-pc/libbox2d.a *.o && rm -f *.o)
		fi
		MODINC="$MODINC -I$T/include"; MODLIB="$MODLIB $T/build-pc/libbox2d.a"
	fi
	if [ "$m" = phys3d ]; then
		T=/tmp/forge-pc/third/bullet3
		if [ ! -f $T/build-pc/libbullet.a ]; then
			mkdir -p /tmp/forge-pc/third && (cd /tmp/forge-pc/third && [ -d bullet3 ] || git clone -q --depth 1 --branch 2.87 https://github.com/bulletphysics/bullet3.git)
			(cd $T && mkdir -p build-pc/obj && find src/LinearMath src/BulletCollision src/BulletDynamics -name "*.cpp" | \
				xargs -P 8 -I{} sh -c 'g++ -std=c++11 -O2 -w -fno-exceptions -Isrc -c {} -o build-pc/obj/$(echo {} | tr / _).o' && \
				ar rcs build-pc/libbullet.a build-pc/obj/*.o && rm -rf build-pc/obj)
		fi
		MODINC="$MODINC -I$T/src"; MODLIB="$MODLIB $T/build-pc/libbullet.a"
	fi
done
g++ -std=gnu++17 -O2 -w -I"$ENG/include" -I"$GAME/source" -I"$B/gen" $MODINC \
	"$GAME"/source/*.cpp $(ls "$ENG"/src/*.cpp 2>/dev/null) $MODSRC "$ENG/platform/pc/backend.cpp" "$GEN" -o "$B/$NAME" $MODLIB -lm
if [ -n "$2" ]; then
	OUT="${3:-$B/shots}"; mkdir -p "$OUT"
	FORGE_SCRIPT="$2" FORGE_OUT="$OUT" FORGE_FONT="$B/font.bin" FORGE_GFX="$B/gfx" FORGE_SAVE="$B/save" FORGE_ROMFS="$GAME/romfs-pc" "$B/$NAME"
fi
echo "$B/$NAME"
