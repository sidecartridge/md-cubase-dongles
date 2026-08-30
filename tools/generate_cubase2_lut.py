#!/usr/bin/env python3
"""Generate and verify the Cubase 2 "black dongle" 8-bit state-machine LUT.

Source model: MiSTery `atarist/cubase2_dongle.v` (gyurco/MiSTery, GPL). The black
dongle is an 8-bit registered finite-state machine:

  state  = D[15:8]           (8 bits)
  input  = A[8:1]            (8 bits)
  clock  = rising edge of /UDS  (uds_n & !uds_nD)   [ /UDS == A0 on the 68000 ]
  reset  = 0x00
  read   = (state << 8) | 0x00FF     (ROM3 access returns state on D15..D8)

The eight next-state equations (transcribed verbatim from the Verilog below)
depend, outside the single special address A[8:1] == 0xD8, only on A1, A4 and A5.
Address 0xD8 makes every equation's first product term 1, so `!(1|...) == 0` for
all eight bits -> the machine resets to 0x00.

So the full 256x256 transition table reduces to `lut[256 states][8 classes]`
where `class = (A5<<2) | (A4<<1) | A1`, plus the 0xD8 reset special-case. The
emitted table is 256 * 8 = 2048 bytes.

This script hardcodes the equations (our faithful transcription), builds the
reduced table, and verifies it exhaustively (all 256 states x 256 addresses)
against the equations, plus structural checks that only hold if the transcription
is correct (input reduces to {A5,A4,A1}; 0xD8 resets; all 256 states reachable).
"""

import argparse
import os

RESET_STATE = 0
SPECIAL_ADDR = 0xD8


def _bit(value, index):
    return (value >> index) & 1


def next_state(state, a):
    """One /UDS-edge transition. state = D[15:8], a = A[8:1] (A1 = bit 0)."""
    d8 = _bit(state, 0)
    d9 = _bit(state, 1)
    d10 = _bit(state, 2)
    d11 = _bit(state, 3)
    d12 = _bit(state, 4)
    d13 = _bit(state, 5)
    d14 = _bit(state, 6)
    d15 = _bit(state, 7)

    a1 = _bit(a, 0)
    a2 = _bit(a, 1)
    a3 = _bit(a, 2)
    a4 = _bit(a, 3)
    a5 = _bit(a, 4)
    a6 = _bit(a, 5)
    a7 = _bit(a, 6)
    a8 = _bit(a, 7)

    def n(x):
        return x ^ 1

    special = a8 & a7 & n(a6) & a5 & a4 & n(a3) & n(a2) & n(a1)

    nd15 = n(special
             | (d14 & d12 & d10 & a1)
             | (d13 & n(d10) & a4)
             | (n(d15) & n(d14) & n(d13) & n(d12) & n(d11) & d10 & n(d9) & a4)
             | (n(d14) & n(d10) & a1)
             | (d15 & n(d10) & a4)
             | (n(d12) & n(d10) & a1)
             | (n(d8) & a5))

    nd14 = n(special
             | (n(d15) & n(d14) & n(d13) & n(d12) & n(d11) & n(d10) & n(d9) & d8 & a4)
             | (d14 & d12 & d10 & d8 & a1)
             | (n(d10) & n(d8) & a1)
             | (n(d12) & n(d8) & a1)
             | (d15 & n(d8) & a4)
             | (n(d14) & n(d8) & a1)
             | (n(d15) & a5))

    nd13 = n(special
             | (d15 & d14 & d13 & d12 & d11 & d10 & d8 & a1)
             | (n(d15) & n(d13) & d11 & a4)
             | (d13 & n(d11) & a4)
             | (n(d12) & n(d11) & a1)
             | (d15 & n(d11) & a4)
             | (n(d14) & n(d11) & a1)
             | (n(d9) & a5))

    nd12 = n(special
             | (d15 & d14 & d13 & d12 & d10 & d8 & a1)
             | (n(d13) & n(d10) & a1)
             | (n(d15) & d13 & a4)
             | (n(d13) & n(d12) & a1)
             | (d15 & n(d13) & a4)
             | (n(d14) & n(d13) & a1)
             | (n(d11) & a5))

    nd11 = n(special
             | (d15 & d14 & d12 & d10 & d8 & a1)
             | (n(d15) & n(d8) & a1)
             | (n(d15) & n(d10) & a1)
             | (n(d15) & n(d12) & a1)
             | (n(d15) & n(d14) & a1)
             | (d15 & a4)
             | (n(d13) & a5))

    nd10 = n(special
             | (d15 & d14 & d13 & d12 & d11 & d10 & d9 & d8 & a1)
             | (n(d15) & n(d13) & n(d11) & d9 & a4)
             | (d11 & n(d9) & a4)
             | (d13 & n(d9) & a4)
             | (d15 & n(d9) & a4)
             | (n(d14) & n(d9) & a1)
             | (n(d14) & a5))

    nd9 = n(special
            | (n(d15) & d14 & n(d13) & n(d11) & n(d9) & a4)
            | (n(d14) & d9 & a4)
            | (n(d14) & d11 & a4)
            | (n(d14) & d13 & a4)
            | (d15 & n(d14) & a4)
            | (d14 & a1)
            | (n(d12) & a5))

    nd8 = n(special
            | (n(d15) & n(d14) & n(d13) & d12 & n(d11) & n(d9) & a4)
            | (d14 & d12 & a1)
            | (n(d12) & d11 & a4)
            | (d13 & n(d12) & a4)
            | (d15 & n(d12) & a4)
            | (n(d14) & n(d12) & a1)
            | (n(d10) & a5))

    return (nd8 | (nd9 << 1) | (nd10 << 2) | (nd11 << 3) | (nd12 << 4)
            | (nd13 << 5) | (nd14 << 6) | (nd15 << 7))


