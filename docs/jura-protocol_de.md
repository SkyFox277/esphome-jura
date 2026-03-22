# Jura Impressa F50 — Protokoll & Schnittstellendokumentation

Stand: 2026-03-22

---

## Quellen

| Quelle                                                                                              | Inhalt                                                |
| --------------------------------------------------------------------------------------------------- | ----------------------------------------------------- |
| http://protocoljura.wiki-site.com/index.php/Hauptseite                                              | Original-Protokolldokumentation (Toptronic V1)        |
| https://github.com/ryanalden/esphome-jura-component                                                | ESPHome Custom Component (Basis dieser Implementierung)|
| https://github.com/Jutta-Proto/protocol-cpp                                                         | C++ Protokoll-Implementierung, Protokoll V2 Analyse   |
| https://github.com/COM8/esp32-jura                                                                  | ESP32 Implementierung (Bluetooth/XMPP, neuere Modelle)|
| https://github.com/oliverk71/Coffeemaker-Payment-System                                             | Arduino RFID Payment (Quelle vieler Befehls-Listen)   |
| https://github.com/sklas/CofFi                                                                      | ESP8266 MQTT Implementierung                          |
| https://github.com/PromyLOPh/juramote                                                               | Frühe Python-Implementierung                          |
| https://community.home-assistant.io/t/control-your-jura-coffee-machine/26604                       | HA Community Thread mit Verdrahtungsfotos             |
| https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/                                       | IC: Bit-Mapping Quelle (ACHTUNG: andere Maschine!)    |
| `Serial interfaces - Protocoljura.pdf`                                                              | Pin-Belegungen aller Jura-Schnittstellen (lokal)      |
| `Commands for coffeemaker - Protocoljura.pdf`                                                       | Vollständige Befehlsliste (lokal)                     |
| `Erweiterte Befehle X und S-Reihe.xlsx`                                                             | Erweiterte Befehle für X- und S-Reihe (lokal)         |

---

## Hardware: Jura Impressa F50

| Eigenschaft       | Wert                              |
| ----------------- | --------------------------------- |
| Modell            | Jura Impressa F50                 |
| Firmware-ID       | `EF532M V02.03` (via `TY:`)       |
| Service-Interface | 7-polig (Toptronic)               |
| Mikrocontroller   | D1 Mini (ESP8266, Wemos)          |
| Logiklevel        | 3.3V (ESP) ↔ 5V (Jura) — Level Shifter nötig! |
| UART Baudrate     | 9600 bps                          |

---

## Schnittstellen-Pinbelegung

### 7-Pin Interface (F50 und viele andere Modelle)

```
(8)  7    6    5    4    3    2    1   (0)
 nc  nc  +5V   nc  RxD  GND  TxD   nc   nc
```

Nummerierung von rechts (Stecker-Ansicht von vorne):

| Pin | Signal | Beschreibung                        |
| --- | ------ | ----------------------------------- |
| 1   | nc     | nicht belegt                        |
| 2   | TxD    | Maschine sendet (→ ESP RX)          |
| 3   | GND    | Masse                               |
| 4   | RxD    | Maschine empfängt (← ESP TX)        |
| 5   | nc     | nicht belegt                        |
| 6   | +5V    | Versorgung (kann ESP speisen)       |
| 7   | nc     | nicht belegt                        |

> **Achtung:** Jura-Interface arbeitet mit 5V TTL. D1 Mini mit 3.3V.
> Level Shifter (bidirektional, z.B. TXS0108E oder BSS138-basiert) zwingend erforderlich.
> Alternativ: 1kΩ + 2kΩ Spannungsteiler auf TX-Leitung (ESP→Jura), Zener-Diode auf RX.

### 4-Pin Interface (z.B. Impressa S95)

```
4    3    2    1
+5V  RxD  GND  TxD
```

### 5-Pin Interface

```
5    4    3    2    1
+5V  nc   RxD  GND  TxD
```

### 9-Pin Interface (z.B. X7)

```
1    2    3    4    5    6    7    8    9
TxD  GND  RxD  +5V  nc   nc   nc   nc   nc
```

---

## Serielle Konfiguration

| Parameter    | Wert                 |
| ------------ | -------------------- |
| Baudrate     | 9600                 |
| Datenbits    | 8                    |
| Parität      | keine                |
| Stopbits     | 1                    |
| Flusskontrolle | keine              |
| Logiklevel   | 5V TTL               |

