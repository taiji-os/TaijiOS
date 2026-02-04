#!/bin/sh
# Force clean rebuild of all .dis files in appl/cmd

echo "Cleaning all .dis files in appl/cmd..."
find . -name '*.dis' -delete
find . -name '*.sbl' -delete
rm -f .last-build .all-modules

echo "Clean complete. Run build-all.sh to rebuild everything."
