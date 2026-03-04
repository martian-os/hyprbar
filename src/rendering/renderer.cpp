#include "hyprbar/rendering/renderer.h"
#include "hyprbar/core/logger.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace hyprbar {

Color Color::from_hex(const std::string& hex) {
  std::string h = hex;
  if (h[0] == '#') {
    h = h.substr(1);
  }

  uint32_t value = 0;
  std::stringstream ss;
  ss << std::hex << h;
  ss >> value;

  double r, g, b, a = 1.0;

  if (h.length() == 6) {
    r = ((value >> 16) & 0xFF) / 255.0;
    g = ((value >> 8) & 0xFF) / 255.0;
    b = (value & 0xFF) / 255.0;
  } else if (h.length() == 8) {
    r = ((value >> 24) & 0xFF) / 255.0;
    g = ((value >> 16) & 0xFF) / 255.0;
    b = ((value >> 8) & 0xFF) / 255.0;
    a = (value & 0xFF) / 255.0;
  } else {
    Logger::instance().warn("Invalid hex color: {}", hex);
    return Color(0, 0, 0, 1);
  }

  return Color(r, g, b, a);
}

Renderer::Renderer()
    : width_(0), height_(0), stride_(0), buffer_data_(nullptr), buffer_size_(0),
      surface_(nullptr), cr_(nullptr) {
}

Renderer::~Renderer() {
  if (cr_) {
    cairo_destroy(cr_);
  }
  if (surface_) {
    cairo_surface_destroy(surface_);
  }
  if (buffer_data_) {
    delete[] buffer_data_;
  }
}

bool Renderer::initialize(uint32_t width, uint32_t height) {
  width_ = width;
  height_ = height;
  stride_ = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
  buffer_size_ = stride_ * height;

  // Allocate buffer
  buffer_data_ = new uint8_t[buffer_size_];
  std::memset(buffer_data_, 0, buffer_size_);

  // Create Cairo surface
  surface_ = cairo_image_surface_create_for_data(
      buffer_data_, CAIRO_FORMAT_ARGB32, width, height, stride_);

  if (cairo_surface_status(surface_) != CAIRO_STATUS_SUCCESS) {
    Logger::instance().error("Failed to create Cairo surface");
    return false;
  }

  // Create Cairo context
  cr_ = cairo_create(surface_);
  if (cairo_status(cr_) != CAIRO_STATUS_SUCCESS) {
    Logger::instance().error("Failed to create Cairo context");
    return false;
  }

  Logger::instance().info("Renderer initialized: {}x{}", width, height);
  return true;
}

void Renderer::begin_frame() {
  // Clear to transparent black by default
  cairo_save(cr_);
  cairo_set_operator(cr_, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr_);
  cairo_restore(cr_);
}

void Renderer::end_frame() {
  cairo_surface_flush(surface_);
}

void Renderer::clear(const Color& color) {
  cairo_save(cr_);
  cairo_set_source_rgba(cr_, color.r, color.g, color.b, color.a);
  cairo_set_operator(cr_, CAIRO_OPERATOR_SOURCE);
  cairo_paint(cr_);
  cairo_restore(cr_);
}

void Renderer::fill_rect(double x, double y, double w, double h,
                         const Color& color) {
  cairo_save(cr_);
  cairo_set_source_rgba(cr_, color.r, color.g, color.b, color.a);
  cairo_rectangle(cr_, x, y, w, h);
  cairo_fill(cr_);
  cairo_restore(cr_);
}

// Helper: build a rounded-rect path in the current Cairo context.
// r is clamped to half the smaller dimension so corners never overdraw.
static void rounded_rect_path(cairo_t* cr, double x, double y, double w,
                              double h, double r) {
  r = std::min(r, std::min(w, h) / 2.0);
  if (r <= 0.0) {
    cairo_rectangle(cr, x, y, w, h);
    return;
  }
  const double pi = M_PI;
  cairo_new_sub_path(cr);
  cairo_arc(cr, x + w - r, y + r, r, -pi / 2, 0);    // top-right
  cairo_arc(cr, x + w - r, y + h - r, r, 0, pi / 2); // bottom-right
  cairo_arc(cr, x + r, y + h - r, r, pi / 2, pi);    // bottom-left
  cairo_arc(cr, x + r, y + r, r, pi, 3 * pi / 2);    // top-left
  cairo_close_path(cr);
}

