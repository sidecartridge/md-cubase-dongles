# Changelog

## v1.0.0beta (2026-08-20) - first release

First public build of the **Cubase Dongle Emulator** — a SidecarTridge
MultiDevice microfirmware that emulates the Steinberg Cubase copy-protection
dongle, so you can run Cubase on a real Atari ST / STE / MegaST(E) / TT / Falcon
without the original hardware key.

### Dongle emulation

- Emulates the **Cubase 3 "red dongle"** (Intel 5C060 registered PLD) — a 16-bit
  state machine, not a stored key — answered on ROM3 (`$FB0000`) exactly like the
  real silicon.
- Covers the **Cubase V3 family**: Cubase 3.0, 3.01, 3.10 and Cubase Score 2.x.
  Verified on hardware — Cubase 3.10 and Cubase Score 2.0 both accept the dongle
  and run.
- The state machine is compressed to a ~24 KB lookup table generated from three
  independent, cross-checked hardware descriptions (JEDEC fuse map, de-fused
  equations, and an m68k reference model) that agree on all 131072 transitions.

### Zero-CPU engine

- The per-access response is served entirely by **PIO + DMA**: the FSM state
  lives in a PIO register and two chained DMA channels do the LUT lookup and
  next-state feedback, with no CPU on the response path. This removes the
  cartridge-bus timing race under Cubase's fast read bursts.

### Boot menu

- A boot menu is shown on every power-on: choose the dongle family, and an
  auto-boot countdown launches straight into GEM. Any key stops the countdown.
  - `[E]` enter GEM with the selected dongle · `[X]` back to Booster · `[D]`
    change dongle (shown once more than one family is available).
  - The physical SELECT button returns to the menu (short press) or to Booster
    (long press), and keeps working while Cubase is running.
- The catalogue is structured for the whole Cubase line — Cubase V1, V2 (black),
  V3, and Cubase Audio Falcon — with the not-yet-developed families hidden. Only
  **Cubase V3** ships in this release; the others are groundwork.

### Slim firmware

- Stripped to the essentials for a dongle emulator: no Wi-Fi, no microSD, no
  USB, no status LED.

### Notes

- Beta build: ships with a placeholder store icon and the development app UUID;
  a real icon and a minted UUID land with the stable public release.
