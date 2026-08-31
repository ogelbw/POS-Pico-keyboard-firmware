/*
 * Main-loop pacing.
 *
 * Decides how long to idle so each scan cadence stays at interval_us, without
 * ever blocking the loop for a huge span. This is the code path that touches
 * the watchdog: a loop that returns a gigantic sleep would trip the 1 s reset.
 *
 * time_us_32() wraps every ~71.6 minutes, so all math is unsigned 32-bit
 * modular subtraction, which stays correct across the wrap as long as a single
 * iteration never takes longer than 2^32 us.
 */
#ifndef KB_PACING_H_
#define KB_PACING_H_

#include <cstdint>

namespace kb {

// How many microseconds to idle so the loop stays at interval_us cadence, or 0
// if the work already overran the interval. Clamps the result so a long or
// wrapped iteration never produces a near-2^32 us sleep.
uint32_t pacing_sleep_us(uint32_t start_us, uint32_t now_us, uint32_t interval_us);

}  // namespace kb

#endif  // KB_PACING_H_
