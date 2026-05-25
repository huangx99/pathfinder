#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_DIR="$(cd "$CLIENT_DIR/../../.." && pwd)"
AXMOL_ROOT="$REPO_DIR/app/frontend/third_party/axmol"
BUILD_DIR="${1:-$REPO_DIR/build/axmol_client}"
PWSH_DIR="$AXMOL_ROOT/tools/external/powershell"

if [[ ! -x "$PWSH_DIR/pwsh" ]]; then
  echo "missing bundled pwsh: $PWSH_DIR/pwsh" >&2
  echo "run setup steps or ask the assistant to prepare Axmol dependencies" >&2
  exit 1
fi

PATH="$PWSH_DIR:$PATH" cmake \
  -S "$CLIENT_DIR" \
  -B "$BUILD_DIR" \
  -DAXMOL_ROOT="$AXMOL_ROOT" \
  -DAX_BUILD_TESTS=OFF

PATH="$PWSH_DIR:$PATH" cmake --build "$BUILD_DIR" --target PathfinderAxmolClient -j2

echo "$BUILD_DIR/bin/PathfinderAxmolClient"
