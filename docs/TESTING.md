# Testing Guide

## Overview

Hyprbar has two test suites:
1. **Unit tests** - Test individual components in isolation
2. **Integration tests** - Test full binary execution with all configs

## Running Tests

```bash
# Unit tests (68 tests)
make test

# Integration tests (6 configs)
make integration

# Run both
make test && make integration
```

## Unit Tests

Located in `tests/`:
- `basic_test.cpp` - Core functionality
- `config_test.cpp` - Config parsing and validation
- `renderer_test.cpp` - Rendering primitives
- `widget_test.cpp` - Widget management
- `test_bar_config.cpp` - Bar configuration and inheritance

Run individually:
```bash
./bin/test_hyprbar
```

## Integration Tests

Located in `tests/integration_test.sh`

Tests all example configs in screenshot mode:
- `config-default.json`
- `config-full.json`
- `config.json`
- `config-minimal.json`
- `config-transparent.json`
- `config-with-scripts.json`

Each test:
1. Runs hyprbar with the config
2. Generates a PNG screenshot
3. Validates PNG format
4. Checks for crashes/timeouts

## Screenshot vs Live Mode

### Screenshot Mode
- Renders one frame to PNG
- No Wayland connection
- Fixed dimensions (1920x{height})
- Immediate exit after render
- Used for: testing, previews, CI

### Live Mode (Wayland)
- Connects to Wayland compositor
- Dynamic dimensions from compositor
- Continuous rendering loop
- Event handling (mouse, keyboard)
- Used for: actual usage

### Shared Code
Both modes use the same:
- Configuration loading
- Widget initialization
- Widget rendering
- Text rendering (Pango)
- Layout calculations

### Key Differences

| Aspect | Screenshot | Live |
|--------|-----------|------|
| Initialization | Direct renderer init | Wayland roundtrip for size |
| Dimensions | Hardcoded 1920 | From compositor |
| Render loop | Single frame | Event-driven loop |
| Buffer | Cairo surface | Wayland shm buffer |
| Exit | Immediate | On signal/close |

## Adding New Tests

### Unit Test
```cpp
// tests/test_new_feature.cpp
#include "test_utils.h"
#include "hyprbar/your_feature.h"

void test_your_feature() {
  test::assert(condition, "description");
}

void run_your_feature_tests() {
  test_your_feature();
}
```

Add to `test_main.cpp`:
```cpp
void run_your_feature_tests();  // Forward declaration

int main() {
  // ...
  run_your_feature_tests();
  // ...
}
```

### Integration Test
Add config to `examples/`:
```json
{
  "bar": { ... },
  "widgets": [ ... ]
}
```

Integration test automatically picks it up!

## Debugging Tests

### Unit Test Failures
```bash
# Run with debug symbols
make debug
./bin/test_hyprbar

# GDB
gdb ./bin/test_hyprbar
(gdb) run
```

### Integration Test Failures
```bash
# Run manually with logs
./bin/hyprbar --config examples/config-full.json --screenshot /tmp/test.png

# Check what's wrong
file /tmp/test.png
identify /tmp/test.png
```

### Common Issues

**Timeout (exit code 124):**
- Widget script hanging (check curl timeouts)
- Infinite loop in widget
- Deadlock in threading

**Segfault:**
- Null pointer dereference
- Use-after-free
- Thread race condition

**No output PNG:**
- Cairo error (check stderr)
- Permission issue (/tmp not writable)
- Renderer initialization failed

## CI Integration

Tests run automatically on:
- Every push to main
- Every pull request
- Pre-commit hook (unit tests only)

GitHub Actions workflow:
```yaml
- name: Run unit tests
  run: make test

- name: Run integration tests
  run: make integration
```

## Coverage

Current coverage:
- Unit tests: 68 tests covering config, rendering, widgets
- Integration: 6 configs covering all example use cases

TODO:
- Add performance benchmarks
- Add thread safety stress tests
- Add memory leak detection (valgrind)
- Add fuzzing for config parser
