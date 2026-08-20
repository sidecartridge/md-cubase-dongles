/**
 * File: cubaseemul.c
 * Author: Diego Parrilla Santamaría
 * Date: August 2026
 * Copyright: 2026 - GOODDATA LABS SL
 * Description: Cubase 3 red-dongle ROM3 state-machine engine (EPIC-05).
 *
 * The PIO program (cubaseemul.pio) does the ROM3 bus dance: it samples A8 into
 * the RX FIFO and drives the word Core1 stages into the TX FIFO. Core1 runs the
 * state machine: it reads A8, looks up cubase_lut[state][a8], advances the
 * state, and stages the next output word. State advances on the /ROM3 rising
 * edge, so the word driven during an access is the previous state's output --
 * the reset output is pre-staged before the SM is enabled and Core1 keeps the
 * pipeline one step ahead (see DECISIONS D-04/D-08).
 */

#include "cubaseemul.h"

#include "commemul.h"
#include "constants.h"
#include "cubase_lut.h"
#include "cubaseemul.pio.h"
#include "debug.h"
#include "hardware/pio.h"
#include "pico/multicore.h"

static PIO donglePio = pio0;
static int dongleSm = -1;

// Core1: the LUT state machine. Blocks on each ROM3 access, advances the state,
// and stages the next access's output word. __not_in_flash_func + the SRAM-
// resident cubase_lut keep it off the XIP path for deterministic timing (C-04).
static void __not_in_flash_func(cubaseemul_core1_loop)(void) {
  uint16_t state = CUBASE_LUT_RESET_STATE_ID;
  while (true) {
    uint32_t sample = pio_sm_get_blocking(donglePio, (uint)dongleSm);
    uint32_t a8 = (sample >> 8) & 1u;
    uint16_t entry = cubase_lut[state][a8];
    state = (uint16_t)CUBASE_LUT_NEXT_STATE(entry);
    pio_sm_put_blocking(donglePio, (uint)dongleSm, CUBASE_LUT_OUTPUT_WORD(entry));
  }
}

void cubaseemul_start(void) {
  // Free ROM3's command channel so pio0 has room for the dongle program; romemul
  // (ROM4) stays. commemul owns the shared bus/READ/WRITE pins on pio0, so the
  // dongle SM inherits them -- we only need ROM3 as a pulled-up input.
  commemul_deinit();
  pio_gpio_init(donglePio, ROM3_GPIO);
  gpio_set_dir(ROM3_GPIO, GPIO_IN);
  gpio_pull_up(ROM3_GPIO);

  int offset = pio_add_program(donglePio, &cubaseemul_read_program);
  if (offset < 0) {
    panic("cubaseemul_start: pio_add_program failed (%d)", offset);
  }
  dongleSm = pio_claim_unused_sm(donglePio, true);
  cubaseemul_read_program_init(donglePio, (uint)dongleSm, (uint)offset,
                               READ_ADDR_GPIO_BASE, READ_ADDR_PIN_COUNT,
                               READ_SIGNAL_GPIO_BASE, SAMPLE_DIV_FREQ);

  pio_sm_set_enabled(donglePio, (uint)dongleSm, false);
  pio_sm_clear_fifos(donglePio, (uint)dongleSm);
  pio_sm_restart(donglePio, (uint)dongleSm);

  // Pre-stage the reset state's output so the first access drives the right D8;
  // Core1 keeps the pipeline fed from then on.
  pio_sm_put_blocking(donglePio, (uint)dongleSm, CUBASE_LUT_RESET_OUTPUT_WORD);
  pio_sm_set_enabled(donglePio, (uint)dongleSm, true);

  // Core1 may already be running (e.g. a SELECT poll); reset it before handing
  // it the dongle loop.
  multicore_reset_core1();
  multicore_launch_core1(cubaseemul_core1_loop);

  DPRINTF("Cubase dongle engine up: pio0/sm%d, LUT %u states, reset id %u\n",
          dongleSm, (unsigned)CUBASE_LUT_NUM_STATES,
          (unsigned)CUBASE_LUT_RESET_STATE_ID);
}
