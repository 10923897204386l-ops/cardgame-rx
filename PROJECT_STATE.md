# Cardgame-RX project state

Updated: 2026-11-19.

## Verified
- M0.1A implementation on `feat/m0-1a-card-foundation`; main remains authoritative
  until PR review and squash merge.
- C17/SDL2 desktop preview built on Fedora 43 Toolbox (GCC 15.3.1, SDL 2.32.74).
- Normal and stress scenes captured at exactly 640×480 and visually inspected.
- Desktop window launch at 1× and 2× verified using one-frame smoke runs.
- Two CTest suites pass without a display: deterministic cards/piles/layout and
  offscreen rendering. See `tasks/M0.1A-REPORT.md` for evidence and limits.
- No physical RG35XXSP build or device test has been performed.

## Goal
Reusable card-game engine for RG35XXSP; Klondike first, 500 later. Separate
pure model/rules from presentation, input and platform services.

## Current milestone
M0.1A implemented and locally verified; awaiting PR review and squash merge.
This is a static preview, not a working game.

## Blockers/open decisions
- Final court artwork, overflow policy and device legibility remain future work.
- Controller behavior, Klondike variant, undo and multiplayer design remain
  outside this implementation.
- Optional sanitizer validation unavailable: container lacks libasan/libubsan.

## One next action
Review the M0.1A PR and evidence against the task criteria before squash merge;
then bound M0.1B around rules and interaction decisions in the handoff report.
