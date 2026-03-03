# Hyprbar Architectural Review
**Date:** 2026-03-02  
**Reviewer:** Martian  
**Total Lines of Code:** ~3200 lines (src + headers)

## Executive Summary

**Overall Architecture: SOLID ✅**
- Clean separation of concerns (core, rendering, wayland, widgets)
- Well-defined interfaces and abstractions
- Testable components with 68 passing tests
- Good performance (non-blocking, threaded widgets)

**Critical Issues: 10 warnings to address**
**Recommendations: 8 architectural improvements**

---

## 1. Code Quality Issues

### 🔴 Critical Warnings (Must Fix)

#### 1.1 Struct/Class Mismatch
**File:** `include/hyprbar/widgets/widget.h:11`  
**Issue:** Forward declares `ConfigValue` as `class`, but it's defined as `struct`  
**Impact:** Potential linker errors on MSVC  
**Fix:**
```cpp
// widget.h
struct ConfigValue;  // Change from class to struct
```

#### 1.2 Constructor Initialization Order
**Files:** `include/hyprbar/core/config_manager.h`  
**Issue:** Member initialization doesn't match declaration order  
**Impact:** Undefined behavior, values initialized in wrong order  
**Declaration order:**
1. `type`
2. `string_value`
3. `int_value`
4. `double_value`
5. `bool_value`
6. `object_value`
7. `array_value`

**Current init order:** Initializes complex types (object/array) before primitives  
**Fix:** Reorder initializer lists to match declaration order

#### 1.3 Unused Variable
**File:** `src/widgets/widget_manager.cpp:129`  
**Issue:** `left_total` calculated but never used  
**Fix:** Remove if truly unused, or add comment explaining why it's calculated

### 🟡 Minor Warnings (Should Fix)

#### 1.4 Unused Parameter
**File:** `src/wayland/wayland_manager.cpp:288`  
**Issue:** `version` parameter not used  
**Fix:** Add `[[maybe_unused]]` or comment `/*version*/`

#### 1.5 Missing Field Initializer
**File:** `src/wayland/wayland_manager.cpp:33`  
**Issue:** `axis_value120` field not initialized in listener struct  
**Fix:** Add explicit initialization

#### 1.6 Constructor Reordering
**File:** `src/widgets/script_widget.cpp:12`  
**Issue:** `running_` initialized before `output_changed_`  
**Fix:** Reorder initializer list

---

## 2. Architectural Review

### ✅ Strengths

#### 2.1 Clean Layered Architecture
```
┌─────────────────┐
│   Application   │ (main.cpp)
├─────────────────┤
│   Widgets       │ (widget_manager, script_widget)
├─────────────────┤
│   Rendering     │ (renderer, surfaces)
├─────────────────┤
│   Core          │ (config, event_loop, logger)
├─────────────────┤
│   Wayland       │ (wayland_manager, protocols)
└─────────────────┘
```

**Good:**
- Clear separation of concerns
- Each layer depends only on layers below
- No circular dependencies

#### 2.2 Interface Abstraction
```cpp
// Widget interface
class Widget {
  virtual bool initialize(const ConfigValue& config) = 0;
  virtual void render(...) = 0;
  virtual bool update() = 0;
};

// Surface interface
class Surface {
  virtual bool initialize(...) = 0;
  virtual uint8_t* get_buffer() = 0;
};
```

**Good:**
- Polymorphic designs allow extensions
- Easy to add new widget types
- Testable with mocks

#### 2.3 Thread Safety
- Background threads for script widgets
- Mutex protection for shared state
- Atomic flags for change detection

**Good:**
- Non-blocking initialization
- No widget blocks another
- Clean shutdown with thread joins

#### 2.4 Configuration System
- JSON-based, human-readable
- Inheritance model (bar → widgets)
- Path resolution (relative, absolute, tilde)
- Type-safe ConfigValue variant

**Good:**
- Flexible and extensible
- Clear defaults and overrides
- Proper error handling

### ⚠️ Weaknesses & Recommendations

#### 2.5 Global State (Minor Issue)
**File:** `src/main.cpp`  
**Issue:** `static AppState app` is global mutable state  
**Impact:** 
- Harder to test
- Potential for race conditions
- Single instance limitation

**Recommendation:**
```cpp
// Wrap in Application class
class Application {
private:
  std::unique_ptr<EventLoop> event_loop_;
  std::unique_ptr<WaylandManager> wayland_;
  // ...
public:
  int run_wayland_mode();
  int run_screenshot_mode();
};

int main() {
  Application app;
  return app.run();
}
```

