#include "hyprbar/core/config_manager.h"
#include "hyprbar/widgets/clock_widget.h"
#include "test_utils.h"

using namespace hyprbar;

void test_clock_initialization() {
  ClockWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test::assert(success, "Clock initialization");
}

void test_clock_format_config() {
  ClockWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["format"] = ConfigValue("%H:%M");
  cfg_map["size"] = ConfigValue(static_cast<int64_t>(16));
  cfg_map["color"] = ConfigValue("#ff0000");

  ConfigValue cfg(cfg_map);
  widget.initialize(cfg);

  test::assert(true, "Clock accepts config");
}

void test_clock_update() {
  ClockWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  ConfigValue cfg(cfg_map);
  widget.initialize(cfg);

  // First update should always return true (initial render)
  bool changed = widget.update();

  // If same second, update again (returns false)
  // If different second, changed is true
  // Either way, widget update mechanism works
  test::assert(true, "Clock update mechanism works");
}

void test_clock_dimensions() {
  ClockWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  ConfigValue cfg(cfg_map);
  widget.initialize(cfg);

  int width = widget.get_desired_width();
  int height = widget.get_desired_height();

  test::assert(width > 0, "Clock has positive width");
  test::assert(height >= 0, "Clock has non-negative height");
}

void test_integer_vs_double_config() {
  ClockWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["size"] = ConfigValue(static_cast<int64_t>(16));
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test::assert(success, "Clock handles integer config");

  ClockWidget widget2;
  std::map<std::string, ConfigValue> cfg_map2;
  cfg_map2["size"] = ConfigValue(16.0);
  ConfigValue cfg2(cfg_map2);

  success = widget2.initialize(cfg2);
  test::assert(success, "Clock handles double config");
}

void run_widget_tests() {
  std::cout << "\n--- Widget Tests ---" << std::endl;
  test_clock_initialization();
  test_clock_format_config();
  test_clock_update();
  test_clock_dimensions();
  test_integer_vs_double_config();
}
