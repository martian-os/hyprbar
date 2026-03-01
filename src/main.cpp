#include <iostream>
#include <wayland-client.h>
#include <string>

// Global Wayland objects
static struct wl_display *display = nullptr;
static struct wl_compositor *compositor = nullptr;
static struct wl_surface *surface = nullptr;

// Registry listener to bind Wayland globals
static void registry_handle_global(void *data, struct wl_registry *registry,
                                   uint32_t name, const char *interface,
                                   uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = static_cast<struct wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 4));
    }
}

static void registry_handle_global_remove(void *data,
                                          struct wl_registry *registry,
                                          uint32_t name) {
    // Handle global removal if needed
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

int main(int argc, char *argv[]) {
    std::cout << "Hyprbar v0.1.0 - Wayland compositor bar" << std::endl;

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

    // Create a surface (basic setup)
    surface = wl_compositor_create_surface(compositor);
    if (!surface) {
        std::cerr << "Error: Could not create surface" << std::endl;
        wl_display_disconnect(display);
        return 1;
    }

    std::cout << "Surface created - Hyprbar initialized" << std::endl;

    // Cleanup
    if (surface) wl_surface_destroy(surface);
    if (compositor) wl_compositor_destroy(compositor);
    if (registry) wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return 0;
}
