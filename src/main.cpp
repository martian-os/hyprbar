#include "hyprbar/core/event_loop.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/core/config_manager.h"
#include <iostream>
#include <wayland-client.h>
#include <string>
#include <cstring>
#include <memory>
#include <sys/epoll.h>

using namespace hyprbar;

// Global Wayland objects
static struct wl_display *display = nullptr;
static struct wl_compositor *compositor = nullptr;
static struct wl_surface *surface = nullptr;
static std::unique_ptr<EventLoop> event_loop = nullptr;

// Registry listener to bind Wayland globals
static void registry_handle_global(void* /*data*/, struct wl_registry *registry,
                                   uint32_t name, const char *interface,
                                   uint32_t /*version*/) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = static_cast<struct wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 4));
    }
}

static void registry_handle_global_remove(void* /*data*/,
                                          struct wl_registry* /*registry*/,
                                          uint32_t /*name*/) {
    // Handle global removal if needed
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove,
};

int main(int /*argc*/, char** /*argv*/) {
    // Initialize logger
    Logger::instance().set_level(Logger::Level::Debug);
    Logger::instance().info("Hyprbar v0.1.0 starting...");

    // Load configuration (use default if not found)
    ConfigManager config_mgr;
    std::string config_path = ConfigManager::get_default_config_path();
    
    if (!config_mgr.load(config_path)) {
        Logger::instance().warn("Could not load config from {}, using defaults", config_path);
    }

    const auto& config = config_mgr.get_config();
    Logger::instance().debug("Bar height: {}", config.bar.height);
    Logger::instance().debug("Widgets configured: {}", config.widgets.size());

    // Initialize event loop
    try {
        event_loop = std::make_unique<EventLoop>();
        Logger::instance().debug("Event loop initialized");
    } catch (const std::exception& e) {
        Logger::instance().error("Failed to create event loop: {}", e.what());
        return 1;
    }

    // Connect to Wayland display
    display = wl_display_connect(nullptr);
    if (!display) {
        Logger::instance().error("Could not connect to Wayland display");
        return 1;
    }

    Logger::instance().info("Connected to Wayland display");

    // Get registry and add listener
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, nullptr);

    // Roundtrip to get all globals
    wl_display_roundtrip(display);

    if (!compositor) {
        Logger::instance().error("Could not bind compositor");
        wl_display_disconnect(display);
        return 1;
    }

    Logger::instance().info("Compositor bound successfully");

    // Create a surface
    surface = wl_compositor_create_surface(compositor);
    if (!surface) {
        Logger::instance().error("Could not create surface");
        wl_display_disconnect(display);
        return 1;
    }

    Logger::instance().info("Surface created");

    // Add Wayland display fd to event loop
    int wayland_fd = wl_display_get_fd(display);
    event_loop->add_fd(wayland_fd, EPOLLIN, [](int /*fd*/, uint32_t /*events*/) {
        if (wl_display_dispatch(display) < 0) {
            Logger::instance().error("Wayland dispatch error");
            event_loop->shutdown();
        }
    });

    // Add a test timer (prints every 2 seconds)
    event_loop->add_timer(std::chrono::milliseconds(2000), []() {
        Logger::instance().debug("Heartbeat - bar running");
    });

    Logger::instance().info("Event loop starting...");

    // Main event loop
    while (event_loop->dispatch()) {
        // Prepare Wayland events
        while (wl_display_prepare_read(display) != 0) {
            wl_display_dispatch_pending(display);
        }
        wl_display_flush(display);
        wl_display_read_events(display);
        wl_display_dispatch_pending(display);
    }

    Logger::instance().info("Shutting down...");

    // Cleanup
    if (surface) wl_surface_destroy(surface);
    if (compositor) wl_compositor_destroy(compositor);
    if (registry) wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return 0;
}
