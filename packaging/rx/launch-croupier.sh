#!/bin/sh
# StockOS Ports launcher for Croupier-RX. Mirrors go-rx launch-go.sh structure.
set -eu
ports=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
game=$ports/croupier-rx
mkdir -p "$game"
exec > "$game/last-launch.log" 2>&1
printf 'Croupier-RX launch: '
date
if [ ! -f "$game/current" ]; then
    echo "Missing $game/current"
    exit 1
fi
IFS= read -r release < "$game/current"
case "$release" in
    ''|*[!0-9a-f]*) echo "Invalid release ID: $release"; exit 1 ;;
esac
cd "$game/releases/$release"
printf 'Release: %s\n' "$release"
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-mali}"
# Keep audio on the default device path when available.
volume_pid=
if [ -x /usr/bin/volumeCtrl.dge ] && ! pidof volumeCtrl.dge >/dev/null 2>&1; then
    /usr/bin/volumeCtrl.dge >/dev/null 2>&1 &
    volume_pid=$!
fi
cleanup_volume() {
    if [ -n "$volume_pid" ]; then
        kill "$volume_pid" 2>/dev/null || :
        wait "$volume_pid" 2>/dev/null || :
    fi
}
trap cleanup_volume EXIT
status=0
./croupier-rx "$@" || status=$?
printf 'Exit status: %s\n' "$status"
exit "$status"
