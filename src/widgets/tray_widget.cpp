#include "hyprbar/widgets/tray_widget.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/rendering/renderer.h"
#include <cstring>

namespace hyprbar {

TrayWidget::TrayWidget() = default;

TrayWidget::~TrayWidget() {
  running_ = false;
  if (dbus_thread_.joinable()) {
    dbus_thread_.join();
  }
}

bool TrayWidget::initialize(const ConfigValue& config) {
  if (!config.is_object()) {
    Logger::instance().warn("Tray widget config must be object");
    return false;
  }

  const auto& obj = config.as_object();

  if (obj.count("icon_size")) {
    icon_size_ = static_cast<int>(obj.at("icon_size").as_int());
  }

  if (obj.count("spacing")) {
    icon_spacing_ = static_cast<int>(obj.at("spacing").as_int());
  }

  // Fetch initial tray items
  fetch_tray_items();

  // Start D-Bus listener thread
  running_ = true;
  dbus_thread_ = std::thread(&TrayWidget::dbus_listener_thread, this);

  return true;
}

bool TrayWidget::update() {
  if (icons_changed_.exchange(false)) {
    return true; // Request redraw
  }
  return false;
}

void TrayWidget::render(Renderer& renderer, int x, int y, int /*width*/,
                        int height) {
  std::lock_guard<std::mutex> lock(icons_mutex_);

  int current_x = x;

  for (size_t i = 0; i < icons_.size(); ++i) {
    // Render placeholder box for each icon
    Color box_color = Color::from_hex("#89b4fa");
    int box_y = y + (height - icon_size_) / 2;

    renderer.fill_rect(current_x, box_y, icon_size_, icon_size_, box_color);

    current_x += icon_size_ + icon_spacing_;
  }
}

int TrayWidget::get_desired_width() const {
  std::lock_guard<std::mutex> lock(icons_mutex_);
  if (icons_.empty())
    return 0;
  return static_cast<int>(icons_.size()) * (icon_size_ + icon_spacing_) -
         icon_spacing_;
}

int TrayWidget::get_desired_height() const {
  return 0; // Flexible
}

void TrayWidget::dbus_listener_thread() {
  // NOTE: Full D-Bus implementation requires libdbus or sd-bus
  // This is a simplified stub that shows the structure

  Logger::instance().info("Tray D-Bus listener started (stub - needs libdbus)");

  while (running_) {
    // Poll for tray item changes every 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // In a real implementation:
    // 1. Connect to session bus
    // 2. Watch org.kde.StatusNotifierWatcher
    // 3. Monitor StatusNotifierItemRegistered/Unregistered signals
    // 4. Fetch icon data when items change

    // For now, just stub
  }
}

void TrayWidget::fetch_tray_items() {
  // NOTE: This is a stub. Real implementation needs D-Bus:
  //
  // 1. Call org.kde.StatusNotifierWatcher.RegisteredStatusNotifierItems
  // 2. For each item, query:
  //    - Title (org.kde.StatusNotifierItem.Title)
  //    - IconPixmap (org.kde.StatusNotifierItem.IconPixmap)
  //    - ToolTip (org.kde.StatusNotifierItem.ToolTip)
  // 3. Store icon data for rendering

  Logger::instance().debug("Fetching tray items (stub - needs D-Bus library)");

  std::lock_guard<std::mutex> lock(icons_mutex_);

  // Placeholder: empty tray until D-Bus is implemented
  icons_.clear();
}

void TrayWidget::fetch_icon_data(TrayIcon& icon) {
  // Extract ARGB pixmap data from D-Bus property
  // Format: array of (width, height, pixels[])
  // where pixels[] is ARGB32 data

  // Stub for now
  (void)icon;
}

} // namespace hyprbar
