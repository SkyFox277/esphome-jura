# ESPHome Jura Coffee Machine

ESPHome external component for Jura coffee machines with Toptronic serial interface (V1 protocol).
Tested on **Jura Impressa F50** with a D1 Mini (ESP8266).

## Features

- Binary sensors: tray missing, water tank empty, need cleaning, ready (operating temperature), needs rinse
- EEPROM counters: coffees, double coffees, espressos, cleanings, coffees since cleaning
- Control buttons: power on/off, coffee, double coffee, rinse
- Raw command action: `jura_coffee.send_command` for use in automations
- Debug dump system for protocol analysis and sensor identification
- AN:0A EEPROM clear protection (command blocklist)
- Last command response text sensor
- Model-agnostic: IC: bit positions configurable for different machine variants

## Hardware

| Component       | Value                                 |
| --------------- | ------------------------------------- |
| Microcontroller | Wemos D1 Mini (ESP8266) or ESP32      |
| Interface       | Jura 7-pin service port (Toptronic)   |
| Logic level     | 3.3V ↔ 5V bidirectional level shifter |
| Baud rate       | 9600                                  |

### Wiring

```
D1 Mini           Level Shifter       Jura 7-pin
─────────────     ─────────────       ──────────
D1 (TX) ──────►  LV1 ──────► HV1 ──► Pin 4 (RxD)
D2 (RX) ◄──────  LV2 ◄────── HV2 ◄── Pin 2 (TxD)
3.3V ──────────► LV
                  HV ◄───────────────  Pin 6 (+5V)
GND ────────────► GND ◄────────────── Pin 3 (GND)
```

Jura 7-pin connector (pins numbered right to left, looking at the port):

```
(8)  7    6    5    4    3    2    1   (0)
 nc  nc  +5V   nc  RxD  GND  TxD   nc
```

> The D1 Mini can be powered directly from the Jura's +5V (Pin 6).

## Installation

### 1. Copy files

Copy the `components/` and `common/` folders into your ESPHome config directory:

```
<esphome-config>/
├── components/
│   └── jura_coffee/
│       ├── __init__.py
│       ├── jura_coffee.h
│       └── jura_coffee.cpp
└── common/
    ├── device_base.yaml    # uptime, WiFi signal, status sensors
    └── wifi.yaml           # WiFi, API, OTA, captive portal
```

> `common/` is only needed if you use `jura-coffee-f50.yaml` as a starting point.
> If you write your own YAML from scratch, only `components/jura_coffee/` is required.

### 2. Configure your device YAML

Reference the component with `external_components`:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [jura_coffee]

jura_coffee:
  id: jura
  uart_id: uart_bus
  machine_type: f50        # auto-configures IC: bit positions for this model
  update_interval: 30s

  tray_missing:
    name: "Tray Missing"
  tank_empty:
    name: "Tank Empty"
  need_clean:
    name: "Need Cleaning"
  num_coffee:
    name: "Coffees"
  num_double_coffee:
    name: "Double Coffees"
  num_clean:
    name: "Cleanings"
