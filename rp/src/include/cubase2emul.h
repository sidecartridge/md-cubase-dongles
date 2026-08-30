/**
 * File: cubase2emul.h
 * Author: Diego Parrilla Santamaría
 * Date: August 2026
 * Copyright: 2026 - GOODDATA LABS SL
 * Description: Cubase 2 black-dongle engine (EPIC-12): /UDS monitor + ROM3 drive.
 */

#ifndef CUBASE2EMUL_H
#define CUBASE2EMUL_H

#include <stdbool.h>
#include <stdint.h>

// Commit to the Cubase 2 black dongle. Frees the setup-mode engines (commemul,
// romemul), stands up the continuous /UDS monitor (pio1 + Core1 FSM tracker) and
// the ROM3 data drive (pio0 + 2 DMAs), and returns. The engines then serve the
// dongle autonomously while Cubase runs. One-way: reset the MultiDevice to exit.
// The frozen red engine (D-11) is never involved.
void cubase2emul_start(void);

// Bring-up observability (Core0 heartbeat): the current tracked FSM state, a
// running capture count, and a consume-and-clear of the monitor's missed-edge
// (RXSTALL) flag. Valid only after cubase2emul_start().
uint8_t cubase2emul_state(void);
uint32_t cubase2emul_captures(void);
bool cubase2emul_consume_missed_edge(void);

#endif  // CUBASE2EMUL_H
