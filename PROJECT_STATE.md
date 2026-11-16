# Cardgame-RX project state

Updated: 2026-11-16.

## Verified
- Repository created by user; initial shared agent workflow documentation being established.
- No game implementation, local build or device tests have been verified.

## Goal
Reusable card-game engine for RG35XXSP, supporting single-player games such as Klondike and later multiplayer games such as 500.

## Proposed architecture (not yet locked)
- Separate deterministic game state and rules, card/deck/pile primitives, player abstraction, UI/rendering, input mapping, persistence, AI and transport.
- Avoid coupling multiplayer transport or game-specific rules to the base engine.
- Use shared Fedora/RG35XXSP devkit for desktop/device tooling, subject to inspection.

## Current milestone
M0 — requirements, devkit interface inspection, engine boundaries and input model.

## Blockers/open decisions
- Inspect shared devkit APIs and select implementation language/rendering path.
- Decide Klondike interaction model, rules variant and undo policy.
- Decide whether initial multiplayer is local pass-and-play, networked or both.

## One next action
Inspect rg35xx-devkit and prepare a bounded M0.1 implementation specification without building the complete engine prematurely.
