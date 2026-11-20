# Card fronts v1.1 (25% larger top-left indices)

## Package
- Drive: `cardgame-rx/assets/croupier_card_fronts_v1_25pct.zip`
- ZIP SHA-256: `7c057287a18c276ac2e99642a80085d8e0b9d71452eb80b5e4267ed9c34caf50`
- Contact sheet: `croupier_card_fronts_v1_25pct_contact_sheet.png`

## Install
- Source: `assets/source/card_fronts_v1_25pct/` (72×100 + 144×200 PNGs, README, SHA256.json)
- Runtime: `assets/runtime/faces_72x100/*.bmp` converted losslessly from package 72×100 PNGs (transparent corners → black for existing color-key)
- Fallback previous faces: `assets/runtime/faces_72x100_prev/`
- Backs, felt, title, font, controls, audio unchanged

## Verification
- All 52 filenames A,2–9,T,J,Q,K × C,D,H,S present
- Package `SHA256.json` matched for all 52 `faces_72x100/*.png`
- Desktop CTest 6/6
- Screenshots: `docs/screenshots/card-fronts-v1-25pct/`
  - long-tableau, tall-held-stack, foundations-courts, tableau-gameplay, contact sheet
