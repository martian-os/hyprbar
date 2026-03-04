#pragma once

#include <optional>
#include <string>

namespace hyprbar {

/**
 * Style - Unified styling properties for bar and widgets
 *
 * CSS-like styling that can be inherited:
 * - Bar defines default style
 * - Widgets inherit from bar
 * - Widgets can override specific properties
 *
 * Supported CSS-inspired attributes:
 *   font          - font family (e.g. "Noto Sans")
 *   size          - font size in pixels
 *   color         - foreground/text color (#RRGGBB or #RRGGBBAA)
 *   background    - background color (#RRGGBB or #RRGGBBAA)
 *   padding       - inner spacing in pixels (uniform, like CSS shorthand)
 *   border_radius - corner radius in pixels (CSS: border-radius)
 *   border_color  - border color (#RRGGBB or #RRGGBBAA)
 *   border_width  - border thickness in pixels (CSS: border-width)
 */
struct Style {
  std::optional<std::string> font;
  std::optional<double> size;
  std::optional<std::string> color;
  std::optional<std::string> background;
  std::optional<int> padding;
  std::optional<int> border_radius;
  std::optional<std::string> border_color;
  std::optional<int> border_width;

  /**
   * Merge with parent style (parent provides defaults)
   * Child properties override parent properties
   */
  Style inherit_from(const Style& parent) const {
    Style result;
    result.font = font.has_value() ? font : parent.font;
    result.size = size.has_value() ? size : parent.size;
    result.color = color.has_value() ? color : parent.color;
    result.background = background.has_value() ? background : parent.background;
    result.padding = padding.has_value() ? padding : parent.padding;
    result.border_radius =
        border_radius.has_value() ? border_radius : parent.border_radius;
    result.border_color =
        border_color.has_value() ? border_color : parent.border_color;
    result.border_width =
        border_width.has_value() ? border_width : parent.border_width;
    return result;
  }

  /**
   * Get resolved font or default
   */
  std::string get_font(const std::string& default_val = "monospace") const {
    return font.value_or(default_val);
  }

  /**
   * Get resolved size or default
   */
  double get_size(double default_val = 14.0) const {
    return size.value_or(default_val);
  }

  /**
   * Get resolved color or default
   */
  std::string get_color(const std::string& default_val = "#cdd6f4") const {
    return color.value_or(default_val);
  }

  /**
   * Get resolved background or default (empty = no background)
   */
  std::string get_background(const std::string& default_val = "") const {
    return background.value_or(default_val);
  }

  /**
   * Get resolved padding or default
   */
  int get_padding(int default_val = 0) const {
    return padding.value_or(default_val);
  }

  /**
   * Get resolved border-radius or default
   */
  int get_border_radius(int default_val = 0) const {
    return border_radius.value_or(default_val);
  }

  /**
   * Get resolved border-color or default (empty = no border)
   */
  std::string get_border_color(const std::string& default_val = "") const {
    return border_color.value_or(default_val);
  }

  /**
   * Get resolved border-width or default
   */
  int get_border_width(int default_val = 0) const {
    return border_width.value_or(default_val);
  }

  /**
   * Check if fully resolved (core text properties)
   */
  bool is_complete() const {
    return font.has_value() && size.has_value() && color.has_value();
  }
};

} // namespace hyprbar
