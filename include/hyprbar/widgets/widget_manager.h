#pragma once

#include "hyprbar/core/config_manager.h"
#include "widget.h"
#include <memory>
#include <string>
#include <vector>

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
   * Wait for all script widgets to have initial output
   * Used in screenshot mode to ensure widgets are populated
   * @param timeout_ms Maximum time to wait (default 2000ms)
   * @return true if all widgets ready, false on timeout
   */
  bool wait_for_ready(int timeout_ms = 2000);

  /**
   * Handle pointer click
   * @param x Click X position
   * @param y Click Y position
   * @param button Button clicked
   */
  void on_click(int x, int y, uint32_t button);

private:
  std::unique_ptr<Widget> create_widget(const std::string& type);

  enum class Position { Left, Center, Right };

  struct WidgetSlot {
    std::unique_ptr<Widget> widget;
    Position position;
    int x;
    int y;
    int width;
    int height;
    WidgetStyle style; // CSS visual overrides drawn before widget content
  };

  std::vector<WidgetSlot> widgets_;
  int gap_ = 10;    // Widget spacing (from bar config, CSS: gap)
  int margin_ = 10; // Bar edge margin (from bar config, CSS: margin)
};

} // namespace hyprbar
