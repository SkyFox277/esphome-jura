# Debug Session 3 — Water Amount Investigation

Date: 2026-04-03 16:30–17:45
Machine: Jura Impressa F50
Goal: Find where water amount setting is stored

---

## Result

**Water amount setting is NOT accessible via the Jura serial interface.**

Tested all known address spaces (RT:, RE:, RR:) at multiple water amount settings.
No register or EEPROM word changed. The setting is stored internally in the
Jura controller and is not exposed via Toptronic V1 protocol.

---

## Test Matrix

| Setting | RT:0000–9000 | RE:00–FF | RR:00–FF | Change found |
| ------- | ------------ | -------- | -------- | ------------ |
| 30ml    | ✅ scanned    | ✅ scanned | ✅ scanned | none         |
| 55ml    | ✅ scanned    | ✅ scanned | ✅ partial  | none         |
| 120ml   | —            | —        | ✅ partial  | none (stable)|
| 240ml   | ✅ scanned    | ✅ scanned | ✅ scanned | none         |

---

## Full EEPROM Dump (at 55ml — identical at 240ml)

### RT:0000 — Page 0 (EEPROM 0x00–0x0F)

```
rt:110A0C3519290BB300000014007752F60046001216780003000202390003004F
```

| Word | Offset | Hex    | Dec   | Content                      |
| ---- | ------ | ------ | ----- | ---------------------------- |
| 0x00 | 3-6    | `110A` | 4362  | Coffee counter               |
| 0x01 | 7-10   | `0C35` | 3125  | Double coffee counter        |
| 0x02 | 11-14  | `1929` | 6441  | Small coffee counter         |
| 0x03 | 15-18  | `0BB3` | 2995  | Double small counter         |
| 0x04 | 19-22  | `0000` | 0     | Espresso (0 on F50)          |
| 0x05 | 23-26  | `0014` | 20    | Special coffee               |
| 0x06 | 27-30  | `0077` | 119   | Powder/grounds counter       |
| 0x07 | 31-34  | `52F6` | 21238 | Rinse counter                |
| 0x08 | 35-38  | `0046` | 70    | Cleaning cycles              |
| 0x09 | 39-42  | `0012` | 18    | Descaling cycles             |
| 0x0A | 43-46  | `1678` | 5752  | unknown                      |
| 0x0B | 47-50  | `0003` | 3     | unknown                      |
| 0x0C | 51-54  | `0002` | 2     | unknown                      |
| 0x0D | 55-58  | `0239` | 569   | unknown (since descaling?)   |
| 0x0E | 59-62  | `0003` | 3     | Coffees since cleaning       |
| 0x0F | 63-66  | `004F` | 79    | unknown counter              |

### RT:1000 — Page 1 (EEPROM 0x10–0x1F)

```
rt:8747003700EC0320C05D429200000000000000001E02012602560005000403DD
```

| Word | Hex    | Dec   | Content                      |
| ---- | ------ | ----- | ---------------------------- |
| 0x10 | `8747` | 34631 | unknown (constant)           |
| 0x11 | `0037` | 55    | unknown                      |
| 0x12 | `00EC` | 236   | unknown                      |
| 0x13 | `0320` | 800   | unknown                      |
| 0x14 | `C05D` | 49245 | unknown                      |
| 0x15 | `4292` | 17042 | unknown                      |
| 0x16 | `0000` | 0     | unknown (zero)               |
| 0x17 | `0000` | 0     | unknown (zero)               |
| 0x18 | `0000` | 0     | unknown (zero, sporadic timeout) |
| 0x19 | `0000` | 0     | unknown (zero)               |
| 0x1A | `1E02` | 7682  | unknown                      |
| 0x1B | `0126` | 294   | unknown                      |
| 0x1C | `0256` | 598   | unknown (sporadic timeout)   |
| 0x1D | `0005` | 5     | unknown                      |
| 0x1E | `0004` | 4     | unknown                      |
| 0x1F | `03DD` | 989   | unknown                      |

### RT:2000 — Page 2 (EEPROM 0x20–0x2F)

```
rt:963C1210012C019000035004140400C803ECA1000C0E0C1414060000160A0704
```

