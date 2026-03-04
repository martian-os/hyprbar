#include "hyprbar/widgets/tray_widget.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/glib_utils.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/rendering/renderer.h"
#include <cstring>
#include <dbus/dbus.h>
#include <filesystem>
#include <gdk-pixbuf/gdk-pixbuf.h>

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
    const auto& icon = icons_[i];
    int box_y = y + (height - icon_size_) / 2;

    bool rendered = false;

    // PRIORITY 1: Try loading from icon theme (MOST COMMON)
    if (!rendered && !icon.icon_name.empty()) {
      cairo_surface_t* icon_surface =
          load_icon_from_theme(icon.icon_name, icon_size_);
      if (icon_surface) {
        int surf_width = cairo_image_surface_get_width(icon_surface);
        int surf_height = cairo_image_surface_get_height(icon_surface);
        renderer.draw_surface(icon_surface, current_x, box_y, surf_width,
                              surf_height);
        cairo_surface_destroy(icon_surface);
        rendered = true;
      }
    }

    // PRIORITY 2: Try rendering from pixmap data (RARE FALLBACK)
    if (!rendered && !icon.pixmap_data.empty() && icon.width > 0 &&
        icon.height > 0) {
      int expected_size = icon.width * icon.height * 4;
      if (icon.pixmap_data.size() >= static_cast<size_t>(expected_size)) {
        cairo_surface_t* icon_surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, icon.width, icon.height);

        if (cairo_surface_status(icon_surface) == CAIRO_STATUS_SUCCESS) {
          unsigned char* surface_data =
              cairo_image_surface_get_data(icon_surface);
          int stride = cairo_image_surface_get_stride(icon_surface);

          for (int row = 0; row < icon.height; ++row) {
            for (int col = 0; col < icon.width; ++col) {
              int src_idx = (row * icon.width + col) * 4;
              int dst_idx = row * stride + col * 4;

              uint8_t a = icon.pixmap_data[src_idx];
              uint8_t r = icon.pixmap_data[src_idx + 1];
              uint8_t g = icon.pixmap_data[src_idx + 2];
              uint8_t b = icon.pixmap_data[src_idx + 3];

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
              surface_data[dst_idx + 0] = b;
              surface_data[dst_idx + 1] = g;
              surface_data[dst_idx + 2] = r;
              surface_data[dst_idx + 3] = a;
#else
              surface_data[dst_idx + 0] = a;
              surface_data[dst_idx + 1] = r;
              surface_data[dst_idx + 2] = g;
              surface_data[dst_idx + 3] = b;
#endif
            }
          }

          cairo_surface_mark_dirty(icon_surface);
          renderer.draw_surface(icon_surface, current_x, box_y, icon_size_,
                                icon_size_);
          cairo_surface_destroy(icon_surface);
          rendered = true;
        }
      }
    }

    // FINAL FALLBACK: Blue placeholder
    if (!rendered) {
      Color box_color = Color::from_hex("#89b4fa");
      renderer.fill_rect(current_x, box_y, icon_size_, icon_size_, box_color);
    }

    current_x += icon_size_ + icon_spacing_;
  }
}

int TrayWidget::get_desired_width() const noexcept {
  std::lock_guard<std::mutex> lock(icons_mutex_);
  if (icons_.empty())
    return 0;
  return static_cast<int>(icons_.size()) * (icon_size_ + icon_spacing_) -
         icon_spacing_;
}

int TrayWidget::get_desired_height() const noexcept {
  return 0; // Flexible
}

