#include "hyprbar/widgets/widget_manager.h"
#include "hyprbar/widgets/clock_widget.h"
#include "hyprbar/widgets/script_widget.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/rendering/renderer.h"

namespace hyprbar {

std::unique_ptr<Widget> WidgetManager::create_widget(const std::string& type) {
    if (type == "clock") {
        return std::make_unique<ClockWidget>();
    }
    
    if (type == "script") {
        return std::make_unique<ScriptWidget>();
    }
    
    Logger::instance().warn("Unknown widget type: {}", type);
    return nullptr;
}

bool WidgetManager::initialize(const ConfigManager& config_mgr) {
    const auto& config = config_mgr.get_config();
    
    for (const auto& widget_config : config.widgets) {
        auto widget = create_widget(widget_config.type);
        if (!widget) {
            continue;
        }
        if (!widget->initialize(widget_config.config)) {
            Logger::instance().error("Failed to initialize {} widget", widget_config.type);
            continue;
        }

        WidgetSlot slot;
        slot.widget = std::move(widget);
        slot.x = 0;
        slot.y = 0;
        slot.width = 0;
        slot.height = 0;
        widgets_.push_back(std::move(slot));
    }

    Logger::instance().info("Initialized {} widgets", widgets_.size());
    return !widgets_.empty();
}

bool WidgetManager::update() {
    bool needs_redraw = false;
    for (auto& slot : widgets_) {
        if (slot.widget->update()) {
            needs_redraw = true;
        }
    }
    return needs_redraw;
}

void WidgetManager::render(Renderer& renderer, int bar_width, int bar_height) {
    if (widgets_.empty()) {
        return;
    }

    // Simple left-to-right layout with 10px spacing
    int x = 10;
    const int spacing = 10;

    for (auto& slot : widgets_) {
        int widget_width = slot.widget->get_desired_width();
        int widget_height = slot.widget->get_desired_height();
        
        if (widget_height == 0) {
            widget_height = bar_height;
        }

        slot.x = x;
        slot.y = 0;
        slot.width = widget_width;
        slot.height = widget_height;

        slot.widget->render(renderer, x, 0, widget_width, bar_height);
        
        x += widget_width + spacing;
    }
}

void WidgetManager::on_click(int x, int y, uint32_t button) {
    for (auto& slot : widgets_) {
        if (x >= slot.x && x < slot.x + slot.width &&
            y >= slot.y && y < slot.y + slot.height) {
            int local_x = x - slot.x;
            int local_y = y - slot.y;
            slot.widget->on_click(local_x, local_y, button);
            break;
        }
    }
}

}  // namespace hyprbar
