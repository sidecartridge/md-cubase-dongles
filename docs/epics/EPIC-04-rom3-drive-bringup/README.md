# EPIC-04 gate #1 — how to run the ROM3-drive test

**Question this answers:** can the MultiDevice *drive* data onto the bus during a
ROM3 (`$FB0000`) read at all? Today `commemul` only ever *captures* on ROM3, so
driving it is unexercised. Everything in Iteration 2 depends on the answer
(DECISIONS.md D-07, C-01).

## How it works

Built with `CUBASE_GATE_TEST=1`, `emul_start` (RP side):

1. serves the cartridge on ROM4 (`romemul`) so it auto-runs on the ST,
2. drives the fixed word **`0xFEFF`** on every ROM3 access (`cubaseemul.c` /
   `cubaseemul.pio`, on pio0 alongside romemul — ROM3 and ROM4 are never active
   at once, so they don't contend), and
3. writes `CMD_START` into the cartridge sentinel so `main.s` hands control to
   `userfw.s` at boot (no command channel needed).

On the ST, **`userfw.s`** (the cartridge's app slot) reads `$FB0000` eight times
and prints them via GEMDOS. `0xFEFF` is D8=0 with every other data line high —
the dongle's "D8 low" response shape.

There is nothing to assemble or copy by hand: the cartridge ROM runs
automatically. The command channel, network and display loop are all skipped.

## Build and flash the gate firmware

```bash
# From the repo root. Debug gives serial heartbeat output.
CUBASE_GATE_TEST=1 ./build.sh pico_w debug 44444444-4444-4444-8444-444444444444
```

`build.sh` rebuilds the m68k cartridge (with the `userfw.s` peek) into
`target_firmware.h` and then the RP firmware. Flash
`dist/<uuid>-<version>.uf2` to the Pico. The serial console shows
`ROM3 gate #1 active ... driving 0xFEFF`.

> Build the **normal** firmware (no `CUBASE_GATE_TEST`) to restore ordinary
> setup-mode behaviour.

## Run it

Power the ST with the gate firmware in the MultiDevice. After GEMDOS init the
cartridge runs automatically and prints:

```
ROM3 peek $FB0000 (expect FEFF x8):
FEFF FEFF FEFF FEFF FEFF FEFF FEFF FEFF
```

**Pass:** all eight words are `FEFF`.
**Fail:** `FFFF`, random, or varying values — the MultiDevice is not driving
ROM3 data. Record the outcome in DECISIONS.md D-07; the dongle would then have
to move to ROM4 (a design pivot), and EPIC-05 changes accordingly.
