# Cardgame-RX project state

Updated: 2026-11-20.

## Verified baseline
- Main contains M0.1A (#1, `97e82e0`), M0.1B (#2, `a880e7c`) and M0.1C
  (#3, `58fb6ac`) squash merges.
- M0.1C post-merge review found passing automated tests, but manual play-through
  requested immediate corrections: spatial arrow navigation, thicker rounded
  focus, rounded card corners and no source ghost while holding cards.
- C17/SDL2 playable desktop app launches at 1×/2×; normal/random seeds, alternate
  back theme, custom keys and configurable undo limits exercised.
- Full desktop CTest: 6/6 pass, including M0.1A/M0.1B regressions. SDL-free Release
  CTest: 4/4 pass; interaction test links only libc/loader. Offscreen desktop
  interaction/render tests also pass without DISPLAY/WAYLAND_DISPLAY.
- Automated SDL key sequence covers stock cycle, tableau move, invalid drop,
  cancellation, undo and confirmed restart. Manual keyboard review drove the
  current corrective branch.
- Whole held overlay without source ghosting, 300 ms invalid outline,
  tall-index visibility, pause/confirmation and win checked offscreen.
  Evidence: `docs/screenshots/m01c/`.
- M0.1A normal/stress captures remain 640×480 and pixel-identical to baseline.
- Configurable bounded undo and GNU/Linux failed-allocation tests pass.
- M0.1D is implemented on `feat/m0-1d-croupier-visuals`: locked title art,
  Kennard card/felt assets, lighter green felt with sparse peripheral petals,
  Go-RX-style font/menu panels, main menu, expanded pause menu and Select-toggled
  controls bar with L2+Select undo. Acceptance screenshots are in
  `docs/screenshots/m01d/`.
- No RG35XXSP hardware testing, device mapping or packaging has been performed.

## Goal
Reusable card-game engine for RG35XXSP; Klondike first, 500 later. Keep rules,
input, transient interaction and rendering separate.

## Current milestone
M0.1D Croupier-RX visuals, menus and controls HUD are implemented locally on
`feat/m0-1d-croupier-visuals`, awaiting final review and PR. See
`tasks/M0.1D-REPORT.md` for commands and limits.

## Known limits
- Tall-column layout is a readable prototype, not final device-polished scrolling.
- Saves, audio and physical device support are absent.

## One next action
Review M0.1D at native 640×480, then open a squash-merge PR if accepted. Do not
begin device packaging or another milestone without approval.
