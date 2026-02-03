#!/bin/sh
#
# run-kryon.sh - Kryon Application Launcher
#
# Usage: ./run-kryon.sh app.kry
#        ./run-kryon.sh app.dis
#
# Compiles .kry files if needed and runs the application

set -e

# Script directory
SCRIPT_DIR=$(dirname "$0")
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)

# Kryon compiler
KRYONC="$ROOT_DIR/Linux/amd64/bin/kryonc"

# Limbo compiler
LIMBO="$ROOT_DIR/Linux/amd64/bin/limbo"

# Display usage
usage() {
    echo "Usage: $0 <app.kry|app.dis>" >&2
    echo "" >&2
    echo "Compiles .kry files if needed and runs the application." >&2
    exit 1
}

# Check arguments
if [ $# -lt 1 ]; then
    usage
fi

INPUT="$1"

# Check if input exists
if [ ! -f "$INPUT" ]; then
    echo "Error: File not found: $INPUT" >&2
    exit 1
fi

# Get file extension
EXT="${INPUT##*.}"
BASE="${INPUT%.*}"

# Compile .kry to .dis if needed
if [ "$EXT" = "kry" ]; then
    echo "Compiling $INPUT..."

    # Check if kryonc exists
    if [ ! -f "$KRYONC" ]; then
        echo "Error: Kryon compiler not found at $KRYONC" >&2
        echo "Please build Kryon first: cd kryon && mk" >&2
        exit 1
    fi

    # Generate .b file
    $KRYONC "$INPUT"

    # Compile to .dis
    if [ -f "$LIMBO" ]; then
        $LIMBO -o "$BASE.dis" "$BASE.b"
        DIS_FILE="$BASE.dis"
    else
        echo "Warning: Limbo compiler not found, trying to run .b file directly" >&2
        DIS_FILE="$BASE.b"
    fi

    # Clean up .b file
    rm -f "$BASE.b"

elif [ "$EXT" = "dis" ]; then
    DIS_FILE="$INPUT"
else
    echo "Error: Unsupported file type: .$EXT" >&2
    echo "Expected .kry or .dis" >&2
    exit 1
fi

# Run the application
echo "Running $DIS_FILE..."
exec "$ROOT/dis/sh" "$DIS_FILE"
