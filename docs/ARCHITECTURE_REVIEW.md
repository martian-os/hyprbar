# Hyprbar Architectural Review - Remaining Issues
**Date:** 2026-03-03 (Updated)  
**Reviewer:** Martian  
**Status:** Phases 1 & 2 Complete ✅

## Executive Summary

**Critical Issues Fixed:** ✅
- Struct/class mismatch
- Constructor initialization order
- Unused variables
- Script execution security (SecurityValidator)
- Path traversal protection
- D-Bus null checks

**Remaining Work:**
- 4 quality issues (use-after-free, RAII, raw pointers, exception safety)
- 8 architectural improvements (low priority, future refactoring)

---

## 1. Remaining Code Quality Issues

### 🔴 Critical (Must Fix Soon)

#### 1.1 Use-After-Free in Event Loop
**File:** `src/core/event_loop.cpp`  
**Issue:** Handler can remove itself during callback, causing use-after-free  
**Code:**
```cpp
void EventLoop::remove_fd(int fd) {
  fd_handlers_.erase(fd);  // ← If called from within a handler callback
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
}

// During event processing:
for (auto& [fd, handler] : fd_handlers_) {
  handler(fd);  // ← Handler calls remove_fd(fd) → iterator invalidated!
}
```

**Impact:** Undefined behavior, potential crash  
**Fix:**
```cpp
class EventLoop {
  std::vector<int> pending_removals_;
  bool in_event_loop_{false};
  
  void remove_fd(int fd) {
    if (in_event_loop_) {
      pending_removals_.push_back(fd);  // Defer removal
    } else {
      fd_handlers_.erase(fd);
      epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    }
  }
  
  void run() {
    in_event_loop_ = true;
    // ... process events ...
    in_event_loop_ = false;
    
    // Process deferred removals
    for (int fd : pending_removals_) {
      fd_handlers_.erase(fd);
      epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    }
    pending_removals_.clear();
  }
};
```

### 🟡 Medium Priority (Should Fix)

#### 1.2 RAII Violations
**File:** `src/widgets/tray_widget.cpp`  
**Issue:** C API resources (GError, GdkPixbuf) not wrapped in RAII  
**Code:**
```cpp
GError* error = nullptr;
GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_size(..., &error);
if (pixbuf) {
  // ... use pixbuf
  g_object_unref(pixbuf);
  return surface;  // ✅ Cleaned up
}
if (error) {
  g_error_free(error);
}
// What if new code path returns early? → Leak!
```

**Impact:** Memory leaks if error handling changes  
**Fix:**
```cpp
// Create RAII wrappers
struct GErrorDeleter { 
  void operator()(GError* e) { 
    if (e) g_error_free(e); 
  } 
};
using GErrorPtr = std::unique_ptr<GError, GErrorDeleter>;

struct GObjectUnref { 
  void operator()(gpointer p) { 
    if (p) g_object_unref(p); 
  } 
};
template<typename T>
using GObjectPtr = std::unique_ptr<T, GObjectUnref>;

// Usage
GErrorPtr error;
GError* raw_error = nullptr;
GObjectPtr<GdkPixbuf> pixbuf(
  gdk_pixbuf_new_from_file_at_size(..., &raw_error)
);
error.reset(raw_error);

if (pixbuf) {
  // Use pixbuf.get()
  return surface;  // ✅ Automatic cleanup
}
// ✅ error automatically freed
```

#### 1.3 Raw Pointers in Public API
**File:** `src/widgets/widget_manager.cpp`  
**Issue:** Returning raw pointer from unique_ptr  
**Code:**
```cpp
Widget* WidgetManager::get_widget(const std::string& id) {
  return widgets_[id].get();  // Raw pointer from unique_ptr
}
```

**Risk:** 
- Caller might delete the widget (double-free)
- Caller might store pointer (dangling after widget removal)

