#!/bin/bash
# Replaces and restarts only the NuBits daemon that requested this update.
set -eu

ASSET_PATH="${1:-}"
PROCESS_ID="${2:-}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -z "$ASSET_PATH" || ! -f "$ASSET_PATH" || ! "$PROCESS_ID" =~ ^[0-9]+$ ]]; then
    echo "Usage: $0 <verified-release.tar.gz> <process-id>" >&2
    exit 1
fi

echo "Waiting for NuBits process $PROCESS_ID to shut down..."
for ((attempt = 0; attempt < 120; ++attempt)); do
    if ! kill -0 "$PROCESS_ID" 2>/dev/null; then
        break
    fi
    sleep 1
done

if kill -0 "$PROCESS_ID" 2>/dev/null; then
    echo "NuBits process $PROCESS_ID did not stop; update cancelled." >&2
    exit 1
fi

TEMP_DIR="$(mktemp -d "$ROOT_DIR/.nubits-update.XXXXXX")"
trap 'rm -rf "$TEMP_DIR"' EXIT

tar -xzf "$ASSET_PATH" -C "$TEMP_DIR"
if [[ ! -s "$TEMP_DIR/nud" || ! -s "$TEMP_DIR/update_posix.sh" ]]; then
    echo "Release package must contain nud and update_posix.sh." >&2
    exit 1
fi

chmod +x "$TEMP_DIR/nud" "$TEMP_DIR/update_posix.sh"
cp -f "$ROOT_DIR/nud" "$ROOT_DIR/nud.previous" 2>/dev/null || true
install -m 0755 "$TEMP_DIR/nud" "$ROOT_DIR/nud.new"
mv -f "$ROOT_DIR/nud.new" "$ROOT_DIR/nud"
install -m 0755 "$TEMP_DIR/update_posix.sh" "$ROOT_DIR/update_posix.sh.new"
mv -f "$ROOT_DIR/update_posix.sh.new" "$ROOT_DIR/update_posix.sh"

echo "Restarting updated NuBits daemon..."
nohup "$ROOT_DIR/nud" -daemon >/dev/null 2>&1 &
echo "Update installed."
