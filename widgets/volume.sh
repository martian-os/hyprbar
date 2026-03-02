#!/bin/bash
# Volume widget for hyprbar
# Shows current audio volume (requires pactl or amixer)

if command -v pactl >/dev/null 2>&1; then
    # PulseAudio
    volume=$(pactl get-sink-volume @DEFAULT_SINK@ 2>/dev/null | grep -oP '\d+%' | head -1 || echo "0%")
    muted=$(pactl get-sink-mute @DEFAULT_SINK@ 2>/dev/null | grep -q 'yes' && echo "true" || echo "false")
elif command -v amixer >/dev/null 2>&1; then
    # ALSA
    volume=$(amixer get Master | grep -oP '\d+%' | head -1 || echo "0%")
    muted=$(amixer get Master | grep -q '\[off\]' && echo "true" || echo "false")
else
    echo "VOL: N/A"
    exit 0
fi

if [ "$muted" = "true" ]; then
    echo "🔇 Muted"
else
    echo "🔊 $volume"
fi
