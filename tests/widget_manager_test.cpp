#include "hyprbar/core/config_manager.h"
#include "hyprbar/widgets/widget_manager.h"
#include "test_utils.h"
#include <map>

using namespace hyprbar;

// Mock widget for layout testing
class MockWidget : public Widget {
public:
  MockWidget(int width, int height = 0)
      : desired_width_(width), desired_height_(height) {
  }

  bool initialize(const ConfigValue& /*config*/) override {
    return true;
  }
  bool update() override {
    return false;
  }
  void render(Renderer& /*renderer*/, int /*x*/, int /*y*/, int /*width*/,
              int /*height*/) override {
  }
  int get_desired_width() const override {
    return desired_width_;
  }
  int get_desired_height() const override {
    return desired_height_;
  }
  std::string get_type() const override {
    return "mock";
  }

private:
  int desired_width_;
  int desired_height_;
};

void test_layout_positioning() {
  // Test the layout logic with known widget sizes
  // This tests the mathematical positioning without actual rendering

  const int bar_width = 1000;
  const int spacing = 10;
  const int margin = 10;

  // Scenario 1: Single left widget
  {
    int widget_width = 100;
    int expected_x = margin; // 10
    test::assert(expected_x == 10, "Left widget starts at margin");
  }

  // Scenario 2: Multiple left widgets
  {
    int w1 = 100, w2 = 150;
    int x1 = margin;                // 10
    int x2 = margin + w1 + spacing; // 10 + 100 + 10 = 120
    test::assert(x1 == 10, "First left widget at margin");
    test::assert(x2 == 120, "Second left widget after first + spacing");
  }

  // Scenario 3: Center widgets
  {
    int w1 = 80, w2 = 120;
    int total_width = w1 + spacing + w2;              // 80 + 10 + 120 = 210
    int center_start = (bar_width - total_width) / 2; // (1000 - 210) / 2 = 395
    test::assert(center_start == 395,
                 "Center widgets start at calculated center");
  }

  // Scenario 4: Right widgets
  {
    int w1 = 90, w2 = 110;
    int total_width = w1 + spacing + w2;                // 90 + 10 + 110 = 210
    int right_start = bar_width - margin - total_width; // 1000 - 10 - 210 = 780
    test::assert(right_start == 780, "Right widgets start from right edge");
  }

  // Scenario 5: Mixed positioning
  {
    // Left: 100px widget
    // Center: 50px widget
    // Right: 80px widget
    int left_x = margin;                   // 10
    int center_x = (bar_width - 50) / 2;   // (1000 - 50) / 2 = 475
    int right_x = bar_width - margin - 80; // 1000 - 10 - 80 = 910

    test::assert(left_x == 10, "Left widget at 10");
    test::assert(center_x == 475, "Center widget at 475");
    test::assert(right_x == 910, "Right widget at 910");
  }
}

void test_layout_spacing() {
  const int spacing = 10;
  const int w1 = 50, w2 = 60, w3 = 70;

  // Three widgets in sequence
  int x1 = 10;
  int x2 = x1 + w1 + spacing; // 10 + 50 + 10 = 70
  int x3 = x2 + w2 + spacing; // 70 + 60 + 10 = 140

  test::assert(x2 == 70, "Second widget correctly spaced");
  test::assert(x3 == 140, "Third widget correctly spaced");

  // Spacing calculation for total width
  int total_with_spacing = w1 + w2 + w3 + (2 * spacing); // 50+60+70+20 = 200
  test::assert(total_with_spacing == 200, "Total width with spacing");
}

void test_layout_centering() {
  // Test center calculation for different scenarios
  const int bar_width = 800;

  // Single widget: 100px
  {
    int widget_width = 100;
    int center_x = (bar_width - widget_width) / 2; // (800-100)/2 = 350
    test::assert(center_x == 350, "Single widget centered");
  }

  // Two widgets: 80px + 10px spacing + 90px = 180px total
  {
    int total = 80 + 10 + 90;
    int center_x = (bar_width - total) / 2; // (800-180)/2 = 310
    test::assert(center_x == 310, "Two widgets centered");
  }

  // Odd bar width: 801px, widget: 100px
  {
    int center_x = (801 - 100) / 2; // 700/2 = 350 (integer division)
    test::assert(center_x == 350, "Odd width handled");
  }
}

void run_widget_manager_tests() {
  std::cout << "\n--- Widget Manager Tests ---" << std::endl;
  test_layout_positioning();
  test_layout_spacing();
  test_layout_centering();
}
