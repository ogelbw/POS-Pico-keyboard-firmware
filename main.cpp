/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board_api.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "kb/pacing.h"
#include "kb/report_builder.h"

/** --------------------------------------------------------------------+ */
/** MACRO CONSTANT */
/** --------------------------------------------------------------------+ */

#define LOW 0
#define HIGH 1
#define CAPSLOCK_LED 3
#define POLLING_INTERVAL_MS 5
#define GPIO_PIN_SETTLE_DELAY_US 10
#define BOARD_LED_GPIO 25
#define WATCHDOG_TIMEOUT 1000
#define HID_FLUSH_TIMEOUT_US (50 * 1000)

// May seem large but there is delay due to bad soldering and lingering presses
#define BAD_KEY_DEBOUNCE_DELAY_US 1000 * 100

void key_scan(void);

/** Timestamp of the last press for each bad key. Guards against the timer
 *  wrapping over by resetting the stamp to 0 right after the wrap. */
std::map<uint8_t, uint32_t> last_bad_key_press;

/*------------- MAIN -------------*/
int main(void)
{
  watchdog_enable(WATCHDOG_TIMEOUT, false);

  /** tinyusb init */
  board_init();

  /** init the gpio pins and setting them up for input and output. */
  for (auto pin : kb::colPins)
  {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, HIGH);
  }

  for (auto pin : kb::rowPins)
  {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
  }

  gpio_init(CAPSLOCK_LED);
  gpio_set_dir(CAPSLOCK_LED, GPIO_OUT);
  gpio_init(BOARD_LED_GPIO);
  gpio_set_dir(BOARD_LED_GPIO, GPIO_OUT);

  /** init device stack on configured roothub port */
  tud_init(BOARD_TUD_RHPORT);
  if (board_init_after_tusb)
  {
    board_init_after_tusb();
  }

  /** init the last bad key press map */
  for (auto key : kb::bad_keys)
  {
    last_bad_key_press[key] = 0;
  }

  while (1)
  {
    uint32_t start = time_us_32();
    tud_task();
    key_scan();

    /* Kick the dog */
    watchdog_update();

    /** Enforce the polling cadence. pacing_sleep_us() clamps the result so a
     *  scan that overran the interval (or an iteration straddling the 32-bit
     *  timer wrap) never sleeps for a near-2^32 us span and trips the
     *  watchdog. */
    uint32_t pause = kb::pacing_sleep_us(start, time_us_32(), POLLING_INTERVAL_MS * 1000);
    if (pause)
      sleep_us(pause);
  }
}

/** --------------------------------------------------------------------+ */
/** Device callbacks */
/** --------------------------------------------------------------------+ */
/** Invoked when device is mounted */
void tud_mount_cb(void) {}

/** Invoked when device is unmounted */
void tud_umount_cb(void) {}

/** Invoked when usb bus is suspended */
/** remote_wakeup_en : if host allow us to perform remote wakeup */
/** Within 7ms, device must draw an average of current less than 2.5 mA from bus */
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
}

/** Invoked when usb bus is resumed */
void tud_resume_cb(void) {}

/** --------------------------------------------------------------------+ */
/** USB HID */
/** --------------------------------------------------------------------+ */

/** Sends the consumer empty report, waiting up to HID_FLUSH_TIMEOUT_US for the
 *  endpoint to accept. Kicks the watchdog while waiting so an unresponsive
 *  host stalls a key release, never the whole board. */
static void flush_empty_consumer_report(void)
{
  uint32_t deadline = time_us_32() + HID_FLUSH_TIMEOUT_US;
  while (!tud_hid_ready())
  {
    tud_task();
    watchdog_update();
    if (time_us_32() >= deadline)
      break;
  }

  uint8_t empty_report[2] = {0x00, 0x00};
  tud_hid_report(REPORT_ID_CONSUMER_CONTROL, empty_report, sizeof(empty_report));
}

