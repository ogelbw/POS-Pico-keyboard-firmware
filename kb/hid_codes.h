/*
 * HID usage codes used by this firmware.
 *
 * These are stable USB HID definitions. The values are mirrored from TinyUSB's
 * class/hid/hid.h (SDK 2.2.0) so that the host-side unit tests can build
 * without dragging in the USB stack. Keep them in sync if you bump the SDK.
 *
 * Keyboard usages (Usage Page 0x07) are one byte. Consumer usages
 * (Usage Page 0x0C) are two bytes with the top byte zero.
 */
#ifndef KB_HID_CODES_H_
#define KB_HID_CODES_H_

#include <cstdint>

namespace kb {

// Keyboard report "no key" placeholder.
constexpr uint8_t HID_KEY_NONE = 0x00;

// Letters.
constexpr uint8_t HID_KEY_A = 0x04;
constexpr uint8_t HID_KEY_B = 0x05;
constexpr uint8_t HID_KEY_C = 0x06;
constexpr uint8_t HID_KEY_D = 0x07;
constexpr uint8_t HID_KEY_E = 0x08;
constexpr uint8_t HID_KEY_F = 0x09;
constexpr uint8_t HID_KEY_G = 0x0A;
constexpr uint8_t HID_KEY_H = 0x0B;
constexpr uint8_t HID_KEY_I = 0x0C;
constexpr uint8_t HID_KEY_J = 0x0D;
constexpr uint8_t HID_KEY_K = 0x0E;
constexpr uint8_t HID_KEY_L = 0x0F;
constexpr uint8_t HID_KEY_M = 0x10;
constexpr uint8_t HID_KEY_N = 0x11;
constexpr uint8_t HID_KEY_O = 0x12;
constexpr uint8_t HID_KEY_P = 0x13;
constexpr uint8_t HID_KEY_Q = 0x14;
constexpr uint8_t HID_KEY_R = 0x15;
constexpr uint8_t HID_KEY_S = 0x16;
constexpr uint8_t HID_KEY_T = 0x17;
constexpr uint8_t HID_KEY_U = 0x18;
constexpr uint8_t HID_KEY_V = 0x19;
constexpr uint8_t HID_KEY_W = 0x1A;
constexpr uint8_t HID_KEY_X = 0x1B;
constexpr uint8_t HID_KEY_Y = 0x1C;
constexpr uint8_t HID_KEY_Z = 0x1D;

// Digits.
constexpr uint8_t HID_KEY_1 = 0x1E;
constexpr uint8_t HID_KEY_2 = 0x1F;
constexpr uint8_t HID_KEY_3 = 0x20;
constexpr uint8_t HID_KEY_4 = 0x21;
constexpr uint8_t HID_KEY_5 = 0x22;
constexpr uint8_t HID_KEY_6 = 0x23;
constexpr uint8_t HID_KEY_7 = 0x24;
constexpr uint8_t HID_KEY_8 = 0x25;
constexpr uint8_t HID_KEY_9 = 0x26;
constexpr uint8_t HID_KEY_0 = 0x27;

// Punctuation and navigation.
constexpr uint8_t HID_KEY_ENTER = 0x28;
constexpr uint8_t HID_KEY_ESCAPE = 0x29;
constexpr uint8_t HID_KEY_BACKSPACE = 0x2A;
constexpr uint8_t HID_KEY_TAB = 0x2B;
constexpr uint8_t HID_KEY_SPACE = 0x2C;
constexpr uint8_t HID_KEY_MINUS = 0x2D;
constexpr uint8_t HID_KEY_EQUAL = 0x2E;
constexpr uint8_t HID_KEY_BRACKET_LEFT = 0x2F;
constexpr uint8_t HID_KEY_BRACKET_RIGHT = 0x30;
constexpr uint8_t HID_KEY_BACKSLASH = 0x31;
constexpr uint8_t HID_KEY_SEMICOLON = 0x33;
constexpr uint8_t HID_KEY_APOSTROPHE = 0x34;
constexpr uint8_t HID_KEY_GRAVE = 0x35;
constexpr uint8_t HID_KEY_COMMA = 0x36;
constexpr uint8_t HID_KEY_PERIOD = 0x37;
constexpr uint8_t HID_KEY_SLASH = 0x38;
constexpr uint8_t HID_KEY_CAPS_LOCK = 0x39;

// Function keys.
constexpr uint8_t HID_KEY_F1 = 0x3A;
constexpr uint8_t HID_KEY_F2 = 0x3B;
constexpr uint8_t HID_KEY_F3 = 0x3C;
constexpr uint8_t HID_KEY_F4 = 0x3D;
constexpr uint8_t HID_KEY_F5 = 0x3E;
constexpr uint8_t HID_KEY_F6 = 0x3F;
constexpr uint8_t HID_KEY_F7 = 0x40;
constexpr uint8_t HID_KEY_F8 = 0x41;
constexpr uint8_t HID_KEY_F9 = 0x42;
constexpr uint8_t HID_KEY_F10 = 0x43;
constexpr uint8_t HID_KEY_F11 = 0x44;
constexpr uint8_t HID_KEY_F12 = 0x45;

constexpr uint8_t HID_KEY_PRINT_SCREEN = 0x46;
constexpr uint8_t HID_KEY_DELETE = 0x4C;

// Arrows.
constexpr uint8_t HID_KEY_ARROW_RIGHT = 0x4F;
constexpr uint8_t HID_KEY_ARROW_LEFT = 0x50;
constexpr uint8_t HID_KEY_ARROW_DOWN = 0x51;
constexpr uint8_t HID_KEY_ARROW_UP = 0x52;

constexpr uint8_t HID_KEY_APPLICATION = 0x65;

// Modifiers occupy 0xE0-0xE7.
constexpr uint8_t HID_KEY_CONTROL_LEFT = 0xE0;
constexpr uint8_t HID_KEY_SHIFT_LEFT = 0xE1;
constexpr uint8_t HID_KEY_ALT_LEFT = 0xE2;
constexpr uint8_t HID_KEY_GUI_LEFT = 0xE3;
constexpr uint8_t HID_KEY_SHIFT_RIGHT = 0xE5;
constexpr uint8_t HID_KEY_ALT_RIGHT = 0xE6;

// Consumer usages.
constexpr uint16_t HID_CONSUMER_SCAN_PREVIOUS = 0x00B6;
constexpr uint16_t HID_CONSUMER_SCAN_NEXT = 0x00B5;
constexpr uint16_t HID_CONSUMER_PLAY_PAUSE = 0x00CD;

}  // namespace kb

#endif  // KB_HID_CODES_H_
