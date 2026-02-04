#!/bin/sh
# Build and install all .dis files to $DISBIN

LIMBO="$ROOT/Linux/amd64/bin/limbo"
if [ ! -f "$LIMBO" ]; then
    LIMBO="limbo"
fi

# Set DISBIN if not already set (matches mkfile)
DISBIN="${DISBIN:-$ROOT/dis/wm}"

# Build any .b files that are newer than their .dis
for b in *.b; do
    if [ -f "$b" ]; then
        dis="${b%.b}.dis"
        if [ ! -f "$dis" ] || [ "$b" -nt "$dis" ]; then
            echo "Building $dis..."
            "$LIMBO" -I"$ROOT/module" -gw "$b" || echo "Warning: failed to build $dis"
        fi
    fi
done

# Copy all .dis files to $DISBIN
for dis in *.dis; do
    if [ -f "$dis" ]; then
        cp "$dis" "$DISBIN/"
    fi
done