| Word | Hex    | Dec   | Content                      |
| ---- | ------ | ----- | ---------------------------- |
| 0x20 | `963C` | 38460 | unknown                      |
| 0x21 | `1210` | 4624  | unknown                      |
| 0x22 | `012C` | 300   | unknown                      |
| 0x23 | `0190` | 400   | unknown                      |
| 0x24 | `0003` | 3     | unknown                      |
| 0x25 | `5004` | 20484 | unknown                      |
| 0x26 | `1404` | 5124  | unknown                      |
| 0x27 | `00C8` | 200   | unknown                      |
| 0x28 | `03EC` | 1004  | unknown                      |
| 0x29 | `A100` | 41216 | unknown                      |
| 0x2A | `0C0E` | 3086  | unknown                      |
| 0x2B | `0C14` | 3092  | unknown                      |
| 0x2C | `1406` | 5126  | unknown                      |
| 0x2D | `0000` | 0     | unknown                      |
| 0x2E | `160A` | 5642  | unknown                      |
| 0x2F | `0704` | 1796  | unknown                      |

### RT:3000 — Page 3 (EEPROM 0x30–0x3F)

```
rt:4C4A000322982279009D0316000003200A3200C8082428500614016814280276
```

| Word | Hex    | Dec   | Content                      |
| ---- | ------ | ----- | ---------------------------- |
| 0x30 | `4C4A` | 19530 | unknown                      |
| 0x31 | `0003` | 3     | unknown (machine type code?) |
| 0x32 | `2298` | 8856  | unknown                      |
| 0x33 | `2279` | 8825  | unknown                      |
| 0x34 | `009D` | 157   | unknown                      |
| 0x35 | `0316` | 790   | unknown                      |
| 0x36 | `0000` | 0     | unknown                      |
| 0x37 | `0320` | 800   | unknown                      |
| 0x38 | `0A32` | 2610  | unknown                      |
| 0x39 | `00C8` | 200   | unknown                      |
| 0x3A | `0824` | 2084  | unknown                      |
| 0x3B | `2850` | 10320 | unknown                      |
| 0x3C | `0614` | 1556  | unknown                      |
| 0x3D | `0168` | 360   | unknown                      |
| 0x3E | `1428` | 5160  | unknown                      |
| 0x3F | `0276` | 630   | unknown                      |

### RT:4000 — Page 4 (EEPROM 0x40–0x4F)

```
rt:000F00E6010A02260222060E0AF009C41B58A0EC04B0320A03B6060E00043006
```

| Word | Hex    | Dec   | Content                      |
| ---- | ------ | ----- | ---------------------------- |
| 0x40 | `000F` | 15    | unknown                      |
| 0x41 | `00E6` | 230   | unknown                      |
| 0x42 | `010A` | 266   | unknown                      |
| 0x43 | `0226` | 550   | unknown                      |
| 0x44 | `0222` | 546   | unknown                      |
| 0x45 | `060E` | 1550  | unknown                      |
| 0x46 | `0AF0` | 2800  | unknown                      |
| 0x47 | `09C4` | 2500  | unknown                      |
| 0x48 | `1B58` | 7000  | unknown                      |
| 0x49 | `A0EC` | 41196 | unknown                      |
| 0x4A | `04B0` | 1200  | unknown                      |
| 0x4B | `320A` | 12810 | unknown                      |
| 0x4C | `03B6` | 950   | unknown                      |
| 0x4D | `060E` | 1550  | unknown                      |
| 0x4E | `0004` | 4     | unknown                      |
| 0x4F | `3006` | 12294 | unknown                      |

### RT:5000 — Page 5 (EEPROM 0x50–0x5F)

```
rt:12251712001350010A051002003C005004041C0004023C1464C8463C50090000
```

| Word | Hex    | Dec   | Content                      |
| ---- | ------ | ----- | ---------------------------- |
| 0x50 | `1225` | 4645  | unknown                      |
| 0x51 | `1712` | 5906  | unknown                      |
| 0x52 | `0013` | 19    | unknown                      |
| 0x53 | `5001` | 20481 | unknown                      |
| 0x54 | `0A05` | 2565  | unknown                      |
| 0x55 | `1002` | 4098  | unknown                      |
| 0x56 | `003C` | 60    | unknown                      |
| 0x57 | `0050` | 80    | unknown                      |
| 0x58 | `0404` | 1028  | unknown (sporadic timeout)   |
| 0x59 | `1C00` | 7168  | unknown                      |
| 0x5A | `0402` | 1026  | unknown                      |
| 0x5B | `3C14` | 15380 | unknown                      |
| 0x5C | `64C8` | 25800 | unknown                      |
| 0x5D | `463C` | 17980 | unknown                      |
| 0x5E | `5009` | 20489 | unknown                      |
| 0x5F | `0000` | 0     | unknown                      |

