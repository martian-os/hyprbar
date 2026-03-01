# Hyprbar Architecture

## Overview

Hyprbar is a modular, extensible status bar for Wayland compositors, designed with clean architecture principles. The system is built around a plugin-based widget system with a clear separation between presentation, business logic, and external integrations.

## Design Principles

1. **Modularity** - Widgets are independent, hot-pluggable components
2. **Separation of Concerns** - Clear boundaries between rendering, data collection, and coordination
3. **Event-Driven** - Reactive updates based on system events and timers
4. **Performance** - Minimal CPU usage when idle, efficient rendering updates
5. **Extensibility** - Easy to add new widgets without modifying core
6. **Configuration** - Declarative JSON/TOML configuration

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Application Layer                        │
│  ┌────────────┐  ┌─────────────┐  ┌──────────────────────────┐ │
│  │ Config     │  │ Event Loop  │  │ Widget Manager           │ │
│  │ Loader     │  │ (epoll/io)  │  │ (lifecycle, positioning) │ │
│  └────────────┘  └─────────────┘  └──────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────┼─────────────────────────────────┐
│                       Widget Layer                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐  │
│  │ Clock    │ │ CPU      │ │ Network  │ │ Workspace        │  │
│  │ Widget   │ │ Widget   │ │ Widget   │ │ Widget           │  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘  │
│       │            │            │               │               │
│  ┌────▼────────────▼────────────▼───────────────▼───────────┐  │
│  │            Widget Base Interface                          │  │
│  │  - update()  - render()  - on_click()  - get_size()      │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────┼─────────────────────────────────┐
│                      Rendering Layer                            │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐ │
│  │ Cairo Renderer   │  │ Pango Text       │  │ Layout       │ │
│  │ (2D graphics)    │  │ (fonts, i18n)    │  │ Engine       │ │
│  └──────────────────┘  └──────────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────┼─────────────────────────────────┐
│                      Platform Layer                             │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐ │
│  │ Wayland Client   │  │ Layer Shell      │  │ wlr Protocols│ │
│  │ (core protocol)  │  │ (positioning)    │  │ (workspaces) │ │
│  └──────────────────┘  └──────────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────┼─────────────────────────────────┐
│                       Data Sources                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐  │
│  │ /proc    │ │ DBus     │ │ Netlink  │ │ Hyprland IPC     │  │
│  │ /sys     │ │ (system) │ │ (network)│ │ (compositor)     │  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Application Core

**Responsibility:** Lifecycle management, event loop, inter-component coordination

**Key Classes:**
- `Application` - Main entry point, initializes all systems
- `EventLoop` - epoll-based event dispatcher (Wayland events, timers, file descriptors)
- `ConfigManager` - Loads and validates configuration from JSON/TOML

**Interfaces:**
```cpp
class Application {
    void initialize();
    void run();
    void shutdown();
    EventLoop* event_loop();
    WidgetManager* widget_manager();
};

class EventLoop {
    void add_fd(int fd, EventHandler handler);
    void add_timer(milliseconds interval, TimerCallback callback);
    void dispatch();  // Main loop iteration
};
```

### 2. Widget System

**Responsibility:** Manages widget lifecycle, positioning, updates, and rendering

**Widget Base Interface:**
```cpp
class Widget {
public:
    virtual ~Widget() = default;
    
    // Lifecycle
    virtual void initialize(const Config& config) = 0;
    virtual void shutdown() = 0;
    
    // Update & Render
    virtual void update() = 0;  // Called when data changes
    virtual void render(cairo_t* cr, Rect bounds) = 0;
    
    // Sizing
    virtual Size get_preferred_size() const = 0;
    virtual Size get_minimum_size() const = 0;
    
    // Events
    virtual void on_click(Point pos, MouseButton button) = 0;
    virtual void on_scroll(ScrollDirection dir) = 0;
    virtual void on_hover(bool entered) = 0;
    
    // Properties
    virtual std::string get_name() const = 0;
    virtual UpdateFrequency get_update_frequency() const = 0;
};
```

