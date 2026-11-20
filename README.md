# Croupier-RX

Croupier-RX is a reusable card-game engine for RG35XXSP-class 640×480 devices,
currently shipping playable desktop Klondike. M0.1D adds the Croupier-RX identity,
locked Hawaiian casino title art, Go-RX-style menus, Kennard card faces, custom
backs and a lighter textured green felt table with sparse peripheral petals. C17, SDL2 and CMake follow the shared
RG35XX Devkit stack. No RG35XXSP build or hardware controls have been verified.

## Build, test and play

On Fedora Kinoite, enter your existing development Toolbox (the shared devkit
uses `rg35xx-dev`). Install missing build dependencies there if needed:

```sh
sudo dnf install gcc cmake ninja-build pkgconf-pkg-config SDL2-devel
cmake -S . -B build/desktop -G Ninja
cmake --build build/desktop
ctest --test-dir build/desktop --output-on-failure
./build/desktop/cardgame
```

Run the application in a logged-in graphical session. All tests can run without
a display; rendering tests use offscreen software surfaces, without windows.

```sh
./build/desktop/cardgame --seed 42 --scale 2 --theme wine
./build/desktop/cardgame --seed random --undo-limit 128
./build/desktop/cardgame --bind b=F1 --bind start=Return
```

Seed 42 is the deterministic default. `random` reads and prints an OS seed for
replay. Confirmed New Game deals seed+1 (wrapping at 32 bits). `--scale 1..4`
uses a fixed-size 4:3 window and nearest-neighbour integer scaling. The default
navy and alternate wine card backs are asset-backed; `--theme wine` selects the
wine back. Custom image import is deferred.

## Keyboard emulates handheld buttons

| Logical input | Default key | In gameplay |
| --- | --- | --- |
| D-pad Left/Right | Left/Right arrows | Spatially adjacent visible pile/card; wraps at row edges |
| D-pad Up/Down | Up/Down arrows | Spatially up/down between tableau cards and the upper row |
| B | S | Pick up; attempt longest legal suffix placement; source pile cancels |
| A | D | Carry waste top at current cursor; while holding cancel and return to source |
| X | W | Move selected single eligible card to first legal foundation |
| Y | A | Draw one, or recycle when stock empty; ignored while holding |
| L1/R1 | Left Shift/Space | Optional area shortcuts: Draw → Tableau → Foundations; L1 reverses |
| L2/R2 | Left Ctrl/Left Alt | First/last pile, including during carry |
| Start | E | Pause / resume |
| Select | Q | Show/hide optional controls bar, hidden by default |
| L2+Select | Left Ctrl+Q | Undo one committed action when not holding cards |
| Quit | Escape | Exit the desktop app |

The area remembers its last pile (initially stock, tableau 0, foundation 0).
Arrow keys are spatial: Left/Right move to neighbouring visible piles/cards with
wraparound, Up from the top selectable tableau card enters the nearest upper-row
target, and Down from the upper row returns to the nearest tableau column. Hidden
cards are never selectable. During carry, these buttons change destination
focus/selection only; the held source and candidate stay fixed. Focus uses a
thicker rounded yellow outline, becoming green when the held candidate has a
legal destination suffix.

The app starts on a Croupier-RX main menu with Play, New Game, Options and Quit.
Pause uses Up/Down and A (D) to choose Resume, New Game, Options or Main Menu.
New Game and Main Menu require confirmation, with **No selected initially**;
arrows toggle No/Yes and A accepts. B or Start backs out of confirmation; at the
main pause menu B/Start resumes. Gameplay actions are gated while paused. Pause
preserves a pending carry.
During gameplay, Select toggles an optional bottom controls bar. The bar uses the
same comic-style font and warm cream/yellow treatment as the status HUD, lists
the active mappings, and is drawn underneath cards so it does not obscure the
tableau or held stacks. L2+Select performs Undo; while holding cards it reports
the holding guard instead of mutating rules state.
Window close or Escape exits. These physical-key defaults match the shared
devkit desktop input mapping. Its M key (X) and Ctrl+R debug reload have no
gameplay binding here; New Game still requires confirmation. T/B preview debug
keys remain confined to the static preview.

Use repeated `--bind action=KeyName` options for configuration. Action names are
`left`, `right`, `up`, `down`, `a`, `b`, `x`, `y`, `l1`, `r1`, `l2`, `r2`, `start`,
`select`, `quit`. Key names are SDL physical scancode names such as `Space`,
`Left Shift`, `Return` or `F1`. Physical positions are used, as in the devkit,
independently of keyboard layout.
Invalid keys and final duplicate assignments are rejected before launch; key
swaps can be supplied as multiple overrides. OS key-repeat events are ignored,
so each physical press generates one action.

## Artwork, carry, feedback and overflow

The locked 640×480 Croupier-RX title image and complete card pack are documented
in `assets/source/ASSET_MANIFEST.json`. Runtime uses deterministic BMP copies
generated from the pinned PNG assets so SDL can load them without a new image
decoder dependency. Source-controlled provenance includes the Kennard SVG source
cards, rasterization script, Permanent Marker Apache-2.0 licence/metadata and the
complete asset manifest. Menus and gameplay HUD use the bundled genuine
Permanent Marker Regular font from Google Fonts / Font Diner, with warm
light-yellow foreground panels.