---

## Jura Toptronic Protokoll (V1)

### Übersicht

Das Jura-Protokoll "verschleiert" ASCII-Zeichen durch Spreizung auf 4 UART-Bytes.
Jedes ASCII-Byte wird in 4 UART-Bytes kodiert, die je 2 Bits tragen.

Alle Befehle und Antworten enden mit `\r\n`.
Befehle werden in GROSSBUCHSTABEN gesendet, Antworten kommen in kleinbuchstaben zurück.

### Kodierung (ASCII → 4 UART-Bytes)

Für jedes ASCII-Zeichen werden 4 UART-Bytes gesendet:

```
UART-Byte 0: Bit2 = ASCII[0], Bit5 = ASCII[1], alle anderen Bits = 1
UART-Byte 1: Bit2 = ASCII[2], Bit5 = ASCII[3], alle anderen Bits = 1
UART-Byte 2: Bit2 = ASCII[4], Bit5 = ASCII[5], alle anderen Bits = 1
UART-Byte 3: Bit2 = ASCII[6], Bit5 = ASCII[7], alle anderen Bits = 1
```

C++ Implementierung:
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

### Beispiel: Kodierung von `AN:01\r\n`

| Zeichen | ASCII | UART Bytes (hex)         |
| ------- | ----- | ------------------------ |
| `A`     | 0x41  | `DF DB DB DF`            |
| `N`     | 0x4E  | `FB FF DB DF`            |
| `:`     | 0x3A  | `FB FB FF DB`            |
| `0`     | 0x30  | `DB DB FF DB`            |
| `1`     | 0x31  | `DF DB FF DB`            |
| `\r`    | 0x0D  | `DF FF DB DB`            |
| `\n`    | 0x0A  | `FB FB DB DB`            |

### Timing

```
[ASCII Byte]
  → UART Byte 0
  → UART Byte 1
  → UART Byte 2
  → UART Byte 3
  → delay(8ms)         ← nach ALLEN 4 Bytes des Zeichens
[nächstes ASCII Byte]
  ...
```

> Hinweis: Einige Quellen beschreiben 8ms ZWISCHEN den 4 UART-Bytes (delay nach jedem).
> Unsere Implementierung (delay nach allen 4) funktioniert in der Praxis korrekt.

### Protokoll V2 (neuere Modelle — NICHT F50)

Neuere Jura-Modelle (E6 2019+, ENA, etc.) verwenden ein erweitertes Protokoll mit
Key-Exchange-Handshake:

```
[Client]: @T1\r\n
[Jura]:   @t1\r\n
[Jura]:   @T2:010001B228\r\n
[Client]: @t2:8120000000\r\n
[Jura]:   @T3:3BDEEF532M V02.03\r\n
[Client]: @t3\r\n
```

Die F50 verwendet **V1** (kein `@`-Handshake). Die Jura Smart Connect Bluetooth-Brücke
spricht V2 mit modernen Modellen.

---

## Befehlsreferenz — Impressa F50

### Maschinen-Steuerung

| Befehl   | Antwort | Beschreibung                                | F50 getestet |
| -------- | ------- | ------------------------------------------- | ------------ |
| `AN:01`  | `ok:`   | Einschalten                                 | ✅            |
| `AN:02`  | `ok:`   | Ausschalten                                 | ✅            |
| `AN:0A`  | ?       | EEPROM löschen — **NIEMALS VERWENDEN!**     | ❌ nein       |
| `AN:20`  | `ok:`   | Test-Modus ein                              | —            |
| `AN:21`  | `ok:`   | Test-Modus aus                              | —            |

### Produkte (FA: Befehle)

| Befehl   | F50-Funktion        | Andere Modelle             | F50 getestet |
| -------- | ------------------- | -------------------------- | ------------ |
| `FA:01`  | —                   | Produkt 1 / Espresso       | —            |
| `FA:02`  | Spülen (Rinse)      | Produkt 2 / Aufwärmen      | ✅            |
| `FA:03`  | —                   | Produkt 3                  | —            |
| `FA:04`  | —                   | Taste 1 (links oben)       | —            |
| `FA:05`  | —                   | Taste 2                    | —            |
| `FA:06`  | Kaffee              | Kaffee / Taste 3           | ✅            |
| `FA:07`  | Doppelkaffee        | Doppelkaffee / Taste 4     | ✅            |
| `FA:08`  | —                   | Heißwasser                 | —            |
| `FA:09`  | —                   | Dampf                      | —            |
| `FA:0A`  | —                   | —                          | —            |
| `FA:0B`  | Spülen (Wasser-ML)  | Menü verlassen (J6)        | —            |

