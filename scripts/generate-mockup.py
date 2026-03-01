#!/usr/bin/env python3
"""
Generate a mockup screenshot of hyprbar using Cairo
Shows what the bar would look like on a real compositor
"""

import cairo
import math

# Catppuccin Mocha colors
COLORS = {
    'background': (0x1e/255, 0x1e/255, 0x2e/255),
    'foreground': (0xcd/255, 0xd6/255, 0xf4/255),
    'red': (0xf3/255, 0x8b/255, 0xa8/255),
    'green': (0xa6/255, 0xe3/255, 0xa1/255),
    'yellow': (0xf9/255, 0xe2/255, 0xaf/255),
    'blue': (0x89/255, 0xb4/255, 0xfa/255),
}

# Bar dimensions
WIDTH = 1920
HEIGHT = 32

# Create surface
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, WIDTH, HEIGHT)
ctx = cairo.Context(surface)

# Fill background
ctx.set_source_rgb(*COLORS['background'])
ctx.rectangle(0, 0, WIDTH, HEIGHT)
ctx.fill()

# Font settings
ctx.select_font_face("monospace", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
ctx.set_font_size(14)

# Helper to draw text
def draw_text(text, x, color_name):
    ctx.set_source_rgb(*COLORS[color_name])
    ctx.move_to(x, HEIGHT / 2 + 5)  # Vertically centered
    ctx.show_text(text)
    extents = ctx.text_extents(text)
    return x + extents.width + 20  # Return next x position with spacing

# Left side widgets
x = 10
x = draw_text("CPU: 12%", x, 'red')
x = draw_text("MEM: 3.2G / 15.0G", x, 'green')
x = draw_text("3 days, 23 hours", x, 'blue')

# Center widget (date/time)
center_text = "Sunday, March 01 - 20:58"
extents = ctx.text_extents(center_text)
center_x = (WIDTH - extents.width) / 2
draw_text(center_text, center_x, 'foreground')

# Right side widget
right_text = "🔋 85%"
extents = ctx.text_extents(right_text)
right_x = WIDTH - extents.width - 10
draw_text(right_text, right_x, 'yellow')

# Save
surface.write_to_png('/home/martian/.openclaw/workspace/hyprbar/docs/mockup-screenshot.png')
print("✓ Mockup saved to docs/mockup-screenshot.png")