cairo_surface_t* TrayWidget::load_icon_from_theme(const std::string& icon_name,
                                                  int size) {
  // Generate icon name variants for fallback
  // 1. Original name (e.g., "nm-signal-75")
  // 2. With -symbolic suffix (e.g., "nm-signal-75-symbolic")
  // 3. Without -symbolic if present (e.g., "nm-signal-75" from
  // "nm-signal-75-symbolic")
  std::vector<std::string> icon_variants;
  icon_variants.push_back(icon_name);

  // Try with -symbolic suffix
  if (icon_name.find("-symbolic") == std::string::npos) {
    icon_variants.push_back(icon_name + "-symbolic");
  }

  // Try without -symbolic if present
  if (icon_name.size() >= 9 &&
      icon_name.substr(icon_name.size() - 9) == "-symbolic") {
    icon_variants.push_back(
        icon_name.substr(0, icon_name.length() - 9)); // Remove "-symbolic"
  }

  // Icon theme search paths (in priority order)
  std::vector<std::string> theme_paths = {
      "/usr/share/icons/Adwaita",
      "/usr/share/icons/Yaru",
      "/usr/share/icons/hicolor",
      "/usr/share/icons/gnome",
      "/usr/share/icons/Humanity",
      std::string(getenv("HOME") ? getenv("HOME") : "") +
          "/.local/share/icons"};

  // Subdirectories to search (common sizes and categories)
  std::vector<std::string> subdirs = {"/symbolic/status/",
                                      "/symbolic/apps/",
                                      "/symbolic/categories/",
                                      std::string("/") + std::to_string(size) +
                                          "x" + std::to_string(size) +
                                          "/status/",
                                      std::string("/") + std::to_string(size) +
                                          "x" + std::to_string(size) + "/apps/",
                                      "/scalable/status/",
                                      "/scalable/apps/",
                                      "/24/status/",
                                      "/24/apps/",
                                      "/22/status/",
                                      "/22/apps/",
                                      "/16/status/",
                                      "/16/apps/"};

  // Try all icon name variants
  for (const auto& variant : icon_variants) {
    // Try finding the icon file
    for (const auto& theme_path : theme_paths) {
      for (const auto& subdir : subdirs) {
        // Try SVG first
        std::string svg_path = theme_path + subdir + variant + ".svg";
        if (std::filesystem::exists(svg_path)) {
          GError* raw_error = nullptr;
          GObjectPtr<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file_at_size(
              svg_path.c_str(), size, size, &raw_error));
          GErrorPtr error(raw_error);

          if (pixbuf) {
            cairo_surface_t* surface = pixbuf_to_cairo_surface(pixbuf.get());
            Logger::instance().debug("Loaded icon from: {}", svg_path);
            return surface;
          }

          if (error) {
            Logger::instance().warn("Failed to load {}: {}", svg_path,
                                    error->message);
          }
        }

        // Try PNG
        std::string png_path = theme_path + subdir + variant + ".png";
        if (std::filesystem::exists(png_path)) {
          GError* raw_error = nullptr;
          GObjectPtr<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file_at_size(
              png_path.c_str(), size, size, &raw_error));
          GErrorPtr error(raw_error);

          if (pixbuf) {
            cairo_surface_t* surface = pixbuf_to_cairo_surface(pixbuf.get());
            Logger::instance().debug("Loaded icon from: {}", png_path);
            return surface;
          }
        }
      }
    }
  }

  Logger::instance().warn("Icon not found in theme: {} (tried {} variants)",
                          icon_name, icon_variants.size());
  return nullptr;
}

cairo_surface_t* TrayWidget::pixbuf_to_cairo_surface(GdkPixbuf* pixbuf) {
  int width = gdk_pixbuf_get_width(pixbuf);
  int height = gdk_pixbuf_get_height(pixbuf);
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

  cairo_surface_t* surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

  unsigned char* surface_data = cairo_image_surface_get_data(surface);
  guchar* pixbuf_data = gdk_pixbuf_get_pixels(pixbuf);
  int pixbuf_stride = gdk_pixbuf_get_rowstride(pixbuf);
  int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
  bool has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);

  // Convert GdkPixbuf (RGBA) to Cairo ARGB32
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      guchar* p = pixbuf_data + y * pixbuf_stride + x * n_channels;
      unsigned char* q = surface_data + y * stride + x * 4;

      guchar r = p[0];
      guchar g = p[1];
      guchar b = p[2];
      guchar a = has_alpha ? p[3] : 255;

// Cairo ARGB32 is native byte order
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      q[0] = b;
      q[1] = g;
      q[2] = r;
      q[3] = a;
#else
      q[0] = a;
      q[1] = r;
      q[2] = g;
      q[3] = b;
#endif
    }
  }

  cairo_surface_mark_dirty(surface);
  return surface;
}

