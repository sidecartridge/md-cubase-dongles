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
