#pragma once

#include <cairo/cairo.h>
#include <cstdint>
#include <memory>
#include <pango/pangocairo.h>
#include <string>

namespace hyprbar {

/**
 * Color - RGBA color representation
 */
struct Color {
  double r, g, b, a;

  Color(double r = 0, double g = 0, double b = 0, double a = 1.0)
      : r(r), g(g), b(b), a(a) {
  }

  // Parse from hex string (#RRGGBB or #RRGGBBAA)
  static Color from_hex(const std::string& hex);
};

/**
 * Renderer - Manages Cairo rendering to Wayland surface
 */
class Renderer {
public:
  Renderer();
  ~Renderer();

  /**
   * Initialize renderer with surface dimensions
   */
  bool initialize(uint32_t width, uint32_t height);

  /**
   * Begin rendering a new frame
   */
  void begin_frame();

  /**
   * End frame and prepare buffer for commit
   */
  void end_frame();

  /**
   * Get the current frame buffer data
   */
  uint8_t* get_buffer_data() const {
    return buffer_data_;
  }

  /**
   * Get buffer size in bytes
   */
  size_t get_buffer_size() const {
    return buffer_size_;
  }

  /**
   * Get Cairo context for drawing
   */
  cairo_t* get_context() const {
    return cr_;
  }

  /**
   * Get surface width
   */
  uint32_t get_width() const {
    return width_;
  }

  /**
   * Get surface height
   */
  uint32_t get_height() const {
    return height_;
  }

  // Convenience drawing methods
  void clear(const Color& color);
  void fill_rect(double x, double y, double w, double h, const Color& color);
  void draw_text(const std::string& text, double x, double y,
                 const std::string& font, double size, const Color& color);
  void draw_surface(cairo_surface_t* source, double x, double y, double width,
                    double height);

  /**
   * Measure text width using Pango
   * Returns the actual rendered width in pixels
   */
  int measure_text_width(const std::string& text, const std::string& font,
                         double size) const;

private:
  uint32_t width_;
  uint32_t height_;
  uint32_t stride_;

  uint8_t* buffer_data_;
  size_t buffer_size_;

  cairo_surface_t* surface_;
  cairo_t* cr_;
};

} // namespace hyprbar
