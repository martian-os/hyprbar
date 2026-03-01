#pragma once

#include "widget.h"
#include <string>
#include <chrono>

namespace hyprbar {

/**
 * ClockWidget - Displays current time
 * 
 * Configuration:
 * - format: Time format string (strftime format, default: "%H:%M:%S")
 * - font: Font name (default: "monospace")
 * - size: Font size (default: 14)
 */
class ClockWidget : public Widget {
public:
    ClockWidget() = default;
    ~ClockWidget() override = default;

    bool initialize(const ConfigValue& config) override;
    bool update() override;
    void render(Renderer& renderer, int x, int y, int width, int height) override;
    int get_desired_width() const override;
    int get_desired_height() const override;
    std::string get_type() const override { return "clock"; }

private:
    std::string format_time() const;

    std::string format_{"% H:%M:%S"};
    std::string font_{"monospace"};
    double font_size_{14.0};
    std::string color_{"#000000"};  // Black default for visibility
    std::string current_time_;
    int cached_width_{0};
};

}  // namespace hyprbar
