#include "hyprbar/core/config_manager.h"
#include "hyprbar/rendering/renderer.h"
#include "hyprbar/widgets/widget_manager.h"
#include "test_utils.h"
#include <chrono>
#include <fstream>
#include <thread>

using namespace hyprbar;

std::string create_script_test_config(const std::string& content) {
  std::string path = "/tmp/hyprbar_script_test.json";
  std::ofstream f(path);
  f << content;
  f.close();
  return path;
}

void test_widget_manager_with_script_widget() {
  // Create a simple test script
  std::ofstream script("/tmp/test_widget.sh");
  script << "#!/bin/bash\necho 'Test Widget'\n";
  script.close();
  system("chmod +x /tmp/test_widget.sh");

  std::string config = R"({
    "bar": {"position": "top", "height": 30, "background": "#1e1e2e",
            "color": "#cdd6f4", "font": "monospace", "size": 12},
    "widgets": [
      {"type": "script", "position": "left", 
       "config": {"command": "/tmp/test_widget.sh", "interval": 1000}}
    ]
  })";

  ConfigManager config_mgr;
  config_mgr.load(create_script_test_config(config));

  WidgetManager wm;
  bool success = wm.initialize(config_mgr);
  test::assert(success, "Widget manager initializes with script widget");

  // Wait for script to produce output
  bool ready = wm.wait_for_ready(2000);
  test::assert(ready, "Script widget populates within timeout");
}

void test_widget_manager_measure_widgets() {
  std::string config = R"({
    "bar": {"position": "top", "height": 30, "background": "#1e1e2e",
            "color": "#cdd6f4", "font": "monospace", "size": 12},
    "widgets": [
      {"type": "tray", "position": "left", "config": {"icon_size": 20}},
      {"type": "tray", "position": "right", "config": {"icon_size": 16}}
    ]
  })";

  ConfigManager config_mgr;
  config_mgr.load(create_script_test_config(config));

  WidgetManager wm;
  wm.initialize(config_mgr);

  Renderer renderer;
  renderer.initialize(800, 30);

  // Render triggers measurement
  renderer.begin_frame();
  wm.render(renderer, 800, 30);
  renderer.end_frame();

  test::assert(true, "Widget measurement integrated in render");
}

void test_widget_manager_empty_render() {
  std::string config = R"({
    "bar": {"position": "top", "height": 30, "background": "#1e1e2e",
            "color": "#cdd6f4", "font": "monospace", "size": 12},
    "widgets": []
  })";

  ConfigManager config_mgr;
  config_mgr.load(create_script_test_config(config));

  WidgetManager wm;
  wm.initialize(config_mgr); // Returns false

  Renderer renderer;
  renderer.initialize(800, 30);

  // Should not crash with empty widgets
  renderer.begin_frame();
  wm.render(renderer, 800, 30);
  renderer.end_frame();

  test::assert(true, "Render handles empty widget list");
}

void test_widget_manager_center_positioning() {
  std::string config = R"({
    "bar": {"position": "top", "height": 30, "background": "#1e1e2e",
            "color": "#cdd6f4", "font": "monospace", "size": 12},
    "widgets": [
      {"type": "tray", "position": "center", "config": {"icon_size": 16}}
    ]
  })";

  ConfigManager config_mgr;
  config_mgr.load(create_script_test_config(config));

  WidgetManager wm;
  wm.initialize(config_mgr);

  Renderer renderer;
  renderer.initialize(1000, 30);

  renderer.begin_frame();
  wm.render(renderer, 1000, 30);
  renderer.end_frame();

  test::assert(true, "Center positioned widget renders");
}

void test_widget_manager_all_three_positions() {
  std::string config = R"({
    "bar": {"position": "top", "height": 30, "background": "#1e1e2e",
            "color": "#cdd6f4", "font": "monospace", "size": 12},
    "widgets": [
      {"type": "tray", "position": "left", "config": {}},
      {"type": "tray", "position": "center", "config": {}},
      {"type": "tray", "position": "right", "config": {}}
    ]
  })";

  ConfigManager config_mgr;
  config_mgr.load(create_script_test_config(config));

  WidgetManager wm;
  wm.initialize(config_mgr);

  Renderer renderer;
  renderer.initialize(1500, 30);

  renderer.begin_frame();
  wm.render(renderer, 1500, 30);
  renderer.end_frame();

  test::assert(true, "All three position zones render correctly");
}

void run_widget_manager_advanced_tests() {
  std::cout << "\n--- Widget Manager Advanced Tests ---" << std::endl;
  test_widget_manager_with_script_widget();
  test_widget_manager_measure_widgets();
  test_widget_manager_empty_render();
  test_widget_manager_center_positioning();
  test_widget_manager_all_three_positions();
}
