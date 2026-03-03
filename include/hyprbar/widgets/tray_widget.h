#pragma once

#include "widget.h"
#include <atomic>
#include <cairo/cairo.h>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Forward declarations
typedef struct _GdkPixbuf GdkPixbuf;

namespace hyprbar {

struct ConfigValue;

/**
 * TrayWidget - System tray for status notifier items (SNI)
 *
 * Implements the StatusNotifierItem specification:
 * - Discovers tray icons via D-Bus
 * - Renders icon pixmaps
 * - Handles click events (menu, activate)
 *
 * Communicates with:
 * - org.kde.StatusNotifierWatcher (discovery)
 * - org.kde.StatusNotifierItem.* (individual icons)
 */
class TrayWidget : public Widget {
public:
  TrayWidget();
  ~TrayWidget() override;

  bool initialize(const ConfigValue& config) override;
  bool update() override;
  void render(Renderer& renderer, int x, int y, int width, int height) override;
  int get_desired_width() const override;
  int get_desired_height() const override;
  std::string get_type() const override {
    return "tray";
  }

private:
  struct TrayIcon {
    std::string service;
    std::string path;
    std::string title;
    std::string tooltip;
    std::string icon_name; // Icon theme name (fallback if no pixmap)
    int width = 16;
    int height = 16;
    std::vector<uint8_t> pixmap_data;
  };

  void dbus_listener_thread();
  void fetch_tray_items();
  void fetch_icon_data(TrayIcon& icon);
  cairo_surface_t* load_icon_from_theme(const std::string& icon_name, int size);
  cairo_surface_t* pixbuf_to_cairo_surface(GdkPixbuf* pixbuf);

  int icon_size_ = 16;
  int icon_spacing_ = 5;

  std::vector<TrayIcon> icons_;
  mutable std::mutex icons_mutex_;

  std::thread dbus_thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> icons_changed_{false};
};

} // namespace hyprbar
