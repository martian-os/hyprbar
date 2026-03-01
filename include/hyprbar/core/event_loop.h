#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <vector>

namespace hyprbar {

using TimePoint = std::chrono::steady_clock::time_point;
using Duration = std::chrono::milliseconds;

/**
 * EventHandler - Callback for file descriptor events
 * @param fd The file descriptor that's ready
 * @param events Bitmask of ready events (EPOLLIN, EPOLLOUT, etc.)
 */
using EventHandler = std::function<void(int fd, uint32_t events)>;

/**
 * TimerCallback - Callback for timer expiration
 */
using TimerCallback = std::function<void()>;

/**
 * EventLoop - epoll-based event dispatcher
 *
 * Manages file descriptors (Wayland, IPC, etc.) and timers.
 * Single-threaded, runs in main thread.
 */
class EventLoop {
public:
  EventLoop();
  ~EventLoop();

  // Non-copyable
  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;

  /**
   * Add a file descriptor to monitor
   * @param fd File descriptor to monitor
   * @param events Events to monitor (EPOLLIN, EPOLLOUT, etc.)
   * @param handler Callback when events occur
   * @return true on success
   */
  bool add_fd(int fd, uint32_t events, EventHandler handler);

  /**
   * Remove a file descriptor from monitoring
   * @param fd File descriptor to remove
   */
  void remove_fd(int fd);

  /**
   * Add a repeating timer
   * @param interval Time between timer fires
   * @param callback Function to call on timer expiration
   * @return Timer ID (for cancellation)
   */
  int add_timer(Duration interval, TimerCallback callback);

  /**
   * Add a one-shot timer
   * @param delay Time until timer fires
   * @param callback Function to call on timer expiration
   * @return Timer ID (for cancellation)
   */
  int add_timer_once(Duration delay, TimerCallback callback);

  /**
   * Cancel a timer
   * @param timer_id ID returned from add_timer
   */
  void cancel_timer(int timer_id);

  /**
   * Run one iteration of the event loop
   * Processes ready file descriptors and expired timers.
   * @param timeout_ms Maximum time to wait for events (-1 = infinite)
   * @return true if should continue, false if shutdown requested
   */
  bool dispatch(int timeout_ms = -1);

  /**
   * Request event loop shutdown
   * dispatch() will return false on next iteration
   */
  void shutdown();

  /**
   * Check if shutdown has been requested
   */
  bool is_shutdown_requested() const {
    return shutdown_requested_;
  }

private:
  struct Timer {
    int id;
    TimePoint next_expiry;
    Duration interval;
    TimerCallback callback;
    bool repeating;
    bool cancelled;
  };

  struct FdHandler {
    int fd;
    uint32_t events;
    EventHandler handler;
  };

  void process_timers();
  void handle_expired_timer(Timer &timer, TimePoint now);
  void remove_cancelled_timers();
  int get_next_timer_timeout() const;

  int epoll_fd_;
  std::vector<FdHandler> handlers_;
  std::vector<Timer> timers_;
  int next_timer_id_;
  bool shutdown_requested_;
};

} // namespace hyprbar
