---
id: STORY-02
epic: EPIC-08
title: Cubase Score support (optional)
status: done
---

## Goal

Ship the same dongle for Cubase Score, which uses byte-identical dongle math.

## Tasks

- [x] Confirm CUBSCORE.S dongle math matches CUBASE.S (same LUT) — **empirically
      confirmed: Cubase Score 2.0 runs on the shipping red-dongle firmware with
      the SAME LUT, no changes (2026-08-20)**
- [x] App identity for the Score variant — **decided: NO separate identity;
      Cubase Score 2.x ships covered by the Cubase V3 dongle (Diego, 2026-08-20)**
- [x] Groundwork for future variants — the menu catalogue already structures the
      four families (Cubase V1 / V2 black / V3 / Audio Falcon), extensible by
      flagging a family `available` and pointing the engine at its LUT

## Acceptance

Cubase Score runs — it does, under the Cubase V3 dongle (listed as "Cubase Score
2.x" in the V3 family), with no separate app.

## Status

Done. Cubase Score 2.0 runs on the current firmware with no changes, and the
decision is that it ships **covered by the Cubase V3 dongle** — no separate
catalogue entry or UUID for v1.

## Notes

CUBSCORE.S differs from CUBASE.S only in patch offsets/text; the dongle routine
is identical.
