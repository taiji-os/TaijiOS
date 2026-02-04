#!/bin/sh
# Build all .b files in this directory and subdirectories
# This version tracks ALL module dependencies dynamically

# Export ROOT for subshells
export ROOT

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

# Build subdirectories first (dependencies)
for dir in asm auth auxi dbm disk fs games install ip lego limbo mash mk mpc ndb sh spki usb zip; do
    if [ -d "$dir" ]; then
        (cd "$dir" && for b in *.b; do
            if [ -f "$b" ]; then
                dis="${b%.b}.dis"
                # Check if file already exists and is up to date
                rebuild=0
                if [ "$FORCE_REBUILD" -eq 1 ]; then
                    rebuild=1
                elif [ ! -f "$dis" ]; then
                    rebuild=1
                elif [ "$b" -nt "$dis" ]; then
                    rebuild=1
                fi
                if [ "$rebuild" -eq 1 ]; then
                    echo "Building $dir/$dis..."
                    "$LIMBO" -I"$ROOT/module" -gw "$b" || echo "Warning: failed to build $dir/$dis"
                fi
            fi
        done)
    fi
done

# Build current directory
for b in *.b; do
    if [ -f "$b" ]; then
        # Skip library/include files (no implement statement)
        if ! grep -q "implement " "$b"; then
            continue
        fi
        dis="${b%.b}.dis"
        rebuild=0
        if [ "$FORCE_REBUILD" -eq 1 ]; then
            rebuild=1
        elif [ ! -f "$dis" ]; then
            rebuild=1
        elif [ "$b" -nt "$dis" ]; then
            rebuild=1
        fi
        if [ "$rebuild" -eq 1 ]; then
            echo "Building $dis..."
            "$LIMBO" -I"$ROOT/module" -gw "$b" || echo "Warning: failed to build $dis"
        fi
    fi
done

# Update build timestamp
date +%s > .last-build
