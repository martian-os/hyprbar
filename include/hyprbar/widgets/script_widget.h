#pragma once

#include "widget.h"
#include <string>
#include <chrono>

namespace hyprbar {

/**
 * ScriptWidget - Executes a command and displays last line of output
 * 
 * Configuration:
 * - command: Command to execute (required)
 * - interval: Update interval in ms (default: 1000)
 * - font: Font name (default: "monospace")
 * - size: Font size (default: 14)
 * - color: Text color hex (default: "#ffffff")
 * 
 * The widget executes the command periodically and displays
 * the last line of stdout. This allows users to write widgets
 * in any language (bash, ruby, go, python, etc.)
 */
class ScriptWidget : public Widget {
public:
    ScriptWidget() = default;
    ~ScriptWidget() override = default;

    bool initialize(const ConfigValue& config) override;
    bool update() override;
    void render(Renderer& renderer, int x, int y, int width, int height) override;
    int get_desired_width() const override;
    int get_desired_height() const override;
    std::string get_type() const override { return "script"; }

private:
    std::string execute_command();

    std::string command_;
    int interval_ms_{1000};
    std::string font_{"monospace"};
    double font_size_{14.0};
    std::string color_{"#000000"};  // Black default
    std::string last_output_;
    std::chrono::steady_clock::time_point last_update_;
};

}  // namespace hyprbar
