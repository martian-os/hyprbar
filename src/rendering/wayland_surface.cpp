#include "hyprbar/rendering/wayland_surface.h"
#include "hyprbar/core/logger.h"

namespace hyprbar {

WaylandSurface::WaylandSurface(WaylandManager* wayland_mgr, wl_buffer* buffer)
    : wayland_mgr_(wayland_mgr)
    , buffer_(buffer)
    , buffer_data_(nullptr) {
}

bool WaylandSurface::initialize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    Logger::instance().info("Wayland surface initialized: {}x{}", width, height);
    return true;
}

void* WaylandSurface::get_buffer_data() {
    return buffer_data_;
}

size_t WaylandSurface::get_buffer_size() const {
    return width_ * height_ * 4;
}

void WaylandSurface::present() {
    if (wayland_mgr_ && buffer_) {
        wayland_mgr_->attach_and_commit(buffer_);
    }
}

bool WaylandSurface::process_events() {
    if (!wayland_mgr_) {
        return false;
    }

    while (wayland_mgr_->prepare_read() != 0) {
        wayland_mgr_->dispatch_pending();
    }
    wayland_mgr_->flush();
    wayland_mgr_->read_events();
    wayland_mgr_->dispatch_pending();

    return true;
}

}  // namespace hyprbar
