#!/usr/bin/env bash
# monitor_cleaning.sh — low-frequency state monitor during cleaning cycle.
# Polls IC:, RT:0000, RR:10 every 60s; appends timestamped line per poll.
# Stop: touch dumps/STOP_MONITOR  OR  kill the process.
set -euo pipefail

HOST="${JURA_HOST:-jura-coffee-f50.local:8080}"
INTERVAL="${INTERVAL:-60}"
OUT_DIR="$(cd "$(dirname "$0")" && pwd)"
TS="$(date +%Y-%m-%d_%H%M)"
OUT="${OUT_DIR}/${TS}_during_cleaning.log"
STOP_FILE="${OUT_DIR}/STOP_MONITOR"

exec_cmd() {
  local cmd="$1"
  curl -sS --max-time 5 "http://${HOST}/jura/cmd?q=${cmd}" >/dev/null || return 1
  for _ in $(seq 1 20); do
    sleep 0.5
    local r
    r=$(curl -sS --max-time 5 "http://${HOST}/jura/result" || true)
    if printf '%s' "$r" | grep -q '"status":"done"'; then
      # Extract response value
      printf '%s' "$r" | python3 -c "import json,sys; print(json.load(sys.stdin).get('response',''))" 2>/dev/null
      return 0
    fi
  done
  printf 'TIMEOUT'
}

{
  printf '# Monitor start: %s\n' "$(date -Iseconds)"
  printf '# Host: %s, Interval: %ss\n' "$HOST" "$INTERVAL"
  printf '# Stop: touch %s\n' "$STOP_FILE"
  printf '\n'
  printf '%-25s %-12s %-68s %-8s\n' "timestamp" "IC" "RT:0000" "RR:10"
} > "$OUT"

rm -f "$STOP_FILE"

while [ ! -f "$STOP_FILE" ]; do
  ts="$(date -Iseconds)"
  ic=$(exec_cmd 'IC:')
  rt=$(exec_cmd 'RT:0000')
  rr=$(exec_cmd 'RR:10')
  printf '%-25s %-12s %-68s %-8s\n' "$ts" "$ic" "$rt" "$rr" >> "$OUT"
  # Sleep in small chunks so STOP file is noticed quickly
  for _ in $(seq 1 $((INTERVAL / 2))); do
    [ -f "$STOP_FILE" ] && break
    sleep 2
  done
done

{
  printf '\n# Monitor stop: %s\n' "$(date -Iseconds)"
} >> "$OUT"

printf 'Monitor stopped. Log: %s\n' "$OUT"
