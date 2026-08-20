---
id: EPIC-04
iteration: 2
title: ROM3-drive bring-up (gate #1)
status: done
---

## Goal

Before building any state machine, prove the one CRITICAL unknown: can the
MultiDevice *drive* data on a ROM3 read at all? `commemul` only ever captures on
ROM3, so ROM3-driving is unexercised on this hardware (D-07, C-01).

## Scope

- In scope: a minimal PIO program that drives a fixed word on every ROM3 access
  and a way to read it back on the ST.
- Out of scope: the LUT/state machine (EPIC-05).

## Stories

- STORY-01: Minimal fixed-word ROM3 drive

## Notes

If the ST cannot read back the fixed word, the data transceiver's direction is
gated by ROM4 chip-select and the dongle must move to ROM4 — a design pivot.
This is why the epic runs first. Needs Diego's MultiDevice + ST (C-02).
