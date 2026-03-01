#include "test_utils.h"
#include <iostream>

// Forward declarations from test files
void run_basic_tests();
void run_config_tests();
void run_renderer_tests();
void run_widget_tests();

int main() {
  std::cout << "\n========================================" << std::endl;
  std::cout << "       Hyprbar Test Suite" << std::endl;
  std::cout << "========================================" << std::endl;

  test::reset_counters();

  run_basic_tests();
  run_config_tests();
  run_renderer_tests();
  run_widget_tests();

  std::cout << "\n========================================" << std::endl;
  std::cout << "Tests passed: " << test::get_passed() << std::endl;
  std::cout << "Tests failed: " << test::get_failed() << std::endl;
  std::cout << "========================================\n" << std::endl;

  return test::get_failed() == 0 ? 0 : 1;
}
