/**
 * File: cubaseemul.h
 * Author: Diego Parrilla Santamaría
 * Date: August 2026
 * Copyright: 2026 - GOODDATA LABS SL
 * Description: Cubase 3 red-dongle ROM3 state-machine engine.
 */

#ifndef CUBASEEMUL_H
#define CUBASEEMUL_H

#include "pico/stdlib.h"

/**
 * Commit to dongle mode. Called from the [F]irmware menu command: tears down
 * the ROM3 command channel (commemul) to free pio0, stands up the dongle ROM3
 * engine from its reset state, and launches the Core1 LUT state machine. romemul
 * (ROM4) is left running so the m68k can keep executing the cartridge / boot.
 *
 * After this the ST side is served entirely by PIO + Core1: every ROM3 access
 * samples A8, the driven word follows cubase_lut[state][a8], and the state
 * advances per access. Panics on a fatal PIO/DMA claim or program-load failure.
 */
void cubaseemul_start(void);

#endif  // CUBASEEMUL_H
