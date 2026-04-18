# Jura Impressa F50 — Protocol & Interface Reference

Last updated: 2026-04-18

---

## Sources

| Source                                                                                         | Content                                                 |
| ---------------------------------------------------------------------------------------------- | ------------------------------------------------------- |
| http://protocoljura.wiki-site.com/index.php/Hauptseite                                         | Original protocol documentation (Toptronic)          |
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

| Command  | Response | Description                                                                |
| -------- | -------- | -------------------------------------------------------------------------- |
| `AN:01`  | `ok:`    | Power on / wake from standby                                               |
| `AN:02`  | `ok:`    | Power off (initiates shutdown sequence)                                    |
| `AN:0A`  | ?        | Clear EEPROM — **BLOCKED by component** (irreversible, no remote use case) |
| `AN:20`  | `ok:`    | Test mode on                                                               |
| `AN:21`  | `ok:`    | Test mode off                                                              |

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

**Layout A — F50 (confirmed from hardware debug sessions 2026-04-03)**

All F50 status bits use inverted logic UNLESS noted: **0 = problem, 1 = OK**.

| Bit | Value 1 means              | Value 0 means             | Config key                 | Logic    | Status           |
| --- | -------------------------- | ------------------------- | -------------------------- | -------- | ---------------- |
| 0   | Session used since cold    | Cold-start state          | `ic_needs_rinse_bit`       | 1=used   | ✅ confirmed     |
| 1   | Not in cleaning cycle      | Cleaning cycle running    | `ic_need_clean_inverted`   | inverted | ✅ confirmed     |
| 2   | Idle (not busy)            | Busy                      | —                          | 1=idle   | ⚠️ hypothesis   |
| 3   | Tank OK                    | Tank empty/missing        | `ic_tank_inverted`         | inverted | ✅ confirmed     |
| 4   | Tray **inserted**          | Tray missing              | `ic_tray_inverted`         | inverted | ✅ confirmed     |
| 5   | —                          | —                         | —                          | —        | ✅ always 0      |
| 6   | —                          | —                         | —                          | —        | ❌ unreliable    |
| 7   | —                          | —                         | —                          | —        | ✅ always 1      |

Example F50 responses:
```
ic:DFB01E00  0xDF = 1101 1111  → bit1=1, bit3=1, bit4=1 → all OK
ic:D7B01E00  0xD7 = 1101 0111  → bit3=0 → tank empty
ic:CFB01E00  0xCF = 1100 1111  → bit4=0 → tray missing
ic:CEB01E00  0xCE = 1100 1110  → bit4=0 → tray missing (confirmed by physically removing tray: 0xDE→0xCE)
ic:D9B01E00  0xD9 = 1101 1001  → bit1=0, bit2=0 → cleaning cycle running ("Pflege" phase active)
```

> Bit 0 (session-used flag): 0 after cold start, 1 after first coffee. Not
> cleared by manual rinse, auto-rinse on shutdown, or a single power cycle —
> extended cool-down is required (Session 4 observation, 2026-04-18).
> Bit 1 is 0 during the actual cleaning cycle, 1 otherwise. It is NOT the
> display-prompt "Pflege drücken" flag — the user can see the prompt while
> bit 1 = 1, because the prompt condition lives elsewhere (see EEPROM 0x0005
> hypothesis).
> Bit 5 is always 0 in all known F50 samples. Bit 6 oscillates between reads
> and is unreliable. Bit 7 is always 1.
>
> Sensor key naming: `ic_needs_rinse_bit` refers to bit 0 and the ESPHome
> sensor key `needs_rinse` is misnamed — see A4 in TODO.md for the planned
> v2.0 rename (`session_used`).

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

Command: `RT:XXYY`
Response: `rt:` + 64 hex chars = 32 bytes = 16 × 16-bit words (total 67 chars)

`RT:XXYY` reads the 16-word EEPROM page starting at word address `0xXXYY`.
Each 4-char hex group in the response is one 16-bit word (big-endian).

The F50 has **10 pages** (160 words, 320 bytes) of readable EEPROM:

