# Cardgame-RX roadmap

M0.1A, M0.1B, M0.1C and M0.1D are merged (#1/#2/#3/#5), with corrective PR #4 on main.
M0.1E desktop polish and validation is implemented on a feature branch awaiting
review. No milestone is device-verified.

- **M0 — Foundation (in progress; M0.1A/B/C/D merged, M0.1E desktop polish under review):** inspect shared devkit, specify engine contracts, controls and platform boundaries.
- **M1 — Card primitives (base and Klondike rules locally tested):** cards, decks, deterministic shuffle/deal, piles/zones, actions, legal move validation and tests.
- **M2 — UI (desktop interaction and Croupier-RX visual identity implemented; M0.1E polish under review):** card rendering, D-pad focus/navigation, selection, animation hooks and legibility on 640x480.
- **M3 — Klondike (rules and desktop play locally tested; M0.1E review pending):** complete playable rules, win/loss conditions and device acceptance tests.
- **M4 — Solitaire polish:** save/resume, undo, statistics, accessibility and optional variants.
- **M5 — Multiplayer abstraction:** seats, hidden information, turn flow, local players, transport boundary and reproducible game logs.
- **M6 — 500:** selectable rules variant, bidding, partnerships, tricks, scoring and full game tests.
- **M7 — Expansion:** computer opponents, network play where appropriate, additional card games.

Do not treat milestones as permission to implement unresolved design decisions. Break each milestone into small reviewable tasks with explicit tests.
