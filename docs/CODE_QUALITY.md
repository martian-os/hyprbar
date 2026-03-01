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

## Refactoring Progress

### ✅ Fixed (1/7):
1. **`main.cpp:main()`** - Extracted helper functions, now passes limits

### ⏳ Remaining (6/7):
2. **`wayland_manager.cpp:create_bar_surface()`** (100+ lines)
3. **`wayland_manager.cpp:~WaylandManager()`** (55+ lines)
4. **`config_manager.cpp:parse_string()`** (129+ lines) - **DEFER**: JSON parser complexity
5. **`event_loop.cpp:process_timers()`** (104+ lines)
6. **`renderer.cpp:draw_text()`** (126+ lines)
7. **`wayland_manager.cpp:pointer_handle_button()`** (321+ lines)

**Note on config_manager:** The JSON parser is intentionally complex - refactoring would create more problems than it solves. Consider exempting parser code from limits or using a library.

**Action Required:** Fix 2, 3, 5, 6, 7 before Phase 4.

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
