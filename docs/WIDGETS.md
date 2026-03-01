# Example Widgets

Hyprbar uses the **ScriptWidget** to allow writing widgets in any language. Each script prints output to stdout, and the last line is displayed in the bar.

## CPU Usage (`cpu.sh`)

Reads `/proc/stat` to calculate CPU usage percentage.

```bash
./examples/widgets/cpu.sh
# Output: CPU: 25%
```

**Update interval:** 2 seconds

## Memory Usage (`memory.sh`)

Reads `/proc/meminfo` to show used/total memory.

```bash
./examples/widgets/memory.sh
# Output: MEM: 8.2G / 16.0G
```

**Update interval:** 2 seconds

## Battery (`battery.sh`)

Reads `/sys/class/power_supply/BAT0` for battery status.

```bash
./examples/widgets/battery.sh
# Output: 🔋 85%
# or: 🔌 95% (charging)
```

**Update interval:** 5 seconds

## Custom Widgets

Write your own in any language! Examples:

**Network status (Ruby):**
```ruby
#!/usr/bin/env ruby
ip = `ip addr show wlan0 | grep 'inet ' | awk '{print $2}'`.strip
puts ip.empty? ? "Disconnected" : "📶 #{ip}"
```

**Weather (Python):**
```python
#!/usr/bin/env python3
import requests
data = requests.get('https://wttr.in/?format=%t').text
print(f"🌡️ {data}")
```

**Load average (Shell):**
```bash
#!/bin/bash
awk '{print "Load: " $1}' /proc/loadavg
```

## Configuration

Add to `~/.config/hyprbar/config.json`:

```json
{
  "widgets": [
    {
      "type": "script",
      "config": {
        "command": "/path/to/your-script.sh",
        "interval": 1000,
        "font": "monospace",
        "size": 14,
        "color": "#ffffff"
      }
    }
  ]
}
```

- `command`: Path to executable script
- `interval`: Update interval in milliseconds
- `font`, `size`, `color`: Optional styling

## Installation

Copy example scripts to your config directory:

```bash
mkdir -p ~/.config/hyprbar/widgets
cp examples/widgets/*.sh ~/.config/hyprbar/widgets/
chmod +x ~/.config/hyprbar/widgets/*.sh
```
