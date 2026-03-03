# Implementation Checklist - Remaining Issues

## Status Legend
- ✅ Done (committed)
- 🔄 In Progress
- ❌ Not Started

---

## Phase 1 & 2: ✅ COMPLETE

**All critical and security issues resolved:**
- Struct/class mismatch
- Constructor initialization order
- Unused variables
- D-Bus null checks
- Script command validation (SecurityValidator)
- Path traversal protection
- Const correctness verified
- Override keywords verified

---

## Phase 3: Quality Improvements (In Progress)

### 🔴 Critical

- [ ] ❌ **Issue 1: Use-after-free in event loop**
  - **Location:** src/core/event_loop.cpp
  - **Problem:** Handler can call remove_fd() during event processing
  - **Impact:** Undefined behavior, iterator invalidation
  - **Fix:** Defer removals with pending_removals_ vector
  - **Code:**
    ```cpp
    std::vector<int> pending_removals_;
    bool in_event_loop_{false};
    
    void remove_fd(int fd) {
      if (in_event_loop_) {
        pending_removals_.push_back(fd);
      } else {
        // immediate removal
      }
    }
    ```

### 🟡 Medium Priority

- [ ] ❌ **Issue 2: RAII violations**
  - **Location:** src/widgets/tray_widget.cpp
  - **Problem:** GError, GdkPixbuf not wrapped in RAII
  - **Impact:** Potential memory leaks on early returns
  - **Fix:** Create RAII wrappers (GErrorPtr, GObjectPtr)
  - **Code:**
    ```cpp
    struct GErrorDeleter { void operator()(GError* e) { g_error_free(e); } };
    using GErrorPtr = std::unique_ptr<GError, GErrorDeleter>;
    
    struct GObjectUnref { void operator()(gpointer p) { g_object_unref(p); } };
    template<typename T>
    using GObjectPtr = std::unique_ptr<T, GObjectUnref>;
    ```

- [ ] ❌ **Issue 3: Raw pointers in API**
  - **Location:** src/widgets/widget_manager.cpp - get_widget()
  - **Problem:** Returns raw pointer from unique_ptr
  - **Impact:** Unclear ownership, potential dangling pointers
  - **Fix Option 1:** Return reference instead
  - **Fix Option 2:** Document ownership clearly

- [ ] ❌ **Issue 4: Exception safety**
  - **Location:** Various files
  - **Problem:** Missing noexcept, unclear exception guarantees
  - **Impact:** Unclear error handling contract
  - **Fix:** Add noexcept to non-throwing functions, document exceptions

---

## Phase 4: Architectural Improvements (Low Priority, Future)

- [ ] ❌ **2.1 Global State** - Wrap in Application class
- [ ] ❌ **2.2 Error Handling** - Standardize approach (std::expected?)
- [ ] ❌ **2.3 Logger Design** - Dependency injection instead of singleton
- [ ] ❌ **2.4 ConfigValue Variant** - Use std::variant instead of manual union
- [ ] ❌ **2.5 Widget Registration** - Plugin architecture with registry
- [ ] ❌ **2.6 Path Resolution** - Extract PathResolver class
- [ ] ❌ **2.7 Renderer Text API** - Abstract text engine interface
- [ ] ❌ **2.8 Magic Numbers** - Named constants

---

## Implementation Priority

### This Week (Critical)
1. ❌ Fix use-after-free in event loop
2. ❌ Add RAII wrappers for GError/GdkPixbuf
3. ❌ Document or fix raw pointer API

### Next Week (Important)
4. ❌ Add exception safety (noexcept, documentation)

### Future (When Time Allows)
5. ❌ Architectural refactoring (Phase 4 items)

---

## Notes

- **Phases 1 & 2 complete** - All critical compiler warnings and security issues fixed
- **4 quality issues remaining** - Use-after-free is the most critical
- **8 architectural improvements** - Low priority, future refinements
- **Static analysis infrastructure in place** - clang-tidy, cppcheck, sanitizers
- **Test infrastructure ready** - Fast tests, mock services, RAII guards

**Next action:** Fix use-after-free bug in event loop (highest priority).
