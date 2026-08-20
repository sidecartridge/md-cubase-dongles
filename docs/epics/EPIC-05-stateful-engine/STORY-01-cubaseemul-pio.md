---
id: STORY-01
epic: EPIC-05
title: Final cubaseemul.pio
status: done
---

## Goal

A PIO program that, per ROM3 access, samples A8 into the RX FIFO and drives the
pre-staged output word from the TX FIFO.

## Tasks

- [x] Clone romemul's sample -> flip-pindirs -> drive -> release structure (minus the DMA lookup)
- [x] Sample the address early and push to RX FIFO so Core1 gets maximum compute margin
- [x] Autopull the pre-staged word and drive it with !WRITE; release to Hi-Z on ROM3 inactive
- [x] Confirm the program assembles to <= 32 words and loads on pio0/sm0 with romemul/commemul absent

## Acceptance

Program assembles within budget and drives/samples correctly on hardware.

## Notes

Reuse the shared `.define` values (ACTIVE/INACTIVE, side-set encodings,
READ_ADDRESS_SAFE_WAIT_CYCLES).