void Renderer::fill_rounded_rect(double x, double y, double w, double h,
                                 double radius, const Color& color) {
  cairo_save(cr_);
  cairo_set_source_rgba(cr_, color.r, color.g, color.b, color.a);
  rounded_rect_path(cr_, x, y, w, h, radius);
  cairo_fill(cr_);
  cairo_restore(cr_);
}

void Renderer::stroke_rounded_rect(double x, double y, double w, double h,
                                   double radius, double line_w,
                                   const Color& color) {
  cairo_save(cr_);
  cairo_set_source_rgba(cr_, color.r, color.g, color.b, color.a);
  cairo_set_line_width(cr_, line_w);
  // Inset by half the line width so the stroke lands inside the rect bounds
  double inset = line_w / 2.0;
  rounded_rect_path(cr_, x + inset, y + inset, w - line_w, h - line_w,
                    radius - inset);
  cairo_stroke(cr_);
  cairo_restore(cr_);
}

void Renderer::draw_text(const std::string& text, double x, double y,
                         const std::string& font, double size,
                         const Color& color) {
  if (text.empty()) {
    Logger::instance().warn("Empty text at ({}, {})", x, y);
    return;
  }

  Logger::instance().debug("draw_text('{}', {}, {}, size={})", text, x, y,
                           size);

  cairo_save(cr_);
  cairo_set_source_rgba(cr_, color.r, color.g, color.b, color.a);

  // Create Pango layout for proper emoji/font rendering
  PangoLayout* layout = pango_cairo_create_layout(cr_);

  // Set font description
  PangoFontDescription* desc = pango_font_description_new();
  pango_font_description_set_family(desc, font.c_str());
  pango_font_description_set_size(desc, size * PANGO_SCALE);
  pango_layout_set_font_description(layout, desc);

  // Set text
  pango_layout_set_text(layout, text.c_str(), -1);

  // Get baseline position to match Cairo toy API behavior
  PangoLayoutIter* iter = pango_layout_get_iter(layout);
  int baseline = pango_layout_iter_get_baseline(iter);
  pango_layout_iter_free(iter);

  // Adjust y to account for baseline (Pango uses top-left, Cairo used baseline)
  double adjusted_y = y - (baseline / (double)PANGO_SCALE);

  // Render at position
  cairo_move_to(cr_, x, adjusted_y);
  pango_cairo_show_layout(cr_, layout);

  // Cleanup
  pango_font_description_free(desc);
  g_object_unref(layout);

  cairo_restore(cr_);
}

int Renderer::measure_text_width(const std::string& text,
                                 const std::string& font, double size) const {
  if (text.empty()) {
    return 0;
  }

  // Create temporary Pango layout for measurement
  PangoLayout* layout = pango_cairo_create_layout(cr_);

  // Set font description
  PangoFontDescription* desc = pango_font_description_new();
  pango_font_description_set_family(desc, font.c_str());
  pango_font_description_set_size(desc, size * PANGO_SCALE);
  pango_layout_set_font_description(layout, desc);

  // Set text
  pango_layout_set_text(layout, text.c_str(), -1);

  // Get actual pixel width
  int width, height;
  pango_layout_get_pixel_size(layout, &width, &height);

  // Cleanup
  pango_font_description_free(desc);
  g_object_unref(layout);

  return width;
}

void Renderer::draw_surface(cairo_surface_t* source, double x, double y,
                            double width, double height) {
  if (!source)
    return;

  cairo_save(cr_);

  // Get source dimensions
  int src_width = cairo_image_surface_get_width(source);
  int src_height = cairo_image_surface_get_height(source);

  // Calculate scaling
  double scale_x = width / src_width;
  double scale_y = height / src_height;

  // Position and scale
  cairo_translate(cr_, x, y);
  cairo_scale(cr_, scale_x, scale_y);

  // Draw
  cairo_set_source_surface(cr_, source, 0, 0);
  cairo_paint(cr_);

  cairo_restore(cr_);
}

} // namespace hyprbar
