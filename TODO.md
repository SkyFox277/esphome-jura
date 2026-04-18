# TODO

Last updated: 2026-04-03

---

## Immediate (MIN)

### D1 — Deploy to HA server ✅
- [x] Push component via SFTP/SCP
- [x] Push YAML
- [x] Compile locally
- [x] Flash via OTA

### D2 — Hardware verification ✅
- [x] IC: bit4 = tray (inverted): confirmed by physically removing tray → 0xDE→0xCE
- [x] IC: bit3 = tank (inverted): confirmed by removing tank → 0xDA→0xD2
- [x] IC: bit1 = need_clean (inverted): bit stays 1 in all tested states (not triggered)
- [x] IC: bit0 = needs_rinse: confirmed — 0 after cold start, 1 after first coffee
- [x] EEPROM num_rinse (0x0007): confirmed — increments on FA:02
- [x] EEPROM num_coffees_since_clean (0x000E): confirmed — increments per coffee, resets after cleaning
- [ ] EEPROM num_clean: run full cleaning cycle → `parse(35,4)` must increment
- [ ] EEPROM num_descale: run descaling → `parse(39,4)` must increment (if possible)

### D3 — GitHub remote ✅
- [x] Repository on GitHub (public): `SkyFox277/esphome-jura`
- [x] Branch protection + auto-delete on merge

---

## Near-term (MID)

### F1 — Machine type sensor (TY:)
- [ ] Send `TY:` once in `setup()` and read response
- [ ] Expose as `text_sensor`
- [ ] Add schema to `__init__.py`

### F2 — Startup & Rinse script ✅
- [x] YAML script: `AN:01` → wait for `ready` sensor → `FA:02`
- [x] Timeout 180s with warning log
- [x] "Start & Rinse" button in HA

### F3 — Ready sensor ✅
- [x] RR:03 bit2 = operating temperature (PID mode) — confirmed
- [x] Binary sensor `ready` implemented
- [x] Returns to 0 after ~10-12 min standby (thermal cooldown)

### F4 — RR:18 temperature hypothesis ✅ DISPROVEN
- [x] Logged RR:18 across all states (cold, heating, ready, brewing)
- [x] Value 0x34-0x35 in ALL states — barely changes
- [x] **Not temperature, not useful as ready indicator**

### ~~F5 — Ready binary sensor~~ → merged into F3
### ~~F6 — Temperature sensor~~ → cancelled (RR:18 disproven)

### F7 — Display text input
- [ ] `text_input` in HA → `DA:` command
- [ ] Button "Clear display" → `DR:`
- [ ] YAML only, no C++ needed

### F8 — Water amount (research) ❌ NOT POSSIBLE
- [x] Change water amount setting on machine physically (tested 30/55/120/240ml)
- [x] Compare EEPROM (RT:0000–9000, RE:00–9F) before/after → **no difference**
- [x] Check RR:00–FF registers → **only volatile values, not correlated**
- [x] **Conclusion: water amount is stored internally, not accessible via serial interface**

### F9 — Coffee strength (research)
- [ ] Test: send FA:06 twice within 2s → does second press register as "stronger"?
- [ ] If yes: implement script with configurable delay between sends
- [ ] If no: check for undocumented FA: command for strength
- [ ] Fallback: FN: low-level sequence (FN:07 grinder on → variable delay → FN:08 off → brew)
- [ ] FN: fallback requires per-phase timing calibration on hardware

---

## Debug Sessions (planned)

### S3 — Water amount EEPROM mapping ✅ DONE (not found)
- [x] Scanned RT:0000–9000 (10 pages, 160 EEPROM words) at 55ml and 240ml — identical
- [x] Scanned RE:00–9F (160 individual words) at 55ml and 240ml — identical
- [x] Scanned RR:00–FF (256 RAM registers) at 30/55/120/240ml — only volatile changes
- [x] Discovered RT:1000–9000 pages and RE:20+ / RR:24+ extended ranges
- [x] **Result: water amount not in any accessible address space**

### S4 — Coffee strength test
- [ ] Start debug dump (IC: + RR:03-06 + RT:0000)
- [ ] Send FA:06 → observe normal coffee
- [ ] Wait for machine to be ready again
- [ ] Send FA:06, then FA:06 again within 1-2s → observe if grinder runs longer
- [ ] If strength changes: document timing requirements
- [ ] If not: test FN:07/FN:08 manual grind sequence with variable delay

