#!/bin/sh
# Build all .b files in this directory
# This version tracks ALL module dependencies dynamically

LIMBO="$ROOT/Linux/amd64/bin/limbo"
if [ ! -f "$LIMBO" ]; then
    LIMBO="limbo"
fi

# Find ALL module files dynamically
find "$ROOT/module" -name "*.m" -type f > .all-modules 2>/dev/null

# Get the newest module file timestamp
NEWEST_MODULE=0
while read -r module; do
    if [ -f "$module" ]; then
        mod_time=$(stat -c "%Y" "$module" 2>/dev/null || stat -f "%m" "$module" 2>/dev/null || echo 0)
        if [ "$mod_time" -gt "$NEWEST_MODULE" ]; then
            NEWEST_MODULE=$mod_time
        fi
    fi
done < .all-modules

# Check if we need to rebuild everything
FORCE_REBUILD=0
if [ -f ".last-build" ]; then
    build_time=$(stat -c "%Y" ".last-build" 2>/dev/null || stat -f "%m" ".last-build" 2>/dev/null || echo 0)
    if [ "$NEWEST_MODULE" -gt "$build_time" ]; then
        echo "Modules changed since last build - forcing rebuild"
        FORCE_REBUILD=1
    fi
else
    echo "No build timestamp found - forcing rebuild"
    FORCE_REBUILD=1
fi

# Function to check if rebuild needed
needs_rebuild() {
    dis_file="$1"
    b_file="$2"

    # Force rebuild if modules changed
    if [ "$FORCE_REBUILD" -eq 1 ]; then
        return 0
    fi

    # Rebuild if .dis doesn't exist
    if [ ! -f "$dis_file" ]; then
        return 0
    fi

    # Rebuild if .b file is newer than .dis
    if [ "$b_file" -nt "$dis_file" ]; then
        return 0
    fi

    return 1
}

for b in *.b; do
    if [ -f "$b" ]; then
        dis="${b%.b}.dis"
        if needs_rebuild "$dis" "$b"; then
            echo "Building $dis..."
            "$LIMBO" -I"$ROOT/module" -gw "$b" || echo "Warning: failed to build $dis"
        fi
    fi
done

# Update build timestamp
date +%s > .last-build
