---
id: STORY-01
epic: EPIC-07
title: PIO+DMA feedback design and implementation
status: done
---

## Goal

The dongle responds with zero CPU involvement on the response path.

## Design

- **State in PIO (Y).** `cubaseemul_dma.pio` keeps the current state id in scratch
  register Y. Per ROM3 access it samples A8, builds the LUT byte address
  `base | (state<<3) | (a8<<2)` in the ISR (`mov isr,x` = base>>16, then
  `in y,13 / in pins,1 / in null,2` shift a 16-bit offset in under it) and
  autopushes it.
- **Two chained DMAs** (romemul's address→data trick): the addr DMA moves the
  autopushed address from the PIO RX FIFO into the lookup DMA's
  `read_addr_trig`; the lookup DMA fetches the 32-bit entry into the PIO TX FIFO
  and chains back. No CPU on the path.
- **Feedback in PIO.** PIO pulls the entry `{output_word<<16 | next_state}`,
  loads `next_state` into Y and drives `output_word` on the bus.
- **Pipeline fix.** A single self-contained access must reproduce the 5C060's
  *registered* output: D8 during access N reflects the CURRENT state, and A8_N
  selects the next state. So each entry stores `output_word(current state)`, not
  of the next state. `cubaseemul_dma.c` derives every state's registered D8 from
  the committed LUT (each entry carries its next state's D8) and builds the
  32-bit table at commit. The reset state's own entry carries 0xFEFF, so no
  pre-staging is needed.
- **pio0 budget.** The program is 23 instructions and must live on pio0 (only
  pio0 drives the bus). romemul (16) + this won't fit 32, so the commit frees
  **both** commemul and romemul (D-04 accepted freeing ROM4). The 48 KB DMA LUT
  is built into the freed, 64 KB-aligned ROM_IN_RAM (0x20030000), below the
  display framebuffer.
- **Ordering.** `cmdEnterGem` sends CMD_START and waits `GEM_BOOT_SETTLE_MS`
  (1500 ms) so the m68k reads the sentinel + runs userfw (ROM4 reads romemul
  must still serve) BEFORE `cubaseemul_dma_start()` frees romemul.

## Tasks

- [x] Keep state in a PIO scratch register (Y); form the index (state, a8)
- [x] DMA fetch a packed record {output_word, next_state} from the LUT
- [x] Drive output_word; feed next_state back into Y
- [x] `romemul_deinit()` to free ROM4 + reclaim pio0
- [x] Build + link clean (PIO fits 32 instructions)
- [x] Verify identical D8 behavior to the Core1 build + Cubase acceptance (HW)

## Acceptance

On hardware, D8 behavior is identical to EPIC-05, with no CPU on the response
path.

## Risks to validate on hardware (ranked)

1. **Freeing romemul mid-boot.** After the commit ROM4 is open-bus. If TOS/GEM
   reads the cartridge after `GEM_BOOT_SETTLE_MS`, it could hang. First check:
   does GEM still boot? Tune the delay or rethink if not.
2. **Engine correctness.** The PIO shift-counter assumptions (MOV resets the
   shift counter), the DMA feedback, and the pipeline LUT could have bugs —
   Cubase acceptance is the end-to-end test.
3. **Within-access DMA latency** vs the ROM3 window (expected fine; the removed
   between-access race was the point).

## Notes

Core1 engine (`cubaseemul.c`/`.pio`/`.h`) is kept in the repo as the reference/
fallback but dropped from the build (`cubase_lut.h` defines the table in one TU).
To fall back: re-add `cubaseemul.c` to CMakeLists and point `cmdEnterGem` at
`cubaseemul_start()`.
