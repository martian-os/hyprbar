#include "hyprbar/widgets/clock_widget.h"
#include "hyprbar/rendering/renderer.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/logger.h"
#include <ctime>

namespace hyprbar {

bool ClockWidget::initialize(const ConfigValue& config) {
    if (config.type != ConfigValue::Type::Object) {
        return true;  // Use defaults
    }

    const auto& obj = config.object_value;

    if (obj.count("format")) {
        format_ = obj.at("format").string_value;
    }

    if (obj.count("font")) {
        font_ = obj.at("font").string_value;
    }

    if (obj.count("size")) {
        font_size_ = obj.at("size").double_value;
    }

    if (obj.count("color")) {
        color_ = obj.at("color").string_value;
    }

    current_time_ = format_time();
    Logger::instance().debug("Clock widget initialized");
    return true;
}

std::string ClockWidget::format_time() const {
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char buffer[128];
    strftime(buffer, sizeof(buffer), format_.c_str(), tm_info);
    return std::string(buffer);
}

bool ClockWidget::update() {
    std::string new_time = format_time();
    if (new_time != current_time_) {
        current_time_ = new_time;
        return true;  // Needs redraw
    }
    return false;
}

void ClockWidget::render(Renderer& renderer, int x, int y, 
                        int /*width*/, int height) {
    Color fg = Color::from_hex(color_);
    double text_y = y + (height / 2.0) + (font_size_ / 3.0);
    renderer.draw_text(current_time_, x, text_y, font_, font_size_, fg);
}

int ClockWidget::get_desired_width() const {
    // Estimate: ~10 pixels per character for monospace
    return static_cast<int>(current_time_.length() * 10);
}

int ClockWidget::get_desired_height() const {
    return 0;  // Flexible height
}

}  // namespace hyprbar
