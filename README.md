# Hyprbar

A Wayland compositor bar for Hyprland written in C++.

## Features

- Native Wayland protocol integration
- Lightweight C++ implementation
- Designed for Hyprland compositor

## Building

### Prerequisites

- Clang compiler
- Wayland development libraries (`libwayland-dev`)
- Make

### Installation

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install clang libwayland-dev make

# Build
make

# Run
./bin/hyprbar
```

## Development

### Build Targets

- `make` - Build the project
- `make test` - Build and run tests
- `make clean` - Remove build artifacts
- `make debug` - Build with debug symbols
- `make release` - Build optimized release version
- `make install` - Install to /usr/local/bin
- `make uninstall` - Remove from /usr/local/bin

### Testing

```bash
make test
```

## License

MIT License

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.
