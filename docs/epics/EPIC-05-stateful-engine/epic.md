---
id: EPIC-05
iteration: 2
title: Stateful dongle engine (Core1 busy-poll)
status: done
---

## Goal

Serve real dongle reads from the verified LUT: PIO drives the pre-staged output
word and samples A8; a dedicated Core1 handler advances the state machine and
stages the next word (D-04).

## Scope

- In scope: the final `cubaseemul.pio`, `cubaseemul.c` runtime, LUT-in-SRAM, and
  build registration.
- Out of scope: mode switching / UX (EPIC-06).

## Stories

- STORY-01: Final cubaseemul.pio
- STORY-02: cubaseemul.c runtime (Core1 busy-poll)
- STORY-03: Build registration

## Notes

Correctness invariant: during access N the bus shows W(S_N), S_{k+1}=F(S_k,A8_k),
W(S)=0xFFFF if D8 else 0xFEFF. Pre-stage W(reset) before enabling the SM. C-03,
C-04 apply.

## Outcome (DONE 2026-08-20)

**Real Cubase 3.10 accepts the emulated dongle and runs.** The engine: a new
`cubaseemul.pio` (16-instr ROM3 dance) + `cubaseemul.c` (Core1 loop:
`state = cubase_lut[state][a8]`, stage next output word). `cubaseemul_start()` is
the `[F]irmware` mode commit — it calls `commemul_deinit()` to free pio0, stands
the dongle SM up from reset (romemul stays for ROM4), and launches Core1. The
shipping `userfw.s` just boots GEM; the dongle is answered entirely by PIO+Core1.
Verified in two steps on hardware: (1) a const-A8=0 peek read `FEFF x12` then
`FFFF x4`, matching the golden vector; (2) Cubase 3.10 runs. D-06 (full-word
`0xFEFF/0xFFFF` drive) confirmed accepted by Cubase.
