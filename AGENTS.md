# AGENTS.md - Hyprbar Development Guide

## Project Overview

**Hyprbar** is a modern Wayland status bar for Hyprland and other wlroots-based compositors. Built in C++17 with a modular, plugin-based architecture.

**Repository:** https://github.com/martian-os/hyprbar  
**Architecture:** See `/docs/ARCHITECTURE.md` for detailed design

## Documentation

- **docs/ARCHITECTURE.md** - System design, component breakdown, data flow
- **docs/WIDGET_API.md** - Widget development guide (TODO)
- **docs/CONFIGURATION.md** - User configuration reference (TODO)
- **docs/IMPLEMENTATION_STATUS.md** - Current implementation progress

## Development Workflow

### Before Starting Work

1. **Read the architecture** - Understand the layered design in `docs/ARCHITECTURE.md`
2. **Check implementation status** - See `docs/IMPLEMENTATION_STATUS.md` for what's done/in-progress
3. **Update status** - Mark components as "In Progress" before starting work

### Implementation Rules (MANDATORY)

**🚨 CRITICAL GUARDRAILS - DO NOT VIOLATE 🚨**

#### 1. Build Before Commit
```bash
make clean && make
```
**If build fails, DO NOT COMMIT.** Fix compilation errors first.

#### 2. Test Before Commit
```bash
make test
```
**If tests fail, DO NOT COMMIT.** All tests must pass.

#### 3. Verify Staged Files
```bash
git status
git diff --staged
```
**Check for:**
- No build artifacts (`build/`, `bin/`, `*.o`)
- No temporary files (`*.tmp`, `*.swp`, `.cache/`)
- Only files you intentionally created/modified

#### 4. Implementation Completeness
**NEVER commit half-finished implementations:**
- ❌ Function stubs without implementation
- ❌ Classes with empty methods marked "TODO"
- ❌ Broken functionality that "will be fixed later"
- ✅ Complete, working implementations (even if minimal)
- ✅ Functionality that compiles, links, and works

**If you can't complete it now, DON'T commit it. Keep it local.**

#### 5. Update Documentation
When implementing a component:
- ✅ Update `docs/IMPLEMENTATION_STATUS.md` to mark as "Complete"
- ✅ Add code comments explaining non-obvious logic
- ✅ Update README.md if user-facing features added

### Commit Message Format

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types:**
- `feat:` - New feature
- `fix:` - Bug fix
- `refactor:` - Code restructuring
- `docs:` - Documentation only
- `test:` - Test additions/changes
- `chore:` - Build, tooling, dependencies

**Examples:**
```
feat(widgets): Implement clock widget with strftime formatting
fix(wayland): Handle compositor disconnect gracefully
refactor(rendering): Extract layout engine from renderer
docs(architecture): Add widget API documentation
```

### Pre-Commit Checklist

Run this before EVERY commit:

```bash
# 1. Clean build
make clean && make

# 2. Run tests
make test

# 3. Check for artifacts
ls build/ bin/ 2>/dev/null && echo "⚠️  Build artifacts exist - check .gitignore"

# 4. Review staged changes
git status
git diff --staged

# 5. Verify completeness
# - Are all functions implemented (not stubs)?
# - Does it compile without warnings?
# - Do tests pass?
# - Is it ready for others to use?

# If ALL checks pass, commit:
git commit -m "feat(scope): description"
```

### Git Pre-Commit Hook

Install this hook to automate checks:

```bash
# Create .git/hooks/pre-commit
cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash
set -e

echo "🔍 Pre-commit checks..."

# Check for build artifacts in staged files
if git diff --cached --name-only | grep -qE '^(build/|bin/|.*\.o$)'; then
    echo "❌ Build artifacts in staged files!"
    exit 1
fi

# Build project
echo "🔨 Building..."
make clean && make > /dev/null 2>&1 || {
    echo "❌ Build failed!"
    exit 1
}

# Run tests
echo "🧪 Running tests..."
make test > /dev/null 2>&1 || {
    echo "❌ Tests failed!"
    exit 1
}

echo "✅ Pre-commit checks passed!"
EOF

chmod +x .git/hooks/pre-commit
```

## Implementation Strategy

### Phase 1: Core Foundation (CURRENT)
- [x] Project structure
- [x] Build system (Makefile)
- [x] CI/CD workflows
- [ ] Event loop (epoll-based)
- [ ] Config manager (JSON/TOML parsing)
- [ ] Logging system

### Phase 2: Wayland Integration
- [ ] Wayland client connection
- [ ] Compositor, shm, seat protocols
- [ ] Layer shell integration
- [ ] Surface creation and buffer management
- [ ] Input handling (pointer, keyboard)

### Phase 3: Rendering System
- [ ] Cairo surface setup
- [ ] Renderer class
- [ ] Layout engine (horizontal boxes)
- [ ] Theme manager (colors, fonts)
- [ ] Damage tracking

### Phase 4: Widget Framework
- [ ] Widget base interface
- [ ] Widget manager (lifecycle, positioning)
- [ ] Update scheduling system
- [ ] Event routing (clicks, scrolls)

### Phase 5: Core Widgets
- [ ] Clock widget
- [ ] CPU widget
- [ ] Memory widget
- [ ] Network widget
- [ ] Workspace widget (Hyprland IPC)

