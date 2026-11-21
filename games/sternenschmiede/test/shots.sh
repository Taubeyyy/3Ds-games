#!/bin/bash
# Erzeugt Screenshots ueber den Desktop-Shim. Nutzung: test/shots.sh <skript> <stunden-im-sim|0>
set -e
cd "$(dirname "$0")/.."
B=/tmp/claude-0/b/sf; mkdir -p $B/save $B/shots
[ -f $B/font.bin ] || python3 test/mkfont.py $B/font.bin
gcc -O2 -w -DDESKTOP_TEST -Itest/shim -o $B/desk source/main.c source/game.c source/expedition.c test/shim/shim.c -lm
gcc -O2 -w -o $B/sim test/sim.c source/game.c -lm
rm -f $B/save/*
[ "$2" != "0" ] && $B/sim 100 2.5 999 "$2" $B/save/save.dat >/dev/null
SHIM_FONT=$B/font.bin SHIM_SCRIPT="$1" SHIM_SAVE=$B/save SHIM_OUT=$B/shots $B/desk
