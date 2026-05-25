#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_DIR="$(cd "$CLIENT_DIR/../../.." && pwd)"
BUILD_DIR="${1:-$REPO_DIR/build/axmol_client}"
APP="$BUILD_DIR/bin/PathfinderAxmolClient"

if [[ "${PATHFINDER_AXMOL_SKIP_BUILD:-0}" != "1" ]]; then
  "$SCRIPT_DIR/build.sh" "$BUILD_DIR" >/dev/null
elif [[ ! -x "$APP" ]]; then
  "$SCRIPT_DIR/build.sh" "$BUILD_DIR" >/dev/null
fi

cd "$BUILD_DIR/bin"
exec ./PathfinderAxmolClient
