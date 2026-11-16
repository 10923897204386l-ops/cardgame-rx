# M0 — Foundation (planning brief)

## Objective
Establish an engine architecture and developer workflow compatible with shared RG35XXSP infrastructure, without prematurely implementing Klondike or 500.

## Inputs to inspect
- rg35xx-devkit desktop simulation, SDL/input abstraction, build and deployment contracts.
- rg35xxsp-devel device operations and stock-modified firmware constraints.
- Existing go-rx and dungeon-rx AGENTS.md / state / decisions conventions.

## Design deliverables
1. Define portable card, deck, zone, seat, visibility, action and game-state interfaces.
2. Specify deterministic state transitions and seeded randomness for test/replay.
3. Establish rules-module interface so Klondike and 500 do not depend on each other.
4. Specify RG35XXSP D-pad/button interaction for selecting and moving cards, with keyboard emulation.
5. Define boundaries for local play, AI and future multiplayer; avoid network implementation in M0.
6. Identify implementation stack only after devkit inspection.

## Acceptance
- Architecture proposal reviewed in ChatGPT.
- Bounded M0.1 Codex task with explicit files, tests and non-goals.
- No unverified claim of working engine or hardware build.
