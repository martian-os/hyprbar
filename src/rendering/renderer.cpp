#include "hyprbar/rendering/renderer.h"
#include "hyprbar/core/logger.h"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>

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
    : width_(0)
    , height_(0)
    , stride_(0)
    , buffer_data_(nullptr)
    , buffer_size_(0)
    , surface_(nullptr)
    , cr_(nullptr)
{
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
        buffer_data_,
        CAIRO_FORMAT_ARGB32,
        width,
        height,
        stride_
    );

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

void Renderer::fill_rect(double x, double y, double w, double h, const Color& color) {
    cairo_save(cr_);
    cairo_set_source_rgba(cr_, color.r, color.g, color.b, color.a);
    cairo_rectangle(cr_, x, y, w, h);
    cairo_fill(cr_);
    cairo_restore(cr_);
}

void Renderer::draw_text(const std::string& text, double x, double y,
                         const std::string& font, double size, const Color& color) {
    cairo_save(cr_);
    
    cairo_select_font_face(cr_, font.c_str(),
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr_, size);
    cairo_set_source_rgba(cr_, color.r, color.g, color.b, color.a);

    cairo_move_to(cr_, x, y);
    cairo_show_text(cr_, text.c_str());

    cairo_restore(cr_);
}

}  // namespace hyprbar
