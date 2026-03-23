# Jura Impressa F50 — Protocol & Interface Reference

Last updated: 2026-03-23

---

## Sources

| Source                                                                                         | Content                                                 |
| ---------------------------------------------------------------------------------------------- | ------------------------------------------------------- |
| http://protocoljura.wiki-site.com/index.php/Hauptseite                                         | Original protocol documentation (Toptronic V1)          |
| https://github.com/ryanalden/esphome-jura-component                                            | ESPHome custom component (basis of this implementation) |
| https://github.com/Jutta-Proto/protocol-cpp                                                    | C++ protocol implementation, V2 protocol analysis       |
| https://github.com/COM8/esp32-jura                                                             | ESP32 implementation (Bluetooth/XMPP, newer models)     |
| https://github.com/oliverk71/Coffeemaker-Payment-System                                        | Arduino RFID payment system (source of many command lists) |
| https://github.com/sklas/CofFi                                                                 | ESP8266 MQTT implementation                             |
| https://github.com/PromyLOPh/juramote                                                          | Early Python implementation                             |
| https://community.home-assistant.io/t/control-your-jura-coffee-machine/26604                  | HA community thread with wiring photos                  |
| https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/                                  | IC: bit mapping source (NOTE: different machine model!) |
| `Commands for coffeemaker - Protocoljura.pdf` (pdfs/)                                         | Full command list (local reference)                     |
| `Serial interfaces - Protocoljura.pdf` (pdfs/)                                                | Pin assignments for all Jura interfaces (local reference) |
| `Erweiterte Befehle X und S-Reihe.xlsx` (pdfs/)                                               | Extended commands for X and S series (local reference)  |

---

## Hardware: Jura Impressa F50

| Property          | Value                                        |
| ----------------- | -------------------------------------------- |
| Model             | Jura Impressa F50                            |
| Firmware ID       | `EF532M V02.03` (via `TY:`)                  |
| Service interface | 7-pin (Toptronic)                            |
| Microcontroller   | D1 Mini (ESP8266, Wemos)                     |
| Logic level       | 3.3V (ESP) ↔ 5V (Jura) — level shifter required |
| UART baud rate    | 9600 bps                                     |

---

## Interface Pin Assignments

### 7-Pin Interface (F50 and many other models)

```
(8)  7    6    5    4    3    2    1   (0)
 nc  nc  +5V   nc  RxD  GND  TxD   nc
```

Pins numbered right to left (looking at the port from the front):

| Pin | Signal | Description                            |
| --- | ------ | -------------------------------------- |
| 1   | nc     | not connected                          |
| 2   | TxD    | machine transmits (→ ESP RX)           |
| 3   | GND    | ground                                 |
| 4   | RxD    | machine receives (← ESP TX)            |
| 5   | nc     | not connected                          |
| 6   | +5V    | supply voltage (can power the ESP)     |
| 7   | nc     | not connected                          |

> **Important:** The Jura interface operates at 5V TTL. The D1 Mini uses 3.3V.
> A bidirectional level shifter (e.g. TXS0108E or BSS138-based) is required.
> Alternative: 1kΩ + 2kΩ voltage divider on TX (ESP→Jura), Zener diode on RX.

### 4-Pin Interface (e.g. Impressa S95)

```
4    3    2    1
+5V  RxD  GND  TxD
```

### 5-Pin Interface

```
5    4    3    2    1
+5V  nc   RxD  GND  TxD
```

### 9-Pin Interface (e.g. X7)

```
1    2    3    4    5    6    7    8    9
TxD  GND  RxD  +5V  nc   nc   nc   nc   nc
```

---

## Serial Configuration

| Parameter    | Value  |
| ------------ | ------ |
| Baud rate    | 9600   |
| Data bits    | 8      |
| Parity       | none   |
| Stop bits    | 1      |
| Flow control | none   |
| Logic level  | 5V TTL |

---

## Jura Toptronic Protocol (V1)

### Overview

The Jura protocol obfuscates ASCII characters by spreading each byte across 4 UART bytes.
Each ASCII byte is encoded into 4 UART bytes, each carrying 2 bits of the original byte.

All commands and responses end with `\r\n`.
Commands are sent in UPPERCASE; responses arrive in lowercase.

### Encoding (ASCII → 4 UART bytes)

For each ASCII character, 4 UART bytes are transmitted:

```
UART byte 0: bit2 = ASCII[0], bit5 = ASCII[1], all other bits = 1
UART byte 1: bit2 = ASCII[2], bit5 = ASCII[3], all other bits = 1
UART byte 2: bit2 = ASCII[4], bit5 = ASCII[5], all other bits = 1
UART byte 3: bit2 = ASCII[6], bit5 = ASCII[7], all other bits = 1
```

C++ implementation:

```cpp
uint8_t encode_jura_byte(uint8_t ascii, uint8_t bit_pair) {
    // bit_pair: 0=bits[1:0], 1=bits[3:2], 2=bits[5:4], 3=bits[7:6]
    uint8_t raw = 0xFF;
    uint8_t shift = bit_pair * 2;
    if (ascii & (1 << shift))       raw |= (1 << 2);
    else                            raw &= ~(1 << 2);
    if (ascii & (1 << (shift + 1))) raw |= (1 << 5);
    else                            raw &= ~(1 << 5);
    return raw;
}
```

### Example: Encoding `AN:01\r\n`

| Char | ASCII | UART bytes (hex) |
| ---- | ----- | ---------------- |
| `A`  | 0x41  | `DF DB DB DF`    |
| `N`  | 0x4E  | `FB FF DB DF`    |
| `:`  | 0x3A  | `FB FB FF DB`    |
| `0`  | 0x30  | `DB DB FF DB`    |
| `1`  | 0x31  | `DF DB FF DB`    |
| `\r` | 0x0D  | `DF FF DB DB`    |
| `\n` | 0x0A  | `FB FB DB DB`    |

### Timing

```
[ASCII byte]
  → UART byte 0
  → UART byte 1
  → UART byte 2
  → UART byte 3
  → delay(8ms)         ← after all 4 bytes of the character
[next ASCII byte]
  ...
```

> Note: Some sources describe 8ms between each of the 4 UART bytes.
> This implementation (delay after all 4) works correctly in practice.

### Protocol V2 (newer models — NOT F50)

Newer Jura machines (E6 2019+, ENA, etc.) use an extended protocol with a key-exchange handshake:

```
[Client]: @T1\r\n
[Jura]:   @t1\r\n
[Jura]:   @T2:010001B228\r\n
[Client]: @t2:8120000000\r\n
[Jura]:   @T3:3BDEEF532M V02.03\r\n
[Client]: @t3\r\n
```

The F50 uses **V1** (no `@` handshake). The Jura Smart Connect Bluetooth bridge
speaks V2 with modern models.

---

## Command Reference

### Machine Control (AN: commands)

| Command  | Response | Description                               |
| -------- | -------- | ----------------------------------------- |
| `AN:01`  | `ok:`    | Power on / wake from standby              |
| `AN:02`  | `ok:`    | Power off (initiates shutdown sequence)   |
| `AN:0A`  | ?        | Clear EEPROM — **NEVER USE!**             |
| `AN:20`  | `ok:`    | Test mode on                              |
| `AN:21`  | `ok:`    | Test mode off                             |

> **Note on Zero-Energy models:** Newer machines (ENA 7, etc.) use a high-voltage
> latching power switch. `AN:01` alone may not suffice — a relay wired in parallel
> to the physical power button may be required.

### Products (FA: commands) — by model

`FA:` commands simulate physical button presses and are **model-specific**.

| Command  | F50 ✅       | F7 / S95      | E6 / E8       | J6            | ENA 7         | X7 / Saphira  |
| -------- | ------------ | ------------- | ------------- | ------------- | ------------- | ------------- |
| `FA:01`  | —            | —             | —             | Off + rinse   | —             | Product 1     |
| `FA:02`  | Rinse        | —             | —             | —             | —             | Product 2     |
| `FA:03`  | —            | —             | —             | Steam         | —             | Product 3     |
| `FA:04`  | —            | Espresso      | Espresso      | Espresso      | Rinse (prompt)| Product 4     |
| `FA:05`  | —            | 2x Espresso   | Ristretto     | Ristretto     | —             | Product 5     |
| `FA:06`  | Coffee ✅    | Coffee        | Hot water     | Hot water     | —             | Product 6     |
| `FA:07`  | Dbl coffee ✅| 2x Coffee     | Cappuccino    | Espresso      | —             | Product 7     |
| `FA:08`  | —            | Hot water     | —             | 2x Espresso   | Steam         | Hot water     |
| `FA:09`  | —            | Steam         | Coffee        | Coffee        | Coffee (small)| Steam         |
| `FA:0A`  | —            | —             | —             | 2x Coffee     | Coffee (large)| —             |
| `FA:0B`  | —            | Rinse         | —             | Cup light     | Hot water     | Rinse         |
| `FA:0C`  | —            | XXL cup       | —             | Enter menu    | —             | —             |

