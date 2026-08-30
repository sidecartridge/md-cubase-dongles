/**
 * File: cubase2_monitor.c
 * Author: Diego Parrilla Santamaría
 * Date: August 2026
 * Copyright: 2026 - GOODDATA LABS SL
 * Description: Cubase 2 black-dongle /UDS bus monitor + FSM tracker (EPIC-11 gate).
 *
 * Gate #1 for the black dongle: prove the RP can track the 8-bit FSM by clocking
 * on A0(/UDS) on EVERY 68000 bus cycle without missing an edge (D-12, C-06). The
 * PIO (cubase2_monitor.pio) captures A[8:1] on each /UDS edge; this Core1 loop
 * applies the verified `cubase2_lut` (+ the 0xD8 reset). Monitor-only, so it
 * never drives the bus and never touches the frozen red engine (D-11).
 */

#include "cubase2_monitor.h"

#include "commemul.h"
#include "constants.h"
#include "cubase2_lut.h"  // defines cubase2_lut[256][8]
#include "cubase2_monitor.pio.h"
#include "debug.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "romemul.h"

static PIO monPio = pio1;  // spare SM on pio1 (pio0 stays with the red engine)
static int monSm = -1;

static volatile uint8_t trackedState = CUBASE2_LUT_RESET_STATE;
static volatile uint32_t captureCount = 0;

// Core1: one FSM step per /UDS-edge capture. SRAM-resident + __not_in_flash_func
// so it never stalls on XIP (C-04). The compute is a single table index.
static void __not_in_flash_func(cubase2_monitor_core1)(void) {
  uint8_t state = CUBASE2_LUT_RESET_STATE;
  uint32_t count = 0;
  while (true) {
    uint32_t sample = pio_sm_get_blocking(monPio, (uint)monSm);
    uint8_t a = (uint8_t)(sample & 0xFFu);  // A[8:1], A1 = bit 0
    if (a == CUBASE2_LUT_SPECIAL_ADDR) {
      state = CUBASE2_LUT_RESET_STATE;
    } else {
      state = cubase2_lut[state][CUBASE2_INPUT_CLASS(a)];
    }
    trackedState = state;
    captureCount = ++count;
  }
}

void cubase2_monitor_gate_start(void) {
  // Take exclusive control of the cartridge bus. The frozen red engine is not
  // involved; commemul/romemul are the setup-mode engines.
  commemul_deinit();
  romemul_deinit();

  // Hold /READ active (low) so the address side-shifter stays enabled and the
  // bus is readable continuously; /WRITE inactive (high); GP6-21 as inputs.
  // gpio_init returns each pin to SIO so we drive/read it directly.
  gpio_init(READ_SIGNAL_GPIO_BASE);
  gpio_set_dir(READ_SIGNAL_GPIO_BASE, GPIO_OUT);
  gpio_put(READ_SIGNAL_GPIO_BASE, 0);
  gpio_init(WRITE_SIGNAL_GPIO_BASE);
  gpio_set_dir(WRITE_SIGNAL_GPIO_BASE, GPIO_OUT);
  gpio_put(WRITE_SIGNAL_GPIO_BASE, 1);
  for (int i = 0; i < READ_ADDR_PIN_COUNT; i++) {
    gpio_init(READ_ADDR_GPIO_BASE + i);
    gpio_set_dir(READ_ADDR_GPIO_BASE + i, GPIO_IN);
  }

  int offset = pio_add_program(monPio, &cubase2_monitor_program);
  if (offset < 0) {
    panic("cubase2_monitor: pio_add_program failed (%d)", offset);
  }
  monSm = pio_claim_unused_sm(monPio, true);
  // A1 = GP7 = READ_ADDR_GPIO_BASE + 1; A0(/UDS) = GP6 is waited on absolutely.
  cubase2_monitor_program_init(monPio, (uint)monSm, (uint)offset,
                               READ_ADDR_GPIO_BASE + 1, 1.f);
  pio_sm_clear_fifos(monPio, (uint)monSm);
  pio_sm_restart(monPio, (uint)monSm);

  // Launch the tracker BEFORE enabling the SM: Core1 reaches its blocking FIFO
  // read while the SM is still stopped (empty FIFO), so the first captures are
  // drained immediately and the FIFO never fills during core launch (otherwise
  // that startup window trips RXSTALL for one report).
  multicore_reset_core1();
  multicore_launch_core1(cubase2_monitor_core1);
  sleep_ms(1);
  monPio->fdebug = 1u << (PIO_FDEBUG_RXSTALL_LSB + monSm);  // clear stale stall
  pio_sm_set_enabled(monPio, (uint)monSm, true);
  DPRINTF("Cubase2 /UDS monitor up: pio1/sm%d\n", monSm);
}

uint8_t cubase2_monitor_state(void) { return trackedState; }

uint32_t cubase2_monitor_captures(void) { return captureCount; }

bool cubase2_monitor_consume_rxstall(void) {
  if (monSm < 0) {
    return false;
  }
  uint32_t mask = 1u << (PIO_FDEBUG_RXSTALL_LSB + monSm);
  bool stalled = (monPio->fdebug & mask) != 0;
  monPio->fdebug = mask;  // write-1-to-clear
  return stalled;
}
