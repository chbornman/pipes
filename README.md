# Windows Pipes Screensaver Clone

A web-based recreation of the classic Windows 3D Pipes screensaver using Svelte and WebAssembly.

## Features

- Classic 3D pipes animation
- WebAssembly-powered rendering for performance
- Full-screen canvas display
- Modern Svelte framework

## Technologies Used

- **Svelte** - Modern reactive UI framework
- **WebAssembly** - High-performance C code compiled to WASM
- **Emscripten** - C to WebAssembly compiler
- **Vite** - Fast build tool and dev server

## Prerequisites

- Node.js (v16 or higher)
- Emscripten SDK (install via Homebrew: `brew install emscripten`)

## Installation

1. Clone the repository:
```bash
git clone [repository-url]
cd pipes
```

2. Install dependencies:
```bash
npm install
```

3. Build the WebAssembly module:
```bash
npm run build:wasm
```

4. Start the development server:
```bash
npm run dev
```

## Building for Production

```bash
npm run build:wasm
npm run build
```

## Project Structure

```
pipes/
├── src/
│   ├── App.svelte          # Main app component with navigation
│   ├── lib/
│   │   └── Screensaver.svelte  # Canvas and WASM integration
│   ├── pipes.c             # C code for pipe animation logic
│   └── wasm/               # Generated WASM files
├── build-wasm.sh           # WASM build script
└── package.json
```

## How It Works

The screensaver uses WebAssembly to render the pipes animation:
1. C code manages the pipe generation and framebuffer
2. Emscripten compiles the C code to WebAssembly
3. Svelte component loads the WASM module and displays the framebuffer on a canvas
4. Animation loop updates the pipes and renders at 60fps

## License

MIT