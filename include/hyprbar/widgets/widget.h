#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace hyprbar {

// Forward declarations
class Renderer;
struct ConfigValue; // Changed from class to struct to match definition

/**
 * Widget - Base interface for all bar widgets
 *
 * Widgets are self-contained UI components that can:
 * - Render themselves using the Renderer
 * - Update their state periodically
 * - Handle input events
 * - Configure themselves from config
 */
class Widget {
public:
  virtual ~Widget() = default;

  /**
   * Initialize widget with configuration
   * @param config Widget-specific configuration
   * @return true on success
   */
  virtual bool initialize(const ConfigValue& config) = 0;

  /**
   * Update widget state (called periodically)
   * @return true if widget needs redraw
   */
  virtual bool update() = 0;

  /**
   * Render widget at given position
   * @param renderer Renderer to draw with
   * @param x X position
   * @param y Y position
   * @param width Available width
   * @param height Available height
   */
  virtual void render(Renderer& renderer, int x, int y, int width,
                      int height) = 0;

  /**
   * Get desired width of widget (0 = flexible)
   * @return Desired width in pixels
   */
  virtual int get_desired_width() const noexcept = 0;

  /**
   * Get desired height of widget (0 = flexible)
   * @return Desired height in pixels
   */
  virtual int get_desired_height() const noexcept = 0;

  /**
   * Get widget type name
   */
  virtual std::string get_type() const = 0;

  /**
   * Handle pointer click
   * @param x X position relative to widget
   * @param y Y position relative to widget
   * @param button Button that was clicked
   */
  virtual void on_click(int x, int y, uint32_t button) noexcept {
    (void)x;
    (void)y;
    (void)button;
    // Default: no-op
  }
};

} // namespace hyprbar