**Widget Manager:**
```cpp
class WidgetManager {
public:
    void register_widget_factory(const std::string& type, WidgetFactory factory);
    Widget* create_widget(const std::string& type, const Config& config);
    
    void add_widget(std::unique_ptr<Widget> widget);
    void remove_widget(const std::string& name);
    
    void update_all();  // Trigger updates for all widgets
    void render_all(cairo_t* cr);  // Render to surface
    
    Widget* get_widget_at(Point pos);  // For click handling
};
```

### 3. Rendering System

**Responsibility:** Drawing widgets to Wayland surfaces using Cairo

**Key Components:**
- `Renderer` - High-level rendering coordinator
- `CairoContext` - Cairo surface and context management
- `LayoutEngine` - Widget positioning (horizontal/vertical boxes, margins, padding)
- `ThemeManager` - Colors, fonts, styling

**Rendering Pipeline:**
```cpp
class Renderer {
public:
    void begin_frame();
    void render_widget(Widget* widget, Rect bounds);
    void end_frame();
    void commit();  // Commit to Wayland surface
    
private:
    cairo_surface_t* surface_;
    cairo_t* cairo_ctx_;
    LayoutEngine layout_;
};

class LayoutEngine {
public:
    struct Layout {
        std::vector<Rect> widget_bounds;
        Size total_size;
    };
    
    Layout compute_layout(const std::vector<Widget*>& widgets, 
                         Size available_space,
                         LayoutDirection direction);
};
```

### 4. Wayland Integration

**Responsibility:** Wayland protocol handling, window management

**Key Protocols:**
- `wl_compositor` - Surface creation
- `wl_shm` - Shared memory buffers
- `zwlr_layer_shell_v1` - Bar positioning (top/bottom, exclusive zone)
- `wl_seat` / `wl_pointer` - Input handling
- `zwlr_foreign_toplevel_v1` - Window list (for workspace widget)

**Wayland Manager:**
```cpp
class WaylandManager {
public:
    void initialize();
    void create_bar_surface(BarPosition position, Size size);
    void set_exclusive_zone(uint32_t height);
    
    void handle_events();  // Process Wayland events
    
    // Callbacks
    void on_pointer_enter(Point pos);
    void on_pointer_motion(Point pos);
    void on_pointer_button(MouseButton button, ButtonState state);
    void on_pointer_scroll(ScrollDirection dir);
    
private:
    wl_display* display_;
    wl_compositor* compositor_;
    wl_surface* surface_;
    zwlr_layer_surface_v1* layer_surface_;
    wl_shm* shm_;
};
```

### 5. Widget Implementations

Each widget is self-contained with its own data source integration.

#### Clock Widget
```cpp
class ClockWidget : public Widget {
private:
    std::string format_;  // strftime format
    std::chrono::system_clock::time_point last_update_;
    std::string cached_text_;
    
    void update() override {
        auto now = std::chrono::system_clock::now();
        cached_text_ = format_time(now, format_);
    }
};
```

#### CPU Widget
```cpp
class CpuWidget : public Widget {
private:
    struct CpuStats {
        uint64_t user, nice, system, idle;
    };
    
    CpuStats last_stats_;
    float cpu_usage_;
    std::unique_ptr<ProcStatReader> proc_reader_;
    
    void update() override {
        auto current = proc_reader_->read_cpu_stats();
        cpu_usage_ = calculate_usage(last_stats_, current);
        last_stats_ = current;
    }
};
```

#### Workspace Widget (Hyprland-specific)
```cpp
class WorkspaceWidget : public Widget {
private:
    struct Workspace {
        int id;
        std::string name;
        bool active;
        bool urgent;
    };
    
    std::vector<Workspace> workspaces_;
    std::unique_ptr<HyprlandIPC> ipc_client_;
    
    void initialize(const Config& config) override {
        ipc_client_ = std::make_unique<HyprlandIPC>();
        ipc_client_->subscribe_events([this](const Event& e) {
            if (e.type == EventType::WorkspaceChanged) {
                this->update();
            }
        });
    }
};
```

