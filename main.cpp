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
//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
#define POLLING_INTERVAL_MS 5
#define GPIO_PIN_SETTLE_DELAY_US 10
#define FN_KEY 0xff
#define HIGH 1
#define LOW 0

void key_scan(void);

/** The pins connected to each column of the key matrix. Left to right when
 * looking at the keyboard face. */
const vector<uint> colPins{10, 9, 8, 7, 6, 5, 16, 17, 18, 19, 20, 21,
                           22, 27, 28};

/** The pins connected to each row of the key matrix. From top to bottom when
 * looking at the keyboard face. */
const vector<uint> rowPins{11, 12, 4, 14, 15};

// keymap[col][row]
// Note, The HID_KEY_NONE are padding. There are some places on the keyboard
// where I skipped over a row while soldering, therefore when scanning
// These ghost keys are needed otherwise we grab a random key from memory
// (Accessing beyond the size of the row vector).
const vector<vector<uint8_t>> keyMap{
    {HID_KEY_ESCAPE, HID_KEY_TAB, HID_KEY_CAPS_LOCK, HID_KEY_SHIFT_LEFT, HID_KEY_CONTROL_LEFT},
    {HID_KEY_1, HID_KEY_Q, HID_KEY_A, HID_KEY_NONE, HID_KEY_GUI_LEFT},
    {HID_KEY_2, HID_KEY_W, HID_KEY_S, HID_KEY_Z},
    {HID_KEY_3, HID_KEY_E, HID_KEY_D, HID_KEY_X, HID_KEY_ALT_LEFT},
    {HID_KEY_4, HID_KEY_R, HID_KEY_F, HID_KEY_C},
    {HID_KEY_5, HID_KEY_T, HID_KEY_G, HID_KEY_V},
    {HID_KEY_6, HID_KEY_Y, HID_KEY_H, HID_KEY_B, HID_KEY_SPACE},
    {HID_KEY_7, HID_KEY_U, HID_KEY_J, HID_KEY_N},
    {HID_KEY_8, HID_KEY_I, HID_KEY_K, HID_KEY_M},
    {HID_KEY_9, HID_KEY_O, HID_KEY_L, HID_KEY_COMMA},
    {HID_KEY_0, HID_KEY_P, HID_KEY_SEMICOLON, HID_KEY_PERIOD, FN_KEY},
    {HID_KEY_MINUS, HID_KEY_BRACKET_LEFT, HID_KEY_APOSTROPHE, HID_KEY_SHIFT_RIGHT, HID_KEY_ALT_RIGHT},
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
    {HID_KEY_W, HID_KEY_ARROW_UP},
    {HID_KEY_S, HID_KEY_ARROW_DOWN},
    {HID_KEY_A, HID_KEY_ARROW_LEFT},
    {HID_KEY_D, HID_KEY_ARROW_RIGHT},
};

/*------------- MAIN -------------*/
int main(void)
{
  // init the gpio pins
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

  // assuming this is related to stm32 stuff. idk tbh
  board_init();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);
  if (board_init_after_tusb)
  {
    board_init_after_tusb();
  }

  while (1)
  {
    tud_task(); // tinyusb device task
    key_scan();

    static uint32_t last_poll_time = 0;
    while (board_millis() - last_poll_time < POLLING_INTERVAL_MS);
    last_poll_time = board_millis();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

void key_scan(void)
{
  // Remote wakeup
  if (tud_suspended())
  {
    uint32_t const btn = board_button_read();
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    if (btn)
    {
      tud_remote_wakeup();
    }
  }
  else
  {
    if (!tud_hid_ready())
      return;

    bool fn_key_held = false;
    bool any_key_held = false;
    uint8_t modifiers_held = 0;
    uint8_t key_index = 0;
    uint8_t held_keys[6] = {HID_KEY_NONE, HID_KEY_NONE, HID_KEY_NONE,
                            HID_KEY_NONE, HID_KEY_NONE, HID_KEY_NONE};

    /* Now we can do the scanning. Set the column being scanned to LOW
     * and then check the rows to see if any are LOW. If they are, then
     * we know that the key at that row and column is pressed. */
    for (int col = 0; col < colPins.size(); col++)
    {
      gpio_put(colPins[col], LOW);
      for (int row = 0; row < rowPins.size(); row++)
      {
        sleep_us(GPIO_PIN_SETTLE_DELAY_US);
        if (gpio_get(rowPins[row]) == LOW)
        {
          uint8_t key = keyMap.at(col).at(row);
          any_key_held = true;

          if (key == FN_KEY)
          {
            fn_key_held = true;
          }

          // check if the key is a modifier key
          if (0xE0 <= key && key <= 0xE7)
          {
            modifiers_held |= (1 << (key - 0xE0));
          }
          // check if we have hit the key limit, if so break through the rest
          // of the loop.
          if (key_index == 6)
          {
            break;
          }
          held_keys[key_index++] = key;
        }
      }
      // setting the column we just scanned back to high
      gpio_put(colPins[col], HIGH);
    }

    // used to track if we previously sent a key report
    static bool has_keyboard_key = false;

    if (any_key_held)
    {
      // if the fn key is held down then check is there is a fn mapping for
      // the key and if so overwrite it.
      if (fn_key_held)
      {
        for (int i = 0; i < 6; i++)
        {
          if (fn_transforms.contains(held_keys[i]))
          {
            held_keys[i] = fn_transforms.at(held_keys[i]);
          }
        }
      }
      tud_hid_keyboard_report(1, modifiers_held, held_keys);
      has_keyboard_key = true;
    }
    else
    {
      // send empty key report if previously has key pressed
      if (has_keyboard_key)
        tud_hid_keyboard_report(1, 0, NULL);
      has_keyboard_key = false;
    }
  }
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  (void)instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_id == 1)
    {
      // bufsize should be (at least) 1
      if (bufsize < 1)
        return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        // Capslock On: disable blink, turn led on
        board_led_write(true);
      }
      else
      {
        // Caplocks Off: back to normal blink
        board_led_write(false);
      }
    }
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}