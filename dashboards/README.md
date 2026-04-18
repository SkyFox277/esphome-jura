# Home Assistant Dashboards für Jura Coffee F50

Drei Dashboard-Views für die Beobachtung der F50 nach Session 4:

1. **Übersicht** — tägliche Maschinen-Nutzung auf einen Blick
2. **Pflege-Monitoring** — die vier Pflege-Trigger-Kandidaten aus Session 4
   (A5 in TODO.md) mit Verlaufs-Graphen und Prognosen
3. **Diagnose** — Raw-IC-Byte, Raw-Pages, Last-Response

## ⚠️ Entity-IDs müssen auf dein Setup angepasst werden

Das Dashboard ist als **Arbeitsbeispiel** für den F50-Autor dieses Repos
geschrieben. Bevor du es 1:1 übernimmst:

- **Modell:** Nur auf Jura Impressa F50 mit dem hier dokumentierten Press-Count-
  Mapping (1×→0x0000, 2×→0x0001, 3×→0x0002) sinnvoll. Andere Jura-Modelle
  haben andere EEPROM-Layouts — siehe `docs/jura-protocol.md`.
- **Entity-ID-Form:** Aktuell zeigt das YAML auf
  `sensor.jura_coffee_f50_jura_coffee_f50_*` (Doppel-Prefix). Grund: mein
  `jura-coffee-f50.yaml` verwendet `${friendly_name}` sowohl als Device- als
  auch als Sensor-Name, HA slugifiziert beide und konkateniert. Dieser
  Doppel-Prefix ist als Breaking-Change in `TODO.md` A4 geplant
  (post-A5-Observation). Wenn dein Setup das `${friendly_name}`-Duplikat
  vermeidet, sind deine Entity-IDs einfach `sensor.<device>_<sensor>` — dann
  musst du im Dashboard alle Referenzen kürzen (siehe unten).
- **Device-Name:** Ersetze `jura_coffee_f50` durch den slugifizierten Namen
  deines Devices.

Quick-Adaption per sed:

```bash
# Wenn dein Device slugifiziert als "my_coffee" heißt und du KEIN Doppel-Prefix hast:
sed -i 's/jura_coffee_f50_jura_coffee_f50_/my_coffee_/g' dashboards/jura-coffee-dashboard.yaml dashboards/template-sensors.yaml

# Wenn du Doppel-Prefix hast wie ich:
sed -i 's/jura_coffee_f50_jura_coffee_f50_/my_coffee_my_coffee_/g' dashboards/jura-coffee-dashboard.yaml dashboards/template-sensors.yaml
```

## Voraussetzungen

- Firmware aus PR #9 geflasht (8 neue Sensoren + `ic_byte0_raw` + 2 Raw-Pages)
- HA hat die neuen Entities registriert (nach Flash ca. 1–2 Polling-Zyklen warten,
  ggf. ESPHome-Integration in *Einstellungen → Geräte & Dienste* neu laden)
- InfluxDB sammelt für mehrere Stunden bis Tage, damit History-Graphen und
  Rate-Berechnungen sinnvoll aussehen

## Installation

### 1. Template-Sensoren hinzufügen

Die YAML-Datei ablegen unter `/config/jura-template-sensors.yaml` (oder einem
anderen Pfad deiner Wahl), dann in `configuration.yaml` einbinden:

```yaml
template: !include jura-template-sensors.yaml
```

Falls du bereits einen `template:`-Block hast, die beiden Template-Einträge
aus `template-sensors.yaml` direkt in deinen bestehenden Block einhängen.

Anschließend in *Entwicklerwerkzeuge → YAML → Template-Entitäten neu laden*
(kein HA-Neustart nötig).

Neu erscheinende Entities:

- `sensor.jura_pflege_prognose_tage` — geschätzte Tage bis zur nächsten
  Pflege-Aufforderung (basiert auf 0x0005-HB-Wochen-Hypothese aus Session 4)
- `sensor.jura_ic_byte0_bits` — IC:byte 0 als Bit-String für Diagnose

### 2. Dashboard registrieren (YAML-Mode)

`jura-coffee-dashboard.yaml` ablegen unter `/config/dashboards/jura-coffee.yaml`,
dann in `configuration.yaml`:

```yaml
lovelace:
  mode: storage
  dashboards:
    jura-coffee:
      mode: yaml
      filename: dashboards/jura-coffee.yaml
      title: Jura Coffee F50
      icon: mdi:coffee-maker
      show_in_sidebar: true
      require_admin: false
```

HA einmal neu starten (`ha core restart`). Anschließend erscheint "Jura Coffee F50"
in der Seitenleiste.

**Alternativ (ohne `configuration.yaml`-Änderung):** In HA *Einstellungen →
Dashboards → Dashboard hinzufügen* → Drei-Punkte-Menü → Rohe Konfiguration →
Inhalt von `jura-coffee-dashboard.yaml` einfügen.

### 3. (Optional) Automation für Pflege-Benachrichtigung

```yaml
- alias: "Jura Pflege steht an"
  trigger:
    - platform: numeric_state
      entity_id: sensor.jura_pflege_prognose_tage
      below: 3
  action:
    - service: notify.mobile_app_<dein_handy>
      data:
        title: "Jura braucht bald Pflege"
        message: >
          In ca. {{ states('sensor.jura_pflege_prognose_tage') }} Tagen
          fordert die Maschine Pflege an.
```

