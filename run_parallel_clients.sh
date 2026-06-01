#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

APP="./build/terrain_app"
CONFIG="${TERRAIN_CONFIG:-files/seminar_config.txt}"
SOCKET="${TERRAIN_SOCKET:-/tmp/terrain_engine_parallel.sock}"
CLIENT_LOG_DIR="logs/parallel_clients"

if [[ ! -x "$APP" ]]; then
    echo "[INFO] terrain_app not found, building first..."
    cmake -S . -B build
    cmake --build build
fi

mkdir -p "$CLIENT_LOG_DIR"
rm -f "$CLIENT_LOG_DIR"/seminar*.log
rm -f "$SOCKET"

shutdown_server() {
    if [[ -n "${SERVER_PID:-}" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        local empty_commands
        empty_commands="$(mktemp)"
        "$APP" --client "$empty_commands" --config "$CONFIG" --pipe "$SOCKET" --shutdown >/dev/null 2>&1 || true
        rm -f "$empty_commands"
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    rm -f "$SOCKET"
}

trap shutdown_server EXIT

echo "[INFO] Starting one server on Unix socket: $SOCKET"
"$APP" --server --config "$CONFIG" --pipe "$SOCKET" &
SERVER_PID=$!

for _ in {1..40}; do
    if [[ -S "$SOCKET" ]]; then
        break
    fi
    sleep 0.25
done

if [[ ! -S "$SOCKET" ]]; then
    echo "[ERROR] Server did not create socket: $SOCKET"
    exit 1
fi

echo "[INFO] Starting seminar clients in parallel..."
"$APP" --client files/seminar1_commands.txt --config "$CONFIG" --pipe "$SOCKET" >"$CLIENT_LOG_DIR/seminar1.log" 2>&1 &
PID1=$!
"$APP" --client files/seminar2_commands.txt --config "$CONFIG" --pipe "$SOCKET" >"$CLIENT_LOG_DIR/seminar2.log" 2>&1 &
PID2=$!
"$APP" --client files/seminar3_commands.txt --config "$CONFIG" --pipe "$SOCKET" >"$CLIENT_LOG_DIR/seminar3.log" 2>&1 &
PID3=$!

FAILED=0
for pid in "$PID1" "$PID2" "$PID3"; do
    if ! wait "$pid"; then
        FAILED=1
    fi
done

if grep -q "ERROR" "$CLIENT_LOG_DIR"/seminar*.log; then
    FAILED=1
fi

if [[ "$FAILED" -ne 0 ]]; then
    echo "[ERROR] At least one client failed. Logs:"
    tail -n +1 "$CLIENT_LOG_DIR"/seminar*.log
    exit 1
fi

echo "[OK] All clients completed."
echo "[INFO] Client logs:"
echo "  $CLIENT_LOG_DIR/seminar1.log"
echo "  $CLIENT_LOG_DIR/seminar2.log"
echo "  $CLIENT_LOG_DIR/seminar3.log"
