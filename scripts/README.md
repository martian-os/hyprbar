# Scripts

## screenshot.sh

**Purpose:** Generate test renders of widgets without running the bar.

**How it works:**
1. Creates temporary C++ program
2. Loads config from `~/.config/hyprbar/config.json`
3. Initializes widgets and renderer
4. Renders one frame
5. Saves Cairo surface to PNG

**What this is:**
- ✅ Proof that widgets render correctly
- ✅ Test for visual appearance
- ✅ Useful for documentation/development

**What this is NOT:**
- ❌ NOT a screenshot of the running bar
- ❌ NOT running the actual `./bin/hyprbar` binary
- ❌ NOT testing compositor integration
- ❌ NOT showing real-time updates

**Limitations:**
- No compositor effects (shadows, blur, transparency)
- No layer-shell positioning
- No live updates (static frame)
- Requires building from source

**For real testing:** Run `./bin/hyprbar` on Hyprland, Sway, or River compositor.

## test-clock-render.sh

Simple test that renders clock widget in isolation. Used for debugging widget rendering issues.
