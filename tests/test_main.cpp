#include "test_utils.h"
#include <cstdlib>
#include <cstring>
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
void run_widget_manager_render_tests();
void run_widget_manager_advanced_tests();

int main() {
  std::cout << "\n========================================" << std::endl;
  std::cout << "       Hyprbar Test Suite" << std::endl;
  std::cout << "========================================" << std::endl;

  // Check test mode
  const char* test_mode = getenv("HYPRBAR_TEST_MODE");
  bool fast_mode = test_mode && strcmp(test_mode, "fast") == 0;
  bool mock_mode = test_mode && strcmp(test_mode, "mocks") == 0;

  if (fast_mode) {
    std::cout << "Running in FAST mode (no external services)" << std::endl;
  } else if (mock_mode) {
    std::cout << "Running in MOCK mode (with mock services)" << std::endl;
  } else {
    std::cout << "Running in DEFAULT mode" << std::endl;
  }

  test::reset_counters();

  // Fast tests - no external dependencies
  run_basic_tests();
  run_config_tests();
  run_renderer_tests();
  run_widget_tests();
  run_bar_config_tests();
  run_style_tests();
  run_widget_manager_tests();

  // Tests that need mocks (skip in fast mode)
  if (!fast_mode) {
    // run_hyprland_mock_tests();  // TODO: Re-enable with proper mocks
    // run_tray_widget_tests();     // TODO: Re-enable with proper mocks
    run_widget_manager_integration_tests();
    run_widget_manager_render_tests();
    run_widget_manager_advanced_tests();
  }

  std::cout << "\n========================================" << std::endl;
  std::cout << "Tests passed: " << test::get_passed() << std::endl;
  std::cout << "Tests failed: " << test::get_failed() << std::endl;
  std::cout << "========================================\n" << std::endl;

  return test::get_failed() == 0 ? 0 : 1;
}
