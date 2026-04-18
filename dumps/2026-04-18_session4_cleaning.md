# Debug Session 4 — Cleaning Cycle + Per-Event Rate Series

Date: 2026-04-18
Machine: Jura Impressa F50 (reports as `ty:EPSILON MASK 11`)
Method: REST Debug API via `dumps/full_dump.sh` + `dumps/monitor_cleaning.sh`
Trigger: machine showed "Pflege drücken" for ~2 days without cleaning performed since session 2

---

## Executive summary

Session 4 pursued four goals and produced three major reclassifications:

1. **Identify the real Pflege (cleaning-required) trigger** — which EEPROM word exceeds
   which threshold when the display prompts for cleaning.
2. **Verify that existing `num_clean` / `num_coffees_since_clean` / `need_clean` sensors
   correctly reflect reality.**
3. **Produce per-event deltas for every brew type** so future correlations (strength
   counters, water amount, phase counters) have ground truth.
4. **Document the full methodology** so subsequent sessions (descaling, filter) follow
   the same protocol without re-discovery.

Key outcomes:

- **`num_coffees_since_clean` (mapped to 0x000E) is misnamed.** 0x000E is a
  `cups_today` counter with daily resets and a transient role during cleaning.
  The real "brews since cleaning" counter is `0x000F`.
- **`needs_rinse` (IC: bit 0) is misnamed.** It tracks "session has been used since
  cold start" — neither manual rinse, auto-rinse, nor power-off clears it. Only
  extended cool-down does.
- **F50 counter mapping was mapped incorrectly.** Physical coffee-button press count
  selects strength and lands on three different EEPROM addresses:
  1x press → 0x0000, 2x press → 0x0001, 3x press → 0x0002. Our current ESPHome
  sensor keys `num_single_espresso` / `num_double_espresso` / `num_coffee`
  correspond to these three addresses but with misleading names.
- **Auto-rinse-on-shutdown DOES increment the rinse counter** (0x0007, +1).
  This contradicts Session 2's conclusion — updated finding today supersedes it.
- **Session 2 EEPROM labels at 0x000D, 0x000E, 0x000F were all wrong.**
  Session 2 labelled 0x000D as "Coffees since cleaning ⚠️", 0x000E as "Unknown
  counter ⚠️", and 0x000F as "Coffees since descaling ✅". Session 4 shows:
  0x000D is a brew-event counter (not reset by cleaning), 0x000E is a daily
  cups counter (not reset by cleaning at all — transient value only), and
  **0x000F is the one that actually resets on cleaning**, so it tracks brews
  since cleaning, not descaling. Session 2's "✅" on 0x000F was extrapolation
  from a single +1 observation after a coffee, not from a descaling test.
- **`0x0005` is our leading Pflege-trigger hypothesis — not yet confirmed.**
  Byte-split interpretation: low byte `0x14` (20) stays constant (configuration),
  high byte ticks from 0 and gets cleared by cleaning. Pre-clean high byte was
  `0x02`, matching ~2 weeks of elapsed time since the previous cleaning (roughly
  "1 tick per week"). The time-based reading is inferred from the growth delta
  alone; no direct tick observed. Requires longitudinal confirmation — see
  Open questions.
- **`0x000F`, `0x0011`, `0x0016` are also reset by cleaning** but appear to
  represent different phenomena (brew-count-since-clean, unknown per-day counter,
  unknown per-session counter). Unlike `0x0005`, their growth rates match
  plausible event counts rather than pure elapsed time — though none of these
  event hypotheses are confirmed either.

The identification of which of these four counters is the primary trigger requires
**longitudinal observation** (Section "Open questions"); all four reset together
today so single-event data cannot distinguish them.

---

## Dump catalogue

All `.log` files live under `dumps/` locally; raw logs are `.gitignore`'d, so
the data below is inlined.

