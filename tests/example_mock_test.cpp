#include "hyprbar/core/config_manager.h"
#include "hyprbar/widgets/hyprland_widget.h"
#include "hyprbar/widgets/tray_widget.h"
#include "test_mocks.h"
#include "test_utils.h"
#include <iostream>

using namespace hyprbar;

void test_tray_with_mock_dbus() {
  // Create mock D-Bus with 2 tray items
  test::mocks::MockDBus mock_dbus = test::mocks::create_dbus_with_tray_items(2);

  // Start mock in RAII guard (auto-cleanup on scope exit)
  test::mocks::MockGuard<test::mocks::MockDBus> guard(mock_dbus);

  // Now test the tray widget
  TrayWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["icon_size"] = ConfigValue(static_cast<int64_t>(20));
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test::assert(success, "Tray widget initializes with mock D-Bus");

  // Give it time to fetch items
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  int width = widget.get_desired_width();
  test::assert(width > 0, "Tray widget has width with mock items");

  // Mock automatically cleaned up when guard goes out of scope
}

void test_hyprland_with_mock_server() {
  // Create mock Hyprland with 5 workspaces
  test::mocks::MockHyprland mock_hypr =
      test::mocks::create_hyprland_with_workspaces(5);

  // Start mock
  test::mocks::MockGuard<test::mocks::MockHyprland> guard(mock_hypr);

  // Wait for service to be ready
  bool ready = test::mocks::wait_for_service(mock_hypr, 1000);
  test::assert(ready, "Mock Hyprland starts within timeout");

  // Test Hyprland widget
  HyprlandWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["max_workspaces"] = ConfigValue(static_cast<int64_t>(10));
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test::assert(success, "Hyprland widget initializes with mock");

  int width = widget.get_desired_width();
  test::assert(width > 0, "Hyprland widget has width with mock");

  // Mock automatically cleaned up
}

void test_fast_without_mocks() {
  // Fast unit test - no external services
  TrayWidget widget;

  // Widget should handle missing D-Bus gracefully
  std::map<std::string, ConfigValue> cfg_map;
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test::assert(success, "Tray widget initializes without D-Bus");

  int width = widget.get_desired_width();
  test::assert(width == 0, "Tray widget has zero width without D-Bus");
}

void run_mock_tests() {
  std::cout << "\n--- Mock Service Tests ---" << std::endl;

  // Fast test (no mocks)
  test_fast_without_mocks();

  // Tests with mocks (slower but isolated)
  test_tray_with_mock_dbus();
  test_hyprland_with_mock_server();
}

int main() {
  test::reset_counters();
  run_mock_tests();

  std::cout << "\nTests passed: " << test::get_passed() << std::endl;
  std::cout << "Tests failed: " << test::get_failed() << std::endl;

  return test::get_failed() == 0 ? 0 : 1;
}
