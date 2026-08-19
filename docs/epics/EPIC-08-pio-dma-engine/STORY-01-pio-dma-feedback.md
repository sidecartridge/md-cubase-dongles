---
id: STORY-01
epic: EPIC-08
title: PIO+DMA feedback design and implementation
status: todo
---

## Goal

The dongle responds with zero CPU involvement on the response path.

## Tasks

- [ ] Keep state in a PIO scratch register (Y); form the index (state, a8)
- [ ] DMA fetch a packed record {output_word, next_state} from the LUT
- [ ] Drive output_word; feed next_state back into Y via a DMA -> FIFO round-trip
- [ ] Verify identical D8 behavior to the Core1 build across all golden vectors + Cubase acceptance

## Acceptance

On hardware, D8 behavior is identical to EPIC-05, with no CPU on the response
path.

## Notes

Only pursue if EPIC-05 works; escalate here if Cubase bursts prove too tight for
Core1 (C-03).
