/**
 * File: emul.c
 * Author: Diego Parrilla Santamaría
 * Date: February 2025, February-August 2026
 * Copyright: 2025-2026 - GOODDATA LABS
 * Description: Cubase dongle emulator — boot menu, dongle selection, and the
 *              [E]nter GEM mode commit.
 *
 * Slim build: no network, no microSD, no USB — just the cartridge-bus engines
 * (romemul/commemul), the VT52 terminal menu, and the ROM3 dongle state machine
 * (cubaseemul). The boot menu (copied from md-drives-emulator's flow) shows the
 * selected dongle version and a countdown; at zero, or on [E], the RP commits to
 * the dongle and boots GEM so Cubase can query it.
 */

#include "emul.h"

#include <stdio.h>
#include <string.h>

#include "aconfig.h"
#include "chandler.h"
#include "commemul.h"
#include "cubaseemul_dma.h"
#ifdef CUBASE2_GATE
#include "cubase2_monitor.h"
#endif
#include "display.h"
#include "memfunc.h"
#include "pico/stdlib.h"
#include "reset.h"
#include "romemul.h"
#include "select.h"
#include "target_firmware.h"  // Embedded m68k cartridge image
#include "term.h"

#define SLEEP_LOOP_MS 100
#define COUNTDOWN_START_SECONDS 20
#define ONE_SECOND_US 1000000
// After CMD_START, wait for the m68k to read the sentinel + run userfw (both
// ROM4 reads served by romemul) before the PIO+DMA engine frees romemul.
#define GEM_BOOT_SETTLE_MS 1500
#define DISPLAY_TERM_CHAR_HEIGHT 8  // 8x8 terminal font
#define STATUS_MSG_MAX 64

// Dongle catalogue. Each family lists the Cubase releases it covers. Only the
// families with `available == true` are implemented and shown in the menu; the
// rest are placeholders so the taxonomy is documented and easy to extend.
typedef struct {
  const char *id;              // persisted in ACONFIG_PARAM_DONGLE
  const char *label;           // family name shown in the menu
  const char *const *covers;   // NULL-terminated list of covered Cubase versions
  bool available;              // is this dongle emulation implemented yet?
} DongleVariant;

static const char *const CUBASE_V1_COVERS[] = {"Cubase 1.0", "Cubase 1.50",
                                               "Cubase 1.51", NULL};
static const char *const CUBASE_V2_COVERS[] = {"Cubase 2.0", "Cubase 2.01",
                                               "Cubase 2.2x", NULL};
static const char *const CUBASE_V3_COVERS[] = {"Cubase 3.0", "Cubase 3.01",
                                               "Cubase 3.10", "Cubase Score 2.x",
                                               NULL};
static const char *const CAF_COVERS[] = {"CAF 2.01", "CAF 2.02", "CAF 2.06",
                                         "CAF 3.01", NULL};

static const DongleVariant DONGLES[] = {
    {"CUBASE_V1", "Cubase V1", CUBASE_V1_COVERS, false},
    {"CUBASE_V2_BLACK", "Cubase V2 (black)", CUBASE_V2_COVERS, false},
    {"CUBASE_V3", "Cubase V3", CUBASE_V3_COVERS, true},
    {"CUBASE_AUDIO_FALCON", "Cubase Audio Falcon", CAF_COVERS, false},
};
#define DONGLE_COUNT ((int)(sizeof(DONGLES) / sizeof(DONGLES[0])))

// Command handlers
static void cmdMenu(const char *arg);
static void cmdEnterGem(const char *arg);
static void cmdBooster(const char *arg);
static void cmdCycleDongle(const char *arg);

// Single-key command table (dispatched by term.c at SINGLE_KEY level).
static const Command commands[] = {
    {"m", cmdMenu},
    {"d", cmdCycleDongle},
    {"e", cmdEnterGem},
    {"x", cmdBooster},
};
static const size_t numCommands = sizeof(commands) / sizeof(commands[0]);

static bool keepActive = true;
static bool jumpToBooster = false;
static bool menuScreenActive = false;

// Auto-boot countdown (copied from md-drives-emulator).
static int countdown = 0;
static bool haltCountdown = false;
static absolute_time_t lastDecrement;
// Redraw the status bar only when its content changes (avoids flicker from
// redrawing every loop). Set on a countdown tick, a halt, or a menu re-render.
static bool infoLineDirty = false;

// Index into DONGLES[] of the current selection, synced with ACONFIG_PARAM_DONGLE.
static int selectedDongle = 0;

// Physical SELECT button. Polled on Core0 via select_checkPushReset() so it
// keeps working during dongle runtime (Core1 is busy serving the dongle). The
// callbacks run on Core0, so they reset the device directly.
//   Short press = back to the dongle menu (reboot into the menu).
//   Long press  = back to Booster.
static void onSelectShortPress(void) { reset_device(); }
static void onSelectLongPress(void) { reset_jump_to_booster(); }

static int availableDongleCount(void) {
  int count = 0;
  for (int i = 0; i < DONGLE_COUNT; i++) {
    if (DONGLES[i].available) {
      count++;
    }
  }
  return count;
}