def input_class(a):
    """{A5, A4, A1} -> 0..7. A5 = bit4, A4 = bit3, A1 = bit0."""
    return (_bit(a, 4) << 2) | (_bit(a, 3) << 1) | _bit(a, 0)


def build_lut():
    lut = [[None] * 8 for _ in range(256)]
    for state in range(256):
        for a in range(256):
            if a == SPECIAL_ADDR:
                continue
            cls = input_class(a)
            ns = next_state(state, a)
            if lut[state][cls] is None:
                lut[state][cls] = ns
            elif lut[state][cls] != ns:
                raise AssertionError(
                    f"input class not uniform: state={state:#04x} a={a:#04x} "
                    f"class={cls} got {ns:#04x} vs {lut[state][cls]:#04x}")
    for state in range(256):
        for cls in range(8):
            if lut[state][cls] is None:
                raise AssertionError(f"class {cls} unpopulated for state {state:#04x}")
    return lut


def verify(lut):
    # Exhaustive: every (state, address) matches the LUT + 0xD8 special-case.
    checked = 0
    for state in range(256):
        for a in range(256):
            ns = next_state(state, a)
            if a == SPECIAL_ADDR:
                if ns != RESET_STATE:
                    raise AssertionError(f"0xD8 must reset: state={state:#04x} -> {ns:#04x}")
            elif ns != lut[state][input_class(a)]:
                raise AssertionError(f"LUT mismatch state={state:#04x} a={a:#04x}")
            checked += 1
    # Reachability from reset.
    reached = {RESET_STATE}
    frontier = [RESET_STATE]
    while frontier:
        s = frontier.pop()
        for a in range(256):
            ns = next_state(s, a)
            if ns not in reached:
                reached.add(ns)
                frontier.append(ns)
    return checked, len(reached)


HEADER = """/* cubase2_lut.h -- Cubase 2 "black dongle" 8-bit state machine.
 *
 * GENERATED by tools/generate_cubase2_lut.py -- DO NOT EDIT BY HAND.
 * Derived from the MiSTery `atarist/cubase2_dongle.v` model (gyurco/MiSTery).
 *
 * state = D[15:8], reset = 0x00, clock = /UDS rising edge (/UDS == A0),
 * input = A[8:1]. A ROM3 read returns (state << 8) | 0x00FF.
 *
 * Transition: if A[8:1] == 0xD8 the machine resets to 0x00; otherwise
 *   class = (A5<<2)|(A4<<1)|A1  and  next = cubase2_lut[state][class].
 * Non-const so it lives in SRAM; include from exactly one translation unit.
 */
#ifndef CUBASE2_LUT_H
#define CUBASE2_LUT_H

#include <stdint.h>

#define CUBASE2_LUT_RESET_STATE  0x00u
#define CUBASE2_LUT_SPECIAL_ADDR 0xD8u
/* {A5,A4,A1} of A[8:1] (A1 = bit 0, A4 = bit 3, A5 = bit 4). */
#define CUBASE2_INPUT_CLASS(a) \\
  ((uint8_t)((((a) >> 4) & 1u) << 2 | (((a) >> 3) & 1u) << 1 | ((a) & 1u)))
/* Word driven on a ROM3 read: state on D15..D8, 0xFF on D7..D0. */
#define CUBASE2_RESPONSE_WORD(state) ((uint16_t)(((uint16_t)(state) << 8) | 0x00FFu))

uint8_t cubase2_lut[256][8] = {
"""


def emit(lut, path):
    with open(path, "w") as fh:
        fh.write(HEADER)
        for state in range(256):
            row = ",".join(f"0x{lut[state][c]:02X}" for c in range(8))
            fh.write(f"    {{{row}}},\n")
        fh.write("};\n\nuint16_t cubase2_lut_num_states = 256u;\n\n")
        fh.write("#endif  /* CUBASE2_LUT_H */\n")


def main():
    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--header",
                    default=os.path.join(repo, "rp/src/include/cubase2_lut.h"))
    ap.add_argument("--check-only", action="store_true",
                    help="verify without writing the header")
    args = ap.parse_args()

    lut = build_lut()
    checked, reachable = verify(lut)
    print(f"PASS: {checked}/65536 (state,address) transitions match the equations")
    print(f"0xD8 -> 0x00 for all 256 states: PASS")
    print(f"reachable states from 0x00: {reachable}/256 "
          f"({'PASS' if reachable == 256 else 'FAIL'})")
    print(f"LUT payload: {256 * 8} bytes")

    if not args.check_only:
        emit(lut, args.header)
        print(f"wrote {args.header}")


if __name__ == "__main__":
    main()