**Benefits:**
- Testable (can instantiate multiple)
- Clear ownership
- RAII cleanup

#### 2.6 Error Handling Inconsistency
**Current mix:**
- Some functions return `bool` (success/fail)
- Some use exceptions (JSON parsing)
- Some use `std::optional`

**Recommendation:** Standardize on one approach:
```cpp
// Option 1: std::expected (C++23) or custom Result<T, Error>
Result<Config, std::string> load_config(const std::string& path);

// Option 2: Exceptions for all errors
// throw ConfigError("Failed to parse...");

// Option 3: Error codes + optional
enum class Error { FileNotFound, ParseError, ... };
std::optional<Config> load_config(const std::string& path, Error& out_error);
```

#### 2.7 Logger Design
**Current:** Singleton pattern (`Logger::instance()`)  
**Issues:**
- Global state
- Hard to test with different loggers
- Can't redirect logs per-component

**Recommendation:**
```cpp
// Dependency injection
class WidgetManager {
  Logger& logger_;  // Injected, not global
public:
  WidgetManager(Logger& logger) : logger_(logger) {}
};
```

**Benefits:**
- Testable (inject mock logger)
- Flexible (different loggers for different components)
- No hidden globals

#### 2.8 ConfigValue Variant
**Current:** Manual discriminated union  
**C++17 Alternative:** `std::variant`

```cpp
using ConfigValue = std::variant<
  std::string,
  int64_t,
  double,
  bool,
  std::map<std::string, ConfigValue>,
  std::vector<ConfigValue>
>;

// Usage
if (auto* str = std::get_if<std::string>(&value)) {
  // use *str
}
```

**Benefits:**
- Type-safe (compiler-checked)
- Less boilerplate
- Standard library support

**Trade-off:** Current approach is fine for simplicity, but variant scales better

#### 2.9 Widget Registration
**Current:** Hardcoded in `create_widget()`  
**Future-proof:** Registry pattern

```cpp
class WidgetRegistry {
  using Factory = std::function<std::unique_ptr<Widget>()>;
  std::map<std::string, Factory> factories_;
  
public:
  void register_widget(const std::string& type, Factory factory) {
    factories_[type] = factory;
  }
  
  std::unique_ptr<Widget> create(const std::string& type) {
    auto it = factories_.find(type);
    return it != factories_.end() ? it->second() : nullptr;
  }
};

// Usage (plugins can register)
registry.register_widget("script", []() { 
  return std::make_unique<ScriptWidget>(); 
});
```

**Benefits:**
- Plugin architecture ready
- No recompilation to add widgets
- Discoverable widget types

#### 2.10 Path Resolution
**Current:** ConfigManager has `resolve_path()`  
**Concern:** Mixed responsibility (config parsing + filesystem)

**Recommendation:**
```cpp
class PathResolver {
public:
  std::string resolve(const std::string& path, 
                      const std::string& base_dir);
  std::string expand_tilde(const std::string& path);
  bool is_absolute(const std::string& path);
};

// ConfigManager uses PathResolver
class ConfigManager {
  PathResolver resolver_;
};
```

**Benefits:**
- Single Responsibility Principle
- Reusable path logic
- Easier to test in isolation

#### 2.11 Renderer Text API
**Current:** Pango integrated directly in Renderer  
**Concern:** Renderer knows about Pango specifics

**Better:**
```cpp
// Abstract text engine interface
class TextEngine {
public:
  virtual void render_text(cairo_t* cr, ...) = 0;
  virtual Size measure_text(...) = 0;
};

class PangoTextEngine : public TextEngine { ... };
class CairoTextEngine : public TextEngine { ... };

class Renderer {
  std::unique_ptr<TextEngine> text_engine_;
};
```

**Benefits:**
- Swappable text engines
- Can test with mock text engine
- Easier to add fallback engines

#### 2.12 Magic Numbers
**Examples:**
```cpp
int spacing = 10;  // widget_manager.cpp:120
int margin = 10;   // widget_manager.cpp:121
```

**Recommendation:**
```cpp
namespace defaults {
  constexpr int widget_spacing = 10;
  constexpr int bar_margin = 10;
}
```

**Benefits:**
- Named constants (self-documenting)
- Easy to make configurable later
- DRY principle

---

## 3. Testing Assessment

### ✅ Current Coverage
- **68 tests passing**
- Config parsing: ✅
- Widget inheritance: ✅
- Renderer basics: ✅
- Widget management: ✅

### 🟡 Missing Test Coverage
1. **Integration tests**
   - End-to-end bar rendering
   - Widget interaction with Wayland
   - Error recovery flows

