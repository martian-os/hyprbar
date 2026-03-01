#include "hyprbar/core/event_loop.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/wayland/wayland_manager.h"
#include "hyprbar/rendering/renderer.h"
#include <iostream>
#include <memory>
#include <sys/epoll.h>
#include <csignal>
#include <ctime>
#include <cstring>

using namespace hyprbar;

static std::unique_ptr<EventLoop> event_loop = nullptr;
static std::unique_ptr<WaylandManager> wayland = nullptr;
static std::unique_ptr<Renderer> renderer = nullptr;
static wl_buffer* buffer = nullptr;
static void* buffer_data = nullptr;  // Mapped memory for Wayland buffer
static uint32_t bar_width = 1920;  // Will be set by layer surface configure

void signal_handler(int /*sig*/) {
    Logger::instance().info("Received shutdown signal");
    if (event_loop) {
        event_loop->shutdown();
    }
}

void render_frame(const Config& config) {
    if (!renderer || !wayland || !buffer || !buffer_data) {
        return;
    }

    renderer->begin_frame();

    // Draw background
    Color bg_color = Color::from_hex(config.bar.background);
    renderer->clear(bg_color);

    // Draw some test content
    Color fg_color = Color::from_hex(config.bar.foreground);
    
    // Left side - "Hyprbar"
    renderer->draw_text("Hyprbar v0.1.0", 10, config.bar.height / 2 + 5,
                       "monospace", 14, fg_color);

    // Center - Time
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    
    cairo_text_extents_t extents;
    cairo_text_extents(renderer->get_context(), time_buf, &extents);
    double center_x = (bar_width - extents.width) / 2.0;
    renderer->draw_text(time_buf, center_x, config.bar.height / 2 + 5,
                       "monospace", 14, fg_color);

    // Right side - Status
    std::string status = "Phase 3: Rendering Complete ✓";
    cairo_text_extents(renderer->get_context(), status.c_str(), &extents);
    double right_x = bar_width - extents.width - 10;
    renderer->draw_text(status, right_x, config.bar.height / 2 + 5,
                       "monospace", 14, fg_color);

    renderer->end_frame();

    // Copy Cairo buffer to Wayland buffer
    std::memcpy(buffer_data, renderer->get_buffer_data(), renderer->get_buffer_size());

    // Commit to Wayland
    wayland->attach_and_commit(buffer);
}

int main(int /*argc*/, char** /*argv*/) {
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize logger
    Logger::instance().set_level(Logger::Level::Debug);
    Logger::instance().info("Hyprbar v0.1.0 starting...");

    // Load configuration
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

    // Initialize Wayland
    wayland = std::make_unique<WaylandManager>();
    if (!wayland->initialize()) {
        Logger::instance().error("Wayland initialization failed");
        return 1;
    }

    // Create bar surface
    WaylandManager::BarPosition position;
    switch (config.bar.position) {
        case BarConfig::Position::Top:    position = WaylandManager::BarPosition::Top; break;
        case BarConfig::Position::Bottom: position = WaylandManager::BarPosition::Bottom; break;
        case BarConfig::Position::Left:   position = WaylandManager::BarPosition::Left; break;
        case BarConfig::Position::Right:  position = WaylandManager::BarPosition::Right; break;
    }

    if (!wayland->create_bar_surface(position, 0, config.bar.height)) {
        Logger::instance().error("Failed to create bar surface");
        return 1;
    }

    // Set exclusive zone
    wayland->set_exclusive_zone(config.bar.height);

    // Initialize renderer
    renderer = std::make_unique<Renderer>();
    if (!renderer->initialize(bar_width, config.bar.height)) {
        Logger::instance().error("Failed to initialize renderer");
        return 1;
    }

    // Create Wayland buffer
    buffer = wayland->create_buffer(renderer->get_buffer_size(), &buffer_data);
    if (!buffer || !buffer_data) {
        Logger::instance().error("Failed to create Wayland buffer");
        return 1;
    }

    // Initial render
    render_frame(config);

    // Add Wayland fd to event loop
    int wayland_fd = wayland->get_fd();
    event_loop->add_fd(wayland_fd, EPOLLIN, [](int /*fd*/, uint32_t /*events*/) {
        if (wayland->dispatch() < 0) {
            Logger::instance().error("Wayland dispatch error");
            event_loop->shutdown();
        }
    });

    // Add render timer (update every second for clock)
    event_loop->add_timer(std::chrono::milliseconds(1000), [&config]() {
        render_frame(config);
    });

    Logger::instance().info("Event loop starting...");

    // Main event loop
    while (event_loop->dispatch()) {
        // Prepare Wayland events
        while (wayland->prepare_read() != 0) {
            wayland->dispatch_pending();
        }
        wayland->flush();
        wayland->read_events();
        wayland->dispatch_pending();
    }

    Logger::instance().info("Shutting down...");
    return 0;
}
