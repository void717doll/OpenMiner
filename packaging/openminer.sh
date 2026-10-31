#!/bin/sh
set -eu
prefix=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
assets="$prefix/share/openminer"
binary="$prefix/bin/openminer-bin"
data="${OPENMINER_DATA_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/openminer}"
mkdir -p "$data/config" "$data/saves" "$data/logs"
for directory in mods resources; do
    if [ ! -e "$data/$directory" ] || [ -L "$data/$directory" ]; then
        ln -sfn "$assets/$directory" "$data/$directory"
    fi
done
cd "$data"
exec "$binary" "$@"
