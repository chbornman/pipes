#!/bin/bash

# Build script for Raylib-based Pipes implementations (both 2D and 3D)

# Create output directories
mkdir -p src/wasm
mkdir -p lib

# Download Raylib WebAssembly library if not present
if [ ! -f "lib/libraylib_web.a" ]; then
    echo "Downloading Raylib WebAssembly library..."
    mkdir -p lib
    cd lib
    wget https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_webassembly.zip
    unzip raylib-5.0_webassembly.zip
    cp raylib-5.0_webassembly/lib/libraylib.a libraylib_web.a
    cd ..
fi

# Common Emscripten flags
COMMON_FLAGS="-O3 -s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 -DPLATFORM_WEB -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME='createModule'"

# Build 2D Raylib version
echo "Building 2D Raylib version..."
emcc src/pipes_2d_raylib.c \
    -o src/wasm/pipes_2d_raylib.js \
    -I lib/raylib-5.0_webassembly/include \
    lib/libraylib_web.a \
    $COMMON_FLAGS \
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
    -s EXPORTED_FUNCTIONS="['_malloc','_free','_pipes2d_init','_pipes2d_frame','_pipes2d_setSpeed','_pipes2d_setThickness','_pipes2d_setPipeCount','_pipes2d_resize','_pipes2d_cleanup']"

# Build 3D Raylib version
echo "Building 3D Raylib version..."
emcc src/pipes_3d.c \
    -o src/wasm/pipes_3d_raylib.js \
    -I lib/raylib-5.0_webassembly/include \
    lib/libraylib_web.a \
    $COMMON_FLAGS \
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
    -s EXPORTED_FUNCTIONS="['_malloc','_free','_pipes3d_init','_pipes3d_frame','_pipes3d_setFadeSpeed','_pipes3d_setSpawnRate','_pipes3d_setTurnProbability','_pipes3d_setMaxPipes','_pipes3d_setCameraSpeed','_pipes3d_setPipeSpeed','_pipes3d_setSegmentDelay','_pipes3d_mouseDown','_pipes3d_mouseUp','_pipes3d_mouseMove','_pipes3d_resize','_pipes3d_cleanup']"

echo "Build complete!"
echo "2D Raylib module: src/wasm/pipes_2d_raylib.js"
echo "3D Raylib module: src/wasm/pipes_3d_raylib.js"