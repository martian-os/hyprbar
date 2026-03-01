#!/bin/bash
# CPU usage widget for hyprbar
# Prints current CPU usage percentage

# Read CPU stats from /proc/stat
read cpu user nice system idle < <(grep '^cpu ' /proc/stat)

# Calculate total and idle time
total=$((user + nice + system + idle))
idle=$idle

# Save to temp file for next calculation
state_file="/tmp/hyprbar-cpu-$$"

if [ -f "$state_file" ]; then
    read prev_total prev_idle < "$state_file"
    
    total_diff=$((total - prev_total))
    idle_diff=$((idle - prev_idle))
    
    if [ $total_diff -gt 0 ]; then
        usage=$((100 * (total_diff - idle_diff) / total_diff))
        echo "CPU: ${usage}%"
    else
        echo "CPU: 0%"
    fi
fi

echo "$total $idle" > "$state_file"

# First run fallback
if [ ! -s "$state_file" ]; then
    echo "CPU: --"
fi