The full candidate follows the cursor as an overlay. Its unchanged source remains
rendered normally; there is no source ghost while holding. The status bar shows
HOLD count and the count that PLACE would transfer. Pickup,
navigation, preview and cancellation do not change game state or undo history.
B tries the full run then shorter suffixes; one valid placement and any exposed
hidden-card flip form one atomic undo step. A rejected drop keeps the same
held cards and cursor, flashes a red outline around the **whole held stack for
300 ms**, then returns to cyan. Time is monotonic and expiry never blocks input.
The dispatcher returns semantic `PLAY_ERROR` for a future sound effect; M0.1C
adds no audio dependency or playback.

Tall columns keep every face-up index visible: normal spacing is used when it
fits; otherwise the hidden prefix collapses into at most 24 pixels and face-up
strips use 22 pixels. DOWN count labels collapsed hidden cards. Only the top
card's lower pip artwork may clip at the canvas bottom, marked by an ellipsis;
selectable indexes and focus remain visible. A held run is independently
positioned to fit entirely below the status bar. This is a minimal desktop
overflow policy, not final handheld layout polish.

Win is shown in the top bar; Start → New Game remains available.

## Undo resource policy

The desktop defaults to the newest **256** snapshot transactions. Change with
`--undo-limit N`; zero explicitly requests unlimited history. The rules library
keeps its M0.1B zero/unlimited default for compatibility. Call
`klondike_set_undo_limit` to configure or immediately prune older snapshots;
the setting survives new-game and disposal. Oldest pruned actions cannot be
undone. Allocation failure rejects the action atomically. GNU/Linux tests inject
failed snapshot allocations for draw, suffix placement and recycle.

## Static preview and exact captures

The M0.1A preview remains independently runnable, with unchanged normal/stress
fixtures and pixel output:

```sh
./build/desktop/card-preview --stress --theme wine
./build/desktop/card-preview --seed 42 --scale 2
./build/desktop/card-preview --seed random
```

Only the static preview uses T for stress, B for back theme and Escape to exit.
Both programs support exact **640×480** logical-surface BMP capture regardless
of window scale, and `--smoke` to present one window frame then exit:

```sh
./build/desktop/cardgame --capture build/desktop/game.bmp
./build/desktop/card-preview --capture build/desktop/tableau.bmp
./build/desktop/card-preview --stress --theme wine --capture build/desktop/stress.bmp
```

Prefix capture commands with `SDL_VIDEODRIVER=dummy` when no display is present.
PNG evidence is in `docs/screenshots/`; M0.1C views are in `m01c/`, M0.1D
views are in `m01d/`, M0.1E desktop-polish validation views are in `m01e/`, and
the Permanent Marker UI font replacement evidence is in `m01e-permanent-marker/`.
The desktop test writes invalid, pause, confirmation, controls-bar hidden/shown,
tall, held and win BMP fixtures in its build/test working directory. BMP capture adds no image library;
the committed PNGs are lossless conversions for review.

## Module and rules boundaries

- `model.*`, `layout.*`: generic card identity, face state, standard deck,
  deterministic shuffle, ordered pile snapshots, bounds and pure rectangles.
- `klondike.*`: deterministic deal, valid state/win, exact and longest-suffix
  legality, atomic actions, reveal, undo and configurable history budget.
- `controls.h`: platform-neutral logical inputs; `keyboard.*`: SDL key mapping.
- `play.*`: Klondike-specific focus, pending carry, menus and timed feedback.
- `play_view.*`: pure playable layout; `play_render.*`: actual-state presentation.
- `desktop.*`, `game.c`: SDL event boundary, window, capture and application loop.
- `scene.*`, `render.*`, `preview.c`: retained static fixture, shared drawing and
  preview entry point. Generic drawing contains no new Klondike rule policy.

The rules, interaction and layout libraries do not depend on SDL. For an SDL-free
Release build (also omits pkg-config discovery):

```sh
cmake -S . -B build/rules-only -G Ninja -DCARDGAME_BUILD_PREVIEW=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/rules-only
ctest --test-dir build/rules-only --output-on-failure
```

Initialize owning `KlondikeGame` with `{0}`, then `klondike_new_game(&game,seed)`.
Treat `game.state` as read-only. `klondike_can_move` and
`klondike_longest_suffix` are pure evaluators; `klondike_move` commits an exact
suffix, `klondike_drop` commits the longest legal candidate suffix. New-game
clears history; `klondike_dispose` frees it. Do not shallow-copy an owning game;
copy `KlondikeState` for independent read-only snapshots. `PlayUI` never enters
rules history.

All piles are bottom-to-top. Tableau builds down in alternating colours; empty
columns require a king. Foundations acquire a suit with their first ace and
build up in that suit. Foundation tops may return to tableau; transfers between
foundations are forbidden. Waste and foundation sources offer only their top
card. Draw one and reverse surviving waste into face-down stock when stock is
empty, without shuffling or redeal limits; its oldest surviving card draws next.
Win is derived from four complete valid foundations. See milestone reports in
`tasks/` for exact verification and limits. Device packaging is not part of this
implementation.
