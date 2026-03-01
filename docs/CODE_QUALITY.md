# Code Quality Standards

## Enforced Limits (via .clang-tidy)

### Function Complexity
- **Maximum function length:** 50 lines
- **Maximum statements:** 40
- **Maximum parameters:** 5
- **Maximum branches:** 8
- **Maximum nesting depth:** 3
- **Cognitive complexity:** 15

### File Size
- **Maximum file length:** 300 lines

### Rationale
These limits force:
- **Single Responsibility:** Functions do one thing well
- **Readability:** Code fits in one screen
- **Testability:** Small functions are easier to test
- **Maintainability:** Less complexity = fewer bugs

## Current Violations (MUST be fixed before Phase 4)

### Functions exceeding complexity thresholds:

1. **`main.cpp:main()`** (75+ lines)
   - Extract: `initialize_wayland()`, `initialize_renderer()`, `setup_event_loop()`
   
2. **`wayland_manager.cpp:create_bar_surface()`** (100+ lines)
   - Extract: `configure_layer_surface()`, `set_surface_anchors()`
   
3. **`wayland_manager.cpp:~WaylandManager()`** (55+ lines)
   - Extract: `cleanup_wayland_objects()`, `cleanup_layer_shell()`
   
4. **`config_manager.cpp:parse_string()`** (129+ lines)
   - Extract: `parse_escape_sequence()`, `validate_string_end()`
   
5. **`event_loop.cpp:process_timers()`** (104+ lines)
   - Extract: `check_expired_timers()`, `reschedule_repeating()`
   
6. **`renderer.cpp:draw_text()`** (126+ lines)
   - Extract: `setup_font()`, `measure_text()`, `render_glyphs()`

7. **`wayland_manager.cpp:pointer_handle_button()`** (321+ lines)
   - Extract button handling callbacks

### Files exceeding 300 lines:
- `src/wayland/wayland_manager.cpp` (364 lines)
- `src/core/config_manager.cpp` (347 lines)

**Action Required:** Refactor before continuing to Phase 4.

## Running Linter

```bash
# Check all files
make lint

# Check specific file
clang-tidy src/main.cpp -- -Iinclude -std=c++17

# Auto-fix (use with caution)
clang-tidy src/main.cpp -fix -- -Iinclude -std=c++17
```

## Architecture Compliance

Verify architecture follows ARCHITECTURE.md:
1. **Layer separation:** Platform → Rendering → Widget → Application
2. **No circular dependencies**
3. **Interfaces over implementations**
4. **RAII for resource management**
