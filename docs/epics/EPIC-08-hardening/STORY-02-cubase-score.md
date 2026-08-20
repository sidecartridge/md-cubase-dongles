---
id: STORY-02
epic: EPIC-08
title: Cubase Score support (optional)
status: todo
---

## Goal

Ship the same dongle for Cubase Score, which uses byte-identical dongle math.

## Tasks

- [ ] Confirm CUBSCORE.S dongle math matches CUBASE.S (same LUT)
- [ ] Separate app identity/UUID for the Score variant
- [ ] Groundwork notes for future Cubase 2 / CAF variants (out of scope, D-01)

## Acceptance

Cubase Score runs with the same LUT under its own app identity.

## Notes

CUBSCORE.S differs from CUBASE.S only in patch offsets/text; the dongle routine
is identical.
