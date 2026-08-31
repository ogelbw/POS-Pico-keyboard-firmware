/*
 * Tests for report building in kb/report_builder.
 */
#include "test_harness.h"

#include "kb/hid_codes.h"
#include "kb/report_builder.h"

namespace {

void add_normal_key() {
  kb::ScanReport r;
  kb::add_key(r, kb::HID_KEY_A);
  TEST_CHECK(r.anyKeyHeld);
  TEST_EQ(r.keyCount, 1u);
  TEST_EQ(r.heldKeys[0], kb::HID_KEY_A);
}

void fn_key_not_added() {
  kb::ScanReport r;
  kb::add_key(r, kb::FN_KEY);
  TEST_CHECK(r.fnKeyHeld);
  TEST_CHECK(!r.anyKeyHeld);  // Fn is not itself a held key
  TEST_EQ(r.keyCount, 0u);
}

void modifier_folds_into_bitmask() {
  kb::ScanReport r;
  kb::add_key(r, kb::HID_KEY_SHIFT_LEFT);   // bit 1
  kb::add_key(r, kb::HID_KEY_ALT_RIGHT);    // bit 6
  kb::add_key(r, kb::HID_KEY_A);
  TEST_EQ(r.modifiers, (1u << 1) | (1u << 6));
  TEST_EQ(r.keyCount, 1u);  // modifiers are not stored as held keys
  TEST_EQ(r.heldKeys[0], kb::HID_KEY_A);
}

void all_modifier_bits() {
  kb::ScanReport r;
  kb::add_key(r, kb::HID_KEY_CONTROL_LEFT);
  kb::add_key(r, kb::HID_KEY_SHIFT_LEFT);
  kb::add_key(r, kb::HID_KEY_ALT_LEFT);
  kb::add_key(r, kb::HID_KEY_GUI_LEFT);
  kb::add_key(r, kb::HID_KEY_SHIFT_RIGHT);
  kb::add_key(r, kb::HID_KEY_ALT_RIGHT);
  TEST_EQ(r.modifiers, 0x6Fu);
}

void six_key_rollover() {
  kb::ScanReport r;
  const uint8_t keys[] = {kb::HID_KEY_A, kb::HID_KEY_B, kb::HID_KEY_C,
                          kb::HID_KEY_D, kb::HID_KEY_E, kb::HID_KEY_F,
                          kb::HID_KEY_G};  // 7 keys
  for (uint8_t k : keys) kb::add_key(r, k);
  TEST_EQ(r.keyCount, 6u);
  TEST_EQ(r.heldKeys[5], kb::HID_KEY_F);   // the 7th (G) is dropped
}

void fn_normal_key_transforms() {
  kb::ScanReport r;
  kb::add_key(r, kb::FN_KEY);
  kb::add_key(r, kb::HID_KEY_1);           // maps to F1
  TEST_CHECK(!kb::resolve_fn_layer(r));    // return value means "send consumer"
  TEST_EQ(r.heldKeys[0], kb::HID_KEY_F1);
  TEST_CHECK(!r.consumerActive);
  TEST_CHECK(!r.rebootRequested);
}

void fn_consumer_reports() {
  kb::ScanReport r;
  kb::add_key(r, kb::FN_KEY);
  kb::add_key(r, kb::HID_KEY_BRACKET_RIGHT);   // maps to SCAN_NEXT consumer
  TEST_CHECK(kb::resolve_fn_layer(r));
  TEST_CHECK(r.consumerActive);
  TEST_EQ(r.consumerUsage, kb::HID_CONSUMER_SCAN_NEXT);
}

void fn_consumer_stops_further_mapping() {
  kb::ScanReport r;
  kb::add_key(r, kb::FN_KEY);
  kb::add_key(r, kb::HID_KEY_BRACKET_LEFT);  // index 0 -> consumer
  kb::add_key(r, kb::HID_KEY_1);             // index 1 -> F1, must NOT be applied
  TEST_CHECK(kb::resolve_fn_layer(r));
  TEST_CHECK(r.consumerActive);
  TEST_EQ(r.consumerUsage, kb::HID_CONSUMER_SCAN_PREVIOUS);
  TEST_EQ(r.heldKeys[1], kb::HID_KEY_1);  // untouched because the loop broke
}

void fn_escape_requests_reboot() {
  kb::ScanReport r;
  kb::add_key(r, kb::FN_KEY);
  kb::add_key(r, kb::HID_KEY_ESCAPE);
  TEST_CHECK(!kb::resolve_fn_layer(r));    // no consumer involved
  TEST_CHECK(r.rebootRequested);
  TEST_CHECK(!r.consumerActive);
}

void no_fn_means_no_transform() {
  kb::ScanReport r;
  kb::add_key(r, kb::HID_KEY_1);  // no Fn held
  TEST_CHECK(!kb::resolve_fn_layer(r));
  TEST_EQ(r.heldKeys[0], kb::HID_KEY_1);  // unchanged
  TEST_CHECK(!r.consumerActive);
  TEST_CHECK(!r.rebootRequested);
}

}  // namespace

void test_report_builder() {
  add_normal_key();
  fn_key_not_added();
  modifier_folds_into_bitmask();
  all_modifier_bits();
  six_key_rollover();
  fn_normal_key_transforms();
  fn_consumer_reports();
  fn_consumer_stops_further_mapping();
  fn_escape_requests_reboot();
  no_fn_means_no_transform();
}
