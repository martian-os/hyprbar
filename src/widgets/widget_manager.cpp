#include "hyprbar/widgets/widget_manager.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/rendering/renderer.h"
#include "hyprbar/widgets/script_widget.h"

namespace hyprbar {

std::unique_ptr<Widget> WidgetManager::create_widget(const std::string& type) {
  if (type == "script") {
    return std::make_unique<ScriptWidget>();
  }

  Logger::instance().warn("Unknown widget type: {}", type);
  return nullptr;
}

bool WidgetManager::initialize(const ConfigManager& config_mgr) {
  const auto& config = config_mgr.get_config();

  // Parse bar font for defaults (e.g., "Noto Sans, Noto Color Emoji 14")
  std::string bar_font = config.bar.font;
  std::string default_font_family = "monospace";
  double default_font_size = 14.0;

  // Extract font family and size from bar font string
  size_t last_space = bar_font.rfind(' ');
  if (last_space != std::string::npos) {
    std::string size_str = bar_font.substr(last_space + 1);
    try {
      default_font_size = std::stod(size_str);
      default_font_family = bar_font.substr(0, last_space);
    } catch (...) {
      // If parsing fails, treat whole string as font family
      default_font_family = bar_font;
    }
  } else {
    default_font_family = bar_font;
  }

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
        obj["color"] = ConfigValue(config.bar.foreground);
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

void WidgetManager::render(Renderer& renderer, int bar_width, int bar_height) {
  if (widgets_.empty()) {
    Logger::instance().warn("No widgets to render");
    return;
  }

  Logger::instance().debug("Rendering {} widgets", widgets_.size());

  const int spacing = 10;
  const int margin = 10;

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

  // Calculate total widths
  int left_total = 0;
  for (auto* slot : left_widgets) {
    left_total += slot->widget->get_desired_width() + spacing;
  }
  if (!left_widgets.empty())
    left_total -= spacing; // Remove last spacing

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