| Command     | EEPROM range | Content                                    |
| ----------- | ------------ | ------------------------------------------ |
| `RT:0000`   | 0x00–0x0F    | Counters (coffee, rinse, clean, descale)   |
| `RT:1000`   | 0x10–0x1F    | Extended counters / unknown                |
| `RT:2000`   | 0x20–0x2F    | Configuration parameters                   |
| `RT:3000`   | 0x30–0x3F    | Configuration parameters                   |
| `RT:4000`   | 0x40–0x4F    | Timing parameters                          |
| `RT:5000`   | 0x50–0x5F    | Timing parameters                          |
| `RT:6000`   | 0x60–0x6F    | Mostly zero, some values at 0x6B–0x6E      |
| `RT:7000`   | 0x70–0x7F    | Calibration / configuration                |
| `RT:8000`   | 0x80–0x8F    | Configuration                              |
| `RT:9000`   | 0x90–0x9F    | Configuration, zero from 0x9B              |
| `RT:A000+`  |              | All zeros (end of EEPROM)                  |

Individual words can also be read via `RE:XX` (see below).

#### EEPROM word map — confirmed from F50 hardware sessions (2026-04-03, 2026-04-18)

On F50, the physical coffee-button press count selects strength and maps to
three distinct EEPROM addresses (Session 4, 2026-04-18):
1× press → `0x0000`, 2× press → `0x0001`, 3× press → `0x0002`.
The existing sensor keys `num_single_espresso` / `num_double_espresso` /
`num_coffee` point at these addresses but with misleading names — the
planned rename lives in TODO.md A4.

| EEPROM addr | RT offset | Length | Content                               | ESPHome sensor key      | Notes                                       |
| ----------- | --------- | ------ | ------------------------------------- | ----------------------- | ------------------------------------------- |
| 0x0000      | 3         | 4      | F50: 1×-press coffee (mild)           | `num_single_espresso`   | ✅ confirmed session 4                       |
| 0x0001      | 7         | 4      | F50: 2×-press coffee (normal)         | `num_double_espresso`   | ✅ confirmed session 4                       |
| 0x0002      | 11        | 4      | F50: 3×-press coffee (strong)         | `num_coffee`            | ✅ confirmed session 4                       |
| 0x0003      | 15        | 4      | F50: double-button coffee             | `num_double_coffee`     | ✅ confirmed session 4 — no strength variant |
| 0x0004      | 19        | 4      | Bezüge Espresso?                      | —                       | meaning model-specific, 0 on F50            |
| 0x0005      | 23        | 4      | Byte-split: LB=config (=0x14 on F50), HB=cleaning-reset time ticker | — | ⚠️ hypothesis — primary Pflege-trigger candidate, see session 4 |
| 0x0006      | 27        | 4      | Pulver (powder / grounds counter)     | —                       | —                                           |
| 0x0007      | 31        | 4      | Spülvorgänge (rinse count)            | `num_rinse`             | ✅ confirmed — increments on FA:02 AND on auto-rinse at shutdown (F50 2026-04-18 supersedes earlier session 2 claim) |
| 0x0008      | 35        | 4      | Reinigungszyklen (cleaning cycles)    | `num_clean`             | ✅ confirmed — +1 per cleaning cycle (session 4) |
| 0x0009      | 39        | 4      | Entkalkungszyklen (descaling cycles)  | `num_descale`           | ✅ confirmed                                |
| 0x000A      | 43        | 4      | unknown                               | —                       | —                                           |
| 0x000B      | 47        | 4      | unknown (typically 17)                | —                       | —                                           |
| 0x000C      | 51        | 4      | unknown                               | —                       | value varies (0 in pre-session-2 sample response below, 2 on session-4 F50) |
| 0x000D      | 55        | 4      | Brew-event counter                    | —                       | ✅ session 4 — +1 per brew command, +3 on strong-double (internal pre-flush), not reset by cleaning |
| 0x000E      | 59        | 4      | Cups counter, resets on cold-start    | `num_coffees_since_clean` | ⚠️ sensor key is misnamed — +1 single / +2 double; resets to 0 on a cold-start (full power loss + reboot); transient 0xFA during cleaning. "Daily reset" patterns in practice are a by-product of nightly power-cycling automations — semantic name is `cups_since_cold_start`, not "since cleaning" |
| 0x000F      | 63        | 4      | Brews since cleaning                  | —                       | ✅ session 4 — +1 per brew command, reset by cleaning. Supersedes session 2's speculative "coffees since descaling" label |

