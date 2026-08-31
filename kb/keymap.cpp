/*
 * Key matrix data and lookups. See keymap.h.
 */
#include "kb/keymap.h"

#include "kb/hid_codes.h"

namespace kb {

const std::vector<uint32_t> colPins{13, 12, 11, 10, 9, 8, 7, 21, 6, 5, 4, 3, 2, 1, 0};

const std::vector<uint32_t> rowPins{20, 19, 18, 17, 16};

// keyMap[col][row]. Rows that end early simply have no key on the last row,
// and lookup_key reports those as HID_KEY_NONE.
const std::vector<std::vector<uint8_t>> keyMap{
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

const std::map<uint8_t, uint16_t> fn_transforms{
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
    {HID_KEY_APPLICATION, HID_KEY_DELETE},
    {HID_KEY_BRACKET_LEFT, HID_CONSUMER_SCAN_PREVIOUS},
    {HID_KEY_BRACKET_RIGHT, HID_CONSUMER_SCAN_NEXT},
    {HID_KEY_P, HID_CONSUMER_PLAY_PAUSE},
};

const std::vector<uint8_t> bad_keys{HID_KEY_B, HID_KEY_ARROW_UP, HID_KEY_U, HID_KEY_N};

uint8_t lookup_key(size_t col, size_t row) {
  if (col >= keyMap.size()) return HID_KEY_NONE;
  const auto& column = keyMap[col];
  if (row >= column.size()) return HID_KEY_NONE;
  return column[row];
}

bool is_fn_key(uint8_t key) { return key == FN_KEY; }

bool is_modifier_key(uint8_t key) {
  return MODIFIER_KEY_LOWER <= key && key <= MODIFIER_KEY_UPPER;
}

uint8_t modifier_bit(uint8_t key) { return key - MODIFIER_KEY_LOWER; }

bool is_bad_key(uint8_t key) {
  for (uint8_t candidate : bad_keys) {
    if (candidate == key) return true;
  }
  return false;
}

std::optional<uint16_t> fn_transform(uint8_t key) {
  auto it = fn_transforms.find(key);
  if (it == fn_transforms.end()) return std::nullopt;
  return it->second;
}

bool is_consumer_usage(uint16_t usage) {
  return usage == HID_CONSUMER_SCAN_NEXT || usage == HID_CONSUMER_SCAN_PREVIOUS ||
         usage == HID_CONSUMER_PLAY_PAUSE;
}

}  // namespace kb
