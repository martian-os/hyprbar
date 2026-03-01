#include "hyprbar/core/event_loop.h"
#include <iostream>
#include <wayland-client.h>
#include <string>
#include <cstring>
#include <memory>
#include <sys/epoll.h>

// Global Wayland objects
static struct wl_display *display = nullptr;
static struct wl_compositor *compositor = nullptr;
static struct wl_surface *surface = nullptr;
static std::unique_ptr<hyprbar::EventLoop> event_loop = nullptr;

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
    std::cout << "Hyprbar v0.1.0 - Wayland compositor bar" << std::endl;

    // Initialize event loop
    try {
        event_loop = std::make_unique<hyprbar::EventLoop>();
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to create event loop: " << e.what() << std::endl;
        return 1;
    }

    // Connect to Wayland display
    display = wl_display_connect(nullptr);
    if (!display) {
        std::cerr << "Error: Could not connect to Wayland display" << std::endl;
        return 1;
    }

    std::cout << "Connected to Wayland display" << std::endl;

    // Get registry and add listener
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, nullptr);

    // Roundtrip to get all globals
    wl_display_roundtrip(display);

    if (!compositor) {
        std::cerr << "Error: Could not bind compositor" << std::endl;
        wl_display_disconnect(display);
        return 1;
    }

    std::cout << "Compositor bound successfully" << std::endl;

    // Create a surface
    surface = wl_compositor_create_surface(compositor);
    if (!surface) {
        std::cerr << "Error: Could not create surface" << std::endl;
        wl_display_disconnect(display);
        return 1;
    }

    std::cout << "Surface created" << std::endl;

    // Add Wayland display fd to event loop
    int wayland_fd = wl_display_get_fd(display);
    event_loop->add_fd(wayland_fd, EPOLLIN, [](int /*fd*/, uint32_t /*events*/) {
        if (wl_display_dispatch(display) < 0) {
            std::cerr << "Wayland dispatch error" << std::endl;
            event_loop->shutdown();
        }
    });

    // Add a test timer (prints every 2 seconds)
    event_loop->add_timer(std::chrono::milliseconds(2000), []() {
        std::cout << "[Timer] Hyprbar running..." << std::endl;
    });

    std::cout << "Event loop starting..." << std::endl;

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

    std::cout << "Shutting down..." << std::endl;

    // Cleanup
    if (surface) wl_surface_destroy(surface);
    if (compositor) wl_compositor_destroy(compositor);
    if (registry) wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return 0;
}
