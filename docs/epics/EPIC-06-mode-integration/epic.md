---
id: EPIC-06
iteration: 2
title: Mode integration & UX
status: todo
---

## Goal

Make the app modal (D-03): a setup mode for config and a dongle mode that owns
ROM3, with clean switching and no spurious cartridge autorun.

## Scope

- In scope: the `emul_start` mode branch, entering dongle mode from setup,
  SELECT-to-exit, and confirming ROM4 shows no cartridge magic in dongle mode.
- Out of scope: app identity/build (EPIC-07).

## Stories

- STORY-01: Dongle-mode branch in emul_start
- STORY-02: Enter dongle mode from setup
- STORY-03: Exit dongle mode via SELECT

## Notes

Reuses `ACONFIG_PARAM_MODE` (read at ~emul.c:308, currently unbranched) and the
existing "set MODE, save, reset" machinery.
