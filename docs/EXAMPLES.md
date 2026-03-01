# Hyprbar Example Configuration

This document shows example configurations and expected output.

## Basic Configuration

Minimal config with clock:

```json
{
  "bar": {
    "position": "top",
    "height": 30,
    "background": "#1e1e2e",
    "foreground": "#cdd6f4"
  },
  "widgets": [
    {
      "type": "clock",
      "config": {
        "format": "%H:%M:%S"
      }
    }
  ]
}
```

**Output:** `20:58:42` (updates every second)

## Full System Monitor

Complete configuration with multiple script widgets:

```json
{
  "bar": {
    "position": "top",
    "height": 32,
    "background": "#1e1e2e",
    "foreground": "#cdd6f4"
  },
  "widgets": [
    {
      "type": "script",
      "config": {
        "command": "~/.config/hyprbar/widgets/cpu.sh",
        "interval": 2000,
        "color": "#f38ba8"
      }
    },
    {
      "type": "script",
      "config": {
        "command": "~/.config/hyprbar/widgets/memory.sh",
        "interval": 2000,
        "color": "#a6e3a1"
      }
    },
    {
      "type": "script",
      "config": {
        "command": "~/.config/hyprbar/widgets/uptime.sh",
        "interval": 60000,
        "color": "#89b4fa"
      }
    },
    {
      "type": "clock",
      "config": {
        "format": "%A, %B %d - %H:%M",
        "color": "#cdd6f4"
      }
    },
    {
      "type": "script",
      "config": {
        "command": "~/.config/hyprbar/widgets/battery.sh",
        "interval": 5000,
        "color": "#f9e2af"
      }
    }
  ]
}
```

## Widget Output Examples

### System Widgets
```bash
$ ./cpu.sh
CPU: 12%

$ ./memory.sh
MEM: 3.2G / 15.0G

$ ./uptime.sh
3 days, 23 hours, 2 minutes

$ ./battery.sh
🔋 85%
# or: 🔌 95% (when charging)
# or: No Battery (desktop)
```

### Date/Time Widgets
```bash
$ ./date.sh
Sunday, March 01 - 20:58

$ date "+%H:%M"
20:58

$ date "+%Y-%m-%d %H:%M:%S"
2026-03-01 20:58:42
```

## Custom Widget Examples

### Weather (using wttr.in)
```bash
#!/bin/bash
curl -s "wttr.in/Berlin?format=%t" | tail -1
# Output: +5°C
```

### Network IP
```bash
#!/bin/bash
ip addr show wlan0 | grep 'inet ' | awk '{print $2}' | cut -d/ -f1
# Output: 192.168.1.100
```

### Disk Usage
```bash
#!/bin/bash
df -h / | awk 'NR==2 {print "Disk: " $5}'
# Output: Disk: 45%
```

### Load Average
```bash
#!/bin/bash
awk '{printf "Load: %.2f\n", $1}' /proc/loadavg
# Output: Load: 1.23
```

### Git Branch (in ~/project)
```bash
#!/bin/bash
cd ~/project
git branch --show-current 2>/dev/null || echo "not a repo"
# Output: main
```

## Visual Layout Example

On a 1920px wide monitor with the full config above:

```
┌────────────────────────────────────────────────────────────────────────┐
│ CPU: 12%  MEM: 3.2G / 15.0G  3 days, 23 hours   Sunday, March 01 - 20:58  🔋 85% │
└────────────────────────────────────────────────────────────────────────┘
   ↑red        ↑green           ↑blue              ↑white             ↑yellow
```

- **Left side:** System stats (CPU, Memory, Uptime)
- **Center:** Date and time
- **Right side:** Battery status
- **Colors:** Catppuccin Mocha theme (configurable)
- **Height:** 32 pixels
- **Font:** Monospace

## Color Schemes

### Catppuccin Mocha (Default)
```json
{
  "background": "#1e1e2e",
  "foreground": "#cdd6f4",
  "red": "#f38ba8",
  "green": "#a6e3a1",
  "yellow": "#f9e2af",
  "blue": "#89b4fa"
}
```

### Dracula
```json
{
  "background": "#282a36",
  "foreground": "#f8f8f2",
  "red": "#ff5555",
  "green": "#50fa7b",
  "yellow": "#f1fa8c",
  "blue": "#8be9fd"
}
```

### Nord
```json
{
  "background": "#2e3440",
  "foreground": "#eceff4",
  "red": "#bf616a",
  "green": "#a3be8c",
  "yellow": "#ebcb8b",
  "blue": "#88c0d0"
}
```

## Installation & Testing

1. **Copy example widgets:**
   ```bash
   mkdir -p ~/.config/hyprbar/widgets
   cp examples/widgets/*.sh ~/.config/hyprbar/widgets/
   chmod +x ~/.config/hyprbar/widgets/*.sh
   ```

2. **Copy config:**
   ```bash
   cp examples/config-with-scripts.json ~/.config/hyprbar/config.json
   ```

3. **Test widgets individually:**
   ```bash
   ~/.config/hyprbar/widgets/cpu.sh
   ~/.config/hyprbar/widgets/memory.sh
   ```

4. **Run hyprbar:**
   ```bash
   ./bin/hyprbar
   ```

   **Note:** Requires wlroots-based compositor (Hyprland, Sway, River).
   GNOME Wayland is not supported (no wlr-layer-shell protocol).

## Troubleshooting

**Widget shows "ERROR":**
- Check script has execute permission (`chmod +x`)
- Test script manually in terminal
- Check command path in config

**Widget shows nothing:**
- Script might not be printing output
- Check script ends with `echo` statement
- Verify interval isn't too fast (minimum 100ms recommended)

**Bar doesn't appear:**
- Check you're running wlroots compositor (not GNOME)
- Check compositor logs for layer-shell errors
- Try `WAYLAND_DEBUG=1 ./bin/hyprbar` for verbose output