| Time  | Dump                                          | Description                                 |
| ----- | --------------------------------------------- | ------------------------------------------- |
| 11:04 | `2026-04-18_1104_before_cleaning.log`         | BEFORE baseline, machine on (after 1 rinse) |
| 11:56 | `2026-04-18_1156_during_cleaning.log`         | Monitor start (60s interval)                |
| 12:10 | `2026-04-18_1156_during_cleaning.log`         | Monitor end (14 polls captured)             |
| 12:11 | `2026-04-18_1211_after_cleaning.log`          | AFTER cleaning completed                    |
| 12:30 | `2026-04-18_1230_post_coffee_normal.log`      | Normal coffee (2× press, 100ml)             |
| 12:36 | `2026-04-18_1236_post_coffee_strong.log`      | Strong coffee (3× press, 55ml)              |
| 12:44 | `2026-04-18_1244_post_coffee_mild.log`        | Mild coffee (1× press, 30ml)                |
| 12:48 | `2026-04-18_1248_post_double_mild.log`        | Mild double (1× on double button, 75ml)     |
| 12:57 | `2026-04-18_1257_post_double_strong.log`      | Strong double (3× on double button, 100ml)  |
| 13:01 | `2026-04-18_1301_post_rinse.log`              | Manual rinse (FA:02)                        |
| 13:15 | `2026-04-18_1315_post_autorinse.log`          | Coffee + Power-off + Auto-rinse             |

---

## Phase 1 — Before-cleaning baseline

Machine on, 1 manual rinse done by user, "bereit / Pflege" alternating on display.

```
IC: ic:9EB01E00  /  ic:DEB01E00   (bit1 oscillates 0/1 under display-idle)
TY: ty:EPSILON MASK 11
RT:0000 rt:110C0C3E19550BB3000002140077536400460012167800030002029800000087
RT:1000 rt:87A6006B00EC0320C1FD42C9001D0000000000001E02012602560005000403DD
```

Parsed counters of interest (RT:0000 + RT:1000):

| Addr    | Value   | Current meaning (per repo docs)         |
| ------- | ------- | --------------------------------------- |
| 0x0000  | 4364    | `num_single_espresso` (= 1× press)      |
| 0x0001  | 3134    | `num_double_espresso` (= 2× press)      |
| 0x0002  | 6485    | `num_coffee` (= 3× press)               |
| 0x0003  | 2995    | `num_double_coffee` (double button)     |
| 0x0007  | 21348   | `num_rinse`                             |
| 0x0008  | 70      | `num_clean`                             |
| 0x0009  | 18      | `num_descale`                           |
| 0x000D  | 664     | unknown (grows per brew event)          |
| 0x000E  | 0       | labelled `num_coffees_since_clean`      |
| 0x000F  | 135     | unknown                                 |
| 0x0005  | 532     | labelled "Spezialkaffee" — always `0x0014` on recent dumps until now |
| 0x0010  | 34726   | unknown, long-term counter              |
| 0x0011  | 107     | unknown                                 |
| 0x0014  | 49661   | unknown                                 |
| 0x0015  | 17097   | unknown                                 |
| 0x0016  | 29      | unknown                                 |

**Paradox observed at this phase:**

- `IC: bit 1` = 1 (= "clean NOT needed" per inverted-logic label) — yet the display
  asks for cleaning.
- `0x000E` = 0 (= "0 coffees since cleaning" per current label) — yet user confirms
  no cleaning performed since 2026-04-03.
- `num_clean` (0x0008) = 70, unchanged since session 2, consistent with user report.

Both sensor labels `need_clean` and `num_coffees_since_clean` disagree with reality.

---

## Phase 2 — Monitor timeline during cleaning (11:56 → 12:10)

User pressed the cleaning button (with tablet) at ~11:58. 14 polls captured.
`0x0005`, `0x0008`, `0x000D`, `0x000E`, `0x000F` shown per poll:

```
time   IC byte0    bit1  0x0005  0x0008  0x000D  0x000E  0x000F
11:56  9E (idle)   1       532      70     664       0     135
11:57  DA          1       532      70     664       0     135
11:58  D8          0       532      70     664     250     135  ← 0x000E: 0 → 250 (cleaning start marker)
11:59  DE          1       532      70     664     250     135
12:00  D9          0       532      70     664     250     135
12:01  98          0       532      70     664     250     135
12:02  D8          0       532      70     664     250     135
12:03  D8          0       532      70     664     250     135
12:04  D8          0       532      70     664     250     135
12:05  99          0       532      70     664     250     135
12:06  D9          0        20      71     664     250       0  ← EEPROM commit: 0x0005/0x0008/0x000F
12:07  D8          0        20      71     664     250       0
12:08  D8          0        20      71     664     250       0
12:09  DE          1        20      71     664     250       0  ← IC bit1 back to 1
12:10  9A          1        20      71     671       0       0  ← 0x000D +7, 0x000E back to 0
```

Three distinct phases:

1. **11:58 — Cleaning-start signal.** `IC:bit1` goes to 0 (in the noisy sense —
   alternates with bit 1 = 1 readings). `0x000E` jumps to a hard-coded `250`
   (0xFA). This is probably a machine-internal "cleaning progress / water units"
   field, reused from what is otherwise a daily cups counter.
2. **12:06 — EEPROM commit.** `0x0005: 532 → 20`, `0x0008: 70 → 71` (num_clean +1),
   `0x000F: 135 → 0`. This is the persistent write that records the cleaning.
3. **12:09 — IC clearance.** `IC:bit1` settles back to 1. At 12:10, `0x000D` jumps
   +7 (likely 7 brew-phase events counted during the full cleaning cycle) and
   `0x000E` returns to 0.

---

## Phase 3 — After-cleaning snapshot (12:11)

Full-dump diff against Phase 1 shows the **complete set of EEPROM words mutated by
cleaning**:

| Addr    | Before  | After  | Δ       | Interpretation                                 |
| ------- | ------- | ------ | ------- | ---------------------------------------------- |
| 0x0005  | 532     | 20     | -512    | High byte zeroed — primary Pflege trigger candidate |
| 0x0008  | 70      | 71     | +1      | `num_clean` (cleaning counter, confirmed)      |
| 0x000D  | 664     | 671    | +7      | Counts 7 brew-phase events during cleaning     |
| 0x000E  | 0       | 0      | 0 (transient 250→0) | cups counter, hijacked for progress display |
| 0x000F  | 135     | 0      | -135    | Reset — "brews since cleaning" candidate       |
| 0x0010  | 34726   | 34733  | +7      | Mirrors 0x000D counting pattern                |
| 0x0011  | 107     | 0      | -107    | Reset — unknown phenomenon, ~7.6/day growth    |
| 0x0016  | 29      | 0      | -29     | Reset — unknown phenomenon, ~2/day growth      |

RT:1000 timeout reads stabilised post-cleaning — 0x0018 and 0x001C returned `0000`
and `0256` after previously returning `(no response)` at these addresses during
Phase 1. Still needs investigation, possibly timing-sensitive.

---

## Phase 4 — Per-event brew rate series

Seven events after cleaning, each followed by a full dump, diffed against the
post-cleaning baseline and each other:

```
Addr      PostClean  Norm 2x  Str 3x  Mild 1x   MDbl    SDbl    Rinse   AutoRinse
                      100ml    55ml    30ml      75ml    100ml           (after coffee)
0x0000         4364     4364    4364     4365     4365    4365    4365     4366  ← 1× press
0x0001         3134     3135    3135     3135     3135    3135    3135     3135  ← 2× press
0x0002         6485     6485    6486     6486     6486    6486    6486     6486  ← 3× press
0x0003         2995     2995    2995     2995     2996    2997    2997     2997  ← double button
0x0007        21348    21348   21348    21348    21348   21348   21349    21350  ← rinses, incl. auto-rinse
0x000D          671      672     673      674      675     678     678      679  ← per-brew event
0x000E            0        1       2        3        5       7       7        8  ← per-cup (Double = +2)
0x000F            0        1       2        3        4       5       5        6  ← per-brew command
0x0010        34733    34734   34735    34736    34737   34740   34740    34741  ← mirrors 0x000D
0x0014        49661    49667   49673    49678    49685   49691   49691    49696  ← brew phases (variable)
0x0015        17097    17098   17099    17100    17101   17102   17102    17103  ← per-brew command

0x0005           20       20      20       20       20      20      20       20  ← does not move
0x0011            0        0       0        0        0       0       0        0  ← does not move
0x0016            0        0       0        0        0       0       0        0  ← does not move
```

