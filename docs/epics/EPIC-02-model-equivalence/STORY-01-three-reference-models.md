---
id: STORY-01
epic: EPIC-02
title: Implement three reference simulators and cross-verify
status: done
---

## Goal

Model A (equations), Model B (fuse map), Model C (m68k crack) each implement the
same `step()`; all three agree on all 131072 transitions.

## Tasks

- [x] Model A: parse the `^:=` XOR-T sum-of-products equations from `srd_ok.jed.txt`
- [x] Model B: build `step()` from the decoded fuse map (EPIC-01 STORY-02)
- [x] Model C: parse the `do_cell1..16` blocks + `update_pins` remap from `CUBASE.S`
- [x] Exhaustively cross-check A == B == C over all 65536 states x {A8=0,1}
- [x] Emit a SHA-256 signature of the full 131072-entry transition table

## Acceptance

Zero divergence across 131072 transitions; product-term counts match across
models (pin03: 2, pin22: 5, others: 3).

## Notes

Done 2026-08-19. A==B==C confirmed; signature
`e039b544a5c15cf4e21612b0aa35435156758451eb24c8e0419fe25156e45a18`. Also
reproduces the prior research trace bit-for-bit (`0000 -> 2401 -> ... -> FFFF`).
