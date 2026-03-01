#include "hyprbar/core/event_loop.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/wayland/wayland_manager.h"
#include <iostream>
#include <memory>
#include <sys/epoll.h>
#include <csignal>

using namespace hyprbar;

static std::unique_ptr<EventLoop> event_loop = nullptr;
static std::unique_ptr<WaylandManager> wayland = nullptr;

void signal_handler(int /*sig*/) {
    Logger::instance().info("Received shutdown signal");
    if (event_loop) {
        event_loop->shutdown();
    }
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

    // Add Wayland fd to event loop
    int wayland_fd = wayland->get_fd();
    event_loop->add_fd(wayland_fd, EPOLLIN, [](int /*fd*/, uint32_t /*events*/) {
        if (wayland->dispatch() < 0) {
            Logger::instance().error("Wayland dispatch error");
            event_loop->shutdown();
        }
    });

    // Add heartbeat timer
    event_loop->add_timer(std::chrono::milliseconds(5000), []() {
        Logger::instance().debug("Bar running (heartbeat)");
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
