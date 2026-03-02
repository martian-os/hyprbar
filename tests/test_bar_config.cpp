#include "hyprbar/core/config_manager.h"
#include "test_utils.h"
#include <cmath>
#include <fstream>

using namespace hyprbar;

void test_bar_size_config() {
  // Create test config file with explicit size
  const char* config_content = R"({
    "bar": {
      "position": "top",
      "height": 40,
      "background": "#1e1e2e",
      "foreground": "#89b4fa",
      "font": "Noto Sans, Noto Color Emoji",
      "size": 16
    },
    "widgets": []
  })";

  std::ofstream f("/tmp/test_bar_size.json");
  f << config_content;
  f.close();

  // Load and verify
  ConfigManager mgr;
  test::assert(mgr.load("/tmp/test_bar_size.json"), "Load config with size");

  const auto& config = mgr.get_config();

  // Verify bar configuration
  test::assert(config.bar.height == 40, "Bar height is 40");
  test::assert(config.bar.background == "#1e1e2e", "Bar background");
  test::assert(config.bar.foreground == "#89b4fa", "Bar foreground");
  test::assert(config.bar.font == "Noto Sans, Noto Color Emoji", "Bar font");
  test::assert(std::fabs(config.bar.size - 16.0) < 0.001, "Bar size is 16");
}

void test_widget_inheritance_structure() {
  // Create test config with widgets that inherit and override
  const char* config_content = R"({
    "bar": {
      "position": "top",
      "height": 30,
      "background": "#1e1e2e",
      "foreground": "#cdd6f4",
      "font": "Noto Sans",
      "size": 14
    },
    "widgets": [
      {
        "type": "script",
        "position": "left",
        "config": {
          "command": "/tmp/test.sh",
          "interval": 1000
        }
      },
      {
        "type": "script",
        "position": "right",
        "config": {
          "command": "/tmp/test2.sh",
          "interval": 2000,
          "size": 12,
          "color": "#f38ba8"
        }
      }
    ]
  })";

  std::ofstream f("/tmp/test_inheritance.json");
  f << config_content;
  f.close();

  // Load configuration
  ConfigManager mgr;
  test::assert(mgr.load("/tmp/test_inheritance.json"),
               "Load inheritance config");

  const auto& config = mgr.get_config();

  // Verify bar defaults
  test::assert(config.bar.font == "Noto Sans", "Bar font");
  test::assert(std::fabs(config.bar.size - 14.0) < 0.001, "Bar size is 14");
  test::assert(config.bar.foreground == "#cdd6f4", "Bar foreground");

  // Verify widgets loaded
  test::assert(config.widgets.size() == 2, "Two widgets loaded");

  const auto& w1_config = config.widgets[0].config;
  const auto& w2_config = config.widgets[1].config;

  test::assert(w1_config.is_object(), "Widget 1 is object");
  test::assert(w2_config.is_object(), "Widget 2 is object");

  // Widget 1 should not have size/color (will inherit from bar)
  test::assert(!w1_config.get("size").has_value(),
               "Widget 1 has no size (inherits)");
  test::assert(!w1_config.get("color").has_value(),
               "Widget 1 has no color (inherits)");

  // Widget 2 should have overridden values
  auto w2_size = w2_config.get("size");
  auto w2_color = w2_config.get("color");
  test::assert(w2_size.has_value(), "Widget 2 has size");
  test::assert(w2_color.has_value(), "Widget 2 has color");
  test::assert(w2_size->as_int() == 12, "Widget 2 size is 12");
  test::assert(w2_color->as_string() == "#f38ba8", "Widget 2 color override");
}

void test_size_as_double() {
  // Test that size can be specified as float
  const char* config_content = R"({
    "bar": {
      "position": "top",
      "height": 30,
      "background": "#1e1e2e",
      "foreground": "#cdd6f4",
      "font": "Noto Sans",
      "size": 14.5
    },
    "widgets": []
  })";

  std::ofstream f("/tmp/test_size_double.json");
  f << config_content;
  f.close();

  ConfigManager mgr;
  test::assert(mgr.load("/tmp/test_size_double.json"),
               "Load config with float size");

  const auto& config = mgr.get_config();
  test::assert(std::fabs(config.bar.size - 14.5) < 0.001, "Bar size is 14.5");
}

void test_missing_size_uses_default() {
  // Test that missing size field uses default
  const char* config_content = R"({
    "bar": {
      "position": "top",
      "height": 30,
      "background": "#1e1e2e",
      "foreground": "#cdd6f4",
      "font": "Noto Sans"
    },
    "widgets": []
  })";

  std::ofstream f("/tmp/test_default_size.json");
  f << config_content;
  f.close();

  ConfigManager mgr;
  test::assert(mgr.load("/tmp/test_default_size.json"),
               "Load config without size");

  const auto& config = mgr.get_config();
  test::assert(std::fabs(config.bar.size - 14.0) < 0.001, "Default size is 14");
}

void run_bar_config_tests() {
  test_bar_size_config();
  test_widget_inheritance_structure();
  test_size_as_double();
  test_missing_size_uses_default();
}