void TrayWidget::dbus_listener_thread() {
  Logger::instance().info("Tray D-Bus listener started");

  DBusError err;
  dbus_error_init(&err);

  DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (dbus_error_is_set(&err)) {
    Logger::instance().error("D-Bus connection error: " +
                             std::string(err.message));
    dbus_error_free(&err);
    return;
  }

  if (!conn) {
    Logger::instance().error("D-Bus connection failed");
    return;
  }

  // Register for StatusNotifierItemRegistered/Unregistered signals
  dbus_bus_add_match(conn,
                     "type='signal',"
                     "interface='org.kde.StatusNotifierWatcher',"
                     "member='StatusNotifierItemRegistered'",
                     &err);

  if (dbus_error_is_set(&err)) {
    Logger::instance().warn("Failed to add D-Bus match: " +
                            std::string(err.message));
    dbus_error_free(&err);
  }

  dbus_connection_flush(conn);

  while (running_) {
    // Poll for messages with timeout
    dbus_connection_read_write(conn, 5000); // 5 second timeout

    DBusMessage* msg = dbus_connection_pop_message(conn);
    if (msg) {
      // Handle StatusNotifierItemRegistered signal
      if (dbus_message_is_signal(msg, "org.kde.StatusNotifierWatcher",
                                 "StatusNotifierItemRegistered")) {
        Logger::instance().info("Tray item registered, refreshing...");
        fetch_tray_items();
        icons_changed_ = true;
      }

      dbus_message_unref(msg);
    }
  }

  dbus_connection_unref(conn);
}

void TrayWidget::fetch_tray_items() {
  Logger::instance().debug("Fetching tray items via D-Bus");

  DBusError err;
  dbus_error_init(&err);

  DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (dbus_error_is_set(&err)) {
    Logger::instance().error("D-Bus connection error: " +
                             std::string(err.message));
    dbus_error_free(&err);

    // Fallback to demo icons
    std::lock_guard<std::mutex> lock(icons_mutex_);
    icons_.clear();
    for (int i = 0; i < 3; ++i) {
      TrayIcon icon;
      icon.service = "demo.app." + std::to_string(i);
      icon.title = "Demo Icon " + std::to_string(i);
      icons_.push_back(icon);
    }
    return;
  }

  // Query StatusNotifierWatcher for registered items
  DBusMessage* msg = dbus_message_new_method_call(
      "org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
      "org.freedesktop.DBus.Properties", "Get");

  if (!msg) {
    Logger::instance().error("Failed to create D-Bus message");
    dbus_connection_unref(conn);
    return;
  }

  const char* interface = "org.kde.StatusNotifierWatcher";
  const char* property = "RegisteredStatusNotifierItems";

  dbus_message_append_args(msg, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING,
                           &property, DBUS_TYPE_INVALID);

  DBusMessage* reply =
      dbus_connection_send_with_reply_and_block(conn, msg, 1000, &err);
  dbus_message_unref(msg);

  if (dbus_error_is_set(&err) || !reply) {
    Logger::instance().warn(
        "Failed to query StatusNotifierWatcher: " +
        std::string(dbus_error_is_set(&err) ? err.message : "no reply"));
    dbus_error_free(&err);
    dbus_connection_unref(conn);

    // Fallback to demo icons
    std::lock_guard<std::mutex> lock(icons_mutex_);
    icons_.clear();
    for (int i = 0; i < 3; ++i) {
      TrayIcon icon;
      icon.service = "demo.app." + std::to_string(i);
      icon.title = "Demo Icon " + std::to_string(i);
      icons_.push_back(icon);
    }
    return;
  }

  // Parse reply (variant containing array of strings)
  DBusMessageIter iter, variant, array;
  dbus_message_iter_init(reply, &iter);

  if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
    dbus_message_iter_recurse(&iter, &variant);

    if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_ARRAY) {
      dbus_message_iter_recurse(&variant, &array);

      std::vector<TrayIcon> new_icons;

      while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRING) {
        const char* service_str;
        dbus_message_iter_get_basic(&array, &service_str);

        TrayIcon icon;
        std::string item_str = service_str;

        // Parse service name and path
        // Format can be:
        // 1. "service@path" (most common)
        // 2. "service/path" (alternative, deprecated)
        size_t at_pos = item_str.find('@');
        size_t slash_pos = item_str.find('/');

        if (at_pos != std::string::npos) {
          // Format: service@path
          icon.service = item_str.substr(0, at_pos);
          icon.path = item_str.substr(at_pos + 1);
          Logger::instance().debug(
              "Parsed item (@ format): service='{}', path='{}'", icon.service,
              icon.path);
        } else if (slash_pos != std::string::npos) {
          // Format: service/path (e.g., ":1.5/org/ayatana/...")
          // Service name is everything before first '/'
          icon.service = item_str.substr(0, slash_pos);
          icon.path = "/" + item_str.substr(slash_pos + 1);
          Logger::instance().debug(
              "Parsed item (/ format): service='{}', path='{}'", icon.service,
              icon.path);
        } else {
          // No separator, assume service only
          icon.service = item_str;
          icon.path = "/StatusNotifierItem";
          Logger::instance().debug(
              "Parsed item (no separator): service='{}', path='{}'",
              icon.service, icon.path);
        }

        // Validate service name (must start with : for unique names or be a
        // well-known name)
        if (icon.service.empty() ||
            (icon.service[0] != ':' &&
             icon.service.find('.') == std::string::npos)) {
          Logger::instance().warn("Invalid service name format: '{}', skipping",
                                  item_str);
          dbus_message_iter_next(&array);
          continue;
        }

        fetch_icon_data(icon);
        new_icons.push_back(icon);

        dbus_message_iter_next(&array);
      }

      std::lock_guard<std::mutex> lock(icons_mutex_);
      icons_ = std::move(new_icons);

      Logger::instance().info("Fetched " + std::to_string(icons_.size()) +
                              " tray items");
    }
  }

  dbus_message_unref(reply);
  dbus_connection_unref(conn);
}