### Strength-by-press-count mapping (F50) — confirmed

| Press count | EEPROM address | Current ESPHome sensor key   | Semantically should be   |
| ----------- | -------------- | ---------------------------- | ------------------------ |
| 1× press    | 0x0000         | `num_single_espresso`        | `num_coffee_mild`        |
| 2× press    | 0x0001         | `num_double_espresso`        | `num_coffee_normal`      |
| 3× press    | 0x0002         | `num_coffee`                 | `num_coffee_strong`      |
| Double btn  | 0x0003         | `num_double_coffee`          | `num_coffee_double`      |

The physical coffee button's press count is strength selection on F50. No strength
variant exists for the double button — both mild-double (1× press) and strong-double
(3× press) increment 0x0003 and none of the other strength counters.

### Cups vs. brews (`0x000E` vs. `0x000F`)

- `0x000E` increments by **2 on double coffees, 1 on single** — it is a cups
  counter, not a brew counter. Resets daily (confirmed from InfluxDB 30-day
  history). Hijacked for a `0xFA` (250) progress value during cleaning.
- `0x000F` increments by **1 per brew command regardless of cups** — and is
  reset by cleaning. This is the real "brews since cleaning" counter.

### Brew-event counter anomaly (`0x000D`, `0x0010`)

- Single-brew events: +1 on both counters.
- Mild double: +1 on both (machine brews two cups as one operation).
- Strong double: **+3** on both. Best hypothesis: machine does an internal
  pre-flush between the two cups for quality, adding a third brew event.
  Not reproduced with mild double, so the trigger is press-count-related.
  Needs a second observation to confirm.

### `0x0014` phase counter (no water correlation)

| Event          | Water  | Δ 0x0014 |
| -------------- | ------ | -------- |
| Mild 1×        | 30ml   | +5       |
| Normal 2×      | 100ml  | +6       |
| Strong 3×      | 55ml   | +6       |
| Mild Double    | 75ml   | +7       |
| Strong Double  | 100ml  | +6       |
| Auto-rinse     | ?      | +5       |

Δ does not correlate with water volume (Mild Double 75ml gives +7 while Strong
Double 100ml gives only +6). Best remaining interpretation: fixed phase count
per product type, with Mild being one phase shorter than other single coffees.
Session 3's conclusion "water amount not accessible" stands — `0x0014` is not
the water sensor.

### Rinse behaviour — two corrections

1. **Commanded rinse (FA:02)** only increments `0x0007` (+1). No other EEPROM
   word or IC bit moves. None of `0x0005` / `0x0011` / `0x0016` tick. This
   refutes the hypothesis that `0x0011` is a rinse-derived counter.

2. **Auto-rinse on power-off DOES increment 0x0007** (+1). Session 2 had
   concluded "auto-rinse does NOT increment rinse counter" — on F50 today
   this is not true. The auto-rinse path after a coffee + power-off correctly
   records one rinse in 0x0007.

### Manual vs. auto rinse flag interaction

Confirmed by user: a manual rinse (FA:02) clears an internal "pending
auto-rinse" flag. If the machine is then powered off without another
brew, no auto-rinse runs on shutdown. If any brew occurs after the manual
rinse, the flag re-arms and the next shutdown triggers auto-rinse.

This internal flag is **not visible** via `IC:bit0`, which stays `1`
regardless of rinse history. `IC:bit0` is therefore better named
`session_used_since_cold_start`.

---

