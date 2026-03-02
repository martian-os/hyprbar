#include "hyprbar/core/style.h"
#include "test_utils.h"

using namespace hyprbar;

void test_style_defaults() {
  Style style;
  test::assert(style.get_font() == "monospace", "Default font");
  test::assert(style.get_size() == 14.0, "Default size");
  test::assert(style.get_color() == "#cdd6f4", "Default color");
}

void test_style_values() {
  Style style;
  style.font = "Arial";
  style.size = 16.0;
  style.color = "#ff0000";

  test::assert(style.get_font() == "Arial", "Custom font");
  test::assert(style.get_size() == 16.0, "Custom size");
  test::assert(style.get_color() == "#ff0000", "Custom color");
}

void test_style_inheritance() {
  // Parent (bar) style
  Style bar_style;
  bar_style.font = "Noto Sans";
  bar_style.size = 14.0;
  bar_style.color = "#cdd6f4";

  // Child (widget) inherits everything
  Style widget_style;
  Style resolved = widget_style.inherit_from(bar_style);

  test::assert(resolved.get_font() == "Noto Sans", "Inherited font");
  test::assert(resolved.get_size() == 14.0, "Inherited size");
  test::assert(resolved.get_color() == "#cdd6f4", "Inherited color");
}

void test_style_override() {
  // Parent (bar) style
  Style bar_style;
  bar_style.font = "Noto Sans";
  bar_style.size = 14.0;
  bar_style.color = "#cdd6f4";

  // Child (widget) overrides size and color
  Style widget_style;
  widget_style.size = 12.0;
  widget_style.color = "#ff0000";

  Style resolved = widget_style.inherit_from(bar_style);

  test::assert(resolved.get_font() == "Noto Sans", "Inherited font");
  test::assert(resolved.get_size() == 12.0, "Overridden size");
  test::assert(resolved.get_color() == "#ff0000", "Overridden color");
}

void test_style_partial_override() {
  // Parent style
  Style bar_style;
  bar_style.font = "Noto Sans";
  bar_style.size = 14.0;
  bar_style.color = "#cdd6f4";

  // Widget only overrides color
  Style widget_style;
  widget_style.color = "#00ff00";

  Style resolved = widget_style.inherit_from(bar_style);

  test::assert(resolved.get_font() == "Noto Sans", "Inherited font");
  test::assert(resolved.get_size() == 14.0, "Inherited size");
  test::assert(resolved.get_color() == "#00ff00", "Overridden color only");
}

void test_style_completeness() {
  Style incomplete;
  test::assert(!incomplete.is_complete(), "Empty style not complete");

  incomplete.font = "Arial";
  test::assert(!incomplete.is_complete(), "Partial style not complete");

  incomplete.size = 12.0;
  test::assert(!incomplete.is_complete(), "Still incomplete");

  incomplete.color = "#fff";
  test::assert(incomplete.is_complete(), "Complete style");
}

void test_style_attribute_names() {
  // Verify attribute names match between bar and widgets
  Style bar_style;
  bar_style.font = "Noto Sans"; // bar uses 'font'
  bar_style.size = 14.0;        // bar uses 'size'
  bar_style.color = "#cdd6f4";  // bar uses 'foreground', widget uses 'color'

  Style widget_style;
  widget_style.font = "monospace"; // widget uses 'font'
  widget_style.size = 12.0;        // widget uses 'size'
  widget_style.color = "#ff0000";  // widget uses 'color'

  // Both use same attribute names via Style struct
  test::assert(bar_style.font.has_value(), "Bar has font");
  test::assert(bar_style.size.has_value(), "Bar has size");
  test::assert(bar_style.color.has_value(), "Bar has color");

  test::assert(widget_style.font.has_value(), "Widget has font");
  test::assert(widget_style.size.has_value(), "Widget has size");
  test::assert(widget_style.color.has_value(), "Widget has color");
}

void run_style_tests() {
  std::cout << "\n--- Style Tests ---" << std::endl;
  test_style_defaults();
  test_style_values();
  test_style_inheritance();
  test_style_override();
  test_style_partial_override();
  test_style_completeness();
  test_style_attribute_names();
}
