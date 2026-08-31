# Architecture

This file explains how the firmware fits together, from boot to each USB key report. The code that does real work lives in `main.cpp`; the pure logic that can be tested without hardware lives in `kb/`.

## Modules

- `kb/keymap.{h,cpp}`, `kb/hid_codes.h`: matrix data and lookups. No hardware, no USB headers. This is what makes keymap, Fn, and modifier behavior testable on the host.
- `kb/report_builder.{h,cpp}`: pure report building. `add_key()` folds one pressed key into a report; `resolve_fn_layer()` applies Fn mappings and reports whether a consumer (media) report should go out.
- `kb/pacing.{h,cpp}`: the main-loop pacing decision. Pure and stateless.
- `main.cpp`: hardware glue. GPIO sweeps, TinyUSB calls, the watchdog, and the USB callbacks.
- `usb_descriptors.c`: USB descriptors. Two HID reports, keyboard (report ID 1) and consumer (report ID 2).

## Boot flow

`main()` starts by enabling the watchdog:

```cpp
watchdog_enable(WATCHDOG_TIMEOUT, false);   // 1000 ms, no pause on debug
```

It arms it before anything else so early hangs also reset the board. Then:

1. `board_init()` and `tud_init()` set up TinyUSB.
2. Column pins are driven high (outputs); row pins are pulled up (inputs).
3. The caps-lock LED (GPIO 3) and onboard LED (GPIO 25) are output.
4. The bad-key timestamps map is initialized to zero.

After setup, the main loop never exits.

## The main loop

```cpp
while (1) {
  uint32_t start = time_us_32();
  tud_task();
  key_scan();
  watchdog_update();                                  // kick the dog
  uint32_t pause = kb::pacing_sleep_us(start, time_us_32(), POLLING_INTERVAL_MS * 1000);
  if (pause) sleep_us(pause);
}
```

`tud_task()` services the USB stack. `key_scan()` reads the matrix and sends reports. `watchdog_update()` is the only kick and must run roughly every millisecond of work; see below for why that constrains the code.

`pacing_sleep_us` measures how long the loop took and returns the leftover time to sleep so the scan cadence stays near 5 ms. It clamps the result to zero on overrun, which is the fix for one of the watchdog escapes.

## key_scan

`key_scan()` has two branches.

When the USB bus is suspended it polls a single matrix point to detect a press to wake the host via `tud_remote_wakeup()`, and resets the bad-key timestamps in case the timer wrapped.

When active it does the real work. It returns early if the HID endpoint is not ready.

1. **Pre-select bad keys.** Every bad key whose last press is inside the 100 ms debounce window is added to the report before scanning, so a solder-glitch second press does not repeat as a new keypress. The wrap guard resets timestamps to zero right after the 32-bit timer rolls over.
2. **Sweep the matrix.** For each column, drive it low, then read each row. A low row means that column/row key is pressed. `kb::lookup_key` resolves the key, `kb::add_key` folds it in. `tud_task()` runs between columns to keep USB serviced during the sweep.
3. **Build and send.** If anything is held, `kb::resolve_fn_layer` applies the Fn layer, `watchdog_reboot` fires on Fn+Esc, a consumer report is sent when a media key is active, and the keyboard report always goes out.
4. **Release.** If a previous scan had keys and this one has none, it sends empty keyboard and consumer reports so the host releases everything.

The `hasKeyboardKey` flag is static, so a fresh press only sends a report, and only a transition to "nothing held" sends the empty clears.

## Report building

`add_key` decides what a key becomes:

- Fn sets the layer flag and is not sent to the host.
- Modifier usages (0xE0 to 0xE7) fold into the modifier bitmask.
- Everything else fills the 6-key rollover array; keys past the sixth are dropped.

`resolve_fn_layer` walks the held keys. A key with an Fn mapping is replaced by its target; a mapping that points at a consumer usage (previous/next track, play/pause) marks a consumer report and stops further Fn mapping for that scan, matching the original behavior. Fn+Esc sets a reboot flag that `main.cpp` turns into `watchdog_reboot`.

## The watchdog and how the resets happened

The watchdog is kicked once per main loop. At 5 ms cadence that is well inside the 1000 ms timeout. The failure was never a slow normal loop; it was blocks inside `key_scan` that never reached the kick.

The release path used to spin like this:

```cpp
while (!tud_hid_ready()) tud_task();   // no timeout, no watchdog kick
```

If the host stopped polling (a hub glitch or a laptop sleeping), this hung forever and the watchdog reset the board about a minute later. The replacement, `flush_empty_consumer_report`, waits at most 50 ms and calls `watchdog_update()` inside the loop, so a stuck host stalls a key release, never the whole board.

The original pacing had the same shape of bug one level down:

```cpp
uint32_t duration = time_us_32() - start;
if (duration < (POLLING_INTERVAL_MS * 1000))
  sleep_us((POLLING_INTERVAL_MS * 1000) - duration);
```

`duration` is a 32-bit unsigned value. On the rare scan that overran 5 ms, `5000 - duration` underflowed to roughly 2^32 microseconds, about 71.6 minutes, and `sleep_us` waited for almost all of it. `pacing_sleep_us` returns zero instead, so the loop never sleeps more than 5 ms and the watchdog has nothing to trip on.

`time_us_32()` itself wraps every 71.6 minutes. All the elapsed-time math uses modular 32-bit subtraction, which stays correct across that wrap as long as a single iteration never lasts a full 2^32 microseconds.

## USB callbacks

- `tud_hid_set_report_cb`: handles the LED output report. When caps-lock is set it drives both the caps-lock LED and the onboard LED; otherwise it clears them.
- `tud_hid_get_report_cb`: returns 0, stalling the request, which is fine for a keyboard that never needs GET_REPORT.
