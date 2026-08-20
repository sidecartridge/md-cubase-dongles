---
id: STORY-03
epic: EPIC-05
title: Build registration
status: done
---

## Goal

The new module and generated header build cleanly in the RP firmware.

## Tasks

- [x] Add `pico_generate_pio_header(... cubaseemul.pio)` in `rp/src/CMakeLists.txt`
- [x] Add `cubaseemul.c` to `target_sources`; add `rp/src/include/cubaseemul.h`
- [x] Place the generated `cubase_lut.h` in `rp/src/include/` (already emitted by the generator)
- [x] `rp/build.sh` compiles clean (clang-tidy included)

## Acceptance

`rp/build.sh` produces a UF2 with no new warnings/lint errors.

## Notes

The 24 KB non-const LUT lands in `.data` (SRAM).
