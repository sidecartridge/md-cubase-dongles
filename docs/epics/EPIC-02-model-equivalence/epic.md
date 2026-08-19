---
id: EPIC-02
iteration: 1
title: Three independent models + equivalence
status: done
---

## Goal

Build the dongle's `step(state, a8) -> next_state` from three independent
sources and prove they agree on every transition — so what we implement is the
silicon's behavior, not one transcription of it. Also settle the pin15 power-on
question.

## Scope

- In scope: three parsed models (equations / fuse map / m68k crack), an
  exhaustive equivalence proof, and the reachable-state / pin15 analysis.
- Out of scope: the packed LUT and its emission (EPIC-03).

## Stories

- STORY-01: Implement three reference simulators
- STORY-02: Resolve reset / pin15 power-on ambiguity

## Notes

State bit map (shared by all models and the runtime): bit0..7 = pin03..10,
bit8..14 = pin15..21, bit15 = pin22 (D8); input a8 = pin14.
