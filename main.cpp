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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>

#include "pico/stdlib.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

using std::vector;

/** --------------------------------------------------------------------+ */
/** MACRO CONSTANT */
/** --------------------------------------------------------------------+ */

#ifndef POS_MACRO
#define POS_MACRO
#define LOW 0
#define HIGH 1
#define FN_KEY 0xff
#define CAPSLOCK_LED 3
#define POLLING_INTERVAL_MS 5
#define MODIFIER_KEY_LOWER 0xE0
#define MODIFIER_KEY_UPPER 0xE7
#define GPIO_PIN_SETTLE_DELAY_US 10

// May seem large but there is delay due to bad soldering and lingering presses
#define BAD_KEY_DEBOUNCE_DELAY_US 1000*100
#endif//POS_MACRO

void key_scan(void);

/** The pins connected to each column of the key matrix. Left to right when
 * looking at the keyboard face. */
const vector<uint> colPins{13, 12, 11, 10, 9, 8, 7, 21, 6, 5, 4, 3, 2, 1, 0};

/** The pins connected to each row of the key matrix. From top to bottom when
 * looking at the keyboard face. */
const vector<uint> rowPins{20, 19, 18, 17, 16};

/** keymap[col][row] */
/** Note, The HID_KEY_NONE are padding for keys that dont actually exist. */
const vector<vector<uint8_t>> keyMap{
    {HID_KEY_ESCAPE, HID_KEY_TAB, HID_KEY_CAPS_LOCK, HID_KEY_SHIFT_LEFT, HID_KEY_CONTROL_LEFT},
    {HID_KEY_1, HID_KEY_Q, HID_KEY_A, HID_KEY_NONE, HID_KEY_GUI_LEFT},
    {HID_KEY_2, HID_KEY_W, HID_KEY_S, HID_KEY_Z, HID_KEY_ALT_LEFT},
    {HID_KEY_3, HID_KEY_E, HID_KEY_D, HID_KEY_X},
    {HID_KEY_4, HID_KEY_R, HID_KEY_F, HID_KEY_C},
    {HID_KEY_5, HID_KEY_T, HID_KEY_G, HID_KEY_V},
    {HID_KEY_6, HID_KEY_Y, HID_KEY_H, HID_KEY_B, HID_KEY_SPACE},
    {HID_KEY_7, HID_KEY_U, HID_KEY_J, HID_KEY_N},
    {HID_KEY_8, HID_KEY_I, HID_KEY_K, HID_KEY_M},
    {HID_KEY_9, HID_KEY_O, HID_KEY_L, HID_KEY_COMMA},
    {HID_KEY_0, HID_KEY_P, HID_KEY_SEMICOLON, HID_KEY_PERIOD, HID_KEY_ALT_RIGHT},
    {HID_KEY_MINUS, HID_KEY_BRACKET_LEFT, HID_KEY_APOSTROPHE, HID_KEY_SHIFT_RIGHT, FN_KEY},
    {HID_KEY_EQUAL, HID_KEY_BRACKET_RIGHT, HID_KEY_GRAVE, HID_KEY_NONE, HID_KEY_ARROW_LEFT},
    {HID_KEY_PRINT_SCREEN, HID_KEY_SLASH, HID_KEY_ENTER, HID_KEY_ARROW_UP, HID_KEY_ARROW_DOWN},
    {HID_KEY_BACKSPACE, HID_KEY_BACKSLASH, HID_KEY_NONE, HID_KEY_APPLICATION, HID_KEY_ARROW_RIGHT}};

const std::map<uint8_t, uint8_t> fn_transforms{
    {HID_KEY_1, HID_KEY_F1},
    {HID_KEY_2, HID_KEY_F2},
    {HID_KEY_3, HID_KEY_F3},
    {HID_KEY_4, HID_KEY_F4},
    {HID_KEY_5, HID_KEY_F5},
    {HID_KEY_6, HID_KEY_F6},
    {HID_KEY_7, HID_KEY_F7},
    {HID_KEY_8, HID_KEY_F8},
    {HID_KEY_9, HID_KEY_F9},
    {HID_KEY_0, HID_KEY_F10},
    {HID_KEY_MINUS, HID_KEY_F11},
    {HID_KEY_EQUAL, HID_KEY_F12},
    // {HID_KEY_W, HID_KEY_ARROW_UP},
    // {HID_KEY_S, HID_KEY_ARROW_DOWN},
    // {HID_KEY_A, HID_KEY_ARROW_LEFT},
    // {HID_KEY_D, HID_KEY_ARROW_RIGHT},
    {HID_KEY_APPLICATION, HID_KEY_DELETE},
    {HID_KEY_BRACKET_LEFT, HID_USAGE_CONSUMER_SCAN_PREVIOUS},
    {HID_KEY_BRACKET_RIGHT, HID_USAGE_CONSUMER_SCAN_NEXT},
    {HID_KEY_P, HID_USAGE_CONSUMER_PLAY_PAUSE},
};

/** These are keys that sometimes double press due to bad soldering */
const vector<uint8_t> bad_keys{HID_KEY_B};
std::map<uint8_t, uint32_t> last_bad_key_press;

