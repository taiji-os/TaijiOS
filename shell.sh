#!/bin/sh
# TaijiOS Shell Launcher

# Change to script directory
cd "$(dirname "$0")"

echo "=== TaijiOS Shell ==="
echo "Starting TaijiOS shell..."
echo "Type 'exit' to quit"
echo ""

# Run the TaijiOS shell
exec ./Linux/amd64/bin/emu -r. dis/sh.dis
