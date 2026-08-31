# POS Pico Keyboard Firmware

Firmware for a homemade POS keyboard built around a Raspberry Pi Pico 2 W. It came from the TinyUSB HID example and grew into the daily driver this repo is named after.

The board scans a 15 by 5 key matrix, turns presses into USB HID reports, and talks to the host as a keyboard with a small Fn layer for media and function keys.

## Hardware

- Board: Raspberry Pi Pico 2 W (`PICO_BOARD=pico2_w`), Pico SDK 2.2.0.
- Matrix: 15 column pins, 5 row pins. The exact wiring lives in `kb/keymap.cpp` and in `docs/KEYMAP.md`.
- Onboard LED on GPIO 25 doubles as the caps-lock indicator, along with the caps-lock LED on GPIO 3.

## Build

The project configures with the standard Pico SDK flow. With the SDK installed:

```sh
export PICO_SDK_PATH=$HOME/.pico-sdk/sdk/2.2.0
cmake -S . -B build -G Ninja
cmake --build build
```

That produces `build/Pico_keyboard_firmware.uf2`. Drag it onto the Pico 2 W in BOOTSEL mode to flash.

## Test

The scanning and report logic is split into hardware-free modules (`kb/`), so it builds and runs on the development machine without the Pico SDK:

```sh
cmake -S tests -B build/tests
cmake --build build/tests
./build/tests/pico_keyboard_tests
```

The suite locks in the main-loop pacing behavior, the watchdog guard, the keymap, the Fn layer, and the 6-key rollover. It is the regression net for the resets described below.

## The layout at a glance

- Base layer: the `keyMap` table maps each column/row to a HID usage.
- Fn layer: hold the Fn key (column 11, row 4) and the number row becomes F1 to F10, minus/equal become F11/F12, application becomes Delete, and a few keys send media controls.
- Fn+Esc reboots the board manually.

## Bad keys

A handful of keys (`B`, `U`, `N`, `Up`) double-press due to bad soldering. They get a 100 ms debounce: once one is seen pressed, it stays in the report for that window so the second press does not register as a repeat.

## The watchdog and the resets

The firmware arms a 1000 ms hardware watchdog and kicks it every main loop. Any path that blocks for over a second without reaching the kick causes a reset. There used to be two of those paths, which is why the board reset roughly monthly:

1. A key-release flush looped on `tud_hid_ready()` with no timeout and no watchdog kick, so an unresponsive host could hang it forever.
2. The polling pacing subtracted in unsigned arithmetic without clamping, so a scan that overran 5 ms could ask `sleep_us` for a near-71.6-minute span.

Both are fixed: the flush is bounded to 50 ms and kicks the watchdog, and the pacing clamps to zero when the loop overruns. See `docs/ARCHITECTURE.md` for the details.
