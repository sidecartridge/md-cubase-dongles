---
id: EPIC-07
iteration: 2
title: App identity & release build
status: todo
---

## Goal

Give the app its published identity and confirm a full build boots and that real
Cubase 3.10 accepts the emulated dongle.

## Scope

- In scope: `desc/app.json`, `version.txt`, UUID, full `build.sh`, and the
  end-to-end Cubase acceptance test.
- Out of scope: the pure-PIO+DMA engine (Iteration 3).

## Stories

- STORY-01: Fill app identity
- STORY-02: Full build + acceptance

## Notes

The m68k cartridge (used only by setup mode) is unchanged and must still fit the
8 KB budget.
