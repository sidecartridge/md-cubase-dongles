/**
 * File: cubase2emul.c
 * Author: Diego Parrilla Santamaría
 * Date: August 2026
 * Copyright: 2026 - GOODDATA LABS SL
 * Description: Cubase 2 black-dongle engine (EPIC-12): /UDS monitor + ROM3 drive.
 *
 * The black dongle is an 8-bit registered FSM clocked on /UDS (== A0), advancing
 * on EVERY 68000 bus cycle, not just ROM3 reads (D-12, C-06). Two engines run
 * together:
 *
 *   - Monitor (pio1 + Core1): cubase2_monitor.pio captures A[8:1] on each /UDS
 *     edge; the Core1 tracker (proven in EPIC-11) applies the verified cubase2_lut
 *     (+ the 0xD8 reset) and keeps a live g_driveWord = (state<<8) | 0xFF.
 *   - Drive (pio0 + 2 DMAs): cubase2emul.pio owns the bus, idles with /READ active
 *     (so the monitor keeps seeing the address), and on a /ROM3 read fetches
 *     g_driveWord fresh (the red engine's address->data DMA trick, fixed address)
 *     and drives it. The 0xFF low byte drives GP6 (A0/UDS position) high during
 *     the drive, keeping the monitor's /UDS pacing well-behaved through the window
 *     where /READ is off.
 *
 * This is an all-new module; the frozen red engine (cubaseemul_dma.*, cubase_lut.h)
 * is untouched (D-11). The two families are mutually exclusive at the commit.
 */

#include "cubase2emul.h"

#include "commemul.h"
#include "constants.h"
#include "cubase2_lut.h"  // defines cubase2_lut[256][8]
#include "cubase2_monitor.pio.h"
#include "cubase2emul.pio.h"
#include "debug.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "romemul.h"

static PIO drivePio = pio0;  // owns the bus + READ/WRITE, drives on ROM3
static PIO monPio = pio1;    // spare SM, reads the bus (never drives)
static int monSm = -1;

// The live response word, (state<<8) | 0xFF in the low 16 bits. Core1 updates it
// every /UDS edge; the drive DMA reads it fresh on each ROM3 access. 32-bit so a
// single DMA_SIZE_32 transfer feeds the PIO TX FIFO. Reset state 0 -> 0x00FF.
static volatile uint32_t g_driveWord = CUBASE2_RESPONSE_WORD(CUBASE2_LUT_RESET_STATE);

// Health/observability for the first hardware bring-up (Core0 heartbeat): the
// tracked state and a capture counter. If the monitor stays healthy while the
// drive runs, captures climb fast and RXSTALL never sets (the drive did not
// corrupt the /UDS pacing).
static volatile uint8_t trackedState = CUBASE2_LUT_RESET_STATE;
static volatile uint32_t captureCount = 0;

// Core1: one FSM step per /UDS-edge capture, then refresh g_driveWord. SRAM-
// resident + __not_in_flash_func so it never stalls on XIP (C-04); the compute is
// a single table index.
static void __not_in_flash_func(cubase2emul_core1)(void) {
  uint8_t state = CUBASE2_LUT_RESET_STATE;
  uint32_t count = 0;
  g_driveWord = CUBASE2_RESPONSE_WORD(state);
  while (true) {
    uint32_t sample = pio_sm_get_blocking(monPio, (uint)monSm);
    uint8_t a = (uint8_t)(sample & 0xFFu);  // A[8:1], A1 = bit 0
    if (a == CUBASE2_LUT_SPECIAL_ADDR) {
      state = CUBASE2_LUT_RESET_STATE;
    } else {
      state = cubase2_lut[state][CUBASE2_INPUT_CLASS(a)];
    }
    g_driveWord = CUBASE2_RESPONSE_WORD(state);
    trackedState = state;
    captureCount = ++count;
  }
}

uint8_t cubase2emul_state(void) { return trackedState; }

uint32_t cubase2emul_captures(void) { return captureCount; }

bool cubase2emul_consume_missed_edge(void) {
  if (monSm < 0) {
    return false;
  }
  uint32_t mask = 1u << (PIO_FDEBUG_RXSTALL_LSB + monSm);
  bool stalled = (monPio->fdebug & mask) != 0;
  monPio->fdebug = mask;  // write-1-to-clear
  return stalled;
}

