#include "test_utils.h"
#include <iostream>

// Forward declarations from test files
void run_basic_tests();
void run_config_tests();
void run_renderer_tests();
void run_widget_tests();
void run_bar_config_tests();
void run_style_tests();
void run_hyprland_mock_tests();
void run_widget_manager_tests();
void run_tray_widget_tests();
void run_widget_manager_integration_tests();

int main() {
  std::cout << "\n========================================" << std::endl;
  std::cout << "       Hyprbar Test Suite" << std::endl;
  std::cout << "========================================" << std::endl;

  test::reset_counters();

  run_basic_tests();
  run_config_tests();
  run_renderer_tests();
  run_widget_tests();
  run_bar_config_tests();
  run_style_tests();
  run_hyprland_mock_tests();
  run_widget_manager_tests();
  run_tray_widget_tests();
  run_widget_manager_integration_tests();

  std::cout << "\n========================================" << std::endl;
  std::cout << "Tests passed: " << test::get_passed() << std::endl;
  std::cout << "Tests failed: " << test::get_failed() << std::endl;
  std::cout << "========================================\n" << std::endl;

  return test::get_failed() == 0 ? 0 : 1;
}
