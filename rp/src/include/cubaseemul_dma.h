/**
 * File: cubaseemul_dma.h
 * Author: Diego Parrilla Santamaría
 * Date: August 2026
 * Copyright: 2026 - GOODDATA LABS SL
 * Description: Cubase 3 red-dongle ROM3 state machine, zero-CPU PIO+DMA (EPIC-07).
 */

#ifndef CUBASEEMUL_DMA_H
#define CUBASEEMUL_DMA_H

#include "pico/stdlib.h"

/**
 * Commit to zero-CPU dongle mode. Frees BOTH the ROM3 command channel
 * (commemul) and the ROM4 read engine (romemul) to reclaim all of pio0 and both
 * DMA channels, builds the 32-bit DMA LUT into the freed ROM_IN_RAM, and stands
 * up the PIO + two-chained-DMA state machine. No Core1 is used.
 *
 * MUST be called only after the m68k has read CMD_START and run userfw (romemul
 * must still be alive to serve those ROM4 reads) -- the caller inserts a settle
 * delay. After this, ROM4 is open-bus, which is safe because the cartridge is
 * never read again once GEM has booted.
 *
 * Panics on a fatal PIO/DMA claim or program-load failure.
 */
void cubaseemul_dma_start(void);

#endif  // CUBASEEMUL_DMA_H