### RT:6000 — Page 6 (EEPROM 0x60–0x6F)

```
rt:000000000000000000000000000000000000000000002578080A28020C020000
```

| Word | Hex    | Dec   | Content                      |
| ---- | ------ | ----- | ---------------------------- |
| 0x60–0x6A | `0000` | 0  | all zero                     |
| 0x6B | `2578` | 9592  | unknown                      |
| 0x6C | `080A` | 2058  | unknown                      |
| 0x6D | `2802` | 10242 | unknown                      |
| 0x6E | `0C02` | 3074  | unknown                      |
| 0x6F | `0000` | 0     | unknown                      |

### RT:7000 — Page 7 (EEPROM 0x70–0x7F)

```
rt:019017D40606017C101D0000057604B002BC02260228020631AA35DB3300AA00
```

| Word | Hex    | Dec   | Content                      |
| ---- | ------ | ----- | ---------------------------- |
| 0x70 | `0190` | 400   | unknown                      |
| 0x71 | `17D4` | 6100  | unknown                      |
| 0x72 | `0606` | 1542  | unknown                      |
| 0x73 | `017C` | 380   | unknown                      |
| 0x74 | `101D` | 4125  | unknown                      |
| 0x75 | `0000` | 0     | unknown                      |
| 0x76 | `0576` | 1398  | unknown                      |
| 0x77 | `04B0` | 1200  | unknown                      |
| 0x78 | `02BC` | 700   | unknown                      |
| 0x79 | `0226` | 550   | unknown                      |
| 0x7A | `0228` | 552   | unknown                      |
| 0x7B | `0206` | 518   | unknown                      |
| 0x7C | `31AA` | 12714 | unknown                      |
| 0x7D | `35DB` | 13787 | unknown                      |
| 0x7E | `3300` | 13056 | unknown                      |
| 0x7F | `AA00` | 43520 | unknown                      |

### RT:8000 — Page 8 (EEPROM 0x80–0x8F)

```
rt:1E1400011E0A780306403623003005FF0301141E0320044C0FA0000203E88000
```

| Word | Hex    | Dec   | Content                      |
| ---- | ------ | ----- | ---------------------------- |
| 0x80 | `1E14` | 7700  | unknown                      |
| 0x81 | `0001` | 1     | unknown                      |
| 0x82 | `1E0A` | 7690  | unknown                      |
| 0x83 | `7803` | 30723 | unknown                      |
| 0x84 | `0640` | 1600  | unknown                      |
| 0x85 | `3623` | 13859 | unknown                      |
| 0x86 | `0030` | 48    | unknown                      |
| 0x87 | `05FF` | 1535  | unknown                      |
| 0x88 | `0301` | 769   | unknown                      |
| 0x89 | `141E` | 5150  | unknown (sporadic timeout)   |
| 0x8A | `0320` | 800   | unknown                      |
| 0x8B | `044C` | 1100  | unknown                      |
| 0x8C | `0FA0` | 4000  | unknown                      |
| 0x8D | `0002` | 2     | unknown                      |
| 0x8E | `03E8` | 1000  | unknown                      |
| 0x8F | `8000` | 32768 | unknown                      |

### RT:9000 — Page 9 (EEPROM 0x90–0x9F)

```
rt:0C00047E05AA6E000DAC1194028A00083E661F5D037700000000000000000000
```

| Word | Hex    | Dec   | Content                      |
| ---- | ------ | ----- | ---------------------------- |
| 0x90 | `0C00` | 3072  | unknown                      |
| 0x91 | `047E` | 1150  | unknown                      |
| 0x92 | `05AA` | 1450  | unknown                      |
| 0x93 | `6E00` | 28160 | unknown                      |
| 0x94 | `0DAC` | 3500  | unknown                      |
| 0x95 | `1194` | 4500  | unknown                      |
| 0x96 | `028A` | 650   | unknown                      |
| 0x97 | `0008` | 8     | unknown                      |
| 0x98 | `3E66` | 15974 | unknown                      |
| 0x99 | `1F5D` | 8029  | unknown (sporadic timeout)   |
| 0x9A | `0377` | 887   | unknown                      |
| 0x9B–0x9F | `0000` | 0 | all zero                    |

