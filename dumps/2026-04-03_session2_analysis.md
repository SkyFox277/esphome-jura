# Debug Session 2 — Full Sequence (Focused Dump)

Date: 2026-04-03 13:15–13:19
Machine: Jura Impressa F50
Config: RR:03–RR:06, interval 2000ms, IC: yes, RT:0000 yes
Goal: Full automated sequence via D1, verify sensors, EEPROM tracking

---

## Timeline

| Time     | t (ms)  | Event                          | IC: byte0        | RR:03  | RR:04  | RR:06  |
| -------- | ------- | ------------------------------ | ---------------- | ------ | ------ | ------ |
| 13:15:57 | 98817   | Baseline (cold standby)        | `9E` `10011110`  | `0000` | `0020` | `0000` |
| 13:16:01 | 102829  | Baseline cycle 2               | `DE` `11011110`  | `0000` | `0020` | `0000` |
| 13:16:05 | 106826  | Baseline cycle 3               | `DE` `11011110`  | `0000` | `0020` | `0006` |
| 13:16:20 | 121902  | **[CMD] AN:01 → ok:**          | —                | —      | —      | —      |
| 13:16:21 | 122807  | After power on                 | `9E` `10011110`  | `0000` | `0020` | `0006` |
| 13:16:25 | 126817  | Startup phase                  | `DE` `11011110`  | `4000` | `0020` | `0003` |
| 13:16:29 | 130842  | Startup phase                  | `9E` `10011110`  | `0000` | `0020` | `0000` |
| 13:16:37 | 138869  | Startup phase                  | `DE` `11011110`  | `4000` | `0020` | `0000` |
| 13:16:45 | 147032  | Startup settling                | `DE` `11011110`  | `0000` | `0020` | `0004` |
| 13:16:49 | 151051  | Startup settling                | `DE` `11011110`  | `0000` | `0020` | `0002` |
| 13:16:58 | 159974  | **Betriebstemperatur erreicht** | `DE` `11011110`  | `0004` | `0420` | `0002` |
| 13:16:59 | 160385  | **[CMD] FA:02 → ok:** (Rinse)  | —                | —      | —      | —      |
| 13:17:02 | 164156  | Rinsing                        | `9A` `10011010`  | `C004` | `0420` | `0000` |
| 13:17:06 | 168157  | Rinsing                        | `DB` `11011011`  | `C004` | `0420` | `0000` |
| 13:17:11 | 172169  | Rinsing                        | `9A` `10011010`  | `0004` | `0420` | `0000` |
| 13:17:15 | 176163  | Rinsing                        | `9C` `10011100`  | `0004` | `0420` | `0005` |
| 13:17:18 | 180162  | Rinsing                        | `DC` `11011100`  | `0004` | `0420` | `0003` |
| 13:17:23 | 184176  | Rinse ending                   | `DA` `11011010`  | `0004` | `0420` | `0001` |
| 13:17:24 | 185963  | **Rinse done** (RT counter+1)  | —                | —      | —      | —      |
| 13:17:25 | 186514  | **[CMD] FA:06 → ok:** (Coffee) | —                | —      | —      | —      |
| 13:17:27 | 188190  | Grinding                       | `DA` `11011010`  | `0004` | `04A2` | `0000` |
| 13:17:31 | 192201  | Grinding/brewing               | `DE` `11011110`  | `0004` | `04A0` | `0000` |
| 13:17:35 | 196211  | Brewing                        | `DA` `11011010`  | `0004` | `04A0` | `0000` |
| 13:17:39 | 200213  | Brewing                        | `DF` `11011111`  | `C004` | `04A0` | `0000` |
| 13:17:43 | 204220  | Brewing                        | `9F` `10011111`  | `4004` | `04A0` | `0005` |
| 13:17:46 | 208223  | Brewing                        | `DF` `11011111`  | `C004` | `0480` | `0003` |
| 13:17:55 | 216388  | Brewing                        | `DF` `11011111`  | `C004` | `04A0` | `0002` |
| 13:17:59 | 220405  | Brewing ending                 | `9D` `10011101`  | `0004` | `04A8` | `0000` |
| 13:18:03 | 224413  | Brew group reset               | `DB` `11011011`  | `0004` | `04A8` | `0000` |
| 13:18:07 | 228428  | **Coffee done** (RT counter+1) | `DD` `11011101`  | `0004` | `04A8` | `0000` |
| 13:18:09 | 230223  | **[CMD] FA:02 → ok:** (Rinse2) | —                | —      | —      | —      |
| 13:18:11 | 232442  | Rinsing 2                      | `9F` `10011111`  | `C004` | `04A8` | `0000` |
| 13:18:15 | 236462  | Rinsing 2                      | `9E` `10011110`  | `C004` | `04A8` | `0000` |
| 13:18:19 | 240479  | Rinsing 2                      | `9E` `10011110`  | `0004` | `04A8` | `0005` |
| 13:18:23 | 244497  | Rinsing 2                      | `D9` `11011001`  | `0004` | `04A8` | `0003` |
| 13:18:27 | 248528  | Rinse 2 ending                 | `D9` `11011001`  | `0004` | `04A8` | `0001` |
| 13:18:31 | 252540  | **Rinse 2 done** (RT counter+1)| `9F` `10011111`  | `4004` | `0428` | `0000` |
| 13:18:34 | 255530  | **[CMD] AN:02 → ok:** (Off)    | —                | —      | —      | —      |
| 13:18:35 | 256563  | Shutting down                  | `9F` `10011111`  | `0004` | `0428` | `0000` |
| 13:18:39 | 260574  | Auto-rinse on shutdown         | `DF` `11011111`  | (tout) | `0428` | `0000` |
| 13:18:48 | 269502  | Auto-rinse                     | `9F` `10011111`  | `0004` | `0428` | `0000` |
| 13:18:52 | 273502  | Auto-rinse                     | `DF` `11011111`  | `0004` | `0428` | `0000` |
| 13:19:08 | 289538  | Auto-rinse ending              | `DF` `11011111`  | `0004` | `0428` | `0000` |
| 13:19:16 | 297561  | Standby (warm)                 | `9F` `10011111`  | `0004` | `0428` | `0000` |
| 13:19:46 | 317607  | **Debug stopped**              | `9F` `10011111`  | `0004` | `0428` | `0000` |

