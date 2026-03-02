# Hyprbar

A modular Wayland status bar for wlroots-based compositors (Hyprland, Sway, River), written in C++17.

![CI Status](https://github.com/martian-os/hyprbar/actions/workflows/ci.yml/badge.svg)

## Features

- 🎨 **Native Wayland:** Uses wlr-layer-shell protocol for proper bar positioning
- ⚡ **Lightweight:** Pure C++17 with minimal dependencies (Cairo, Wayland)
- 🔧 **Modular Widgets:** ScriptWidget system allows widgets in any language
- 📸 **Screenshot Mode:** Test bar appearance without running compositor
- 🎯 **Event Loop:** epoll-based for efficient Wayland integration
- 🛡️ **Quality Enforced:** CI with clang-format, pre-commit hooks

## Compatibility

**✅ Works On:**
- Hyprland
- Sway
- River
- Any wlroots-based compositor

**❌ Does NOT Work On:**
- GNOME Wayland (no wlr-layer-shell)
- KDE Plasma Wayland (different protocol)

## Quick Start

### Prerequisites

```bash
# Debian/Ubuntu
sudo apt install clang libwayland-dev libcairo2-dev libpango1.0-dev make

# Arch
sudo pacman -S clang wayland cairo pango make
```

### Build & Run

```bash
# Clone
git clone https://github.com/martian-os/hyprbar.git
cd hyprbar

# Build
make

# Install widgets and config
mkdir -p ~/.config/hyprbar/widgets
cp widgets/*.sh ~/.config/hyprbar/widgets/
chmod +x ~/.config/hyprbar/widgets/*.sh
cp examples/config-minimal.json ~/.config/hyprbar/config.json

# Run (on Hyprland/Sway/River)
./bin/hyprbar

# Run with custom config
./bin/hyprbar --config /path/to/config.json

# Test without compositor (screenshot mode)
./bin/hyprbar --screenshot output.png
```

## Configuration

Default config location: `~/.config/hyprbar/config.json`

Specify custom config: `hyprbar --config /path/to/config.json`

### Bar Configuration

- **position**: `top`, `bottom`, `left`, `right`
- **height**: Bar height in pixels
- **background**: Background color in hex (#RRGGBB or #RRGGBBAA for transparency)
- **foreground**: Default text color (inherited by widgets)
- **font**: Font family (e.g., "Noto Sans, Noto Color Emoji")
- **size**: Font size in points (default: 14)

### Widget Configuration

Widgets **inherit** font, size, and color from bar defaults. Override only when needed.

**Inheritance example:**
```json
{
  "bar": {
    "background": "#1e1e2ecc",
    "foreground": "#cdd6f4",
    "font": "Noto Sans, Noto Color Emoji",
    "size": 14
  },
  "widgets": [
    {
      "type": "script",
      "position": "left",
      "config": {
        "command": "widgets/date.sh",
        "interval": 1000
        // Inherits: font, size (14), color (#cdd6f4)
      }
    },
    {
      "type": "script",
      "position": "right",
      "config": {
        "command": "widgets/cpu.sh",
        "interval": 2000,
        "size": 12,  // Override: smaller text
        "color": "#f38ba8"  // Override: red color
      }
    }
  ]
}
```

**Widget properties:**
- **type**: Widget type (`script`)
- **position**: `left`, `center`, `right`
- **config**: Widget-specific configuration
  - **command**: Script/command to execute (required)
  - **interval**: Update interval in milliseconds (default: 1000)
  - **font**: Font family (inherits from bar if not set)
  - **size**: Font size (inherits from bar if not set)
  - **color**: Text color hex (inherits from bar.foreground if not set)

### Example Configuration

```json
{
  "bar": {
    "position": "top",
    "height": 30,
    "background": "#1e1e2ecc",
    "foreground": "#cdd6f4",
    "font": "Noto Sans, Noto Color Emoji",
    "size": 14
  },
  "widgets": [
    {
      "type": "script",
      "position": "left",
      "config": {
        "command": "widgets/date.sh",
        "interval": 1000
      }
    },
    {
      "type": "script",
      "position": "right",
      "config": {
        "command": "widgets/cpu.sh",
        "interval": 2000,
        "size": 12,
        "color": "#f38ba8"
      }
    }
  ]
}
```

**Transparency Support:** Use 8-digit hex colors for transparency:
- `#1e1e2eff` = 100% opaque (ff = 255)
- `#1e1e2ecc` = 80% opacity (cc = 204)
- `#1e1e2e99` = 60% opacity (99 = 153)
- `#1e1e2e66` = 40% opacity (66 = 102)

See `examples/config-transparent.json` for transparency examples.

## Widget System

Hyprbar uses **ScriptWidget** - any script that prints to stdout can be a widget!

### Example CPU Widget (`~/.config/hyprbar/widgets/cpu.sh`)

```bash
#!/bin/bash
# Read CPU usage from /proc/stat
prev=$(cat /tmp/hyprbar_cpu_prev 2>/dev/null || echo "0 0")
read prev_idle prev_total <<< "$prev"

cpu_line=$(head -1 /proc/stat)
read -a vals <<< "${cpu_line#cpu }"
idle=${vals[3]}
total=0
for v in "${vals[@]}"; do total=$((total + v)); done

idle_delta=$((idle - prev_idle))
total_delta=$((total - prev_total))
usage=$((100 * (total_delta - idle_delta) / total_delta))

echo "$idle $total" > /tmp/hyprbar_cpu_prev
echo "CPU: ${usage}%"
```

### Pre-built Widgets

Included in `widgets/`:

**System Info:**
- `cpu.sh` - CPU usage percentage
- `memory.sh` - RAM usage (used/total)
- `disk.sh` - Disk usage for root partition
- `battery.sh` - Battery level and charging status
- `uptime.sh` - System uptime

**Network & Media:**
- `network.sh` - Active network interface and IP/WiFi info
- `volume.sh` - Audio volume and mute status

**Time:**
- `date.sh` - Custom date/time formatting

**Other:**
- `updates.sh` - Available system updates (apt)

See [widgets/README.md](widgets/README.md) for detailed widget documentation and how to create custom widgets.

## Development

### Build Targets

```bash
make           # Build project
make test      # Run tests
make clean     # Remove build artifacts
make debug     # Build with debug symbols
make release   # Optimized release build
make lint      # Check code formatting
make install   # Install to /usr/local/bin
```

### Project Structure

```
hyprbar/
├── src/
│   ├── core/           # Event loop, config, logging
│   ├── wayland/        # Wayland protocol integration
│   ├── rendering/      # Cairo rendering, surfaces
│   └── widgets/        # Widget system
├── include/hyprbar/    # Public headers
├── tests/              # Unit tests
├── widgets/            # Widget script collection
├── examples/           # Example configurations
└── docs/               # Architecture & guides
```

### Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed design documentation.

**Key Design Decisions:**
- **EventLoop over async/await:** Direct epoll integration with Wayland
- **Flex-like layout:** Familiar to web developers
- **ScriptWidget pattern:** Any language can create widgets
- **Dual surface rendering:** Wayland compositor + file-based screenshots

## Contributing

Contributions welcome! Guidelines:

1. **Code Quality:** Passes clang-format and pre-commit checks
2. **Tests:** Add tests for new features
3. **Documentation:** Update relevant docs
4. **Architecture:** Follow existing patterns (see ARCHITECTURE.md)

## Documentation

- [Architecture](docs/ARCHITECTURE.md) - System design and patterns
- [Examples](docs/EXAMPLES.md) - Widget examples and configuration
- [Implementation Status](docs/IMPLEMENTATION_STATUS.md) - Current progress
- [Code Quality](docs/CODE_QUALITY.md) - Coding standards

## License

MIT License - See [LICENSE](LICENSE) for details

## Credits

Built by [Martian](https://github.com/martian-os) for the Hyprland community.

Uses:
- [Wayland](https://wayland.freedesktop.org/) - Display protocol
- [wlr-protocols](https://gitlab.freedesktop.org/wlroots/wlr-protocols) - Layer shell
- [Cairo](https://www.cairographics.org/) - 2D graphics
- [Pango](https://pango.gnome.org/) - Text rendering
