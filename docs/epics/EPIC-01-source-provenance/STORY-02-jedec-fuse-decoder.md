---
id: STORY-02
epic: EPIC-01
title: Raw JEDEC fuse-map decoder
status: done
---

## Goal

Decode the raw `SRD_OK` fuse map into per-macrocell sum-of-products equations,
independently of the human-readable equation listing, so the actual silicon is
a first-class cross-check source.

## Tasks

- [x] Parse the JEDEC records (QF6482, L##### bit runs) into a flat 6482-fuse array
- [x] Map the 5C060 architecture: 16 macrocells x 10 product terms x 40 columns (20 input pins, complement/true fuse pair)
- [x] Extract the SOP (product terms 0..7), excluding the async (pt8) and output-enable (pt9) terms
- [x] Confirm the decoded equations match `srd_ok.jed.txt` (block 0 == pin22, etc.)

## Acceptance

The decoder reproduces the pin22 output-enable (`/pin02`) and all 16 registered
cells; decoded equations are logically identical to the listing.

## Notes

Done 2026-08-19 in `tools/generate_lut.py` (`load_fuses`, `fuses_to_eqs`).
`BLOCK_TO_PIN = [22,21,20,19,18,17,16,15,3,4,5,6,7,8,9,10]`. Block 0 decoded
exactly to pin22's five SOP terms + `/pin02` OE.