Nach Session 5 (Entkalkungs-Observation) kann eine analoge Automation für den
Entkalkungs-Countdown ergänzt werden.

## Entity-ID-Referenz (F50 mit Doppel-Prefix)

| Entity-ID                                                                     | Quelle                                                     |
| ----------------------------------------------------------------------------- | ---------------------------------------------------------- |
| `sensor.jura_coffee_f50_jura_coffee_f50_maintenance_weeks_since_cleaning`     | EEPROM 0x0005 HB (Hypothese Wochen-Ticker)                 |
| `sensor.jura_coffee_f50_jura_coffee_f50_maintenance_config_0x0005_low`        | EEPROM 0x0005 LB (Konfig, konstant 20 auf F50)             |
| `sensor.jura_coffee_f50_jura_coffee_f50_brews_since_cleaning`                 | EEPROM 0x000F (echter seit-Reinigung-Counter)              |
| `sensor.jura_coffee_f50_jura_coffee_f50_maintenance_counter_0x0011`           | EEPROM 0x0011 (Treiber unbekannt, ~7.6/Tag)                |
| `sensor.jura_coffee_f50_jura_coffee_f50_maintenance_counter_0x0016`           | EEPROM 0x0016 (Treiber unbekannt, ~2/Tag)                  |
| `sensor.jura_coffee_f50_jura_coffee_f50_ic_byte0_raw`                         | IC:byte 0 (numerisch 0-255)                                |
| `sensor.jura_coffee_f50_jura_coffee_f50_eeprom_page_rt0000_raw`               | RT:0000 Rohdaten (Forensik)                                |
| `sensor.jura_coffee_f50_jura_coffee_f50_eeprom_page_rt1000_raw`               | RT:1000 Rohdaten (Forensik)                                |
| `sensor.jura_coffee_f50_jura_coffee_f50_cleanings`                            | `num_clean` (0x0008)                                       |
| `sensor.jura_coffee_f50_jura_coffee_f50_descalings`                           | `num_descale` (0x0009)                                     |
| `sensor.jura_coffee_f50_jura_coffee_f50_single_espressos`                     | 0x0000 (1×-Press)                                          |
| `sensor.jura_coffee_f50_jura_coffee_f50_double_espressos`                     | 0x0001 (2×-Press)                                          |
| `sensor.jura_coffee_f50_jura_coffee_f50_coffees`                              | 0x0002 (3×-Press)                                          |
| `sensor.jura_coffee_f50_jura_coffee_f50_double_coffees`                       | 0x0003 (Doppel-Taste)                                      |
| `sensor.jura_coffee_f50_jura_coffee_f50_rinses`                               | 0x0007 (Rinse + Auto-Rinse)                                |
| `sensor.jura_coffee_f50_jura_coffee_f50_coffees_since_cleaning`               | 0x000E — **misnamed**, ist Tassen-Tageszähler              |
| `binary_sensor.jura_coffee_f50_jura_coffee_f50_ready`                         | RR:03 bit 2                                                |
| `binary_sensor.jura_coffee_f50_jura_coffee_f50_need_cleaning`                 | IC:bit 1 — **misnamed**, ist "cleaning-cycle-running"      |
| `binary_sensor.jura_coffee_f50_jura_coffee_f50_needs_rinse`                   | IC:bit 0 — **misnamed**, ist "session_used"                |
| `binary_sensor.jura_coffee_f50_jura_coffee_f50_tank_empty`                    | IC:bit 3                                                   |
| `binary_sensor.jura_coffee_f50_jura_coffee_f50_tray_missing`                  | IC:bit 4                                                   |
| `binary_sensor.jura_coffee_f50_jura_coffee_f50_status`                        | ESPHome Online-Status                                      |

Die mit **misnamed** markierten Labels werden in v2.0 korrigiert (`TODO.md` A4);
bis dahin wird im Dashboard der funktionale Name in der `name:`-Überschreibung
verwendet statt des irreführenden Entity-Namens.

## Session-4-Hypothesen — was das Dashboard zeigen soll

Das Pflege-Monitoring-View ist als **Experiment** ausgelegt: die vier
cleaning-reset-Counter und ihre Wachstumsraten werden nebeneinander sichtbar,
damit bei der nächsten "Pflege drücken"-Meldung direkt ablesbar ist, welcher
Counter zuerst seine Schwelle erreicht hat. Das identifiziert den echten
Pflege-Trigger und lässt v2.0 des Components den passenden Sensor-Namen
vergeben.

A5-Ablauf (TODO.md):

1. **Baseline** direkt nach einer Reinigung:
   `maintenance_weeks_since_cleaning = 0`, `brews_since_cleaning` klein,
   `maintenance_counter_0x0011` = 0, `maintenance_counter_0x0016` = 0
2. **Beobachtungsphase** (ca. 14 Tage bzw. bis zur nächsten Pflege-Meldung):
   welche Counter wachsen mit welcher Rate?
3. **Trigger-Event:** bei nächster "Pflege drücken"-Anzeige Werte aller vier
   Counter notieren — die echten Schwellen
4. **v2.0** bestätigt den Trigger-Counter + benennt die misnamed Sensoren um
