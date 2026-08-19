---
id: STORY-01
epic: EPIC-04
title: Minimal fixed-word ROM3 drive smoke test
status: todo
---

## Goal

The ST reads a known constant (0xFEFF) from the ROM3 window, proving the RP can
source data on a ROM3 read.

## Tasks

- [ ] Write a minimal `cubaseemul.pio` that, on ROM3 active, samples the address and drives 0xFEFF (clone romemul's drive path)
- [ ] Minimal Core0 bring-up that loads the SM on pio0 with romemul/commemul absent
- [ ] A tiny ST-side reader of `$FB0000+off` (or use Cubase's probe) to display the value
- [ ] Verify on MultiDevice + ST that the value reads back as 0xFEFF

## Acceptance

ST reads back 0xFEFF from ROM3. On failure, record the ROM4 pivot in D-07.

## Notes

Reuse `romemul.pio`'s sample -> flip-pindirs -> drive -> release pattern and the
`READ_ADDRESS_SAFE_WAIT_CYCLES` settle timing.
