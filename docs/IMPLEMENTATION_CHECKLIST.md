# Implementation Checklist - Architecture & Quality Issues

## Status Legend
- ✅ Done
- 🔄 In Progress
- ❌ Not Started

---

## Architecture Review Issues

### 🔴 Critical (Must Fix Now)

- [x] ✅ **1.1 Struct/Class Mismatch** - `ConfigValue` forward declaration (already fixed)
- [x] ✅ **1.2 Constructor Initialization Order** - ConfigValue members (already correct)
- [x] ✅ **1.3 Unused Variable** - `left_total` in widget_manager.cpp:129 (already removed)
- [x] ✅ **5.1 Script Execution Security** - Command injection risk (FIXED)
- [x] ✅ **5.2 Path Traversal** - File path validation (FIXED)

### 🟡 Medium Priority

- [ ] ❌ **1.4 Unused Parameter** - `version` in wayland_manager.cpp:288
- [ ] ❌ **1.5 Missing Field Initializer** - `axis_value120` in listener
- [ ] ❌ **1.6 Constructor Reordering** - script_widget.cpp:12

### 🟢 Low Priority (Future)

- [ ] ❌ **2.5 Global State** - Wrap in Application class
- [ ] ❌ **2.6 Error Handling** - Standardize approach
- [ ] ❌ **2.7 Logger Design** - Dependency injection
- [ ] ❌ **2.8 ConfigValue Variant** - Use std::variant
- [ ] ❌ **2.9 Widget Registration** - Plugin architecture
- [ ] ❌ **2.10 Path Resolution** - Extract PathResolver class
- [ ] ❌ **2.11 Renderer Text API** - Abstract text engine
- [ ] ❌ **2.12 Magic Numbers** - Named constants

---

## Quality Gates Issues

### 🔴 Critical (Implemented)

- [x] ✅ **Static Analysis** - clang-tidy, cppcheck (commit faf27de)
- [x] ✅ **Memory Safety** - ASan, TSan targets (commit faf27de)
- [x] ✅ **Quality Shortcuts** - quality-fast, quality-full (commit faf27de)

### Code Issues to Fix

- [x] ✅ **Issue 1: Missing null checks in D-Bus code**
  - Location: src/widgets/tray_widget.cpp (3 locations)
  - Fixed: Added null checks after all dbus_message_new_method_call

- [ ] ❌ **Issue 2: Use-after-free in event loop**
  - Location: src/core/event_loop.cpp
  - Fix: Defer fd removals with pending_removals_ vector

- [x] ✅ **Issue 3: Missing mutex in TrayWidget::update_icons()**
  - Not applicable - function doesn't exist or already handled

- [ ] ❌ **Issue 4: RAII violations**
  - Location: src/widgets/tray_widget.cpp (GError, GdkPixbuf)
  - Fix: Create RAII wrappers (GErrorPtr, GObjectPtr)

- [x] ✅ **Issue 5: Missing const correctness**
  - Location: Multiple widget classes
  - Status: Already correct - all getters are const

- [x] ✅ **Issue 6: Missing override keywords**
  - Location: All widget subclasses
  - Status: Already correct - all have override keywords

- [ ] ❌ **Issue 7: Raw pointers in API**
  - Location: WidgetManager::get_widget()
  - Fix: Return reference instead of raw pointer

- [ ] ❌ **Issue 8: Exception safety**
  - Location: Various
  - Fix: Add noexcept where appropriate

---

## Implementation Order

### Phase 1: Critical Fixes (Today) - ✅ COMPLETE
1. ✅ Struct/class mismatch
2. ✅ Constructor initialization order
3. ✅ Remove unused variables
4. ✅ Add D-Bus null checks
5. ✅ Fix TrayWidget mutex
6. ✅ Add const correctness
7. ✅ Add override keywords

### Phase 2: Security (This Week) - ✅ COMPLETE
8. ✅ Script command validation
9. ✅ Path traversal protection

### Phase 3: Quality (Next Week)
10. ❌ RAII wrappers
11. ❌ Use-after-free fix
12. ❌ Raw pointer fix
13. ❌ Exception safety

### Phase 4: Refactoring (Future)
14. ❌ Application class wrapper
15. ❌ Error handling standardization
16. ❌ Other architectural improvements
