# Cardgame-RX roadmap

M0.1A static desktop foundation is implemented and locally verified on its task
branch, pending PR review and squash merge. No milestone is device-verified.

- **M0 — Foundation (in progress; M0.1A desktop slice implemented):** inspect shared devkit, specify engine contracts, controls and platform boundaries.
- **M1 — Card primitives:** cards, decks, deterministic shuffle/deal, piles/zones, actions, legal move validation and tests.
- **M2 — UI:** card rendering, D-pad focus/navigation, selection, animation hooks and legibility on 640x480.
- **M3 — Klondike:** complete playable rules, win/loss conditions and device acceptance tests.
- **M4 — Solitaire polish:** save/resume, undo, statistics, accessibility and optional variants.
- **M5 — Multiplayer abstraction:** seats, hidden information, turn flow, local players, transport boundary and reproducible game logs.
- **M6 — 500:** selectable rules variant, bidding, partnerships, tricks, scoring and full game tests.
- **M7 — Expansion:** computer opponents, network play where appropriate, additional card games.

Do not treat milestones as permission to implement unresolved design decisions. Break each milestone into small reviewable tasks with explicit tests.