> Sources: F50 confirmed on hardware. Others from community projects:
> ryanalden/esphome-jura-component (J6), alextrical/Jura-F7-ESPHOME (F7),
> tiaanv/jura (ENA), oliverk71/Coffeemaker-Payment-System (X7/S95).
> Verify commands for your specific model with the `TY:` response.

### Display

| Command            | Description                                                               |
| ------------------ | ------------------------------------------------------------------------- |
| `DA:XXXXXXXXXXXX`  | Show text on display (exactly 12 chars, UPPERCASE, no special characters) |
| `DR:`              | Clear display                                                             |

### Machine Info

| Command | Response example          | Description                          |
| ------- | ------------------------- | ------------------------------------ |
| `TY:`   | `ty:EF532M V02.03`        | Machine type and firmware version    |
| `TL:`   | `tl:BL_RL78 V01.31`       | Bootloader version (E6/E8)           |

Known `TY:` responses:

| Model              | `TY:` response           |
| ------------------ | ------------------------ |
| Impressa F50       | `ty:EF532M V02.03`       |
| E6 2019 / E8 / E65 | `ty:EF532M V02.03`       |
| Impressa J6        | `ty: PIM V01.01`         |

### Low-Level Control (FN: commands)

> ⚠️ Warning: These commands control individual actuators directly.
> Wrong combinations can damage the machine or produce coffee without rinsing first.

| Command  | Function                                              |
| -------- | ----------------------------------------------------- |
| `FN:01`  | Coffee pump on                                        |
| `FN:02`  | Coffee pump off                                       |
| `FN:03`  | Coffee heater on                                      |
| `FN:04`  | Coffee heater off                                     |
| `FN:05`  | Steam heater on                                       |
| `FN:06`  | Steam heater off                                      |
| `FN:07`  | Left grinder on (continuous)                          |
| `FN:08`  | Left grinder off                                      |
| `FN:09`  | Right grinder on / brew group (?)                     |
| `FN:0A`  | Right grinder off / brew group (?)                    |
| `FN:0B`  | Steam pump / coffee press on                          |
| `FN:0C`  | Steam pump / coffee press off                         |
| `FN:0D`  | Brew group initialize (reset)                         |
| `FN:0E`  | Brew group to grounds-eject position                  |
| `FN:0F`  | Brew group to grinding position                       |
| `FN:11`  | Brew group (?)                                        |
| `FN:12`  | Brew group (?)                                        |
| `FN:13`  | Brew group to brewing position (F50)                  |
| `FN:1D`  | Drain valve on                                        |
| `FN:1E`  | Drain valve off                                       |
| `FN:22`  | Brew group to brewing position (E6 / other models)    |
| `FN:24`  | Drain valve / draining on                             |
| `FN:25`  | Drain valve / draining off                            |
| `FN:26`  | Steam valve on                                        |
| `FN:27`  | Steam valve off                                       |
| `FN:28`  | Cappuccino valve on                                   |
| `FN:29`  | Cappuccino valve off                                  |
| `FN:51`  | Power off (alternative to AN:02)                      |
| `FN:8A`  | Debug mode on (machine sends `ku:`/`Ku:` clock bytes) |

### Manual Brew Sequence via FN: (F50)

```
FN:07          # grinder on
delay(3000)    # grind time → determines coffee strength (3s = standard)
FN:08          # grinder off
FN:13          # brew group to brew position  (F50: FN:13, E6: FN:22)
FN:0B          # coffee press on
delay(500)     # compress coffee grounds
FN:0C          # coffee press off
FN:03          # heater on
FN:01          # pump on
delay(2000)    # pre-infusion (~initial water burst)
FN:02          # pump off
FN:04          # heater off
delay(2000)    # water distribution
FN:03          # heater on
FN:01          # pump on
delay(40000)   # 40s brewing → ~200ml coffee
FN:02          # pump off
FN:04          # heater off
FN:0D          # brew group reset + eject grounds
```

---

## Status Queries

### IC: — Read Inputs (Sensor Status)

Command: `IC:`
Response: `ic:XXYYZZ00` (hex string, multiple bytes)

#### Byte 0 — Bit mapping by model

IC: bit positions differ between model families. Two distinct layouts have been identified:

**Layout A — F50 (confirmed on hardware)**

