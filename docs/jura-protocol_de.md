# Jura Impressa F50 — Protokoll & Schnittstellendokumentation

Stand: 2026-04-03

---

## Quellen

| Quelle                                                                                              | Inhalt                                                    |
| --------------------------------------------------------------------------------------------------- | --------------------------------------------------------- |
| http://protocoljura.wiki-site.com/index.php/Hauptseite                                              | Original-Protokolldokumentation (Toptronic)            |
| https://github.com/ryanalden/esphome-jura-component                                                | ESPHome Custom Component (Basis dieser Implementierung)   |
| https://github.com/Jutta-Proto/protocol-cpp                                                         | C++ Protokoll-Implementierung, Protokoll V2 Analyse       |
| https://github.com/COM8/esp32-jura                                                                  | ESP32 Implementierung (Bluetooth/XMPP, neuere Modelle)    |
| https://github.com/oliverk71/Coffeemaker-Payment-System                                             | Arduino RFID Payment (Quelle vieler Befehls-Listen)       |
| https://github.com/sklas/CofFi                                                                      | ESP8266 MQTT Implementierung                              |
| https://github.com/PromyLOPh/juramote                                                               | Frühe Python-Implementierung                              |
| https://community.home-assistant.io/t/control-your-jura-coffee-machine/26604                       | HA Community Thread mit Verdrahtungsfotos                 |
| https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/                                       | IC: Bit-Mapping Quelle (ACHTUNG: andere Maschine!)        |
| `Commands for coffeemaker - Protocoljura.pdf` (pdfs/)                                              | Vollständige Befehlsliste (lokale Referenz)               |
| `Serial interfaces - Protocoljura.pdf` (pdfs/)                                                     | Pin-Belegungen aller Jura-Schnittstellen (lokale Referenz)|
| `Erweiterte Befehle X und S-Reihe.xlsx` (pdfs/)                                                    | Erweiterte Befehle für X- und S-Reihe (lokale Referenz)   |

---

## Hardware: Jura Impressa F50

| Eigenschaft       | Wert                                             |
| ----------------- | ------------------------------------------------ |
| Modell            | Jura Impressa F50                                |
| Firmware-ID       | `EF532M V02.03` (via `TY:`)                      |
| Service-Interface | 7-polig (Toptronic)                              |
| Mikrocontroller   | D1 Mini (ESP8266, Wemos)                         |
| Logiklevel        | 3.3V (ESP) ↔ 5V (Jura) — Level Shifter erforderlich |
| UART Baudrate     | 9600 bps                                         |

---

## Schnittstellen-Pinbelegung

### 7-Pin Interface (F50 und viele andere Modelle)

