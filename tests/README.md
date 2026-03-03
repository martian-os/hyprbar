# Testing Strategy

## Current State

**Total Tests:** 108 (all passing)
**Coverage:** ~63% overall

### Widget Test Coverage

#### Script Widget ✅
- Initialization, configuration, updates
- Render timing and dimensions
- Font size affects width
- **Coverage:** 78.9%

#### Tray Widget ✅
- Initialization, configuration
- Dimensions, rendering (empty tray)
- **Coverage:** 84.7%
- **Note:** D-Bus integration not yet implemented

#### Hyprland Widget ⚠️
- Config parsing (colors, max_workspaces)
- Fallback without env var (zero width)
- Default value handling
- **Coverage:** ~30-35% (estimated)
- **Limitation:** Socket communication not tested

### What's Not Tested

1. **Widget Manager** (0% coverage)
   - Layout logic (left/center/right positioning)
   - Widget spacing calculations
   - Runs in main loop, needs refactoring

2. **Wayland Integration** (0% coverage)
   - Surface creation/destruction
   - Event handling
   - Protocol handshakes
   - Requires compositor mock

3. **Hyprland Widget IPC** (not tested)
   - Socket connection logic
   - JSON parsing of workspace data
   - Event stream handling
   - Requires mock server (see mock_hyprland_server.py)

## Mock Infrastructure

### Hyprland Mock Server

`tests/mock_hyprland_server.py` - Simulates Hyprland IPC sockets

**Features:**
- Creates `.socket.sock` (command responses)
- Creates `.socket2.sock` (event stream)
- Responds to `j/workspaces` and `j/activeworkspace`

**Usage:**
```bash
# Start mock server
python3 tests/mock_hyprland_server.py /tmp/mock_hypr/test_instance &

# Set environment
export HYPRLAND_INSTANCE_SIGNATURE=test_instance

# Run tests
./bin/test_hyprbar
```

**Status:** Created but not integrated into test suite (tests hang waiting for socket)

### Future Mocks Needed

1. **D-Bus Mock** - For tray widget StatusNotifierItem protocol
2. **Wayland Compositor Mock** - For surface and protocol testing
3. **Widget Manager Test Harness** - Extract layout logic for unit testing

## Improving Coverage

### Priority 1: Widget Manager (Quick Win)
**Current:** 0% (174 lines)
**Target:** >70%

**Approach:**
- Extract layout calculation into testable functions
- Create `WidgetLayout` struct with position calculations
- Test left/center/right positioning logic
- Test spacing and widget arrangement

**Example:**
```cpp
struct WidgetLayout {
  int x, y, width;
};

std::vector<WidgetLayout> calculate_layout(
  const std::vector<Widget*>& widgets,
  int bar_width
);
```

### Priority 2: Hyprland Widget Socket Logic
**Current:** ~30%
**Target:** >70%

**Approach:**
- Fix mock server hanging issue
- Add proper socket timeout handling
- Test JSON parsing separately from socket I/O
- Mock socket responses in tests

### Priority 3: Wayland Integration
**Current:** 0% (358 lines)
**Target:** >50%

**Approach:**
- Use `wl_display_connect(NULL)` mock
- Test protocol object creation
- Test surface configuration
- Separate protocol logic from rendering

## Running Coverage

```bash
# Generate coverage report
make coverage

# View detailed HTML report
make coverage-report
open coverage/html/index.html

# Clean coverage data
make coverage-clean
```

## Test File Organization

- `basic_test.cpp` - Basic infrastructure tests
- `config_test.cpp` - Configuration parsing
- `renderer_test.cpp` - Cairo rendering primitives
- `style_test.cpp` - CSS-like style inheritance
- `test_bar_config.cpp` - Bar configuration
- `widget_test.cpp` - Script/Tray widget tests
- `hyprland_widget_test.cpp` - Hyprland widget config tests
- `test_main.cpp` - Test runner
- `test_utils.h` - Test assertions and utilities

## Known Issues

1. **gcov version mismatch** - Clang 15 vs gcov 15 format incompatibility
2. **Mock server hangs** - Socket tests need timeout handling
3. **Wayland mock complexity** - Requires significant infrastructure

## Recommendations

1. Extract testable logic from main loop (Widget Manager)
2. Add JSON parsing unit tests (separate from I/O)
3. Create lightweight mocks for external dependencies
4. Add integration tests using screenshot comparison