### Display

| Befehl          | Beschreibung                                            |
| --------------- | ------------------------------------------------------- |
| `DA:XXXXXXXXXXXX` | Text auf Display anzeigen (genau 12 Zeichen, GROSSBUCHSTABEN, keine Sonderzeichen) |
| `DR:`           | Display löschen                                         |

### Maschinen-Info

| Befehl  | Antwort-Beispiel          | Beschreibung               |
| ------- | ------------------------- | -------------------------- |
| `TY:`   | `ty:EF532M V02.03`        | Maschinentyp und Firmware  |

### Low-Level Steuerung (FN: Befehle)

> ⚠️ Achtung: Diese Befehle steuern einzelne Aktoren direkt.
> Falsche Kombination kann die Maschine beschädigen oder Kaffee produzieren ohne Spülung!

| Befehl   | Funktion                                       |
| -------- | ---------------------------------------------- |
| `FN:01`  | Kaffee-Pumpe ein                               |
| `FN:02`  | Kaffee-Pumpe aus                               |
| `FN:03`  | Kaffee-Heizung ein                             |
| `FN:04`  | Kaffee-Heizung aus                             |
| `FN:05`  | Dampf-Heizung ein                              |
| `FN:06`  | Dampf-Heizung aus                              |
| `FN:07`  | Mühle links (dauerhaft) ein                    |
| `FN:08`  | Mühle links aus                                |
| `FN:09`  | Mühle rechts ein / Brühgruppe (?)              |
| `FN:0A`  | Mühle rechts aus / Brühgruppe (?)              |
| `FN:0B`  | Dampf-Pumpe / Kaffee-Presse ein                |
| `FN:0C`  | Dampf-Pumpe / Kaffee-Presse aus                |
| `FN:0D`  | Brühgruppe initialisieren (Reset)              |
| `FN:0E`  | Brühgruppe in Trester-Auswerf-Position         |
| `FN:0F`  | Brühgruppe in Mahlposition                     |
| `FN:11`  | Brühgruppe (?)                                 |
| `FN:12`  | Brühgruppe (?)                                 |
| `FN:13`  | Brühgruppe in Brühposition (F50)               |
| `FN:1D`  | Ablassventil ein                               |
| `FN:1E`  | Ablassventil aus                               |
| `FN:22`  | Brühgruppe in Brühposition (E6/andere)         |
| `FN:24`  | Ablassventil / Entleerung ein                  |
| `FN:25`  | Ablassventil / Entleerung aus                  |
| `FN:26`  | Dampfventil ein                                |
| `FN:27`  | Dampfventil aus                                |
| `FN:28`  | Cappuccino-Ventil ein                          |
| `FN:29`  | Cappuccino-Ventil aus                          |
| `FN:51`  | Ausschalten (alternativ zu AN:02)              |
| `FN:8A`  | Debug-Modus ein (Maschine sendet `ku:`/`Ku:` Takt-Bytes) |

### Kaffee-Brüh-Sequenz (manuell via FN:, F50)

```
FN:07          # Mühle ein
delay(3000)    # Mahlzeit → bestimmt Kaffeestärke (3s = Standard)
FN:08          # Mühle aus
FN:13          # Brühgruppe in Brühposition  (F50: FN:13, E6: FN:22)
FN:0B          # Kaffee-Presse ein
delay(500)     # Kaffeemehl komprimieren
FN:0C          # Kaffee-Presse aus
FN:03          # Heizung ein
FN:01          # Pumpe ein
delay(2000)    # Vorlauf (~initialer Wasserstoß)
FN:02          # Pumpe aus
FN:04          # Heizung aus
delay(2000)    # Wasser verteilen
FN:03          # Heizung ein
FN:01          # Pumpe ein
delay(40000)   # 40s Brühen → ~200ml Kaffee
FN:02          # Pumpe aus
FN:04          # Heizung aus
FN:0D          # Brühgruppe zurücksetzen + Trester auswerfen
```

