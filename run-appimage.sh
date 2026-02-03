#!/usr/bin/env bash
# TaijiOS AppImage Launcher
# Simple script to run TaijiOS AppImage using appimage-run

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APPIMAGE="$SCRIPT_DIR/build/TaijiOS-1.0-x86_64.AppImage"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=== TaijiOS AppImage Launcher ==="
echo ""

# Check if AppImage exists
if [ ! -f "$APPIMAGE" ]; then
    echo -e "${YELLOW}AppImage not found at:$NC $APPIMAGE"
    echo ""
    echo "Build it first with:"
    echo "  nix-shell build/shell-appimage.nix --run './build/build-appimage.sh'"
    echo ""
    exit 1
fi

# Check if appimage-run is available
if ! command -v appimage-run >/dev/null 2>&1; then
    echo -e "${YELLOW}appimage-run not found.${NC}"
    echo ""
    echo "On NixOS, install it with:"
    echo "  nix-shell -p appimage-run"
    echo ""
    echo "Or add to configuration.nix:"
    echo "  environment.systemPackages = [ pkgs.appimage-run ];"
    echo ""
    exit 1
fi

# Check for X11
if [ -z "$DISPLAY" ]; then
    echo -e "${YELLOW}Warning: DISPLAY not set. Are you in a graphical session?${NC}"
fi

echo "Launching TaijiOS..."
echo ""

# Run the AppImage
exec appimage-run "$APPIMAGE" "$@"
