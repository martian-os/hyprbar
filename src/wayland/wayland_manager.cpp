#include "hyprbar/wayland/wayland_manager.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/wayland/layer_shell_protocol.h"
#include <cairo/cairo.h>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

namespace hyprbar {

static const wl_registry_listener registry_listener = {
    WaylandManager::registry_handle_global,
    WaylandManager::registry_handle_global_remove,
};

static const wl_seat_listener seat_listener = {
    WaylandManager::seat_handle_capabilities,
    WaylandManager::seat_handle_name,
};

static const wl_pointer_listener pointer_listener = {
    WaylandManager::pointer_handle_enter,
    WaylandManager::pointer_handle_leave,
    WaylandManager::pointer_handle_motion,
    WaylandManager::pointer_handle_button,
    WaylandManager::pointer_handle_axis,
    WaylandManager::pointer_handle_frame,
    WaylandManager::pointer_handle_axis_source,
    WaylandManager::pointer_handle_axis_stop,
    WaylandManager::pointer_handle_axis_discrete,
};

static const zwlr_layer_surface_v1_listener layer_surface_listener = {
    WaylandManager::layer_surface_handle_configure,
    WaylandManager::layer_surface_handle_closed,
};

WaylandManager::WaylandManager()
    : display_(nullptr)
    , registry_(nullptr)
    , compositor_(nullptr)
    , shm_(nullptr)
    , seat_(nullptr)
    , pointer_(nullptr)
    , surface_(nullptr)
    , layer_shell_(nullptr)
    , layer_surface_(nullptr)
    , pointer_x_(0)
    , pointer_y_(0)
{
}

WaylandManager::~WaylandManager() {
    cleanup();
    
    if (display_) {
        wl_display_disconnect(display_);
        display_ = nullptr;
    }
}

void WaylandManager::cleanup() {
    if (pointer_) {
        wl_pointer_destroy(pointer_);
        pointer_ = nullptr;
    }
    if (seat_) {
        wl_seat_destroy(seat_);
        seat_ = nullptr;
    }
    if (layer_surface_) {
        zwlr_layer_surface_v1_destroy(layer_surface_);
        layer_surface_ = nullptr;
    }
    if (surface_) {
        wl_surface_destroy(surface_);
        surface_ = nullptr;
    }
    if (layer_shell_) {
        zwlr_layer_shell_v1_destroy(layer_shell_);
        layer_shell_ = nullptr;
    }
    if (shm_) {
        wl_shm_destroy(shm_);
        shm_ = nullptr;
    }
    if (compositor_) {
        wl_compositor_destroy(compositor_);
        compositor_ = nullptr;
    }
    if (registry_) {
        wl_registry_destroy(registry_);
        registry_ = nullptr;
    }
}

bool WaylandManager::initialize() {
    display_ = wl_display_connect(nullptr);
    if (!display_) {
        Logger::instance().error("Could not connect to Wayland display");
        return false;
    }

    Logger::instance().info("Connected to Wayland display");

    registry_ = wl_display_get_registry(display_);
    wl_registry_add_listener(registry_, &registry_listener, this);

    wl_display_roundtrip(display_);

    if (!compositor_) {
        Logger::instance().error("Compositor not available");
        return false;
    }

    if (!layer_shell_) {
        Logger::instance().error("Layer shell not available");
        return false;
    }

    if (!shm_) {
        Logger::instance().error("Shared memory not available");
        return false;
    }

    Logger::instance().info("Wayland protocols bound successfully");
    return true;
}

bool WaylandManager::create_bar_surface(BarPosition position, uint32_t width, uint32_t height) {
    if (!compositor_ || !layer_shell_) {
        Logger::instance().error("Cannot create surface: missing protocols");
        return false;
    }

    surface_ = wl_compositor_create_surface(compositor_);
    if (!surface_) {
        Logger::instance().error("Failed to create surface");
        return false;
    }

    uint32_t layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    layer_surface_ = zwlr_layer_shell_v1_get_layer_surface(
        layer_shell_, surface_, nullptr, layer, "hyprbar");

    if (!layer_surface_) {
        Logger::instance().error("Failed to create layer surface");
        wl_surface_destroy(surface_);
        surface_ = nullptr;
        return false;
    }

    configure_layer_surface(position, width, height);
    zwlr_layer_surface_v1_add_listener(layer_surface_, &layer_surface_listener, this);

    wl_surface_commit(surface_);
    wl_display_roundtrip(display_);

    Logger::instance().info("Bar surface created");
    return true;
}

uint32_t WaylandManager::calculate_anchor(BarPosition position) const {
    switch (position) {
        case BarPosition::Top:
            return ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        case BarPosition::Bottom:
            return ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        case BarPosition::Left:
            return ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        case BarPosition::Right:
            return ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                   ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    }
    return 0;
}

void WaylandManager::configure_layer_surface(BarPosition position, uint32_t width, uint32_t height) {
    uint32_t anchor = calculate_anchor(position);
    zwlr_layer_surface_v1_set_anchor(layer_surface_, anchor);
    
    switch (position) {
        case BarPosition::Top:
        case BarPosition::Bottom:
            zwlr_layer_surface_v1_set_size(layer_surface_, 0, height);
            break;
        case BarPosition::Left:
        case BarPosition::Right:
            zwlr_layer_surface_v1_set_size(layer_surface_, width, 0);
            break;
    }
}

void WaylandManager::set_exclusive_zone(uint32_t size) {
    if (layer_surface_) {
        zwlr_layer_surface_v1_set_exclusive_zone(layer_surface_, size);
        wl_surface_commit(surface_);
    }
}

int WaylandManager::get_fd() const {
    return display_ ? wl_display_get_fd(display_) : -1;
}

int WaylandManager::dispatch() {
    return display_ ? wl_display_dispatch(display_) : -1;
}

int WaylandManager::prepare_read() {
    return display_ ? wl_display_prepare_read(display_) : -1;
}

int WaylandManager::flush() {
    return display_ ? wl_display_flush(display_) : -1;
}

int WaylandManager::read_events() {
    return display_ ? wl_display_read_events(display_) : -1;
}

int WaylandManager::dispatch_pending() {
    return display_ ? wl_display_dispatch_pending(display_) : -1;
}

wl_buffer* WaylandManager::create_buffer(size_t size, void** data) {
    // Create anonymous file
    int fd = memfd_create("hyprbar-buffer", MFD_CLOEXEC);
    if (fd < 0) {
        Logger::instance().error("Failed to create memfd: {}", strerror(errno));
        return nullptr;
    }

    // Set size
    if (ftruncate(fd, size) < 0) {
        Logger::instance().error("Failed to truncate memfd: {}", strerror(errno));
        close(fd);
        return nullptr;
    }

    // Map memory
    void* pool_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pool_data == MAP_FAILED) {
        Logger::instance().error("Failed to mmap buffer: {}", strerror(errno));
        close(fd);
        return nullptr;
    }