### S5 — Cleaning cycle observation
- [ ] Start debug dump before cleaning
- [ ] Run full cleaning cycle (with tablet)
- [ ] Observe: which EEPROM words reset? (0x000E should reset to 0)
- [ ] Confirm num_clean (0x0008) increments
- [ ] Identify threshold: at what 0x000E value does the machine show "Pflege drücken"?
- [ ] Also observe 0x000D and 0x000F behavior

### S6 — EEPROM 0x000D / 0x000F identification
- [ ] Track 0x000D (currently 566) and 0x000F (currently 78) over several coffees
- [ ] Run descaling cycle → check which word resets
- [ ] Compare with user's descaling/cleaning history to confirm assignment

### S7 — Extended EEPROM (RT:0010+)
- [ ] Read RT:0010, RT:0020 etc. — are there more EEPROM lines?
- [ ] Document any new words found
- [ ] Look for water amount / strength settings in extended EEPROM

---

## Long-term (MAX)

### A1 — C++ startup sequence → ✅ replaced by YAML script (F2)

### A2 — HA automations
- [ ] Automation: `need_clean = true` → push notification
- [ ] Automation: `coffees_since_clean` > threshold → warning notification
- [ ] Automation: daily coffee start (e.g. 7:30) using Start & Rinse script
- [ ] Blueprint for coffee routine

### A3 — Identify EEPROM 0x000A–0x000F → partially done
- [x] 0x000E = coffees since cleaning (confirmed)
- [ ] 0x000A, 0x000B, 0x000C — still unknown
- [ ] 0x000D — possibly since descaling (value 566, observe)
- [ ] 0x000F — unknown (value 78, observe)

### A4 — Fix counter naming quirk (breaking change)

Scheduled after all protocol-observation sessions (S5/S6) are complete and
the true semantics of every sensor are confirmed from long-term InfluxDB
history. Renaming config keys breaks the ESPHome unique_id and therefore
splits InfluxDB history — do it in one coordinated release.

Config keys to rename (confirmed misnomers):

- [ ] F50 strength-by-press-count mapping:
  - `num_single_espresso` (0x0000) → represents 1x-press coffee
  - `num_double_espresso` (0x0001) → represents 2x-press normal coffee
  - `num_coffee` (0x0002) → represents 3x-press strong coffee
  - Target: `num_coffee_mild` / `num_coffee_normal` / `num_coffee_strong`
    (or a model-specific schema in `MACHINE_PROFILES`)
- [ ] `num_coffees_since_clean` (0x000E) → misnamed, actually a cups-today
  counter with daily resets. Keep a sensor on 0x000E renamed to
  `cups_today`, and point a new `num_coffees_since_clean` at 0x000F (the
  real brews-since-cleaning counter)
- [ ] `needs_rinse` (IC:bit0) → actually "session has been used since
  cold start" — neither manual nor auto-rinse clears it. Rename to
  something like `session_used` or `has_brewed`
- [ ] Tag as major version (v2.0.0)
- [ ] Update all docs + YAML examples
- [ ] Provide migration guide: old entity_ids stay in HA as `unavailable`,
  new ones start fresh — note that InfluxDB queries need to union both

---

## Maintenance

### M1 — GitHub Actions Node.js upgrade (before June 2026)
- [ ] Update `actions/checkout` to v5+ (Node.js 24 support)
- [ ] Update `actions/setup-python` to v6+
- [ ] Update `actions/cache` to v5+

---

## Open questions

- IC: bit 2 = idle/busy? (1 in standby+ready, 0 during rinsing/startup — hypothesis)
- IC: bit 6 oscillates between consecutive reads — cause unknown, unreliable
- RR:18/19 = unknown meaning (value ~0x34-0x35, static across states)
- RR:21 = brew group position? (changes during operations, not confirmed)
- EEPROM 0x000D = since descaling? (value 566, observe during descaling cycle)
- EEPROM 0x000F = unknown (value 78, observe)
- EEPROM 0x0010+ — never read, content unknown
- Auto-rinse on shutdown does NOT increment rinse counter (0x0007) — why?
- What is the `need_clean` threshold? (0x000E value when machine shows "Pflege drücken")
