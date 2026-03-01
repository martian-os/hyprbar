#include "hyprbar/core/config_manager.h"
#include "hyprbar/rendering/renderer.h"
#include "hyprbar/widgets/clock_widget.h"
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

using namespace hyprbar;

int tests_passed = 0;
int tests_failed = 0;

void test_assert(bool condition, const char* test_name) {
  if (condition) {
    std::cout << "✓ " << test_name << std::endl;
    tests_passed++;
  } else {
    std::cerr << "✗ " << test_name << " FAILED" << std::endl;
    tests_failed++;
  }
}

// ===== Basic Tests =====

void test_basic_sanity() {
  test_assert(true, "Sanity check");
  test_assert(1 + 1 == 2, "Basic arithmetic");
}

void test_string_operations() {
  std::string test = "hyprbar";
  test_assert(test.length() == 7, "String length check");
  test_assert(test.substr(0, 4) == "hypr", "String substring check");
}

void test_chrono_duration() {
  using namespace std::chrono;
  auto duration = milliseconds(1000);
  auto seconds_count = duration_cast<seconds>(duration).count();
  test_assert(seconds_count == 1, "Duration conversion");
}

// ===== Config Tests =====

void test_config_value_types() {
  ConfigValue int_val(static_cast<int64_t>(42));
  test_assert(int_val.type == ConfigValue::Type::Integer, "Integer type");
  test_assert(int_val.int_value == 42, "Integer value");

  ConfigValue double_val(3.14);
  test_assert(double_val.type == ConfigValue::Type::Double, "Double type");
  test_assert(double_val.double_value == 3.14, "Double value");

  ConfigValue str_val("test");
  test_assert(str_val.type == ConfigValue::Type::String, "String type");
  test_assert(str_val.string_value == "test", "String value");

  ConfigValue bool_val(true);
  test_assert(bool_val.type == ConfigValue::Type::Boolean, "Boolean type");
  test_assert(bool_val.bool_value == true, "Boolean true value");
}

void test_config_array() {
  std::vector<ConfigValue> arr = {ConfigValue(static_cast<int64_t>(1)),
                                  ConfigValue(static_cast<int64_t>(2)),
                                  ConfigValue(static_cast<int64_t>(3))};
  ConfigValue val(arr);
  test_assert(val.type == ConfigValue::Type::Array, "Array type");
  test_assert(val.array_value.size() == 3, "Array size");
  test_assert(val.array_value[0].int_value == 1, "Array element");
}

void test_default_config() {
  Config cfg;
  test_assert(cfg.bar.height == 30, "Default bar height");
  test_assert(cfg.bar.background == "#1e1e2e", "Default background");
}

// ===== Renderer Tests =====

void test_renderer_init() {
  Renderer r;
  bool success = r.initialize(800, 60);
  test_assert(success, "Renderer initialization");
  test_assert(r.get_buffer_data() != nullptr, "Buffer allocated");
  test_assert(r.get_buffer_size() == 800 * 60 * 4, "Buffer size");
}

void test_color_parsing() {
  Color c = Color::from_hex("#ff0000");
  test_assert(c.r == 1.0, "Red channel");
  test_assert(c.g == 0.0, "Green channel");
  test_assert(c.b == 0.0, "Blue channel");
}

void test_render_lifecycle() {
  Renderer r;
  r.initialize(100, 50);
  r.begin_frame();
  r.clear(Color{0.5, 0.5, 0.5, 1.0});
  r.end_frame();
  test_assert(true, "Frame lifecycle completes");
}

// ===== Widget Tests =====

void test_clock_widget() {
  ClockWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test_assert(success, "Clock initialization");

  int width = widget.get_desired_width();
  test_assert(width > 0, "Clock has width");
}

void test_widget_config_types() {
  ClockWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["size"] = ConfigValue(static_cast<int64_t>(16));
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test_assert(success, "Widget handles integer config");
}

int main() {
  std::cout << "\n========================================" << std::endl;
  std::cout << "       Hyprbar Test Suite" << std::endl;
  std::cout << "========================================\n" << std::endl;

  std::cout << "--- Basic Tests ---" << std::endl;
  test_basic_sanity();
  test_string_operations();
  test_chrono_duration();

  std::cout << "\n--- Config Tests ---" << std::endl;
  test_config_value_types();
  test_config_array();
  test_default_config();

  std::cout << "\n--- Renderer Tests ---" << std::endl;
  test_renderer_init();
  test_color_parsing();
  test_render_lifecycle();

  std::cout << "\n--- Widget Tests ---" << std::endl;
  test_clock_widget();
  test_widget_config_types();

  std::cout << "\n========================================" << std::endl;
  std::cout << "Tests passed: " << tests_passed << std::endl;
  std::cout << "Tests failed: " << tests_failed << std::endl;
  std::cout << "========================================\n" << std::endl;

  return tests_failed == 0 ? 0 : 1;
}
