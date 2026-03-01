#pragma once

#include <cstdint>
#include <string>

namespace hyprbar {

/**
 * Surface - Abstract rendering target
 *
 * Allows switching between Wayland compositor output
 * and file-based screenshot generation.
 */
class Surface {
public:
  virtual ~Surface() = default;

  /**
   * Initialize the surface
   * @param width Surface width
   * @param height Surface height
   * @return true on success
   */
  virtual bool initialize(uint32_t width, uint32_t height) = 0;

  /**
   * Get buffer data for rendering
   * @return Pointer to buffer data
   */
  virtual void *get_buffer_data() = 0;

  /**
   * Get buffer size in bytes
   * @return Buffer size
   */
  virtual size_t get_buffer_size() const = 0;

  /**
   * Present/commit the rendered frame
   */
  virtual void present() = 0;

  /**
   * Get surface width
   */
  virtual uint32_t get_width() const = 0;

  /**
   * Get surface height
   */
  virtual uint32_t get_height() const = 0;

  /**
   * Process events (for Wayland, no-op for file)
   * @return true to continue running
   */
  virtual bool process_events() = 0;
};

} // namespace hyprbar
