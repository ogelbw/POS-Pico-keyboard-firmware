/*
 * Host-side unit tests for the keyboard firmware logic.
 *
 * Build with plain host g++ (no Pico SDK) and run the resulting binary. See
 * tests/CMakeLists.txt.
 */
#include <cstdio>

#include "test_harness.h"

void test_keymap();
void test_pacing();
void test_report_builder();

int main() {
  std::printf("Pico keyboard firmware unit tests\n\n");
  test_keymap();
  test_pacing();
  test_report_builder();
  test::finish("all suites");
  return test::stats().failed == 0 ? 0 : 1;
}
