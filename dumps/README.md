# Debug Dumps

Helper scripts and session artifacts for investigating the Jura protocol on real hardware.

Dumps exercise the REST Debug API exposed by the ESPHome component when built with
`-DUSE_DEBUG_HTTP`. See the [REST Debug API](../README.md#rest-debug-api) section of
the main README for endpoint details.

## Prerequisites

- Firmware flashed with `-DUSE_DEBUG_HTTP` build flag (enabled in
  [`jura-coffee-f50.yaml`](../jura-coffee-f50.yaml))
- HTTP port 8080 reachable from your workstation
- Default host: `jura-coffee-f50.local:8080` (override via `JURA_HOST` env var)

## Scripts

### `full_dump.sh <label>`

Captures a comprehensive snapshot of machine state:

| Area                     | Commands                                  |
| ------------------------ | ----------------------------------------- |
| IC: input byte           | 2 reads (catches bit6 oscillation)        |
| Machine type             | `TY:`                                     |
| EEPROM pages             | `RT:0000` through `RT:9000` (160 words)   |
| Extended EEPROM          | `RE:10` through `RE:1F` via one scan call |
| Live state registers     | `RR:00` through `RR:23`                   |

Output: `dumps/YYYY-MM-DD_HHMM_<label>.log` (human-readable, one line per command).

```bash
dumps/full_dump.sh before_event
# ... user performs event ...
dumps/full_dump.sh after_event
```

Runtime: ~80 seconds per dump. Safe to run at any time including during brewing
(UART commands queue, machine handles concurrent IO).

### `monitor_cleaning.sh`

Low-frequency state monitor for long operations (cleaning, descaling, brewing).
Polls `IC:`, `RT:0000`, `RR:10` every 60 seconds (override via `INTERVAL`).

```bash
dumps/monitor_cleaning.sh &     # start in background
# ... user runs the operation ...
touch dumps/STOP_MONITOR        # signal graceful stop
```

Output: `dumps/YYYY-MM-DD_HHMM_during_cleaning.log` with a tabular timeline.
Cleans up `STOP_MONITOR` on next run.

## Session pattern

Successful protocol investigations follow a before → event → after → diff pattern:

1. `full_dump.sh before_<event>` to capture baseline
2. Optionally `monitor_cleaning.sh` in background if the event takes minutes
3. User performs the event (coffee, rinse, cleaning, descaling, power cycle …)
4. `touch dumps/STOP_MONITOR` if monitoring
5. `full_dump.sh after_<event>`
6. Diff both dumps — every changed EEPROM word is a candidate finding

Session-level analysis belongs in a markdown file alongside the logs:
`dumps/YYYY-MM-DD_sessionN_<topic>.md`. Session 2–4 follow this pattern.

## Python diff template

Every session uses the same parse-and-diff approach:

```python
import re

def parse_log(path):
    rt = {}
    with open(path) as f:
        for line in f:
            m = re.match(r'(RT:\w+)\s+.*"response":"(\S+?)"', line)
            if m:
                rt[m.group(1)] = m.group(2).replace('rt:', '')
    return rt

def words(hex_str):
    return [int(hex_str[i*4:(i+1)*4], 16) for i in range(16)]

before = parse_log('dumps/2026-MM-DD_HHMM_before_event.log')
after  = parse_log('dumps/2026-MM-DD_HHMM_after_event.log')

for page, offset in [('RT:0000', 0x00), ('RT:1000', 0x10)]:
    wb, wa = words(before[page]), words(after[page])
    for i, (b, a) in enumerate(zip(wb, wa)):
        if b != a:
            print(f"0x{offset+i:04X}: {b:>6} -> {a:<6}  delta={a-b:+d}")
```

## Past sessions

| Session | Date       | Topic                          | Artifacts                                  |
| ------- | ---------- | ------------------------------ | ------------------------------------------ |
| 1       | 2026-04-03 | Startup protocol observation   | `2026-04-03_session1_startup.md`           |
| 2       | 2026-04-03 | Full sensor/EEPROM sweep       | `2026-04-03_session2_analysis.md`          |
| 3       | 2026-04-03 | Water amount investigation     | `2026-04-03_session3_water_amount.md`      |
| 4       | 2026-04-18 | Cleaning cycle + per-event     | `2026-04-18_session4_cleaning.md`          |

## InfluxDB entity ids

Home Assistant slugifies ESPHome entity names with device prefix. For the F50
configured via `jura-coffee-f50.yaml`, the InfluxDB `entity_id` tag is:

```
jura_coffee_f50_jura_coffee_f50_<sensor_key>
```

Example: `jura_coffee_f50_jura_coffee_f50_cleanings` for the `num_clean` sensor.
The device name appears twice because HA prefixes with the device friendly name
and the sensor name already contains `${friendly_name}`.

## Safety

- `AN:0A` (EEPROM wipe) is blocked at the component level — all endpoints reject it
- REST API and the in-component `start_debug` action share the UART — only one at a time
- `monitor_cleaning.sh` polls at 60s intervals by default, safe during operations

## Untracked artifacts

Raw `.log` output files are listed in `.gitignore` — they are session-local evidence.
Only the session analysis markdown is committed. If you want to preserve a specific
log file for a PR or issue, attach it inline rather than committing it.
