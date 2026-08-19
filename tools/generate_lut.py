#!/usr/bin/env python3
"""Generate and verify the Cubase 3 "red dongle" state-machine lookup table.

The Steinberg Cubase 3 red dongle is an Intel 5C060 / Altera EP600 PLD wired as
a 16-bit registered state machine: one meaningful input (A8, pin14), one output
(D8, pin22), advanced on each /ROM3 access. This tool reconstructs that machine
from three INDEPENDENT hardware descriptions, proves they agree on every
transition, then emits a compact lookup table for the RP2040 firmware.

The three source models (all reduced to the same step(state, a8) -> next_state):
  A  de-fused equation listing            (srd_ok.jed.txt)
  B  raw JEDEC fuse map, decoded here     (SRD_OK)          <- the actual silicon
  C  Medway Boys 2022 m68k emulator       (CUBASE.S)        <- 3rd-party reimpl.

Verification (see the plan's "Verification strategy"):
  * A == B == C over all 65536 states x {A8=0,1} = 131072 transitions.
  * The emitted LUT reproduces the model on every reachable transition and over
    a long pseudo-random A8 walk from reset.

The raw sources are reverse-engineered and are NOT committed to this public
repo; pass their location with --source-dir. Only the derived cubase_lut.h,
the golden vectors, and tools/reference/PROVENANCE.md (SHA-256s) are committed.

State bit mapping (shared by all models, and by the runtime):
  bit 0..7  = pin03..pin10
  bit 8..14 = pin15..pin21
  bit 15    = pin22   (D8, the output)
  input a8  = pin14
"""

import argparse
import hashlib
import os
import re
import sys

# --- canonical mapping -------------------------------------------------------
STATE_PINS = [3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 17, 18, 19, 20, 21, 22]
BIT = {p: i for i, p in enumerate(STATE_PINS)}
D8_BIT = BIT[22]  # 15

# JEDEC fuse-map architecture of the 5C060 (verified against the equations):
# 16 macrocells x 10 product terms x 40 columns; columns are 20 input pins,
# each with a (complement, true) fuse pair; product terms 0..7 feed the OR
# gate, pt8 is the async control and pt9 the output-enable (both excluded).
AND_PINS = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23]
BLOCK_TO_PIN = [22, 21, 20, 19, 18, 17, 16, 15, 3, 4, 5, 6, 7, 8, 9, 10]
SOP_TERMS = range(8)

CANONICAL = {"equations": "srd_ok.jed.txt", "fusemap": "SRD_OK", "crack": "CUBASE.S"}


# --- a term is a list of (pin, want_true) literals; an equation is a list of
#     such product terms; step() XOR-accumulates the OR-of-products (^:= reg) --
def make_step(eqs):
    def step(state, a8):
        val = {p: (state >> BIT[p]) & 1 for p in STATE_PINS}
        val[14] = a8 & 1
        nxt = 0
        for p in STATE_PINS:
            sop = 0
            for term in eqs[p]:
                prod = 1
                for pin, want_true in term:
                    if (val[pin] == 1) != want_true:
                        prod = 0
                        break
                sop |= prod
            if val[p] ^ sop:
                nxt |= 1 << BIT[p]
        return nxt

    return step


# --- Model A: de-fused equation listing --------------------------------------
def parse_equations(path):
    body = open(path).read().split("start", 1)[1].rsplit("end", 1)[0]
    eqs = {}
    for m in re.finditer(r"pin(\d+)\s*\^:=(.*?);", body, re.S):
        terms = []
        for term in m.group(2).split("+"):
            lits = [(int(x.group(2)), x.group(1) != "/")
                    for x in re.finditer(r"(/?)\s*pin(\d+)", term)]
            if lits:
                terms.append(lits)
        eqs[int(m.group(1))] = terms
    return eqs


# --- Model B: raw JEDEC fuse map ---------------------------------------------
def load_fuses(path):
    s = open(path, "rb").read().decode("latin-1")
    qf = int(re.search(r"QF(\d+)\*", s).group(1))
    f = ["?"] * qf
    for m in re.finditer(r"L(\d+)\s+([01]+)\*", s):
        st = int(m.group(1))
        f[st:st + len(m.group(2))] = list(m.group(2))
    if "?" in f:
        raise ValueError("fuse map has uncovered positions")
    return f


