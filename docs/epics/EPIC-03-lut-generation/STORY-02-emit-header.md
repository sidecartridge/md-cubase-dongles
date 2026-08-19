---
id: STORY-02
epic: EPIC-03
title: Emit cubase_lut.h in the house style
status: done
---

## Goal

Emit a self-contained `rp/src/include/cubase_lut.h` with the packed table, reset
constants, and decode macros, placed to live in SRAM.

## Tasks

- [x] Emit `uint16_t cubase_lut[NUM_STATES][2]` (non-const so it lands in SRAM, C-04)
- [x] Emit `CUBASE_LUT_NUM_STATES / RESET_STATE_ID / RESET_OUTPUT_WORD` and NEXT_STATE/D8/OUTPUT_WORD macros
- [x] Include the full-transition SHA-256 in the header comment for traceability
- [x] Compile-check the header standalone

## Acceptance

Header compiles clean under `gcc -std=c11 -Wall -Wextra`.

## Notes

Done 2026-08-19. `RESET_STATE_ID=0`, `RESET_OUTPUT_WORD=0xFEFF`,
`OUTPUT_WORD(e)` -> `0xFFFF`/`0xFEFF` mirroring MiSTer `{7'h7f,d8}` (D-06).
