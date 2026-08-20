---
id: STORY-01
epic: EPIC-09
title: Fill app identity
status: todo
---

## Goal

`desc/app.json` and version reflect a real Cubase dongle app.

## Tasks

- [ ] Fill name (e.g. "Cubase 3 Dongle"), description, tags, devices (ST/STE/MegaST/MegaSTE), binary URL with placeholders
- [ ] Leave <APP_UUID>/<APP_VERSION>/<BINARY_MD5_HASH> for build.sh to substitute
- [ ] Bump version.txt

## Acceptance

`build.sh` substitution produces a valid descriptor.

## Notes

Currently the pristine "APP DEV Template" identity.
