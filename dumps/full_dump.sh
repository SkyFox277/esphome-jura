#!/usr/bin/env bash
# full_dump.sh — capture comprehensive Jura state via REST debug API.
# Usage: dumps/full_dump.sh <label>
# Env:   JURA_HOST=host:port (default jura-coffee-f50.local:8080)
# Output: dumps/YYYY-MM-DD_HHMM_<label>.log
set -euo pipefail

HOST="${JURA_HOST:-jura-coffee-f50.local:8080}"
LABEL="${1:-dump}"
TS="$(date +%Y-%m-%d_%H%M)"
OUT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT="${OUT_DIR}/${TS}_${LABEL}.log"

exec_cmd() {
  local cmd="$1"
  curl -sS --max-time 5 "http://${HOST}/jura/cmd?q=${cmd}" >/dev/null
  for _ in $(seq 1 25); do
    sleep 0.8
    local r
    r=$(curl -sS --max-time 5 "http://${HOST}/jura/result" || true)
    if printf '%s' "$r" | grep -q '"status":"done"'; then
      printf '%s' "$r"
      return 0
    fi
  done
  printf '{"error":"timeout","cmd":"%s"}' "$cmd"
}

exec_scan() {
  local from="$1" to="$2" step="$3"
  curl -sS --max-time 5 "http://${HOST}/jura/scan?from=${from}&to=${to}&step=${step}" >/dev/null
  for _ in $(seq 1 40); do
    sleep 1
    local r
    r=$(curl -sS --max-time 5 "http://${HOST}/jura/result" || true)
    if printf '%s' "$r" | grep -q '"status":"done"'; then
      printf '%s' "$r"
      return 0
    fi
  done
  printf '{"error":"timeout","scan":"%s-%s/%s"}' "$from" "$to" "$step"
}

record() { printf '%-18s %s\n' "$1" "$2" >> "$OUT"; }

{
  printf '# Jura Full Dump — %s\n' "$LABEL"
  printf '# timestamp: %s\n' "$(date -Iseconds)"
  printf '# host:      %s\n' "$HOST"
  printf '\n'
} > "$OUT"

printf '## IC (x2 to catch bit6 oscillation)\n' >> "$OUT"
record "IC_1"  "$(exec_cmd 'IC:')"
record "IC_2"  "$(exec_cmd 'IC:')"

printf '\n## Machine type\n' >> "$OUT"
record "TY"    "$(exec_cmd 'TY:')"

printf '\n## EEPROM pages RT:0000..9000\n' >> "$OUT"
for p in 0 1 2 3 4 5 6 7 8 9; do
  record "RT:${p}000" "$(exec_cmd "RT:${p}000")"
done

printf '\n## Extended EEPROM RE:10..1F (single scan)\n' >> "$OUT"
record "RE_SCAN_10_1F" "$(exec_scan 10 1F 1)"

printf '\n## Live state registers\n' >> "$OUT"
for reg in 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 20 21 22 23; do
  record "RR:${reg}" "$(exec_cmd "RR:${reg}")"
done

printf '\nDone: %s\n' "$OUT"
printf 'Wrote: %s\n' "$OUT"