def fuses_to_eqs(f):
    eqs = {}
    for bi, pout in enumerate(BLOCK_TO_PIN):
        terms = []
        for pt in SOP_TERMS:
            bits = f[bi * 400 + pt * 40:bi * 400 + (pt + 1) * 40]
            lits, impossible = [], False
            for j, p in enumerate(AND_PINS):
                c0, c1 = bits[2 * j] == "0", bits[2 * j + 1] == "0"
                if c0 and c1:
                    impossible = True
                elif c0:
                    lits.append((p, False))
                elif c1:
                    lits.append((p, True))
            if not impossible:
                terms.append(lits)
        eqs[pout] = terms
    return eqs


# --- Model C: Medway Boys m68k emulator --------------------------------------
CELL_TO_PIN = {1: 22, 2: 21, 3: 20, 4: 19, 5: 18, 6: 17, 7: 16, 8: 15,
               9: 3, 10: 4, 11: 5, 12: 6, 13: 7, 14: 8, 15: 9, 16: 10}


def parse_crack(path):
    txt = open(path).read()
    eqs = {}
    for n in range(1, 17):
        block = re.search(rf"^do_cell{n}\b(.*?)\brts", txt, re.S | re.M).group(1)
        reg_terms, order = {}, []
        for line in block.splitlines():
            mm = re.match(r"\s*(move|and)\.b\s+pin(\d+)(\+1)?\(pc\),d(\d)", line)
            if not mm:
                continue
            op, pin, compl, reg = mm.group(1), int(mm.group(2)), bool(mm.group(3)), mm.group(4)
            lit = (pin, not compl)  # pinX+1 is the complement column
            if op == "move":
                reg_terms[reg] = [lit]
                if reg not in order:
                    order.append(reg)
            else:
                reg_terms[reg].append(lit)
        eqs[CELL_TO_PIN[n]] = [reg_terms[r] for r in order]
    return eqs


# --- verification ------------------------------------------------------------
def build_table(step):
    """Flatten a model into t[2*state + a8] = next_state (131072 entries)."""
    t = [0] * (65536 * 2)
    for s in range(65536):
        t[2 * s] = step(s, 0)
        t[2 * s + 1] = step(s, 1)
    return t


def cross_verify(tables):
    """Assert every model's transition table is identical."""
    ref = tables[0]
    for other in tables[1:]:
        if other != ref:
            i = next(k for k in range(len(ref)) if ref[k] != other[k])
            raise AssertionError(
                f"model divergence at state={i // 2:#06x} a8={i & 1}")
    return len(ref)


def reachable_from(table, start):
    seen = {start}
    stack = [start]
    while stack:
        s = stack.pop()
        for a8 in (0, 1):
            ns = table[2 * s + a8]
            if ns not in seen:
                seen.add(ns)
                stack.append(ns)
    return seen


def build_lut(table):
    reset0 = 0                # pin15 = 0 (matches CUBASE.S / MiSTer)
    reset1 = 1 << BIT[15]     # pin15 = 1 (power-on ambiguity)
    r0 = reachable_from(table, reset0)
    r1 = reachable_from(table, reset1)
    states = sorted(r0 | r1)  # union universe covers both power-on values
    sset = set(states)
    if not all(table[2 * s + a8] in sset for s in states for a8 in (0, 1)):
        raise AssertionError("reachable set is not closed")
    if len(states) > 8192:
        raise AssertionError("more than 8192 states; 13-bit id insufficient")
    ident = {s: i for i, s in enumerate(states)}
    lut = []
    for s in states:
        row = []
        for a8 in (0, 1):
            ns = table[2 * s + a8]
            row.append(ident[ns] | (((ns >> D8_BIT) & 1) << 13))
        lut.append(row)
    meta = {
        "num_states": len(states),
        "reset_state_id": ident[reset0],
        "reset_d8": (reset0 >> D8_BIT) & 1,
        "reachable_p15_0": len(r0),
        "reachable_p15_1": len(r1),
        "converges": _converges(table, reset0, reset1),
    }
    return states, ident, lut, meta