---

## EEPROM (RT:0000) Progression

```
Baseline:       rt:11090C3519280BB300000014007752F00046001216780003000202350001004D
                   ^^^^                                ^^^^                ^^^^  ^^^^
                   0x0000=4361                         0x0007=21232       0x000D=565  0x000F=77

After Rinse 1:  rt:11090C3519280BB300000014007752F10046001216780003000202350001004D
                                                   ^^
                                                   0x0007: F0→F1 (+1 rinse)

After Coffee:   rt:110A0C3519280BB300000014007752F10046001216780003000202360002004E
                   ^^                                                      ^^  ^^^^  ^^
                   0x0000: 1109→110A (+1 coffee)                           0x000D: 0235→0236 (+1)
                                                                           0x000E: 0001→0002 (+1)
                                                                           0x000F: 004D→004E (+1)

After Rinse 2:  rt:110A0C3519280BB300000014007752F20046001216780003000202360002004E
                                                   ^^
                                                   0x0007: F1→F2 (+1 rinse)

After Shutdown:  (same as after rinse 2 — auto-rinse does NOT increment counter)
```

### EEPROM Counter Summary

| Word   | Offset | Before | After  | Delta | Content                      |
| ------ | ------ | ------ | ------ | ----- | ---------------------------- |
| 0x0000 | 3-6    | `1109` | `110A` | +1    | Coffee counter ✅             |
| 0x0007 | 31-34  | `52F0` | `52F2` | +2    | Rinse counter (2 rinses) ✅   |
| 0x000D | 55-58  | `0235` | `0236` | +1    | **Coffees since cleaning** ⚠️ |
| 0x000E | 59-62  | `0001` | `0002` | +1    | **Unknown counter** ⚠️        |
| 0x000F | 63-66  | `004D` | `004E` | +1    | Coffees since descaling ✅    |

---

## IC: Byte 0 — Bit Analysis (All States)

