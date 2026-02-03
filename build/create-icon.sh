#!/usr/bin/env bash
# TaijiOS Icon Creation Script
# Creates a yin-yang inspired icon for TaijiOS

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/../build/TaijiOS.AppDir/usr/share/icons/hicolor"
PNG_DIR="$OUTPUT_DIR/256x256/apps"
SCALABLE_DIR="$OUTPUT_DIR/scalable/apps"

# Create directories
mkdir -p "$PNG_DIR"
mkdir -p "$SCALABLE_DIR"

echo "Creating TaijiOS icon..."

# Create PNG icon (256x256) using ImageMagick
convert -size 256x256 xc:transparent \
    # Background circle (white)
    -fill white \
    -draw "circle 128,128 128,10" \
    # Black half (left side)
    -fill black \
    -draw "path 'M 128,10 A 118,118 0 0,1 128,246 A 59,59 0 0,1 128,128 Z'" \
    # Small white circle in black half
    -fill white \
    -draw "circle 128,75 128,62" \
    # Small black circle in white half
    -fill black \
    -draw "circle 128,181 128,168" \
    # Add subtle border
    -stroke '#333' \
    -strokewidth 2 \
    -draw "circle 128,128 128,10" \
    "$PNG_DIR/taijios.png"

echo "PNG icon created: $PNG_DIR/taijios.png"

# Create SVG icon (scalable)
cat > "$SCALABLE_DIR/taijios.svg" <<'SVG_EOF'
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

echo "SVG icon created: $SCALABLE_DIR/taijios.svg"

# Create symbolic links in AppDir
APPDIR="$SCRIPT_DIR/../build/TaijiOS.AppDir"
if [ -d "$APPDIR" ]; then
    ln -sf "usr/share/icons/hicolor/256x256/apps/taijios.png" "$APPDIR/.DirIcon"
    ln -sf "usr/share/icons/hicolor/256x256/apps/taijios.png" "$APPDIR/taijios.png"
    echo "Symbolic links created in AppDir"
fi

echo ""
echo "Icon creation complete!"
echo "  PNG: $PNG_DIR/taijios.png"
echo "  SVG: $SCALABLE_DIR/taijios.svg"
