# Iterations

Epics are grouped into iterations via the `iteration: N` frontmatter field; the
cockpit groups the dashboard by it. This file is the narrative: what each
iteration set out to do and how it ended.

| Iteration | Theme | Status |
| --- | --- | --- |
| 1 | Verified LUT generation (host-only) — turn three reverse-engineered hardware descriptions into one proven-correct 24 KB state-machine table, with no hardware and no bus risk | complete |
| 2 | RP2040 dongle engine (Core1 busy-poll) — a working, correct Cubase 3 dongle on real hardware using the simple correctness-first engine | complete |
| 3 | Pure PIO+DMA engine (target end-state) + hardening + release — move the per-access path fully into PIO+DMA (zero CPU), removing the timing race entirely, then ship | not started |

---

## Iteration 1: Verified LUT generation (host-only)

**Goal:** Produce a `cubase_lut.h` that is *provably* the same state machine as
the real Intel 5C060 silicon, cross-checked against three independent hardware
descriptions before any firmware exists. De-risk correctness first; write no
RP2040 code.

**Outcome:** Complete. `tools/generate_lut.py` builds three independent models —
the de-fused equation listing (A), a from-scratch decode of the raw JEDEC fuse
map (B, the actual silicon), and the Medway Boys m68k crack (C) — and proves
**A == B == C on all 131072 transitions**. From the silicon model it enumerates
the reachable states (5999 for pin15=0, 6149 for pin15=1, 6150 union), packs
them into a 24 600-byte `uint16_t cubase_lut[6150][2]`, and verifies the table
against the model on every reachable transition plus a 2 M-step random walk.
`tools/test_lut.py` is a source-free regression (replays golden vectors against
the committed header) and runs in CI. The raw sources are not committed; their
SHA-256s live in `tools/reference/PROVENANCE.md` (D-02). Notable finding: the
pin15 power-on ambiguity is behaviorally moot — the two universes converge on
identical D8 output (D-05), so the reset choice cannot affect Cubase.

**Epics**

| Epic | Status | Note |
| --- | --- | --- |
| EPIC-01 source ingestion & provenance | done | 3 sources hashed; from-scratch JEDEC fuse decoder matches the equation listing |
| EPIC-02 three models + equivalence | done | A==B==C on all 131072 transitions; pin15 universes converge (D-05) |
| EPIC-03 LUT generation & verification | done | 6150-state / 24.6 KB table; verified on all reachable + 2 M random walk; CI regression wired in |

---

## Iteration 2: RP2040 dongle engine (Core1 busy-poll)

**Goal:** A working, correct dongle on real hardware using the simple engine.
The design is modal (setup mode vs dongle mode), forced by the PIO instruction
budget (C-01). The per-access response lives entirely in PIO plus a dedicated
Core1 handler; the ~10 Hz foreground loop is far too slow to serve the bus.

**Outcome:** Complete. Gate #1 passed (the MultiDevice drives ROM3 — the ST
reads the driven word back), the Core1 busy-poll engine serves the full 5C060
state machine, and **real Cubase 3.10 runs on the emulated dongle**. EPIC-06
added the always-on boot menu (dongle selection + auto-boot countdown, copied
from md-drives-emulator) and slimmed the app to the cartridge-bus essentials.
App identity & release moved to Iteration 3 so it ships after the PIO+DMA
end-state and hardening.

**Epics**

| Epic | Status | Note |
| --- | --- | --- |
| EPIC-04 ROM3-drive bring-up | done | Gate #1 passed — ST reads the driven word from ROM3 |
| EPIC-05 stateful engine | done | `cubaseemul.pio` + `.c` Core1 busy-poll; real Cubase 3.10 runs |
| EPIC-06 boot menu, dongle selection & slim-down | done | boot menu + countdown + SELECT; dropped network/microSD/USB/LED |

---

## Iteration 3: Pure PIO+DMA engine (target end-state) + hardening

**Goal:** Move the per-access path fully into PIO+DMA (zero CPU), removing the
inter-access-gap timing race entirely (C-03). This is Diego's preferred
end-state; the Core1 version stays as reference/fallback.

**Outcome:** _pending._

**Epics (planned)**

| Epic | Status | Note |
| --- | --- | --- |
| EPIC-07 zero-CPU PIO+DMA state machine | done | state in PIO Y; 2 chained DMAs fetch + feed back next-state; romemul freed for pio0; Cubase 3.10 accepts it |
| EPIC-08 hardening | todo | burst-timing validation, pin15/full-word confirmation, optional Cubase Score |
| EPIC-09 app identity & release build | todo | `desc/app.json`, version, UUID; full build; real Cubase acceptance — done last |

Same working rules throughout: strictly sequential, a hardware-verification
checkpoint after each epic, one branch per epic, merged only after Diego
verifies.