| Bit | Value 1 means          | Value 0 means   | Notes                              |
| --- | ---------------------- | --------------- | ---------------------------------- |
| 0   | Cleaning required      | OK              | ✅ confirmed                        |
| 4   | Tray **inserted**      | Tray missing    | ✅ confirmed — inverted logic!      |
| 5   | Tank empty             | Tank OK         | ⚠️ unconfirmed for F50              |

> **F50 quirk:** Bit 4 = 1 means tray PRESENT (not missing) — opposite of most models.
> Set `ic_tray_inverted: true` in YAML. Implementation: `tray_missing = !((val >> 4) & 1)`.

Example F50 responses:
- `ic:DFB01E00` → Byte 0 = `0xDF` = `11011111` → bit4=1 (tray present), bit5=0 (tank OK)
- `ic:CFB01E00` → Byte 0 = `0xCF` = `11001111` → bit4=0 (tray missing)

**Layout B — E6, E8, ENA, J6 (confirmed from multiple community projects)**

| Bit | Value 1 means          | Value 0 means   | Notes                              |
| --- | ---------------------- | --------------- | ---------------------------------- |
| 0   | Tray missing           | Tray OK         | ✅ confirmed                        |
| 1   | Tank empty             | Tank OK         | ✅ confirmed                        |
| 2   | Cleaning required      | OK              | ⚠️ unconfirmed                      |

Example E6/ENA responses:
- `ic:00` → everything OK
- `ic:01` → tray missing (bit 0 set)
- `ic:02` → tank empty (bit 1 set)
- `ic:03` → both tray missing and tank empty

> **TODO:** Determine bits 3–7 for both layouts by systematic observation.
> Goal: identify the "ready" bit for Startup Sequence Phase B.

### RT:0000 — Read EEPROM Counters

Command: `RT:0000`
Response: `rt:XXXXXXXXXXXXXXXX...` (min. 35 hex characters)

#### Known offsets (confirmed for F50)

| Offset | Length | Content           | Hex example | Notes                         |
| ------ | ------ | ----------------- | ----------- | ----------------------------- |
| 3      | 4      | Single espresso   | `0000`      | F50: always 0 (no espresso)   |
| 7      | 4      | Double espresso   | `0000`      | F50: always 0                 |
| 11     | 4      | Coffee            | `0042`      | = 66 coffees                  |
| 15     | 4      | Double coffee     | `0018`      | = 24                          |
| 19     | 4      | Counter 1         | ?           | meaning unknown               |
| 23     | 4      | Counter 2         | ?           | meaning unknown               |
| 27     | 4      | Counter 3         | ?           | meaning unknown               |
| 31     | 4      | Cleanings 1       | `0008`      | = 8 cleanings                 |
| 35     | 4      | Counter 4         | ?           | meaning unknown               |
| 39     | 4      | Counter 5         | ?           | meaning unknown               |
| 43     | 4      | Counter 6         | ?           | meaning unknown               |
| 47     | 4      | Counter 7         | ?           | meaning unknown               |
| 51     | 4      | Counter 8         | ?           | meaning unknown               |
| 55     | 4      | Cleanings 2 (?)   | ?           | from reference code, unconfirmed |
| 59     | 4      | Counter 9         | ?           | meaning unknown               |
| 63     | 4      | Counter 10        | ?           | meaning unknown               |

> **TODO:** Identify counters by observation — brew coffee, read counters after 5 min, match offset.

### RR: — Read RAM (Debug)

Command: `RR:XX` (XX = hex address, e.g. `RR:00` to `RR:23`)
Response: `rr:YYYY` (16-bit hex value)

#### Observations on F50 during startup sequence

Recorded during: Off → Tray missing → Heating → Rinsing → Ready

| Register | Off    | Tray missing | Heating | Rinsing | Ready  | Interpretation           |
| -------- | ------ | ------------ | ------- | ------- | ------ | ------------------------ |
| RR:03    | `0000` | `0000`       | `0004`  | `0004`  | `0004` | Heater active (bit 2)    |
| RR:04    | `0029` | `0029`       | `0429`  | `0429`  | `0429` | Heater status (?)        |
| RR:18    | `003B` | `003B`       | —       | —       | `003C` | Temperature? (0x3B=59°C, 0x3C=60°C) |
| RR:19    | `3B00` | `3B00`       | —       | —       | `3C00` | Big-endian copy of RR:18 |
| RR:21    | `0024` | `0024`       | varies  | `0013`  | `0034` | Brew group position?     |
| RR:22    | `242E` | `242E`       | varies  | `105E`  | `316C` | Heat-up progress?        |
| RR:23    | `2E00` | `2E00`       | varies  | `5E00`  | `6C00` | Big-endian copy?         |

