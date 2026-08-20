---
id: STORY-01
epic: EPIC-08
title: Timing & correctness validation
status: todo
---

## Goal

Confirm the remaining proposed decisions against real hardware behavior.

## Tasks

- [ ] Confirm Cubase's read bursts are served correctly (functional, C-02)
- [ ] Finalize the pin15 reset-universe decision against observed behavior (D-05)
- [ ] Validate the 0xFEFF/0xFFFF full-word drive is masked to D8 by Cubase (D-06)

## Acceptance

The proposed decisions D-06 (and any residual D-05 concern) move to accepted, or
are revised with evidence.

## Notes

Depends on a working engine (EPIC-05 or EPIC-08).
