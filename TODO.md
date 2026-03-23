# TODO

Last updated: 2026-03-23

---

## Immediate (MIN)

### D1 — Deploy to HA server
- [ ] Push component via SFTP: `jura_coffee.cpp`, `jura_coffee.h`, `__init__.py`
- [ ] Push YAML: `jura-coffee-f50.yaml`
- [ ] Compile locally: `~/.venv/esphome/bin/esphome compile jura-coffee-f50.yaml`
- [ ] Flash via OTA: `~/.venv/esphome/bin/esphome upload --device jura-coffee-f50.local jura-coffee-f50.yaml`

### D2 — Hardware verification (after D1)
- [ ] IC: bit3 = tank: remove water tank → `IC:` in log → bit 3 must go to 0
- [ ] IC: bit1 = need_clean: trigger "clean" prompt → bit 1 must go to 0
- [ ] EEPROM num_rinse: run rinse (FA:02) → `parse(31,4)` must increment
- [ ] EEPROM num_clean: run cleaning cycle → `parse(35,4)` must increment
- [ ] EEPROM num_descale: run descaling → `parse(39,4)` must increment (if possible)

### D3 — GitHub remote
- [ ] Create GitHub repository (public, name: `esphome-jura`)
- [ ] `git remote add origin <url>`
- [ ] `git push -u origin main`

---

## Near-term (MID)

### F1 — Machine type sensor (TY:)
- [ ] Send `TY:` once in `setup()` and read response
- [ ] Expose as `text_sensor`
- [ ] Add schema to `__init__.py`

### F2 — Startup sequence YAML script
- [ ] `script:` in YAML: `AN:01` → delay 90s → `FA:02`
- [ ] Add "Start & Rinse" button to YAML example
- [ ] Add as commented example in `jura-coffee-f50.yaml`

### F3 — Power state binary sensor (research)
- [ ] Investigate: compare IC: and RR: logs with machine on vs. off (standby) — which bit indicates power state?
- [ ] Check if `AN:01` response or silence can be used to detect off state
- [ ] Implement `binary_sensor` "power" once bit is identified
- [ ] Add `switch` entity to turn machine on (`AN:01`) / off (`AU:01`) if `AU:` command exists

### F4 — RR:18 verify (temperature hypothesis)
- [ ] Log `RR:18` every second during warm-up
- [ ] Check if value = °C (hypothesis: ≥ 0x3C = 60°C → ready)
- [ ] Alternative: observe IC: bits 6–7 during warm-up

### F5 — Ready binary sensor (after F4)
- [ ] New binary sensor: machine ready
- [ ] Publish in `read_sensors()`

### F6 — Temperature sensor (after F4)
- [ ] Poll `RR:18` periodically (every 30s)
- [ ] Expose as `sensor` in °C

### F7 — Display text input
- [ ] `text_input` in HA → `DA:` command
- [ ] Button "Clear display" → `DR:`
- [ ] YAML only, no C++ needed

### F8 — Coffee strength & water amount (research)
- [ ] Investigate: strength (mild/normal/strong) is selected on the machine by pressing the coffee button multiple times — is there an FA: command for this, or does it require button simulation?
- [ ] Investigate: water amount — is there an RW:/DA: command to set it, or only via button combinations?
- [ ] Compare IC: and RR: logs before/after changing strength — does any status bit change?
- [ ] Compare EEPROM (RT:0000) before/after change — is the setting persisted?
- [ ] If controllable: implement `select` entity for strength (mild/normal/strong) + `number` for water amount in YAML/C++

---

## Long-term (MAX)

### A1 — C++ startup sequence (after F3/F4)
- [ ] `startup_sequence()` in C++: `AN:01` → poll `RR:18` every 5s → wait for ≥ 0x3C → `FA:02`
- [ ] Timeout 180s with `ESP_LOGW`
- [ ] Register action `jura_coffee.startup_sequence` in `__init__.py`

### A2 — HA automations
- [ ] Automation: `need_clean = true` → push notification
- [ ] Automation: daily coffee start (e.g. 7:30)
- [ ] Blueprint for coffee routine

### A3 — Identify EEPROM 0x000A–0x000E
- [ ] Systematically: brew coffee, compare RT:0000 before/after
- [ ] Name changed offsets and expose as sensors
- [ ] Update protocol documentation

### A4 — Fix counter naming quirk (breaking change)
- [ ] `num_single_espresso` → better name (e.g. `num_coffee_small`)
- [ ] Tag as major version (v2.0.0)
- [ ] Update all docs + YAML examples

---

## Maintenance

### M1 — GitHub Actions Node.js upgrade (before June 2026)
- [ ] Update `actions/checkout` to v5+ (Node.js 24 support)
- [ ] Update `actions/setup-python` to v6+
- [ ] Update `actions/cache` to v5+

---

## Open questions

- IC: bits 0, 2, 5, 6, 7 — meaning unknown (F50)
- IC: bit 2 changes simultaneously with bit 1 when cleaning is triggered — why?
- RR:03 bit 2 = heater active? (hypothesis, not confirmed)
- RR:21 = brew group position? (hypothesis, not confirmed)
- EEPROM 0x0010+ — never read, content unknown
