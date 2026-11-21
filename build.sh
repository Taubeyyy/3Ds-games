#!/bin/bash
# Baut alle Spiele (oder nur das angegebene) als .3dsx und .cia nach dist/
# Nutzung: ./build.sh [spielname]
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
IMG=3ds-sammlung-build
if [ -z "$IN_DOCKER" ]; then
  if ! docker image inspect $IMG >/dev/null 2>&1; then
    if [ -x "$ROOT/tools/bin/makerom" ]; then docker build -q -t $IMG -f "$ROOT/tools/Dockerfile.local" "$ROOT/tools" >/dev/null
    else docker build -q -t $IMG "$ROOT/tools" >/dev/null; fi
  fi
  exec docker run --rm -e IN_DOCKER=1 -v "$ROOT":/w -w /w $IMG ./build.sh "$@"
fi
mkdir -p dist
# Engine-Demos (Compile-Check)
if [ -z "$1" ]; then
  for d in engine/demos/*/; do
    [ -f "$d/Makefile" ] && { echo "== Demo $(basename "$d")"; make -C "$d" -s >/dev/null; }
  done
fi
for dir in games/${1:-*}/; do
  g="$(basename "$dir")"
  source "$dir/game.cfg"
  echo "== $TITLE"
  make -C "$dir" -s
  out="$dir/build"
  bannertool makebanner -i "$dir/meta/banner.png" -a "$dir/meta/banner.wav" -o "$out/banner.bnr" >/dev/null
  bannertool makesmdh -s "$TITLE" -l "$DESC" -p "$AUTHOR" -i "$dir/meta/icon.png" -o "$out/icon.smdh" >/dev/null
  IFS=. read MA MI MC <<< "$VERSION"
  makerom -f cia -o "dist/$NAME.cia" -target t -exefslogo -elf "$dir/$NAME.elf" -rsf tools/cia.rsf \
    -icon "$out/icon.smdh" -banner "$out/banner.bnr" -major $MA -minor $MI -micro $MC \
    -DAPP_TITLE="$TITLE" -DAPP_PRODUCT_CODE="$PRODUCT_CODE" -DAPP_UNIQUE_ID="$UNIQUE_ID" \
    -DAPP_SYSTEM_MODE=64MB -DAPP_SYSTEM_MODE_EXT=Legacy -DAPP_CATEGORY=Application \
    -DAPP_USE_ON_SD=true -DAPP_MEMORY_TYPE=Application -DAPP_CPU_SPEED=268MHz \
    -DAPP_ENABLE_L2_CACHE=false -DAPP_VERSION_MAJOR=$MA -DAPP_ENCRYPTED=false
  cp "$dir/$NAME.3dsx" dist/
  ls -la "dist/$NAME.cia" "dist/$NAME.3dsx" | awk '{print $5, $9}'
done
