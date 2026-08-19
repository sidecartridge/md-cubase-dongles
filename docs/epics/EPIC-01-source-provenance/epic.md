---
id: EPIC-01
iteration: 1
title: Source ingestion & provenance
status: done
---

## Goal

Bring the three reverse-engineered hardware descriptions of the Cubase 3 red
dongle into the build in a reproducible, auditable way — without committing the
raw files to this public repo (D-02) — and stand up a from-scratch decoder of
the raw JEDEC fuse map so the actual silicon is one of the cross-check sources.

## Scope

- In scope: locating and hashing the sources, a provenance manifest, a
  `--source-dir` intake path, a `.gitignore` guard, and a JEDEC fuse-map decoder
  that derives per-macrocell equations.
- Out of scope: the state-machine models themselves (EPIC-02) and the LUT
  (EPIC-03).

## Stories

- STORY-01: Hash and register the sources
- STORY-02: Raw JEDEC fuse-map decoder

## Notes

Sources (out-of-tree): `srd_ok.jed.txt` (de-fused equations), `SRD_OK` (raw
JEDEC fuse map, QF6482, device 5C060-45/-55), `CUBASE.S` (Medway Boys 2022
m68k emulator). See D-02 and `tools/reference/PROVENANCE.md`.
