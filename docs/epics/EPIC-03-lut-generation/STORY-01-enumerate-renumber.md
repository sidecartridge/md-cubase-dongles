---
id: STORY-01
epic: EPIC-03
title: Enumerate reachable states and renumber to compact ids
status: done
---

## Goal

Collapse the 16-bit physical state space to only the reachable states, renumber
them to 13-bit ids, and pack each transition into a uint16.

## Tasks

- [x] BFS from the reset union universe; assign compact ids 0..N-1
- [x] Pack lut[id][a8] = next_id | (D8_of_next << 13)
- [x] Assert N <= 8192 (13-bit id fits) and the renumber is bijective
- [x] Assert every packed entry decodes back to the model's (next_id, D8)

## Acceptance

6150 states, ids 0..6149, all entries < 2^14; closure holds.

## Notes

Done 2026-08-19 (`build_lut` in `tools/generate_lut.py`). Generated from the
silicon model (B).
