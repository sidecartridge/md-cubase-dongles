#!/usr/bin/env python3
"""Regression test for the committed Cubase LUT -- needs no raw sources.

Runs in CI. It parses the generated rp/src/include/cubase_lut.h and the golden
vectors, then checks:
  * the table is well formed (NUM_STATES rows, every next-state id in range,
    i.e. the reachable set is closed);
  * replaying each golden A8 sequence from the reset state reproduces the
    recorded D8 sequence.

Full re-derivation from the reverse-engineered sources is a separate, local-only
step (tools/generate_lut.py --source-dir <dir>); this test guards the artifact
that is actually committed.
"""

import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HEADER = os.path.join(REPO, "rp/src/include/cubase_lut.h")
VECTORS = os.path.join(REPO, "tools/vectors/cubase3.txt")


def parse_header(path):
    txt = open(path).read()
    defs = dict(re.findall(r"#define\s+CUBASE_LUT_(\w+)\s+(0x[0-9A-Fa-f]+|\d+)", txt))
    num = int(defs["NUM_STATES"], 0)
    reset = int(defs["RESET_STATE_ID"], 0)
    reset_d8 = 0 if "0xFEFF" in defs["RESET_OUTPUT_WORD"] else 1
    body = txt.split("cubase_lut", 1)[1].split("{", 1)[1].rsplit("}", 1)[0]
    pairs = re.findall(r"\{\s*(0x[0-9A-Fa-f]+)\s*,\s*(0x[0-9A-Fa-f]+)\s*\}", body)
    lut = [(int(a, 16), int(b, 16)) for a, b in pairs]
    return num, reset, reset_d8, lut


def main():
    num, reset, reset_d8, lut = parse_header(HEADER)
    fails = []

    if len(lut) != num:
        fails.append(f"row count {len(lut)} != NUM_STATES {num}")
    for i, row in enumerate(lut):
        for a8, e in enumerate(row):
            nid = e & 0x1FFF
            if nid >= num:
                fails.append(f"id {i} a8 {a8}: next id {nid} out of range")

    for line in open(VECTORS):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, a8s, d8s = line.split()
        sid, got = reset, []
        for ch in a8s:
            e = lut[sid][int(ch)]
            got.append((e >> 13) & 1)  # D8 of the NEXT state == output next access
            sid = e & 0x1FFF
        # the recorded D8 stream is the output BEFORE each access; reconstruct:
        # output(access k) = D8 of state entered by access k-1, output(0)=reset D8.
        rebuilt = [reset_d8] + got[:-1]
        expect = [int(c) for c in d8s]
        if rebuilt != expect:
            k = next(i for i in range(len(expect)) if rebuilt[i] != expect[i])
            fails.append(f"vector {name}: D8 mismatch at access {k}")

    if fails:
        print("FAIL:")
        for f in fails:
            print("  -", f)
        sys.exit(1)
    print(f"OK: {num} states, {len(lut)} rows, all golden vectors reproduced")


if __name__ == "__main__":
    main()
