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

void test_style_new_css_attributes() {
  Style style;

  // Defaults
  test::assert(style.get_background() == "", "Default background empty");
  test::assert(style.get_padding() == 0, "Default padding 0");
  test::assert(style.get_border_radius() == 0, "Default border-radius 0");
  test::assert(style.get_border_color() == "", "Default border-color empty");
  test::assert(style.get_border_width() == 0, "Default border-width 0");

  // Set values
  style.background = "#313244";
  style.padding = 6;
  style.border_radius = 8;
  style.border_color = "#89b4fa";
  style.border_width = 2;

  test::assert(style.get_background() == "#313244", "Custom background");
  test::assert(style.get_padding() == 6, "Custom padding");
  test::assert(style.get_border_radius() == 8, "Custom border-radius");
  test::assert(style.get_border_color() == "#89b4fa", "Custom border-color");
  test::assert(style.get_border_width() == 2, "Custom border-width");
}

void test_style_css_inheritance() {
  Style parent;
  parent.background = "#1e1e2e";
  parent.padding = 4;
  parent.border_radius = 6;
  parent.border_color = "#45475a";
  parent.border_width = 1;

  // Child inherits all
  Style child;
  Style resolved = child.inherit_from(parent);
  test::assert(resolved.get_background() == "#1e1e2e", "Inherited background");
  test::assert(resolved.get_padding() == 4, "Inherited padding");
  test::assert(resolved.get_border_radius() == 6, "Inherited border-radius");

  // Child overrides just background and radius
  Style child2;
  child2.background = "#313244";
  child2.border_radius = 12;
  Style resolved2 = child2.inherit_from(parent);
  test::assert(resolved2.get_background() == "#313244", "Override background");
  test::assert(resolved2.get_border_radius() == 12, "Override border-radius");
  test::assert(resolved2.get_padding() == 4, "Inherited padding unchanged");
  test::assert(resolved2.get_border_color() == "#45475a",
               "Inherited border-color");
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
  test_style_new_css_attributes();
  test_style_css_inheritance();
}
