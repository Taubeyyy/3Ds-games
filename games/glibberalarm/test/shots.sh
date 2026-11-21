#!/bin/bash
# Screenshots ueber den Desktop-Shim. Nutzung: test/shots.sh <skript>
set -e
cd "$(dirname "$0")/.."
B=/tmp/claude-0/b/g; mkdir -p $B/save $B/shots
[ -f $B/font.bin ] || python3 test/mkfont.py $B/font.bin
python3 gfx/make_gfx.py $B/gfx >/dev/null
gcc -O2 -w -DDESKTOP_TEST -Itest/shim -Isource -o $B/desk source/main.c source/td.c test/shim/shim.c -lm
rm -f $B/save/* $B/shots/*
SHIM_GFX=$B/gfx SHIM_FONT=$B/font.bin SHIM_SCRIPT="$1" SHIM_SAVE=$B/save SHIM_OUT=$B/shots $B/desk
