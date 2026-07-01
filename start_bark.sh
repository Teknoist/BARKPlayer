#!/bin/sh
set -eu

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$APP_DIR"

if [ -d "$APP_DIR/libs_hf" ]; then
    export LD_LIBRARY_PATH="$APP_DIR/libs_hf${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

exec "$APP_DIR/barkplayer" "$@"