```

See [`jura-coffee-f50.yaml`](jura-coffee-f50.yaml) for a complete device config.

### 3. Compile and flash

```bash
esphome compile jura-coffee-f50.yaml
esphome upload --device jura-coffee-f50.local jura-coffee-f50.yaml
```

## Configuration Reference

| Key                      | Type    | Default | Description                                                              |
| ------------------------ | ------- | ------- | ------------------------------------------------------------------------ |
| `uart_id`                | id      | —       | UART bus ID                                                              |
| `update_interval`        | time    | `60s`   | How often to poll `IC:` and `RT:0000`                                    |
| `machine_type`           | string  | —       | Known model: `f50`, `e6`, `j6`, `x7` etc. — auto-sets all IC: bits      |
| `ic_tray_bit`            | int 0–7 | `4`     | IC: byte 0 bit for tray sensor (overrides `machine_type`)               |
| `ic_tank_bit`            | int 0–7 | `5`     | IC: byte 0 bit for tank sensor (overrides `machine_type`)               |
| `ic_need_clean_bit`      | int 0–7 | `0`     | IC: byte 0 bit for cleaning sensor (overrides `machine_type`)           |
| `ic_tray_inverted`       | bool    | `false` | `true` if bit=1 means tray PRESENT (F50 quirk)                          |
| `ic_tank_inverted`       | bool    | `false` | `true` if bit=1 means tank OK, 0=empty (F50 quirk)                      |
| `ic_need_clean_inverted` | bool    | `false` | `true` if bit=1 means clean NOT needed, 0=needed (F50 quirk)            |
| `ic_needs_rinse_bit`     | int 0-7 | `0`     | IC: byte 0 bit for needs_rinse sensor                                    |
| `ic_needs_rinse_inverted`| bool    | `false` | `true` if bit=1 means rinse NOT needed                                   |
| `tray_missing`           | sensor  | —       | Binary sensor: tray missing                                              |
| `tank_empty`             | sensor  | —       | Binary sensor: water tank empty                                          |
| `need_clean`             | sensor  | —       | Binary sensor: cleaning required                                         |
| `ready`                  | sensor  | —       | Binary sensor: machine at operating temperature (RR:03 bit 2)            |
| `needs_rinse`            | sensor  | —       | Binary sensor: auto-rinse pending on power off (IC: bit 0)               |
| `num_single_espresso`    | sensor  | —       | Counter: EEPROM addr 0x0000 (on F50: Coffee FA:06)                       |
| `num_double_espresso`    | sensor  | —       | Counter: EEPROM addr 0x0001 (on F50: Double Coffee FA:07)                |
| `num_coffee`             | sensor  | —       | Counter: EEPROM addr 0x0002 (on F50: always 0 — no small-size button)   |
| `num_double_coffee`      | sensor  | —       | Counter: EEPROM addr 0x0003 (on F50: always 0)                          |
| `num_clean`              | sensor  | —       | Counter: cleanings (EEPROM addr 0x0008)                                  |
| `num_rinse`              | sensor  | —       | Counter: rinse cycles (EEPROM addr 0x0007)                               |
| `num_descale`            | sensor  | —       | Counter: descaling cycles (EEPROM addr 0x0009)                           |
| `num_coffees_since_clean`| sensor  | —       | Counter: EEPROM addr 0x000E — **misnamed** (session 4): actually a cups counter with daily resets, not a since-cleaning counter. Kept for InfluxDB continuity; will be renamed in v2.0 |
| `num_brews_since_clean`    | sensor      | —       | Counter: EEPROM addr 0x000F — the real brews-since-cleaning counter, +1 per brew command, reset on cleaning (session 4)     |
| `maintenance_weeks_since_clean`| sensor  | —       | EEPROM addr 0x0005 high byte — cleaning-reset time ticker (hypothesis: ~1/week), leading Pflege-trigger candidate (session 4) |
| `maintenance_config_0x0005_low`| sensor  | —       | EEPROM addr 0x0005 low byte — constant configuration value (=20 on F50), unchanged by cleaning                              |
| `maintenance_counter_0x0011` | sensor    | —       | EEPROM addr 0x0011 — unknown cleaning-reset counter, grows ~7.6/day on F50, driver unknown (session 4)                      |
| `maintenance_counter_0x0016` | sensor    | —       | EEPROM addr 0x0016 — unknown cleaning-reset counter, grows ~2/day on F50, driver unknown (session 4)                        |
| `ic_byte0_raw`             | sensor      | —       | Numeric IC: byte 0 (0-255) — preserves all 8 bits in InfluxDB for retroactive bit derivation via HA template sensors         |
| `raw_page_rt0000`          | text_sensor | —       | Raw RT:0000 response (67-char hex string) — forensic record of the full page 0 state on every EEPROM poll                   |
| `raw_page_rt1000`          | text_sensor | —       | Raw RT:1000 response — forensic record of the full page 1 (extended EEPROM 0x0010-0x001F) on every EEPROM poll              |
| `last_response`            | text_sensor | —       | Text sensor: last `send_command` response                                                                                    |

## Sending Commands

Send any raw Jura command from automations or button presses:

```yaml
on_press:
  - jura_coffee.send_command:
      id: jura
      command: "FA:06"   # Coffee
