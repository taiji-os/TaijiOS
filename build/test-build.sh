#!/bin/bash
set -e
cd "$(git rev-parse --show-toplevel)"
docker build -f Dockerfile.build -t taijios-build-test .
docker run --rm taijios-build-test
echo "Build succeeded!"
