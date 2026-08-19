---
id: STORY-03
epic: EPIC-06
title: Exit dongle mode via SELECT
status: todo
---

## Goal

A SELECT short-press returns the device to setup mode, safely tearing down
Core1 first.

## Tasks

- [ ] Poll `select_checkPushReset()` from the Core0 loop (NOT select_coreWaitPush — Core1 is busy)
- [ ] On press: stop Core1 (multicore_reset_core1) BEFORE the settings flash write
- [ ] Set MODE=255, save, reset
- [ ] Confirm the ST does not autorun a cartridge in dongle mode (romemul unloaded -> open bus != $ABCDEFxx magic)

## Acceptance

Round-trip setup <-> dongle works; no spurious cartridge autorun (D-07 / C-01).

## Notes

Core1 must stop before any flash write even with PICO_FLASH_ASSUME_CORE0_SAFE=1.
