# Provenance -- Cubase 3 red dongle reference sources

The raw reverse-engineered sources are NOT committed to this repo. They are
recorded here by SHA-256 so the generated `cubase_lut.h` can be re-derived and
verified. Regenerate with `tools/generate_lut.py --source-dir <dir>`.

| role | canonical filename | SHA-256 |
| --- | --- | --- |
| equations | `srd_ok.jed.txt` | `fd197818887bf55cd0cbe95654a201b3feb2b674b4ed6d7a04140bbf40f6aa0a` |
| fusemap | `SRD_OK` | `8aa830998dab68e2d12ff3e10b644294ea33e98f244be4a0e428874ec39971f7` |
| crack | `CUBASE.S` | `7f16933feafec7f3f88a1d82e131d0f42c49f28f301a8655d228fd657e9f5322` |

Full 131072-transition SHA-256 signature: `e039b544a5c15cf4e21612b0aa35435156758451eb24c8e0419fe25156e45a18`

Reachable states: 5999 (pin15=0), 6149 (pin15=1), 6150 (union). Power-on universes converge: True.

Origin: atari-forum thread "Cartridge keys and emulation" (t=20130); the m68k
emulator is the Medway Boys 2022 Cubase 3.10 dongle crack.

---

# Provenance -- Cubase 2 black dongle reference source

The Cubase 2 "black dongle" (routine A) is derived from the MiSTery open-source
Atari ST core's functional model, `atarist/cubase2_dongle.v`. It is public GPL
code; we do not vendor it here. The eight next-state equations are transcribed
into `tools/generate_lut.py`'s sibling `tools/generate_cubase2_lut.py`, and the
2048-byte `rp/src/include/cubase2_lut.h` is verified exhaustively against them
(`tools/test_cubase2_lut.py`, run in CI).

| role | source | identifier |
| --- | --- | --- |
| Verilog model | `gyurco/MiSTery` `atarist/cubase2_dongle.v` | git blob `2df21d25e65ca09d1419b37a5f36104d3e7b7ef7` |

- Source URL: <https://github.com/gyurco/MiSTery/blob/master/atarist/cubase2_dongle.v>
- Referenced commit: `15b528b54beb48fbcb26e32677685015ae5d6629`
- SHA-256 of the file content: `544ee172b71bfda889fda2289bdf3ce23d8a5ea376340b95467c87171b9ab546`

Model: 8-bit registered FSM, state = D[15:8], reset 0x00, clock = /UDS rising
edge, input = A[8:1], read = `state<<8 | 0xFF`. All 256 states reachable from
0x00; input reduces to `{A5,A4,A1}` outside the special address `A[8:1]==0xD8`
(which resets the machine). MiSTery added Cubase 2 dongle support in Apr 2022 --
an independent confirmation the equations are used in a working implementation.
