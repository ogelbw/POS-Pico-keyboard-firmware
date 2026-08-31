/*
 * Tiny dependency-free test harness.
 *
 * A global counter tracks checks and failures. TEST_CHECK and TEST_EQ run a
 * single check; TEST_EQ_BYTES compares a byte array. Call test::finish(name)
 * from main to print the pass/fail summary.
 */
#ifndef TEST_HARNESS_H_
#define TEST_HARNESS_H_

#include <cstddef>
// #include <cstdint>
#include <stdint.h>
#include <cstdio>
#include <string>

namespace test {

struct Stats {
  long total = 0;
  long failed = 0;
};

inline Stats& stats() {
  static Stats s;
  return s;
}

inline void report_failure(const char* file, int line, const std::string& msg) {
  stats().failed++;
  std::fprintf(stderr, "  FAIL %s:%d: %s\n", file, line, msg.c_str());
}

inline void check_bytes(const uint8_t* got, const uint8_t* want, size_t n,
                        const char* label, const char* file, int line) {
  stats().total++;
  for (size_t i = 0; i < n; i++) {
    if (got[i] != want[i]) {
      char buf[256];
      std::snprintf(buf, sizeof buf,
                    "%s mismatch at [%zu] got 0x%02X want 0x%02X", label, i,
                    (unsigned)got[i], (unsigned)want[i]);
      report_failure(file, line, buf);
      return;
    }
  }
}

inline void finish(const char* suite) {
  bool ok = stats().failed == 0;
  std::printf("%s: %ld checks, %ld failed, %s\n", suite, stats().total,
              stats().failed, ok ? "PASS" : "FAIL");
  std::fflush(stdout);
}

}  // namespace test

#define TEST_CHECK(cond)                                                   \
  do {                                                                     \
    test::stats().total++;                                                 \
    if (!(cond)) test::report_failure(__FILE__, __LINE__, "CHECK(" #cond ")"); \
  } while (0)

#define TEST_EQ(a, b)                                                      \
  do {                                                                     \
    test::stats().total++;                                                 \
    auto _va = (a);                                                        \
    auto _vb = (b);                                                        \
    if (!(_va == _vb)) {                                                   \
      char _buf[256];                                                      \
      std::snprintf(_buf, sizeof _buf, "EQ(%s, %s) values differ", #a, #b); \
      test::report_failure(__FILE__, __LINE__, _buf);                      \
    }                                                                      \
  } while (0)

#define TEST_EQ_BYTES(got, want, n)                                        \
  test::check_bytes((got), (want), (n), #got, __FILE__, __LINE__)

#endif  // TEST_HARNESS_H_
