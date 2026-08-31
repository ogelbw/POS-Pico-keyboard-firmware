/*
 * Key matrix data and lookups.
 *
 * Pure data plus pure helper functions. No hardware touches here, so this
 * module compiles both into firmware and into the host-side unit tests.
 */
#ifndef KB_KEYMAP_H_
#define KB_KEYMAP_H_

#include <cstdint>
#include <cstddef>
#include <map>
#include <optional>
#include <vector>

namespace kb {

// A fake key used for the Fn layer; the host never sees it.
constexpr uint32_t FN_KEY = 0xFF;

// Modifier keys live in the range 0xE0-0xE7 and fold into a bitmask.
constexpr uint8_t MODIFIER_KEY_LOWER = 0xE0;
constexpr uint8_t MODIFIER_KEY_UPPER = 0xE7;

// Max number of non-modifier keys in a single HID report.
constexpr size_t REPORT_KEY_COUNT = 6;

// Column pins, left to right when looking at the keyboard face.
extern const std::vector<uint32_t> colPins;

// Row pins, top to bottom when looking at the keyboard face.
extern const std::vector<uint32_t> rowPins;

// keyMap[col][row]. HID_KEY_NONE marks keys that do not exist.
extern const std::vector<std::vector<uint8_t>> keyMap;

// Keys that double-press and need debounce. Hashed by HID usage.
extern const std::vector<uint8_t> bad_keys;

// Maps a held key to its Fn-layer usage. Values above 0xFF are consumer
// usages and are sent on the consumer report instead of the keyboard report.
extern const std::map<uint8_t, uint16_t> fn_transforms;

// HID key at a column/row. Out-of-range or padding returns HID_KEY_NONE.
uint8_t lookup_key(size_t col, size_t row);

bool is_fn_key(uint8_t key);
bool is_modifier_key(uint8_t key);

// Bit position within the modifier byte for a modifier usage, 0-7.
// Call only with keys where is_modifier_key(key) is true.
uint8_t modifier_bit(uint8_t key);

bool is_bad_key(uint8_t key);

// The Fn-layer usage for a key, or nullopt if it has no mapping.
std::optional<uint16_t> fn_transform(uint8_t key);

// True if a usage is a consumer (media) usage sent on the consumer report.
bool is_consumer_usage(uint16_t usage);

}  // namespace kb

#endif  // KB_KEYMAP_H_
