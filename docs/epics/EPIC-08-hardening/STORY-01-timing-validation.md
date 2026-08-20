---
id: STORY-01
epic: EPIC-08
title: Timing & correctness validation
status: done
---

## Goal

Confirm the remaining proposed decisions against real hardware behavior.

## Tasks

- [x] Confirm Cubase's read bursts are served correctly (functional, C-02)
- [x] Finalize the pin15 reset-universe decision against observed behavior (D-05)
- [x] Validate the 0xFEFF/0xFFFF full-word drive is masked to D8 by Cubase (D-06)

## Acceptance

The proposed decisions D-06 (and any residual D-05 concern) move to accepted, or
are revised with evidence.

## Outcome

All three decisions are **accepted** (see DECISIONS.md), validated functionally
(C-02, no logic analyzer):

- **Read bursts (C-03)** — served correctly. EPIC-07's PIO+DMA engine serves
  each access entirely within the access (no between-access compute), so the
  inter-access-gap race is gone by construction. Two applications with different
  read patterns run to completion.
- **pin15 (D-05)** — behaviorally moot; proven on the host model (both power-on
  universes converge on identical D8) and never observed as an issue.
- **Full-word drive → D8 (D-06)** — confirmed by **two** independent titles,
  **Cubase 3.10 and Cubase Score 2.0**, both accepting the dongle on **both** the
  Core1 (EPIC-05) and PIO+DMA (EPIC-07) engines. Strong functional evidence the
  ST masks the read to D8.

DECISIONS.md was reconciled with the EPIC-07 as-built engine (new D-09; D-03/
D-04/C-03/C-05 updated).

## Notes

Depends on a working engine (EPIC-05 or EPIC-07).