## The 0x0005 byte-split hypothesis (primary finding)

Pre-clean `0x0214` → post-clean `0x0014`. The low byte (`0x14` = 20)
is identical before and after; only the high byte changed from `0x02` to
`0x00`. This is not a counter reset, it is a masking operation:
`value &= 0x00FF`.

Interpretation:

- **Low byte (= 20)** is a stored configuration — possibly the Pflege-interval
  setting in some machine-internal unit. It has been `0x14` in every F50
  dump in the repository (sessions 2, 3, and early session 4 before cleaning
  completed).
- **High byte** is a time-based ticker that the cleaning cycle resets. Pre-clean
  value `0x02` = 2. User reports ~14 days since last cleaning. `2 = 14 days / 7`
  suggests the high byte increments **once per week** of elapsed time or
  operating time. Threshold: high byte ≥ 2 ⇒ display "Pflege drücken".

This would cleanly explain:

- Why neither coffees nor rinses move `0x0005` during Session 4 (it is time-based,
  not event-based).
- Why the display appeared ~2 days ago even though the cleaning counter and
  coffee counters had been quiet.
- Why the value reset to exactly `0x0014`, not to `0x0000` — the config byte is
  preserved.

This remains a **hypothesis pending observation**: deploying a sensor for the
high byte and watching its trajectory over the next 3–7 days in InfluxDB will
either confirm weekly ticks or rule them out.

---

## IC: byte 0 — revised semantic map

| Bit | Value 1 means                                | Value 0 means        | Confirmed from session 4                  |
| --- | -------------------------------------------- | -------------------- | ----------------------------------------- |
| 0   | Session has been used since cold start       | Cold-start state     | Not cleared by manual or auto rinse       |
| 1   | Not in cleaning cycle                        | Cleaning in progress | Stays 0 from 11:58 to 12:08 during clean  |
| 2   | Idle                                         | Busy                 | (Session 2, unchanged)                    |
| 3   | Tank OK                                      | Tank empty           | (Session 2, unchanged)                    |
| 4   | Tray inserted                                | Tray missing         | (Session 2, unchanged)                    |
| 5   | Always 0                                     | —                    | —                                         |
| 6   | Oscillates unreliably                        | —                    | Mask out in any consensus logic           |
| 7   | Always 1                                     | —                    | —                                         |

Bit 1 is not a "Pflege pending" flag (display prompt does not correspond to
bit 1 = 0); rather, it is "cleaning cycle actively running" — it goes to 0
when the user initiates the cleaning and back to 1 when the cycle completes.

The IC:bit1 InfluxDB history from past weeks (26 on-events, 51 off-events over
7 days) is consistent with real machine states (cleaning-in-progress during
brief internal cycles) rather than pure noise.

---

## Counter classification matrix (confirmed)

| Category                 | Address(es)                            | Per-event rate                                         |
| ------------------------ | -------------------------------------- | ------------------------------------------------------ |
| Single coffee by strength| 0x0000 (1×), 0x0001 (2×), 0x0002 (3×)  | +1 on matching-strength coffee                         |
| Double coffee            | 0x0003                                 | +1 on any double (no strength variant)                 |
| Cups                     | 0x000E                                 | +1 single, +2 double; daily reset; 0xFA during clean   |
| Brew command             | 0x000F, 0x0015                         | +1 per brew command                                    |
| Brew event               | 0x000D, 0x0010                         | +1 normally, +3 on strong double                       |
| Brew phase (per-product) | 0x0014                                 | +5 mild, +6 normal/strong/sd, +7 mild double           |
| Rinse counter            | 0x0007                                 | +1 on FA:02 AND on auto-rinse                          |
| Cleaning counter         | 0x0008                                 | +1 per cleaning cycle                                  |
| Descaling counter        | 0x0009                                 | +1 per descale (expected, not exercised this session)  |
| Cleaning-reset set       | 0x0005 HB, 0x000F, 0x0011, 0x0016      | All cleared on cleaning, none move on brew/rinse — which one is the Pflege trigger is an open question |

