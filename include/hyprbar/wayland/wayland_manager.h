#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <wayland-client.h>

// Forward declare layer shell types
struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;

namespace hyprbar {

/**
 * WaylandManager - Manages Wayland connection and protocols
 *
 * Handles display connection, registry, compositor, shm, layer shell, and
 * input.
 */
class WaylandManager {
public:
  enum class BarPosition { Top, Bottom, Left, Right };

  using PointerButtonCallback = std::function<void(
      uint32_t button, uint32_t state, int32_t x, int32_t y)>;
  using PointerMotionCallback = std::function<void(int32_t x, int32_t y)>;

  WaylandManager();
  ~WaylandManager();

  // Non-copyable
  WaylandManager(const WaylandManager &) = delete;
  WaylandManager &operator=(const WaylandManager &) = delete;

  /**
   * Initialize Wayland connection
   * @return true on success
   */
  bool initialize();

  /**
   * Create bar surface with layer shell
   * @param position Bar position (top/bottom/left/right)
   * @param width Surface width
   * @param height Surface height
   * @return true on success
   */
  bool create_bar_surface(BarPosition position, uint32_t width,
                          uint32_t height);

  /**
   * Set exclusive zone (pushes windows away from bar)
   * @param size Zone size in pixels
   */
  void set_exclusive_zone(uint32_t size);

  /**
   * Get Wayland display file descriptor for event loop
   */
  int get_fd() const;

  /**
   * Dispatch pending Wayland events
   * @return 0 on success, -1 on error
   */
  int dispatch();

  /**
   * Prepare for reading events
   */
  int prepare_read();

  /**
   * Flush outgoing requests
   */
  int flush();

  /**
   * Read events after prepare_read()
   */
  int read_events();

  /**
   * Dispatch pending events
   */
  int dispatch_pending();

  /**
   * Get surface for rendering
   */
  wl_surface *get_surface() const { return surface_; }

  /**
   * Create shared memory buffer
   * @param size Buffer size in bytes
   * @return wl_buffer or nullptr on failure
   */
  wl_buffer *create_buffer(size_t size, void **data);

  /**
   * Attach buffer and commit surface
   * @param buffer Buffer to attach
   */
  void attach_and_commit(wl_buffer *buffer);

  /**
   * Set pointer button callback
   */
  void set_pointer_button_callback(PointerButtonCallback callback) {
    pointer_button_callback_ = callback;
  }

  /**
   * Set pointer motion callback
   */
  void set_pointer_motion_callback(PointerMotionCallback callback) {
    pointer_motion_callback_ = callback;
  }

  /**
   * Check if initialization was successful
   */
  bool is_initialized() const { return display_ != nullptr; }

private:
  void cleanup();
  uint32_t calculate_anchor(BarPosition position) const;
  void configure_layer_surface(BarPosition position, uint32_t width,
                               uint32_t height);
  // Wayland objects
  wl_display *display_;
  wl_registry *registry_;
  wl_compositor *compositor_;
  wl_shm *shm_;
  wl_seat *seat_;
  wl_pointer *pointer_;
  wl_surface *surface_;

  // Layer shell objects
  zwlr_layer_shell_v1 *layer_shell_;
  zwlr_layer_surface_v1 *layer_surface_;

  // Callbacks
  PointerButtonCallback pointer_button_callback_;
  PointerMotionCallback pointer_motion_callback_;

  // Current pointer position
  int32_t pointer_x_;
  int32_t pointer_y_;

public:
  // Static callback handlers (must be public for C linkage)
  static void registry_handle_global(void *data, wl_registry *registry,
                                     uint32_t name, const char *interface,
                                     uint32_t version);
  static void registry_handle_global_remove(void *data, wl_registry *registry,
                                            uint32_t name);

  static void pointer_handle_enter(void *data, wl_pointer *pointer,
                                   uint32_t serial, wl_surface *surface,
                                   wl_fixed_t x, wl_fixed_t y);
  static void pointer_handle_leave(void *data, wl_pointer *pointer,
                                   uint32_t serial, wl_surface *surface);
  static void pointer_handle_motion(void *data, wl_pointer *pointer,
                                    uint32_t time, wl_fixed_t x, wl_fixed_t y);
  static void pointer_handle_button(void *data, wl_pointer *pointer,
                                    uint32_t serial, uint32_t time,
                                    uint32_t button, uint32_t state);
  static void pointer_handle_axis(void *data, wl_pointer *pointer,
                                  uint32_t time, uint32_t axis,
                                  wl_fixed_t value);
  static void pointer_handle_frame(void *data, wl_pointer *pointer);
  static void pointer_handle_axis_source(void *data, wl_pointer *pointer,
                                         uint32_t source);
  static void pointer_handle_axis_stop(void *data, wl_pointer *pointer,
                                       uint32_t time, uint32_t axis);
  static void pointer_handle_axis_discrete(void *data, wl_pointer *pointer,
                                           uint32_t axis, int32_t discrete);

  static void seat_handle_capabilities(void *data, wl_seat *seat,
                                       uint32_t capabilities);
  static void seat_handle_name(void *data, wl_seat *seat, const char *name);

  static void layer_surface_handle_configure(void *data,
                                             zwlr_layer_surface_v1 *surface,
                                             uint32_t serial, uint32_t width,
                                             uint32_t height);
  static void layer_surface_handle_closed(void *data,
                                          zwlr_layer_surface_v1 *surface);
};

} // namespace hyprbar
