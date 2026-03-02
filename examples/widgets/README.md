# Hyprbar Widget Scripts

This directory contains example shell script widgets for hyprbar. Script widgets allow you to create custom widgets in any scripting language.

## Available Widgets

### System Information

- **cpu.sh** - Shows CPU usage percentage
  - Updates: Every 2 seconds
  - Format: `CPU: 45%`
  - Stateful: Maintains state in `/tmp/hyprbar-cpu-state`

- **memory.sh** - Shows memory usage
  - Updates: Every 5 seconds  
  - Format: `MEM: 4.2G / 16.0G`

- **disk.sh** - Shows disk usage for root partition
  - Updates: Every minute
  - Format: `DISK: 120G / 500G (24%)`

- **battery.sh** - Shows battery level and charging status
  - Updates: Every 30 seconds
  - Format: `🔋 75%` or `🔌 75%` (charging)
  - Note: Shows "No Battery" on desktops

### Network & Connectivity

- **network.sh** - Shows active network interface
  - Updates: Every 10 seconds
  - Format: `📡 MyWiFi (-45 dBm)` or `🔌 eth0: 192.168.1.100`
  - Requires: `iw` for wireless info

### Time & Date

- **date.sh** - Shows current date and time
  - Updates: Every second
  - Format: `Monday, March 02 - 14:30`

- **uptime.sh** - Shows system uptime
  - Updates: Every minute
  - Format: `2 days, 5 hours`

### Media

- **volume.sh** - Shows audio volume
  - Updates: Every 5 seconds
  - Format: `🔊 75%` or `🔇 Muted`
  - Requires: `pactl` (PulseAudio) or `amixer` (ALSA)

### Other

- **weather.sh** - Shows current weather
  - Updates: Every 5 minutes (cached for 30 minutes)
  - Format: `⛅ +12°C`
  - Requires: `curl`
  - Environment: Set `HYPRBAR_LOCATION=YourCity` to customize location

- **updates.sh** - Shows available system updates
  - Updates: Every hour (cached)
  - Format: `📦 15 updates` or `✓ Up to date`
  - Requires: `apt` (Debian/Ubuntu)

## Installation

1. Create config directory:
```bash
mkdir -p ~/.config/hyprbar/widgets
```

2. Copy widget scripts:
```bash
cp examples/widgets/*.sh ~/.config/hyprbar/widgets/
chmod +x ~/.config/hyprbar/widgets/*.sh
```

3. Copy a config file:
```bash
# Minimal config (date, CPU, memory)
cp examples/config-minimal.json ~/.config/hyprbar/config.json

# Full config (all widgets)
cp examples/config-full.json ~/.config/hyprbar/config.json
```

## Creating Custom Widgets

Script widgets execute a command and display the last line of stdout. This makes it easy to create custom widgets in any language.

### Widget Configuration

```json
{
  "type": "script",
  "position": "left",
  "config": {
    "command": "/path/to/script.sh",
    "interval": 1000,
    "font": "monospace",
    "size": 14,
    "color": "#ffffff"
  }
}
```

- **command**: Script to execute (required)
- **interval**: Update interval in milliseconds (default: 1000)
- **font**: Font name (default: "monospace")
- **size**: Font size in points (default: 14)
- **color**: Text color in hex (default: "#ffffff")

### Example: Custom Widget in Bash

```bash
#!/bin/bash
# custom-widget.sh - Shows random emoji

emojis=("😀" "😎" "🚀" "⚡" "🎉")
random_index=$((RANDOM % ${#emojis[@]}))
echo "${emojis[$random_index]}"
```

Make it executable and add to config:
```bash
chmod +x ~/.config/hyprbar/widgets/custom-widget.sh
```

```json
{
  "type": "script",
  "position": "center",
  "config": {
    "command": "~/.config/hyprbar/widgets/custom-widget.sh",
    "interval": 5000,
    "size": 20
  }
}
```

### Example: Custom Widget in Python

```python
#!/usr/bin/env python3
# github-stars.py - Shows GitHub stars for a repo

import requests
import sys

repo = "hyprwm/Hyprland"
try:
    r = requests.get(f"https://api.github.com/repos/{repo}", timeout=5)
    stars = r.json()["stargazers_count"]
    print(f"⭐ {stars:,}")
except:
    print("⭐ N/A")
```

### Best Practices

1. **Output only the last line** - Hyprbar displays only the last line of stdout
2. **Keep scripts fast** - Widget updates should complete in <100ms
3. **Use caching** - For expensive operations (API calls, etc.), cache results
4. **Handle errors gracefully** - Always have a fallback output
5. **Use state files** - Store state in `/tmp/hyprbar-*` for stateful widgets
6. **Respect the interval** - Don't poll too frequently (respect system resources)

### Debugging

Test your widget script manually:
```bash
~/.config/hyprbar/widgets/your-script.sh
```

Check hyprbar logs:
```bash
hyprbar --config ~/.config/hyprbar/config.json 2>&1 | grep -i error
```

## Color Scheme

The example configs use Catppuccin Mocha colors:

- `#89b4fa` - Blue (date/time)
- `#f38ba8` - Red (CPU)
- `#fab387` - Peach (memory)
- `#a6e3a1` - Green (disk)
- `#b4befe` - Lavender (volume)
- `#74c7ec` - Sapphire (network)
- `#cba6f7` - Mauve (battery)
- `#94e2d5` - Teal (uptime)
- `#f9e2af` - Yellow (weather)

Customize colors in your config as needed!