#### Extended EEPROM (RE: only — not visible via RT:0000)

The F50 has 32 EEPROM words (0x00–0x1F). `RT:0000` only returns words 0x00–0x0F.
Words 0x10–0x1F can only be read via `RE:XX` and written via `WE:XX,YYYY`.

| EEPROM addr | RE: value (F50 sample) | Content                                        |
| ----------- | ---------------------- | ---------------------------------------------- |
| 0x0010      | `8747`                 | Long-term brew-event counter — mirrors 0x000D pattern, grows +1 per brew, not reset by cleaning |
| 0x0011      | `0037`                 | Cleaning-reset counter — grows ~7.6/day in normal use, driver unknown (neither coffee nor rinse moves it) |
| 0x0012      | `00EC`                 | unknown                                        |
| 0x0013      | `0320`                 | unknown                                        |
| 0x0014      | `C05D`                 | Per-product brew-phase counter — +5 mild / +6 normal & strong / +7 mild-double. Not water volume |
| 0x0015      | `4292`                 | Long-term brew-command counter — mirrors 0x000F pattern, +1 per brew command |
| 0x0016      | `0000`                 | Cleaning-reset counter — grows ~2/day in normal use, hypothesis: per session or power-on |
| 0x0017      | `0000`                 | unknown (zero)                                 |
| 0x0018      | `0000`                 | unknown (intermittent timeout on read)         |
| 0x0019      | `0000`                 | unknown (zero)                                 |
| 0x001A      | `1E02`                 | unknown                                        |
| 0x001B      | `0126`                 | unknown                                        |
| 0x001C      | `0256`                 | unknown (intermittent timeout on read)         |
| 0x001D      | `0005`                 | unknown                                        |
| 0x001E      | `0004`                 | unknown                                        |
| 0x001F      | `03DD`                 | unknown                                        |

> **Water amount setting:** Exhaustively tested at 30ml, 55ml, 120ml, 240ml (max).
> Scanned all 160 EEPROM words (RT:0000–9000, RE:00–9F) — no difference between settings.
> Scanned all 256 RR: RAM registers (0x00–0xFF) — only time-dependent volatile changes,
> no reproducible correlation to water amount. Also no change after power cycle.
> The water amount setting is stored internally by the Jura controller and is
> **not accessible via the serial interface**.

Example F50 response (decoded):
```
rt:0FF307B210630BB1000000140077392E002D000D167800000000021B00020040
     ^^^^              → 0x0FF3 = 4083 1×-press coffees (addr 0x0000)
         ^^^^          → 0x07B2 = 1970 2×-press coffees (addr 0x0001)
                     ^^^^       → 0x392E = 14638 Rinses (addr 0x0007)
                         ^^^^   → 0x002D = 45 Cleanings (addr 0x0008)
                             ^^^^→ 0x000D = 13 Descalings (addr 0x0009)
```

> **Note on counter labels (session 4 reconciliation, 2026-04-18):**
>
> - On F50 the press-count maps strength to three different addresses:
>   1× press → `0x0000` (sensor `num_single_espresso`),
>   2× press → `0x0001` (sensor `num_double_espresso`),
>   3× press → `0x0002` (sensor `num_coffee`). All three ESPHome sensor keys
>   are misnamed on F50 — none of them represents espresso nor a "small"
>   size. The double-coffee button always increments `0x0003` regardless of
>   press count.
> - `num_coffees_since_clean` points at `0x000E`, which is a cups counter with
>   daily resets — not a since-cleaning counter. The real brews-since-cleaning
>   counter is `0x000F` (unexposed; planned new sensor). The `0x000E` sensor
>   also takes a transient value of 0xFA (250) during the cleaning cycle.
> - `needs_rinse` points at IC:bit0, which is actually a "session used since
>   cold start" flag — not cleared by manual rinse, auto-rinse, or a brief
>   power cycle. Sensor name will be corrected in v2.0 (see TODO A4).
>
> All ESPHome sensor keys are preserved as-is to keep existing HA entity_ids
> and InfluxDB history intact. The coordinated rename lands in the v2.0
> release once observation sessions complete.

