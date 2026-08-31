/*
 * Pure report building: turn pressed keys into keyboard/consumer reports.
 *
 * This does no hardware work, so it compiles both into firmware and into the
 * host-side unit tests. The matrix sweep that reads GPIO lives in main.cpp and
 * feeds each detected key through add_key().
 */
#ifndef KB_REPORT_BUILDER_H_
#define KB_REPORT_BUILDER_H_

#include <cstdint>

#include "kb/keymap.h"

namespace kb {

// State for one scan building into one set of reports.
struct ScanReport {
  uint8_t modifiers = 0;                                   // modifier bitmask
  uint8_t heldKeys[REPORT_KEY_COUNT] = {};                 // 6-key rollover
  uint8_t keyCount = 0;                                    // keys actually stored
  bool anyKeyHeld = false;                                 // anything to report
  bool fnKeyHeld = false;                                  // Fn pressed this scan
  bool consumerActive = false;                             // send consumer report
  uint16_t consumerUsage = 0;                              // usage when active
  bool rebootRequested = false;                            // Fn+Esc
};

// Fold one matrix key into the report. Handles Fn (marks the layer, not sent),
// modifiers (folded into the bitmask), and the 6-key rollover limit.
void add_key(ScanReport& report, uint8_t key);

// Resolve Fn mappings once all keys of a scan are added. Non-consumer Fn
// usages replace keys in place; a consumer usage marks consumerActive; Fn+Esc
// sets rebootRequested. Returns whether the consumer report should be sent.
//
// Matches the original behaviour: the first consumer usage found stops further
// Fn mapping for this scan.
bool resolve_fn_layer(ScanReport& report);

}  // namespace kb

#endif  // KB_REPORT_BUILDER_H_
