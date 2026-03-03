#include "hyprbar/core/config_manager.h"
#include "hyprbar/widgets/widget_manager.h"
#include "test_utils.h"
#include <fstream>

using namespace hyprbar;

// Helper to create a test config file
std::string create_test_config(const std::string& content) {
  std::string path = "/tmp/hyprbar_wm_test_config.json";
  std::ofstream f(path);
  f << content;
  f.close();
  return path;
}

void test_widget_manager_empty_config() {
  std::string config_content = R"({
    "bar": {
      "position": "top",
      "height": 30,
      "background": "#1e1e2e",
      "color": "#cdd6f4",
      "font": "monospace",
      "size": 12
    },
    "widgets": []
  })";

  std::string config_path = create_test_config(config_content);
  ConfigManager config_mgr;
  config_mgr.load(config_path);

  WidgetManager wm;
  bool success = wm.initialize(config_mgr);

  // Empty widgets array should return false (no widgets)
  test::assert(!success, "Widget manager with empty widgets returns false");
}

void test_widget_manager_single_widget() {
  std::string config_content = R"({
    "bar": {
      "position": "top",
      "height": 30,
      "background": "#1e1e2e",
      "color": "#cdd6f4",
      "font": "monospace",
      "size": 12
    },
    "widgets": [
      {
        "type": "tray",
        "position": "right",
        "config": {
          "icon_size": 16
        }
      }
    ]
  })";

  std::string config_path = create_test_config(config_content);
  ConfigManager config_mgr;
  config_mgr.load(config_path);

  WidgetManager wm;
  bool success = wm.initialize(config_mgr);

  test::assert(success, "Widget manager initializes with one widget");
}

void test_widget_manager_multiple_widgets() {
  std::string config_content = R"({
    "bar": {
      "position": "top",
      "height": 30,
      "background": "#1e1e2e",
      "color": "#cdd6f4",
      "font": "monospace",
      "size": 12
    },
    "widgets": [
      {
        "type": "tray",
        "position": "left",
        "config": {}
      },
      {
        "type": "hyprland",
        "position": "center",
        "config": {}
      }
    ]
  })";

  std::string config_path = create_test_config(config_content);
  ConfigManager config_mgr;
  config_mgr.load(config_path);

  WidgetManager wm;
  bool success = wm.initialize(config_mgr);

  test::assert(success, "Widget manager initializes with multiple widgets");
}

void test_widget_manager_unknown_widget_skipped() {
  std::string config_content = R"({
    "bar": {
      "position": "top",
      "height": 30,
      "background": "#1e1e2e",
      "color": "#cdd6f4",
      "font": "monospace",
      "size": 12
    },
    "widgets": [
      {
        "type": "unknown_widget_type",
        "position": "left",
        "config": {}
      },
      {
        "type": "tray",
        "position": "right",
        "config": {}
      }
    ]
  })";

  std::string config_path = create_test_config(config_content);
  ConfigManager config_mgr;
  config_mgr.load(config_path);

  WidgetManager wm;
  bool success = wm.initialize(config_mgr);

  // Should succeed even if one widget is unknown (skips it)
  test::assert(success, "Widget manager skips unknown widget types");
}

void test_widget_manager_update_no_changes() {
  std::string config_content = R"({
    "bar": {
      "position": "top",
      "height": 30,
      "background": "#1e1e2e",
      "color": "#cdd6f4",
      "font": "monospace",
      "size": 12
    },
    "widgets": [
      {
        "type": "tray",
        "position": "left",
        "config": {}
      }
    ]
  })";

  std::string config_path = create_test_config(config_content);
  ConfigManager config_mgr;
  config_mgr.load(config_path);

  WidgetManager wm;
  wm.initialize(config_mgr);

  // Update should return false (tray doesn't animate)
  bool needs_redraw = wm.update();
  test::assert(!needs_redraw,
               "Widget manager update returns false when no changes");
}

void run_widget_manager_integration_tests() {
  std::cout << "\n--- Widget Manager Integration Tests ---" << std::endl;
  test_widget_manager_empty_config();
  test_widget_manager_single_widget();
  test_widget_manager_multiple_widgets();
  test_widget_manager_unknown_widget_skipped();
  test_widget_manager_update_no_changes();
}
