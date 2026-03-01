#!/bin/bash
# CPU usage widget for hyprbar
# Prints current CPU usage percentage

# Read CPU stats
prev_idle=0
prev_total=0
state_file="/tmp/hyprbar-cpu-state"

if [ -f "$state_file" ]; then
    read prev_total prev_idle < "$state_file"
fi

# Get current stats
cpu_line=$(grep '^cpu ' /proc/stat)
read -r cpu user nice system idle iowait irq softirq steal guest guest_nice <<< "$cpu_line"

idle=$((idle + iowait))
total=$((user + nice + system + idle + iowait + irq + softirq + steal))

# Calculate usage
if [ "$prev_total" -gt 0 ]; then
    total_diff=$((total - prev_total))
    idle_diff=$((idle - prev_idle))
    
    if [ $total_diff -gt 0 ]; then
        usage=$((100 * (total_diff - idle_diff) / total_diff))
        echo "CPU: ${usage}%"
    else
        echo "CPU: 0%"
    fi
else
    echo "CPU: --"
fi

# Save state
echo "$total $idle" > "$state_file"
