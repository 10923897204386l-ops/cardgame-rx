# Cardgame-RX

M0.1A is a static desktop card preview, not playable Klondike. It uses C17,
SDL2 and CMake, following the shared RG35XX Devkit desktop stack. Models and
layout do not depend on SDL or physical input. Court portraits are temporary,
original geometric illustrations; final traditional court artwork is pending.

## Build, test and run

On Fedora Kinoite, enter your existing development Toolbox (the shared devkit
uses `rg35xx-dev`). Inside it install missing build dependencies if needed:

```sh
sudo dnf install gcc cmake ninja-build pkgconf-pkg-config SDL2-devel
cmake -S . -B build/desktop -G Ninja
cmake --build build/desktop
ctest --test-dir build/desktop --output-on-failure
./build/desktop/card-preview
```

Run the preview in a logged-in graphical session. Tests need no display; they
include software rendering without opening a window. Dependencies are standard
Fedora packages; no bundled platform code or new external artwork is required.

The fixed-size window defaults to 640×480. `--scale 2` (also 3 or 4) uses exact
integer nearest-neighbour scaling with the same 4:3 aspect ratio. Resize is
intentionally disabled. Escape or window close exits. T toggles the tall-stack
stress scene; B toggles blue/wine card backs. These are preview controls only.

```sh
./build/desktop/card-preview --stress --theme wine
./build/desktop/card-preview --seed 42 --scale 2
./build/desktop/card-preview --seed random
```

Seed 42 is the reproducible default; the random option reads a seed from the
OS and prints it for replay. The shuffle algorithm is documented in `model.h`.
Theme selection never changes the scene model. Change `BackTheme` colours and
pattern in `render.c` to add designs; custom image loading is deferred.

## Exact 640×480 captures

Capture writes the logical surface, regardless of window scale, then exits:

```sh
./build/desktop/card-preview --seed 42 --capture build/desktop/tableau.bmp
./build/desktop/card-preview --stress --theme wine --capture build/desktop/stress.bmp
```

For capture on a machine without a display, prefix with `SDL_VIDEODRIVER=dummy`.
BMP files require no image libraries. PNG evidence is in `docs/screenshots/`.
`--smoke` presents one frame in a window and exits for launch verification.

## Source boundaries

- `src/model.*`: identities, 52-card deck, explicit seed shuffle, ordered pile
  snapshots and checked reads. Index 0 is bottom; count−1 is top. Face state is
  separate from identity. Construction rejects invalid/duplicate identities.
- `src/layout.*`: configurable card size, spacing and overlap; pure rectangles
  and exposed strips. Invalid configurations and out-of-range reads fail.
- `src/scene.*`: deterministic preview fixture; no move legality is implied.
- `src/render.*`: pixel drawing, grouped rank/suit index, pips and back themes.
- `src/preview.c`: SDL window, capture, command line and preview-only keys.
- `tests/`: model/layout/fixture checks and offscreen renderer checks.

The normal tableau fits the canvas. Stress intentionally extends below it and
clips; final overflow policy, controller mappings, game rules and hardware
packaging are outside M0.1A. See `tasks/M0.1A-REPORT.md` for verification and
M0.1B handoff. No RG35XXSP execution has been verified.
