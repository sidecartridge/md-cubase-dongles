# Changelog

## v1.0.0beta (2026-08-20) - first release

First public build of the **Cubase Dongle Emulator**, a SidecarTridge
MultiDevice microfirmware that emulates Steinberg's Cubase copy-protection
dongles, so you can run Cubase on a real Atari ST / STE / MegaST(E) / TT / Falcon
without the original hardware key.

### Dongle emulation

- Emulates **two** dongles, picked from the boot menu:
  - **Cubase 3 "red dongle"** (Intel 5C060 registered PLD): a 16-bit state
    machine, answered on ROM3 (`$FB0000`) exactly like the real silicon. Covers
    the **Cubase V3 family**: Cubase 3.0, 3.01, 3.10 and Cubase Score 2.x.
  - **Cubase 2 "black dongle"**: an 8-bit state machine that advances on every
    68000 bus cycle (clocked on `/UDS`). Covers the **Cubase V2 family**: Cubase
    2.0, 2.01 and 2.2x.
- Neither is a stored key. Each state machine is compressed to a lookup table
  derived from cross-checked hardware descriptions and verified exhaustively: the
  red over all 131072 transitions (JEDEC fuse map, de-fused equations, and an
  m68k reference model), the black (2 KB table) over all 65536.
- Verified on hardware: Cubase 3.10 and Cubase Score 2.0 (red) and Cubase 2.01
  (black) all accept the emulated dongle and run.

### Response engines

- **Red (V3)**: served entirely by **PIO + DMA**. The FSM state lives in a PIO
  register and two chained DMA channels do the LUT lookup and next-state feedback,
  with no CPU on the response path. This removes the cartridge-bus timing race
  under Cubase's fast read bursts.
- **Black (V2)**: one PIO plus a dedicated core continuously track the machine on
  every bus cycle; a second PIO drives the response on each ROM3 read via the same
  DMA fetch trick.

### Boot menu

- A boot menu is shown on every power-on: choose the dongle family, and an
  auto-boot countdown launches straight into GEM. Any key stops the countdown.
  - `[E]` enter GEM with the selected dongle · `[X]` back to Booster · `[D]`
    change dongle (shown once more than one family is available).
  - The physical SELECT button returns to the menu (short press) or to Booster
    (long press), and keeps working while Cubase is running.
- The catalogue is structured for the whole Cubase line: Cubase V1, V2 (black),
  V3, and Cubase Audio Falcon, with the not-yet-developed families hidden.
  **Cubase V2 (black)** and **Cubase V3** ship in this release; Cubase V1 and
  Cubase Audio Falcon are groundwork.

### Slim firmware

- Stripped to the essentials for a dongle emulator: no Wi-Fi, no microSD, no
  USB, no status LED.

### Notes

- Beta build: ships with a placeholder store icon and the development app UUID;
  a real icon and a minted UUID land with the stable public release.
