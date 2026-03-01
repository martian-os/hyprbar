#pragma once

#include "surface.h"
#include <cairo/cairo.h>
#include <memory>
#include <string>

namespace hyprbar {

/**
 * FileSurface - Renders to PNG file
 *
 * For screenshot generation without compositor.
 * Renders one frame and exits.
 */
class FileSurface : public Surface {
public:
  FileSurface(const std::string &output_path);
  ~FileSurface() override;

  bool initialize(uint32_t width, uint32_t height) override;
  void *get_buffer_data() override;
  size_t get_buffer_size() const override;
  void present() override;
  uint32_t get_width() const override {
    return width_;
  }
  uint32_t get_height() const override {
    return height_;
  }
  bool process_events() override;

private:
  std::string output_path_;
  uint32_t width_{0};
  uint32_t height_{0};
  cairo_surface_t *surface_{nullptr};
  uint8_t *buffer_data_{nullptr};
  bool presented_{false};
};

} // namespace hyprbar