void TrayWidget::fetch_icon_data(TrayIcon& icon) {
  DBusError err;
  dbus_error_init(&err);

  DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (dbus_error_is_set(&err) || !conn) {
    Logger::instance().warn("Failed to connect to D-Bus for icon data: {}",
                            dbus_error_is_set(&err) ? err.message
                                                    : "no connection");
    dbus_error_free(&err);
    return;
  }

  Logger::instance().debug("Querying IconPixmap for: {} {}", icon.service,
                           icon.path);

  // Try IconPixmap first
  DBusMessage* msg =
      dbus_message_new_method_call(icon.service.c_str(), icon.path.c_str(),
                                   "org.freedesktop.DBus.Properties", "Get");

  if (!msg) {
    Logger::instance().warn(
        "Failed to create D-Bus message for IconPixmap query");
    dbus_connection_unref(conn);
    return;
  }

  const char* interface = "org.kde.StatusNotifierItem";
  const char* property = "IconPixmap";

  dbus_message_append_args(msg, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING,
                           &property, DBUS_TYPE_INVALID);

  DBusMessage* reply =
      dbus_connection_send_with_reply_and_block(conn, msg, 500, &err);
  dbus_message_unref(msg);

  bool got_pixmap = false;

  if (!dbus_error_is_set(&err) && reply) {
    // Parse IconPixmap: variant containing array of (width, height, pixels)
    DBusMessageIter iter, variant, array_iter, struct_iter, pixels_iter;
    dbus_message_iter_init(reply, &iter);

    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
      dbus_message_iter_recurse(&iter, &variant);

      if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_ARRAY) {
        dbus_message_iter_recurse(&variant, &array_iter);

        // Get first icon representation
        if (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT) {
          dbus_message_iter_recurse(&array_iter, &struct_iter);

          int width, height;
          dbus_message_iter_get_basic(&struct_iter, &width);
          dbus_message_iter_next(&struct_iter);
          dbus_message_iter_get_basic(&struct_iter, &height);
          dbus_message_iter_next(&struct_iter);

          icon.width = width;
          icon.height = height;

          Logger::instance().debug("Icon dimensions: {}x{}", width, height);

          // Get pixel data (array of bytes)
          if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_ARRAY) {
            dbus_message_iter_recurse(&struct_iter, &pixels_iter);

            int elem_type = dbus_message_iter_get_arg_type(&pixels_iter);
            if (elem_type == DBUS_TYPE_BYTE) {
              // Read all bytes
              while (dbus_message_iter_get_arg_type(&pixels_iter) ==
                     DBUS_TYPE_BYTE) {
                uint8_t byte;
                dbus_message_iter_get_basic(&pixels_iter, &byte);
                icon.pixmap_data.push_back(byte);
                dbus_message_iter_next(&pixels_iter);
              }
              Logger::instance().info("Fetched {} bytes of pixmap data for {}",
                                      icon.pixmap_data.size(), icon.service);
              got_pixmap = true;
            }
          }
        }
      }
    }
    dbus_message_unref(reply);
  }

  dbus_error_free(&err);

  // If no pixmap, try IconName
  if (!got_pixmap) {
    Logger::instance().debug("No IconPixmap, trying IconName for {}",
                             icon.service);

    dbus_error_init(&err);
    msg =
        dbus_message_new_method_call(icon.service.c_str(), icon.path.c_str(),
                                     "org.freedesktop.DBus.Properties", "Get");

    if (!msg) {
      Logger::instance().warn(
          "Failed to create D-Bus message for IconName query");
      dbus_connection_unref(conn);
      return;
    }

    property = "IconName";
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &interface,
                             DBUS_TYPE_STRING, &property, DBUS_TYPE_INVALID);

    reply = dbus_connection_send_with_reply_and_block(conn, msg, 500, &err);
    dbus_message_unref(msg);

    if (!dbus_error_is_set(&err) && reply) {
      DBusMessageIter iter, variant;
      dbus_message_iter_init(reply, &iter);

      if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
        dbus_message_iter_recurse(&iter, &variant);

        if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_STRING) {
          const char* icon_name_str;
          dbus_message_iter_get_basic(&variant, &icon_name_str);
          icon.icon_name = icon_name_str;
          Logger::instance().info("Using icon theme name '{}' for {}",
                                  icon.icon_name, icon.service);
        }
      }
      dbus_message_unref(reply);
    }

    dbus_error_free(&err);
  }

  dbus_connection_unref(conn);
}

