# Cardgame-RX visual asset pack (pre-integration)

Native target: 640 × 480. Current card size: 72 × 100.

## Ready assets
- `backgrounds/felt_640x480.png`: original graduated-lighting green felt texture.
- `backs/back_{navy,wine,forest}_72x100.png`: three optional card backs.
- `overlays/{invalid_red,focus_gold,held_cyan}_72x100.png`: transparent border overlays. Invalid drop should show red for ~250–300 ms without altering game state.

## Kennard face artwork: download required
The original Kennard deck is **not included** in this archive: direct download was not available in the preparation environment. Do not mistake the placeholder-free pack for a complete 52-face deck.

1. Visit https://www.me.uk/cards/ and choose **Standard deck**.
2. Download the SVG ZIP from the deck customiser.
3. Extract the SVGs to `kennard-source/`.
4. Inspect the source filenames and use `tools/rasterize_kennard.py` to render them at 72×100 (and optionally 144×200) to `faces/`. The script preserves original filenames; rename/manifest mapping should be completed after inspecting the chosen download.
5. At 640×480 inspect overlap readability, indices and court cards. The renderer can retain its own adjacent rank/suit indexes and custom pips if that improves legibility.

Source/rights: Adrian Kennard, https://www.me.uk/cards/ — site declares CC0 / public domain. Court designs derived from nineteenth-century Goodall & Son. Verify exact downloaded source and keep its license statement with bundled SVGs. No attribution legally required; source credit recommended.

## Integration
Keep backgrounds precomputed and load once. Composite transparent outlines at draw time. Do not change Klondike rules or M0.1C interaction semantics. Source files and raster conversions should be versioned with a reproducible manifest before final deployment.


## Completed Kennard source artwork (2026-11-19)
User-supplied `poker-qr.zip` is preserved as 56 original SVGs under `kennard-source/`: 52 standard faces and four alternate back/joker SVGs (`1B`, `2B`, `1J`, `2J`).
The 52 face PNGs are available in `faces_72x100/` and `faces_144x200/` (2x rendering for sampling); `contact_sheet.png` provides a visual inventory. All conversions were generated using CairoSVG with a 72:100 aspect ratio matching the game assets. The originals have a 240:336 SVG viewBox, i.e. 5:7, so final raster sizes should be checked for any minor scaling distortion. Preserve originals for regeneration.
Source: Adrian Kennard, https://www.me.uk/cards/ ; CC0 as represented by original source. Verify the specific downloaded generator output and retain provenance in the repository.