```

Supports templates:

```yaml
command: !lambda 'return "FA:0" + std::to_string(id(select).active_index().value() + 6);'
```

## Debug Dump

The debug dump system polls IC:, RR: registers, and RT:0000 cyclically and outputs the
results to the ESPHome log. This is useful for protocol analysis and identifying which
registers change when machine state changes (e.g. tray removed, heating, brewing).

Start a dump with `jura_coffee.start_debug`:

```yaml
on_press:
  - jura_coffee.start_debug:
      id: jura
      rr_start: 0x00      # first RR: register (default: 0x00)
      rr_end: 0x23         # last RR: register (default: 0x23)
      interval_ms: 5000    # pause between cycles in ms (default: 5000)
      poll_ic: true        # include IC: in each cycle (default: true)
      poll_rt: true        # include RT:0000 in each cycle (default: true)
```

Stop a running dump:

```yaml
on_press:
  - jura_coffee.stop_debug:
      id: jura
```

Add a timestamped note to the log (e.g. to mark when you remove the tray):

```yaml
on_press:
  - jura_coffee.annotate_debug:
      id: jura
      message: "Tray removed"
```

Log output format:

```
[DUMP] t=12345 IC: ic:DFB01E00
[DUMP] t=12645 RR:00 rr:0000
[DUMP] t=12945 RR:18 rr:003C
[CMD]  t=15200 FA:06 -> ok:
[NOTE] t=15200 Machine warming up
```

> **Note:** The normal `update()` polling cycle is suspended while a debug dump is
> running. Sensor values in Home Assistant will not update until the dump is stopped.

> **Timing:** Each register poll takes ~300ms. A full RR sweep (0x00-0x23, 36 registers)
> takes ~11s. Add IC: and RT:0000 for ~12s total per cycle.

## REST Debug API

Optional HTTP API on port 8080 for rapid protocol investigation. Enabled at compile time
via build flag — disabled by default, adds ~1KB RAM overhead.

> **Debug workflow:** Helper scripts and session methodology live in
> [`dumps/README.md`](dumps/README.md) — includes `full_dump.sh`,
> `monitor_cleaning.sh`, and the before/after diff pattern used for all protocol
> investigation sessions.

### Enable

```yaml
esphome:
  platformio_options:
    build_flags:
      - "-DUSE_DEBUG_HTTP"
    build_unflags:
      - "-Werror"
```

### Endpoints

**Send a single command:**

```bash
curl "http://jura-coffee-f50.local:8080/jura/cmd?q=RE:10"
# → {"status":"pending","poll":"/jura/result"}

sleep 2
curl "http://jura-coffee-f50.local:8080/jura/result"
# → {"status":"done","cmd":"RE:10","response":"re:8747"}
```

**Scan an EEPROM range (RE: addresses):**

```bash
curl "http://jura-coffee-f50.local:8080/jura/scan?from=00&to=1F&step=01"
# → {"status":"pending","poll":"/jura/result"}