#### Network Widget
```cpp
class NetworkWidget : public Widget {
private:
    struct NetworkInterface {
        std::string name;
        bool connected;
        std::string ip_address;
        uint64_t rx_bytes, tx_bytes;
    };
    
    std::vector<NetworkInterface> interfaces_;
    std::unique_ptr<NetlinkMonitor> netlink_;
    
    void initialize(const Config& config) override {
        netlink_ = std::make_unique<NetlinkMonitor>();
        netlink_->on_link_change([this]() { this->update(); });
    }
};
```

## Data Flow

### 1. Initialization
```
main() 
  → Application::initialize()
    → ConfigManager::load("~/.config/hyprbar/config.json")
    → WaylandManager::initialize()
    → WidgetManager::create_widgets_from_config()
      → Widget::initialize() for each widget
    → EventLoop::add_timers() for periodic widget updates
```

### 2. Update Cycle
```
EventLoop::dispatch()
  → Timer fires for widget update
    → WidgetManager::update_specific_widget()
      → Widget::update() (fetches new data)
      → Widget data changed
        → Renderer::render_frame()
          → LayoutEngine::compute_layout()
          → Renderer::render_widget() for each widget
          → cairo_surface_flush()
          → WaylandManager::commit_surface()
```

### 3. Input Handling
```
Wayland pointer event
  → WaylandManager::on_pointer_button()
    → WidgetManager::get_widget_at(click_position)
    → Widget::on_click()
      → Widget performs action (e.g., switch workspace)
      → Widget::update() to reflect change
      → Trigger re-render
```

## Configuration System

**Config Format (TOML):**
```toml
[bar]
position = "top"
height = 30
background = "#1e1e2e"
foreground = "#cdd6f4"
font = "JetBrainsMono Nerd Font 10"

[[widgets]]
type = "workspaces"
position = "left"

[widgets.config]
format = "{id}: {name}"
show_empty = false

[[widgets]]
type = "clock"
position = "center"

[widgets.config]
format = "%Y-%m-%d %H:%M:%S"
update_interval = 1000  # milliseconds

[[widgets]]
type = "cpu"
position = "right"

[widgets.config]
format = "CPU: {usage}%"
update_interval = 2000

[[widgets]]
type = "network"
position = "right"

[widgets.config]
interface = "wlan0"
show_icon = true
```

**Config Schema:**
```cpp
struct BarConfig {
    enum Position { Top, Bottom, Left, Right };
    Position position;
    uint32_t height;
    Color background;
    Color foreground;
    std::string font;
};

struct WidgetConfig {
    std::string type;
    enum Position { Left, Center, Right };
    Position position;
    nlohmann::json config;  // Widget-specific config
};

struct Config {
    BarConfig bar;
    std::vector<WidgetConfig> widgets;
};
```

## Plugin System (Future)

**Dynamic Widget Loading:**
```cpp
class WidgetPlugin {
public:
    virtual const char* get_name() const = 0;
    virtual const char* get_version() const = 0;
    virtual Widget* create_instance() = 0;
};

// Plugin loader
class PluginManager {
public:
    void load_plugin(const std::string& path);
    void unload_plugin(const std::string& name);
    
private:
    std::map<std::string, void*> loaded_plugins_;  // dlopen handles
};
```

**Plugin API:**
```cpp
extern "C" {
    WidgetPlugin* hyprbar_plugin_init() {
        return new MyCustomWidget();
    }
}
```

## Performance Considerations

### 1. Rendering Optimization
- **Damage Tracking:** Only re-render changed widgets
- **Layer Caching:** Cache static widget renders to surfaces
- **Dirty Regions:** Only commit damaged regions to Wayland

```cpp
class DamageTracker {
public:
    void mark_widget_dirty(Widget* widget);
    std::vector<Rect> get_dirty_regions();
    void clear();
};
```

