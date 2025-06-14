#!/bin/bash

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "emcc not found. Please ensure Emscripten is installed and activated."
    echo "If installed via Homebrew, you may need to run: source $(brew --prefix)/opt/emscripten/libexec/emscripten.sh"
    exit 1
fi

# Create wasm output directory
mkdir -p src/wasm

echo "Building 2D pipes..."
# Compile 2D pipes
emcc src/pipes.c \
  -o src/wasm/pipes.js \
  -s EXPORTED_FUNCTIONS='["_init_pipes", "_update_pipes", "_get_framebuffer", "_cleanup_pipes", "_malloc", "_free", "_set_fade_speed", "_set_spawn_rate", "_set_turn_probability", "_set_max_pipes", "_set_animation_speed"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAP8", "HEAPU8", "HEAP16", "HEAPU16", "HEAP32", "HEAPU32", "HEAPF32", "HEAPF64"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='createPipesModule' \
  -s EXPORT_ES6=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s WASM=1 \
  -O2

echo "2D build complete!"

# Download Raylib web library if not exists
if [ ! -f "lib/libraylib_web.a" ]; then
    echo "Downloading precompiled Raylib for web..."
    mkdir -p lib
    cd lib
    wget https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_webassembly.zip
    unzip raylib-5.0_webassembly.zip
    cp raylib-5.0_webassembly/lib/libraylib.a libraylib_web.a
    cd ..
fi

echo "Building 3D pipes..."
# Compile 3D pipes with Raylib
emcc src/pipes_3d.c \
  -o src/wasm/pipes_3d.js \
  -I lib/raylib-5.0_webassembly/include \
  lib/libraylib_web.a \
  -s EXPORTED_FUNCTIONS='["_init_3d_pipes", "_update_3d_pipes", "_get_3d_framebuffer", "_cleanup_3d_pipes", "_set_3d_fade_speed", "_set_3d_spawn_rate", "_set_3d_turn_probability", "_set_3d_max_pipes", "_handle_mouse_down", "_handle_mouse_up", "_handle_mouse_move", "_malloc", "_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAP8", "HEAPU8", "HEAP16", "HEAPU16", "HEAP32", "HEAPU32", "HEAPF32", "HEAPF64"]' \
  -s USE_GLFW=3 \
  -s ASYNCIFY \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='createPipes3DModule' \
  -s EXPORT_ES6=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s WASM=1 \
  -s TOTAL_MEMORY=67108864 \
  -O2

echo "3D build complete!"
echo "All WASM builds complete!"