### RT:A000+ — Pages 10+ (EEPROM 0xA0+)

All zeros — end of EEPROM.

---

## RR: Full RAM Dump (at 240ml — RR:06/0B/0C/13 fluctuate)

| Register | Value  | Dec   | Notes                        |
| -------- | ------ | ----- | ---------------------------- |
| RR:00    | `0002` | 2     | unknown                      |
| RR:01    | `0202` | 514   | fluctuates between readings  |
| RR:02    | `1200` | 4608  | fluctuates between readings  |
| RR:03    | `0004` | 4     | Operating temperature (ready)|
| RR:04    | `0421` | 1057  | Heater status                |
| RR:05    | `2100` | 8448  | Big-endian of RR:04 low byte |
| RR:06    | `0006` | 6     | Motor/actuator cycle         |
| RR:07    | `0000` | 0     | unknown                      |
| RR:08    | `041C` | 1052  | fluctuates                   |
| RR:09    | `1C00` | 7168  | Big-endian copy?             |
| RR:0A    | `0011` | 17    | unknown                      |
| RR:0B    | `1102` | 4354  | fluctuates                   |
| RR:0C    | `0100` | 256   | fluctuates                   |
| RR:0D    | `0000` | 0     | unknown                      |
| RR:0E    | `0001` | 1     | unknown                      |
| RR:0F    | `0100` | 256   | unknown                      |
| RR:10    | `0000` | 0     | unknown                      |
| RR:11    | `0000` | 0     | unknown                      |
| RR:12    | varies | —     | sporadic timeout             |
| RR:13    | `0100` | 256   | fluctuates                   |
| RR:14    | `0000` | 0     | unknown                      |
| RR:15    | `0000` | 0     | unknown                      |
| RR:16    | `0000` | 0     | unknown                      |
| RR:17    | `0000` | 0     | unknown                      |
| RR:18    | `0037` | 55    | unknown (NOT temperature)    |
| RR:19    | `3700` | 14080 | Big-endian copy of RR:18     |
| RR:1A    | `0010` | 16    | unknown                      |
| RR:1B    | `1000` | 4096  | Big-endian copy of RR:1A     |
| RR:1C    | `0009` | 9     | unknown                      |
| RR:1D    | `0900` | 2304  | Big-endian copy of RR:1C     |
| RR:1E    | `001E` | 30    | unknown                      |
| RR:1F    | varies | —     | fluctuates                   |
| RR:20    | varies | —     | fluctuates                   |
| RR:21    | varies | —     | Brew group position?         |
| RR:22    | varies | —     | fluctuates                   |
| RR:23    | varies | —     | Big-endian copy of RR:22?    |

---

## Address Space Summary

| Command     | Address range | Words | Content                                          |
| ----------- | ------------- | ----- | ------------------------------------------------ |
| `RT:0000`   | 0x00–0x0F     | 16    | Counters (coffee, rinse, clean, descale)         |
| `RT:1000`   | 0x10–0x1F     | 16    | Extended counters / config                       |
| `RT:2000`   | 0x20–0x2F     | 16    | Configuration?                                   |
| `RT:3000`   | 0x30–0x3F     | 16    | Configuration?                                   |
| `RT:4000`   | 0x40–0x4F     | 16    | Timing parameters?                               |
| `RT:5000`   | 0x50–0x5F     | 16    | Timing parameters?                               |
| `RT:6000`   | 0x60–0x6F     | 16    | Mostly zero, some values at 0x6B–0x6E            |
| `RT:7000`   | 0x70–0x7F     | 16    | Configuration / calibration?                     |
| `RT:8000`   | 0x80–0x8F     | 16    | Configuration?                                   |
| `RT:9000`   | 0x90–0x9F     | 16    | Configuration? Zero from 0x9B                    |
| `RT:A000+`  | 0xA0+         | —     | All zeros (end of EEPROM)                        |
| `RE:XX`     | 0x00–0x9F     | 160   | Same data as RT: pages, individual word access   |
| `RR:XX`     | 0x00–0x23+    | 36+   | RAM registers (volatile, some beyond 0x23)       |

Total readable EEPROM: **160 words** (320 bytes) across 10 pages.
