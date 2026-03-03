#include "test_mocks.h"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace hyprbar {
namespace test {
namespace mocks {

// ============================================================================
// MockHyprland Implementation
// ============================================================================

MockHyprland::MockHyprland(const std::string& instance_sig)
    : instance_sig_(instance_sig), mock_pid_(-1) {
  socket_dir_ = "/tmp/hypr_test_" + instance_sig_;
}

MockHyprland::~MockHyprland() {
  stop();
}

bool MockHyprland::start() {
  if (is_running()) {
    return true;
  }

  // Create socket directory
  mkdir(socket_dir_.c_str(), 0755);

  // Fork mock server process
  mock_pid_ = fork();

  if (mock_pid_ == 0) {
    // Child process - run mock server
    execl("/usr/bin/env", "env", "python3", "tests/mock_hyprland_server.py",
          socket_dir_.c_str(), nullptr);
    exit(1); // execl failed
  }

  if (mock_pid_ < 0) {
    return false;
  }

  // Set environment variable
  setenv("HYPRLAND_INSTANCE_SIGNATURE", instance_sig_.c_str(), 1);

  // Create /tmp/hypr symlink
  std::string hypr_link = "/tmp/hypr/" + instance_sig_;
  mkdir("/tmp/hypr", 0755);
  symlink(socket_dir_.c_str(), hypr_link.c_str());

  // Wait for sockets to exist
  std::string cmd_socket = socket_dir_ + "/.socket.sock";
  for (int i = 0; i < 20; i++) {
    struct stat st;
    if (stat(cmd_socket.c_str(), &st) == 0) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  return false;
}

void MockHyprland::stop() {
  if (!is_running()) {
    return;
  }

  kill(mock_pid_, SIGTERM);
  waitpid(mock_pid_, nullptr, 0);
  mock_pid_ = -1;

  // Cleanup
  std::string hypr_link = "/tmp/hypr/" + instance_sig_;
  unlink(hypr_link.c_str());

  // Remove socket directory
  std::string rm_cmd = "rm -rf " + socket_dir_;
  system(rm_cmd.c_str());

  unsetenv("HYPRLAND_INSTANCE_SIGNATURE");
}

bool MockHyprland::is_running() const {
  if (mock_pid_ <= 0) {
    return false;
  }

  int status;
  pid_t result = waitpid(mock_pid_, &status, WNOHANG);
  return result == 0; // Still running
}

void MockHyprland::set_workspaces(const std::vector<Workspace>& workspaces) {
  // TODO: Write workspace config to mock server
  // For now, mock server has hardcoded workspaces
}

void MockHyprland::trigger_workspace_event(int workspace_id) {
  // TODO: Send event to mock server via control socket
}

// ============================================================================
// MockDBus Implementation
// ============================================================================

MockDBus::MockDBus() : mock_pid_(-1) {
  config_file_ = "/tmp/mock_dbus_config.json";
}

MockDBus::~MockDBus() {
  stop();
}

void MockDBus::write_config() {
  std::ofstream f(config_file_);
  f << "[\n";
  for (size_t i = 0; i < items_.size(); i++) {
    const auto& item = items_[i];
    f << "  {\n";
    f << "    \"service\": \"" << item.service << "\",\n";
    f << "    \"path\": \"" << item.path << "\",\n";
    if (!item.icon_name.empty()) {
      f << "    \"icon_name\": \"" << item.icon_name << "\",\n";
    }
    f << "    \"icon_pixmap\": null\n";
    f << "  }";
    if (i < items_.size() - 1) {
      f << ",";
    }
    f << "\n";
  }
  f << "]\n";
  f.close();
}

bool MockDBus::start() {
  if (is_running()) {
    return true;
  }

  // Write config file
  write_config();

  // Fork mock D-Bus server
  mock_pid_ = fork();

  if (mock_pid_ == 0) {
    // Child process
    execl("/usr/bin/env", "env", "python3", "tests/mock_dbus_server.py",
          config_file_.c_str(), nullptr);
    exit(1);
  }

  if (mock_pid_ < 0) {
    return false;
  }

  // Wait for D-Bus service to be ready
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  return true;
}

void MockDBus::stop() {
  if (!is_running()) {
    return;
  }

  kill(mock_pid_, SIGTERM);
  waitpid(mock_pid_, nullptr, 0);
  mock_pid_ = -1;

  // Cleanup config file
  unlink(config_file_.c_str());
}

bool MockDBus::is_running() const {
  if (mock_pid_ <= 0) {
    return false;
  }

  int status;
  pid_t result = waitpid(mock_pid_, &status, WNOHANG);
  return result == 0;
}

void MockDBus::add_tray_item(const TrayItem& item) {
  items_.push_back(item);
}

void MockDBus::clear_tray_items() {
  items_.clear();
}

// ============================================================================
// Helper Functions
// ============================================================================

MockHyprland create_hyprland_with_workspaces(int count) {
  MockHyprland mock("test_" + std::to_string(getpid()));

  std::vector<MockHyprland::Workspace> workspaces;
  for (int i = 1; i <= count; i++) {
    workspaces.push_back({i, std::to_string(i), 0});
  }
  mock.set_workspaces(workspaces);

  return mock;
}

MockDBus create_dbus_with_tray_items(int count) {
  MockDBus mock;

  for (int i = 0; i < count; i++) {
    MockDBus::TrayItem item;
    item.service = ":1." + std::to_string(100 + i);
    item.path = "/StatusNotifierItem";
    item.icon_name = "test-icon-" + std::to_string(i);
    mock.add_tray_item(item);
  }

  return mock;
}

bool wait_for_service(const MockService& service, int timeout_ms) {
  auto start = std::chrono::steady_clock::now();

  while (!service.is_running()) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    if (elapsed.count() >= timeout_ms) {
      return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  return true;
}

} // namespace mocks
} // namespace test
} // namespace hyprbar
