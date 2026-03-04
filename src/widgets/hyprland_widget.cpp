#include "hyprbar/widgets/hyprland_widget.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/rendering/renderer.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace hyprbar {

HyprlandWidget::HyprlandWidget() = default;

HyprlandWidget::~HyprlandWidget() {
  running_ = false;
  if (event_thread_.joinable()) {
    event_thread_.join();
  }
}

bool HyprlandWidget::initialize(const ConfigValue& config) {
  if (!config.is_object()) {
    Logger::instance().warn("Hyprland widget config must be object");
    return false;
  }

  const auto& obj = config.as_object();

  if (obj.count("font")) {
    font_ = obj.at("font").as_string();
  }

  if (obj.count("size")) {
    const auto& size_val = obj.at("size");
    if (size_val.type == ConfigValue::Type::Integer) {
      font_size_ = static_cast<double>(size_val.as_int());
    } else if (size_val.type == ConfigValue::Type::Double) {
      font_size_ = size_val.as_double();
    }
  }

  if (obj.count("active_color")) {
    active_color_ = obj.at("active_color").as_string();
  }

  if (obj.count("occupied_color")) {
    occupied_color_ = obj.at("occupied_color").as_string();
  }

  if (obj.count("empty_color")) {
    empty_color_ = obj.at("empty_color").as_string();
  }

  if (obj.count("max_workspaces")) {
    max_workspaces_ = static_cast<int>(obj.at("max_workspaces").as_int());
  }

  if (obj.count("button_width")) {
    button_width_ = static_cast<int>(obj.at("button_width").as_int());
  }

  if (obj.count("spacing")) {
    spacing_ = static_cast<int>(obj.at("spacing").as_int());
  }

  // Fetch initial workspace state
  fetch_workspaces();

  // Start event listener thread
  running_ = true;
  event_thread_ = std::thread(&HyprlandWidget::event_listener_thread, this);

  return true;
}

bool HyprlandWidget::update() {
  if (workspace_changed_.exchange(false)) {
    return true; // Request redraw
  }
  return false;
}

void HyprlandWidget::render(Renderer& renderer, int x, int y, int /*width*/,
                            int height) {
  std::lock_guard<std::mutex> lock(workspaces_mutex_);

  int current_x = x;

  for (const auto& ws : workspaces_) {
    // Choose color based on state
    std::string color;
    if (ws.active) {
      color = active_color_;
    } else if (ws.windows > 0) {
      color = occupied_color_;
    } else {
      color = empty_color_;
    }

    Color fg = Color::from_hex(color);
    double text_y = y + (height / 2.0) + (font_size_ / 3.0);

    renderer.draw_text(ws.name, current_x + 8, text_y, font_, font_size_, fg);

    current_x += button_width_ + spacing_;
  }
}

int HyprlandWidget::get_desired_width() const noexcept {
  std::lock_guard<std::mutex> lock(workspaces_mutex_);
  return static_cast<int>(workspaces_.size()) * (button_width_ + spacing_);
}

int HyprlandWidget::get_desired_height() const noexcept {
  return 0; // Flexible
}

void HyprlandWidget::event_listener_thread() {
  const char* his = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if (!his) {
    Logger::instance().error("HYPRLAND_INSTANCE_SIGNATURE not set");
    return;
  }

  std::string socket_path = std::string("/tmp/hypr/") + his + "/.socket2.sock";

  while (running_) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
      Logger::instance().error("Failed to create socket");
      std::this_thread::sleep_for(std::chrono::seconds(5));
      continue;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
      Logger::instance().error("Failed to connect to Hyprland socket");
      close(sock);
      std::this_thread::sleep_for(std::chrono::seconds(5));
      continue;
    }

    char buffer[4096];
    while (running_) {
      ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
      if (n <= 0) {
        break;
      }

      buffer[n] = '\0';
      std::string events(buffer);

      // Check for workspace events
      if (events.find("workspace>>") != std::string::npos ||
          events.find("createworkspace>>") != std::string::npos ||
          events.find("destroyworkspace>>") != std::string::npos ||
          events.find("moveworkspace>>") != std::string::npos) {
        fetch_workspaces();
        workspace_changed_ = true;
      }
    }

    close(sock);
  }
}

void HyprlandWidget::fetch_workspaces() {
  std::string response = send_hyprland_command("j/workspaces");
  if (response.empty()) {
    return;
  }

  // Parse JSON response (simple parsing for workspace data)
  std::vector<Workspace> new_workspaces;

  // Get active workspace
  std::string active_response = send_hyprland_command("j/activeworkspace");
  int active_id = 1;

  // Simple JSON parsing to extract "id": value
  size_t id_pos = active_response.find("\"id\":");
  if (id_pos != std::string::npos) {
    size_t start = id_pos + 5;
    size_t end = active_response.find_first_of(",}", start);
    if (end != std::string::npos) {
      active_id = std::stoi(active_response.substr(start, end - start));
    }
  }

  // Parse workspaces (extract id, name, windows count)
  for (int i = 1; i <= max_workspaces_; ++i) {
    std::string search = "\"id\":" + std::to_string(i);
    size_t pos = response.find(search);

    Workspace ws;
    ws.id = i;
    ws.name = std::to_string(i);
    ws.active = (i == active_id);
    ws.windows = 0;

    if (pos != std::string::npos) {
      // Extract windows count
      size_t windows_pos = response.find("\"windows\":", pos);
      if (windows_pos != std::string::npos) {
        size_t start = windows_pos + 10;
        size_t end = response.find_first_of(",}", start);
        if (end != std::string::npos) {
          ws.windows = std::stoi(response.substr(start, end - start));
        }
      }
    }

    new_workspaces.push_back(ws);
  }

  std::lock_guard<std::mutex> lock(workspaces_mutex_);
  workspaces_ = std::move(new_workspaces);
}

std::string HyprlandWidget::send_hyprland_command(const std::string& command) {
  const char* his = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if (!his) {
    return "";
  }

  std::string socket_path = std::string("/tmp/hypr/") + his + "/.socket.sock";

  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    return "";
  }

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

  if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
    close(sock);
    return "";
  }

  write(sock, command.c_str(), command.length());

  std::string response;
  char buffer[4096];
  ssize_t n;
  while ((n = read(sock, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[n] = '\0';
    response += buffer;
  }

  close(sock);
  return response;
}

} // namespace hyprbar
