# Croupier-RX complete card-front deck v1.1 (25% larger top-left indices)

52 distinct PNGs in `faces_72x100/` and `faces_144x200/`, plus contact sheet, user reference, original illustrated court portrait sheet and deterministic generation script.

Top-left rank and suit rendered at approximately 125% of v1 nominal font sizes; bottom-right index and all main pips remain unchanged.

Style: warm ivory faces, rounded charcoal border, bold traditional red/black pips and indices, illustrated single-upright court figures based on the user reference. Ranks A,2..9,T(10),J,Q,K; suits S,H,D,C. All pips generated deterministically; no AI-generated pip counts.

Court illustrations are newly generated artwork, not original Kennard. They share one illustration per rank (J/Q/K) across four suits; this is a **style prototype**, not a historically accurate four-suit court deck. Please review court styling and 72x100 legibility before integrating. The blue-and-gold back and approved felt are separate assets from the previous M0.1F package.

These sprites have transparent corners and no baked shadow; Warp should add a soft contact shadow at render time. Use `faces_72x100` for native gameplay and `faces_144x200` for higher-resolution display. Preserve original deck as a selectable fallback.