def _converges(table, s0, s1, steps=1_000_000):
    """Do the two power-on universes emit the same D8 stream? (pseudo-random A8)"""
    rng, a, b = 0x1234567, s0, s1
    for _ in range(steps):
        rng = (1103515245 * rng + 12345) & 0x7fffffff
        a8 = (rng >> 16) & 1
        if ((a >> D8_BIT) & 1) != ((b >> D8_BIT) & 1):
            return False
        a, b = table[2 * a + a8], table[2 * b + a8]
    return True


def verify_lut(table, states, ident, lut):
    # every reachable transition matches the model
    for i, s in enumerate(states):
        for a8 in (0, 1):
            ns = table[2 * s + a8]
            e = lut[i][a8]
            if (e & 0x1FFF) != ident[ns] or ((e >> 13) & 1) != ((ns >> D8_BIT) & 1):
                raise AssertionError(f"LUT mismatch at id={i} a8={a8}")
    # long pseudo-random A8 walk: model vs LUT in lockstep
    rng, phys, sid = 0x0BADF00D, 0, ident[0]
    for _ in range(2_000_000):
        rng = (1103515245 * rng + 12345) & 0x7fffffff
        a8 = (rng >> 20) & 1
        phys = table[2 * phys + a8]
        e = lut[sid][a8]
        sid = e & 0x1FFF
        if ((e >> 13) & 1) != ((phys >> D8_BIT) & 1) or states[sid] != phys:
            raise AssertionError("LUT diverged from model on random walk")


# --- emission ----------------------------------------------------------------
def sha256(path):
    return hashlib.sha256(open(path, "rb").read()).hexdigest()


def transition_signature(table):
    h = hashlib.sha256()
    for ns in table:
        h.update(ns.to_bytes(2, "big"))
    return h.hexdigest()


def emit_header(path, lut, meta, sig):
    out = ["uint16_t cubase_lut[CUBASE_LUT_NUM_STATES][2] = {"]
    for i in range(0, len(lut), 8):
        chunk = lut[i:i + 8]
        out.append("    " + " ".join(f"{{0x{a:04X},0x{b:04X}}}," for a, b in chunk))
    body = "\n".join(out).rstrip(",") + "\n};\n"
    rst_word = "0xFFFFu" if meta["reset_d8"] else "0xFEFFu"
    with open(path, "w") as fh:
        fh.write(f"""\
/* cubase_lut.h -- Cubase 3 red dongle (Intel 5C060) state machine.
 *
 * GENERATED by tools/generate_lut.py -- DO NOT EDIT BY HAND.
 * Derived from three cross-checked hardware descriptions (JEDEC fuse map,
 * de-fused equations, m68k emulator); all agree on every transition.
 * Full-transition SHA-256: {sig}
 *
 * Packed entry: bits 0..12 = next state id, bit 13 = D8 of the next state.
 * Index as cubase_lut[state_id][a8]. The array is intentionally non-const so
 * it lives in SRAM (deterministic Core1 access -- see plan Risk 7); include
 * from exactly one translation unit.
 */
#ifndef CUBASE_LUT_H
#define CUBASE_LUT_H

#include <stdint.h>

#define CUBASE_LUT_NUM_STATES       {meta['num_states']}u
#define CUBASE_LUT_RESET_STATE_ID   {meta['reset_state_id']}u
#define CUBASE_LUT_RESET_OUTPUT_WORD {rst_word}

#define CUBASE_LUT_NEXT_STATE(e) ((e) & 0x1FFFu)
#define CUBASE_LUT_D8(e)         (((e) >> 13) & 1u)
/* MiSTer drives {{7'h7f, d8}} on the data byte; mirror that on all 16 lines. */
#define CUBASE_LUT_OUTPUT_WORD(e) (CUBASE_LUT_D8(e) ? 0xFFFFu : 0xFEFFu)

{body}
#endif /* CUBASE_LUT_H */
""")


def emit_vectors(path, table):
    """Human-checkable golden traces: reset -> A8 sequence -> D8 sequence."""
    def trace(seq):
        s, d = 0, []
        for a8 in seq:
            d.append((s >> D8_BIT) & 1)
            s = table[2 * s + a8]
        return d

    seqs = {
        "const0": [0] * 64,
        "const1": [1] * 64,
        "alt": [i & 1 for i in range(64)],
    }
    rng = 0xC0FFEE
    rnd = []
    for _ in range(256):
        rng = (1103515245 * rng + 12345) & 0x7fffffff
        rnd.append((rng >> 16) & 1)
    seqs["random256"] = rnd
    with open(path, "w") as fh:
        fh.write("# Cubase 3 red dongle golden vectors (reset=state0, pin15=0)\n")
        fh.write("# format: <name> <A8-bits> <D8-bits>\n")
        for name, seq in seqs.items():
            fh.write(f"{name} {''.join(map(str, seq))} {''.join(map(str, trace(seq)))}\n")


