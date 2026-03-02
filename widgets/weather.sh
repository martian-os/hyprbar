#!/bin/bash
# Weather widget for hyprbar
# Shows current weather using wttr.in
# Requires: curl

cache_file="/tmp/hyprbar-weather-cache"
cache_duration=1800  # 30 minutes

# Check cache
if [ -f "$cache_file" ]; then
    cache_age=$(($(date +%s) - $(stat -c %Y "$cache_file" 2>/dev/null || echo 0)))
    if [ $cache_age -lt $cache_duration ]; then
        cat "$cache_file"
        exit 0
    fi
fi

# Fetch weather (use Berlin as default, can be customized)
# Use --connect-timeout and --max-time to avoid hanging
location="${HYPRBAR_LOCATION:-Berlin}"
weather=$(curl -s --connect-timeout 3 --max-time 5 "wttr.in/${location}?format=%c+%t" 2>/dev/null | tr -d '\n')

if [ -n "$weather" ]; then
    echo "$weather" | tee "$cache_file"
else
    # If fetch fails, check if we have old cached data
    if [ -f "$cache_file" ]; then
        cat "$cache_file"
    else
        echo "Weather: N/A"
    fi
fi