---

## Status-Abfragen

### IC: — Eingänge lesen (Sensor-Status)

Befehl: `IC:`
Antwort: `ic:XXYYZZ00` (Hex-String, mehrere Bytes)

#### Byte 0 — bekanntes Bit-Mapping (F50)

| Bit | Wert 1 bedeutet         | Wert 0 bedeutet       | Status für F50       |
| --- | ----------------------- | --------------------- | -------------------- |
| 0   | Reinigung nötig         | OK                    | ✅ bestätigt          |
| 1   | ?                       | ?                     | unbekannt            |
| 2   | ?                       | ?                     | unbekannt            |
| 3   | ?                       | ?                     | unbekannt            |
| 4   | Schale eingesetzt       | Schale fehlt          | ✅ bestätigt (invertiert vs. Referenzcode!) |
| 5   | Tank leer               | Tank OK               | ⚠️ unbestätigt für F50 |
| 6   | ?                       | ?                     | unbekannt            |
| 7   | ?                       | ?                     | unbekannt            |

> **Wichtig:** Die Bedeutung von Bit 4 (Schale) ist beim F50 INVERTIERT
> gegenüber dem häufig zitierten Referenzcode (Instructables-Artikel).
> Bit 4 = 1 → Schale VORHANDEN (nicht fehlend).
> Unsere Implementierung: `tray_missing = !((val >> 4) & 1)` ← korrekt für F50.

Beispiel-Antwort beim Betrieb: `ic:DFB01E00`
- Byte 0 = `0xDF` = `11011111`
- Bit 4 = 1 → Schale vorhanden ✓
- Bit 5 = 0 → Tank OK ✓
- Bit 0 = 1 → Reinigung nötig?

> **TODO:** Bits 1–3 und 6–7 durch systematische Beobachtung beim Aufheizen bestimmen.
> Ziel: "Bereit"-Bit für Startup-Sequenz Phase B identifizieren.

### RT:0000 — EEPROM-Zähler lesen

Befehl: `RT:0000`
Antwort: `rt:XXXXXXXXXXXXXXXX...` (min. 35 Hex-Zeichen)

#### Bekannte Offsets (für F50 bestätigt)

| Offset | Länge | Inhalt            | Hex-Wert-Beispiel | Bemerkung        |
| ------ | ----- | ----------------- | ----------------- | ---------------- |
| 3      | 4     | Single Espresso   | `0000`            | F50: immer 0 (kein Espresso) |
| 7      | 4     | Double Espresso   | `0000`            | F50: immer 0     |
| 11     | 4     | Kaffee            | `0042`            | = 66 Kaffees     |
| 15     | 4     | Doppelkaffee      | `0018`            | = 24             |
| 19     | 4     | Counter 1         | ?                 | Bedeutung unbekannt |
| 23     | 4     | Counter 2         | ?                 | Bedeutung unbekannt |
| 27     | 4     | Counter 3         | ?                 | Bedeutung unbekannt |
| 31     | 4     | Reinigungen 1     | `0008`            | = 8 Reinigungen  |
| 35     | 4     | Counter 4         | ?                 | Bedeutung unbekannt |
| 39     | 4     | Counter 5         | ?                 | Bedeutung unbekannt |
| 43     | 4     | Counter 6         | ?                 | Bedeutung unbekannt |
| 47     | 4     | Counter 7         | ?                 | Bedeutung unbekannt |
| 51     | 4     | Counter 8         | ?                 | Bedeutung unbekannt |
| 55     | 4     | Reinigungen 2 (?) | ?                 | Aus Referenzcode, unbestätigt |
| 59     | 4     | Counter 9         | ?                 | Bedeutung unbekannt |
| 63     | 4     | Counter 10        | ?                 | Bedeutung unbekannt |

> **TODO:** Zählerwerte durch Nutzungsbeobachtung bestimmen.
> Kaffee brühen → Counter nach 5 min ablesen → Offset identifizieren.

### RR: — RAM lesen (Debug)

Befehl: `RR:XX` (XX = hex Adresse, z.B. `RR:00` bis `RR:23`)
Antwort: `rr:YYYY` (16-bit Hex-Wert)

#### Beobachtungen beim F50 (Startup-Sequenz)

Aufgezeichnet während: Aus → Schale fehlt → Aufheizen → Spülen → Bereit