static int firstAvailableDongle(void) {
  for (int i = 0; i < DONGLE_COUNT; i++) {
    if (DONGLES[i].available) {
      return i;
    }
  }
  return 0;  // no dongle implemented (should not happen)
}

// Next implemented dongle after `from`, wrapping. Stays put if only one exists.
static int nextAvailableDongle(int from) {
  for (int step = 1; step <= DONGLE_COUNT; step++) {
    int idx = (from + step) % DONGLE_COUNT;
    if (DONGLES[idx].available) {
      return idx;
    }
  }
  return from;
}

// Resolve a persisted id to an implemented dongle, else the first available one.
static int findAvailableDongle(const char *dongleId) {
  if (dongleId != NULL) {
    for (int i = 0; i < DONGLE_COUNT; i++) {
      if (DONGLES[i].available && (strcmp(dongleId, DONGLES[i].id) == 0)) {
        return i;
      }
    }
  }
  return firstAvailableDongle();
}

// Read the persisted dongle id, defaulting to the first available family.
static void loadSelectedDongle(void) {
  SettingsConfigEntry *entry =
      settings_find_entry(aconfig_getContext(), ACONFIG_PARAM_DONGLE);
  selectedDongle = findAvailableDongle((entry != NULL) ? entry->value : NULL);
}

static void persistSelectedDongle(void) {
  settings_put_string(aconfig_getContext(), ACONFIG_PARAM_DONGLE,
                      DONGLES[selectedDongle].id);
  settings_save(aconfig_getContext(), true);
}

static void showTitle(void) {
  term_printString("\x1B"
                   "E"
                   "Cubase Dongle Emulator - " RELEASE_VERSION "\n");
}

// Draw the inverted status/countdown bar on the bottom terminal row directly
// via u8g2 (copied from md-drives-emulator's drawSetupInfoLine).
static void drawSetupInfoLine(const char *message) {
  u8g2_SetDrawColor(display_getU8g2Ref(), 1);
  u8g2_DrawBox(display_getU8g2Ref(), 0,
               DISPLAY_HEIGHT - DISPLAY_TERM_CHAR_HEIGHT, DISPLAY_WIDTH,
               DISPLAY_TERM_CHAR_HEIGHT);
  u8g2_SetFont(display_getU8g2Ref(), u8g2_font_squeezed_b7_tr);
  u8g2_SetDrawColor(display_getU8g2Ref(), 0);
  u8g2_DrawStr(display_getU8g2Ref(), 0, DISPLAY_HEIGHT - 1,
               (message != NULL) ? message : "");
  u8g2_SetDrawColor(display_getU8g2Ref(), 1);
  u8g2_SetFont(display_getU8g2Ref(), u8g2_font_amstrad_cpc_extended_8f);
}

static void __not_in_flash_func(showCounter)(int cdown) {
  char msg[STATUS_MSG_MAX];
  if (cdown > 0) {
    snprintf(msg, sizeof(msg), "Booting GEM in %d seconds...", cdown);
  } else {
    snprintf(msg, sizeof(msg), "Booting GEM... please wait...");
  }
  drawSetupInfoLine(msg);
}

// Redraw the bottom status bar when its content has changed (infoLineDirty):
// the live countdown while it runs, or a "stopped" note once any key halts it.
static void refreshInfoLine(void) {
  if (!menuScreenActive || !infoLineDirty) {
    return;
  }
  infoLineDirty = false;
  if (haltCountdown) {
    drawSetupInfoLine("Countdown stopped. [E] Enter GEM    [X] Back to Booster");
  } else {
    showCounter(countdown);
  }
}

static void menu(void) {
  menuScreenActive = true;
  infoLineDirty = true;  // the screen clear below wipes the status bar
  showTitle();
  term_printString("\n");
  term_printString("Emulated dongle:  ");
  term_printString(DONGLES[selectedDongle].label);
  term_printString("\n");
  for (const char *const *cover = DONGLES[selectedDongle].covers;
       *cover != NULL; cover++) {
    term_printString("    - ");
    term_printString(*cover);
    term_printString("\n");
  }
  term_printString("\n");
  if (availableDongleCount() > 1) {
    term_printString("[D] Change dongle\n\n");
  }
  term_printString("[E] Enter GEM      [X] Back to Booster\n\n");
  term_printString("Select an option: ");
  term_markMenuPromptCursor();
}

static void cmdMenu(const char *arg) {
  haltCountdown = true;
  menu();
  display_refresh();
}

static void cmdCycleDongle(const char *arg) {
  haltCountdown = true;
  selectedDongle = nextAvailableDongle(selectedDongle);
  persistSelectedDongle();
  menu();
  display_refresh();
}

