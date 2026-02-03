#!/usr/bin/env bash
# TaijiOS WM-Only AppImage Build Script
# Creates a minimal AppImage with only the Window Manager essentials
# This results in a smaller distribution (~50MB vs ~100MB for full)
#
# Usage on NixOS:
#   nix-shell build/shell-appimage.nix --run './build/build-wm-appimage.sh'

set -e

# ============================================================================
# Configuration
# ============================================================================
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="${SCRIPT_DIR}/.."  # Go up from build/ to project root

APP_NAME="TaijiOS"
APP_VERSION="1.0-wm"
APPDIR="${ROOT}/build/${APP_NAME}.AppDir"
OUTPUT_DIR="${ROOT}/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "=== Building ${APP_NAME} WM-Only AppImage ==="
echo "========================================"
echo ""

# ============================================================================
# Helper Functions
# ============================================================================

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_binary() {
    if [ ! -f "$1" ]; then
        log_error "Required binary not found: $1"
        return 1
    fi
    return 0
}

# ============================================================================
# Step 1: Build TaijiOS
# ============================================================================
log_info "Step 1: Building TaijiOS..."

# Change to ROOT directory to run the build
cd "$ROOT"

if [ -f "./run.sh" ]; then
    chmod +x ./run.sh
    ./run.sh --build
else
    log_error "run.sh not found. Cannot build TaijiOS."
    exit 1
fi

# Verify emu binary exists
EMU_BIN="$ROOT/Linux/amd64/bin/emu"
if ! check_binary "$EMU_BIN"; then
    log_error "Emulator binary not found after build: $EMU_BIN"
    exit 1
fi

log_info "TaijiOS build complete."
echo ""

# ============================================================================
# Step 2: Create AppDir Structure
# ============================================================================
log_info "Step 2: Creating AppDir structure..."

# Clean up any existing AppDir
rm -rf "$APPDIR"

# Create main AppDir directories
mkdir -p "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/share/icons/hicolor/scalable/apps"
mkdir -p "$APPDIR/ROOT"

log_info "AppDir structure created."
echo ""

# ============================================================================
# Step 3: Copy Emulator Binary
# ============================================================================
log_info "Step 3: Copying emulator binary..."

cp "$EMU_BIN" "$APPDIR/emu"
chmod +x "$APPDIR/emu"

EMU_SIZE=$(du -h "$APPDIR/emu" | cut -f1)
log_info "Emulator copied (${EMU_SIZE})."
echo ""

# ============================================================================
# Step 4: Copy TaijiOS Root Filesystem (WM-Only)
# ============================================================================
log_info "Step 4: Copying TaijiOS filesystem (WM-Only)..."

# Essential dis files - create directory structure first
mkdir -p "$APPDIR/ROOT/dis/wm"
mkdir -p "$APPDIR/ROOT/dis/lib"
mkdir -p "$APPDIR/ROOT/dis/cmd"

# Core system files
ESSENTIAL_DIS=(
    "sh.dis"
    "wminit.dis"
    "emuinit.dis"
    "disdep.dis"
    "load.dis"
    "os.dis"
)

# Copy core dis files
for f in "${ESSENTIAL_DIS[@]}"; do
    if [ -f "dis/$f" ]; then
        cp "dis/$f" "$APPDIR/ROOT/dis/"
        log_info "  - dis/$f"
    fi
done

# Copy WM directory
if [ -d "dis/wm" ]; then
    cp -r dis/wm "$APPDIR/ROOT/dis/"
    WM_SIZE=$(du -sh "$APPDIR/ROOT/dis/wm" | cut -f1)
    log_info "  - dis/wm/ copied (${WM_SIZE})"
fi

# Copy essential lib files
if [ -d "dis/lib" ]; then
    cp -r dis/lib "$APPDIR/ROOT/dis/"
    log_info "  - dis/lib/ copied"
fi

# Copy module definitions (needed for runtime)
if [ -d "module" ]; then
    cp -r module "$APPDIR/ROOT/"
    MODULE_SIZE=$(du -sh "$APPDIR/ROOT/module" | cut -f1)
    log_info "  - module/ copied (${MODULE_SIZE})"
else
    log_warn "module directory not found, skipping..."