**Hypotheses (unconfirmed):**
- `RR:03` bit 2 = heater currently active
- `RR:18/19` = current temperature in °C (0x3B = 59°C, 0x3C = 60°C → target ~60°C)
- Temperature rise in RR:18 can be used for Startup Sequence Phase B

> **TODO for Startup Phase B:** After power on, poll `RR:18` every few seconds.
> Once value ≥ `003C` (60°C) → machine ready → start rinsing (`FA:02`).
> Alternative: monitor changes in IC: bits 1–3/6–7.

### RE: — Read EEPROM Word

| Command | Response example | Description        |
| ------- | ---------------- | ------------------ |
| `RE:31` | `re:XXXX`        | Machine type code  |

### WE: — Write EEPROM Word

| Command       | Description         |
| ------------- | ------------------- |
| `WE:31,2712`  | Set machine type    |

> ⚠️ Use write commands with extreme caution!

---

## D1 Mini Wiring (current setup)

```
D1 Mini (ESP8266)     Level Shifter      Jura 7-pin
─────────────────     ─────────────      ──────────
D1 (GPIO5) TX    →   LV1 → HV1      →   Pin 4 (RxD)
D2 (GPIO4) RX    ←   LV2 ← HV2      ←   Pin 2 (TxD)
3.3V             →   LV
                      HV              ←   Pin 6 (+5V)
GND              →   GND             ←   Pin 3 (GND)
```

YAML configuration:

```yaml
uart:
  id: uart_bus
  tx_pin: D1
  rx_pin: D2
  baud_rate: 9600
```

---

## Model Differences

| Model           | IC: layout | Espresso    | Coffee      | Rinse       | Brew pos. | Connector |
| --------------- | ---------- | ----------- | ----------- | ----------- | --------- | --------- |
| Impressa F50    | Layout A   | ❌ none     | FA:06 ✅    | FA:02 ✅    | FN:13     | 7-pin     |
| Impressa F7/F8  | Layout B   | FA:04       | FA:06       | FA:0B       | FN:13     | 7-pin     |
| Impressa S95/S90| Layout B   | FA:04       | FA:06       | FA:0B       | FN:13     | 4-pin     |
| Impressa J6     | Layout B   | FA:07       | FA:09       | FA:0C       | FN:22     | 7-pin     |
| E6 / E8 / E65   | Layout B   | FA:04       | FA:09       | —           | FN:22     | 7-pin     |
| ENA 5/7/Micro90 | Layout B   | —           | FA:09/0A    | FA:04       | FN:22     | 7-pin     |
| X7 / Saphira    | unknown    | FA:01       | FA:06       | FA:0B       | FN:13     | 9-pin     |

> `FA:XX` commands are machine-specific and correspond to physical buttons.
> `FN:XX` low-level commands are largely model-independent.
> Sources: confirmed = tested on hardware; others from community reverse-engineering.

### Model-Specific Quirks

| Model          | Quirk                                                                                  |
| -------------- | -------------------------------------------------------------------------------------- |
| Impressa F50   | IC: tray bit inverted (bit4=1 → tray present)                                          |
| ENA Micro 90   | Service port goes to sleep a few minutes after power-off — cannot be used as permanent power source |
| ENA 7          | High-voltage latching power switch — `AN:01` alone may not work; relay on power button needed |
| ENA Micro 90   | ⚠️ Thermoblock held at 150°C during milk cleaning with no timeout — fire risk if interrupted |
| All models     | Brew group mechanical limit: do not exceed ~8–9g coffee dose via `FN:` sequences       |
| Jura C5        | `FA:09` responds `ok:` but performs no action; `FA:` commands unreliable on this model |
| Z10 / modern   | Bluetooth/WiFi uses XOR encryption with session key — standard serial protocol blocked |

---

## Further Reading

| Link                                                                          | Description                                         |
| ----------------------------------------------------------------------------- | --------------------------------------------------- |
| https://uk.jura.com/en/homeproducts/accessories/SmartConnect-Main-72167       | Jura Smart Connect (official Bluetooth bridge)      |
| https://github.com/Jutta-Proto/hardware-pi                                    | Raspberry Pi hardware implementation                |
| http://heise.de/-3058350                                                      | Heise article: coffee machine payment system (DE)   |
| https://github.com/sklas/CofFi                                                | CofFi: full ESP8266 MQTT implementation             |
