---
id: STORY-03
epic: EPIC-03
title: Exhaustive + behavioral LUT equivalence
status: done
---

## Goal

Prove the emitted table is behaviorally identical to the model.

## Tasks

- [x] LUT vs model on all reachable transitions (~12300), zero mismatches
- [x] Lockstep model-vs-LUT over a 2 M-step pseudo-random A8 walk
- [x] Emit golden regression vectors (const0, const1, alt, random256) as A8 -> D8 traces

## Acceptance

All reachable transitions and the 2 M random walk match; golden vectors written.

## Notes

Done 2026-08-19. `verify_lut` + `emit_vectors`. Vectors in
`tools/vectors/cubase3.txt`; const0 shows D8=0 for 12 accesses then 1 at `FFFF`.
