/*
 * Main-loop pacing. See pacing.h.
 */
#include "kb/pacing.h"

namespace kb {

uint32_t pacing_sleep_us(uint32_t start_us, uint32_t now_us, uint32_t interval_us) {
  // Modular subtraction: correct elapsed time even when time_us_32() wraps
  // between start and now, as long as the iteration itself is < 2^32 us.
  uint32_t elapsed = now_us - start_us;
  if (elapsed >= interval_us) return 0;
  return interval_us - elapsed;
}

}  // namespace kb
