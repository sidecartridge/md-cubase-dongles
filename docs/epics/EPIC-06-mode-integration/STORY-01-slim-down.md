---
id: STORY-01
epic: EPIC-06
title: Slim-down — drop network, microSD, USB and LED
status: done
---

## Goal

Strip the app to the minimum needed for a dongle emulator: the cartridge-bus
engines, the terminal display, and settings. No Wi-Fi, no microSD, no USB, no
status LED.

## Tasks

- [x] Remove network sources + libs: `network.c`, `download.c`, `httpc/`, the
      `pico_cyw43_arch_lwip_poll` link, and `lwipopts` usage
- [x] Remove microSD sources + libs: `sdcard.c`, `hw_config.c`, the FatFs
      subdir (`add_subdirectory(FATFS_SDK)`), the `no-OS-FatFS-*` link, and the
      `rp/src/ff/ffconf.h` override
- [x] Remove the LED: drop `blink.c`/`blink.h` and all `blink_*` calls
- [x] Strip the network/SD/live-info code from `term.c` (`term_printNetworkInfo`,
      `term_refreshMenuLiveInfo`, IP/DNS helpers) and their `term.h` decls
- [x] Link the hardware SDK libs the code actually uses (`hardware_pio`,
      `hardware_dma`, `hardware_clocks`, `hardware_vreg`, `hardware_watchdog`,
      `hardware_resets`, `hardware_sync`) that were previously pulled in
      transitively by the removed libraries

## Acceptance

`build.sh pico_w release <uuid>` links cleanly; the UF2 contains no Wi-Fi/SD/USB
code paths.

## Notes

Removing the network/FatFs/cyw43 links also removed the transitive include paths
for `hardware/pio.h` and `hardware/dma.h`, so those hardware libraries must be
linked explicitly now.
