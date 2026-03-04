# System Tray - Known Issues

## Ayatana/Ubuntu Indicator Menus Don't Appear

**Status:** Known Limitation  
**Severity:** Medium  
**Affects:** Ayatana indicators (Ubuntu's system tray implementation)

### Issue

When clicking on tray icons from Ayatana indicators (e.g., Network Manager, System Updates), the menu does not appear even though the click is being sent to the application.

### Root Cause

Ayatana indicators expose their menus via the **DBusMenu protocol** (`com.canonical.dbusmenu`), which requires the status bar application to:
1. Read the menu structure from D-Bus
2. Render the menu using GTK widgets (via `libdbusmenu-gtk3`)
3. Display it as a popup window

Currently, hyprbar:
- ✅ Sends click events to tray icons
- ✅ Attempts multiple activation methods (Activate, ContextMenu, SecondaryActivate)
- ❌ Does not implement DBusMenu rendering

### Workarounds

**For KDE/Qt Applications:**
- Full support - menus work correctly via `ContextMenu` D-Bus method

**For Ayatana Applications:**
- Click events are sent but menus won't display
- Most indicators have alternative UIs (e.g., `nm-connection-editor` for Network Manager)

### Future Implementation

Implementing proper DBusMenu support requires:
1. GTK integration (currently pure Cairo/Wayland)
2. Popup window management for menus
3. GTK event loop integration
4. libdbusmenu-gtk3 dependency

**Estimated effort:** 2-3 days of development

### Affected Applications

- Network Manager (`nm-applet`)
- Update Manager indicators
- Most Ubuntu/Unity indicators
- Other Ayatana-based tray apps

### Alternative Solutions

1. **Use KDE tray apps** (they work correctly)
2. **Launch apps directly** instead of relying on tray menus
3. **Wait for future DBusMenu implementation**

---

**Last Updated:** 2026-03-04  
**Hyprbar Version:** 0.1.0
