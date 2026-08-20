---
id: EPIC-06
iteration: 2
title: Boot menu, dongle selection & slim-down
status: done
---

## Goal

Show a boot menu on every power-on that lets the user pick a dongle version,
persists the choice, and either enters GEM with the emulated dongle or returns
to Booster. An auto-boot countdown launches GEM with the selected dongle when it
reaches zero. Strip the app to the bare minimum needed for this — no network, no
microSD, no USB, no status LED.

Workflow, keys, fonts, and the countdown are copied from `md-drives-emulator`.

## Scope

- In scope: the always-on boot menu, the dongle-version cycle selector persisted
  in `ACONFIG_PARAM_DONGLE`, the auto-boot countdown, the `[E]`/`[X]`/`[D]` key
  bindings, and dropping all network/microSD/USB/LED code and libraries.
- Out of scope: app identity/store descriptor (EPIC-09); adding a second dongle
  variant (the catalogue is built to extend, but only Red exists today).

## Stories

- STORY-01: Slim-down — drop network, microSD, USB and LED
- STORY-02: Boot menu with persisted dongle selection
- STORY-03: Auto-boot countdown & key bindings

## Notes

`[E]` reuses the proven EPIC-05 commit path (`cubaseemul_start()` +
`DISPLAY_COMMAND_START` → m68k `userfw` → boot GEM); the RP main loop keeps
running so Core1 serves the dongle while Cubase runs. `[X]` reuses the existing
reset/jump-to-booster path. The old `[F]irmware` / `[E]xit desktop` / settings
CLI are removed. Dongle selection is persisted via the new `ACONFIG_PARAM_DONGLE`
string setting (default `RED`), replacing the now-unused `ACONFIG_PARAM_FOLDER`.
