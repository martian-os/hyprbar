#pragma once

#include "widget.h"
#include <vector>
#include <memory>
#include <string>

namespace hyprbar {

class ConfigManager;
class Renderer;

/**
 * WidgetManager - Manages widget lifecycle and rendering
 * 
 * Responsibilities:
 * - Create widgets from configuration
 * - Update widgets periodically
 * - Layout widgets horizontally
 * - Render all widgets
 * - Route input events to widgets
 */
class WidgetManager {
public:
    WidgetManager() = default;
    ~WidgetManager() = default;

    /**
     * Initialize widgets from configuration
     * @param config_mgr Configuration manager
     * @return true on success
     */
    bool initialize(const ConfigManager& config_mgr);

    /**
     * Update all widgets
     * @return true if any widget needs redraw
     */
    bool update();

    /**
     * Render all widgets
     * @param renderer Renderer to use
     * @param bar_width Total bar width
     * @param bar_height Total bar height
     */
    void render(Renderer& renderer, int bar_width, int bar_height);

    /**
     * Handle pointer click
     * @param x Click X position
     * @param y Click Y position
     * @param button Button clicked
     */
    void on_click(int x, int y, uint32_t button);

private:
    std::unique_ptr<Widget> create_widget(const std::string& type);
    
    struct WidgetSlot {
        std::unique_ptr<Widget> widget;
        int x;
        int y;
        int width;
        int height;
    };

    std::vector<WidgetSlot> widgets_;
};

}  // namespace hyprbar
