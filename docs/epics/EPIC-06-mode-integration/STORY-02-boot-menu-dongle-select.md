---
id: STORY-02
epic: EPIC-06
title: Boot menu with persisted dongle selection
status: done
---

## Goal

Show a boot menu on every power-on that displays the currently selected dongle
version and lets the user cycle through the available versions, persisting the
choice for future sessions.

## Tasks

- [x] Add `ACONFIG_PARAM_DONGLE` (string, default `"RED"`) to `aconfig.h` /
      `aconfig.c`, replacing the now-unused `ACONFIG_PARAM_FOLDER`
- [x] Build an extensible dongle catalogue in `emul.c` (`DONGLES[]` = id + label;
      only `RED` / "Cubase 3 red dongle (Intel 5C060)" today)
- [x] Load the persisted id at boot into `selectedDongle`
- [x] `[D]` cycles to the next dongle version, persists it, and redraws the menu
- [x] Menu copies md-drives-emulator's fonts/layout (amstrad_cpc_extended_8f menu
      text, squeezed_b7_tr status bar)

## Acceptance

The menu shows the selected dongle; `[D]` changes it; the choice survives a power
cycle.

## Notes

Only the red dongle exists today, so cycling stays on `RED`, but the catalogue +
persistence machinery is in place for future variants (D-01 keeps v1 red-only).
