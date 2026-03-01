#include "hyprbar/core/event_loop.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/wayland/wayland_manager.h"
#include "hyprbar/rendering/renderer.h"
#include "hyprbar/widgets/widget_manager.h"
#include <iostream>
#include <memory>
#include <cstring>
#include <sys/epoll.h>
#include <csignal>

using namespace hyprbar;

// Application state
struct AppState {
    std::unique_ptr<EventLoop> event_loop;
    std::unique_ptr<WaylandManager> wayland;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<WidgetManager> widget_manager;
    wl_buffer* buffer = nullptr;
    void* buffer_data = nullptr;
    uint32_t bar_width = 1920;
    uint32_t bar_height = 32;
};

static AppState app;

void signal_handler(int /*sig*/) {
    Logger::instance().info("Received shutdown signal");
    if (app.event_loop) {
        app.event_loop->shutdown();
    }
}

void render_frame(const Config& config) {
    if (!app.renderer || !app.wayland || !app.buffer || !app.buffer_data) {
        return;
    }

    app.renderer->begin_frame();

    Color bg = Color::from_hex(config.bar.background);
    app.renderer->clear(bg);

    // Render widgets
    if (app.widget_manager) {
        app.widget_manager->render(*app.renderer, app.bar_width, app.bar_height);
    }

    app.renderer->end_frame();

    memcpy(app.buffer_data, app.renderer->get_buffer_data(),
                app.renderer->get_buffer_size());
    app.wayland->attach_and_commit(app.buffer);
}

bool initialize_wayland(const Config& config) {
    app.wayland = std::make_unique<WaylandManager>();
    if (!app.wayland->initialize()) {
        Logger::instance().error("Wayland initialization failed");
        return false;
    }

    WaylandManager::BarPosition position;
    switch (config.bar.position) {
        case BarConfig::Position::Top:    position = WaylandManager::BarPosition::Top; break;
        case BarConfig::Position::Bottom: position = WaylandManager::BarPosition::Bottom; break;
        case BarConfig::Position::Left:   position = WaylandManager::BarPosition::Left; break;
        case BarConfig::Position::Right:  position = WaylandManager::BarPosition::Right; break;
    }

    if (!app.wayland->create_bar_surface(position, 0, config.bar.height)) {
        Logger::instance().error("Failed to create bar surface");
        return false;
    }

    app.wayland->set_exclusive_zone(config.bar.height);
    app.bar_height = config.bar.height;
    return true;
}

bool initialize_renderer(const Config& config) {
    app.renderer = std::make_unique<Renderer>();
    if (!app.renderer->initialize(app.bar_width, config.bar.height)) {
        Logger::instance().error("Failed to initialize renderer");
        return false;
    }

    app.buffer = app.wayland->create_buffer(
        app.renderer->get_buffer_size(), &app.buffer_data);
    if (!app.buffer || !app.buffer_data) {
        Logger::instance().error("Failed to create Wayland buffer");
        return false;
    }

    return true;
}

bool initialize_widgets(const ConfigManager& config_mgr) {
    app.widget_manager = std::make_unique<WidgetManager>();
    if (!app.widget_manager->initialize(config_mgr)) {
        Logger::instance().warn("No widgets initialized");
        return false;
    }
    return true;
}

void setup_event_loop(const Config& config) {
    int wayland_fd = app.wayland->get_fd();
    app.event_loop->add_fd(wayland_fd, EPOLLIN, [](int /*fd*/, uint32_t /*events*/) {
        if (app.wayland->dispatch() < 0) {
            Logger::instance().error("Wayland dispatch error");
            app.event_loop->shutdown();
        }
    });

    app.event_loop->add_timer(std::chrono::milliseconds(1000), [&config]() {
        if (app.widget_manager && app.widget_manager->update()) {
            render_frame(config);
        }
    });
}

int main(int /*argc*/, char** /*argv*/) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    Logger::instance().set_level(Logger::Level::Debug);
    Logger::instance().info("Hyprbar v0.1.0 starting...");

    ConfigManager config_mgr;
    std::string config_path = ConfigManager::get_default_config_path();
    
    if (!config_mgr.load(config_path)) {
        Logger::instance().warn("Could not load config from {}, using defaults", config_path);
    }

    const auto& config = config_mgr.get_config();
    Logger::instance().debug("Bar height: {}", config.bar.height);
    Logger::instance().debug("Widgets configured: {}", config.widgets.size());

    try {
        app.event_loop = std::make_unique<EventLoop>();
        Logger::instance().debug("Event loop initialized");
    } catch (const std::exception& e) {
        Logger::instance().error("Failed to create event loop: {}", e.what());
        return 1;
    }

    if (!initialize_wayland(config)) {
        return 1;
    }

    if (!initialize_renderer(config)) {
        return 1;
    }

    initialize_widgets(config_mgr);

    render_frame(config);
    setup_event_loop(config);

    Logger::instance().info("Event loop starting...");

    while (app.event_loop->dispatch()) {
        while (app.wayland->prepare_read() != 0) {
            app.wayland->dispatch_pending();
        }
        app.wayland->flush();
        app.wayland->read_events();
        app.wayland->dispatch_pending();
    }

    Logger::instance().info("Shutting down...");
    return 0;
}
