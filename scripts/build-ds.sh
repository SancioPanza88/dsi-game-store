#!/bin/bash
# Build script for DSi Game Store
# Requires devkitPro environment

set -e

echo "=== DSi Game Store Build Script ==="

# Check environment
if [ -z "$DEVKITPRO" ]; then
    echo "ERROR: DEVKITPRO not set"
    echo "Please install devkitPro and set DEVKITPRO environment variable"
    echo "Download from: https://devkitpro.org/wiki/Getting_Started"
    exit 1
fi

if [ -z "$DEVKITARM" ]; then
    export DEVKITARM="$DEVKITPRO/devkitARM"
fi

echo "DEVKITPRO: $DEVKITPRO"
echo "DEVKITARM: $DEVKITARM"

# Check tools
if [ ! -f "$DEVKITARM/bin/arm-none-eabi-gcc" ]; then
    echo "ERROR: arm-none-eabi-gcc not found"
    echo "Please install devkitARM"
    exit 1
fi

# Build
cd ds-app
echo "Building DSi application..."
make clean
make

if [ -f "dsi-game-store.nds" ]; then
    echo ""
    echo "=== BUILD SUCCESSFUL ==="
    echo "Output: ds-app/dsi-game-store.nds"
    ls -lh dsi-game-store.nds
else
    echo "ERROR: Build failed - .nds file not created"
    exit 1
fi