| Bit | Baseline | Power On | Heating | Rinsing | Brewing | Shutdown | Standby | Interpretation           |
| --- | -------- | -------- | ------- | ------- | ------- | -------- | ------- | ------------------------ |
| 0   | 0        | 0        | 0       | 0       | 1       | 1        | 1       | **Needs rinse on off**   |
| 1   | 1        | 1        | 1       | 1       | 1       | 1        | 1       | Need clean inv ✅         |
| 2   | 1        | 1        | 1       | 0       | 1→0     | 1        | 1       | Busy/idle (0=busy)       |
| 3   | 1        | 1        | 1       | 1       | 1       | 1        | 1       | Tank OK inv ✅            |
| 4   | 1        | 1        | 1       | 1       | 1       | 1        | 1       | Tray OK inv ✅            |
| 5   | 0        | 0        | 0       | 0       | 0       | 0        | 0       | Always 0                 |
| 6   | 0→1      | 0→1      | 0→1     | 0→1     | 0→1     | 0→1      | 0→1     | Oscillating / unreliable |
| 7   | 1        | 1        | 1       | 1       | 1       | 1        | 1       | Always 1                 |

### IC: bit0 — "Needs rinse on power off"
- Cold start: bit0=0
- After first coffee: bit0=1
- Stays 1 through rinse and shutdown
- Machine auto-rinses on power off when bit0=1
- This is the flag the user described!

### IC: bits 6,7 — Unreliable
- bit6 oscillates between consecutive reads in every state
- bit7 always 1
- These should be ignored for sensor purposes

---

## RR:03 — State Patterns

| Value  | Meaning                                    |
| ------ | ------------------------------------------ |
| `0000` | Off / cold / not at operating temperature  |
| `0004` | At operating temperature (PID mode)        |
| `4000` | Startup transition (brief)                 |
| `4004` | Motor active + at temperature              |
| `C004` | Pump/heater active (rinsing, brewing)      |

Bit breakdown:
- bit2 (0x0004): operating temperature reached
- bit14 (0x4000): motor/actuator active (startup, brew group)
- bit15 (0x8000): pump active (combined with bit14 → 0xC004)

---

## RR:04 — Heater/Pump Status

| Value  | Context                  |
| ------ | ------------------------ |
| `0020` | Idle / standby           |
| `0420` | Heater active            |
| `04A0` | Heater + pump (brewing)  |
| `04A2` | Grinding + heater        |
| `04A8` | Post-brew / settling     |
| `0428` | Shutdown/standby (warm)  |

---

## RR:06 — Motor/Actuator Cycling

Values cycle through 0000→0006→0003→0002→0005→0004→0001→0000
during operations. Likely a brew group position encoder or motor phase counter.
Not useful as a binary sensor — informational only.

---

## Key Findings

### 1. Ready sensor: RR:03 bit2 confirmed
- `0000` = not ready (cold)
- `0004` = ready (operating temperature, PID mode)
- Verified: goes to `0000` after ~10-12 min standby, `0004` within ~40s of power on

### 2. IC: bit0 = "needs rinse on power off" confirmed
- 0 after cold start
- 1 after first coffee
- Triggers auto-rinse on AN:02

### 3. IC: bit2 = "idle" (not busy)
- 1 when idle/ready
- 0 during rinsing and some brewing phases
- Unreliable during transitions

### 4. EEPROM 0x000D = likely "coffees since last cleaning"
- Increments by 1 per coffee (not per rinse)
- Current value: 566 — likely resets after cleaning cycle
- Combined with need_clean threshold → cleaning countdown sensor

### 5. EEPROM 0x000E = unknown counter
- Increments by 1 per coffee
- Current value: 2 (very low — recently reset?)
- Needs more observation

### 6. Auto-rinse does NOT increment rinse counter
- Only manual/commanded rinses (FA:02) increment 0x0007
- Shutdown auto-rinse is not counted

### 7. IC: bit6 oscillates
- Not a reliable state indicator
- Should be masked out in sensor logic

---

## Next Steps

- [ ] Implement ready sensor (RR:03 bit2) in component
- [ ] Implement needs_rinse sensor (IC: bit0, inverted)
- [ ] Implement coffees_since_cleaning sensor (EEPROM 0x000D)
- [ ] Test cleaning cycle → confirm 0x000D resets
- [ ] Test FA:06 double-send for strength
- [ ] Test water amount change → EEPROM diff
