#include "hyprbar/core/config_manager.h"
#include "hyprbar/widgets/hyprland_widget.h"
#include "test_utils.h"
#include <map>

using namespace hyprbar;

void test_hyprland_config_parsing() {
  HyprlandWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  cfg_map["max_workspaces"] = ConfigValue(static_cast<int64_t>(8));
  cfg_map["active_color"] = ConfigValue("#89b4fa");
  cfg_map["occupied_color"] = ConfigValue("#cdd6f4");
  cfg_map["empty_color"] = ConfigValue("#45475a");
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test::assert(success, "Hyprland widget parses all config options");
}

void test_hyprland_fallback_without_env() {
  // Without HYPRLAND_INSTANCE_SIGNATURE, widget should gracefully degrade
  unsetenv("HYPRLAND_INSTANCE_SIGNATURE");

  HyprlandWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test::assert(success, "Hyprland widget initializes without env var");

  // Should have zero width (no connection)
  int width = widget.get_desired_width();
  test::assert(width == 0, "Widget has zero width without Hyprland");
}

void test_hyprland_default_values() {
  HyprlandWidget widget;
  std::map<std::string, ConfigValue> cfg_map;
  // Empty config - should use defaults
  ConfigValue cfg(cfg_map);

  bool success = widget.initialize(cfg);
  test::assert(success, "Hyprland widget uses default values");
}

void run_hyprland_mock_tests() {
  std::cout << "\n--- Hyprland Widget Tests ---" << std::endl;
  test_hyprland_fallback_without_env();
  test_hyprland_config_parsing();
  test_hyprland_default_values();
}