static void cmdEnterGem(const char *arg) {
  menuScreenActive = false;
  haltCountdown = true;
  persistSelectedDongle();
  term_printString("Committing to dongle mode and booting GEM...\n");
  display_refresh();
  // Mode commit (EPIC-07, zero-CPU PIO+DMA). Boot GEM FIRST: tell the m68k to
  // read CMD_START and run userfw — both are ROM4 reads that romemul must still
  // serve. After a settle delay, cubaseemul_dma_start() frees commemul AND
  // romemul to reclaim all of pio0, builds the DMA LUT, and stands up the
  // PIO + 2-DMA engine (no Core1). One-way — reset the MultiDevice for the menu.
  SEND_COMMAND_TO_DISPLAY(DISPLAY_COMMAND_START);
  sleep_ms(GEM_BOOT_SETTLE_MS);
  cubaseemul_dma_start();
  // Stay in the loop: the PIO+DMA engine serves the dongle while Cubase runs.
}

static void cmdBooster(const char *arg) {
  menuScreenActive = false;
  haltCountdown = true;
  term_printString("Launching Booster app...\n");
  display_refresh();
  jumpToBooster = true;
  keepActive = false;
}

static bool getKeepActive(void) { return keepActive; }

static void init(void) {
  term_setCommands(commands, numCommands);
  term_clearScreen();
  countdown = COUNTDOWN_START_SECONDS;
  haltCountdown = false;
  menu();
  refreshInfoLine();
  display_refresh();
}

#ifdef CUBASE2_GATE
// EPIC-11 gate: boot GEM (so the ST generates real bus traffic and romemul is no
// longer needed), then take the bus and watch /UDS, reporting over the serial
// console. Build with `CUBASE2_GATE=1 ./build.sh pico_w debug <uuid>` (debug so
// stdio reaches the UART). Never returns; reset the MultiDevice to exit.
static void cubase2_gate_run(void) {
  term_clearScreen();
  term_printString("\x1B"
                   "E"
                   "Cubase 2 /UDS monitor gate\n\n"
                   "Booting GEM, then watching /UDS on the serial console...\n");
  display_refresh();
  SEND_COMMAND_TO_DISPLAY(DISPLAY_COMMAND_START);
  sleep_ms(GEM_BOOT_SETTLE_MS);
  cubase2_monitor_gate_start();

  uint32_t previous = 0;
  while (true) {
    sleep_ms(1000);
    uint32_t captures = cubase2_monitor_captures();
    bool stalled = cubase2_monitor_consume_rxstall();
    printf("[cubase2-gate] state=0x%02X captures=%lu (+%lu/s) missed_edge=%s\n",
           cubase2_monitor_state(), (unsigned long)captures,
           (unsigned long)(captures - previous), stalled ? "YES(!)" : "no");
    previous = captures;
  }
}
#endif

void emul_start() {
  // Copy the m68k cartridge driver into the emulated ROM and bring up the
  // cartridge-bus engines: ROM4 read (romemul) + ROM3 command capture
  // (commemul). Both are load-bearing, so failure is fatal.
  COPY_FIRMWARE_TO_RAM((uint16_t *)target_firmware, target_firmware_length);
  if (init_romemul(false) < 0) {
    panic("init_romemul failed: PIO/DMA claim or program load returned <0");
  }
  if (commemul_init() < 0) {
    panic("commemul_init failed: PIO/DMA claim or program load returned <0");
  }
  chandler_init();
  chandler_addCB(term_command_cb);

  // Terminal display + SELECT button. Polled on Core0 (select_checkPushReset in
  // the main loop) so it survives the dongle commit that claims Core1.
  display_setupU8g2();
  select_configure();
  select_setResetCallback(onSelectShortPress);
  select_setLongResetCallback(onSelectLongPress);

#ifdef CUBASE2_GATE
  cubase2_gate_run();  // EPIC-11 /UDS-monitor gate; never returns
#endif

  // Read the persisted dongle selection and show the boot menu.
  loadSelectedDongle();
  init();

  // Main loop: drain ROM3 commands, run the terminal, tick the auto-boot
  // countdown. Any command handler halts the countdown (menu redraw, cycle,
  // enter, booster). At zero it enters GEM with the selected dongle.
  lastDecrement = get_absolute_time();
  while (getKeepActive()) {
    sleep_ms(SLEEP_LOOP_MS);
    chandler_loop();
    term_loop();
    // Poll the physical SELECT button (short = back to menu, long = Booster).
    select_checkPushReset();

    // Any keystroke (mapped or not) stops the auto-boot countdown.
    if (term_consumeKeyPressed() && menuScreenActive && !haltCountdown) {
      haltCountdown = true;
      infoLineDirty = true;
    }

    if (menuScreenActive && !haltCountdown) {
      absolute_time_t now = get_absolute_time();
      if (absolute_time_diff_us(lastDecrement, now) >= ONE_SECOND_US) {
        lastDecrement = now;
        countdown--;
        infoLineDirty = true;
        if (countdown <= 0) {
          cmdEnterGem(NULL);
        }
      }
    }

    // Redraw the status bar only when its content changed (no flicker).
    refreshInfoLine();
  }

  // The loop only exits for Booster. Reset the m68k, then jump to Booster.
  sleep_ms(SLEEP_LOOP_MS);
  SEND_COMMAND_TO_DISPLAY(DISPLAY_COMMAND_RESET);
  sleep_ms(SLEEP_LOOP_MS);
  if (jumpToBooster) {
    reset_jump_to_booster();
  } else {
    reset_device();
  }
}
