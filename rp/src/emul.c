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

// Dongle catalogue — a cycle list, extensible. Only the Cubase 3 red dongle is
// implemented today; its id must exist in tools/generate_lut.py's output.
typedef struct {
  const char *id;     // persisted in ACONFIG_PARAM_DONGLE
  const char *label;  // shown in the menu
} DongleVariant;

static const DongleVariant DONGLES[] = {
    {"RED", "Cubase 3 red dongle (Intel 5C060)"},
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

static int findDongleIndex(const char *dongleId) {
  if (dongleId != NULL) {
    for (int i = 0; i < DONGLE_COUNT; i++) {
      if (strcmp(dongleId, DONGLES[i].id) == 0) {
        return i;
      }
    }
  }
  return 0;
}

// Read the persisted dongle id, defaulting to the first entry.
static void loadSelectedDongle(void) {
  SettingsConfigEntry *entry =
      settings_find_entry(aconfig_getContext(), ACONFIG_PARAM_DONGLE);
  selectedDongle = findDongleIndex((entry != NULL) ? entry->value : NULL);
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
    drawSetupInfoLine("Countdown stopped. [E] GEM   [X] Booster   [D] change");
  } else {
    showCounter(countdown);
  }
}

static void menu(void) {
  menuScreenActive = true;
  infoLineDirty = true;  // the screen clear below wipes the status bar
  showTitle();
  term_printString("\n");
  term_printString("Emulated dongle:\n");
  term_printString("  ");
  term_printString(DONGLES[selectedDongle].label);
  term_printString("\n\n");
  term_printString("[D] Change dongle\n\n");
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
  selectedDongle = (selectedDongle + 1) % DONGLE_COUNT;
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
