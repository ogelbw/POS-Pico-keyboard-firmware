/*
 * Pure report building. See report_builder.h.
 */
#include "kb/report_builder.h"

#include "kb/hid_codes.h"

namespace kb {

void add_key(ScanReport& report, uint8_t key) {
  report.anyKeyHeld = report.anyKeyHeld || !is_fn_key(key);

  if (is_fn_key(key)) {
    report.fnKeyHeld = true;
    return;
  }

  if (is_modifier_key(key)) {
    report.modifiers |= (uint8_t)(1u << modifier_bit(key));
    return;
  }

  // 6-key rollover: drop anything past the report limit.
  if (report.keyCount >= REPORT_KEY_COUNT) return;

  report.heldKeys[report.keyCount++] = key;
}

bool resolve_fn_layer(ScanReport& report) {
  if (!report.fnKeyHeld) return false;

  for (size_t i = 0; i < REPORT_KEY_COUNT; i++) {
    uint8_t key = report.heldKeys[i];
    auto transformed = fn_transform(key);

    if (transformed) {
      if (is_consumer_usage(*transformed)) {
        report.consumerActive = true;
        report.consumerUsage = *transformed;
        return true;  // stop Fn mapping for this scan, as the original did
      }
      report.heldKeys[i] = (uint8_t)(*transformed & 0xFF);
    } else if (key == HID_KEY_ESCAPE) {
      report.rebootRequested = true;
    }
  }
  return report.consumerActive;
}

}  // namespace kb
