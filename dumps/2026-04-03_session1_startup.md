# Debug Session 1 — Startup Sequence

Date: 2026-04-03 12:26–12:38
Machine: Jura Impressa F50
Firmware: EF532M V02.03
Goal: Identify ready sensor, verify IC: bits, observe RR: registers during startup

---

## Timeline

| Time     | t (ms)  | Event                                      |
| -------- | ------- | ------------------------------------------ |
| 12:26:08 | 316949  | Debug dump started, machine in **standby** |
| ~12:27   | —       | Machine powered on via HA button           |
| ~12:28   | —       | Display: "Pflege drücken" (please rinse)   |
| ~12:29   | —       | Rinse button pressed on machine            |
| 12:31:03 | 611078  | Machine **rinsing**                        |
| ~12:32   | —       | Rinse complete                             |
| ~12:33   | —       | ESPHome addon crashed, debug stopped       |
| 12:36:24 | 212990  | Debug restarted (ESP rebooted, new t=0)    |
| 12:38:44 | 352779  | Machine **ready** (idle, heated)           |

---

## Full Register Dumps by State

### State: Standby (machine off) — 12:26, t=350603

```
IC:   9EB01E00  → byte0 = 0x9E = 10011110
RR:00 0002
RR:01 0212
RR:02 1200
RR:03 0000      ← heater OFF
RR:04 0029
RR:05 2900
RR:06 0000
RR:07 0105
RR:08 0912
RR:09 (timeout)
RR:0A 0011
RR:0B 1102
RR:0C 0000
RR:0D 0000
RR:0E 0001
RR:0F 0100
RR:10 0000
RR:11 0000
RR:12 0000
RR:13 0000
RR:14 0000
RR:15 0000
RR:16 0000
RR:17 0000
RR:18 0034      ← 0x34 = 52 dec (NOT temperature — see notes)
RR:19 3400      ← big-endian copy of RR:18
RR:1A 0000
RR:1B (timeout)
RR:1C 0009
RR:1D 0900
RR:1E 001E
RR:1F 1E0C
RR:20 1F00
RR:21 0024
RR:22 243F
RR:23 3F00
RT:0000 rt:11080C3519280BB300000014007752EE0046001216780003000202330004004C
```

### State: Powered on (before heating) — ~12:28, t=438244

```
IC:   DAB01E00  → byte0 = 0xDA = 11011010
RR:03 0000      ← heater not yet active
RR:04 0029
RR:06 0003      ← changed from 0000
RR:0B 1104      ← changed from 1102
RR:0C 0200      ← changed from 0000
RR:12 0001      ← changed from 0000
RR:18 0034      ← unchanged
RR:21 0028      ← changed from 0024
```

### State: "Pflege drücken" (please rinse prompt) — ~12:29, t=504105

```
IC:   DAB01E00  → byte0 = 0xDA = 11011010
        (note: earlier captured as 0x9A — IC: fluctuates between reads)
RR:03 0004      ← heater ON (PID mode)
RR:04 0429      ← heater status active
RR:06 0000
RR:18 0034      ← unchanged — machine already at operating temp
```

### State: Rinsing — 12:31, t=611078

```
IC:   DAB01E00  → byte0 = 0xDA = 11011010
RR:03 0004      ← heater ON
RR:04 0429
RR:06 0000
RR:18 0035      ← +1 from standby (52→53 dec)
```

### State: Ready (idle, heated) — 12:38, t=352779 (after ESP reboot)

```
IC:   DEB01E00  → byte0 = 0xDE = 11011110
RR:00 0002
RR:01 0202
RR:02 0200
RR:03 0004      ← heater ON (PID mode)
RR:04 0429
RR:05 2900
RR:06 0003
RR:07 0000
RR:08 001C
RR:09 1C00
RR:0A 0011
RR:0B 1100
RR:0C 0400
RR:0D 0000
RR:0E 0001
RR:0F 0100
RR:10 0000
RR:11 0000
RR:12 0001
RR:13 0100
RR:14 0000
RR:15 0000
RR:16 0000
RR:17 0000
RR:18 0035
RR:19 3500
RR:1A 0010
RR:1B 1000
RR:1C 0009
RR:1D 0900
RR:1E 001E
RR:1F 1E2B
RR:20 0B00
RR:21 001B
RR:22 185F
RR:23 (timeout)
RT:0000 rt:11080C3519280BB300000014007752EF0046001216780003000202330004004C
```

---

## IC: Byte 0 — Bit Analysis Across States

| Bit | Standby `9E` | Power On `DA` | Pflege `9A`* | Rinsing `DA` | Ready `DE` | Interpretation         |
| --- | ------------ | ------------- | ------------ | ------------ | ---------- | ---------------------- |
| 0   | 0            | 0             | 0            | 0            | 0          | always 0               |
| 1   | 1            | 1             | 1            | 1            | 1          | need_clean inv ✅       |
| 2   | 1            | 0             | 0            | 0            | 1          | idle/ready? (0=busy)   |
| 3   | 1            | 1             | 1            | 1            | 1          | tank OK inv ✅          |
| 4   | 1            | 1             | 1            | 1            | 1          | tray OK inv ✅          |
| 5   | 0            | 0             | 0            | 0            | 0          | always 0               |
| 6   | 0            | 1             | 0            | 1            | 1          | motor/pump active?     |
| 7   | 1            | 1             | 1            | 1            | 1          | always 1               |

*Pflege 0x9A was captured in a separate read — IC: byte0 fluctuates between consecutive reads (bits 6,7 unstable).

---

## EEPROM (RT:0000) Diff

```
Before rinse: rt:...007752EE...
After rinse:  rt:...007752EF...
                          ^^
Offset 31-34 (word 0x0007 = rinse counter): 52EE → 52EF = +1 ✅
```

All other EEPROM words unchanged.

---

## Key Findings

### 1. RR:18 is NOT temperature
- Standby: 0x34 (52 dec), Ready: 0x35 (53 dec)
- Barely changes between cold and heated state
- Coffee output is 70-80°C, thermoblock must be 90°C+
- Previous hypothesis (RR:18 = °C, ≥0x3C = ready) is WRONG

### 2. RR:03 = best ready indicator
- `0x0000` = machine off / standby
- `0x0004` = heater active (PID mode) = machine operational
- Binary, reliable, clear distinction

### 3. IC: bit 2 = idle/busy
- 1 in standby and ready (idle states)
- 0 during startup, rinsing, transitions (busy states)
- Could be used as "machine idle" sensor

### 4. IC: bit 6 = unstable / motor related
- Fluctuates between consecutive reads
- Not reliable for state detection

### 5. Rinse counter confirmed
- EEPROM word 0x0007 increments by 1 after rinse ✅

### 6. Registers that timeout
- RR:09 and RR:1B occasionally return empty (timeout)
- May not exist on F50 or have intermittent responses

---

## Next Steps

- [ ] Brew coffee → observe RR: changes during brew + EEPROM diff
- [ ] Power off → confirm RR:03 returns to 0x0000
- [ ] Test FA:06 double-send for strength control
- [ ] Change water amount on machine → EEPROM diff
