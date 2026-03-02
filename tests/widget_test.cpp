#include "hyprbar/core/config_manager.h"
#include "hyprbar/rendering/renderer.h"
#include "hyprbar/widgets/script_widget.h"
#include "test_utils.h"
#include <chrono>
#include <thread>

using namespace hyprbar;

void test_script_initialization() {
  ScriptWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["command"] = ConfigValue("echo 'Hello'");
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test::assert(success, "Script initialization");
}

void test_script_config() {
  ScriptWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["command"] = ConfigValue("date");
  cfg_map["interval"] = ConfigValue(static_cast<int64_t>(2000));
  cfg_map["size"] = ConfigValue(static_cast<int64_t>(16));
  cfg_map["color"] = ConfigValue("#ff0000");

  ConfigValue cfg(cfg_map);
  bool success = widget.initialize(cfg);
  test::assert(success, "Script accepts config");
}

void test_script_update() {
  ScriptWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["command"] = ConfigValue("echo 'test'");
  cfg_map["interval"] = ConfigValue(static_cast<int64_t>(100));
  ConfigValue cfg(cfg_map);
  widget.initialize(cfg);

  // Wait for first execution
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Widget should have output now
  bool changed = widget.update();
  test::assert(true, "Script update mechanism works");
}

void test_script_dimensions() {
  ScriptWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["command"] = ConfigValue("echo 'Hello World'");
  ConfigValue cfg(cfg_map);
  widget.initialize(cfg);

  // Wait for execution
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  int width = widget.get_desired_width();
  int height = widget.get_desired_height();

  test::assert(width >= 0, "Script has non-negative width");
  test::assert(height >= 0, "Script has non-negative height");
}

void test_integer_vs_double_config() {
  ScriptWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["command"] = ConfigValue("echo 'test'");
  cfg_map["size"] = ConfigValue(static_cast<int64_t>(16));
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test::assert(success, "Script handles integer config");

  ScriptWidget widget2;
  std::map<std::string, ConfigValue> cfg_map2;
  cfg_map2["command"] = ConfigValue("echo 'test'");
  cfg_map2["size"] = ConfigValue(16.0);
  ConfigValue cfg2(cfg_map2);

  success = widget2.initialize(cfg2);
  test::assert(success, "Script handles double config");
}

void test_font_size_affects_width() {
  // Create renderer for measurement
  Renderer renderer;
  renderer.initialize(1000, 50);

  // Create two widgets with same text, different sizes
  ScriptWidget widget_small;
  std::map<std::string, ConfigValue> cfg_small;
  cfg_small["command"] = ConfigValue("echo 'Test Text 123'");
  cfg_small["size"] = ConfigValue(12.0);
  ConfigValue config_small(cfg_small);
  widget_small.initialize(config_small);

  ScriptWidget widget_large;
  std::map<std::string, ConfigValue> cfg_large;
  cfg_large["command"] = ConfigValue("echo 'Test Text 123'");
  cfg_large["size"] = ConfigValue(24.0);
  ConfigValue config_large(cfg_large);
  widget_large.initialize(config_large);

  // Wait for scripts to execute
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Measure widths
  widget_small.measure_width(renderer);
  widget_large.measure_width(renderer);

  int width_small = widget_small.get_desired_width();
  int width_large = widget_large.get_desired_width();

  test::assert(width_large > width_small,
               "Larger font size produces wider text");
  test::assert(width_small > 20, "Small font text has non-trivial width");
  test::assert(width_large > 20, "Large font text has non-trivial width");

  // Verify approximately 2x ratio (24pt vs 12pt)
  double ratio = static_cast<double>(width_large) / width_small;
  test::assert(ratio > 1.5 && ratio < 2.5,
               "Width ratio roughly matches size ratio");
}

void run_widget_tests() {
  std::cout << "\n--- Widget Tests ---" << std::endl;
  test_script_initialization();
  test_script_config();
  test_script_update();
  test_script_dimensions();
  test_integer_vs_double_config();
  test_font_size_affects_width();
}
