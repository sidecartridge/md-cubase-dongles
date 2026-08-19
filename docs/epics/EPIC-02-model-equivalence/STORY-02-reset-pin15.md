---
id: STORY-02
epic: EPIC-02
title: Resolve reset / pin15 power-on ambiguity
status: done
---

## Goal

Determine whether the under-specified pin15 power-on value can affect the
dongle's observable output, and pick the runtime reset accordingly (D-05).

## Tasks

- [x] BFS reachable set from reset for pin15=0 (expect ~5999) and pin15=1 (~6149)
- [x] Compute the union (~6150) and confirm closure (every transition stays in-set)
- [x] Test whether the two power-on roots emit the same D8 stream for equal A8 input
- [x] Choose the runtime reset constant and document it (D-05)

## Acceptance

Reachable counts are 5999 / 6149 / 6150; the union is closed; the two universes
converge on identical D8; runtime reset = pin15=0.

## Notes

Done 2026-08-19. Converged over 1 M pseudo-random steps -> D-05. The union LUT
covers both universes regardless, at a cost of ~600 bytes.