void TrayWidget::on_click(int x, int /*y*/, uint32_t button) noexcept {
  std::lock_guard<std::mutex> lock(icons_mutex_);

  // Find which icon was clicked
  int current_x = 0;
  for (const auto& icon : icons_) {
    if (x >= current_x && x < current_x + icon_size_) {
      Logger::instance().info("Clicked tray icon: {} (button {})", icon.service,
                              button);

      // Activate the icon (will trigger menu or default action)
      // Note: We can't use x/y here because we're in noexcept context
      // and don't have proper error handling for D-Bus. Instead, we'll
      // just call Activate with dummy coordinates.
      activate_tray_icon(icon.service, icon.path, x, 0);
      return;
    }
    current_x += icon_size_ + icon_spacing_;
  }
}

void TrayWidget::activate_tray_icon(const std::string& service,
                                    const std::string& path, int x, int y) {
  DBusError err;
  dbus_error_init(&err);

  DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (dbus_error_is_set(&err) || !conn) {
    Logger::instance().error("Failed to connect to D-Bus: {}",
                             dbus_error_is_set(&err) ? err.message
                                                     : "no connection");
    dbus_error_free(&err);
    return;
  }

  // Strategy: Try multiple activation methods (different implementations
  // support different methods)
  // 1. Activate (primary action, KDE standard)
  // 2. ContextMenu (show menu, KDE standard)
  // 3. SecondaryActivate (middle-click, Ayatana/Ubuntu)

  bool success = false;

  // Try 1: Activate method (left-click action)
  DBusMessage* msg = dbus_message_new_method_call(
      service.c_str(), path.c_str(), "org.kde.StatusNotifierItem", "Activate");
  if (msg) {
    dbus_message_append_args(msg, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y,
                             DBUS_TYPE_INVALID);

    DBusMessage* reply =
        dbus_connection_send_with_reply_and_block(conn, msg, 500, &err);
    dbus_message_unref(msg);

    if (!dbus_error_is_set(&err) && reply) {
      Logger::instance().debug("Sent Activate to {}", service);
      dbus_message_unref(reply);
      success = true;
    }
    dbus_error_free(&err);
  }

  // Try 2: ContextMenu method
  if (!success) {
    dbus_error_init(&err);
    msg = dbus_message_new_method_call(service.c_str(), path.c_str(),
                                       "org.kde.StatusNotifierItem",
                                       "ContextMenu");
    if (msg) {
      dbus_message_append_args(msg, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y,
                               DBUS_TYPE_INVALID);

      DBusMessage* reply =
          dbus_connection_send_with_reply_and_block(conn, msg, 500, &err);
      dbus_message_unref(msg);

      if (!dbus_error_is_set(&err) && reply) {
        Logger::instance().debug("Sent ContextMenu to {}", service);
        dbus_message_unref(reply);
        success = true;
      }
      dbus_error_free(&err);
    }
  }

  // Try 3: SecondaryActivate (Ayatana/Ubuntu fallback)
  if (!success) {
    dbus_error_init(&err);
    msg = dbus_message_new_method_call(service.c_str(), path.c_str(),
                                       "org.kde.StatusNotifierItem",
                                       "SecondaryActivate");
    if (msg) {
      dbus_message_append_args(msg, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y,
                               DBUS_TYPE_INVALID);

      DBusMessage* reply =
          dbus_connection_send_with_reply_and_block(conn, msg, 500, &err);
      dbus_message_unref(msg);

      if (!dbus_error_is_set(&err) && reply) {
        Logger::instance().debug("Sent SecondaryActivate to {}", service);
        dbus_message_unref(reply);
        success = true;
      }
      dbus_error_free(&err);
    }
  }

  if (!success) {
    Logger::instance().warn(
        "Failed to activate tray icon {} - no supported methods responded",
        service);
  }

  dbus_connection_unref(conn);
}

} // namespace hyprbar
