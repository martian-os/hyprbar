#include "hyprbar/rendering/file_surface.h"
#include "hyprbar/core/logger.h"
#include <cstring>

namespace hyprbar {

FileSurface::FileSurface(const std::string& output_path)
    : output_path_(output_path) {
}

FileSurface::~FileSurface() {
    if (surface_) {
        cairo_surface_destroy(surface_);
    }
    delete[] buffer_data_;
}

bool FileSurface::initialize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    // Allocate buffer
    size_t buffer_size = width * height * 4;  // RGBA
    buffer_data_ = new uint8_t[buffer_size];
    std::memset(buffer_data_, 0, buffer_size);

    // Create Cairo surface from buffer
    surface_ = cairo_image_surface_create_for_data(
        buffer_data_,
        CAIRO_FORMAT_RGB24,
        width,
        height,
        width * 4  // stride
    );

    if (cairo_surface_status(surface_) != CAIRO_STATUS_SUCCESS) {
        Logger::instance().error("Failed to create Cairo surface");
        return false;
    }

    Logger::instance().info("File surface initialized: {}x{}", width, height);
    return true;
}

void* FileSurface::get_buffer_data() {
    return buffer_data_;
}

size_t FileSurface::get_buffer_size() const {
    return width_ * height_ * 4;
}

void FileSurface::present() {
    if (presented_) {
        return;  // Only save once
    }

    // Flush any pending operations
    cairo_surface_flush(surface_);
    
    // Save to PNG
    cairo_surface_write_to_png(surface_, output_path_.c_str());
    Logger::instance().info("Screenshot saved to: {}", output_path_);
    presented_ = true;
}

bool FileSurface::process_events() {
    // File surface runs once and exits
    return false;
}

}  // namespace hyprbar
