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

## Refactoring Status: DEFERRED

### Decision (2026-03-01)

After initial refactoring attempt, **deferring remaining violations** to maintain velocity:

**Rationale:**
1. **Working code is stable** - These functions work correctly and have been tested
2. **Parser complexity is inherent** - JSON/config parsers are naturally complex
3. **Refactoring risk** - Breaking working Wayland/event code introduces more risk than benefit
4. **Phase 4 priority** - Widget system is more valuable than cosmetic refactoring

### ✅ Fixed (1/7):
1. **`main.cpp:main()`** - Extracted helper functions ✓

### ⏳ Deferred (6/7):
2. **`wayland_manager.cpp:create_bar_surface()`** (100+ lines) - Wayland setup, stable
3. **`wayland_manager.cpp:~WaylandManager()`** (55+ lines) - Cleanup, works correctly
4. **`config_manager.cpp:parse_string()`** (129+ lines) - JSON parser, inherently complex
5. **`event_loop.cpp:process_timers()`** (104+ lines) - Timer logic, tested and working
6. **`renderer.cpp:draw_text()`** (126+ lines) - Cairo text rendering, stable
7. **`wayland_manager.cpp:pointer_handle_button()`** (321+ lines) - Input handling, functional

### Future Refactoring

**When to revisit:**
- When adding new features to these components
- When bugs are found requiring fixes
- When code becomes unmaintainable
- After Phase 6 (full widget system complete)

**Better approach for new code:**
- Apply 50-line limit to Phase 4+ code (Widget system)
- Use limits as guide, not blocker for working legacy code
- Prefer working code over perfectly structured broken code

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
