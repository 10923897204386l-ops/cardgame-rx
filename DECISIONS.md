# Cardgame-RX decisions

## 2026-11-16 — Project scope (user direction)
- Independent project named cardgame-rx.
- Build a reusable card-game engine rather than a Klondike-only game.
- Target eventual single-player and multiplayer card games, including Klondike and 500.
- Reuse go-rx and dungeon-rx workflow and project documentation conventions.

## 2026-11-16 — Initial architectural guidance (proposed, pending review)
- Separate rules/game state from presentation and input.
- Treat 500 as a future architectural test for hidden information, bidding, partnerships and trick-taking; do not implement networking in M0.
- Target RG35XXSP 640x480 and shared development infrastructure.
- Do not lock specific programming language, renderer, control mapping or network transport before devkit inspection.

## 2026-11-16 — Klondike as first implementation (confirmed)
- Klondike is the first playable game and acceptance vehicle, not the architecture's defining ruleset.
- Design the core as a reusable engine with independent game modules and shared platform/presentation services.
- Core concepts must accommodate hands, hidden information, bidding, partnerships and tricks for future games such as 500; do not force all games into solitaire piles.
- A new game should normally be added as a module/assets without changing core rules logic. Avoid speculative generality: implement shared abstractions incrementally, supported by concrete Klondike tests and future-game interface examples.

## 2026-11-19 — M0.1A implementation
- Use C17, SDL2, CMake/Ninja and CTest, matching the inspected shared
  rg35xx-devkit README. Do not copy its Go-specific UI, deployment or controller
  code into this independent project. Local inspection also confirmed the
  rg35xxsp-devel platform/input boundary; no device functionality added here.
- Keep the model/layout library free of SDL. Piles own ordered snapshots with
  separate face state and checked const reads; no move operations in M0.1A.
- Seeded Fisher–Yates uses explicit xorshift32 and rejection sampling to avoid
  platform RNG differences and modulo bias. Seed 0 aliases 0x6d2b79f5; default
  preview seed 42. OS random seed acquisition stays in the desktop entry point.
- Render original bitmap lettering/suits and temporary geometric courts without
  third-party art dependencies. Final traditional court art remains pending.
- Fix desktop windows to integer multiples of 640×480; render one native surface
  and scale with nearest-neighbour. Capture that surface directly as BMP.
- Default 72×100 cards and 24/12-pixel up/down overlap are configurable layout
  values, not final handheld measurements. Stress clipping is intentional and
  does not establish a final overflow strategy.

## 2026-11-19 — M0.1B rules and transaction boundaries
- Implement the user-approved M0.1B draw-one/unlimited-redeal rules in a separate
  `klondike` C library. Generic card/deck/pile and M0.1A preview modules stay unchanged.
- Deal column-wise from the seeded shuffled array: indices 0..27 go to tableau
  counts 1..7, with only each top face up. Store stock as array indices 51..28
  bottom-to-top, making array index 28 the first draw.
- Recycle only with empty stock and nonempty waste: reverse surviving waste
  into face-down stock. Its oldest surviving card is next to draw. No shuffle,
  redeal counter or redeal limit is needed. Draw and recycle are separate APIs.
- Foundation slots acquire their suit with an ace; build one rank at a time.
  Standard foundation-to-tableau reversals are allowed. Foundation-to-foundation
  transfers are forbidden, even when an ace could fit an empty foundation.
- Evaluate exact suffixes and longest legal suffixes without mutation. The
  selected candidate must itself be a valid face-up run; it cannot expand to
  include cards below the selected index. Invalid drops preserve state/history.
- Apply successful changes on a state copy, validate 52 identities and pile
  invariants, allocate an undo snapshot, then commit. Tableau reveal belongs to
  the same transaction. Snapshot undo restores orientation and pile order too.
- Keep owning history separate from the copyable authoritative state; do not
  store UI focus, held/candidate state or a redundant win flag. Win is derived
  from four valid 13-card foundations. Full snapshots favor correctness for this
  small state; history is released by undo, reset or dispose.
- `CARDGAME_BUILD_PREVIEW=OFF` allows rules/model tests without SDL discovery or
  linkage. Default builds preserve the M0.1A preview and renderer test suite.
