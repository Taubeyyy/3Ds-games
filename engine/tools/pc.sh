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
g++ -std=gnu++17 -O2 -w -I"$ENG/include" -I"$GAME/source" -I"$B/gen" \
	"$GAME"/source/*.cpp $(ls "$ENG"/src/*.cpp 2>/dev/null) "$ENG/platform/pc/backend.cpp" "$GEN" -o "$B/$NAME" -lm
if [ -n "$2" ]; then
	OUT="${3:-$B/shots}"; mkdir -p "$OUT"
	FORGE_SCRIPT="$2" FORGE_OUT="$OUT" FORGE_FONT="$B/font.bin" FORGE_GFX="$B/gfx" FORGE_SAVE="$B/save" FORGE_ROMFS="$GAME/romfs-pc" "$B/$NAME"
fi
echo "$B/$NAME"
