#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
DIST_DIR="${DIST_DIR:-$ROOT_DIR/dist}"
APP_DIR="$DIST_DIR/BARKPlayer"

rm -rf "$APP_DIR"
mkdir -p "$APP_DIR"

if [ -f "$BUILD_DIR/barkplayer" ]; then
    cp "$BUILD_DIR/barkplayer" "$APP_DIR/barkplayer"
elif [ -f "$ROOT_DIR/barkplayer" ]; then
    cp "$ROOT_DIR/barkplayer" "$APP_DIR/barkplayer"
else
    echo "No compiled barkplayer binary found; creating source-only package." >&2
fi

cp "$ROOT_DIR/start_bark.sh" "$APP_DIR/start_bark.sh"
chmod +x "$APP_DIR/start_bark.sh" 2>/dev/null || true

for path in assets extensions libs_hf; do
    if [ -e "$ROOT_DIR/$path" ]; then
        cp -R "$ROOT_DIR/$path" "$APP_DIR/$path"
    fi
done

cat > "$APP_DIR/README-FIRST.txt" <<'EOF'
BARKPlayer Kindle package

Copy this BARKPlayer folder to your Kindle root/userstore.
Launch with ./start_bark.sh from kterm/KUAL.

If the package is source-only, the GitHub runner did not have the Kindle ARMHF toolchain and private Kindle/FAAD/LIPC dependencies available.
EOF

mkdir -p "$DIST_DIR"
(
    cd "$DIST_DIR"
    zip -r BARKPlayer.zip BARKPlayer >/dev/null
)

echo "$DIST_DIR/BARKPlayer.zip"
