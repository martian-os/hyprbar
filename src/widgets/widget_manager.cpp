#include "hyprbar/widgets/widget_manager.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/rendering/renderer.h"
#include "hyprbar/widgets/hyprland_widget.h"
#include "hyprbar/widgets/script_widget.h"
#include "hyprbar/widgets/tray_widget.h"
#include <chrono>
#include <thread>

namespace hyprbar {

std::unique_ptr<Widget> WidgetManager::create_widget(const std::string& type) {
  if (type == "script") {
    return std::make_unique<ScriptWidget>();
  }
  if (type == "hyprland") {
    return std::make_unique<HyprlandWidget>();
  }
  if (type == "tray") {
    return std::make_unique<TrayWidget>();
  }

  Logger::instance().warn("Unknown widget type: {}", type);
  return nullptr;
}

bool WidgetManager::initialize(const ConfigManager& config_mgr) {
  const auto& config = config_mgr.get_config();

  // Get bar layout config
  spacing_ = config.bar.spacing;
  margin_ = config.bar.margin;

  // Get bar defaults for widget inheritance
  std::string default_font_family = config.bar.font;
  double default_font_size = config.bar.size;

  for (const auto& widget_config : config.widgets) {
    auto widget = create_widget(widget_config.type);
    if (!widget) {
      continue;
    }

    // For script widgets, resolve command path relative to config
    ConfigValue resolved_config = widget_config.config;
    if (widget_config.type == "script" &&
        resolved_config.type == ConfigValue::Type::Object) {
      auto& obj = resolved_config.object_value;
      if (obj.count("command")) {
        std::string command = obj.at("command").string_value;
        std::string resolved = config_mgr.resolve_path(command);
        obj["command"] = ConfigValue(resolved);
      }

      // Apply bar defaults if not specified in widget config
      if (!obj.count("font")) {
        obj["font"] = ConfigValue(default_font_family);
      }
      if (!obj.count("size")) {
        obj["size"] = ConfigValue(default_font_size);
      }
      if (!obj.count("color")) {
        obj["color"] = ConfigValue(config.bar.color);
      }
    }

    if (!widget->initialize(resolved_config)) {
      Logger::instance().error("Failed to initialize {} widget",
                               widget_config.type);
      continue;
    }

    // Convert WidgetConfig::Position to WidgetManager::Position
    Position pos = Position::Left;
    switch (widget_config.position) {
    case WidgetConfig::Position::Left:
      pos = Position::Left;
      break;
    case WidgetConfig::Position::Center:
      pos = Position::Center;
      break;
    case WidgetConfig::Position::Right:
      pos = Position::Right;
      break;
    }

    WidgetSlot slot;
    slot.widget = std::move(widget);
    slot.position = pos;
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

bool WidgetManager::wait_for_ready(int timeout_ms) {
  auto start = std::chrono::steady_clock::now();
  const auto timeout = std::chrono::milliseconds(timeout_ms);

  while (true) {
    bool all_ready = true;

    // Check if all script widgets have output
    for (const auto& slot : widgets_) {
      if (slot.widget->get_type() == "script") {
        auto* script_widget =
            dynamic_cast<const ScriptWidget*>(slot.widget.get());
        if (script_widget && !script_widget->has_output()) {
          all_ready = false;
          break;
        }
      }
    }

    if (all_ready) {
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - start);
      Logger::instance().debug("All widgets ready after {}ms", elapsed.count());
      return true;
    }

    // Check timeout
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed >= timeout) {
      Logger::instance().warn("Timeout waiting for widgets to populate");
      return false;
    }

    // Small sleep to avoid busy-waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void WidgetManager::render(Renderer& renderer, int bar_width, int bar_height) {
  if (widgets_.empty()) {
    Logger::instance().warn("No widgets to render");
    return;
  }

  Logger::instance().debug("Rendering {} widgets", widgets_.size());

  const int spacing = spacing_;
  const int margin = margin_;

  // Separate widgets by position
  std::vector<WidgetSlot*> left_widgets;
  std::vector<WidgetSlot*> center_widgets;
  std::vector<WidgetSlot*> right_widgets;

  for (auto& slot : widgets_) {
    switch (slot.position) {
    case Position::Left:
      left_widgets.push_back(&slot);
      break;
    case Position::Center:
      center_widgets.push_back(&slot);
      break;
    case Position::Right:
      right_widgets.push_back(&slot);
      break;
    }
  }

  // Measure script widget widths before layout
  for (auto& slot : widgets_) {
    if (slot.widget->get_type() == "script") {
      auto* script_widget = dynamic_cast<ScriptWidget*>(slot.widget.get());
      if (script_widget) {
        script_widget->measure_width(renderer);
      }
    }
  }

  // Calculate total widths for center and right positioning
  // (left widgets don't need total, positioned incrementally)

  int center_total = 0;
  for (auto* slot : center_widgets) {
    center_total += slot->widget->get_desired_width() + spacing;
  }
  if (!center_widgets.empty())
    center_total -= spacing;

  int right_total = 0;
  for (auto* slot : right_widgets) {
    right_total += slot->widget->get_desired_width() + spacing;
  }
  if (!right_widgets.empty())
    right_total -= spacing;

  // Layout left widgets
  int x = margin;
  for (auto* slot : left_widgets) {
    int widget_width = slot->widget->get_desired_width();
    int widget_height = slot->widget->get_desired_height();
    if (widget_height == 0)
      widget_height = bar_height;

    slot->x = x;
    slot->y = 0;
    slot->width = widget_width;
    slot->height = widget_height;

    Logger::instance().debug("Left widget at x={}, width={}", x, widget_width);
    slot->widget->render(renderer, x, 0, widget_width, bar_height);

    x += widget_width + spacing;
  }

  // Layout center widgets
  if (!center_widgets.empty()) {
    x = (bar_width - center_total) / 2;
    for (auto* slot : center_widgets) {
      int widget_width = slot->widget->get_desired_width();
      int widget_height = slot->widget->get_desired_height();
      if (widget_height == 0)
        widget_height = bar_height;

      slot->x = x;
      slot->y = 0;
      slot->width = widget_width;
      slot->height = widget_height;

      Logger::instance().debug("Center widget at x={}, width={}", x,
                               widget_width);
      slot->widget->render(renderer, x, 0, widget_width, bar_height);

      x += widget_width + spacing;
    }
  }

  // Layout right widgets (from right to left)
  if (!right_widgets.empty()) {
    x = bar_width - margin - right_total;
    for (auto* slot : right_widgets) {
      int widget_width = slot->widget->get_desired_width();
      int widget_height = slot->widget->get_desired_height();
      if (widget_height == 0)
        widget_height = bar_height;

      slot->x = x;
      slot->y = 0;
      slot->width = widget_width;
      slot->height = widget_height;

      Logger::instance().debug("Right widget at x={}, width={}", x,
                               widget_width);
      slot->widget->render(renderer, x, 0, widget_width, bar_height);

      x += widget_width + spacing;
    }
  }

  Logger::instance().debug("Layout complete");
}

void WidgetManager::on_click(int x, int y, uint32_t button) {
  for (auto& slot : widgets_) {
    if (x >= slot.x && x < slot.x + slot.width && y >= slot.y &&
        y < slot.y + slot.height) {
      int local_x = x - slot.x;
      int local_y = y - slot.y;
      slot.widget->on_click(local_x, local_y, button);
      break;
    }
  }
}

} // namespace hyprbar