---

## Open questions (require observation, not further interactive tests)

These cannot be settled by single events because the candidates are either time-based
or tick on rare/unreproducible internal events. They are resolved by **deploying
sensors, then watching InfluxDB trajectories** over days/weeks.

- **Which of `0x0005 HB`, `0x000F`, `0x0011`, `0x0016` is the actual Pflege
  trigger?** All four reset on cleaning today. Observation will show their
  per-day growth rates and the value at which the next "Pflege drücken" appears.
- **What drives `0x0011` growth (~7.6/day historically)?** Not coffees, not manual
  rinse, not auto-rinse. Possibly heater cycles, brew-group movements, or specific
  internal maintenance ticks.
- **What drives `0x0016` growth (~2/day historically)?** Possibly power-on sessions
  (user turns machine on morning + evening) or sleep/wake cycles.
- **`0x0005 HB` weekly-tick hypothesis** — does the high byte increment by `1`
  exactly every 7 days, or does it accumulate operating-time units? A 14-day
  observation window will distinguish.
- **Strong-double `+3` on 0x000D** — is this reproducible, or an artefact of the
  specific state the machine was in today? Rerun on a different day.

---

## Appendix — Session 5 protocol (descaling)

The method below is the re-usable pattern for the next protocol observation
(descaling / Entkalken). Written in the imperative as a check-list so either
a human or an agent can execute it. The dump tooling it relies on
(`dumps/full_dump.sh`, `dumps/monitor_cleaning.sh`, `dumps/README.md`) lives
under `dumps/` in the repo.

1. **Prerequisites.**
   - Firmware flashed with `-DUSE_DEBUG_HTTP`
   - REST API reachable at `jura-coffee-f50.local:8080`
   - `dumps/full_dump.sh` and `dumps/monitor_cleaning.sh` executable
   - Machine powered on, display showing "Entkalken" prompt OR user willing to
     trigger the cycle regardless of prompt

2. **Baseline.**
   - `dumps/full_dump.sh before_descale`
   - Inspect `num_descale` (0x0009) and note the pre-run value
   - Snapshot `0x0005`, `0x000F`, `0x0011`, `0x0016` — these were cleaning-related
     in session 4; descaling may reset a *different* set

3. **Monitor during descale.**
   - `dumps/monitor_cleaning.sh &` (renames to `during_descale` optional by
     editing the script once, or accept the filename)
   - Descaling typically runs 30–45 minutes with several user interactions
     (add descaling agent, press continue, refill tank, etc.)

4. **Post-descale.**
   - `touch dumps/STOP_MONITOR` when display returns to ready
   - `dumps/full_dump.sh after_descale`
   - Diff using the Python template in `dumps/README.md`

5. **Expected findings.**
   - `0x0009` should increment +1 (confirms `num_descale` sensor)
   - A **different** set of 2–4 EEPROM words should reset to 0 (or high-byte
     zeroed) — these are the descaling analogue of our cleaning-reset set
   - If any of `0x0005` / `0x000F` / `0x0011` / `0x0016` also reset: they are
     "any-maintenance" counters, not cleaning-specific
   - The InfluxDB `need_cleaning` sensor is unlikely to be relevant — look for
     a comparable "need_descale" bit in `IC:byte0` or elsewhere, which we have
     not yet investigated

6. **Writeup.**
   - Create `dumps/2026-MM-DD_session5_descaling.md` following this document's
     structure: summary, dump catalogue, phases, classification updates, open
     questions, and cross-links to session 4 for comparison.

---

## Cross-references

- Main README: [REST Debug API](../README.md#rest-debug-api)
- Workflow docs: [dumps/README.md](README.md)
- Protocol reference: [docs/jura-protocol.md](../docs/jura-protocol.md) —
  reconciled with Session 4 findings in PR #8
- TODO tracking: [TODO.md](../TODO.md) — A3 (0x000E re-classification) and A4
  (post-observation rename plan) both reference this session
