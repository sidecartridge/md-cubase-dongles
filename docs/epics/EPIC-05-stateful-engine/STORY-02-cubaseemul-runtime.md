---
id: STORY-02
epic: EPIC-05
title: cubaseemul.c runtime (Core1 busy-poll)
status: done
---

## Goal

The Core1 handler reads A8, does the LUT step, and stages the next output word,
holding the correctness invariant.

## Tasks

- [x] Core1 busy-poll: `v=rxf; a8=(v>>8)&1; e=lut[state][a8]; state=e&0x1FFF; txf = (e>>13)&1 ? 0xFFFF : 0xFEFF;`
- [x] Pre-stage RESET_OUTPUT_WORD into TX before enabling the SM; init state = RESET_STATE_ID
- [x] LUT in SRAM (C-04); handler `__not_in_flash_func`
- [x] Validate the D8 stream against golden vectors for a scripted A8 sequence on hardware

## Acceptance

On hardware, the D8 stream matches the golden vectors for a scripted A8 input.

## Notes

Core1 (not the ~10 Hz foreground loop). SELECT in dongle mode must use the
Core0-only poll (EPIC-06 STORY-03), since Core1 is busy.
