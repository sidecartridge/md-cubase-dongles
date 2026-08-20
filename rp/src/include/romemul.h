/**
 * File: romemul.h
 * Author: Diego Parrilla Santamaría
 * Date: July 2023-2025, February 2026
 * Copyright: 2023-2026 - GOODDATA LABS SL
 * Description: Header file for the ROM emulator C program.
 */

#ifndef ROMEMUL_H
#define ROMEMUL_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "constants.h"
#include "debug.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/vreg.h"
#include "memfunc.h"
#include "pico/stdlib.h"
#include "romemul.pio.h"

#define ROMEMUL_BUS_BITS 16

// Function Prototypes
int init_romemul(bool copyFlashToRAM);

// Tear down the ROM4 read engine (SM, both DMA channels, PIO program) so pio0
// can be fully reclaimed by the Cubase PIO+DMA dongle engine. ROM4 becomes
// open-bus afterwards. Idempotent.
void romemul_deinit(void);

#endif  // ROMEMUL_H
