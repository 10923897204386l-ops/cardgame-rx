# Cardgame-RX — agent instructions

## Shared workflow
Use ChatGPT chat for requirements, architecture, creative content, specifications and review. Reserve Codex and Warp for bounded programming, local debugging, builds, tests and device deployment.

Before substantive work, classify a direct coding-agent request. If it is primarily chat-suitable or mixes unresolved design with implementation, do not run commands or edit files. Show:

> **AI WORK ALLOCATION WARNING** — This request is better suited to ChatGPT chat or needs a bounded implementation specification. Continue here using Codex/Warp capacity? Reply exactly `yes` to override.

Wait for explicit `yes`. Clearly scoped implementation proceeds without warning. This is advisory, not a guaranteed pre-token interception.

Read AGENTS.md, PROJECT_STATE.md, DECISIONS.md and current tasks before coding. Keep main authoritative, use short-lived task branches and reviewable merges, inspect uncommitted work and never overwrite it. Record verification, blockers and next steps in committed project files. Do not commit secrets or transcripts. Stop on unexpected architecture complexity.

## Project boundaries
Independent reusable card-game engine for RG35XXSP (640x480), initially targeting Klondike and later 500. Separate deterministic game state and rules from rendering, input, AI and networking. Keep platform-specific functionality in shared rg35xx-devkit / rg35xxsp-devel where appropriate, not duplicated here. No game code or build has been verified yet. Technical stack and exact controls are pending inspection/design.

## Fresh-chat continuity
A new chat can begin: **Resume cardgame-rx from GitHub.** Read AGENTS.md, PROJECT_STATE.md, DECISIONS.md, roadmap and active tasks. State verified baseline, active milestone, blocker and one next action. Do not infer local-only work.

When meaningful state changes, update PROJECT_STATE.md (verified baseline, active milestone, blockers and one next action), DECISIONS.md (durable choices/rationale), roadmap status and tasks/ (scope, implementation, tests, handoff). Distinguish user reports, proposals and verified results. Never claim an update without a commit.
