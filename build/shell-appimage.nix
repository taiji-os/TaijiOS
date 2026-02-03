# Nix shell for building TaijiOS AppImage
# Usage: nix-shell build/shell-appimage.nix --run "./build/build-appimage.sh"

{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # AppImage build dependencies
    wget
    imagemagick
    fuse  # FUSE 2 for AppImage building (appimagetool needs libfuse.so.2)
    # Note: appimagetool is downloaded separately, not from nixpkgs

    # Core build tools (inherited from main shell.nix)
    gcc
    gnumake
    binutils
    coreutils
    bash
    perl

    # X11 for graphics support
    xorg.libX11
    xorg.libXext

    # For building emu
    linuxHeaders
  ];

  # Set environment variables for the build
  shellHook = ''
    echo "=== TaijiOS AppImage Build Environment ==="
    echo ""
    echo "Available commands:"
    echo "  ./build/build-appimage.sh      - Build full AppImage"
    echo "  ./build/build-wm-appimage.sh   - Build minimal WM-only AppImage"
    echo "  ./run-appimage.sh             - Run the AppImage"
    echo ""
    echo "Quick start:"
    echo "  nix-shell build/shell-appimage.nix --run './build/build-appimage.sh'"
    echo ""

    # Find the TaijiOS root directory (parent of build/ dir)
    if [ -f mkfile ] && [ -d emu ] && [ -d lib ]; then
      ROOT="$(pwd)"
    elif [ -f ../mkfile ] && [ -d ../emu ] && [ -d ../lib ]; then
      ROOT="$(cd .. && pwd)"
    else
      ROOT="$(pwd)"
    fi
    export ROOT
    export TAIJI_PATH="$ROOT"

    # Set PATH for TaijiOS tools
    export PATH="$ROOT/utils/mk:$ROOT/Linux/amd64/bin:$PATH"

    # Add X11 library paths
    export LD_LIBRARY_PATH="${pkgs.xorg.libX11}/lib:${pkgs.xorg.libXext}/lib:$LD_LIBRARY_PATH"
    export LIBRARY_PATH="${pkgs.xorg.libX11}/lib:${pkgs.xorg.libXext}/lib:$LIBRARY_PATH"

    # Add FUSE library paths for AppImage building
    export LD_LIBRARY_PATH="${pkgs.fuse}/lib:$LD_LIBRARY_PATH"
  '';

  # Hardening disabled for Inferno
  hardeningDisable = [ "all" ];
}
