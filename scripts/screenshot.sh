#!/bin/bash
# Generate screenshot by rendering directly from hyprbar
# Usage: ./scripts/screenshot.sh [output.png]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT="${1:-$PROJECT_DIR/docs/screenshot.png}"

cd "$PROJECT_DIR"

# Ensure build is up to date
make -s

# Generate temporary C++ screenshot program
cat > /tmp/hyprbar_screenshot.cpp << 'EOF'
#include "hyprbar/core/config_manager.h"
#include "hyprbar/rendering/renderer.h"
#include "hyprbar/widgets/widget_manager.h"
#include <cairo/cairo.h>

using namespace hyprbar;

int main(int argc, char** argv) {
    const char* output = argc > 1 ? argv[1] : "/tmp/screenshot.png";
    
    // Load config
    ConfigManager config_mgr;
    std::string config_path = ConfigManager::get_default_config_path();
    config_mgr.load(config_path);
    const auto& config = config_mgr.get_config();
    
    // Initialize renderer
    Renderer renderer;
    uint32_t width = 1920;
    uint32_t height = config.bar.height;
    renderer.initialize(width, height);
    
    // Initialize widgets
    WidgetManager widget_mgr;
    widget_mgr.initialize(config_mgr);
    
    // Trigger widget updates
    widget_mgr.update();
    
    // Render frame
    renderer.begin_frame();
    
    Color bg = Color::from_hex(config.bar.background);
    renderer.clear(bg);
    
    widget_mgr.render(renderer, width, height);
    
    renderer.end_frame();
    
    // Save to PNG
    cairo_surface_write_to_png(cairo_get_target(renderer.get_context()), output);
    
    return 0;
}
EOF

# Compile screenshot tool
clang++ -std=c++17 -Iinclude /tmp/hyprbar_screenshot.cpp \
    build/core/config_manager.o \
    build/core/logger.o \
    build/rendering/renderer.o \
    build/widgets/widget_manager.o \
    build/widgets/clock_widget.o \
    build/widgets/script_widget.o \
    -lcairo -lpango-1.0 -lpangocairo-1.0 -lgobject-2.0 -lglib-2.0 \
    -o /tmp/hyprbar_screenshot 2>&1 | grep -v "warning:" || true

# Run and generate screenshot
/tmp/hyprbar_screenshot "$OUTPUT"

# Cleanup
rm /tmp/hyprbar_screenshot.cpp /tmp/hyprbar_screenshot

echo "✓ Screenshot saved to: $OUTPUT"
file "$OUTPUT"
