{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # Core build tools
    gcc
    gnumake
    binutils

    # X11 for graphics support (optional but useful)
    xorg.libX11
    xorg.libXext

    # Utilities
    coreutils
    bash
    perl

    # For building emu (Inferno emulator)
    linuxHeaders

    # Debugging tools
    gdb
    valgrind
  ];

  # Set environment variables for the build
  shellHook = ''
    echo "Welcome to TaijiOS (Inferno OS amd64) build environment"

    # Find the TaijiOS root directory by looking for mkfile
    if [ -f mkfile ] && [ -d emu ] && [ -d lib ]; then
      ROOT="$(pwd)"
    elif [ -f ./TaijiOS/mkfile ] && [ -d ./TaijiOS/emu ]; then
      ROOT="$(pwd)/TaijiOS"
    else
      echo "Warning: Cannot find TaijiOS root directory. Please run from the TaijiOS directory."
      ROOT="$(pwd)"
    fi
    export ROOT
    echo "TaijiOS root: $ROOT"
    echo "Current directory: $(pwd)"
    echo ""
    echo "Quick start:"
    echo "  build9ferno    - Build TaijiOS"
    echo "  run9ferno      - Run emu (Inferno emulator)"
    echo "  emu            - Run emu directly"
    echo ""

    # Set PATH for TaijiOS tools
    # Include utils/mk for the mk build tool, and Linux/amd64/bin for built binaries
    export PATH="$ROOT/utils/mk:$ROOT/Linux/amd64/bin:$PATH"

    # Add X11 library paths to linker search path
    export LD_LIBRARY_PATH="${pkgs.xorg.libX11}/lib:${pkgs.xorg.libXext}/lib:$LD_LIBRARY_PATH"
    export LIBRARY_PATH="${pkgs.xorg.libX11}/lib:${pkgs.xorg.libXext}/lib:$LIBRARY_PATH"

    # Helper function to build
    build9ferno() {
      echo "Building TaijiOS..."
      cd "$ROOT"

      # Set LDFLAGS to help find X11 libraries
      export LDFLAGS="-L${pkgs.xorg.libX11}/lib -L${pkgs.xorg.libXext}/lib $LDFLAGS"

      # Ensure mk can find its configuration
      export SYSHOST=Linux
      export SYSTARG=Linux
      export OBJTYPE=amd64
      export SHELLTYPE=sh

      # Use mk from utils/mk explicitly if needed
      mk install
    }

    # Helper function to run emu
    run9ferno() {
      echo "Starting TaijiOS (Inferno emulator)..."
      echo "Type 'exit' or Ctrl+D to quit"
      echo ""
      cd "$ROOT" || exit 1
      exec "$ROOT/Linux/amd64/bin/emu" -r "$ROOT" "$@"
    }

    # Alias for direct emu access
    emu() {
      run9ferno "$@"
    }
  '';

  # Hardening disabled for Inferno (it has its own build system)
  hardeningDisable = [ "all" ];
}
