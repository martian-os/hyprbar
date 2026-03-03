#ifndef HYPRBAR_TEST_MOCKS_H
#define HYPRBAR_TEST_MOCKS_H

#include <cstdint>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace hyprbar {
namespace test {
namespace mocks {

/**
 * Mock service manager - handles starting/stopping mock servers
 */
class MockService {
public:
  virtual ~MockService() = default;
  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool is_running() const = 0;
};

/**
 * Mock Hyprland server
 */
class MockHyprland : public MockService {
public:
  struct Workspace {
    int id;
    std::string name;
    int windows;
  };

  explicit MockHyprland(const std::string& instance_sig);
  ~MockHyprland();

  bool start() override;
  void stop() override;
  bool is_running() const override;

  // Configure mock responses
  void set_workspaces(const std::vector<Workspace>& workspaces);
  void trigger_workspace_event(int workspace_id);

  std::string get_instance_signature() const {
    return instance_sig_;
  }

private:
  std::string instance_sig_;
  std::string socket_dir_;
  int mock_pid_;
};

/**
 * Mock D-Bus StatusNotifierWatcher
 */
class MockDBus : public MockService {
public:
  struct TrayItem {
    std::string service;
    std::string path;
    std::string icon_name;
    std::vector<uint8_t> icon_pixmap;
    int width;
    int height;
  };

  MockDBus();
  ~MockDBus();

  bool start() override;
  void stop() override;
  bool is_running() const override;

  // Configure mock items
  void add_tray_item(const TrayItem& item);
  void clear_tray_items();

private:
  std::vector<TrayItem> items_;
  int mock_pid_;
  std::string config_file_;
  void write_config();
};

/**
 * RAII wrapper for mock services
 */
template <typename T> class MockGuard {
public:
  explicit MockGuard(T& mock) : mock_(mock) {
    if (!mock_.start()) {
      throw std::runtime_error("Failed to start mock service");
    }
  }

  ~MockGuard() {
    mock_.stop();
  }

  MockGuard(const MockGuard&) = delete;
  MockGuard& operator=(const MockGuard&) = delete;

  T& get() {
    return mock_;
  }

private:
  T& mock_;
};

/**
 * Helper functions for common test scenarios
 */
// Create mock Hyprland with default workspace setup
MockHyprland create_hyprland_with_workspaces(int count);

// Create mock D-Bus with sample tray items
MockDBus create_dbus_with_tray_items(int count);

// Wait for service to be ready
bool wait_for_service(const MockService& service, int timeout_ms = 1000);

} // namespace mocks
} // namespace test
} // namespace hyprbar

#endif // HYPRBAR_TEST_MOCKS_H
