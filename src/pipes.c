#include <emscripten.h>
#include <stdlib.h>

static unsigned char* framebuffer = NULL;
static int width = 0;
static int height = 0;

EMSCRIPTEN_KEEPALIVE
void init_pipes(int w, int h) {
    width = w;
    height = h;
    
    if (framebuffer) {
        free(framebuffer);
    }
    
    framebuffer = (unsigned char*)malloc(width * height * 4);
    
    // Fill with red color to test
    for (int i = 0; i < width * height * 4; i += 4) {
        framebuffer[i] = 255;     // R
        framebuffer[i + 1] = 0;   // G
        framebuffer[i + 2] = 0;   // B
        framebuffer[i + 3] = 255; // A
    }
}

EMSCRIPTEN_KEEPALIVE
unsigned char* get_framebuffer() {
    return framebuffer;
}

EMSCRIPTEN_KEEPALIVE
void update_pipes() {
    if (!framebuffer) return;
    
    // Simple animation: cycle through colors
    static int frame = 0;
    frame++;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            framebuffer[idx] = (x + frame) % 256;     // R
            framebuffer[idx + 1] = (y + frame) % 256; // G
            framebuffer[idx + 2] = frame % 256;       // B
            framebuffer[idx + 3] = 255;               // A
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void cleanup_pipes() {
    if (framebuffer) {
        free(framebuffer);
        framebuffer = NULL;
    }
}