fi

# Copy usr directory (minimal)
if [ -d "usr" ]; then
    mkdir -p "$APPDIR/ROOT/usr"
    # Copy only essential parts of usr if needed
    cp -r usr/*.Dis "$APPDIR/ROOT/usr/" 2>/dev/null || true
    log_info "  - usr/ (essential files) copied"
else
    log_warn "usr directory not found, skipping..."
fi

# Copy lib directory (contains wmsetup and other config files needed by WM)
if [ -d "lib" ]; then
    cp -r lib "$APPDIR/ROOT/"
    LIB_SIZE=$(du -sh "$APPDIR/ROOT/lib" | cut -f1)
    log_info "  - lib/ copied (${LIB_SIZE})"
else
    log_warn "lib directory not found, WM may not work properly..."
fi

# Copy fonts (only essential ones - lucida, lucm, misc)
if [ -d "fonts" ]; then
    mkdir -p "$APPDIR/ROOT/fonts"

    # Copy only essential font directories
    for fontdir in lucida lucm misc; do
        if [ -d "fonts/$fontdir" ]; then
            cp -r "fonts/$fontdir" "$APPDIR/ROOT/fonts/"
            FONT_SIZE=$(du -sh "$APPDIR/ROOT/fonts/$fontdir" | cut -f1)
            log_info "  - fonts/$fontdir/ copied (${FONT_SIZE})"
        fi
    done
else
    log_error "fonts directory not found!"
    exit 1
fi

# Copy icons/bitmaps (needed for toolbar and WM UI)
if [ -d "icons" ]; then
    cp -r icons "$APPDIR/ROOT/"
    ICONS_SIZE=$(du -sh "$APPDIR/ROOT/icons" | cut -f1)
    log_info "  - icons/ copied (${ICONS_SIZE})"
else
    log_warn "icons directory not found, WM may be missing bitmaps..."
fi

echo ""

# ============================================================================
# Step 5: Create Runtime Directories
# ============================================================================
log_info "Step 5: Creating runtime directories..."

# Standard directories
mkdir -p "$APPDIR/ROOT/tmp"
mkdir -p "$APPDIR/ROOT/mnt"

# Network namespace structure
mkdir -p "$APPDIR/ROOT/n"
mkdir -p "$APPDIR/ROOT/n"/{cd,client,chan,dev,disk,dist,dump,ftp,gridfs,kfs,local,rdbg,registry,remote}
mkdir -p "$APPDIR/ROOT/n/client"/{chan,dev}
mkdir -p "$APPDIR/ROOT/services/logs"

# Set permissions (match mkfile behavior)
chmod 555 "$APPDIR/ROOT/n"
chmod 755 "$APPDIR/ROOT/tmp"
chmod 755 "$APPDIR/ROOT/mnt"

log_info "Runtime directories created."
echo ""

# ============================================================================
# Step 6: Create AppRun Launcher Script
# ============================================================================
log_info "Step 6: Creating AppRun launcher script..."

cat > "$APPDIR/AppRun" <<'APPRUN_EOF'
#!/bin/bash
# TaijiOS AppImage Launcher (WM-Only)
# This script sets up the environment and launches emu with the WM

set -e

# Get AppImage mount point
SELF=$(readlink -f "$0")
HERE=${SELF%/*}

# Set up environment
export ROOT="${HERE}/ROOT"
export PATH="${HERE}/ROOT/Linux/amd64/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"

# Ensure runtime directories exist
mkdir -p "$ROOT/tmp" "$ROOT/mnt"

# Check for display
if [ -z "$DISPLAY" ]; then
    echo "Error: DISPLAY environment variable not set"
    echo "Are you running in a graphical environment?"
    exit 1
fi

# Check if wminit.dis exists
if [ ! -f "$ROOT/dis/wminit.dis" ]; then
    echo "Error: wminit.dis not found in $ROOT/dis/"
    exit 1
fi

# Check if wm.dis exists
if [ ! -f "$ROOT/dis/wm/wm.dis" ]; then
    echo "Error: wm.dis not found in $ROOT/dis/wm/"
    exit 1
fi

# Launch Window Manager
cd "$HERE"
exec ./emu -r "$ROOT" /dis/wm/wm.dis "$@"
APPRUN_EOF

chmod +x "$APPDIR/AppRun"
log_info "AppRun launcher created."
echo ""

# ============================================================================
# Step 7: Copy/Bundle Libraries
# ============================================================================
log_info "Step 7: Bundling libraries..."

# Get library paths from ldd
LIBS_COPIED=0

# Try to copy NixOS libraries first
for lib in $(ldd "$EMU_BIN" | grep -o '/nix/store[^ ]*\.so[^ ]*' | sort -u); do
    if [ -f "$lib" ]; then
        cp "$lib" "$APPDIR/usr/lib/"
        LIBS_COPIED=$((LIBS_COPIED + 1))
    fi
done

# If no NixOS libs found, try system libraries
if [ "$LIBS_COPIED" -eq 0 ]; then
    log_warn "No NixOS libraries found, trying system libraries..."

    for lib in libX11.so.6 libXext.so.6 libxcb.so.1 libXau.so.6 libXdmcp.so.6; do
        path=$(find /usr/lib /usr/lib64 /lib /lib64 -name "$lib" 2>/dev/null | head -1)
        if [ -n "$path" ]; then
            cp "$path" "$APPDIR/usr/lib/"
            LIBS_COPIED=$((LIBS_COPIED + 1))
            log_info "  - Copied: $lib"
        fi
    done
fi

if [ "$LIBS_COPIED" -gt 0 ]; then
    log_info "Bundled $LIBS_COPIED library files."
else
    log_warn "No libraries bundled - will rely on system libraries."
fi

echo ""

# ============================================================================
# Step 8: Create Desktop Entry
# ============================================================================
log_info "Step 8: Creating desktop entry..."

cat > "$APPDIR/taijios.desktop" <<'DESKTOP_EOF'
[Desktop Entry]
Name=TaijiOS (WM)
GenericName=Inferno OS Window Manager
Comment=Run the TaijiOS Window Manager (minimal edition)
Exec=apprun %F
Icon=taijios
Terminal=false
Type=Application
Categories=System;Emulator;
StartupNotify=true
StartupWMClass=TaijiOS
Keywords=Inferno;Plan9;Operating System;Window Manager;
X-AppImage-Name=TaijiOS
X-AppImage-Version=1.0-wm
DESKTOP_EOF

# Also place it in the standard location
mkdir -p "$APPDIR/usr/share/applications"
cp "$APPDIR/taijios.desktop" "$APPDIR/usr/share/applications/"

log_info "Desktop entry created."
echo ""

# ============================================================================
# Step 9: Create Icon
# ============================================================================
log_info "Step 9: Creating application icon..."

# Detect if we're on NixOS
ON_NIXOS=false
if [ -f /etc/NIXOS ]; then
    ON_NIXOS=true
fi

ICON_CREATED=false

# Function to create icon with ImageMagick
create_icon_imagemagick() {
    magick -size 256x256 xc:transparent \
        -fill white -draw "circle 128,128 128,10" \
        -fill black -draw "path 'M 128,10 A 118,118 0 0,1 128,246 A 59,59 0 0,1 128,128 Z'" \
        -fill white -draw "circle 128,75 128,62" \
        -fill black -draw "circle 128,181 128,168" \
        -stroke "#333" -strokewidth 2 -draw "circle 128,128 128,10" \
        "$1" 2>/dev/null
}

# Check if ImageMagick is available
if command -v magick >/dev/null 2>&1; then
    create_icon_imagemagick "$APPDIR/usr/share/icons/hicolor/256x256/apps/taijios.png"
    ICON_CREATED=true
    log_info "Icon created with ImageMagick (magick)."
elif command -v convert >/dev/null 2>&1; then
    # Old ImageMagick version
    convert -size 256x256 xc:transparent \
        -fill white -draw "circle 128,128 128,10" \
        -fill black -draw "path 'M 128,10 A 118,118 0 0,1 128,246 A 59,59 0 0,1 128,128 Z'" \
        -fill white -draw "circle 128,75 128,62" \
        -fill black -draw "circle 128,181 128,168" \
        -stroke "#333" -strokewidth 2 -draw "circle 128,128 128,10" \
        "$APPDIR/usr/share/icons/hicolor/256x256/apps/taijios.png" 2>/dev/null
    ICON_CREATED=true
    log_info "Icon created with ImageMagick (convert)."
elif [ "$ON_NIXOS" = true ]; then
    log_info "Using nix-shell to get ImageMagick for icon creation..."
    nix-shell -p imagemagick --run "
        magick -size 256x256 xc:transparent \
            -fill white -draw \"circle 128,128 128,10\" \
            -fill black -draw \"path 'M 128,10 A 118,118 0 0,1 128,246 A 59,59 0 0,1 128,128 Z'\" \
            -fill white -draw \"circle 128,75 128,62\" \
            -fill black -draw \"circle 128,181 128,168\" \
            -stroke \"#333\" -strokewidth 2 -draw \"circle 128,128 128,10\" \
            \"$APPDIR/usr/share/icons/hicolor/256x256/apps/taijios.png\" 2>/dev/null
    "
    ICON_CREATED=true
    log_info "Icon created with ImageMagick via nix-shell."
fi

# Create SVG icon
cat > "$APPDIR/usr/share/icons/hicolor/scalable/apps/taijios.svg" <<'SVG_EOF'
<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
  <!-- Main circle (white background) -->
  <circle cx="128" cy="128" r="118" fill="white" stroke="#333" stroke-width="2"/>
  <!-- Black half (left side, top to bottom through center) -->
  <path d="M 128,10 A 118,118 0 0,1 128,246 A 59,59 0 0,1 128,128 Z" fill="black"/>
  <!-- Small white circle in black half -->
  <circle cx="128" cy="75" r="13" fill="white"/>
  <!-- Small black circle in white half -->
  <circle cx="128" cy="181" r="13" fill="black"/>
</svg>
SVG_EOF

# Create symbolic links
ln -sf "usr/share/icons/hicolor/256x256/apps/taijios.png" "$APPDIR/.DirIcon"
ln -sf "usr/share/icons/hicolor/256x256/apps/taijios.png" "$APPDIR/taijios.png"

if [ "$ICON_CREATED" = false ]; then
    log_warn "Could not create PNG icon. Please create manually."
    touch "$APPDIR/usr/share/icons/hicolor/256x256/apps/taijios.png"
fi

echo ""

# ============================================================================
# Step 10: Download appimagetool
# ============================================================================
log_info "Step 10: Checking for appimagetool..."

APPIMAGETOOL="$OUTPUT_DIR/appimagetool-x86_64.AppImage"

if [ ! -f "$APPIMAGETOOL" ]; then
    log_info "Downloading appimagetool..."
    wget -q --show-progress \
        "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" \
        -O "$APPIMAGETOOL"
    chmod +x "$APPIMAGETOOL"
    log_info "appimagetool downloaded."
else
    log_info "appimagetool already present."
fi

echo ""

# ============================================================================
# Step 11: Build AppImage
# ============================================================================
log_info "Step 11: Building AppImage..."

# Ensure output directory exists
mkdir -p "$OUTPUT_DIR"

# Build the AppImage
# Unset SOURCE_DATE_EPOCH to avoid conflict with mksquashfs (NixOS issue)
unset SOURCE_DATE_EPOCH

ARCH=x86_64 "$APPIMAGETOOL" "$APPDIR" "$OUTPUT_DIR/${APP_NAME}-${APP_VERSION}-x86_64.AppImage"

echo ""
echo "========================================"
log_info "WM-Only AppImage created successfully!"
echo "========================================"
echo ""
echo "Location: $OUTPUT_DIR/${APP_NAME}-${APP_VERSION}-x86_64.AppImage"

# Show final size
FINAL_SIZE=$(du -h "$OUTPUT_DIR/${APP_NAME}-${APP_VERSION}-x86_64.AppImage" | cut -f1)
echo "Size: $FINAL_SIZE"
echo ""
echo "To run:"
echo "  chmod +x $OUTPUT_DIR/${APP_NAME}-${APP_VERSION}-x86_64.AppImage"
echo "  ./$OUTPUT_DIR/${APP_NAME}-${APP_VERSION}-x86_64.AppImage"
echo ""
