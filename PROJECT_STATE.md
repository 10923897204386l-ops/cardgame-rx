# Cardgame-RX project state

Updated: 2026-11-19.

## Verified
- M0.1A squash-merged via PR #1 at `97e82e0`; main is the authoritative baseline.
- M0.1B pure rules implemented on `feat/m0-1b-klondike-rules`, pending PR review
  and squash merge. No direct changes pushed to main.
- C17/SDL2 desktop build: 3/3 CTest suites pass, including M0.1A regression tests.
- Rules-only Release build passes 2/2 suites without SDL discovery or linkage.
- 1,000 deterministic deals plus 32,000 legal actions validate card conservation,
  replay and full undo. See `tasks/M0.1B-REPORT.md` for exact coverage and limits.
- M0.1A preview still launches; both 640×480 captures match its baseline pixels.
- No physical RG35XXSP build or device test has been performed.

## Goal
Reusable card-game engine for RG35XXSP; Klondike first, 500 later. Separate
pure model/rules from presentation, input and platform services.

## Current milestone
M0.1B implemented and locally verified, awaiting PR review and squash merge.
Pure Klondike rules exist; the desktop preview remains static, with no gameplay UI.

## Blockers/open decisions
- No blocker to scoped M0.1B acceptance.
- Controller integration, carry/selection UI and final overflow policy remain
  future work. The remaining interaction questions are in `tasks/M0-CONTROLS.md`.
- Final court art and native handheld legibility/device packaging remain unverified.

## One next action
Review the M0.1B PR and acceptance evidence before squash merge, then bound the
next interaction/UI milestone using the pure legality and transaction APIs.