```
(8)  7    6    5    4    3    2    1   (0)
 nc  nc  +5V   nc  RxD  GND  TxD   nc
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

| Parameter      | Wert   |
| -------------- | ------ |
| Baudrate       | 9600   |
| Datenbits      | 8      |
| Parität        | keine  |
| Stopbits       | 1      |
| Flusskontrolle | keine  |
| Logiklevel     | 5V TTL |

---

## Jura Toptronic Protokoll (V1)

### Übersicht

Das Jura-Protokoll verschleiert ASCII-Zeichen durch Spreizung auf 4 UART-Bytes.
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

| Zeichen | ASCII | UART Bytes (hex) |
| ------- | ----- | ---------------- |
| `A`     | 0x41  | `DF DB DB DF`    |
| `N`     | 0x4E  | `FB FF DB DF`    |
| `:`     | 0x3A  | `FB FB FF DB`    |
| `0`     | 0x30  | `DB DB FF DB`    |
| `1`     | 0x31  | `DF DB FF DB`    |
| `\r`    | 0x0D  | `DF FF DB DB`    |
| `\n`    | 0x0A  | `FB FB DB DB`    |

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

> Hinweis: Einige Quellen beschreiben 8ms zwischen den einzelnen 4 UART-Bytes.
> Die Implementierung (delay nach allen 4) funktioniert in der Praxis korrekt.

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

## Befehlsreferenz

### Maschinen-Steuerung (AN: Befehle)

| Befehl   | Antwort | Beschreibung                                                               |
| -------- | ------- | -------------------------------------------------------------------------- |
| `AN:01`  | `ok:`   | Einschalten / Aufwecken aus Standby                                        |
| `AN:02`  | `ok:`   | Ausschalten (startet Abschalt-Sequenz)                                     |
| `AN:0A`  | ?       | EEPROM löschen — **Von Komponente BLOCKIERT** (irreversibel, kein Nutzen)  |
| `AN:20`  | `ok:`   | Test-Modus ein                                                             |
| `AN:21`  | `ok:`   | Test-Modus aus                                                             |

> **Hinweis zu Zero-Energy-Modellen:** Neuere Maschinen (ENA 7 etc.) verwenden einen
> Hochspannungs-Einrastschalter. `AN:01` allein reicht möglicherweise nicht —
> ein Relais parallel zum physischen Ein/Aus-Schalter kann nötig sein.

### Produkte (FA: Befehle) — nach Modell

`FA:` Befehle simulieren physische Tastendruck und sind **modellspezifisch**.

| Befehl   | F50 ✅       | F7 / S95      | E6 / E8       | J6            | ENA 7             | X7 / Saphira  |
| -------- | ------------ | ------------- | ------------- | ------------- | ----------------- | ------------- |
| `FA:01`  | —            | —             | —             | Aus + Spülen  | —                 | Produkt 1     |
| `FA:02`  | Spülen       | —             | —             | —             | —                 | Produkt 2     |
| `FA:03`  | —            | —             | —             | Dampf         | —                 | Produkt 3     |
| `FA:04`  | —            | Espresso      | Espresso      | Espresso      | Spülen (Aufforder)| Produkt 4     |
| `FA:05`  | —            | 2x Espresso   | Ristretto     | Ristretto     | —                 | Produkt 5     |
| `FA:06`  | Kaffee ✅    | Kaffee        | Heißwasser    | Heißwasser    | —                 | Produkt 6     |
| `FA:07`  | 2x Kaffee ✅ | 2x Kaffee     | Cappuccino    | Espresso      | —                 | Produkt 7     |
| `FA:08`  | —            | Heißwasser    | —             | 2x Espresso   | Dampf             | Heißwasser    |
| `FA:09`  | —            | Dampf         | Kaffee        | Kaffee        | Kaffee (klein)    | Dampf         |
| `FA:0A`  | —            | —             | —             | 2x Kaffee     | Kaffee (groß)     | —             |
| `FA:0B`  | —            | Spülen        | —             | Tassenbeleucht.| Heißwasser       | Spülen        |
| `FA:0C`  | —            | XXL Tasse     | —             | Menü öffnen   | —                 | —             |

> Quellen: F50 auf Hardware getestet. Andere aus Community-Projekten:
> ryanalden/esphome-jura-component (J6), alextrical/Jura-F7-ESPHOME (F7),
> tiaanv/jura (ENA), oliverk71/Coffeemaker-Payment-System (X7/S95).
> Befehle für das eigene Modell mit `TY:`-Antwort abgleichen.

### Display

| Befehl             | Beschreibung                                                              |
| ------------------ | ------------------------------------------------------------------------- |
| `DA:XXXXXXXXXXXX`  | Text auf Display anzeigen (genau 12 Zeichen, GROSSBUCHSTABEN, keine Sonderzeichen) |
| `DR:`              | Display löschen                                                           |

### Maschinen-Info

| Befehl  | Antwort-Beispiel          | Beschreibung                          |
| ------- | ------------------------- | ------------------------------------- |
| `TY:`   | `ty:EF532M V02.03`        | Maschinentyp und Firmware-Version     |
| `TL:`   | `tl:BL_RL78 V01.31`       | Bootloader-Version (E6/E8)            |

Bekannte `TY:` Antworten:

| Modell             | `TY:` Antwort            |
| ------------------ | ------------------------ |
| Impressa F50       | `ty:EF532M V02.03`       |
| E6 2019 / E8 / E65 | `ty:EF532M V02.03`       |
| Impressa J6        | `ty: PIM V01.01`         |

### Low-Level Steuerung (FN: Befehle)

> ⚠️ Achtung: Diese Befehle steuern einzelne Aktoren direkt.
> Falsche Kombinationen können die Maschine beschädigen oder Kaffee ohne Spülung produzieren.

| Befehl   | Funktion                                              |
| -------- | ----------------------------------------------------- |
| `FN:01`  | Kaffee-Pumpe ein                                      |
| `FN:02`  | Kaffee-Pumpe aus                                      |
| `FN:03`  | Kaffee-Heizung ein                                    |
| `FN:04`  | Kaffee-Heizung aus                                    |
| `FN:05`  | Dampf-Heizung ein                                     |
| `FN:06`  | Dampf-Heizung aus                                     |
| `FN:07`  | Mühle links (dauerhaft) ein                           |
| `FN:08`  | Mühle links aus                                       |
| `FN:09`  | Mühle rechts ein / Brühgruppe (?)                     |
| `FN:0A`  | Mühle rechts aus / Brühgruppe (?)                     |
| `FN:0B`  | Dampf-Pumpe / Kaffee-Presse ein                       |
| `FN:0C`  | Dampf-Pumpe / Kaffee-Presse aus                       |
| `FN:0D`  | Brühgruppe initialisieren (Reset)                     |
| `FN:0E`  | Brühgruppe in Trester-Auswerf-Position                |
| `FN:0F`  | Brühgruppe in Mahlposition                            |
| `FN:11`  | Brühgruppe (?)                                        |
| `FN:12`  | Brühgruppe (?)                                        |
| `FN:13`  | Brühgruppe in Brühposition (F50)                      |
| `FN:1D`  | Ablassventil ein                                      |
| `FN:1E`  | Ablassventil aus                                      |
| `FN:22`  | Brühgruppe in Brühposition (E6 / andere Modelle)      |
| `FN:24`  | Ablassventil / Entleerung ein                         |
| `FN:25`  | Ablassventil / Entleerung aus                         |
| `FN:26`  | Dampfventil ein                                       |
| `FN:27`  | Dampfventil aus                                       |
| `FN:28`  | Cappuccino-Ventil ein                                 |
| `FN:29`  | Cappuccino-Ventil aus                                 |
| `FN:51`  | Ausschalten (alternativ zu AN:02)                     |
| `FN:8A`  | Debug-Modus ein (Maschine sendet `ku:`/`Ku:` Takt-Bytes) |

### Manuelle Brüh-Sequenz via FN: (F50)

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

#### Byte 0 — Bit-Belegung nach Modell

IC: Bit-Positionen unterscheiden sich je nach Modell-Familie. Zwei Layouts wurden identifiziert:

**Layout A — F50 (bestätigt durch Hardware-Debug-Sessions 2026-04-03)**

Alle F50-Statusbits verwenden invertierte Logik SOFERN NICHT anders angegeben: **0 = Problem, 1 = OK**.

| Bit | Wert 1 bedeutet              | Wert 0 bedeutet          | Config-Key                 | Logik    | Status            |
| --- | ---------------------------- | ------------------------ | -------------------------- | -------- | ----------------- |
| 0   | Session benutzt seit Kaltstart | Kaltstart-Zustand      | `ic_needs_rinse_bit`       | 1=benutzt | ✅ bestätigt     |
| 1   | Nicht in Reinigungszyklus    | Reinigungszyklus läuft   | `ic_need_clean_inverted`   | invertiert | ✅ bestätigt    |
| 2   | Bereit (nicht beschäftigt)   | Beschäftigt              | —                          | 1=bereit | ⚠️ Hypothese     |
| 3   | Tank OK                      | Tank leer / fehlt        | `ic_tank_inverted`         | invertiert | ✅ bestätigt    |
| 4   | Schale eingesetzt            | Schale fehlt             | `ic_tray_inverted`         | invertiert | ✅ bestätigt    |
| 5   | —                            | —                        | —                          | —        | ✅ immer 0        |
| 6   | —                            | —                        | —                          | —        | ❌ unzuverlässig  |
| 7   | —                            | —                        | —                          | —        | ✅ immer 1        |

Beispiel-Antworten F50:
```
ic:DFB01E00  0xDF = 1101 1111  → Bit1=1, Bit3=1, Bit4=1 → alles OK
ic:D7B01E00  0xD7 = 1101 0111  → Bit3=0 → Tank leer
ic:CFB01E00  0xCF = 1100 1111  → Bit4=0 → Schale fehlt
ic:CEB01E00  0xCE = 1100 1110  → Bit4=0 → Schale fehlt (bestätigt durch physisches Entfernen: 0xDE→0xCE)
ic:D9B01E00  0xD9 = 1101 1001  → Bit1=0, Bit2=0 → Reinigungszyklus aktiv ("Pflege"-Phase läuft)
```

> Bit 0 (Session-Used-Flag): 0 nach Kaltstart, 1 nach erstem Kaffee. Wird
> weder durch manuelles Spülen noch durch Auto-Spülung beim Ausschalten
> oder einen einzelnen Power-Cycle gelöscht — längeres Abkühlen nötig
> (Session 4, 2026-04-18).
> Bit 1 ist 0 während des aktiven Reinigungszyklus, sonst 1. Es ist NICHT
> das "Pflege drücken"-Display-Flag — die Anzeige kann erscheinen während
> Bit 1 = 1 ist, weil die Display-Bedingung woanders sitzt (siehe EEPROM
> 0x0005-Hypothese).
> Bit 5 ist in allen bekannten F50-Messwerten immer 0. Bit 6 oszilliert zwischen Abfragen und ist unzuverlässig. Bit 7 ist immer 1.
>
> Hinweis zur Sensor-Benennung: `ic_needs_rinse_bit` verweist auf Bit 0 und
> der ESPHome-Sensor-Key `needs_rinse` ist irreführend — siehe A4 in TODO.md
> für den geplanten v2.0-Rename (`session_used`).

**Layout B — E6, E8, ENA, J6 (bestätigt aus mehreren Community-Projekten)**

| Bit | Wert 1 bedeutet  | Wert 0 bedeutet | Status          |
| --- | ---------------- | --------------- | --------------- |
| 0   | Schale fehlt     | Schale OK       | ✅ bestätigt    |
| 1   | Tank leer        | Tank OK         | ✅ bestätigt    |
| 2   | Reinigung nötig  | OK              | ⚠️ unbestätigt  |

Beispiel-Antworten E6/ENA:
```
ic:00  → alles OK
ic:01  → Schale fehlt (Bit 0 gesetzt)
ic:02  → Tank leer (Bit 1 gesetzt)
ic:03  → Schale fehlt und Tank leer
```

### RT:0000 — EEPROM-Zähler lesen

Befehl: `RT:XXYY`
Antwort: `rt:` + 64 Hex-Zeichen = 32 Bytes = 16 × 16-Bit-Wörter (gesamt 67 Zeichen)

`RT:XXYY` liest die 16-Wort-EEPROM-Page ab Wort-Adresse `0xXXYY`.
Jede 4-Zeichen-Hex-Gruppe in der Antwort ist ein 16-Bit-Wort (Big-Endian).

Der F50 hat **10 Pages** (160 Wörter, 320 Bytes) lesbares EEPROM:

| Befehl      | EEPROM-Bereich | Inhalt                                    |
| ----------- | -------------- | ----------------------------------------- |
| `RT:0000`   | 0x00–0x0F      | Zähler (Kaffee, Spülung, Reinigung, etc.) |
| `RT:1000`   | 0x10–0x1F      | Erweiterte Zähler / unbekannt             |
| `RT:2000`   | 0x20–0x2F      | Konfigurationsparameter                   |
| `RT:3000`   | 0x30–0x3F      | Konfigurationsparameter                   |
| `RT:4000`   | 0x40–0x4F      | Timing-Parameter                          |
| `RT:5000`   | 0x50–0x5F      | Timing-Parameter                          |
| `RT:6000`   | 0x60–0x6F      | Meist Null, einige Werte bei 0x6B–0x6E    |
| `RT:7000`   | 0x70–0x7F      | Kalibrierung / Konfiguration              |
| `RT:8000`   | 0x80–0x8F      | Konfiguration                             |
| `RT:9000`   | 0x90–0x9F      | Konfiguration, ab 0x9B Null               |
| `RT:A000+`  |                | Nur Nullen (Ende des EEPROM)              |

Einzelne Wörter können auch über `RE:XX` gelesen werden (siehe unten).

#### EEPROM-Wort-Tabelle — bestätigt aus F50-Hardware-Sessions (2026-04-03, 2026-04-18)

Beim F50 wählt die Anzahl der Tastendrücke die Stärke und mappt auf drei
verschiedene EEPROM-Adressen (Session 4, 2026-04-18):
1× Druck → `0x0000`, 2× Druck → `0x0001`, 3× Druck → `0x0002`.
Die bestehenden Sensor-Keys `num_single_espresso` / `num_double_espresso` /
`num_coffee` zeigen auf diese Adressen, sind aber irreführend benannt —
der geplante Rename steht in TODO.md A4.

| EEPROM-Adr. | RT-Offset | Länge | Inhalt                            | ESPHome-Sensor-Key      | Hinweise                                      |
| ----------- | --------- | ----- | --------------------------------- | ----------------------- | --------------------------------------------- |
| 0x0000      | 3         | 4     | F50: 1×-Druck Kaffee (mild)       | `num_single_espresso`   | ✅ bestätigt Session 4                        |
| 0x0001      | 7         | 4     | F50: 2×-Druck Kaffee (normal)     | `num_double_espresso`   | ✅ bestätigt Session 4                        |
| 0x0002      | 11        | 4     | F50: 3×-Druck Kaffee (stark)      | `num_coffee`            | ✅ bestätigt Session 4                        |
| 0x0003      | 15        | 4     | F50: Doppeltaste-Kaffee           | `num_double_coffee`     | ✅ bestätigt Session 4 — keine Strength-Variante |
| 0x0004      | 19        | 4     | Bezüge Espresso?                  | —                       | modellspezifisch, 0 beim F50                  |
| 0x0005      | 23        | 4     | Byte-Split: LB=Konfig (=0x14 beim F50), HB=reinigungs-zurückgesetzter Zeit-Ticker | — | ⚠️ Hypothese — primärer Pflege-Trigger-Kandidat, siehe Session 4 |
| 0x0006      | 27        | 4     | Pulver (Trester-Zähler)           | —                       | —                                             |
| 0x0007      | 31        | 4     | Spülvorgänge                      | `num_rinse`             | ✅ bestätigt — zählt bei FA:02 UND bei Auto-Spülung beim Ausschalten (F50 2026-04-18 ersetzt früheres Session-2-Statement) |
| 0x0008      | 35        | 4     | Reinigungszyklen                  | `num_clean`             | ✅ bestätigt — +1 pro Reinigungszyklus (Session 4) |
| 0x0009      | 39        | 4     | Entkalkungszyklen                 | `num_descale`           | ✅ bestätigt                                  |
| 0x000A      | 43        | 4     | unbekannt                         | —                       | —                                             |
| 0x000B      | 47        | 4     | unbekannt (typisch 17)            | —                       | —                                             |
| 0x000C      | 51        | 4     | unbekannt (typisch 1)             | —                       | —                                             |
| 0x000D      | 55        | 4     | Brüh-Event-Zähler                 | —                       | ✅ Session 4 — +1 pro Brüh-Befehl, +3 bei Strong-Double (interne Vorspülung), nicht durch Reinigung zurückgesetzt |
| 0x000E      | 59        | 4     | Tassen-Zähler mit täglichem Reset | `num_coffees_since_clean` | ⚠️ Sensor-Key ist irreführend benannt — +1 Single / +2 Double, resettet täglich, transienter Wert 0xFA während Reinigung. NICHT "seit Reinigung" |
| 0x000F      | 63        | 4     | Bezüge seit Reinigung             | —                       | ✅ Session 4 — +1 pro Brüh-Befehl, Reset durch Reinigung. Ersetzt Session-2-Spekulation "Bezüge seit Entkalkung" |

#### Erweitertes EEPROM (nur via RE: — nicht sichtbar über RT:0000)

Der F50 hat 32 EEPROM-Wörter (0x00–0x1F). `RT:0000` gibt nur Wörter 0x00–0x0F zurück.
Wörter 0x10–0x1F können nur über `RE:XX` gelesen und über `WE:XX,YYYY` geschrieben werden.

| EEPROM-Adr. | RE:-Wert (F50 Probe) | Inhalt                                            |
| ----------- | -------------------- | ------------------------------------------------- |
| 0x0010      | `8747`               | Langzeit-Brüh-Event-Zähler — spiegelt 0x000D-Muster, +1 pro Brew, nicht durch Reinigung zurückgesetzt |
| 0x0011      | `0037`               | Reinigungs-zurückgesetzter Zähler — wächst ~7.6/Tag im Normalbetrieb, Treiber unbekannt (weder Kaffee noch Spülung bewegen ihn) |
| 0x0012      | `00EC`               | unbekannt                                         |
| 0x0013      | `0320`               | unbekannt                                         |
| 0x0014      | `C05D`               | Per-Produkt Brüh-Phasen-Zähler — +5 mild / +6 normal & stark / +7 mild-double. Nicht Wassermenge |
| 0x0015      | `4292`               | Langzeit-Brüh-Befehls-Zähler — spiegelt 0x000F-Muster, +1 pro Brüh-Befehl |
| 0x0016      | `0000`               | Reinigungs-zurückgesetzter Zähler — wächst ~2/Tag im Normalbetrieb, Hypothese: pro Session oder Power-On |
| 0x0017      | `0000`               | unbekannt (Null)                                  |
| 0x0018      | `0000`               | unbekannt (sporadischer Timeout beim Lesen)       |
| 0x0019      | `0000`               | unbekannt (Null)                                  |
| 0x001A      | `1E02`               | unbekannt                                         |
| 0x001B      | `0126`               | unbekannt                                         |
| 0x001C      | `0256`               | unbekannt (sporadischer Timeout beim Lesen)       |
| 0x001D      | `0005`               | unbekannt                                         |
| 0x001E      | `0004`               | unbekannt                                         |
| 0x001F      | `03DD`               | unbekannt                                         |

> **Wassermenge-Einstellung:** Erschöpfend getestet bei 30ml, 55ml, 120ml, 240ml (Maximum).
> Alle 160 EEPROM-Wörter (RT:0000–9000, RE:00–9F) gescannt — kein Unterschied zwischen Einstellungen.
> Alle 256 RR:-RAM-Register (0x00–0xFF) gescannt — nur zeitabhängige flüchtige Änderungen,
> keine reproduzierbare Korrelation zur Wassermenge. Auch nach Power-Cycle keine Änderung.
> Die Wassermenge wird intern im Jura-Controller gespeichert und ist
> **nicht über das Service-Interface zugänglich**.

Beispiel F50 Antwort (dekodiert):
```
rt:0FF307B210630BB1000000140077392E002D000D167800000000021B00020040
     ^^^^              → 0x0FF3 = 4083 Normale Kaffees (Adr. 0x0000)
         ^^^^          → 0x07B2 = 1970 Doppelkaffees  (Adr. 0x0001)
                     ^^^^       → 0x392E = 14638 Spülvorgänge (Adr. 0x0007)
                         ^^^^   → 0x002D = 45 Reinigungen (Adr. 0x0008)
                             ^^^^→ 0x000D = 13 Entkalkungen (Adr. 0x0009)
