---
id: EPIC-05
iteration: 2
title: Stateful dongle engine (Core1 busy-poll)
status: todo
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
