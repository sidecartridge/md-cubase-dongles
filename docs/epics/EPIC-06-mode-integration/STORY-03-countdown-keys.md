---
id: STORY-03
epic: EPIC-06
title: Auto-boot countdown & key bindings
status: done
---

## Goal

An auto-boot countdown makes the boot process agile: it enters GEM with the
selected dongle when it reaches zero, unless the user interacts. Key bindings
match the confirmed layout.

## Tasks

- [x] Copy md-drives-emulator's countdown (`drawSetupInfoLine` + `showCounter`,
      20 s, 1 s tick in the main loop via `absolute_time_diff_us`)
- [x] Any keypress halts the countdown (`haltCountdown`)
- [x] `[E]` Enter GEM: commit the selected dongle (`cubaseemul_start()`) then
      boot GEM (`DISPLAY_COMMAND_START` → m68k `userfw`); RP loop keeps running so
      Core1 serves the dongle
- [x] `[X]` Back to Booster: reset + `reset_jump_to_booster()`
- [x] Countdown reaching zero calls the same Enter-GEM path
- [x] Remove the old `[F]irmware` / `[E]xit desktop` / settings CLI commands

## Acceptance

On hardware: menu shows a live countdown; leaving it alone boots GEM into the
selected dongle; `[E]` does it immediately; `[X]` returns to Booster; any key
stops the countdown.

## Notes

`[E]` is the proven EPIC-05 commit path (was `[F]irmware`); it is a one-way mode
commit (reset the MultiDevice to return to the menu). Verified on hardware:
_pending user test_.
