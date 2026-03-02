#!/bin/bash
# Disk usage widget for hyprbar
# Shows used space for root partition

df -h / | awk 'NR==2 {print "DISK: " $3 " / " $2 " (" $5 ")"}'
