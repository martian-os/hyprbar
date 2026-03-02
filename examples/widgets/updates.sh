#!/bin/bash
# System updates widget for hyprbar
# Shows number of available updates (Ubuntu/Debian)

cache_file="/tmp/hyprbar-updates-cache"
cache_duration=3600  # 1 hour

# Check cache
if [ -f "$cache_file" ]; then
    cache_age=$(($(date +%s) - $(stat -c %Y "$cache_file" 2>/dev/null || echo 0)))
    if [ $cache_age -lt $cache_duration ]; then
        cat "$cache_file"
        exit 0
    fi
fi

# Check for updates
if command -v apt >/dev/null 2>&1; then
    # Update package lists silently (requires sudo or running as root)
    count=$(apt list --upgradable 2>/dev/null | grep -c 'upgradable')
    
    if [ "$count" -gt 0 ]; then
        echo "📦 $count updates" | tee "$cache_file"
    else
        echo "✓ Up to date" | tee "$cache_file"
    fi
else
    echo "UPDATES: N/A"
fi