sleep 20  # ~300ms per address, 32 addresses ≈ 10s
curl "http://jura-coffee-f50.local:8080/jura/result"
# → {"scan":[{"addr":"RE:00","response":"re:110A"}, ...], "status":"done"}
```

Parameters: `from` (hex, default 00), `to` (hex, default F0), `step` (hex, default 10).
Maximum 32 addresses per scan. Capped to prevent watchdog resets.

**How it works:** HTTP handlers return `202 Accepted` immediately. UART commands execute
in the main `loop()` (one per iteration, ~300ms each). Poll `/jura/result` until
`status` is `done`. AN:0A protection applies to all endpoints.

> **Note:** The REST API and debug dump cannot run simultaneously (both use UART).
> If a dump is active, REST commands return `{"error":"debug_dump_active"}`.

## Safety

The component blocks dangerous commands that have no valid remote use case.

| Command | Effect                                               | Status                  |
| ------- | ---------------------------------------------------- | ----------------------- |
| `AN:0A` | Clears all EEPROM data (counters, settings) — irreversible | Blocked by component |

Blocked commands are rejected before reaching the UART. The component logs an error
and sets `last_response` to "BLOCKED".

## Known Working Models

| Model            | `machine_type` | Status          | Notes                                               |
| ---------------- | -------------- | --------------- | --------------------------------------------------- |
| Impressa F50     | `f50`          | ✅ tested        | IC: tray bit inverted, no espresso                  |
| Impressa F7 / F8 | `f7`           | ⚠️ community     | FA:04=Espresso, FA:06=Coffee, FA:0B=Rinse           |
| Impressa S95/S90 | `s95`          | ⚠️ community     | 4-pin connector, same FA: layout as F7              |
| Impressa J6      | `j6`           | ⚠️ community     | FA:07=Espresso, FA:09=Coffee, FA:0C=Menu/Rinse      |
| E6 / E8 / E65    | `e6`           | ⚠️ community     | FA:04=Espresso, FA:09=Coffee, IC: bit0/bit1         |
| ENA 5/7/Micro 90 | `ena`          | ⚠️ community     | FA:09/0A=Coffee, sleep quirk on Micro 90            |
| X7 / Saphira     | `x7`           | ⚠️ community     | 9-pin connector, FA:01–07=Products, dual grinder    |

> ⚠️ community = commands confirmed from reverse-engineering projects, not tested personally.
> Newer models (Z10, connected via Bluetooth/WiFi) use encrypted protocol — NOT supported.

## Protocol

The Jura Toptronic protocol encodes each ASCII character as 4 UART bytes using bits 2 and 5.
All commands end with `\r\n`. Commands are uppercase, responses lowercase.

See [`docs/jura-protocol.md`](docs/jura-protocol.md) for full protocol documentation.

## Disclaimer

> **Use at your own risk.**
>
> This project involves opening or modifying the service interface of a coffee machine and connecting third-party hardware to it. Doing so may **void your warranty**, and incorrect wiring or software could **damage your machine**.
>
> The `FN:` low-level commands in the protocol documentation can control individual actuators (pumps, heaters, valves) directly. Sending wrong command combinations may cause the machine to malfunction or be damaged. Never send `AN:0A` (EEPROM clear).
>
> This project is **not affiliated with, endorsed by, or supported by JURA Elektroapparate AG** in any way. All product names, trademarks, and protocols belong to their respective owners.
>
> The software is provided "as is", without warranty of any kind.

## License

MIT — see [LICENSE](LICENSE).

## Attribution

This project was developed independently. The following community projects were used as **protocol documentation references** (no code was copied):

| Project                                                                    | License      | Accessed    | Role                                              |
| -------------------------------------------------------------------------- | ------------ | ----------- | ------------------------------------------------- |
| [ryanalden/esphome-jura-component](https://github.com/ryanalden/esphome-jura-component) | No license   | 2026-03-23  | ESPHome integration reference                     |
| [Jutta-Proto/protocol-cpp](https://github.com/Jutta-Proto/protocol-cpp)   | GPL-3.0      | 2026-03-23  | Protocol encoding/decoding reference              |
| [Jutta-Proto/hardware-pi](https://github.com/Jutta-Proto/hardware-pi)     | GPL-3.0      | 2026-03-23  | Hardware interface reference                      |
| [alextrical/Jura-F7-ESPHOME](https://github.com/alextrical/Jura-F7-ESPHOME) | GPL-3.0   | 2026-03-23  | F7 model FA: command reference                    |
| [thankthemaker/sharespresso](https://github.com/thankthemaker/sharespresso) | MIT         | 2026-03-23  | ENA/E-series command reference                    |
| [oliverk71/Coffeemaker-Payment-System](https://github.com/oliverk71/Coffeemaker-Payment-System) | MIT | 2026-03-23 | Payment system integration reference    |
| [Q42/coffeehack](https://github.com/Q42/coffeehack)                       | No license   | 2026-03-23  | Protocol encoding reference                       |
| [tiaanv/jura](https://github.com/tiaanv/jura)                             | No license   | 2026-03-23  | General protocol reference                        |
| [sklas/CofFi](https://github.com/sklas/CofFi)                             | No license   | 2026-03-23  | General protocol reference                        |
| [niklasdathe/jurabridge](https://github.com/niklasdathe/jurabridge)       | No license   | 2026-03-23  | General protocol reference                        |
| [thomaswitt/CoffeeMaker](https://github.com/thomaswitt/CoffeeMaker)       | No license   | 2026-03-23  | General protocol reference                        |

## References

- Protocol documentation: http://protocoljura.wiki-site.com/
- HA community thread: https://community.home-assistant.io/t/control-your-jura-coffee-machine/26604