void key_scan(void)
{
  /** Remote wakeup */
  if (tud_suspended())
  {
    gpio_put(kb::colPins[0], LOW);
    sleep_us(GPIO_PIN_SETTLE_DELAY_US);
    if (gpio_get(kb::rowPins[0]) == LOW)
    {
      tud_remote_wakeup();

      /* The timer may have wrapped over so just reset the last presses to be
       * sure. */
      for (auto key : kb::bad_keys)
      {
        last_bad_key_press[key] = 0;
      }
    }
    gpio_put(kb::colPins[0], HIGH);
  }
  else
  {
    if (!tud_hid_ready())
      return;

    kb::ScanReport report;

    /** Before scanning check if any of the bad keys have been pressed in the
     *  past period and if so preemptively add them to the report. */
    for (auto key : kb::bad_keys)
    {
      auto now = time_us_32();
      /* Handle the case when the time wraps over, set the last key presses to 0
       * and wait for the debounce delay to pass before checking again. */
      if (now < BAD_KEY_DEBOUNCE_DELAY_US)
      {
        last_bad_key_press[key] = 0;
        continue;
      }

      if ((now - last_bad_key_press.at(key)) < BAD_KEY_DEBOUNCE_DELAY_US)
      {
        kb::add_key(report, key);
      }
    }

    /** Scan the matrix. Pull the column being scanned LOW and check each row;
     *  a LOW row means the key at that row and column is pressed. */
    for (size_t col = 0; col < kb::colPins.size(); col++)
    {
      gpio_put(kb::colPins[col], LOW);
      for (size_t row = 0; row < kb::rowPins.size(); row++)
      {
        sleep_us(GPIO_PIN_SETTLE_DELAY_US);
        if (gpio_get(kb::rowPins[row]) == LOW)
        {
          uint8_t key = kb::lookup_key(col, row);

          if (kb::is_bad_key(key))
          {
            last_bad_key_press[key] = time_us_32();
          }

          kb::add_key(report, key);
        }
      }
      /** Setting the column we just scanned back to high and let tud task run */
      gpio_put(kb::colPins[col], HIGH);
      tud_task();
    }

    /** Track if we previously sent a key report, so a release can clear it. */
    static bool hasKeyboardKey = false;

    if (report.anyKeyHeld)
    {
      const bool consumer = kb::resolve_fn_layer(report);

      /* Manual reboot button on Fn+Esc */
      if (report.rebootRequested)
      {
        watchdog_reboot(0, 0, 0);
      }

      if (consumer)
      {
        uint8_t report_bytes[2] = {
            (uint8_t)(report.consumerUsage & 0xFF),
            (uint8_t)((report.consumerUsage >> 8) & 0xFF)};
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, report_bytes, sizeof(report_bytes));
      }

      tud_hid_keyboard_report(REPORT_ID_KEYBOARD, report.modifiers, report.heldKeys);
      hasKeyboardKey = true;
    }
    else
    {
      /** Send empty reports if a scan previously had keys and they have all
       *  now been released. */
      if (hasKeyboardKey)
      {
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        flush_empty_consumer_report();
      }
      hasKeyboardKey = false;
    }
  }
}

/** Invoked when received SET_REPORT control request or
 *  received data on OUT endpoint ( Report ID = 0, Type = 0 ) */
void tud_hid_set_report_cb(
    uint8_t instance,
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t const *buffer,
    uint16_t bufsize)
{
  (void)instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    /** Set keyboard LED e.g Capslock, Numlock etc... */
    if (report_id == 1)
    {
      if (bufsize < 1)
        return;

      uint8_t const kbd_leds = buffer[0];

      /** Turn on the on board light if capslock is active. */
      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        gpio_put(BOARD_LED_GPIO, HIGH);
        gpio_put(CAPSLOCK_LED, HIGH);
      }
      else
      {
        gpio_put(BOARD_LED_GPIO, LOW);
        gpio_put(CAPSLOCK_LED, LOW);
      }
    }
  }
}

/** Invoked when received GET_REPORT control request */
/** Application must fill buffer report's content and return its length. */
/** Return zero will cause the stack to STALL request */
uint16_t tud_hid_get_report_cb(
    uint8_t instance,
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t *buffer,
    uint16_t reqlen)
{
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}
