---
id: STORY-01
epic: EPIC-06
title: Dongle-mode branch in emul_start
status: todo
---

## Goal

In dongle mode, bring up only the dongle engine and skip everything ROM4/command
channel/UI related.

## Tasks

- [ ] Branch after `ACONFIG_PARAM_MODE` is read (~emul.c:308)
- [ ] Dongle path skips COPY_FIRMWARE_TO_RAM, init_romemul, commemul_init, chandler_*, display refresh, sdcard, and the network block
- [ ] Dongle path keeps select_configure(), calls cubaseemul_init(), launches Core1, then a minimal Core0 loop
- [ ] Setup mode remains unchanged

## Acceptance

Dongle mode boots without ROM4/command channel; setup mode behaves as before.

## Notes

C-01: not loading commemul/romemul frees pio0 for the cubase program.
