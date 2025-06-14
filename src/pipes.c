#include <emscripten.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define MAX_PIPES 10
#define GRID_SIZE 30
#define PIPE_RADIUS 12
#define SEGMENT_LENGTH (GRID_SIZE)
#define MAX_PIPE_LENGTH 50

// Tunable parameters
static int fade_speed = 1;
static int spawn_rate = 10;
static int turn_probability = 30;
static int max_active_pipes = 3;
static int animation_speed = 60; // FPS

typedef struct {
    int x, y, z;
} Point3D;

typedef enum {
    DIR_RIGHT = 0,
    DIR_LEFT = 1,
    DIR_UP = 2,
    DIR_DOWN = 3,
    DIR_FORWARD = 4,
    DIR_BACKWARD = 5
} Direction;

typedef enum {
    PIPE_STRAIGHT,
    PIPE_ELBOW,
    PIPE_JOINT
} PipeType;

typedef struct {
    Point3D pos;
    Direction dir;
    int color;
    int active;
    int length;
} Pipe;

typedef struct {
    int width;
    int height;
    unsigned char* framebuffer;
    int*** grid; // 3D grid to track occupied spaces
    Pipe pipes[MAX_PIPES];
    int active_pipes;
} PipeSystem;

static PipeSystem* pipe_system = NULL;

// Color palette for pipes
static const unsigned int pipe_colors[] = {
    0xFF4444FF, // Red
    0xFF44FF44, // Green
    0xFFFF4444, // Blue
    0xFFFFFF44, // Yellow
    0xFFFF44FF, // Magenta
    0xFF44FFFF, // Cyan
    0xFFFF8844, // Orange
    0xFF8844FF  // Purple
};

EMSCRIPTEN_KEEPALIVE
void init_pipes(int width, int height) {
    if (pipe_system) {
        // Free existing grid
        if (pipe_system->grid) {
            for (int x = 0; x < width / GRID_SIZE; x++) {
                for (int y = 0; y < height / GRID_SIZE; y++) {
                    free(pipe_system->grid[x][y]);
                }
                free(pipe_system->grid[x]);
            }
            free(pipe_system->grid);
        }
        if (pipe_system->framebuffer) {
            free(pipe_system->framebuffer);
        }
        free(pipe_system);
    }
    
    pipe_system = (PipeSystem*)malloc(sizeof(PipeSystem));
    pipe_system->width = width;
    pipe_system->height = height;
    pipe_system->active_pipes = 0;
    pipe_system->framebuffer = (unsigned char*)malloc(width * height * 4);
    
    // Initialize 3D grid
    int grid_width = width / GRID_SIZE + 1;
    int grid_height = height / GRID_SIZE + 1;
    int grid_depth = 30; // Z depth
    
    pipe_system->grid = (int***)malloc(grid_width * sizeof(int**));
    for (int x = 0; x < grid_width; x++) {
        pipe_system->grid[x] = (int**)malloc(grid_height * sizeof(int*));
        for (int y = 0; y < grid_height; y++) {
            pipe_system->grid[x][y] = (int*)calloc(grid_depth, sizeof(int));
        }
    }
    
    // Clear framebuffer to black
    memset(pipe_system->framebuffer, 0, width * height * 4);
    
    // Set alpha channel
    for (int i = 3; i < width * height * 4; i += 4) {
        pipe_system->framebuffer[i] = 255;
    }
    
    // Initialize pipes
    for (int i = 0; i < MAX_PIPES; i++) {
        pipe_system->pipes[i].active = 0;
    }
    
    srand(time(NULL));
}

EMSCRIPTEN_KEEPALIVE
unsigned char* get_framebuffer() {
    return pipe_system ? pipe_system->framebuffer : NULL;
}

// Parameter setters
EMSCRIPTEN_KEEPALIVE
void set_fade_speed(int speed) {
    fade_speed = speed;
}

EMSCRIPTEN_KEEPALIVE
void set_spawn_rate(int rate) {
    spawn_rate = rate;
}

EMSCRIPTEN_KEEPALIVE
void set_turn_probability(int prob) {
    turn_probability = prob;
}

EMSCRIPTEN_KEEPALIVE
void set_max_pipes(int max) {
    max_active_pipes = max;
}

EMSCRIPTEN_KEEPALIVE
void set_animation_speed(int fps) {
    animation_speed = fps;
}

