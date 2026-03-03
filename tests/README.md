# Test Infrastructure with Mock Services

## Overview

This directory contains test infrastructure for hyprbar, including mock services for external dependencies (D-Bus, Hyprland).

## Test Modes

### Fast Mode (Default for Pre-commit)
```bash
make test-fast
```
- Runs unit tests only
- No external services required
- Fast execution (~5-10 seconds for 118 tests)
- Safe for pre-commit hooks

### Mock Mode (Integration Tests)
```bash
make test-mocks
```
- Runs tests with mock D-Bus and Hyprland services
- Slower but tests real integration
- Isolated from system services
- Use for comprehensive testing

## Mock Services

### MockHyprland
Simulates Hyprland compositor socket interface:
- Command socket (`.socket.sock`)
- Event socket (`.socket2.sock`)
- Returns workspace JSON responses
- Triggers workspace change events

**Usage:**
```cpp
#include "test_mocks.h"

// Create mock with 5 workspaces
auto mock = test::mocks::create_hyprland_with_workspaces(5);

// RAII guard handles start/stop
test::mocks::MockGuard guard(mock);

// Test your widget
HyprlandWidget widget;
widget.initialize(config);
```

### MockDBus
Simulates D-Bus StatusNotifierWatcher:
- Registers tray items
- Provides IconName and IconPixmap properties
- Configurable via JSON

**Usage:**
```cpp
// Create mock with 2 tray items
auto mock = test::mocks::create_dbus_with_tray_items(2);

// Auto-cleanup with guard
test::mocks::MockGuard guard(mock);

// Test tray widget
TrayWidget widget;
widget.initialize(config);
```

## Files

- `test_mocks.h` - Mock service interface definitions
- `test_mocks.cpp` - Mock service implementations
- `mock_hyprland_server.py` - Python mock for Hyprland sockets
- `mock_dbus_server.py` - Python mock for D-Bus services
- `example_mock_test.cpp` - Example usage (standalone)

## Adding New Tests

### Unit Test (Fast)
```cpp
void test_my_feature() {
  // No external services
  MyWidget widget;
  widget.initialize(config);
  
  test::assert(widget.works(), "Feature works");
}
```

### Integration Test (Mocks)
```cpp
void test_with_dbus() {
  auto mock = test::mocks::create_dbus_with_tray_items(1);
  test::mocks::MockGuard guard(mock);
  
  TrayWidget widget;
  widget.initialize(config);
  
  test::assert(widget.get_desired_width() > 0, "Has width");
}
```

## Best Practices

1. **Use fast tests by default** - Pre-commit hooks run fast tests only
2. **Mock external services** - Don't rely on system D-Bus or Hyprland
3. **Use RAII guards** - `MockGuard` ensures cleanup even on exceptions
4. **Isolated tests** - Each test starts fresh mocks
5. **Check test mode** - Use `HYPRBAR_TEST_MODE` env var to control behavior

## Pre-commit Hook

The pre-commit hook runs `make test-fast` which:
- Skips D-Bus/Hyprland integration tests
- Completes in ~10 seconds
- Only runs pure unit tests
- No external service dependencies

For full integration testing, run `make test-mocks` manually.
