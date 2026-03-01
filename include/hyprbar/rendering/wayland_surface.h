#pragma once

#include "hyprbar/wayland/wayland_manager.h"
#include "surface.h"
#include <memory>

namespace hyprbar {

/**
 * WaylandSurface - Renders to Wayland compositor
 *
 * Integrates with WaylandManager for real compositor output.
 */
class WaylandSurface : public Surface {
public:
  WaylandSurface(WaylandManager *wayland_mgr, wl_buffer *buffer);
  ~WaylandSurface() override = default;

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
  WaylandManager *wayland_mgr_;
  wl_buffer *buffer_;
  void *buffer_data_;
  uint32_t width_{0};
  uint32_t height_{0};
};

} // namespace hyprbar