static void draw_circle_3d(int cx, int cy, int radius, int z, unsigned int color, float intensity) {
    if (!pipe_system) return;
    
    // Extract RGB components
    unsigned char r = (color >> 16) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = color & 0xFF;
    
    // Apply lighting based on z-depth
    float depth_factor = 1.0f - (z / 30.0f) * 0.3f;
    r = (unsigned char)(r * intensity * depth_factor);
    g = (unsigned char)(g * intensity * depth_factor);
    b = (unsigned char)(b * intensity * depth_factor);
    
    // Draw filled circle with 3D shading
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            float dist = sqrtf(x * x + y * y);
            if (dist <= radius) {
                int px = cx + x;
                int py = cy + y;
                
                if (px >= 0 && px < pipe_system->width && py >= 0 && py < pipe_system->height) {
                    // Calculate 3D shading
                    float norm_dist = dist / radius;
                    float shade = 1.0f - norm_dist * 0.6f;
                    
                    // Add highlight for 3D effect
                    float highlight = 0.0f;
                    if (norm_dist < 0.5f) {
                        highlight = (0.5f - norm_dist) * 0.4f;
                    }
                    
                    int idx = (py * pipe_system->width + px) * 4;
                    pipe_system->framebuffer[idx] = fmin(255, r * shade + 255 * highlight);
                    pipe_system->framebuffer[idx + 1] = fmin(255, g * shade + 255 * highlight);
                    pipe_system->framebuffer[idx + 2] = fmin(255, b * shade + 255 * highlight);
                }
            }
        }
    }
}

static void draw_cylinder_segment(Point3D start, Point3D end, int radius, unsigned int color) {
    if (!pipe_system) return;
    
    // Calculate 2D projection
    int x1 = start.x;
    int y1 = start.y - start.z / 2; // Simple 3D projection
    int x2 = end.x;
    int y2 = end.y - end.z / 2;
    
    // Draw cylinder as a series of circles
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = end.z - start.z;
    float length = sqrtf(dx * dx + dy * dy);
    
    if (length < 1) return;
    
    int steps = (int)length / 2;
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        int x = x1 + (int)(dx * t);
        int y = y1 + (int)(dy * t);
        int z = start.z + (int)(dz * t);
        
        // Draw circle at this position
        draw_circle_3d(x, y, radius, z, color, 1.0f);
    }
}

static void draw_elbow(Point3D pos, Direction from_dir, Direction to_dir, int radius, unsigned int color) {
    if (!pipe_system) return;
    
    // Draw a joint/elbow at the turn
    int x = pos.x;
    int y = pos.y - pos.z / 2;
    
    // Draw a larger sphere for the joint
    draw_circle_3d(x, y, radius + 2, pos.z, color, 1.2f);
}

static int is_valid_position(int gx, int gy, int gz) {
    int grid_width = pipe_system->width / GRID_SIZE + 1;
    int grid_height = pipe_system->height / GRID_SIZE + 1;
    
    if (gx < 0 || gx >= grid_width || gy < 0 || gy >= grid_height || gz < 0 || gz >= 30) {
        return 0;
    }
    
    return pipe_system->grid[gx][gy][gz] == 0;
}

static Direction get_new_direction(Point3D pos, Direction current_dir) {
    Direction possible_dirs[6];
    int count = 0;
    
    // Check all 6 directions except the opposite of current
    for (int d = 0; d < 6; d++) {
        if ((d == DIR_RIGHT && current_dir == DIR_LEFT) ||
            (d == DIR_LEFT && current_dir == DIR_RIGHT) ||
            (d == DIR_UP && current_dir == DIR_DOWN) ||
            (d == DIR_DOWN && current_dir == DIR_UP) ||
            (d == DIR_FORWARD && current_dir == DIR_BACKWARD) ||
            (d == DIR_BACKWARD && current_dir == DIR_FORWARD)) {
            continue;
        }
        
        int gx = pos.x / GRID_SIZE;
        int gy = pos.y / GRID_SIZE;
        int gz = pos.z;
        
        switch (d) {
            case DIR_RIGHT: gx++; break;
            case DIR_LEFT: gx--; break;
            case DIR_UP: gy--; break;
            case DIR_DOWN: gy++; break;
            case DIR_FORWARD: gz++; break;
            case DIR_BACKWARD: gz--; break;
        }
        
        if (is_valid_position(gx, gy, gz)) {
            possible_dirs[count++] = d;
        }
    }
    
    if (count == 0) return -1;
    return possible_dirs[rand() % count];
}

