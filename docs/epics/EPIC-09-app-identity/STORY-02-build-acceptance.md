---
id: STORY-02
epic: EPIC-09
title: Full build + Cubase acceptance
status: todo
---

## Goal

A clean release build that boots to setup and, in dongle mode, satisfies real
Cubase.

## Tasks

- [ ] Run root `./build.sh pico_w release <uuid>`; confirm the m68k cartridge still fits 8 KB
- [ ] UF2 boots to setup mode
- [ ] Activate dongle mode; real Cubase 3.10 accepts the dongle and runs on MultiDevice + ST

## Acceptance

Cubase 3.10 runs with the emulated dongle (the ultimate acceptance test, C-02).

## Notes

This is the functional stand-in for a logic-analyzer capture (D-06).
