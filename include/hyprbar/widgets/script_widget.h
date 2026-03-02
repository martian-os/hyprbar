#pragma once

#include "widget.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

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
 * The widget executes the command periodically in a background thread
 * and displays the last line of stdout. This allows users to write widgets
 * in any language (bash, ruby, go, python, etc.)
 *
 * Thread-safe: Commands run in background, output is synchronized.
 */
class ScriptWidget : public Widget {
public:
  ScriptWidget();
  ~ScriptWidget() override;

  bool initialize(const ConfigValue& config) override;
  bool update() override;
  void render(Renderer& renderer, int x, int y, int width, int height) override;
  int get_desired_width() const override;
  int get_desired_height() const override;
  std::string get_type() const override {
    return "script";
  }

  /**
   * Measure and cache text width using renderer (call before layout)
   */
  void measure_width(Renderer& renderer);

  /**
   * Check if widget has produced output yet
   */
  bool has_output() const;

private:
  void worker_thread();
  std::string execute_command();

  std::string command_;
  int interval_ms_{1000};
  std::string font_{"monospace"};
  double font_size_{14.0};
  std::string color_{"#000000"}; // Black default

  // Thread-safe state
  mutable std::mutex output_mutex_;
  std::string last_output_;
  mutable int cached_width_{20}; // Cached measured width
  std::atomic<bool> output_changed_{false};
  std::atomic<bool> running_{false};
  std::thread worker_;
};

} // namespace hyprbar