2. **Performance tests**
   - Widget update frequency
   - Memory usage over time
   - Thread contention

3. **Edge cases**
   - Malformed configs
   - Missing dependencies (fonts, scripts)
   - Race conditions in threading

**Recommendation:** Add integration test suite in `tests/integration/`

---

## 4. Performance Considerations

### ✅ Good Practices
- Non-blocking widget initialization
- Background threads for script execution
- Efficient layout caching

### 🟡 Potential Improvements

#### 4.1 Widget Update Batching
**Current:** Each widget updates independently  
**Opportunity:** Batch updates to reduce redraws

```cpp
// Collect all changes, render once
bool any_changed = false;
for (auto& widget : widgets_) {
  any_changed |= widget->update();
}
if (any_changed) {
  render_all();
}
```

#### 4.2 String Allocations
**Hot path:** `get_desired_width()` called every layout  
**Optimization:** Cache text metrics

```cpp
class ScriptWidget {
  mutable std::optional<int> cached_width_;
  std::string last_measured_text_;
  
  int get_desired_width() const override {
    if (last_output_ != last_measured_text_) {
      cached_width_ = measure_text(last_output_);
      last_measured_text_ = last_output_;
    }
    return *cached_width_;
  }
};
```

---

## 5. Security Review

### ✅ Good Practices
- No raw pointers (use smart pointers)
- RAII for resource management
- Bounds checking in config parsing

### 🔴 Potential Issues

#### 5.1 Script Execution
**File:** `src/widgets/script_widget.cpp`  
**Issue:** Executes arbitrary shell commands via `popen()`  
**Risk:** Command injection if user provides malicious config

**Current mitigation:** None  
**Recommendation:**
```cpp
// Validate command path
bool is_safe_command(const std::string& cmd) {
  // Must be absolute path or in trusted dir
  // Must exist and be executable
  // No shell metacharacters (unless intentional)
}

// Use execvp instead of popen (no shell)
```

#### 5.2 File Path Traversal
**File:** `src/core/config_manager.cpp`  
**Issue:** `resolve_path()` accepts user input  
**Risk:** Path traversal (`../../etc/passwd`)

**Recommendation:**
```cpp
std::string resolve_path(const std::string& path) {
  std::filesystem::path resolved = ...;
  resolved = std::filesystem::canonical(resolved);  // Resolve symlinks
  
  // Ensure result is within allowed directories
  if (!is_within_allowed_dirs(resolved)) {
    throw SecurityError("Path outside allowed directories");
  }
  
  return resolved.string();
}
```

---

## 6. Documentation

### ✅ Good
- README with examples
- Widget documentation
- Inline comments for complex logic

### 🟡 Missing
- Architecture diagram (layers, data flow)
- API documentation (Doxygen comments incomplete)
- Contributor guide (coding standards, PR process)
- Troubleshooting guide

**Recommendation:** Add `docs/` directory:
```
docs/
├── architecture.md    # System design, diagrams
├── api/              # Generated from Doxygen
├── development.md    # Build, test, contribute
└── troubleshooting.md
```

---

## 7. Priority Recommendations

### 🔴 High Priority (Fix Now)
1. Fix struct/class mismatch (linker error risk)
2. Fix constructor initialization order (undefined behavior)
3. Add script command validation (security)
4. Add path traversal protection (security)

### 🟡 Medium Priority (Next Release)
5. Remove unused `left_total` variable
6. Standardize error handling
7. Add integration tests
8. Cache text metrics (performance)

### 🟢 Low Priority (Future)
9. Refactor to Application class (testability)
10. Consider std::variant for ConfigValue
11. Add plugin architecture for widgets
12. Abstract text engine interface

---

## 8. Conclusion

**Overall Grade: B+ (Very Good)**

Hyprbar demonstrates solid architectural principles with clean separation of concerns, good abstractions, and comprehensive testing. The codebase is maintainable and extensible.

**Key Strengths:**
- Well-structured layers
- Thread-safe widget system
- Flexible configuration
- Good test coverage

**Critical Improvements Needed:**
- Fix compiler warnings (especially UB risks)
- Add security validation for script execution
- Standardize error handling

**Long-term Vision:**
- Plugin architecture for extensibility
- Richer widget ecosystem
- Performance optimizations

The foundation is excellent. With the critical fixes applied, this codebase is production-ready.

---

**Next Steps:**
1. Apply critical fixes (warnings, security)
2. Run static analysis (clang-tidy, cppcheck)
3. Add CI checks for warnings-as-errors
4. Document architecture