| Register | Aus    | Schale fehlt | Aufheizen | Spülen | Bereit | Interpretation        |
| -------- | ------ | ------------ | --------- | ------ | ------ | --------------------- |
| RR:03    | `0000` | `0000`       | `0004`    | `0004` | `0004` | Heizung aktiv (bit 2) |
| RR:04    | `0029` | `0029`       | `0429`    | `0429` | `0429` | Heizstatus (?)        |
| RR:18    | `003B` | `003B`       | —         | —      | `003C` | Temperatur? (0x3B=59, 0x3C=60°C) |
| RR:19    | `3B00` | `3B00`       | —         | —      | `3C00` | Big-Endian Kopie von RR:18 |
| RR:21    | `0024` | `0024`       | variiert  | `0013` | `0034` | Brühgruppen-Position? |
| RR:22    | `242E` | `242E`       | variiert  | `105E` | `316C` | Aufheizfortschritt?   |
| RR:23    | `2E00` | `2E00`       | variiert  | `5E00` | `6C00` | Big-Endian Kopie?     |

**Hypothesen (noch unbestätigt):**
- `RR:03` Bit 2 = Heizung gerade aktiv
- `RR:18/19` = aktuelle Temperatur in °C (0x3B = 59°C, 0x3C = 60°C → Solltemperatur ~60°C)
- Temperaturanstieg in RR:18 könnte für Startup-Sequenz Phase B genutzt werden

> **TODO für Startup Phase B:** Wenn Maschine eingeschaltet, periodisch `RR:18` abfragen.
> Sobald Wert ≥ `003C` (60°C) → Maschine bereit → Spülen starten.
> Alternativ: Änderung in `IC:` Bits 1–3/6–7 verfolgen.

### RE: — EEPROM-Wort lesen

| Befehl  | Antwort-Beispiel | Beschreibung         |
| ------- | ---------------- | -------------------- |
| `RE:31` | `re:XXXX`        | Maschinentyp-Code    |

### WE: — EEPROM-Wort schreiben

| Befehl        | Beschreibung                     |
| ------------- | -------------------------------- |
| `WE:31,2712`  | Maschinentyp setzen              |

> ⚠️ Schreib-Befehle mit äußerster Vorsicht verwenden!

---

## D1 Mini Verdrahtung (aktuelles Setup)

```
D1 Mini (ESP8266)     Level Shifter      Jura 7-Pin
─────────────────     ─────────────      ──────────
D1 (GPIO5) TX    →   LV1 → HV1      →   Pin 4 (RxD)
D2 (GPIO4) RX    ←   LV2 ← HV2      ←   Pin 2 (TxD)
3.3V             →   LV
                      HV              ←   Pin 6 (+5V)
GND              →   GND             ←   Pin 3 (GND)
```

YAML-Konfiguration:
```yaml
uart:
  id: uart_bus
  tx_pin: D1
  rx_pin: D2
  baud_rate: 9600
```

---

## Bekannte Unterschiede zwischen Modellen

| Modell       | Espresso  | FA:06       | FA:07        | FA:02      | Brühpos. |
| ------------ | --------- | ----------- | ------------ | ---------- | -------- |
| F50          | ❌ keiner | Kaffee      | Doppelkaffee | Spülen     | FN:13    |
| J6 / E6      | FA:07     | Kaffee      | Espresso     | Aufwärmen  | FN:22    |
| X7 / Saphira | FA:01     | Kaffee      | —            | FA:02      | FN:13    |

> Die Befehle `FA:XX` sind maschinenspezifisch und entsprechen den physischen Tasten.
> `FN:XX` Low-Level Befehle sind weitgehend modellübergreifend.

---

## Weiterführende Links

| Link                                                                       | Beschreibung                                      |
| -------------------------------------------------------------------------- | ------------------------------------------------- |
| https://uk.jura.com/en/homeproducts/accessories/SmartConnect-Main-72167    | Jura Smart Connect (offizielle Bluetooth-Brücke)  |
| https://github.com/Jutta-Proto/hardware-pi                                 | Raspberry Pi Hardware-Implementierung             |
| http://heise.de/-3058350                                                   | Heise-Artikel: Kaffeemaschinen-Bezahlsystem (DE)  |
| https://github.com/sklas/CofFi                                             | CofFi: vollständige ESP8266 MQTT Implementierung  |
