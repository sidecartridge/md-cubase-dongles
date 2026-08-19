# Iterations

Epics are grouped into iterations via the `iteration: N` frontmatter field; the
cockpit groups the dashboard by it. This file is the narrative: what each
iteration set out to do and how it ended.

| Iteration | Theme | Status |
| --- | --- | --- |
| 1 | Verified LUT generation (host-only) — turn three reverse-engineered hardware descriptions into one proven-correct 24 KB state-machine table, with no hardware and no bus risk | complete |
| 2 | RP2040 dongle engine (Core1 busy-poll) — a working, correct Cubase 3 dongle on real hardware using the simple correctness-first engine | not started |
| 3 | Pure PIO+DMA engine (target end-state) + hardening — move the per-access path fully into PIO+DMA (zero CPU), removing the timing race entirely | not started |

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

**Outcome:** _pending._ Starts with EPIC-04 (gate #1): prove the MultiDevice can
*drive* data on a ROM3 read at all before building the state machine — this is
the one critical unknown that needs Diego's MultiDevice + ST (D-07, C-01).

**Epics (planned order)**

| Epic | Status | Note |
| --- | --- | --- |
| EPIC-04 ROM3-drive bring-up | todo | Gate #1 — fixed-word drive smoke test; a fail means the dongle must move to ROM4 |
| EPIC-05 stateful engine | todo | `cubaseemul.pio` + `.c` Core1 busy-poll + build registration |
| EPIC-06 mode integration & UX | todo | setup↔dongle mode branch, SELECT exit, no ROM4 autorun |
| EPIC-07 app identity & release build | todo | `desc/app.json`, version, UUID; full build; real Cubase acceptance |

---

## Iteration 3: Pure PIO+DMA engine (target end-state) + hardening

**Goal:** Move the per-access path fully into PIO+DMA (zero CPU), removing the
inter-access-gap timing race entirely (C-03). This is Diego's preferred
end-state; the Core1 version stays as reference/fallback.

**Outcome:** _pending._

**Epics (planned)**

| Epic | Status | Note |
| --- | --- | --- |
| EPIC-08 zero-CPU PIO+DMA state machine | todo | state in a PIO register; DMA feeds back next-state; identical D8 vs the Core1 build |
| EPIC-09 hardening | todo | burst-timing validation, pin15/full-word confirmation, optional Cubase Score |

Same working rules throughout: strictly sequential, a hardware-verification
checkpoint after each epic, one branch per epic, merged only after Diego
verifies.
