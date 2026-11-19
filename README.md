# Cardgame-RX

M0.1A provides a static desktop preview; M0.1B adds a pure Klondike rules
engine with undo. There is no interactive Klondike UI yet. It uses C17,
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

## Klondike rules (M0.1B)

`src/klondike.h` exposes deterministic deal, validation, exact legality,
longest-legal-suffix evaluation, atomic transfer/draw/recycle and undo. It uses
only the generic model and C standard library, with no SDL or input dependency.
The static preview remains its original fixture; it does not run the rules.

For a rules-only build without SDL, pkg-config or a graphical session:

```sh
cmake -S . -B build/rules-only -G Ninja -DCARDGAME_BUILD_PREVIEW=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/rules-only
ctest --test-dir build/rules-only --output-on-failure
```

Initialize an owning `KlondikeGame` with `{0}`, then call
`klondike_new_game(&game, seed)`. Use `game.state` as read-only authoritative
state. Evaluate proposals with `klondike_can_move` or
`klondike_longest_suffix`; neither changes state or creates undo history.
`klondike_move` commits an exact suffix; `klondike_drop` commits the longest
legal suffix from the candidate. A rejected operation preserves state and
history. `klondike_undo` restores the previous complete state. New-game clears
history; `klondike_dispose` frees history when finished. Do not shallow-copy an
owning game; copy `KlondikeState` for independent read-only snapshots instead.

All piles are bottom-to-top. A transfer index selects `[index, count)`; only
face-up valid runs can be candidates. Waste and foundation sources offer only
their top card. Tableau builds down with alternating colours, and empty
columns require a king. Foundation slots acquire a suit with their first ace
and build up in that suit. Foundation tops can return to tableau; transfers
between foundations are forbidden. Successful transfers automatically reveal
an exposed hidden tableau top in the same undo step.

Draw one with `klondike_draw`. When stock is empty, `klondike_recycle` reverses
waste into face-down stock without shuffling. The oldest surviving waste card
is then next to draw; there is no redeal limit. Drawing empty stock and
recycling with nonempty stock or empty waste fail without creating history.
Win status is derived with `klondike_won` rather than stored redundantly.

Undo stores one full state snapshot per committed action in a dynamically
allocated history. It grows until undone, reset or disposed; allocation failure
rejects the action atomically. No cursor/selection state enters this history.
See `tasks/M0.1B-REPORT.md` for acceptance evidence and the next handoff.
