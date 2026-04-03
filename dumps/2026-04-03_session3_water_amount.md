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

## RR: Full RAM Dump (0x00–0xFF, non-zero values only)

RR: address space extends to 0xFF (256 registers). Many odd-numbered registers
are big-endian copies of the previous even register. ~160 non-zero registers found.
Values from 0x24+ often mirror EEPROM content (e.g. RR:A0=`000F` matches RE:40).

### RR:00–RR:23 (known range, at 240ml)

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
| RR:09    | `1C00` | 7168  | Big-endian copy              |
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
| RR:14–17 | `0000` | 0     | all zero                     |
| RR:18    | `0037` | 55    | unknown (NOT temperature)    |
| RR:19    | `3700` | 14080 | Big-endian copy of RR:18     |
| RR:1A    | `0010` | 16    | unknown                      |
| RR:1B    | `1000` | 4096  | Big-endian copy of RR:1A     |
| RR:1C    | `0009` | 9     | unknown                      |
| RR:1D    | `0900` | 2304  | Big-endian copy of RR:1C     |

### RR:24–RR:FF (extended range, non-zero only)

| Register | Value  | Dec   | Notes                        |
| -------- | ------ | ----- | ---------------------------- |
| RR:29    | `0003` | 3     | (responded as re: not rr:)   |
| RR:2A    | `00FF` | 255   |                              |
| RR:2B    | `FFFF` | 65535 |                              |
| RR:2C    | `FF00` | 65280 | Big-endian copy              |
| RR:31    | `0006` | 6     |                              |
| RR:32    | `8700` | 34560 |                              |
| RR:34    | `0002` | 2     |                              |
| RR:35    | `0202` | 514   |                              |
| RR:36    | `0203` | 515   |                              |
| RR:37    | `0200` | 512   |                              |
| RR:38    | `0008` | 8     |                              |
| RR:39    | `0801` | 2049  |                              |
| RR:3A    | `0105` | 261   |                              |
| RR:3B    | `053E` | 1342  |                              |
| RR:3C    | `3E66` | 15974 |                              |
| RR:3D    | `661F` | 26143 |                              |
| RR:3E    | `1F5D` | 8029  |                              |
| RR:3F    | `5D6F` | 23919 |                              |
| RR:40    | `6F7F` | 28543 |                              |
| RR:41    | `8200` | 33280 |                              |
| RR:43    | `0001` | 1     |                              |
| RR:44    | `0121` | 289   |                              |
| RR:45    | `2100` | 8448  |                              |
| RR:46    | `003C` | 60    |                              |
| RR:47    | `3C00` | 15360 | Big-endian copy              |
| RR:4A    | `004F` | 79    |                              |
| RR:4B    | `4F00` | 20224 | Big-endian copy              |
| RR:4C    | `0003` | 3     |                              |
| RR:4D    | `0302` | 770   |                              |
| RR:4E    | `0239` | 569   |                              |
| RR:4F    | `3900` | 14592 | Big-endian copy              |
| RR:51    | `0004` | 4     |                              |
| RR:52    | `0404` | 1028  |                              |
| RR:53    | `0401` | 1025  |                              |
| RR:54    | `0130` | 304   |                              |
| RR:55    | `3004` | 12292 |                              |
| RR:56    | `0001` | 1     |                              |
| RR:57    | `0100` | 256   |                              |
| RR:59    | `0003` | 3     |                              |
| RR:5B    | `0008` | 8     |                              |
| RR:5C    | `0804` | 2052  |                              |
| RR:5D    | `0400` | 1024  |                              |
| RR:5F    | `0002` | 2     |                              |
| RR:60    | `028A` | 650   |                              |
| RR:61    | `8A00` | 35328 | Big-endian copy              |
| RR:62    | `00EC` | 236   |                              |
| RR:63    | `EC00` | 60416 | Big-endian copy              |
| RR:64    | `0029` | 41    |                              |
| RR:65    | `0908` | 2312  |                              |
| RR:6B    | `00A1` | 161   |                              |
| RR:6C    | `A100` | 41216 | Big-endian copy              |
| RR:6F    | `0023` | 35    |                              |
| RR:70    | `2300` | 8960  | Big-endian copy              |
| RR:72    | `0001` | 1     |                              |
| RR:73    | `0130` | 304   |                              |
| RR:74    | `3036` | 12342 |                              |
| RR:75    | `3600` | 13824 |                              |
| RR:76    | `0051` | 81    |                              |
| RR:77    | `5100` | 20736 | Big-endian copy              |
| RR:7F    | `00FF` | 255   |                              |
| RR:80    | `009D` | 157   |                              |
| RR:81    | `9D00` | 40192 | Big-endian copy              |
| RR:82    | `006E` | 110   |                              |
| RR:83    | `6E00` | 28160 | Big-endian copy              |
| RR:84    | `00B4` | 180   |                              |
| RR:85    | `B400` | 46080 | Big-endian copy              |
| RR:89    | `00FD` | 253   |                              |
| RR:8A    | `FD07` | 64775 |                              |
| RR:8B    | `0700` | 1792  |                              |
| RR:8F    | `0006` | 6     |                              |
| RR:90    | `06B5` | 1717  |                              |
| RR:91    | `B405` | 46085 |                              |
| RR:92    | `0601` | 1537  |                              |
| RR:93    | `03F2` | 1010  |                              |
| RR:94    | `F230` | 62000 |                              |
| RR:95    | `309E` | 12446 |                              |
| RR:96    | `DEB0` | 57008 |                              |
| RR:97    | `B01E` | 45086 |                              |
| RR:98    | `1E00` | 7680  |                              |
| RR:9B    | `0021` | 33    |                              |
| RR:9D    | `C000` | 49152 |                              |
| RR:A0    | `000F` | 15    | matches RE:40                |
| RR:A1    | `0F00` | 3840  | Big-endian copy              |
| RR:A2    | `00E6` | 230   | matches RE:41                |
| RR:A3    | `E601` | 58881 |                              |
| RR:A4    | `010A` | 266   | matches RE:42                |
| RR:A5    | `0A02` | 2562  |                              |
| RR:A6    | `0226` | 550   | matches RE:43                |
| RR:A7    | `2602` | 9730  |                              |
| RR:A8    | `0222` | 546   | matches RE:44                |
| RR:A9    | `221B` | 8731  |                              |
| RR:AA    | `1B58` | 7000  | matches RE:48                |
| RR:AB    | `5809` | 22537 |                              |
| RR:AC    | `09C4` | 2500  | matches RE:47                |
| RR:AD    | `C40A` | 50186 |                              |
| RR:AE    | `0AF0` | 2800  | matches RE:46                |
| RR:AF    | `F006` | 61446 |                              |
| RR:B0    | `060E` | 1550  | matches RE:45                |
| RR:B1    | `0E32` | 3634  |                              |
| RR:B2    | `320A` | 12810 | matches RE:4B                |
| RR:B3    | `0A04` | 2564  |                              |
| RR:B4    | `04B0` | 1200  | matches RE:4A                |
| RR:B5    | `B000` | 45056 |                              |
| RR:B8    | `0091` | 145   |                              |
| RR:B9    | `9100` | 37120 | Big-endian copy              |
| RR:BD    | `001F` | 31    |                              |
| RR:BE    | `1F40` | 8000  |                              |
| RR:BF    | `400C` | 16396 |                              |
| RR:C0    | `0C0E` | 3086  | matches RE:2A                |
| RR:C1    | `0E0C` | 3596  |                              |
| RR:C2    | `0C14` | 3092  | matches RE:2B                |
| RR:C3    | `1414` | 5140  |                              |
| RR:C4    | `1404` | 5124  | matches RE:26                |
| RR:C5    | `0400` | 1024  |                              |
| RR:C6    | `00C8` | 200   | matches RE:27                |
| RR:C7    | `C800` | 51200 | Big-endian copy              |
| RR:C8    | `0050` | 80    |                              |
| RR:C9    | `5004` | 20484 | matches RE:25                |
| RR:CA    | `04B0` | 1200  |                              |
| RR:CB    | `B002` | 45058 |                              |
| RR:CC    | `02BC` | 700   | matches RE:78                |
| RR:CD    | `BC02` | 48130 |                              |
| RR:CE    | `0226` | 550   | matches RE:79                |
| RR:CF    | `263C` | 9788  |                              |
| RR:D0    | `3C14` | 15380 |                              |
| RR:D1    | `1410` | 5136  |                              |
| RR:D2    | `1002` | 4098  |                              |
| RR:D3    | `0208` | 520   |                              |
| RR:D4    | `0824` | 2084  | matches RE:3A                |
| RR:D5    | `2428` | 9256  |                              |
| RR:D6    | `2850` | 10320 | matches RE:3B                |
| RR:D7    | `5012` | 20498 |                              |
| RR:D8    | `1225` | 4645  | matches RE:50                |
| RR:D9    | `2517` | 9495  |                              |
| RR:DA    | `1712` | 5906  | matches RE:51                |
| RR:DB    | `120A` | 4618  |                              |
| RR:DC    | `0A05` | 2565  | matches RE:54                |
| RR:DD    | `0502` | 1282  |                              |
| RR:DE    | `0228` | 552   |                              |
| RR:DF    | `2806` | 10246 |                              |
| RR:E0    | `0606` | 1542  | matches RE:72                |
| RR:E1    | `0600` | 1536  |                              |
| RR:E4    | `0004` | 4     |                              |
| RR:E5    | `0406` | 1030  |                              |
| RR:E6    | `060E` | 1550  | matches RE:4D                |
| RR:E7    | `0E03` | 3587  |                              |
| RR:E8    | `03B6` | 950   | matches RE:4C                |
| RR:E9    | `B617` | 46615 |                              |
| RR:EA    | `17D4` | 6100  | matches RE:71                |
| RR:EB    | `D401` | 54273 |                              |
| RR:EC    | `0190` | 400   | matches RE:70                |
| RR:ED    | `9000` | 36864 | Big-endian copy              |
| RR:EF    | `0008` | 8     |                              |
| RR:F0    | `080A` | 2058  |                              |
| RR:F1    | `0A0A` | 2570  |                              |
| RR:F2    | `0A00` | 2560  |                              |
| RR:F5    | `0003` | 3     |                              |
| RR:F6    | `0320` | 800   | matches RE:37/RE:8A          |
| RR:F7    | `2000` | 8192  |                              |
| RR:F8    | `0003` | 3     |                              |
| RR:F9    | `0300` | 768   |                              |
| RR:FB    | `0016` | 22    |                              |
| RR:FC    | `1678` | 5752  | matches RE:0A                |
| RR:FD    | `7850` | 30800 |                              |
| RR:FE    | `5004` | 20484 |                              |
| RR:FF    | `0400` | 1024  |                              |
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