// Stand up the ROM3 drive on pio0: the SM plus the two chained DMAs that fetch
// g_driveWord fresh on every access (RX addr -> lookup.read_addr_trig -> TX word).
static void start_drive_engine(void) {
  // The bus + READ/WRITE pins stay assigned to pio0 from romemul; only ROM3 needs
  // (re)claiming as a pulled-up input.
  pio_gpio_init(drivePio, ROM3_GPIO);
  gpio_set_dir(ROM3_GPIO, GPIO_IN);
  gpio_pull_up(ROM3_GPIO);

  int offset = pio_add_program(drivePio, &cubase2emul_drive_program);
  if (offset < 0) {
    panic("cubase2emul: drive pio_add_program failed (%d)", offset);
  }
  int sm = pio_claim_unused_sm(drivePio, true);
  cubase2emul_drive_program_init(drivePio, (uint)sm, (uint)offset,
                                 READ_ADDR_GPIO_BASE, READ_SIGNAL_GPIO_BASE,
                                 SAMPLE_DIV_FREQ);

  int lookupDma = dma_claim_unused_channel(true);
  int addrDma = dma_claim_unused_channel(true);

  // lookup DMA: read the 32-bit word at [read_addr] -> PIO TX FIFO, chain to addr.
  dma_channel_config cLookup = dma_channel_get_default_config(lookupDma);
  channel_config_set_transfer_data_size(&cLookup, DMA_SIZE_32);
  channel_config_set_read_increment(&cLookup, false);
  channel_config_set_write_increment(&cLookup, false);
  channel_config_set_dreq(&cLookup, pio_get_dreq(drivePio, (uint)sm, true));
  channel_config_set_chain_to(&cLookup, (uint)addrDma);
  dma_channel_configure(lookupDma, &cLookup, &drivePio->txf[sm], NULL, 1, false);

  // addr DMA: read the fetch address from PIO RX FIFO -> lookup's read_addr_trig.
  dma_channel_config cAddr = dma_channel_get_default_config(addrDma);
  channel_config_set_transfer_data_size(&cAddr, DMA_SIZE_32);
  channel_config_set_read_increment(&cAddr, false);
  channel_config_set_write_increment(&cAddr, false);
  channel_config_set_dreq(&cAddr, pio_get_dreq(drivePio, (uint)sm, false));
  dma_channel_configure(addrDma, &cAddr,
                        &dma_hw->ch[lookupDma].al3_read_addr_trig,
                        &drivePio->rxf[sm], 1, true);

  pio_sm_clear_fifos(drivePio, (uint)sm);
  pio_sm_restart(drivePio, (uint)sm);
  pio_sm_set_enabled(drivePio, (uint)sm, true);
  // Hand the preamble the fixed fetch address (consumed by its `pull`).
  pio_sm_put_blocking(drivePio, (uint)sm, (uint32_t)&g_driveWord);

  DPRINTF("Cubase2 ROM3 drive up: pio0/sm%d, g_driveWord @ %08lx\n", sm,
          (unsigned long)&g_driveWord);
}

// Stand up the continuous /UDS monitor on pio1 and launch the Core1 FSM tracker.
static void start_monitor_engine(void) {
  int offset = pio_add_program(monPio, &cubase2_monitor_program);
  if (offset < 0) {
    panic("cubase2emul: monitor pio_add_program failed (%d)", offset);
  }
  monSm = pio_claim_unused_sm(monPio, true);
  // A1 = GP7 = READ_ADDR_GPIO_BASE + 1; A0(/UDS) = GP6 is waited on absolutely.
  cubase2_monitor_program_init(monPio, (uint)monSm, (uint)offset,
                               READ_ADDR_GPIO_BASE + 1, SAMPLE_DIV_FREQ);
  pio_sm_clear_fifos(monPio, (uint)monSm);
  pio_sm_restart(monPio, (uint)monSm);

  // Launch the tracker BEFORE enabling the SM (EPIC-11): Core1 reaches its
  // blocking FIFO read while the SM is still stopped, so no capture is lost while
  // the core spins up.
  multicore_reset_core1();
  multicore_launch_core1(cubase2emul_core1);
  sleep_ms(1);
  pio_sm_set_enabled(monPio, (uint)monSm, true);
  DPRINTF("Cubase2 /UDS monitor up: pio1/sm%d\n", monSm);
}

void cubase2emul_start(void) {
  // Take exclusive control of the cartridge bus: the setup-mode engines are done
  // (romemul already served the CMD_START read; the caller waited). The bus +
  // READ/WRITE pins stay assigned to pio0, so the drive SM inherits them.
  commemul_deinit();
  romemul_deinit();

  start_drive_engine();
  start_monitor_engine();
}
