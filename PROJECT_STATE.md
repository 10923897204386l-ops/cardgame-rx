# Cardgame-RX project state

Updated: 2026-11-19.

## Verified baseline
- Main contains M0.1A (#1, `97e82e0`) and M0.1B (#2, `a880e7c`) squash merges.
- M0.1C implemented on `feat/m0-1c-playable-controls`, pending PR review and
  squash merge. No direct push to main or automatic merge.
- C17/SDL2 playable desktop app launches at 1×/2×; normal/random seeds, alternate
  back theme, custom keys and configurable undo limits exercised.
- Full desktop CTest: 6/6 pass, including M0.1A/M0.1B regressions. SDL-free Release
  CTest: 4/4 pass; interaction test links only libc/loader. Offscreen desktop
  interaction/render tests also pass without DISPLAY/WAYLAND_DISPLAY.
- Automated SDL key sequence covers stock cycle, tableau move, invalid drop,
  cancellation, undo and confirmed restart. Human keyboard testing unavailable
  in this tool session; not claimed as manually played.
- Whole held overlay, ghost source, 300 ms invalid outline, tall-index visibility,
  pause/confirmation and win checked offscreen. Evidence: `docs/screenshots/m01c/`.
- M0.1A normal/stress captures remain 640×480 and pixel-identical to baseline.
- Configurable bounded undo and GNU/Linux failed-allocation tests pass.
- No RG35XXSP hardware testing, device mapping or packaging has been performed.

## Goal
Reusable card-game engine for RG35XXSP; Klondike first, 500 later. Keep rules,
input, transient interaction and rendering separate.

## Current milestone
M0.1C playable desktop slice implemented and locally verified, awaiting review.
M0.1D has not been started. See `tasks/M0.1C-REPORT.md` for commands and limits.

## Known limits
- Human keyboard play-through remains a reviewer check; automated SDL coverage
  and real desktop frame launches are distinct evidence.
- Tall-column layout is a minimal readable prototype, not final visual polish.
- Court art remains temporary; saves, audio and physical device support are absent.

## One next action
Review the M0.1C PR, desktop behavior and acceptance evidence; squash merge only
when approved. Do not begin M0.1D or device packaging without a separate request.