    // Get configured dimensions (set by layer_surface_configure)
    uint32_t width = 1920;  // Default, should be updated by configure
    uint32_t height = 30;
    uint32_t stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

    // Create wl_shm_pool
    wl_shm_pool* pool = wl_shm_create_pool(shm_, fd, size);
    close(fd);  // Can close fd after creating pool

    // Create buffer from pool with proper dimensions
    wl_buffer* buffer = wl_shm_pool_create_buffer(pool, 0, 
        width, height, stride, WL_SHM_FORMAT_ARGB8888);

    wl_shm_pool_destroy(pool);

    if (data) {
        *data = pool_data;
    }

    Logger::instance().debug("Created wl_buffer: {} bytes ({}x{}, stride={})", size, width, height, stride);
    return buffer;
}

void WaylandManager::attach_and_commit(wl_buffer* buffer) {
    wl_surface_attach(surface_, buffer, 0, 0);
    wl_surface_damage_buffer(surface_, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(surface_);
    Logger::instance().debug("Buffer attached and surface committed");
}

// Registry callbacks
void WaylandManager::registry_handle_global(void* data, wl_registry* registry,
                                           uint32_t name, const char* interface,
                                           uint32_t version) {
    auto* wm = static_cast<WaylandManager*>(data);

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        wm->compositor_ = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 4));
        Logger::instance().debug("Bound compositor");
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        wm->shm_ = static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
        Logger::instance().debug("Bound shm");
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        wm->seat_ = static_cast<wl_seat*>(
            wl_registry_bind(registry, name, &wl_seat_interface, 5));
        wl_seat_add_listener(wm->seat_, &seat_listener, wm);
        Logger::instance().debug("Bound seat");
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        wm->layer_shell_ = static_cast<zwlr_layer_shell_v1*>(
            wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1));
        Logger::instance().debug("Bound layer shell");
    }
}

