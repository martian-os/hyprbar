#!/bin/bash
# Memory usage widget for hyprbar
# Prints memory used / total

awk '/MemTotal/ {total=$2} /MemAvailable/ {avail=$2} END {
    used = total - avail
    printf "MEM: %.1fG / %.1fG\n", used/1024/1024, total/1024/1024
}' /proc/meminfo
