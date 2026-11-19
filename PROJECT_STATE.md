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
- User keyboard correction applied: shared devkit physical-key defaults and
  A-confirm/B-back pause menus. Manual feedback corrections are in progress on
  `fix/m0-1c-manual-feedback`.
- No RG35XXSP hardware testing, device mapping or packaging has been performed.

## Goal
Reusable card-game engine for RG35XXSP; Klondike first, 500 later. Keep rules,
input, transient interaction and rendering separate.

## Current milestone
M0.1C playable desktop slice is merged. Post-merge corrective work is in progress
for manual keyboard/visual feedback before starting M0.1D. See
`tasks/M0.1C-REPORT.md` for commands and limits.

## Known limits
- Original M0.1C accepted area-cycle navigation; manual review now requires
  spatial arrows as the primary navigation model.
- Tall-column layout is a minimal readable prototype, not final visual polish.
- Court art remains temporary; saves, audio and physical device support are absent.

## One next action
Finish the manual-feedback corrective branch, rerun tests and relaunch for user
verification. Do not begin M0.1D or device packaging until this correction is
accepted.