void WaylandManager::registry_handle_global_remove(void* /*data*/, wl_registry* /*registry*/,
                                                   uint32_t /*name*/) {
    // Handle global removal if needed
}

// Seat callbacks
void WaylandManager::seat_handle_capabilities(void* data, wl_seat* seat,
                                             uint32_t capabilities) {
    auto* wm = static_cast<WaylandManager*>(data);

    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        if (!wm->pointer_) {
            wm->pointer_ = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(wm->pointer_, &pointer_listener, wm);
            Logger::instance().debug("Pointer capability available");
        }
    } else {
        if (wm->pointer_) {
            wl_pointer_destroy(wm->pointer_);
            wm->pointer_ = nullptr;
        }
    }
}

void WaylandManager::seat_handle_name(void* /*data*/, wl_seat* /*seat*/, const char* /*name*/) {
    // Optional: log seat name
}

// Pointer callbacks
void WaylandManager::pointer_handle_enter(void* /*data*/, wl_pointer* /*pointer*/,
                                         uint32_t /*serial*/, wl_surface* /*surface*/,
                                         wl_fixed_t /*x*/, wl_fixed_t /*y*/) {
}

void WaylandManager::pointer_handle_leave(void* /*data*/, wl_pointer* /*pointer*/,
                                         uint32_t /*serial*/, wl_surface* /*surface*/) {
}

void WaylandManager::pointer_handle_motion(void* data, wl_pointer* /*pointer*/,
                                          uint32_t /*time*/, wl_fixed_t x, wl_fixed_t y) {
    auto* wm = static_cast<WaylandManager*>(data);
    wm->pointer_x_ = wl_fixed_to_int(x);
    wm->pointer_y_ = wl_fixed_to_int(y);

    if (wm->pointer_motion_callback_) {
        wm->pointer_motion_callback_(wm->pointer_x_, wm->pointer_y_);
    }
}

void WaylandManager::pointer_handle_button(void* data, wl_pointer* /*pointer*/,
                                          uint32_t /*serial*/, uint32_t /*time*/,
                                          uint32_t button, uint32_t state) {
    auto* wm = static_cast<WaylandManager*>(data);

    if (wm->pointer_button_callback_) {
        wm->pointer_button_callback_(button, state, wm->pointer_x_, wm->pointer_y_);
    }
}

void WaylandManager::pointer_handle_axis(void* /*data*/, wl_pointer* /*pointer*/,
                                        uint32_t /*time*/, uint32_t /*axis*/, wl_fixed_t /*value*/) {
}

void WaylandManager::pointer_handle_frame(void* /*data*/, wl_pointer* /*pointer*/) {
}

void WaylandManager::pointer_handle_axis_source(void* /*data*/, wl_pointer* /*pointer*/,
                                               uint32_t /*source*/) {
}

void WaylandManager::pointer_handle_axis_stop(void* /*data*/, wl_pointer* /*pointer*/,
                                              uint32_t /*time*/, uint32_t /*axis*/) {
}

void WaylandManager::pointer_handle_axis_discrete(void* /*data*/, wl_pointer* /*pointer*/,
                                                  uint32_t /*axis*/, int32_t /*discrete*/) {
}

// Layer surface callbacks
void WaylandManager::layer_surface_handle_configure(void* /*data*/,
                                                   zwlr_layer_surface_v1* surface,
                                                   uint32_t serial, uint32_t width,
                                                   uint32_t height) {
    zwlr_layer_surface_v1_ack_configure(surface, serial);
    Logger::instance().debug("Layer surface configured: {}x{}", width, height);
}

void WaylandManager::layer_surface_handle_closed(void* /*data*/,
                                                zwlr_layer_surface_v1* /*surface*/) {
    Logger::instance().warn("Layer surface closed");
}

}  // namespace hyprbar
