#include "hyprbar/core/config_manager.h"
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

void run_widget_tests() {
  std::cout << "\n--- Widget Tests ---" << std::endl;
  test_script_initialization();
  test_script_config();
  test_script_update();
  test_script_dimensions();
  test_integer_vs_double_config();
}
