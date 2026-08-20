---
id: STORY-01
epic: EPIC-04
title: Minimal fixed-word ROM3 drive smoke test
status: done
---

## Goal

The ST reads a known constant (0xFEFF) from the ROM3 window, proving the RP can
source data on a ROM3 read.

## Tasks

- [x] Write a minimal `cubaseemul.pio` that drives 0xFEFF on every ROM3 access (clones romemul's drive path; no address sampling — driving is the isolated unknown)
- [x] `cubaseemul.c` `cubaseemul_fixed_start()` loads the drive SM on pio0 alongside romemul (ROM3/ROM4 never active together)
- [x] Put the ST-side `$FB0000` peek in the cartridge (`target/atarist/src/userfw.s`) so it auto-runs — no separate program to build/copy
- [x] Wire a `CUBASE_GATE_TEST` build flag: `emul_start` serves ROM4, drives ROM3, and writes CMD_START so `main.s` jumps to `userfw`
- [x] Verify on MultiDevice + ST that the value reads back as 0xFEFF — **PASSED 2026-08-20: ST reads `FEFF FEFF FEFF FEFF`**

## Acceptance

ST prints `FEFF` from `$FB0000` — met. Final design used a unified ROM3 engine
in `commemul` (capture + drive) on pio0; see the epic notes and D-08.

## Notes

Reuse `romemul.pio`'s flip-pindirs -> drive -> release pattern and the side-set
`!WRITE` strobe. Code done 2026-08-20; PIO is 12 instructions, sharing pio0 with
romemul (16) = 28/32. The ST side lives in `userfw.s` (the cartridge app slot),
reached via the existing CMD_START path — no command channel, network or display
loop needed. See `README.md` for the build/run procedure. Only the hardware
read-back remains — Diego's to run.
