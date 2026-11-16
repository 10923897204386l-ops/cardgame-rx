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