**Fix Option 1 - Return reference:**
```cpp
Widget& WidgetManager::get_widget(const std::string& id) {
  if (!widgets_.count(id)) {
    throw std::runtime_error("Widget not found: " + id);
  }
  return *widgets_[id];  // Reference clearly non-owning
}
```

**Fix Option 2 - Return observer_ptr (C++26) or comment ownership:**
```cpp
// Returns non-owning pointer. WidgetManager retains ownership.
// Pointer valid until widget is removed or WidgetManager destroyed.
Widget* WidgetManager::get_widget(const std::string& id) {
  return widgets_[id].get();
}
```

#### 1.4 Exception Safety
**Files:** Various  
**Issue:** No exception specifications, unclear exception guarantees  

**Missing noexcept:**
```cpp
// Should be noexcept (doesn't throw)
int Widget::get_desired_width() const { return width_; }
int Widget::get_desired_height() const { return height_; }
bool Widget::update() { return false; }
```

**Unclear exception behavior:**
```cpp
// Can this throw? What guarantee?
bool ConfigManager::load(const std::string& path);
```

**Fix:**
```cpp
// Mark non-throwing functions
int get_desired_width() const noexcept { return width_; }

// Document exception behavior
/// Loads configuration from file
/// @throws std::runtime_error if file not found or parse error
/// @throws std::bad_alloc if out of memory
/// @exception-guarantee Basic (object remains valid)
bool load(const std::string& path);
```

---

## 2. Architectural Improvements (Low Priority, Future)

### 2.1 Global State
**Current:** `static AppState app` in main.cpp  
**Better:** Application class with proper encapsulation  
**Benefit:** Testable, multiple instances possible

### 2.2 Error Handling Standardization
**Current:** Mix of bool, exceptions, optional  
**Better:** std::expected<T, Error> or consistent approach  
**Benefit:** Uniform error handling across codebase

### 2.3 Logger Design
**Current:** Singleton Logger::instance()  
**Better:** Dependency injection  
**Benefit:** Testable with mock loggers

### 2.4 ConfigValue Variant
**Current:** Manual discriminated union  
**Better:** std::variant  
**Benefit:** Type-safe, less boilerplate

### 2.5 Widget Registration
**Current:** Hardcoded in create_widget()  
**Better:** Registry pattern with factories  
**Benefit:** Plugin architecture ready

### 2.6 Path Resolution
**Current:** Mixed in ConfigManager  
**Better:** Separate PathResolver class  
**Benefit:** Single Responsibility Principle

### 2.7 Renderer Text API
**Current:** Pango directly in Renderer  
**Better:** Abstract TextEngine interface  
**Benefit:** Swappable text engines

### 2.8 Magic Numbers
**Current:** Hardcoded spacing, margin values  
**Better:** Named constants  
**Benefit:** Self-documenting, easy to configure

---

## 3. Priority Recommendations

### 🔴 Fix Now (This Week)
1. **Use-after-free in event loop** - Critical bug, undefined behavior
2. **RAII wrappers** - Prevent memory leaks
3. **Raw pointer API** - Document ownership or use references

### 🟡 Fix Soon (Next Week)
4. **Exception safety** - Add noexcept, document guarantees

### 🟢 Future (When Time Allows)
5. Application class refactoring
6. Error handling standardization
7. Logger dependency injection
8. Plugin architecture
9. Other architectural improvements

---

## 4. Summary

**✅ Completed:**
- All critical compiler warnings fixed
- Security vulnerabilities addressed (script injection, path traversal)
- D-Bus null checks added
- Static analysis infrastructure in place
- Test infrastructure with mocks

**🔴 Critical Remaining:**
- Use-after-free bug in event loop

**🟡 Important Remaining:**
- RAII violations (potential leaks)
- Raw pointer API (ownership unclear)
- Exception safety (missing specifications)

**🟢 Nice-to-Have:**
- 8 architectural improvements (future refactoring)

**Overall:** The codebase is in good shape. With the critical use-after-free fixed and RAII wrappers added, it will be production-ready. The architectural improvements are optional refinements for long-term maintainability.
