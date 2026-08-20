/**
 * File: cubaseemul_dma.c
 * Author: Diego Parrilla Santamaría
 * Date: August 2026
 * Copyright: 2026 - GOODDATA LABS SL
 * Description: Cubase 3 red-dongle ROM3 state machine, zero-CPU PIO+DMA (EPIC-07).
 *
 * The Core1 engine (cubaseemul.c) keeps state in C and does the LUT step on
 * Core1. This version removes the CPU from the response path: the PIO program
 * (cubaseemul_dma.pio) keeps the state in scratch register Y and two chained DMA
 * channels do the LUT lookup + next-state feedback -- the romemul "address ->
 * data" trick plus feedback. Kept alongside the Core1 build as the preferred
 * end-state (DECISIONS D-04); the Core1 version remains the reference/fallback.
 */

#include "cubaseemul_dma.h"

#include "commemul.h"
#include "constants.h"
#include "cubase_lut.h"
#include "cubaseemul_dma.pio.h"
#include "debug.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "romemul.h"

static PIO donglePio = pio0;

// Per-state registered output D8 (the CURRENT state's output). Derived from the
// committed 16-bit LUT: each entry stores the D8 of its NEXT state, so one scan
// of the table yields every reachable state's registered output.
static uint8_t stateD8[CUBASE_LUT_NUM_STATES];

// Build the 32-bit DMA LUT: entry[s][a] = (output_word(s) << 16) | F(s, a).
// output_word(s) is the current state's registered D8 -> 0xFFFF / 0xFEFF, so a
// single self-contained access reproduces the real 5C060 pipeline (D8 during an
// access reflects the current state; A8 selects the next state).
static void build_dma_lut(uint32_t *dmaLut) {
  stateD8[CUBASE_LUT_RESET_STATE_ID] = 0;  // reset outputs D8 = 0 (0xFEFF)
  for (uint32_t s = 0; s < CUBASE_LUT_NUM_STATES; s++) {
    for (uint32_t a = 0; a < 2; a++) {
      uint16_t entry = cubase_lut[s][a];
      stateD8[CUBASE_LUT_NEXT_STATE(entry)] = (uint8_t)CUBASE_LUT_D8(entry);
    }
  }
  for (uint32_t s = 0; s < CUBASE_LUT_NUM_STATES; s++) {
    uint32_t outWord = stateD8[s] ? 0xFFFFu : 0xFEFFu;
    for (uint32_t a = 0; a < 2; a++) {
      uint16_t next = (uint16_t)CUBASE_LUT_NEXT_STATE(cubase_lut[s][a]);
      dmaLut[(s * 2) + a] = (outWord << 16) | next;
    }
  }
}

void cubaseemul_dma_start(void) {
  // Reclaim all of pio0: free the ROM3 command channel and the ROM4 read
  // engine. romemul must already have served the CMD_START read (caller waited).
  commemul_deinit();
  romemul_deinit();

  // Build the DMA LUT into the freed ROM_IN_RAM. It is 64KB-aligned
  // (0x20030000, memmap_rp.ld), so the PIO can form addresses as base | offset.
  // ~48KB, inside the 64KB region and below the display framebuffer.
  uint32_t *dmaLut = (uint32_t *)&__rom_in_ram_start__;
  build_dma_lut(dmaLut);
  uint32_t lutBase = (uint32_t)dmaLut;

  // ROM3 as a pulled-up input. The bus + READ/WRITE pins stay assigned to pio0
  // from romemul, so the new SM inherits them.
  pio_gpio_init(donglePio, ROM3_GPIO);
  gpio_set_dir(ROM3_GPIO, GPIO_IN);
  gpio_pull_up(ROM3_GPIO);

  int offset = pio_add_program(donglePio, &cubaseemul_dma_read_program);
  if (offset < 0) {
    panic("cubaseemul_dma_start: pio_add_program failed (%d)", offset);
  }
  int sm = pio_claim_unused_sm(donglePio, true);
  cubaseemul_dma_read_program_init(donglePio, (uint)sm, (uint)offset,
                                   READ_ADDR_GPIO_BASE, READ_ADDR_GPIO_BASE + 8,
                                   READ_SIGNAL_GPIO_BASE, SAMPLE_DIV_FREQ);

  // Two chained DMAs (romemul's address->data trick, 32-bit entry):
  //   lookup DMA: reads the entry at [addr] -> PIO TX FIFO, chains back to addr.
  //   addr DMA:   reads the address from PIO RX FIFO -> lookup's read_addr_trig.
  int lookupDma = dma_claim_unused_channel(true);
  int addrDma = dma_claim_unused_channel(true);

  dma_channel_config cLookup = dma_channel_get_default_config(lookupDma);
  channel_config_set_transfer_data_size(&cLookup, DMA_SIZE_32);
  channel_config_set_read_increment(&cLookup, false);
  channel_config_set_write_increment(&cLookup, false);
  channel_config_set_dreq(&cLookup, pio_get_dreq(donglePio, (uint)sm, true));
  channel_config_set_chain_to(&cLookup, (uint)addrDma);
  dma_channel_configure(lookupDma, &cLookup, &donglePio->txf[sm], NULL, 1,
                        false);

  dma_channel_config cAddr = dma_channel_get_default_config(addrDma);
  channel_config_set_transfer_data_size(&cAddr, DMA_SIZE_32);
  channel_config_set_read_increment(&cAddr, false);
  channel_config_set_write_increment(&cAddr, false);
  channel_config_set_dreq(&cAddr, pio_get_dreq(donglePio, (uint)sm, false));
  dma_channel_configure(addrDma, &cAddr,
                        &dma_hw->ch[lookupDma].al3_read_addr_trig,
                        &donglePio->rxf[sm], 1, true);

  pio_sm_clear_fifos(donglePio, (uint)sm);
  pio_sm_restart(donglePio, (uint)sm);
  pio_sm_set_enabled(donglePio, (uint)sm, true);
  // Hand the SM the LUT base high word (consumed by the preamble `pull`).
  pio_sm_put_blocking(donglePio, (uint)sm, lutBase >> 16);

  DPRINTF("Cubase PIO+DMA dongle up: pio0/sm%d, LUT @ %08lx, %u states\n", sm,
          (unsigned long)lutBase, (unsigned)CUBASE_LUT_NUM_STATES);
}
