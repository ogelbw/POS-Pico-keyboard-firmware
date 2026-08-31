/*
 * Tests for the keymap data and lookups in kb/keymap.
 */
#include "test_harness.h"

#include "kb/hid_codes.h"
#include "kb/keymap.h"

namespace {

void keymap_dimensions() {
  TEST_EQ(kb::colPins.size(), 15u);
  TEST_EQ(kb::rowPins.size(), 5u);
  TEST_EQ(kb::keyMap.size(), 15u);
  TEST_EQ(kb::fn_transforms.size(), 16u);
  TEST_EQ(kb::bad_keys.size(), 4u);
}

void keymap_known_keys() {
  TEST_EQ(kb::lookup_key(0, 0), kb::HID_KEY_ESCAPE);
  TEST_EQ(kb::lookup_key(0, 1), kb::HID_KEY_TAB);
  TEST_EQ(kb::lookup_key(0, 4), kb::HID_KEY_CONTROL_LEFT);
  TEST_EQ(kb::lookup_key(6, 0), kb::HID_KEY_6);
  TEST_EQ(kb::lookup_key(6, 3), kb::HID_KEY_B);
  TEST_EQ(kb::lookup_key(6, 4), kb::HID_KEY_SPACE);
  TEST_EQ(kb::lookup_key(11, 4), kb::FN_KEY);  // Fn lives on col 11, row 4
  TEST_EQ(kb::lookup_key(14, 0), kb::HID_KEY_BACKSPACE);
  TEST_EQ(kb::lookup_key(14, 4), kb::HID_KEY_ARROW_RIGHT);
}

void keymap_padding_is_none() {
  TEST_EQ(kb::lookup_key(1, 3), kb::HID_KEY_NONE);   // explicit padding
  TEST_EQ(kb::lookup_key(3, 4), kb::HID_KEY_NONE);   // column 3 has no row 4
  TEST_EQ(kb::lookup_key(8, 4), kb::HID_KEY_NONE);   // column 8 has no row 4
}

void keymap_out_of_range_is_none() {
  TEST_EQ(kb::lookup_key(15, 0), kb::HID_KEY_NONE);  // past the last column
  TEST_EQ(kb::lookup_key(0, 5), kb::HID_KEY_NONE);   // past the last row
  TEST_EQ(kb::lookup_key(999, 999), kb::HID_KEY_NONE);
}

void is_modifier_key_bounds() {
  TEST_CHECK(kb::is_modifier_key(kb::HID_KEY_CONTROL_LEFT));  // 0xE0
  TEST_CHECK(kb::is_modifier_key(kb::HID_KEY_ALT_RIGHT));     // 0xE6
  TEST_CHECK(!kb::is_modifier_key(kb::HID_KEY_ARROW_UP));     // 0x52
  TEST_CHECK(!kb::is_modifier_key(kb::HID_KEY_F1));           // 0x3A
  TEST_CHECK(!kb::is_modifier_key(kb::FN_KEY));
}

void modifier_bit_positions() {
  TEST_EQ(kb::modifier_bit(kb::HID_KEY_CONTROL_LEFT), 0u);
  TEST_EQ(kb::modifier_bit(kb::HID_KEY_SHIFT_LEFT), 1u);
  TEST_EQ(kb::modifier_bit(kb::HID_KEY_ALT_LEFT), 2u);
  TEST_EQ(kb::modifier_bit(kb::HID_KEY_GUI_LEFT), 3u);
  TEST_EQ(kb::modifier_bit(kb::HID_KEY_SHIFT_RIGHT), 5u);
  TEST_EQ(kb::modifier_bit(kb::HID_KEY_ALT_RIGHT), 6u);
}

void is_bad_key_membership() {
  TEST_CHECK(kb::is_bad_key(kb::HID_KEY_B));
  TEST_CHECK(kb::is_bad_key(kb::HID_KEY_U));
  TEST_CHECK(kb::is_bad_key(kb::HID_KEY_N));
  TEST_CHECK(kb::is_bad_key(kb::HID_KEY_ARROW_UP));
  TEST_CHECK(!kb::is_bad_key(kb::HID_KEY_A));
}

void fn_mappings() {
  TEST_EQ(kb::fn_transform(kb::HID_KEY_1).value(), kb::HID_KEY_F1);
  TEST_EQ(kb::fn_transform(kb::HID_KEY_0).value(), kb::HID_KEY_F10);
  TEST_EQ(kb::fn_transform(kb::HID_KEY_MINUS).value(), kb::HID_KEY_F11);
  TEST_EQ(kb::fn_transform(kb::HID_KEY_EQUAL).value(), kb::HID_KEY_F12);
  TEST_EQ(kb::fn_transform(kb::HID_KEY_APPLICATION).value(), kb::HID_KEY_DELETE);

  // Consumer mappings.
  TEST_EQ(kb::fn_transform(kb::HID_KEY_BRACKET_LEFT).value(), kb::HID_CONSUMER_SCAN_PREVIOUS);
  TEST_EQ(kb::fn_transform(kb::HID_KEY_BRACKET_RIGHT).value(), kb::HID_CONSUMER_SCAN_NEXT);
  TEST_EQ(kb::fn_transform(kb::HID_KEY_P).value(), kb::HID_CONSUMER_PLAY_PAUSE);

  // Keys with no Fn mapping.
  TEST_CHECK(!kb::fn_transform(kb::HID_KEY_ESCAPE).has_value());
  TEST_CHECK(!kb::fn_transform(kb::HID_KEY_A).has_value());
}

void consumer_usage_classification() {
  TEST_CHECK(kb::is_consumer_usage(kb::HID_CONSUMER_SCAN_PREVIOUS));
  TEST_CHECK(kb::is_consumer_usage(kb::HID_CONSUMER_SCAN_NEXT));
  TEST_CHECK(kb::is_consumer_usage(kb::HID_CONSUMER_PLAY_PAUSE));
  TEST_CHECK(!kb::is_consumer_usage(kb::HID_KEY_F1));  // 0x3A is a plain key
}

}  // namespace

void test_keymap() {
  keymap_dimensions();
  keymap_known_keys();
  keymap_padding_is_none();
  keymap_out_of_range_is_none();
  is_modifier_key_bounds();
  modifier_bit_positions();
  is_bad_key_membership();
  fn_mappings();
  consumer_usage_classification();
}
