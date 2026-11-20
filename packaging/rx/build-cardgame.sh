#!/bin/sh
# Native on-device build for StockOS / RG35XXSP, patterned after go-rx packaging/rx/build-go.sh.
set -eu
project=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$project"
out=${1:-croupier-rx}
cflags=$(sdl2-config --cflags)
libs=$(sdl2-config --libs)
# Assets live beside the binary in the release tree.
asset_dir=${CARDGAME_ASSET_DIR:-assets/runtime}
font_path=${CARDGAME_FONT_PATH:-assets/fonts/PermanentMarker-Regular.ttf}
gcc -std=c17 -O2 -Wall -Wextra -Werror \
    -DCARDGAME_ASSET_DIR=\"$asset_dir\" \
    -DCARDGAME_FONT_PATH=\"$font_path\" \
    $cflags -Isrc \
    src/model.c src/layout.c src/scene.c src/klondike.c \
    src/play.c src/play_view.c src/play_render.c \
    src/keyboard.c src/desktop.c src/render.c src/fx.c src/audio.c \
    src/game.c \
    $libs -lm -o "$out"
printf 'Built %s\n' "$out"
file "$out" || true
ldd "$out" || true