### Phase 6: Polish
- [ ] Configuration file support
- [ ] Error handling
- [ ] Documentation
- [ ] Examples

## Code Style

### Naming Conventions
- **Classes:** `PascalCase` (e.g., `WidgetManager`, `EventLoop`)
- **Functions/Methods:** `snake_case` (e.g., `get_size()`, `on_click()`)
- **Variables:** `snake_case` (e.g., `widget_count`, `last_update_`)
- **Private Members:** `snake_case_` with trailing underscore (e.g., `display_`, `surface_`)
- **Constants:** `kPascalCase` (e.g., `kDefaultHeight`)

### File Organization
```cpp
// header.h
#pragma once

#include <system_headers>
#include "project_headers.h"

namespace hyprbar {

class MyClass {
public:
    // Public interface
private:
    // Private implementation
};

}  // namespace hyprbar
```

### Error Handling
- Use exceptions for initialization failures
- Return `std::optional<T>` for operations that can fail
- Log errors with appropriate severity

```cpp
auto result = operation();
if (!result) {
    logger.error("Operation failed: {}", error);
    return std::nullopt;
}
```

## Dependencies

**Required:**
- clang (C++17 support)
- libwayland-client
- libwayland-protocols
- wlr-protocols
- cairo
- pango

**Installation (Ubuntu/Debian):**
```bash
sudo apt install clang libwayland-dev wayland-protocols \
    libcairo2-dev libpango1.0-dev
```

**For wlr-protocols (layer-shell):**
```bash
git clone https://gitlab.freedesktop.org/wlroots/wlr-protocols.git
cd wlr-protocols
meson build
sudo ninja -C build install
```

## Testing

### Unit Tests
- Each component has corresponding test file in `tests/`
- Tests run automatically in CI
- Use simple assertions, no heavy test framework initially

### Manual Testing
```bash
# Build and run
make && ./bin/hyprbar

# Run under Wayland compositor
# Should connect to display and create surface
```

## Common Pitfalls

### 1. Wayland Protocol Order
**Wrong:**
```cpp
surface = wl_compositor_create_surface(compositor);  // compositor not bound yet!
```

**Right:**
```cpp
registry = wl_display_get_registry(display);
wl_registry_add_listener(registry, &listener, nullptr);
wl_display_roundtrip(display);  // Wait for globals
// Now compositor is bound, safe to use
```

### 2. Memory Management
- Wayland objects require explicit cleanup
- Use RAII wrappers or ensure cleanup in destructors

```cpp
class WaylandManager {
public:
    ~WaylandManager() {
        if (surface_) wl_surface_destroy(surface_);
        if (compositor_) wl_compositor_destroy(compositor_);
        if (display_) wl_display_disconnect(display_);
    }
};
```

### 3. Cairo Surface Lifecycle
- Create Cairo surface from Wayland buffer
- Flush before committing to Wayland
- Don't leak surfaces

```cpp
cairo_surface_flush(cairo_surface);
wl_surface_attach(surface, buffer, 0, 0);
wl_surface_commit(surface);
```

## Debugging

### Enable Debug Build
```bash
make debug
```

### Wayland Debug
```bash
WAYLAND_DEBUG=1 ./bin/hyprbar
```

### Logging
Add debug output liberally during development:
```cpp
std::cout << "[DEBUG] Compositor bound: " << compositor << std::endl;
```

## Resources

- **Wayland Book:** https://wayland-book.com/
- **Cairo Documentation:** https://www.cairographics.org/documentation/
- **Pango Documentation:** https://docs.gtk.org/Pango/
- **wlroots protocols:** https://gitlab.freedesktop.org/wlroots/wlr-protocols
- **Layer Shell Protocol:** https://wayland.app/protocols/wlr-layer-shell-unstable-v1

## Questions?

If uncertain about:
- Architecture decisions → Read `docs/ARCHITECTURE.md`
- Implementation approach → Check existing code patterns
- Wayland protocols → Consult Wayland Book
- Commit readiness → Run pre-commit checklist

**When in doubt, ask before committing incomplete work.**

---

_This guide ensures quality, consistency, and prevents broken code from entering the repository._

## Generating Screenshots

Since hyprbar requires wlroots-based compositors (not available on GNOME), use the screenshot script to render directly from the Cairo renderer:

```bash
./scripts/screenshot.sh [output.png]
```

**How it works:**
1. Builds project if needed
2. Generates temporary C++ program that:
   - Loads config from `~/.config/hyprbar/config.json`
   - Initializes Renderer and WidgetManager
   - Renders one frame with all widgets
   - Saves Cairo surface directly to PNG
3. Compiles and runs the program
4. Outputs PNG at specified path (default: `docs/screenshot.png`)

**Use cases:**
- Documenting new features
- Testing visual changes
- Creating examples for README
- CI/CD visual regression tests

**Example:**
```bash
# Generate screenshot with current config
./scripts/screenshot.sh docs/new-feature.png

# Test different config
cp examples/config-dracula.json ~/.config/hyprbar/config.json
./scripts/screenshot.sh docs/dracula-theme.png
```

**Note:** Screenshot shows actual rendering but doesn't include compositor effects (shadows, transparency) or live updates. For full testing, use actual Hyprland/Sway.
