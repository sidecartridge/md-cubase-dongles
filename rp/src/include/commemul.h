/**
 * File: commemul.h
 * Author: Diego Parrilla Santamaría
 * Date: March 2026
 * Copyright: 2026 - GOODDATA LABS SL
 * Description: ROM3 communication emulator backed by a DMA ring buffer.
 */

#ifndef COMMEMUL_H
#define COMMEMUL_H

#include <inttypes.h>
#include <stdbool.h>

#include "pico/stdlib.h"

// Fixed word driven on the ROM3 data bus (the Cubase dongle response). D8 = 0
// (0xFEFF), every other data line high — mirrors MiSTer's {7'h7f, d8}. EPIC-05
// will replace this fixed value with the state-machine output.
#define CUBASE_ROM3_DRIVE_WORD 0xFEFFu

typedef void (*CommEmulSampleCallback)(uint16_t sample);

// Returns 0 on success, < 0 on failure (PIO program load failed). The
// PIO state-machine and DMA-channel claims call the SDK's "panic on
// exhaustion" variants, so those paths abort the whole boot rather
// than returning here.
int commemul_init(void);
void __not_in_flash_func(commemul_poll)(CommEmulSampleCallback callback);

#endif  // COMMEMUL_H
