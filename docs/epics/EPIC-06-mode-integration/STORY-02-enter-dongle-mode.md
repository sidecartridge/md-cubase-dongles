---
id: STORY-02
epic: EPIC-06
title: Enter dongle mode from setup
status: todo
---

## Goal

A setup-mode affordance switches the app into dongle mode across a reset.

## Tasks

- [ ] Add a terminal command (or menu entry) that sets ACONFIG_PARAM_MODE to the dongle value
- [ ] settings_put_integer + settings_save, then reset via the existing exit path
- [ ] Reboot lands in dongle mode

## Acceptance

The command flips the mode and the device reboots into dongle mode.

## Notes

Reuses the template's existing exit/reset machinery (emul.c:~511).
