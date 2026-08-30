/**
 * File: cubase2_monitor.h
 * Author: Diego Parrilla Santamaría
 * Date: August 2026
 * Copyright: 2026 - GOODDATA LABS SL
 * Description: Cubase 2 black-dongle /UDS bus monitor + FSM tracker (EPIC-11 gate).
 */

#ifndef CUBASE2_MONITOR_H
#define CUBASE2_MONITOR_H

#include "pico/stdlib.h"

/**
 * Stand up the /UDS bus monitor. Frees the ROM3/ROM4 engines (commemul/romemul)
 * to take exclusive control of the cartridge bus, holds /READ active so the
 * address bus is readable continuously, and launches a Core1 tracker that clocks
 * the black-dongle 8-bit FSM on A0(/UDS) rising edges every bus cycle.
 *
 * Monitor-only (never drives the bus); does not touch the frozen red engine.
 * Call only after the m68k has booted GEM (so there is real bus traffic and
 * romemul is no longer needed).
 */
void cubase2_monitor_gate_start(void);

/** Current tracked FSM state (updated by Core1). */
uint8_t cubase2_monitor_state(void);

/** Number of /UDS-edge captures processed so far. */
uint32_t cubase2_monitor_captures(void);

/**
 * True if the monitor SM stalled on a full RX FIFO since the last call — i.e.
 * the tracker fell behind a bus cycle (missed edge). Write-1-to-clear.
 */
bool cubase2_monitor_consume_rxstall(void);

#endif  // CUBASE2_MONITOR_H
