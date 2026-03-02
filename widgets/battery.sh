#!/bin/bash
# Battery widget for hyprbar
# Prints battery percentage and charging status

battery="/sys/class/power_supply/BAT0"

if [ ! -d "$battery" ]; then
    echo "No Battery"
    exit 0
fi

capacity=$(cat "$battery/capacity" 2>/dev/null || echo "0")
status=$(cat "$battery/status" 2>/dev/null || echo "Unknown")

case "$status" in
    "Charging") icon="🔌" ;;
    "Discharging") icon="🔋" ;;
    "Full") icon="✓" ;;
    *) icon="?" ;;
esac

echo "$icon ${capacity}%"
