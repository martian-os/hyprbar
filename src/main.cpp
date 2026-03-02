#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/event_loop.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/rendering/renderer.h"
#include "hyprbar/wayland/wayland_manager.h"
#include "hyprbar/widgets/widget_manager.h"
#include <cairo/cairo.h>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <sys/epoll.h>
#include <thread>

using namespace hyprbar;

// Application state
struct AppState {
  std::unique_ptr<EventLoop> event_loop;
  std::unique_ptr<WaylandManager> wayland;
  std::unique_ptr<Renderer> renderer;
  std::unique_ptr<WidgetManager> widget_manager;
  Config config;
};

static AppState app;

void signal_handler(int /*sig*/) {
  Logger::instance().info("Received shutdown signal");
  if (app.event_loop) {
    app.event_loop->shutdown();
  }
}

void render_frame(void* wayland_buffer) {
  if (!app.renderer) {
    return;
  }

  app.renderer->begin_frame();

  Color bg = Color::from_hex(app.config.bar.background);
  app.renderer->clear(bg);

  // Render widgets
  if (app.widget_manager) {
    app.widget_manager->render(*app.renderer, app.renderer->get_width(),
                               app.renderer->get_height());
  }

  app.renderer->end_frame();

  // Copy to Wayland buffer if provided
  if (wayland_buffer) {
    std::memcpy(wayland_buffer, app.renderer->get_buffer_data(),
                app.renderer->get_buffer_size());
  }
}

int run_screenshot_mode(const std::string& output_path,
                        ConfigManager& config_mgr) {
  Logger::instance().info("Screenshot mode: {}", output_path);
  const Config& config = config_mgr.get_config();

  // Initialize renderer
  app.renderer = std::make_unique<Renderer>();
  if (!app.renderer->initialize(1920, config.bar.height)) {
    Logger::instance().error("Failed to initialize renderer");
    return 1;
  }

  // Initialize widgets
  app.widget_manager = std::make_unique<WidgetManager>();
  app.widget_manager->initialize(config_mgr);
  app.widget_manager->update();

  // Wait briefly for script widgets to execute first time
  // Most scripts complete in < 100ms, but give 500ms for slower ones
  Logger::instance().debug("Waiting for widgets to populate...");
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Update again after wait to pick up script output
  app.widget_manager->update();

  // Render one frame
  render_frame(nullptr);

  // Save directly from renderer's Cairo surface
  cairo_surface_write_to_png(cairo_get_target(app.renderer->get_context()),
                             output_path.c_str());
  Logger::instance().info("Screenshot saved to: {}", output_path);

  return 0;
}

int run_wayland_mode(ConfigManager& config_mgr) {
  Logger::instance().info("Wayland mode starting...");
  const Config& config = config_mgr.get_config();

  app.wayland = std::make_unique<WaylandManager>();
  if (!app.wayland->initialize()) {
    Logger::instance().error("Wayland initialization failed");
    return 1;
  }

  WaylandManager::BarPosition position;
  switch (config.bar.position) {
  case BarConfig::Position::Top:
    position = WaylandManager::BarPosition::Top;
    break;
  case BarConfig::Position::Bottom:
    position = WaylandManager::BarPosition::Bottom;
    break;
  case BarConfig::Position::Left:
    position = WaylandManager::BarPosition::Left;
    break;
  case BarConfig::Position::Right:
    position = WaylandManager::BarPosition::Right;
    break;
  }

  if (!app.wayland->create_bar_surface(position, 0, config.bar.height)) {
    Logger::instance().error("Failed to create bar surface");
    return 1;
  }

  app.wayland->set_exclusive_zone(config.bar.height);

  // Wait for compositor to configure surface dimensions
  wl_display_roundtrip(app.wayland->get_display());

  uint32_t bar_width = app.wayland->get_configured_width();
  uint32_t bar_height = app.wayland->get_configured_height();

  if (bar_width == 0 || bar_height == 0) {
    Logger::instance().error("Invalid surface dimensions: {}x{}", bar_width,
                             bar_height);
    return 1;
  }

  Logger::instance().info("Bar surface: {}x{}", bar_width, bar_height);

  // Initialize renderer
  app.renderer = std::make_unique<Renderer>();
  if (!app.renderer->initialize(bar_width, bar_height)) {
    Logger::instance().error("Failed to initialize renderer");
    return 1;
  }

  // Create Wayland buffer
  void* buffer_data = nullptr;
  wl_buffer* buffer =
      app.wayland->create_buffer(app.renderer->get_buffer_size(), &buffer_data);
  if (!buffer || !buffer_data) {
    Logger::instance().error("Failed to create Wayland buffer");
    return 1;
  }

  // Initialize widgets
  app.widget_manager = std::make_unique<WidgetManager>();
  app.widget_manager->initialize(config_mgr);

  // Setup event loop
  app.event_loop = std::make_unique<EventLoop>();

  int wayland_fd = app.wayland->get_fd();
  app.event_loop->add_fd(wayland_fd, EPOLLIN,
                         [](int /*fd*/, uint32_t /*events*/) {
                           if (app.wayland->dispatch() < 0) {
                             Logger::instance().error("Wayland dispatch error");
                             app.event_loop->shutdown();
                           }
                         });

  app.event_loop->add_timer(
      std::chrono::milliseconds(1000), [buffer, buffer_data]() {
        if (app.widget_manager && app.widget_manager->update()) {
          render_frame(buffer_data);
          app.wayland->attach_and_commit(buffer);
        }
      });

  // Initial render
  render_frame(buffer_data);
  app.wayland->attach_and_commit(buffer);

  Logger::instance().info("Event loop starting...");
  while (app.event_loop->dispatch()) {
    while (app.wayland->prepare_read() != 0) {
      app.wayland->dispatch_pending();
    }
    app.wayland->flush();
    app.wayland->read_events();
    app.wayland->dispatch_pending();
  }

  return 0;
}

void print_usage(const char* program) {
  std::cout
      << "Usage: " << program << " [OPTIONS]\n\n"
      << "Options:\n"
      << "  --config <path>       Path to configuration file (JSON)\n"
      << "  --screenshot <path>   Generate screenshot to file (no "
         "compositor needed)\n"
      << "  --help                Show this help\n\n"
      << "Without options, runs in normal Wayland mode with default config.\n"
      << "Default config location: ~/.config/hyprbar/config.json\n";
}

int main(int argc, char** argv) {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  Logger::instance().set_level(Logger::Level::Debug);
  Logger::instance().info("Hyprbar v0.1.0");

  // Parse arguments
  std::string screenshot_path;
  std::string config_path;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
    } else if (arg == "--screenshot" && i + 1 < argc) {
      screenshot_path = argv[++i];
    } else if (arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      print_usage(argv[0]);
      return 1;
    }
  }

  // Load config
  ConfigManager config_mgr;
  if (config_path.empty()) {
    config_path = ConfigManager::get_default_config_path();
  }
  if (!config_mgr.load(config_path)) {
    Logger::instance().warn("Could not load config from {}, using defaults",
                            config_path);
  } else {
    Logger::instance().info("Loaded configuration from {}", config_path);
  }
  app.config = config_mgr.get_config();

  // Run in appropriate mode
  if (!screenshot_path.empty()) {
    return run_screenshot_mode(screenshot_path, config_mgr);
  } else {
    return run_wayland_mode(config_mgr);
  }
}