/*------------- MAIN -------------*/
int main(void)
{
  /** assuming this is related to stm32 stuff. idk tbh */
  board_init();

  /** init the gpio pins and setting them up for input and output. */
  for (auto pin : colPins)
  {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, HIGH);
  }

  for (auto pin : rowPins)
  {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
  }

  gpio_init(CAPSLOCK_LED);
  gpio_set_dir(CAPSLOCK_LED, GPIO_OUT);

  /** init device stack on configured roothub port */
  tud_init(BOARD_TUD_RHPORT);
  if (board_init_after_tusb)
  {
    board_init_after_tusb();
  }

  /** init the last bad key press map */
  for (auto key : bad_keys)
  {
    last_bad_key_press[key] = 0;
  }

  while (1)
  {
    static uint32_t start = time_us_32();
    tud_task();
    key_scan();

    /** This is enforcing a delay between loops. If the time taken for
     *  tud_task and key_scan is already greater than the time for the polling
     *  interval then no busy waiting occurs */
    static uint32_t duration = time_us_32() - start;
    if (duration < (POLLING_INTERVAL_MS * 1000))
      sleep_us((POLLING_INTERVAL_MS * 1000) - duration);
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
void key_scan(void)
{
  /** Remote wakeup */
  if (tud_suspended())
  {
    gpio_put(colPins[0], LOW);
    sleep_us(GPIO_PIN_SETTLE_DELAY_US);
    if (gpio_get(rowPins[0]) == LOW)
    {
      tud_remote_wakeup();

      /* The timer may have wrapped over so just reset the last presses to be 
      sure. */
      for (auto key : bad_keys)
      {
        last_bad_key_press[key] = 0;
      }
    }
    gpio_put(colPins[0], HIGH);
  }
  else
  {
    if (!tud_hid_ready())
      return;

    bool fnKeyHeld = false;
    bool anyKeyHeld = false;
    uint8_t modifiers_held = 0;
    uint8_t keyIndex = 0;
    uint8_t heldKeys[6] = {HID_KEY_NONE, HID_KEY_NONE, HID_KEY_NONE,
                            HID_KEY_NONE, HID_KEY_NONE, HID_KEY_NONE};

    /** Before scanning check if any of the bad keys have been pressed in the 
     * past period and if so preemptively add them to the report. */
    for (auto key : bad_keys)
    {
      auto now = time_us_32();
      /* Handle the case when the time wraps over, set the last key presses to 0 
      and wait for the debounce delay to pass before checking again. */
      if (now < BAD_KEY_DEBOUNCE_DELAY_US) {
        last_bad_key_press[key] = 0;
        continue;
      }

      if ((now - last_bad_key_press.at(key)) < BAD_KEY_DEBOUNCE_DELAY_US)
      {
        heldKeys[keyIndex++] = key;
        anyKeyHeld = true;
      }
    }

    /** Now we can do the scanning. Set the column being scanned to LOW
     * and then check the rows to see if any are LOW. If they are, then
     * we know that the key at that row and column is pressed. */
    for (int col = 0; col < colPins.size(); col++)
    {
      /** Pulling the column being scanned low */
      gpio_put(colPins[col], LOW);
      for (int row = 0; row < rowPins.size(); row++)
      {
        sleep_us(GPIO_PIN_SETTLE_DELAY_US);
        if (gpio_get(rowPins[row]) == LOW)
        {
          uint8_t key = keyMap.at(col).at(row);
          anyKeyHeld = true;

          if (last_bad_key_press.contains(key)){
            last_bad_key_press[key] = time_us_32();
          }

          if (key == FN_KEY)
          {
            /** As far as the pc is concerned the Fn key doesn't exist. */
            fnKeyHeld = true;
            continue;
          }

          if (MODIFIER_KEY_LOWER <= key && key <= MODIFIER_KEY_UPPER)
          {
            /** Modifier keys are controlled by a bit string and we can get the
             * bit position by subtracting 0xE0 */
            modifiers_held |= (1 << (key - MODIFIER_KEY_LOWER));
          }
          /** Check if we have hit the max number of key we can send in a single
           *  report and if so we can just break through the rest of the
           *  loops */
          if (keyIndex == 6){ break; }
          heldKeys[keyIndex++] = key;
        }
      }
      /** setting the column we just scanned back to high */
      gpio_put(colPins[col], HIGH);
    }

    /** used to track if we previously sent a key report */
    static bool hasKeyboardKey = false;
    if (anyKeyHeld)
    {
      /** if the fn key is held down then we should map the keys to their
       *  fn key mappings, if a fn map doesn't exist don't change it */
      if (fnKeyHeld)
      {
        for (int i = 0; i < 6; i++)
        {
          if (fn_transforms.contains(heldKeys[i]))
          {
            heldKeys[i] = fn_transforms.at(heldKeys[i]);
            if (heldKeys[i] == HID_USAGE_CONSUMER_SCAN_NEXT 
                || heldKeys[i] == HID_USAGE_CONSUMER_SCAN_PREVIOUS)
            {
              uint16_t consumerKey = heldKeys[i];
              for (int j = 0; j < 6; j++) heldKeys[j] = HID_KEY_NONE;
              // send a consumer report instead of a keyboard report
              uint8_t report[2] =
              {
                (uint8_t)(consumerKey & 0xff),
                (uint8_t)((consumerKey >> 8) & 0xff)
              };
              tud_hid_report(REPORT_ID_CONSUMER_CONTROL, report, sizeof(report));
              break;
            }
          }
        }
      }
      tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifiers_held, heldKeys);
      hasKeyboardKey = true;
    }
    else
    {
      /** send empty key report if previously has key pressed and all keys have
       * been released now */
      if (hasKeyboardKey)
      {
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        uint8_t empty_report[2] = { 0x00, 0x00 };
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, empty_report, sizeof(empty_report));
      }
      hasKeyboardKey = false;
    }
  }
}

/** Invoked when received SET_REPORT control request or
 * received data on OUT endpoint ( Report ID = 0, Type = 0 ) */
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

      /** Turn on the on board light if caplock is active. */
      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        board_led_write(true);
        gpio_put(CAPSLOCK_LED, HIGH);
      }
      else
      {
        board_led_write(false);
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