---
id: STORY-01
epic: EPIC-01
title: Hash and register the reverse-engineered sources
status: done
---

## Goal

The generated table can be re-derived and audited from the raw sources, but the
raw sources never enter the public repo (D-02).

## Tasks

- [x] Compute SHA-256 of `srd_ok.jed.txt`, `SRD_OK`, `CUBASE.S`
- [x] Record them in `tools/reference/PROVENANCE.md` with source descriptions and the atari-forum origin (t=20130)
- [x] Add a `--source-dir` intake to the generator so it reads raw sources from a local path
- [x] Add `.gitignore` entries so the raw source filenames are never committed

## Acceptance

`PROVENANCE.md` lists three hashes plus the full-transition signature; `git
status` shows no raw dongle files tracked.

## Notes

Done 2026-08-19. Hashes: equations `fd197818…`, fusemap `8aa83099…`, crack
`7f16933f…`. `.gitignore` guards `srd_ok.jed.txt`, `SRD_OK`, `CUBASE.S`,
`CUBSCORE.S`, `tools/reference/sources/`.
