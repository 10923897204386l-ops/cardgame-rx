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
- M0.1D is merged as PR #5 (`02c61ff`): locked title art,
  Kennard card/felt assets, lighter green felt with sparse peripheral petals,
  Go-RX-style font/menu panels, main menu, expanded pause menu and Select-toggled
  controls bar with L2+Select undo. Acceptance screenshots are in
  `docs/screenshots/m01d/`.
- M0.1E is implemented on `feat/m0-1e-desktop-polish`: controls bar now reflects
  active key bindings, desktop/rules-only/clean-clone validation passes, and
  release-candidate screenshots are in `docs/screenshots/m01e/`.
- The approved Permanent Marker interface font replacement is implemented on the
  same M0.1E branch: genuine Google Fonts / Font Diner TTF, Apache-2.0 licence
  and metadata are bundled, Go RX Ink is removed, and evidence screenshots are in
  `docs/screenshots/m01e-permanent-marker/`.
- M0.1F is implemented on `feat/m0-1f-card-table-art` using the approved Drive
  package (`croupier_m01f_approved_assets.zip`): unchanged felt, warm-ivory
  Kennard faces, tropical blue/gold back, and runtime card shadows. Before/after
  screenshots are in `docs/screenshots/m01f/`.
- No RG35XXSP hardware testing, device mapping or packaging has been performed.

## Goal
Reusable card-game engine for RG35XXSP; Klondike first, 500 later. Keep rules,
input, transient interaction and rendering separate.

## Current milestone
M0.1G animation/audio polish is implemented on
`feat/m0-1g-animation-audio` (from the M0.1F art branch). See
`tasks/M0.1G-REPORT.md`. Music is intentionally not committed pending an approved
licensed shortlist. M0.1F PR #7 remains open separately.

## Known limits
- Tall-column layout is a readable prototype, not final device-polished scrolling.
- Desktop SFX exist; music and physical device support are absent. Saves absent.

## One next action
Review M0.1G screenshots/SFX and decide music shortlist approval. Keep M0.1F and
M0.1G PRs unmerged until accepted. Do not begin device packaging without approval.
