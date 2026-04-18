# TODO

Last updated: 2026-04-18

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
- [x] IC: bit1 = cleaning-cycle-running (inverted): bit goes to 0 during
  the active cleaning cycle (session 4, 2026-04-18). Earlier believed to
  indicate the Pflege display prompt — superseded.
- [x] IC: bit0 = session-used-since-cold-start (sensor misnamed `needs_rinse`):
  0 after cold start, 1 after first coffee. Not cleared by manual rinse,
  auto-rinse, or a brief power cycle (session 4).
- [x] EEPROM num_rinse (0x0007): confirmed — increments on FA:02 AND on
  auto-rinse on shutdown (session 4 supersedes session 2's claim)
- [x] EEPROM 0x000E: session 4 reclassified as a cups counter with daily
  resets, NOT coffees-since-cleaning. Sensor key `num_coffees_since_clean`
  is misnamed — target rename in v2.0 (see A4). The real brews-since-clean
  counter is 0x000F.
- [x] EEPROM num_clean (0x0008): confirmed +1 per cleaning cycle (session 4)
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

### F9 — Coffee strength (research) ✅ ANSWERED
- [x] Session 4 showed the physical button press count selects strength
  directly on the machine and maps to three different EEPROM addresses:
  1× → `0x0000`, 2× → `0x0001`, 3× → `0x0002`. The `FA:06` command
  always hits `0x0000` (1×-press equivalent). There is no FA: command
  variant for strength selection over the service port — controlling
  strength from HA would require either physical button simulation
  (hardware-level) or the FN:07/08 manual-grind fallback.
- See session 4 per-event rate series: `dumps/2026-04-18_session4_cleaning.md`

---

## Debug Sessions (planned)

### S3 — Water amount EEPROM mapping ✅ DONE (not found)
- [x] Scanned RT:0000–9000 (10 pages, 160 EEPROM words) at 55ml and 240ml — identical
- [x] Scanned RE:00–9F (160 individual words) at 55ml and 240ml — identical
- [x] Scanned RR:00–FF (256 RAM registers) at 30/55/120/240ml — only volatile changes
- [x] Discovered RT:1000–9000 pages and RE:20+ / RR:24+ extended ranges
- [x] **Result: water amount not in any accessible address space**

### S4 — Coffee strength test ✅ DONE (session 4, 2026-04-18)
- [x] Per-event rate series across 1×, 2×, 3× press single coffees and
  both double-coffee variants — confirmed strength maps to
  `0x0000` / `0x0001` / `0x0002` respectively
- [x] Double button has no strength variant (both mild-double and
  strong-double increment `0x0003`)
- Report: `dumps/2026-04-18_session4_cleaning.md` (Phase 4)

### S5 — Cleaning cycle observation ✅ DONE (session 4, 2026-04-18)
- [x] Full before/during/after dump of the cleaning cycle
- [x] `num_clean` (0x0008) confirmed +1
- [x] Cleaning-reset set identified: `0x0005` high byte, `0x000F`,
  `0x0011`, `0x0016` — all cleared by cleaning
- [x] Found that `0x000E` was misclassified — it is a cups counter, not
  coffees-since-cleaning. The real brews-since-cleaning counter is
  `0x000F`. The original "0x000E threshold" question is void.
- [ ] Which of the four cleaning-reset candidates is the primary Pflege
  trigger is open — requires longitudinal observation (A5)
- Report: `dumps/2026-04-18_session4_cleaning.md`

### S6 — EEPROM 0x000D / 0x000F identification ✅ DONE (session 4, 2026-04-18)
- [x] `0x000D`: brew-event counter, +1 per brew command, +3 on
  strong-double (internal pre-flush), not reset by cleaning
- [x] `0x000F`: brews since cleaning, +1 per brew command, reset by
  cleaning cycle — supersedes session 2's "coffees since descaling"
  label which was speculative

### S7 — Extended EEPROM (RT:0010+) ✅ DONE (session 3, 2026-04-03)
- [x] RT:1000 through RT:9000 mapped (sessions 3 and 4)
- [x] Notable RT:1000 words documented in `docs/jura-protocol.md`
- [x] Water amount not found in any extended EEPROM (F8)

### S8 — Descaling cycle observation (planned)
- [ ] Run the protocol from `dumps/2026-04-18_session4_cleaning.md`
  appendix (reusable checklist written during session 4)
- [ ] Expected: `num_descale` (0x0009) +1, plus a **different** set of
  2–4 EEPROM words reset. Compare against the cleaning-reset set to see
  if any counter is "any-maintenance" rather than cleaning-specific.
- [ ] Writeup: `dumps/YYYY-MM-DD_session5_descaling.md`

---

## Long-term (MAX)

### A1 — C++ startup sequence → ✅ replaced by YAML script (F2)

### A2 — HA automations
- [ ] Automation: `need_clean = true` → push notification (note: the
  underlying IC:bit1 signal is "cleaning-cycle-running", not
  "cleaning-pending" — use the EEPROM-based threshold sensor once
  available from session 5 observation)
- [ ] Automation: brews-since-cleaning (0x000F) > threshold → warning
  notification. Needs new sensor (A5).
- [ ] Automation: daily coffee start (e.g. 7:30) using Start & Rinse script
- [ ] Blueprint for coffee routine

### A3 — Identify EEPROM 0x000A–0x000F ✅ DONE (session 4, 2026-04-18)
- [x] 0x000D = brew-event counter (supersedes session 2's "coffees since
  cleaning" label)
- [x] 0x000E = cups counter with daily reset + transient 0xFA during
  cleaning (supersedes session 2's "unknown counter" and ESPHome sensor
  name `num_coffees_since_clean`)
- [x] 0x000F = brews since cleaning (supersedes session 2's "coffees
  since descaling" label)
- [ ] 0x000A, 0x000B, 0x000C — still unknown, no session-4 movement

### A5 — Confirm Pflege trigger via longitudinal observation

Gates v2.0 (A4) and the HA automation in A2. Depends on Branch 4 component
changes exposing the four cleaning-reset candidates as sensors.

- [ ] Deploy sensors for `0x0005` (byte-split), `0x000F`, `0x0011`, `0x0016`
- [ ] Dashboard or InfluxDB query: per-day growth rate of each candidate
- [ ] Observe until the next "Pflege drücken" prompt — record which counter
  is furthest above its post-session-4 baseline at that moment
- [ ] Validate the `0x0005` weekly-tick hypothesis specifically: high byte
  should increment by 1 roughly every 7 days regardless of usage
- [ ] Resolution feeds A4 (the correct Pflege-related sensor name) and A2
  (the automation threshold)

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

- IC: bit 2 = idle/busy? (1 in standby+ready, 0 during rinsing/startup — hypothesis)
- IC: bit 6 oscillates between consecutive reads — cause unknown, unreliable
- IC: bit 0 — what duration of cold-down is required to clear it? (extended
  sessions observed, exact threshold unknown)
- RR:18/19 = unknown meaning (value ~0x34-0x35, static across states)
- RR:21 = brew group position? (changes during operations, not confirmed)
- Which of `0x0005` HB / `0x000F` / `0x0011` / `0x0016` is the primary
  Pflege trigger? All four reset on cleaning (session 4). Resolved by
  longitudinal observation post-sensor-deployment.
- Does `0x0005` high byte tick by calendar time (1/week), operating
  time, or something else? (session 4 hypothesis: weekly)
- What drives `0x0011` growth (~7.6/day)? Not coffee, not manual rinse,
  not auto-rinse (session 4).
- What drives `0x0016` growth (~2/day)? Plausibly power-on sessions,
  unconfirmed.
- Is the strong-double `0x000D +3` anomaly (internal pre-flush between
  cups) reproducible? Session 4 saw it once.
- RR:10 dropped from 0x8000 to 0x0000 during cleaning — is it a volatile
  cleaning-pending flag, or unrelated?
