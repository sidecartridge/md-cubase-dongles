---
id: STORY-04
epic: EPIC-03
title: CI-safe verification without raw sources
status: done
---

## Goal

A committed regression that guards the committed header, runnable in CI without
the reverse-engineered sources.

## Tasks

- [x] `tools/test_lut.py`: parse the header, check row count + id bounds (closure)
- [x] Replay each golden vector from the reset state and assert the D8 stream
- [x] Wire `python3 tools/test_lut.py` into `.github/workflows/build.yml`

## Acceptance

`test_lut.py` passes with no raw sources present; CI runs it before the build.

## Notes

Done 2026-08-19. Full re-derivation stays a local-only step gated on
`--source-dir`.