static void spawn_pipe() {
    if (pipe_system->active_pipes >= MAX_PIPES) return;
    
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!pipe_system->pipes[i].active) {
            // Random starting position on grid
            int gx = rand() % (pipe_system->width / GRID_SIZE);
            int gy = rand() % (pipe_system->height / GRID_SIZE);
            int gz = 5 + rand() % 20;
            
            if (!is_valid_position(gx, gy, gz)) continue;
            
            pipe_system->pipes[i].pos.x = gx * GRID_SIZE + GRID_SIZE / 2;
            pipe_system->pipes[i].pos.y = gy * GRID_SIZE + GRID_SIZE / 2;
            pipe_system->pipes[i].pos.z = gz;
            pipe_system->pipes[i].dir = rand() % 6;
            pipe_system->pipes[i].color = rand() % 8;
            pipe_system->pipes[i].active = 1;
            pipe_system->pipes[i].length = 0;
            
            // Mark grid position as occupied
            pipe_system->grid[gx][gy][gz] = 1;
            pipe_system->active_pipes++;
            break;
        }
    }
}

static void update_pipe(Pipe* pipe) {
    if (!pipe->active) return;
    
    Point3D old_pos = pipe->pos;
    Point3D new_pos = pipe->pos;
    
    // Move in current direction
    switch (pipe->dir) {
        case DIR_RIGHT: new_pos.x += SEGMENT_LENGTH; break;
        case DIR_LEFT: new_pos.x -= SEGMENT_LENGTH; break;
        case DIR_UP: new_pos.y -= SEGMENT_LENGTH; break;
        case DIR_DOWN: new_pos.y += SEGMENT_LENGTH; break;
        case DIR_FORWARD: new_pos.z += 1; break;
        case DIR_BACKWARD: new_pos.z -= 1; break;
    }
    
    // Check bounds
    if (new_pos.x < PIPE_RADIUS || new_pos.x >= pipe_system->width - PIPE_RADIUS ||
        new_pos.y < PIPE_RADIUS || new_pos.y >= pipe_system->height - PIPE_RADIUS ||
        new_pos.z < 0 || new_pos.z >= 30) {
        pipe->active = 0;
        pipe_system->active_pipes--;
        return;
    }
    
    // Draw pipe segment
    unsigned int color = pipe_colors[pipe->color % 8];
    draw_cylinder_segment(old_pos, new_pos, PIPE_RADIUS, color);
    
    // Update position
    pipe->pos = new_pos;
    pipe->length++;
    
    // Mark new grid position
    int gx = new_pos.x / GRID_SIZE;
    int gy = new_pos.y / GRID_SIZE;
    int gz = new_pos.z;
    
    if (gx >= 0 && gx < pipe_system->width / GRID_SIZE + 1 &&
        gy >= 0 && gy < pipe_system->height / GRID_SIZE + 1 &&
        gz >= 0 && gz < 30) {
        pipe_system->grid[gx][gy][gz] = 1;
    }
    
    // Randomly change direction
    if (rand() % 100 < turn_probability || pipe->length % 5 == 0) {
        Direction new_dir = get_new_direction(pipe->pos, pipe->dir);
        if (new_dir != -1 && new_dir != pipe->dir) {
            draw_elbow(pipe->pos, pipe->dir, new_dir, PIPE_RADIUS, color);
            pipe->dir = new_dir;
        }
    }
    
    // Deactivate after max length
    if (pipe->length > MAX_PIPE_LENGTH) {
        pipe->active = 0;
        pipe_system->active_pipes--;
    }
}

EMSCRIPTEN_KEEPALIVE
void update_pipes() {
    if (!pipe_system) return;
    
    // Fade effect
    for (int i = 0; i < pipe_system->width * pipe_system->height * 4; i += 4) {
        for (int c = 0; c < 3; c++) {
            if (pipe_system->framebuffer[i + c] > fade_speed) {
                pipe_system->framebuffer[i + c] -= fade_speed;
            } else {
                pipe_system->framebuffer[i + c] = 0;
            }
        }
    }
    
    // Update existing pipes
    for (int i = 0; i < MAX_PIPES; i++) {
        update_pipe(&pipe_system->pipes[i]);
    }
    
    // Spawn new pipes
    if (pipe_system->active_pipes < max_active_pipes && rand() % 100 < spawn_rate) {
        spawn_pipe();
    }
}

EMSCRIPTEN_KEEPALIVE
void cleanup_pipes() {
    if (pipe_system) {
        if (pipe_system->grid) {
            int grid_width = pipe_system->width / GRID_SIZE + 1;
            int grid_height = pipe_system->height / GRID_SIZE + 1;
            for (int x = 0; x < grid_width; x++) {
                for (int y = 0; y < grid_height; y++) {
                    free(pipe_system->grid[x][y]);
                }
                free(pipe_system->grid[x]);
            }
            free(pipe_system->grid);
        }
        if (pipe_system->framebuffer) {
            free(pipe_system->framebuffer);
        }
        free(pipe_system);
        pipe_system = NULL;
    }
}