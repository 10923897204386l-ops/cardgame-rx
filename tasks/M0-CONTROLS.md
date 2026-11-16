# M0 — Handheld control contract (user specified, 2026-11-16)

## Shared input model
- D-pad Left/Right: navigate piles in current area.
- D-pad Up/Down: navigate cards within a stack. Other games may use vertical navigation for selecting cards to play/discard.
- L1/R1: cycle table areas.
- L2/R2: jump to leftmost/rightmost pile of current area, including while carrying cards.
- B: pick up selected card/stack; press again to attempt placement.
- X: auto-move eligible card to foundation (Klondike).
- Y: flip/deal from draw pile (Klondike).
- A: bring top waste card to current cursor position and select it for movement; B attempts to place it (Klondike).
- Select: undo.
- Start: pause menu.

## Klondike areas
- Draw area (stock and waste), tableau (seven columns), foundations (four piles).
- Cursor, selection and held/pending move are distinct engine/UI concepts. A waste-to-tableau move is pending until B confirms, not a premature state mutation.
- D-pad navigation and L2/R2 must work while carrying a card or stack.
- Game modules map generic inputs to game-specific actions; do not hardcode Klondike behavior in shared input services.

## Design questions to resolve before implementation
- Exact L1/R1 area cycling order and entry focus (including draw area stock/waste).
- Up/down card targeting for partly face-down tableau columns and selecting a valid movable suffix.
- B behavior on invalid destinations, cancellation and return-to-source.
- A behavior if cursor is over a foundation or draw area, if waste is empty, or while already holding cards.
- Whether X can be used from waste, tableau, held cards, and which auto-move safety rules apply.
- Y behavior for draw-one/draw-three and recycling stock.
- Undo granularity, including selection-only changes versus completed state transitions.
- Device-specific key names and physical mappings verified in devkit.

## Acceptance
- Document and test focus navigation, card selection, carry/placement, auto-foundation, draw/waste interaction and undo.
- Keep rules transitions deterministic and independent of display or hardware input.

## Confirmed B pickup and destination-aware placement (2026-11-16)
- B picks up the maximum legal movable face-up run starting at the cursor-selected card; with default cursor at bottom of a tableau run, this is the entire valid face-up run.
- The player may move the cursor upward before pickup to explicitly choose the highest card to carry, restricting the candidate sequence.
- On B drop, first attempt the complete held run; if illegal, try progressively shorter suffixes (drop the leading/top card of the proposed moved run each time), selecting the longest suffix legal at the destination.
- Cards excluded from the chosen suffix remain at the source in their original order; commit only the successful transfer as one atomic, undoable game action.
- If no candidate suffix is legal, preserve held state and allow another destination.
- This is a Klondike-specific placement policy exposed through a general game-rule interface, not universal behaviour for all card games.
- Test examples: held 9♠ 8♥ 7♣ 6♦, destination 9? full sequence only when legal; destination requiring 8♥ moves 8♥ 7♣ 6♦; destination requiring 7♣ moves 7♣ 6♦.

## Confirmed visual pickup versus atomic transaction (2026-11-16)
- While B-held, render the **entire** candidate stack as moving with the cursor, even if the eventual legal transfer is only a suffix.
- The authoritative game state is unchanged during pickup, navigation and preview. Keep a distinct transient interaction state with source, cursor, held cards and candidate destination.
- On drop, evaluate full run then progressively shorter suffixes and choose the longest legal transfer. Commit source/destination changes together as one atomic, undoable action only after validation.
- Render the successful suffix settling at destination; non-transferred cards remain at source. If no suffix is legal, continue displaying the full held stack and do not mutate the game.
- Initial renderer may snap instantly. Future animations must not affect deterministic rules or transaction semantics.
