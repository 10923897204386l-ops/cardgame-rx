# Cardgame-RX roadmap

M0.1A, M0.1B and M0.1C are merged (#1/#2/#3). M0.1C post-merge manual review
requested immediate navigation and visual corrections before M0.1D starts. No
milestone is device-verified.

- **M0 — Foundation (in progress; M0.1A/B/C merged, M0.1C corrective review underway):** inspect shared devkit, specify engine contracts, controls and platform boundaries.
- **M1 — Card primitives (base and Klondike rules locally tested):** cards, decks, deterministic shuffle/deal, piles/zones, actions, legal move validation and tests.
- **M2 — UI (desktop interaction slice implemented):** card rendering, D-pad focus/navigation, selection, animation hooks and legibility on 640x480.
- **M3 — Klondike (rules and desktop play locally tested; corrective review underway):** complete playable rules, win/loss conditions and device acceptance tests.
- **M4 — Solitaire polish:** save/resume, undo, statistics, accessibility and optional variants.
- **M5 — Multiplayer abstraction:** seats, hidden information, turn flow, local players, transport boundary and reproducible game logs.
- **M6 — 500:** selectable rules variant, bidding, partnerships, tricks, scoring and full game tests.
- **M7 — Expansion:** computer opponents, network play where appropriate, additional card games.

Do not treat milestones as permission to implement unresolved design decisions. Break each milestone into small reviewable tasks with explicit tests.
