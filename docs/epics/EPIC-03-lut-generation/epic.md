---
id: EPIC-03
iteration: 1
title: LUT generation, table & verification
status: done
---

## Goal

Turn the verified model into the compact runtime table `cubase_lut.h`, prove the
table reproduces the model, and leave a source-free regression that CI can run.

## Scope

- In scope: reachable-state renumbering, packed-uint16 table, header emission
  (house style), exhaustive + behavioral LUT verification, golden vectors, and a
  CI regression that needs no raw sources.
- Out of scope: any RP2040 firmware consuming the header (Iteration 2).

## Stories

- STORY-01: Enumerate and renumber
- STORY-02: Emit cubase_lut.h
- STORY-03: Exhaustive + behavioral equivalence
- STORY-04: CI-safe verification without raw sources

## Notes

Packed entry: bits0..12 = next state id (13 bits), bit13 = D8 of the next state.
`cubase_lut[6150][2]` = 24 600 bytes, SRAM-resident (C-04).
