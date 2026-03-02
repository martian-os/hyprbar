#!/bin/bash
# Network widget for hyprbar
# Shows active network interface and status

# Find the first active non-loopback interface
interface=$(ip route get 1.1.1.1 2>/dev/null | grep -oP 'dev \K\S+' | head -1)

if [ -z "$interface" ]; then
    echo "NET: Offline"
    exit 0
fi

# Get IP address
ip_addr=$(ip addr show "$interface" | grep 'inet ' | awk '{print $2}' | cut -d/ -f1)

# Check if wireless
if [ -d "/sys/class/net/$interface/wireless" ]; then
    # Get SSID and signal strength
    ssid=$(iw dev "$interface" link | grep 'SSID' | awk '{print $2}')
    signal=$(iw dev "$interface" link | grep 'signal' | awk '{print $2}')
    echo "📡 $ssid ($signal dBm)"
else
    # Wired connection
    echo "🔌 $interface: $ip_addr"
fi
