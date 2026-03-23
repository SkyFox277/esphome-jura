# ESPHome Jura Coffee Machine

ESPHome external component for Jura coffee machines with Toptronic serial interface (V1 protocol).
Tested on **Jura Impressa F50** with a D1 Mini (ESP8266).

## Features

- Binary sensors: tray missing, water tank empty, need cleaning
- EEPROM counters: coffees, double coffees, espressos, cleanings
- Control buttons: power on/off, coffee, double coffee, rinse
- Raw command action: `jura_coffee.send_command` for use in automations
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

### 1. Copy component

Copy the `components/jura_coffee/` folder into your ESPHome config directory:

```
<esphome-config>/
└── components/
    └── jura_coffee/
        ├── __init__.py
        ├── jura_coffee.h
        └── jura_coffee.cpp
```

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

| Key                  | Type    | Default | Description                                        |
| -------------------- | ------- | ------- | -------------------------------------------------- |
| `uart_id`            | id      | —       | UART bus ID                                        |
| `update_interval`    | time    | `60s`   | How often to poll `IC:` and `RT:0000`              |
| `machine_type`       | string  | —       | Known model: `f50`, `e6`, `j6`, `x7` — auto-sets IC: bits |
| `ic_tray_bit`        | int 0–7 | `4`     | Bit position in IC: byte 0 for tray sensor (overrides machine_type) |
| `ic_tank_bit`        | int 0–7 | `5`     | Bit position in IC: byte 0 for tank sensor (overrides machine_type) |
| `ic_need_clean_bit`  | int 0–7 | `0`     | Bit position in IC: byte 0 for cleaning sensor (overrides machine_type) |
| `ic_tray_inverted`   | bool    | `false` | `true` if bit=1 means tray PRESENT — set automatically for `f50` |
| `tray_missing`       | sensor  | —       | Binary sensor: tray missing                        |
| `tank_empty`         | sensor  | —       | Binary sensor: water tank empty                    |
| `need_clean`         | sensor  | —       | Binary sensor: cleaning required                   |
| `num_single_espresso`| sensor  | —       | Counter: single espressos                          |
| `num_double_espresso`| sensor  | —       | Counter: double espressos                          |
| `num_coffee`         | sensor  | —       | Counter: coffees                                   |
| `num_double_coffee`  | sensor  | —       | Counter: double coffees                            |
| `num_clean`          | sensor  | —       | Counter: cleanings                                 |

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

The Jura Toptronic V1 protocol encodes each ASCII character as 4 UART bytes using bits 2 and 5.
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

## References

- Protocol documentation: http://protocoljura.wiki-site.com/
- Original ESPHome component: https://github.com/ryanalden/esphome-jura-component
- Protocol C++ reference: https://github.com/Jutta-Proto/protocol-cpp
- HA community thread: https://community.home-assistant.io/t/control-your-jura-coffee-machine/26604
