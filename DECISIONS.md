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
