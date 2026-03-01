#pragma once

#include <iostream>

namespace test {

// Inline to avoid multiple definition errors
inline int tests_passed = 0;
inline int tests_failed = 0;

inline void assert(bool condition, const char* test_name) {
  if (condition) {
    std::cout << "✓ " << test_name << std::endl;
    tests_passed++;
  } else {
    std::cerr << "✗ " << test_name << " FAILED" << std::endl;
    tests_failed++;
  }
}

inline void reset_counters() {
  tests_passed = 0;
  tests_failed = 0;
}

inline int get_passed() {
  return tests_passed;
}

inline int get_failed() {
  return tests_failed;
}

} // namespace test
