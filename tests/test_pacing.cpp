/*
 * Tests for kb::pacing_sleep_us.
 *
 * These lock in the watchdog guard: an iteration that overruns the polling
 * interval, or that straddles the 32-bit timer wrap, must never be told to
 * sleep for a near-71.6-minute span (which would trip the 1 s watchdog).
 */
#include "test_harness.h"

#include <cstdint>

#include "kb/pacing.h"

namespace {

void pacing_normal() {
  TEST_EQ(kb::pacing_sleep_us(1000, 2000, 5000), 4000u);   // 1 ms in, sleep 4 ms
  TEST_EQ(kb::pacing_sleep_us(1000, 4000, 5000), 2000u);   // 3 ms in, sleep 2 ms
  TEST_EQ(kb::pacing_sleep_us(1000, 5999, 5000), 1u);      // just under
}

void pacing_exactly_interval() {
  TEST_EQ(kb::pacing_sleep_us(1000, 6000, 5000), 0u);      // exactly 5 ms: no sleep
}

void pacing_overrun_returns_zero() {
  TEST_EQ(kb::pacing_sleep_us(1000, 6100, 5000), 0u);      // 5.1 ms: overran
  TEST_EQ(kb::pacing_sleep_us(1000, 7100, 5000), 0u);      // 6.1 ms: overran
  TEST_EQ(kb::pacing_sleep_us(1000, 1000000000, 5000), 0u);  // huge stall
}

void pacing_wrap_boundary() {
  // An iteration spanning the time_us_32() wrap (every ~71.6 minutes) still
  // measures the elapsed time correctly and sleeps only the remainder.
  constexpr uint32_t start = 0xFFFFFF00u;  // 256 us before the wrap
  // now = start + 768 scan us, mod 2^32 (wrapped over).
  constexpr uint32_t now = 0x00000200u;    // 0x200
  // (now - start) mod 2^32 == 768
  TEST_EQ(kb::pacing_sleep_us(start, now, 5000), 5000u - 768u);
}

void pacing_wrap_huge_iteration_returns_zero() {
  // Simulate an iteration that lasts nearly the whole 32-bit period (~71.6
  // minutes). Modular math reads it as an elapsed just under 2^32, which is
  // >= the interval, so the function must return 0 (no sleep), not a
  // near-2^32 uS block. Without the clamp this path could hang the loop and
  // trip the watchdog.
  constexpr uint32_t start = 1000;
  constexpr uint32_t now = 500;  // now == start - 500 (mod 2^32), i.e. huge elapsed
  TEST_EQ(kb::pacing_sleep_us(start, now, 5000), 0u);
}

}  // namespace

void test_pacing() {
  pacing_normal();
  pacing_exactly_interval();
  pacing_overrun_returns_zero();
  pacing_wrap_boundary();
  pacing_wrap_huge_iteration_returns_zero();
}
