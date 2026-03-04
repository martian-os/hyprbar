#pragma once

#include "widget.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace hyprbar {

struct ConfigValue;

/**
 * HyprlandWidget - Display Hyprland workspaces
 *
 * Shows workspace buttons with indicators for:
 * - Active workspace (highlighted)
 * - Occupied workspaces (has windows)
 * - Empty workspaces (dimmed)
 *
 * Communicates with Hyprland via IPC socket.
 */
class HyprlandWidget : public Widget {
public:
  HyprlandWidget();
  ~HyprlandWidget() override;

  bool initialize(const ConfigValue& config) override;
  bool update() override;
  void render(Renderer& renderer, int x, int y, int width, int height) override;
  int get_desired_width() const noexcept override;
  int get_desired_height() const noexcept override;
  std::string get_type() const override {
    return "hyprland";
  }

private:
  struct Workspace {
    int id;
    std::string name;
    bool active;
    int windows;
  };

  void event_listener_thread();
  void fetch_workspaces();
  std::string send_hyprland_command(const std::string& command);

  std::string font_ = "monospace";
  double font_size_ = 14.0;
  std::string active_color_ = "#89b4fa";
  std::string occupied_color_ = "#cdd6f4";
  std::string empty_color_ = "#45475a";
  int max_workspaces_ = 10;
  int width_ = 30; // Width of each workspace button (CSS: width)
  int gap_ = 5;    // Spacing between workspace buttons (CSS: gap)

  std::vector<Workspace> workspaces_;
  mutable std::mutex workspaces_mutex_;

  std::thread event_thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> workspace_changed_{false};
};

} // namespace hyprbar
