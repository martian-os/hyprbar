#!/bin/bash
# Simple test script to render clock widget
# Used for debugging widget rendering

cat > /tmp/simple_test.cpp << 'EOF'
#include "hyprbar/widgets/clock_widget.h"
#include "hyprbar/rendering/renderer.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/logger.h"
#include <cairo/cairo.h>
#include <iostream>

using namespace hyprbar;

int main() {
    Logger::instance().set_level(Logger::Level::Debug);
    
    // Simple clock widget
    ClockWidget clock;
    ConfigValue config;
    config.type = ConfigValue::Type::Object;
    
    clock.initialize(config);
    clock.update();
    
    std::cout << "Clock desired width: " << clock.get_desired_width() << std::endl;
    
    Renderer r;
    r.initialize(400, 100);
    r.begin_frame();
    r.clear(Color{1.0, 1.0, 1.0, 1.0});
    
    std::cout << "Rendering clock..." << std::endl;
    clock.render(r, 10, 50, 200, 100);
    
    // Also manually draw text
    Color black{0.0, 0.0, 0.0, 1.0};
    r.draw_text("MANUAL TEXT", 10, 80, "sans-serif", 20, black);
    
    r.end_frame();
    cairo_surface_write_to_png(cairo_get_target(r.get_context()), "/tmp/clock-test.png");
    std::cout << "Saved to /tmp/clock-test.png" << std::endl;
    
    return 0;
}
EOF

cd ~/.openclaw/workspace/hyprbar

clang++ -std=c++17 -Iinclude /tmp/simple_test.cpp \
    build/core/logger.o build/rendering/renderer.o build/widgets/clock_widget.o \
    -lcairo -lpango-1.0 -lpangocairo-1.0 -lgobject-2.0 -lglib-2.0 \
    -o /tmp/simple_test 2>&1 | grep -v warning || true

/tmp/simple_test
