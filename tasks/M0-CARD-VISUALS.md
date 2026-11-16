# M0 — Card visual design brief

Status: proposed visual direction, awaiting user approval and handheld testing.

## Direction
- Hybrid traditional playing-card appearance and crisp retro handheld readability, referencing Game Gear Solitaire Poker as a stylistic influence rather than copying artwork.
- Card fronts prioritise legibility when overlapped: bold rank and suit together in the top-left exposed strip; conventional red/black suits; recognisable but simplified court illustrations.
- Proposed initial card size approximately 72x100 pixels; exposed tableau index strip 22–26 pixels. Test against 640x480 RG35XXSP layout and adjust before locking.
- Use crisp pixel-aligned artwork/text and high-contrast outlines; avoid scaling that blurs symbols.
- Card backs must be theme-selectable, with bundled patterns and potential user-supplied custom images, cropped/scaled to a fixed card aspect ratio.
- Rendering assets and theme choice are separate from rules/state, reusable across Klondike, 500 and future modules.
- The entire selected run renders as a held visual overlay, without mutating game state until a valid drop.

## Acceptance ideas
- Test all ranks and suits at native 640x480, including 10 and court cards in a 22–26 px exposed strip.
- Test seven tableau columns, foundation and stock/waste arrangement without illegible compression.
- Test red/black contrast, selected/held states, face-down pattern distinction, and minimum readable scaling.
- Verify a card-back theme switch does not change gameplay or saved game rules.

## Open decisions
- Final card size, suit glyph design, court art style and optional accessibility/large-index mode.
- Card-back custom image import workflow and on-device constraints.

## 2026-11-16 — User-approved visual refinements
- Preferred mockup: conventional playing-card pip counts on visible full faces (e.g. seven spade pips on 7♠), with traditional court-card illustrations; retain crisp, high-contrast retro-handheld rendering.
- **Place the suit glyph immediately beside the rank** in the upper-left index, with minimal horizontal gap. This must visually group rank and suit into a single index so the index is not mistaken for the central pip field, especially when cards are overlapped.
- Show face-up tableau stacks with every exposed card index readable; maintain sufficient overlap strip height for rank plus suit.
- Keep only the fixed top status bar; remove the bottom controls/help bar from normal gameplay.
- Card-back artwork remains theme-customisable.
- Latest image mockup is an illustrative visual reference from chat, **not** a pixel-accurate, game-ready 640x480 asset. Implement a native-resolution mockup and verify legibility on device before locking exact measurements.
- If source mockup assets are not present in repository, do not claim they were committed; recreate from these specifications.