```

> **Hinweis zur Sensor-Benennung (Session-4-Abgleich, 2026-04-18):**
>
> - Beim F50 mappt die Anzahl der Tastendrücke die Stärke auf drei
>   verschiedene Adressen: 1× Druck → `0x0000` (Sensor `num_single_espresso`),
>   2× Druck → `0x0001` (Sensor `num_double_espresso`),
>   3× Druck → `0x0002` (Sensor `num_coffee`). Alle drei ESPHome-Sensor-Keys
>   sind beim F50 irreführend — keiner davon repräsentiert Espresso oder eine
>   "kleine" Größe. Die Doppelkaffee-Taste zählt unabhängig von der Stärke
>   immer `0x0003`.
> - `num_coffees_since_clean` zeigt auf `0x000E`, einen Tassen-Zähler mit
>   täglichem Reset — kein Seit-Reinigung-Zähler. Der echte
>   Bezüge-seit-Reinigung-Zähler ist `0x000F` (nicht exponiert; geplanter
>   neuer Sensor). Der `0x000E`-Sensor nimmt außerdem während des
>   Reinigungszyklus einen transienten Wert von 0xFA (250) an.
> - `needs_rinse` zeigt auf IC:bit0, das tatsächlich ein "Session benutzt seit
>   Kaltstart"-Flag ist — wird nicht durch manuelles Spülen, Auto-Spülung oder
>   einen kurzen Power-Cycle gelöscht. Sensor-Name wird in v2.0 korrigiert
>   (siehe TODO A4).
>
> Alle ESPHome-Sensor-Keys bleiben zunächst unverändert, damit bestehende
> HA-Entity-IDs und InfluxDB-Historie erhalten bleiben. Der koordinierte
> Rename landet im v2.0-Release sobald die Observation-Sessions abgeschlossen
> sind.

### RR: — RAM lesen (Debug)

Befehl: `RR:XX` (XX = Hex-Adresse, 0x00–0xFF)

Der F50 hat **256 lesbare RAM-Register** (0x00–0xFF). Register 0x00–0x23 enthalten
den Live-Maschinenstatus. Register ab 0x24 scheinen EEPROM-Inhalte und
Konfigurationsparameter zu spiegeln. Viele ungerade Register sind Byte-vertauschte
(Big-Endian) Kopien des vorherigen geraden Registers.
Antwort: `rr:YYYY` (16-Bit Hex-Wert)

#### RR:03 — Betriebstemperatur / Bereitschaft (✅ bestätigt)

RR:03 Bit 2 (0x0004) zeigt an, dass die Maschine Betriebstemperatur erreicht hat (PID-Modus).
Dies wird als **ready** Binary-Sensor verwendet.

| Wert   | Bedeutung                                  |
| ------ | ------------------------------------------ |
| `0000` | Aus / kalt / nicht auf Betriebstemperatur  |
| `0004` | Betriebstemperatur erreicht (PID-Modus)    |
| `4000` | Startup-Übergang (kurzzeitig)              |
| `4004` | Motor aktiv + auf Temperatur               |
| `C004` | Pumpe/Heizung aktiv (Spülen, Brühen)      |

> `0x0004` kehrt nach ~10-12 Min. Standby zu `0x0000` zurück (thermische Abkühlung).

#### RR:18/19 — **WIDERLEGT** als Temperatur

Vorherige Hypothese: RR:18 = Temperatur in Grad C (>=0x3C = 60 Grad C = bereit).
**Widerlegt:** RR:18 zeigt 0x34-0x35 in ALLEN Zuständen (kalt, heizend, bereit). Wert
ändert sich kaum zwischen den Zuständen. Nicht nutzbar als Temperatur- oder Bereitschaftsindikator.

#### Beobachtungen beim F50 (Startup-Sequenz)

Aufgezeichnet während: Aus → Schale fehlt → Aufheizen → Spülen → Bereit

| Register | Aus    | Schale fehlt | Aufheizen | Spülen | Bereit | Interpretation                         |
| -------- | ------ | ------------ | --------- | ------ | ------ | -------------------------------------- |
| RR:03    | `0000` | `0000`       | `0004`    | `0004` | `0004` | ✅ Betriebstemperatur erreicht (Bit 2) |
| RR:04    | `0029` | `0029`       | `0429`    | `0429` | `0429` | Heizstatus (?)                         |
| RR:18    | `003B` | `003B`       | —         | —      | `003C` | ❌ NICHT Temperatur — ändert sich kaum |
| RR:19    | `3B00` | `3B00`       | —         | —      | `3C00` | ❌ Big-Endian Kopie von RR:18          |
| RR:21    | `0024` | `0024`       | variiert  | `0013` | `0034` | ⚠️ unbestätigt — Brühgruppe?          |
| RR:22    | `242E` | `242E`       | variiert  | `105E` | `316C` | ⚠️ unbestätigt                        |
| RR:23    | `2E00` | `2E00`       | variiert  | `5E00` | `6C00` | ⚠️ unbestätigt — Big-Endian?          |

> **Bereitschaftserkennung:** RR:03 Bit 2 (0x0004) verwenden um Betriebstemperatur zu erkennen.
> RR:18/19 funktionieren hierfür NICHT.

### RE: — EEPROM-Wort lesen

`RE:XX` liest ein einzelnes 16-Bit-EEPROM-Wort an Adresse `XX` (hex).
Im Gegensatz zu `RT:0000` (gibt 16 Wörter als Block zurück) kann `RE:` den
gesamten EEPROM-Bereich lesen, einschließlich erweiterter Adressen 0x10–0x1F.

| Befehl   | Antwort-Beispiel | Beschreibung                                         |
| -------- | ---------------- | ---------------------------------------------------- |
| `RE:00`  | `re:110A`        | EEPROM-Wort 0x00 (identisch zum ersten Wort in RT:0000) |
| `RE:10`  | `re:8747`        | Erweitertes EEPROM-Wort 0x10 (NICHT sichtbar via RT:0000) |
| `RE:31`  | `re:XXXX`        | Maschinentyp-Code                                    |

> Der F50 hat 32 lesbare EEPROM-Wörter (0x00–0x1F). Adressen ab 0x20 scheinen
> zu wrappen oder gleiche Daten zurückzugeben. Einige Adressen (0x18, 0x1C)
> haben sporadische Timeouts.

### WE: — EEPROM-Wort schreiben

| Befehl        | Beschreibung        |
| ------------- | ------------------- |
| `WE:31,2712`  | Maschinentyp setzen |

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

## Modell-Unterschiede

| Modell            | IC: Layout | Espresso    | Kaffee      | Spülen      | Brühpos. | Stecker |
| ----------------- | ---------- | ----------- | ----------- | ----------- | -------- | ------- |
| Impressa F50      | Layout A   | ❌ keiner   | FA:06 ✅    | FA:02 ✅    | FN:13    | 7-polig |
| Impressa F7/F8    | Layout B   | FA:04       | FA:06       | FA:0B       | FN:13    | 7-polig |
| Impressa S95/S90  | Layout B   | FA:04       | FA:06       | FA:0B       | FN:13    | 4-polig |
| Impressa J6       | Layout B   | FA:07       | FA:09       | FA:0C       | FN:22    | 7-polig |
| E6 / E8 / E65     | Layout B   | FA:04       | FA:09       | —           | FN:22    | 7-polig |
| ENA 5/7/Micro90   | Layout B   | —           | FA:09/0A    | FA:04       | FN:22    | 7-polig |
| X7 / Saphira      | unbekannt  | FA:01       | FA:06       | FA:0B       | FN:13    | 9-polig |

> `FA:XX` Befehle sind maschinenspezifisch und entsprechen physischen Tasten.
> `FN:XX` Low-Level Befehle sind weitgehend modellübergreifend.

### Modell-spezifische Besonderheiten

| Modell         | Besonderheit                                                                                      |
| -------------- | ------------------------------------------------------------------------------------------------- |
| Impressa F50   | IC: Schalen-Bit invertiert (Bit4=1 → Schale vorhanden)                                           |
| ENA Micro 90   | Service-Port schläft nach dem Ausschalten einige Minuten — nicht als Dauerversorgung verwendbar  |
| ENA 7          | Hochspannungs-Einrastschalter — `AN:01` allein funktioniert ggf. nicht; Relais an Einschalttaste nötig |
| ENA Micro 90   | ⚠️ Thermoblock bleibt bei 150°C wenn Milch-Reinigung unterbrochen wird — Brandgefahr!            |
| Alle Modelle   | Brühgruppe mechanisch begrenzt: Kaffeemenge über `FN:` Sequenz nicht über ~8–9g erhöhen          |
| Jura C5        | `FA:09` antwortet mit `ok:` führt aber nichts aus; `FA:` Befehle allgemein unzuverlässig         |
| Z10 / modern   | Bluetooth/WiFi verwendet XOR-Verschlüsselung mit Session-Key — serielles Standardprotokoll gesperrt |

---

## Weiterführende Links

| Link                                                                          | Beschreibung                                         |
| ----------------------------------------------------------------------------- | ---------------------------------------------------- |
| https://uk.jura.com/en/homeproducts/accessories/SmartConnect-Main-72167       | Jura Smart Connect (offizielle Bluetooth-Brücke)     |
| https://github.com/Jutta-Proto/hardware-pi                                    | Raspberry Pi Hardware-Implementierung                |
| http://heise.de/-3058350                                                      | Heise-Artikel: Kaffeemaschinen-Bezahlsystem (DE)     |
| https://github.com/sklas/CofFi                                                | CofFi: vollständige ESP8266 MQTT Implementierung     |
