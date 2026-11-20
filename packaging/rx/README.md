# Croupier-RX native StockOS packaging

Patterned on go-rx `packaging/rx`:

1. Copy sources to the device.
2. Run `packaging/rx/build-cardgame.sh` **on the handheld** with its gcc/SDL2.
3. Stage `releases/<git-sha>/{croupier-rx,assets/}` and point `croupier-rx/current` at that sha.
4. Install `Croupier-RX.sh` into `Roms/PORTS/`.

Launch through StockOS Ports. Previous release remains for rollback by rewriting `current`.
