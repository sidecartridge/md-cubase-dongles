#!/usr/bin/env python3
"""Regression test for the committed Cubase 2 black-dongle LUT -- no .v needed.

Runs in CI. Parses `rp/src/include/cubase2_lut.h` and checks every one of the
65536 (state, address) transitions against the equations transcribed in
`tools/generate_cubase2_lut.py` (the committed model of MiSTery's
`cubase2_dongle.v`), plus the 0xD8 reset special-case.

Full re-derivation from the source Verilog is a separate, local-only step; this
test guards the artifact that is actually committed (the header).
"""

import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(REPO, "tools"))
import generate_cubase2_lut as g  # noqa: E402

HEADER = os.path.join(REPO, "rp/src/include/cubase2_lut.h")


def parse_lut(path):
    text = open(path).read()
    rows = re.findall(r"\{(0x[0-9A-Fa-f]{2}(?:,0x[0-9A-Fa-f]{2}){7})\}", text)
    return [[int(v, 16) for v in row.split(",")] for row in rows]


def main():
    lut = parse_lut(HEADER)
    if len(lut) != 256:
        print(f"FAIL: expected 256 rows in cubase2_lut.h, got {len(lut)}")
        sys.exit(1)

    checked = 0
    for state in range(256):
        for a in range(256):
            got = g.next_state(state, a)
            want = g.RESET_STATE if a == g.SPECIAL_ADDR else lut[state][g.input_class(a)]
            if got != want:
                print(f"FAIL: state={state:#04x} a={a:#04x}: "
                      f"equation {got:#04x} vs committed LUT {want:#04x}")
                sys.exit(1)
            checked += 1

    print(f"PASS: {checked}/65536 committed-LUT transitions match the equations")


if __name__ == "__main__":
    main()
