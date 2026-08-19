---
id: EPIC-08
iteration: 3
title: Zero-CPU PIO+DMA state machine
status: todo
---

## Goal

Move the per-access path fully into PIO+DMA (Diego's preferred end-state, D-04),
removing the inter-access-gap timing race (C-03) entirely. Keep the Core1
version as reference/fallback.

## Scope

- In scope: state in a PIO scratch register, DMA-fed lookup + next-state
  feedback, output drive.
- Out of scope: further variant support.

## Stories

- STORY-01: PIO+DMA feedback design and implementation

## Notes

Mirrors romemul's within-access DMA proof, plus feedback of next-state into the
PIO register. LUT re-emitted in whatever layout the DMA indexing needs.