### 2. Update Scheduling
- Different widgets have different update frequencies
- Clock: 1000ms
- CPU: 2000ms
- Network: 5000ms
- Workspaces: event-driven

```cpp
enum class UpdateFrequency {
    VeryHigh = 100,   // 100ms
    High = 500,       // 500ms
    Normal = 1000,    // 1s
    Low = 5000,       // 5s
    EventDriven = 0   // Only on external events
};
```

### 3. Thread Safety
- Main thread: Event loop, rendering
- Worker threads: Heavy data collection (optional)
- Lock-free queues for cross-thread communication

## Error Handling

**Strategy:**
- Fail fast during initialization (missing protocols, config errors)
- Graceful degradation for widget failures (show error icon, continue bar operation)
- Comprehensive logging with levels (DEBUG, INFO, WARN, ERROR)

```cpp
class Logger {
public:
    enum Level { Debug, Info, Warn, Error };
    void log(Level level, const std::string& message);
    void set_level(Level min_level);
};

// Usage
Logger::instance().log(Logger::Error, 
    "Failed to connect to Hyprland IPC, workspace widget disabled");
```

## Testing Strategy

### Unit Tests
- Widget logic (update calculations, formatting)
- Layout engine (positioning algorithms)
- Config parsing

### Integration Tests
- Mock Wayland compositor
- Widget lifecycle
- Event handling

### Performance Tests
- Frame time benchmarks (<16ms target for 60fps)
- Memory usage over time (leak detection)

## Technology Stack

**Core:**
- C++17 (std::optional, std::variant, structured bindings)
- Clang as primary compiler

**Libraries:**
- `wayland-client` - Wayland protocol
- `wayland-protocols` - Extended protocols
- `cairo` - 2D rendering
- `pango` / `pangocairo` - Text rendering, i18n
- `nlohmann/json` or `toml++` - Configuration parsing

**Optional:**
- `spdlog` - High-performance logging
- `doctest` or `catch2` - Testing framework
- `libdbus-1` - D-Bus integration (system notifications)

## Future Enhancements

1. **Theming System** - CSS-like styling
2. **Scripting** - Lua/Python for custom widgets
3. **Multi-Monitor** - Per-monitor bars with independent configs
4. **Animations** - Smooth transitions (workspace switching, widget updates)
5. **Tray Icons** - System tray protocol support
6. **Notifications** - Inline notification display
7. **Media Controls** - MPRIS integration

## Directory Structure

```
hyprbar/
├── src/
│   ├── core/
│   │   ├── application.cpp
│   │   ├── event_loop.cpp
│   │   └── config_manager.cpp
│   ├── widgets/
│   │   ├── widget.h (base interface)
│   │   ├── widget_manager.cpp
│   │   ├── clock_widget.cpp
│   │   ├── cpu_widget.cpp
│   │   ├── network_widget.cpp
│   │   └── workspace_widget.cpp
│   ├── rendering/
│   │   ├── renderer.cpp
│   │   ├── layout_engine.cpp
│   │   └── theme_manager.cpp
│   ├── wayland/
│   │   ├── wayland_manager.cpp
│   │   ├── layer_shell.cpp
│   │   └── input_handler.cpp
│   ├── data_sources/
│   │   ├── proc_reader.cpp
│   │   ├── netlink_monitor.cpp
│   │   └── hyprland_ipc.cpp
│   └── main.cpp
├── include/
│   └── hyprbar/
│       ├── core/
│       ├── widgets/
│       ├── rendering/
│       └── wayland/
├── tests/
│   ├── widget_tests.cpp
│   ├── layout_tests.cpp
│   └── config_tests.cpp
├── docs/
│   ├── ARCHITECTURE.md (this file)
│   ├── WIDGET_API.md
│   └── CONFIGURATION.md
├── examples/
│   └── config.toml
└── Makefile
```

---

This architecture provides a solid foundation for building a modern, performant, and extensible Wayland status bar. The modular design allows for easy addition of new widgets while maintaining clean separation of concerns.