### RR: — Read RAM (Debug)

Command: `RR:XX` (XX = hex address, 0x00–0xFF)
Response: `rr:YYYY` (16-bit hex value)

The F50 has **256 readable RAM registers** (0x00–0xFF). Registers 0x00–0x23 contain
live machine state. Registers 0x24+ appear to mirror EEPROM content and configuration
parameters. Many odd-numbered registers are big-endian byte-swapped copies of the
preceding even register.

#### RR:03 — Operating Temperature / Ready Status (✅ confirmed)

RR:03 bit 2 (0x0004) indicates the machine has reached operating temperature (PID mode).
This is used as the **ready** binary sensor.

| Value  | Meaning                                    |
| ------ | ------------------------------------------ |
| `0000` | Off / cold / not at operating temperature  |
| `0004` | At operating temperature (PID mode)        |
| `4000` | Startup transition (brief)                 |
| `4004` | Motor active + at temperature              |
| `C004` | Pump/heater active (rinsing, brewing)      |

> `0x0004` returns to `0x0000` after ~10-12 min standby (thermal cooldown).

#### RR:18/19 — **DISPROVEN** as temperature

Previous hypothesis: RR:18 = temperature in degrees C (>=0x3C = 60 degrees C = ready).
**Disproven:** RR:18 shows 0x34-0x35 in ALL states (cold, heating, ready). Value barely
changes between states. Not useful as temperature or ready indicator.

#### Observations on F50 during startup sequence

Recorded during: Off → Tray missing → Heating → Rinsing → Ready

| Register | Off    | Tray missing | Heating | Rinsing | Ready  | Interpretation                    |
| -------- | ------ | ------------ | ------- | ------- | ------ | --------------------------------- |
| RR:03    | `0000` | `0000`       | `0004`  | `0004`  | `0004` | ✅ operating temp reached (bit 2) |
| RR:04    | `0029` | `0029`       | `0429`  | `0429`  | `0429` | Heater status (?)                 |
| RR:18    | `003B` | `003B`       | —       | —       | `003C` | ❌ NOT temperature — barely changes |
| RR:19    | `3B00` | `3B00`       | —       | —       | `3C00` | ❌ big-endian copy of RR:18       |
| RR:21    | `0024` | `0024`       | varies  | `0013`  | `0034` | ⚠️ unconfirmed — brew group?     |
| RR:22    | `242E` | `242E`       | varies  | `105E`  | `316C` | ⚠️ unconfirmed                   |
| RR:23    | `2E00` | `2E00`       | varies  | `5E00`  | `6C00` | ⚠️ unconfirmed — big-endian?     |

> **Ready detection:** Use RR:03 bit 2 (0x0004) to detect operating temperature.
> RR:18/19 do NOT work for this purpose.

### RE: — Read EEPROM Word

`RE:XX` reads a single 16-bit EEPROM word at address `XX` (hex).
Unlike `RT:0000` (which returns 16 words as a block), `RE:` can read the full
EEPROM range including extended addresses 0x10–0x1F.

| Command  | Response example | Description                                          |
| -------- | ---------------- | ---------------------------------------------------- |
| `RE:00`  | `re:110A`        | EEPROM word 0x00 (same as first word in RT:0000)     |
| `RE:10`  | `re:8747`        | Extended EEPROM word 0x10 (NOT visible via RT:0000)  |
| `RE:31`  | `re:XXXX`        | Machine type code                                    |

> The F50 has 32 readable EEPROM words (0x00–0x1F). Addresses 0x20+ appear to
> wrap or return the same data. Some addresses (0x18, 0x1C) occasionally timeout.

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
