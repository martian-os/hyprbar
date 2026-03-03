# Code Coverage Report

**Overall Coverage:** 62.7% (2433/3880 lines)

## By Component

### Core (Config & Logging)
- `config_manager.cpp`: 64.2% (279 lines)
- `logger.cpp`: 69.1% (55 lines)

### Rendering
- `renderer.cpp`: 69.1% (110 lines)

### Widgets
- `script_widget.cpp`: 78.9% (95 lines) ✅ Good
- `tray_widget.cpp`: 84.7% (59 lines) ✅ Excellent
- `hyprland_widget.cpp`: 28.5% (151 lines) ⚠️ Needs work

### Wayland Integration
- `wayland_manager.cpp`: 0.0% (220 lines) ❌ Not tested
- `wayland_surface.cpp`: 0.0% (25 lines) ❌ Not tested
- `event_loop.cpp`: 0.0% (113 lines) ❌ Not tested

### Widget Management
- `widget_manager.cpp`: 0.0% (174 lines) ❌ Not tested (runs in main, not unit tests)

## Test Coverage
All test files have 100% coverage:
- `basic_test.cpp`: 100%
- `config_test.cpp`: 100%
- `renderer_test.cpp`: 100%
- `style_test.cpp`: 100%
- `test_bar_config.cpp`: 100%
- `widget_test.cpp`: 100%

## Recommendations

### High Priority (Easy Wins)
1. **Widget Manager**: Add unit tests for layout logic (currently 0%)
2. **Hyprland Widget**: Increase from 28.5% to >70%
   - Test workspace parsing
   - Test event handling
   - Mock Hyprland socket communication

### Medium Priority
3. **Wayland Integration**: Add integration tests
   - Mock wayland compositor
   - Test surface creation/destruction
   - Test event handling

### Future
4. Consider integration tests for main event loop
5. Add screenshot-based visual regression tests

## How to Generate This Report

```bash
# Run coverage analysis
make coverage

# View summary
lcov --summary coverage/coverage.info --ignore-errors inconsistent

# Generate HTML report
make coverage-report
# Opens: coverage/html/index.html

# Clean coverage data
make coverage-clean
```
