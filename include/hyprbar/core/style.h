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
 */
struct Style {
  std::optional<std::string> font;
  std::optional<double> size;
  std::optional<std::string> color;

  /**
   * Merge with parent style (parent provides defaults)
   * Child properties override parent properties
   */
  Style inherit_from(const Style& parent) const {
    Style result;
    result.font = font.has_value() ? font : parent.font;
    result.size = size.has_value() ? size : parent.size;
    result.color = color.has_value() ? color : parent.color;
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
   * Check if fully resolved (no optionals)
   */
  bool is_complete() const {
    return font.has_value() && size.has_value() && color.has_value();
  }
};

} // namespace hyprbar