def emit_provenance(path, src_paths, sig, meta):
    with open(path, "w") as fh:
        fh.write("# Provenance -- Cubase 3 red dongle reference sources\n\n")
        fh.write("The raw reverse-engineered sources are NOT committed to this "
                 "repo. They are\nrecorded here by SHA-256 so the generated "
                 "`cubase_lut.h` can be re-derived and\nverified. Regenerate "
                 "with `tools/generate_lut.py --source-dir <dir>`.\n\n")
        fh.write("| role | canonical filename | SHA-256 |\n")
        fh.write("| --- | --- | --- |\n")
        for role, p in src_paths.items():
            fh.write(f"| {role} | `{CANONICAL[role]}` | `{sha256(p)}` |\n")
        fh.write(f"\nFull 131072-transition SHA-256 signature: "
                 f"`{sig}`\n\n")
        fh.write(f"Reachable states: {meta['reachable_p15_0']} (pin15=0), "
                 f"{meta['reachable_p15_1']} (pin15=1), "
                 f"{meta['num_states']} (union). "
                 f"Power-on universes converge: {meta['converges']}.\n\n")
        fh.write("Origin: atari-forum thread \"Cartridge keys and emulation\" "
                 "(t=20130); the m68k\nemulator is the Medway Boys 2022 Cubase "
                 "3.10 dongle crack.\n")


# --- main --------------------------------------------------------------------
def resolve_sources(args):
    paths = {}
    for role in CANONICAL:
        p = getattr(args, role)
        if p is None and args.source_dir:
            p = os.path.join(args.source_dir, CANONICAL[role])
        if not p or not os.path.isfile(p):
            sys.exit(f"error: missing {role} source (expected {CANONICAL[role]}). "
                     f"Pass --source-dir or --{role}.")
        paths[role] = p
    return paths


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--source-dir", help=f"dir holding {', '.join(CANONICAL.values())}")
    ap.add_argument("--equations", help="path to srd_ok.jed.txt")
    ap.add_argument("--fusemap", help="path to raw JEDEC SRD_OK")
    ap.add_argument("--crack", help="path to CUBASE.S")
    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ap.add_argument("--header", default=os.path.join(repo, "rp/src/include/cubase_lut.h"))
    ap.add_argument("--vectors", default=os.path.join(repo, "tools/vectors/cubase3.txt"))
    ap.add_argument("--provenance", default=os.path.join(repo, "tools/reference/PROVENANCE.md"))
    args = ap.parse_args()

    src = resolve_sources(args)
    print("Building three independent models ...")
    ta = build_table(make_step(parse_equations(src["equations"])))
    tb = build_table(make_step(fuses_to_eqs(load_fuses(src["fusemap"]))))
    tc = build_table(make_step(parse_crack(src["crack"])))

    print("Cross-verifying A (equations) == B (fuse map) == C (m68k) ...")
    n = cross_verify([ta, tb, tc])
    print(f"  OK: all three agree on {n} transitions")

    print("Building lookup table (BFS from reset, both pin15 universes) ...")
    states, ident, lut, meta = build_lut(tb)  # generate from the silicon
    print(f"  states: {meta['reachable_p15_0']} (pin15=0) / "
          f"{meta['reachable_p15_1']} (pin15=1) / {meta['num_states']} (union); "
          f"converge={meta['converges']}")

    print("Verifying LUT vs model (all reachable + 2M random walk) ...")
    verify_lut(tb, states, ident, lut)
    print("  OK")

    sig = transition_signature(tb)
    emit_header(args.header, lut, meta, sig)
    emit_vectors(args.vectors, tb)
    emit_provenance(args.provenance, src, sig, meta)
    print(f"Wrote {args.header}")
    print(f"Wrote {args.vectors}")
    print(f"Wrote {args.provenance}")
    print(f"LUT size: {meta['num_states'] * 2 * 2} bytes")


if __name__ == "__main__":
    main()
