#!/usr/bin/env bash
set -Eeuo pipefail

# Rebuild and restart only the Pathfinder H5 playable server on port 1999.
# Safety rule: this script must not touch frp / frpc / tunneling processes.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build/backend"
SERVER_BIN="$BUILD_DIR/pathfinder_h5_playable_server"
STATIC_ROOT="$ROOT_DIR/frontend/h5_playable"
HOST="0.0.0.0"
PORT="1999"
LOG_FILE="/tmp/pathfinder_h5_1999.log"
PID_FILE="/tmp/pathfinder_h5_1999.pid"
JOBS="${JOBS:-2}"
HEALTH_SESSION="script_health_1999_$(date +%s)"

cd "$ROOT_DIR"

echo "[1/5] Configure backend build"
cmake -S backend -B "$BUILD_DIR"

echo "[2/5] Build H5 playable server"
cmake --build "$BUILD_DIR" --target pathfinder_h5_playable_server_exe -j "$JOBS"

if [[ ! -x "$SERVER_BIN" ]]; then
  echo "ERROR: server binary not found or not executable: $SERVER_BIN" >&2
  exit 1
fi

echo "[3/5] Stop old H5 server on port 1999 only"
find_h5_1999_pids() {
  local pid comm args
  {
    ss -ltnp 2>/dev/null |
    awk '$4 ~ /:1999$/ {print}' |
      sed -n 's/.*pid=\([0-9][0-9]*\).*/\1/p'
    ps -eo pid=,comm=,args= |
      awk -v self="$$" -v bin="$SERVER_BIN" '$1 != self && ($2 ~ /^pathfinder_h5/ || $3 == bin) && /--port 1999/ {print $1}'
  } |
    sort -u |
    while read -r pid; do
      [[ -z "$pid" ]] && continue
      comm="$(ps -p "$pid" -o comm= 2>/dev/null || true)"
      args="$(ps -p "$pid" -o args= 2>/dev/null || true)"
      if [[ "$comm" == pathfinder_h5* || "$args" == *pathfinder_h5_playable_server* ]]; then
        echo "$pid"
      else
        echo "ERROR: port 1999 is used by non-H5 process pid=$pid comm=$comm" >&2
        return 2
      fi
    done
}

old_pids="$(find_h5_1999_pids)"
if [[ -n "$old_pids" ]]; then
  echo "Stopping PIDs: $(paste -sd ' ' - <<<"$old_pids")"
  while read -r pid; do
    [[ -z "$pid" ]] && continue
    kill "$pid"
  done <<<"$old_pids"

  for _ in {1..20}; do
    if [[ -z "$(find_h5_1999_pids)" ]]; then
      break
    fi
    sleep 0.2
  done

  remaining_pids="$(find_h5_1999_pids)"
  if [[ -n "$remaining_pids" ]]; then
    echo "ERROR: port 1999 is still occupied by H5 server PIDs: $(paste -sd ' ' - <<<"$remaining_pids")" >&2
    ss -ltnp 2>/dev/null | awk '$4 ~ /:1999$/ {print}' >&2 || true
    exit 1
  fi
else
  echo "No existing 1999 H5 server found"
fi

echo "[4/5] Start H5 server on 1999"
setsid "$SERVER_BIN" \
  --host "$HOST" \
  --port "$PORT" \
  --static-root "$STATIC_ROOT" \
  >"$LOG_FILE" 2>&1 < /dev/null &
new_pid="$!"
echo "$new_pid" > "$PID_FILE"
sleep 1

if ! ps -p "$new_pid" >/dev/null 2>&1; then
  echo "ERROR: H5 server exited immediately. Log:" >&2
  tail -80 "$LOG_FILE" >&2 || true
  exit 1
fi

echo "[5/5] Verify H5 server"
if command -v curl >/dev/null 2>&1; then
  curl --max-time 5 -fsS "http://127.0.0.1:${PORT}/" >/tmp/pathfinder_h5_1999_index_check.html
  curl --max-time 5 -fsS "http://127.0.0.1:${PORT}/api/playable/bootstrap" \
    -X POST \
    -d "session_id=${HEALTH_SESSION}&reset=true" \
    >/tmp/pathfinder_h5_1999_bootstrap_check.json
  python3 - <<'PY'
import json
from pathlib import Path
path = Path('/tmp/pathfinder_h5_1999_bootstrap_check.json')
data = json.loads(path.read_text())
status = data.get('execution_status') or {}
print('Health ok:', data.get('ok'))
print('Execution status visible:', status.get('visible'))
if not data.get('ok'):
    raise SystemExit('bootstrap response ok=false')
if status.get('visible') is not True:
    raise SystemExit('execution_status.visible is not true')
PY
else
  echo "curl not found; skipped HTTP verification"
fi

echo "H5 server restarted successfully"
echo "PID: $new_pid"
echo "URL: http://127.0.0.1:${PORT}/"
echo "Log: $LOG_FILE"
echo "PID file: $PID_FILE"
echo "Note: if phone still shows old UI, refresh/clear browser cache. This script does not touch frp."
