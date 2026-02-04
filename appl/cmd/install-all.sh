#!/bin/sh
# Install all .dis files to $DISBIN

# Ensure ROOT and DISBIN are set
if [ -z "$ROOT" ]; then
    ROOT=$(pwd)/../..
    while [ ! -f "$ROOT/mkconfig" ] && [ "$ROOT" != "/" ]; do
        ROOT=$(dirname "$ROOT")
    done
fi

DISBIN="$ROOT/dis"

find . -name '*.dis' -type f | while read -r dis; do
    # Get the directory path relative to current dir
    dir=$(dirname "$dis")
    # Get just the filename
    file=$(basename "$dis")
    # Create target directory if needed
    mkdir -p "$DISBIN/$dir"
    cp "$dis" "$DISBIN/$dir/$file"
done
