# TODO

Stand: 2026-03-23

---

## Sofort (MIN)

### D1 — Deploy auf HA-Server
- [ ] Komponente via SFTP pushen: `jura_coffee.cpp`, `jura_coffee.h`, `__init__.py`
- [ ] YAML pushen: `jura-coffee-f50.yaml`
- [ ] Lokal kompilieren: `~/.venv/esphome/bin/esphome compile jura-coffee-f50.yaml`
- [ ] OTA flashen: `~/.venv/esphome/bin/esphome upload --device jura-coffee-f50.local jura-coffee-f50.yaml`

### D2 — Hardware-Verifikation (nach D1)
- [ ] IC: bit3 = Tank: Wassertank entleeren → `IC:` im Log → Bit 3 muss 0 werden
- [ ] IC: bit1 = need_clean: „Pflege drücken" auslösen → Bit 1 muss 0 werden
- [ ] EEPROM num_rinse: Spülung (FA:02) → `parse(31,4)` muss steigen
- [ ] EEPROM num_clean: Reinigung durchführen → `parse(35,4)` muss steigen
- [ ] EEPROM num_descale: Entkalkung → `parse(39,4)` muss steigen (wenn möglich)

### D3 — GitHub Remote
- [ ] Repository auf GitHub anlegen (public, Name: `esphome-jura`)
- [ ] `git remote add origin <url>`
- [ ] `git push -u origin main`

---

## Kurzfristig (MID)

### F1 — Maschinentyp-Sensor (TY:)
- [ ] `TY:` einmalig in `setup()` senden + Response lesen
- [ ] Als `text_sensor` exponieren
- [ ] Schema in `__init__.py` ergänzen

### F2 — Startup-Sequenz YAML-Script
- [ ] `script:` in YAML: `AN:01` → delay 90s → `FA:02`
- [ ] Button „Start & Rinse" in YAML-Beispiel ergänzen
- [ ] In `jura-coffee-f50.yaml` als kommentiertes Beispiel

### F3 — Power state binary sensor (research)
- [ ] Investigate: compare IC: and RR: logs with machine on vs. off (standby) — which bit indicates power state?
- [ ] Check if `AN:01` response or silence can be used to detect off state
- [ ] Implement `binary_sensor` "power" once bit is identified
- [ ] Add `switch` entity to turn machine on (`AN:01`) / off (`AU:01`) if `AU:` command exists

### F4 — RR:18 verifizieren (Temperatur-Hypothese)
- [ ] Beim Aufheizen `RR:18` jede Sekunde loggen
- [ ] Prüfen ob Wert = °C (Hypothese: ≥ 0x3C = 60°C → bereit)
- [ ] Alternativ: IC: Bits 6–7 beim Aufheizen beobachten

### F4 — ready Binary Sensor (nach F3)
- [ ] Neuer Binary Sensor: Maschine bereit
- [ ] In `read_sensors()` publishen

### F5 — Temperatur-Sensor (nach F3)
- [ ] `RR:18` periodisch auslesen (alle 30s)
- [ ] Als `sensor` in °C exponieren

### F6 — Display-Text Input
- [ ] `text_input` in HA → `DA:` Command
- [ ] Button „Clear display" → `DR:`
- [ ] YAML only, no C++ needed

### F7 — Coffee strength & water amount (research)
- [ ] Investigate: strength (mild/normal/strong) is selected on the machine by pressing the coffee button multiple times — is there an FA: command for this, or does it require button simulation?
- [ ] Investigate: water amount — is there an RW:/DA: command to set it, or only via button combinations?
- [ ] Compare IC: and RR: logs before/after changing strength — does any status bit change?
- [ ] Compare EEPROM (RT:0000) before/after change — is the setting persisted?
- [ ] If controllable: implement `select` entity for strength (mild/normal/strong) + `number` for water amount in YAML/C++

---

## Langfristig (MAX)

### A1 — C++ Startup-Sequenz (nach F3/F4)
- [ ] `startup_sequence()` in C++: `AN:01` → Poll `RR:18` alle 5s → warten auf ≥ 0x3C → `FA:02`
- [ ] Timeout 180s mit `ESP_LOGW`
- [ ] Action `jura_coffee.startup_sequence` in `__init__.py` registrieren

### A2 — HA Automations
- [ ] Automation: `need_clean = true` → Push-Benachrichtigung
- [ ] Automation: Täglicher Kaffeestart (z.B. 7:30)
- [ ] Blueprint für Kaffee-Routine

### A3 — EEPROM 0x000A–0x000E identifizieren
- [ ] Systematisch: Kaffee kochen, RT:0000 vorher/nachher vergleichen
- [ ] Geänderte Offsets benennen und exponieren
- [ ] Protokolldoku aktualisieren

### A4 — Counter-Naming Quirk beheben (Breaking Change)
- [ ] `num_single_espresso` → sinnvollerer Name (z.B. `num_coffee_small`)
- [ ] Als Major-Version taggen (v2.0.0)
- [ ] Alle Docs + YAML-Beispiele aktualisieren

---

## Offene Fragen / Zu untersuchen

- IC: Bits 0, 2, 5, 6, 7 — Bedeutung unbekannt (F50)
- IC: Bit 2 ändert sich gleichzeitig mit Bit 1 bei „Pflege drücken" — warum?
- RR:03 Bit 2 = Heizung aktiv? (Hypothese, nicht bestätigt)
- RR:21 = Brühgruppe Position? (Hypothese, nicht bestätigt)
- EEPROM 0x0010+ — nie ausgelesen, Inhalt unbekannt
