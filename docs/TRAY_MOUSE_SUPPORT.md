# Tray Widget Mouse Support Implementation Plan

## StatusNotifierItem Protocol - Mouse Interaction

Based on Waybar's implementation and the org.kde.StatusNotifierItem spec:

### D-Bus Methods (Tray Icon Actions)

```
Interface: org.kde.StatusNotifierItem
```

**Left Click (Primary):**
```cpp
proxy->call("Activate", x, y);
```
- Opens the main application window
- Example: Click NetworkManager → Opens network settings

**Right Click (Context Menu):**
```cpp
proxy->call("ContextMenu", x, y);
```
- Shows context menu
- Menu is DBusMenu protocol (requires separate implementation)
- Alternative: Application handles menu internally

**Middle Click (Secondary):**
```cpp
proxy->call("SecondaryActivate", x, y);
```
- Secondary action (application-defined)
- Example: Pause/play in media player

**Scroll Events:**
```cpp
proxy->call("Scroll", delta, orientation);
```
- orientation: "vertical" or "horizontal"
- delta: scroll amount
- Example: Volume control

### Parameters

All methods receive coordinates:
- `x`: int32 - X position relative to screen (not widget)
- `y`: int32 - Y position relative to screen (not widget)

## Implementation Requirements

### 1. Wayland Input Handling

Currently hyprbar has no input handling. Need to:

**Add input region to layer surface:**
```cpp
// In wayland_manager.cpp
zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface, 1);

// Set input region
struct wl_region* input_region = wl_compositor_create_region(compositor);
wl_region_add(input_region, 0, 0, width, height);
wl_surface_set_input_region(surface, input_region);
wl_region_destroy(input_region);
```

**Add pointer listener:**
```cpp
static const struct wl_pointer_listener pointer_listener = {
  .enter = pointer_enter_handler,
  .leave = pointer_leave_handler,
  .motion = pointer_motion_handler,
  .button = pointer_button_handler,
  .axis = pointer_axis_handler,  // Scroll
};

wl_pointer* pointer = wl_seat_get_pointer(seat);
wl_pointer_add_listener(pointer, &pointer_listener, this);
```

### 2. Hit Testing

Need to map click coordinates to widgets:

```cpp
struct ClickEvent {
  int x;        // Relative to bar
  int y;        // Relative to bar
  uint32_t button;  // 272=left, 273=right, 274=middle
  uint32_t serial;
};

// In WidgetManager
Widget* get_widget_at(int x, int y) {
  for (auto& widget : widgets_) {
    int wx = widget->get_x();
    int wy = widget->get_y();
    int ww = widget->get_width();
    int wh = widget->get_height();
    
    if (x >= wx && x < wx + ww && y >= wy && y < wy + wh) {
      return widget.get();
    }
  }
  return nullptr;
}
```

### 3. Tray Widget Click Handling

```cpp
// In TrayWidget
bool handle_click(int x, int y, uint32_t button) override {
  // Find which icon was clicked
  int current_x = 0;
  for (const auto& icon : icons_) {
    if (x >= current_x && x < current_x + icon_size_) {
      // Icon clicked!
      send_dbus_click(icon, button, x, y);
      return true;
    }
    current_x += icon_size_ + icon_spacing_;
  }
  return false;
}

void send_dbus_click(const TrayIcon& icon, uint32_t button, int x, int y) {
  // Get global coordinates
  int global_x = bar_x + widget_x + x;
  int global_y = bar_y + widget_y + y;
  
  const char* method = nullptr;
  if (button == BTN_LEFT) {
    method = "Activate";
  } else if (button == BTN_RIGHT) {
    method = "ContextMenu";
  } else if (button == BTN_MIDDLE) {
    method = "SecondaryActivate";
  }
  
  if (method) {
    call_dbus_method(icon.service, icon.path, method, global_x, global_y);
  }
}
```

### 4. D-Bus Method Call

```cpp
void TrayWidget::call_dbus_method(const std::string& service,
                                 const std::string& path,
                                 const char* method,
                                 int x, int y) {
  DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, nullptr);
  if (!conn) return;
  
  DBusMessage* msg = dbus_message_new_method_call(
    service.c_str(),
    path.c_str(),
    "org.kde.StatusNotifierItem",
    method
  );
  
  dbus_message_append_args(msg,
    DBUS_TYPE_INT32, &x,
    DBUS_TYPE_INT32, &y,
    DBUS_TYPE_INVALID
  );
  
  // Fire and forget (async)
  dbus_connection_send(conn, msg, nullptr);
  dbus_connection_flush(conn);
  
  dbus_message_unref(msg);
  dbus_connection_unref(conn);
}
```

## Implementation Phases

### Phase 1: Wayland Input Setup (Foundation)
- Add seat listener to wayland_manager
- Add pointer listener
- Parse pointer events (enter, leave, motion, button, axis)
- Store global pointer position
- Forward events to event loop

### Phase 2: Widget Click Interface
- Add virtual `handle_click()` to Widget base class
- Add virtual `handle_scroll()` to Widget base class
- Implement hit testing in WidgetManager
- Dispatch click events to correct widget

### Phase 3: Tray Click Handling
- Implement icon hit testing in TrayWidget
- Add D-Bus method call helper
- Implement Activate (left click)
- Implement ContextMenu (right click)
- Implement SecondaryActivate (middle click)

### Phase 4: Additional Features
- Implement scroll handling
- Add tooltip support (hover detection + D-Bus ToolTip property)
- Add visual feedback (hover state, click animation)

## Mouse Button Codes (Wayland/Linux)

```cpp
#define BTN_LEFT    0x110  // 272
#define BTN_RIGHT   0x111  // 273
#define BTN_MIDDLE  0x112  // 274
```

## Testing

Manual test with NetworkManager:
1. Left click → Should open network settings
2. Right click → Should show context menu
3. Middle click → (depends on app)

## References

- **Waybar implementation:** `/tmp/Waybar/src/modules/sni/item.cpp`
- **StatusNotifierItem spec:** https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/
- **DBusMenu spec:** https://github.com/AyatanaIndicators/libdbusmenu
- **Wayland protocols:** wl_seat, wl_pointer
