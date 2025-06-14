#!/bin/bash

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "emcc not found. Please ensure Emscripten is installed and activated."
    echo "If installed via Homebrew, you may need to run: source $(brew --prefix)/opt/emscripten/libexec/emscripten.sh"
    exit 1
fi

# Create wasm output directory
mkdir -p src/wasm

# Compile C to WASM
emcc src/pipes.c \
  -o src/wasm/pipes.js \
  -s EXPORTED_FUNCTIONS='["_init_pipes", "_update_pipes", "_get_framebuffer", "_cleanup_pipes", "_malloc", "_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAP8", "HEAPU8", "HEAP16", "HEAPU16", "HEAP32", "HEAPU32", "HEAPF32", "HEAPF64"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='createPipesModule' \
  -s EXPORT_ES6=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s WASM=1 \
  -O2

echo "WASM build complete!"