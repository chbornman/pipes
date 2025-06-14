#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <emscripten/emscripten.h>
#include <raylib.h>
#include <raymath.h>

#define MAX_PIPES 10
#define GRID_SIZE 4.0f
#define PIPE_RADIUS 0.4f
#define SEGMENT_LENGTH 2.0f
#define MAX_PIPE_LENGTH 30
#define GRID_DIMENSION 20

// Tunable parameters
static int fade_speed = 1;
static int spawn_rate = 15;
static int turn_probability = 25;
static int max_active_pipes = 4;
static float camera_rotation_speed = 0.002f;
static float pipe_growth_speed = 0.05f;
static int segment_update_delay = 10;

typedef struct {
    Vector3 pos;
    Vector3 direction;
    Color color;
    int active;
    int length;
    Vector3 segments[MAX_PIPE_LENGTH];
    int segment_count;
    int update_counter;
    float growth_progress;
} Pipe3D;

typedef struct {
    Pipe3D pipes[MAX_PIPES];
    int active_pipes;
    bool grid[GRID_DIMENSION][GRID_DIMENSION][GRID_DIMENSION];
    Camera3D camera;
    Vector2 lastMousePos;
    bool mouseDown;
    RenderTexture2D target;
    float rotation;
} PipeSystem3D;

static PipeSystem3D* system3d = NULL;

// Available pipe colors
static Color pipe_colors[] = {
    { 255, 67, 67, 255 },   // Red
    { 67, 255, 67, 255 },   // Green
    { 67, 67, 255, 255 },   // Blue
    { 255, 255, 67, 255 },  // Yellow
    { 255, 67, 255, 255 },  // Magenta
    { 67, 255, 255, 255 },  // Cyan
    { 255, 165, 67, 255 },  // Orange
    { 165, 67, 255, 255 }   // Purple
};

// Direction vectors
static Vector3 directions[] = {
    { 1, 0, 0 },   // Right
    { -1, 0, 0 },  // Left
    { 0, 1, 0 },   // Up
    { 0, -1, 0 },  // Down
    { 0, 0, 1 },   // Forward
    { 0, 0, -1 }   // Back
};

EMSCRIPTEN_KEEPALIVE
void pipes3d_init(int canvasWidth, int canvasHeight) {
    if (!system3d) {
        system3d = (PipeSystem3D*)calloc(1, sizeof(PipeSystem3D));
    }
    
    // Initialize Raylib with proper flags
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(canvasWidth, canvasHeight, "Pipes 3D");
    SetTargetFPS(60);
    
    // Create render texture for offscreen rendering
    system3d->target = LoadRenderTexture(canvasWidth, canvasHeight);
    
    // Setup camera
    system3d->camera.position = (Vector3){ 30.0f, 30.0f, 30.0f };
    system3d->camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    system3d->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    system3d->camera.fovy = 45.0f;
    system3d->camera.projection = CAMERA_PERSPECTIVE;
    
    // Clear grid
    for (int x = 0; x < GRID_DIMENSION; x++) {
        for (int y = 0; y < GRID_DIMENSION; y++) {
            for (int z = 0; z < GRID_DIMENSION; z++) {
                system3d->grid[x][y][z] = false;
            }
        }
    }
    
    // Initialize pipes
    for (int i = 0; i < MAX_PIPES; i++) {
        system3d->pipes[i].active = 0;
        system3d->pipes[i].segment_count = 0;
    }
    
    system3d->active_pipes = 0;
    system3d->rotation = 0;
}

EMSCRIPTEN_KEEPALIVE
void set_3d_fade_speed(int speed) { fade_speed = speed; }

EMSCRIPTEN_KEEPALIVE
void set_3d_spawn_rate(int rate) { spawn_rate = rate; }

EMSCRIPTEN_KEEPALIVE
void set_3d_turn_probability(int prob) { turn_probability = prob; }

EMSCRIPTEN_KEEPALIVE
void set_3d_max_pipes(int max) { max_active_pipes = max; }

EMSCRIPTEN_KEEPALIVE
void handle_mouse_down(int x, int y) {
    if (system3d) {
        system3d->mouseDown = true;
        system3d->lastMousePos = (Vector2){ x, y };
    }
}

EMSCRIPTEN_KEEPALIVE
void handle_mouse_up() {
    if (system3d) {
        system3d->mouseDown = false;
    }
}

EMSCRIPTEN_KEEPALIVE
void handle_mouse_move(int x, int y) {
    if (system3d && system3d->mouseDown) {
        Vector2 currentPos = { x, y };
        Vector2 delta = Vector2Subtract(currentPos, system3d->lastMousePos);
        
        // Rotate camera around origin
        float angleX = delta.x * 0.01f;
        float angleY = delta.y * 0.01f;
        
        // Horizontal rotation
        Vector3 cameraPos = system3d->camera.position;
        float cosAngle = cosf(angleX);
        float sinAngle = sinf(angleX);
        float x_new = cameraPos.x * cosAngle - cameraPos.z * sinAngle;
        float z_new = cameraPos.x * sinAngle + cameraPos.z * cosAngle;
        system3d->camera.position.x = x_new;
        system3d->camera.position.z = z_new;
        
        // Vertical rotation (limited)
        system3d->camera.position.y += angleY * 10.0f;
        system3d->camera.position.y = Clamp(system3d->camera.position.y, -50.0f, 50.0f);
        
        system3d->lastMousePos = currentPos;
    }
}

static bool is_grid_free(int x, int y, int z) {
    if (x < 0 || x >= GRID_DIMENSION || 
        y < 0 || y >= GRID_DIMENSION || 
        z < 0 || z >= GRID_DIMENSION) {
        return false;
    }
    return !system3d->grid[x][y][z];
}

static Vector3 get_random_direction(Vector3 current_dir, Vector3 pos) {
    Vector3 possible_dirs[6];
    int count = 0;
    
    for (int i = 0; i < 6; i++) {
        // Don't go backwards
        if (Vector3DotProduct(directions[i], Vector3Scale(current_dir, -1)) > 0.9f) {
            continue;
        }
        
        // Check if position is free
        Vector3 newPos = Vector3Add(pos, Vector3Scale(directions[i], SEGMENT_LENGTH));
        int gx = (int)((newPos.x + GRID_DIMENSION * GRID_SIZE / 2) / GRID_SIZE);
        int gy = (int)((newPos.y + GRID_DIMENSION * GRID_SIZE / 2) / GRID_SIZE);
        int gz = (int)((newPos.z + GRID_DIMENSION * GRID_SIZE / 2) / GRID_SIZE);
        
        if (is_grid_free(gx, gy, gz)) {
            possible_dirs[count++] = directions[i];
        }
    }
    
    if (count == 0) return current_dir;
    return possible_dirs[rand() % count];
}

static void spawn_pipe() {
    if (system3d->active_pipes >= max_active_pipes) return;
    
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!system3d->pipes[i].active) {
            // Random starting position
            int gx = GRID_DIMENSION/4 + rand() % (GRID_DIMENSION/2);
            int gy = GRID_DIMENSION/4 + rand() % (GRID_DIMENSION/2);
            int gz = GRID_DIMENSION/4 + rand() % (GRID_DIMENSION/2);
            
            if (!is_grid_free(gx, gy, gz)) continue;
            
            system3d->pipes[i].pos.x = (gx - GRID_DIMENSION/2) * GRID_SIZE;
            system3d->pipes[i].pos.y = (gy - GRID_DIMENSION/2) * GRID_SIZE;
            system3d->pipes[i].pos.z = (gz - GRID_DIMENSION/2) * GRID_SIZE;
            system3d->pipes[i].direction = directions[rand() % 6];
            system3d->pipes[i].color = pipe_colors[rand() % 8];
            system3d->pipes[i].active = 1;
            system3d->pipes[i].length = 0;
            system3d->pipes[i].segment_count = 0;
            system3d->pipes[i].update_counter = 0;
            system3d->pipes[i].growth_progress = 0.0f;
            
            system3d->grid[gx][gy][gz] = true;
            system3d->active_pipes++;
            break;
        }
    }
}

static void update_pipe(Pipe3D* pipe) {
    if (!pipe->active) return;
    
    // Only update at specified intervals
    pipe->update_counter++;
    if (pipe->update_counter < segment_update_delay) {
        // Continue growing current segment
        pipe->growth_progress += pipe_growth_speed;
        return;
    }
    
    // Reset counter and complete segment
    pipe->update_counter = 0;
    pipe->growth_progress = 0.0f;
    
    // Store current segment
    if (pipe->segment_count < MAX_PIPE_LENGTH) {
        pipe->segments[pipe->segment_count++] = pipe->pos;
    }
    
    // Move pipe to next segment
    Vector3 movement = Vector3Scale(pipe->direction, SEGMENT_LENGTH);
    Vector3 newPos = Vector3Add(pipe->pos, movement);
    
    // Check boundaries
    if (fabs(newPos.x) > GRID_DIMENSION * GRID_SIZE / 2 ||
        fabs(newPos.y) > GRID_DIMENSION * GRID_SIZE / 2 ||
        fabs(newPos.z) > GRID_DIMENSION * GRID_SIZE / 2) {
        pipe->active = 0;
        system3d->active_pipes--;
        return;
    }
    
    pipe->pos = newPos;
    pipe->length++;
    
    // Mark grid position
    int gx = (int)((newPos.x + GRID_DIMENSION * GRID_SIZE / 2) / GRID_SIZE);
    int gy = (int)((newPos.y + GRID_DIMENSION * GRID_SIZE / 2) / GRID_SIZE);
    int gz = (int)((newPos.z + GRID_DIMENSION * GRID_SIZE / 2) / GRID_SIZE);
    
    if (gx >= 0 && gx < GRID_DIMENSION &&
        gy >= 0 && gy < GRID_DIMENSION &&
        gz >= 0 && gz < GRID_DIMENSION) {
        system3d->grid[gx][gy][gz] = true;
    }
    
    // Randomly change direction
    if (rand() % 100 < turn_probability) {
        Vector3 newDir = get_random_direction(pipe->direction, pipe->pos);
        pipe->direction = newDir;
    }
    
    // Deactivate after max length
    if (pipe->length > MAX_PIPE_LENGTH - 5) {
        pipe->active = 0;
        system3d->active_pipes--;
    }
}

static void draw_pipe(Pipe3D* pipe) {
    if (!pipe->active || pipe->segment_count < 2) return;
    
    // Draw pipe segments
    for (int i = 0; i < pipe->segment_count - 1; i++) {
        Vector3 start = pipe->segments[i];
        Vector3 end = pipe->segments[i + 1];
        
        // Draw cylinder between points
        Vector3 midpoint = Vector3Scale(Vector3Add(start, end), 0.5f);
        Vector3 direction = Vector3Normalize(Vector3Subtract(end, start));
        float length = Vector3Distance(start, end);
        
        // Calculate rotation for cylinder
        Vector3 up = { 0, 1, 0 };
        if (fabs(Vector3DotProduct(direction, up)) > 0.99f) {
            up = (Vector3){ 1, 0, 0 };
        }
        
        DrawCylinderEx(start, end, PIPE_RADIUS, PIPE_RADIUS, 16, pipe->color);
        
        // Draw sphere at joint
        if (i < pipe->segment_count - 2) {
            DrawSphere(end, PIPE_RADIUS * 1.1f, pipe->color);
        }
    }
    
    // Draw current segment being built with smooth growth
    if (pipe->segment_count > 0 && pipe->growth_progress > 0) {
        Vector3 start = pipe->segments[pipe->segment_count-1];
        Vector3 direction = Vector3Normalize(pipe->direction);
        float currentLength = SEGMENT_LENGTH * pipe->growth_progress;
        Vector3 end = Vector3Add(start, Vector3Scale(direction, currentLength));
        
        DrawCylinderEx(start, end, PIPE_RADIUS, PIPE_RADIUS, 16, pipe->color);
    }
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_frame() {
    if (!system3d) return;
    
    // Update existing pipes
    for (int i = 0; i < MAX_PIPES; i++) {
        update_pipe(&system3d->pipes[i]);
    }
    
    // Spawn new pipes
    if (system3d->active_pipes < max_active_pipes && rand() % 100 < spawn_rate) {
        spawn_pipe();
    }
    
    // Auto-rotate camera at controlled speed
    system3d->rotation += camera_rotation_speed;
    float radius = 40.0f;
    system3d->camera.position.x = sinf(system3d->rotation) * radius;
    system3d->camera.position.z = cosf(system3d->rotation) * radius;
    
    // Render directly to window
    BeginDrawing();
        ClearBackground(BLACK);
        
        BeginMode3D(system3d->camera);
            // Draw grid bounds (optional)
            DrawCubeWires((Vector3){0, 0, 0}, 
                         GRID_DIMENSION * GRID_SIZE,
                         GRID_DIMENSION * GRID_SIZE,
                         GRID_DIMENSION * GRID_SIZE,
                         (Color){50, 50, 50, 255});
            
            // Draw all pipes
            for (int i = 0; i < MAX_PIPES; i++) {
                draw_pipe(&system3d->pipes[i]);
            }
        EndMode3D();
        
    EndDrawing();
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_resize(int width, int height) {
    if (!system3d) return;
    
    SetWindowSize(width, height);
    
    // Recreate render texture with new size
    UnloadRenderTexture(system3d->target);
    system3d->target = LoadRenderTexture(width, height);
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_cleanup() {
    if (system3d) {
        UnloadRenderTexture(system3d->target);
        CloseWindow();
        free(system3d);
        system3d = NULL;
    }
}

// Parameter setters
EMSCRIPTEN_KEEPALIVE
void pipes3d_setFadeSpeed(int speed) {
    fade_speed = speed;
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_setSpawnRate(int rate) {
    spawn_rate = rate;
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_setTurnProbability(int prob) {
    turn_probability = prob;
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_setMaxPipes(int max) {
    if (max > 0 && max <= MAX_PIPES) {
        max_active_pipes = max;
    }
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_setCameraSpeed(float speed) {
    camera_rotation_speed = speed * 0.001f; // Scale down for reasonable rotation
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_setPipeSpeed(int speed) {
    pipe_growth_speed = speed * 0.001f; // Scale to reasonable growth rate
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_setSegmentDelay(int delay) {
    if (delay > 0) {
        segment_update_delay = delay;
    }
}

// Mouse control functions
EMSCRIPTEN_KEEPALIVE
void pipes3d_mouseDown(int x, int y) {
    if (!system3d) return;
    system3d->mouseDown = true;
    system3d->lastMousePos = (Vector2){ x, y };
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_mouseUp() {
    if (!system3d) return;
    system3d->mouseDown = false;
}

EMSCRIPTEN_KEEPALIVE
void pipes3d_mouseMove(int x, int y) {
    if (!system3d || !system3d->mouseDown) return;
    
    Vector2 currentPos = { x, y };
    Vector2 delta = Vector2Subtract(currentPos, system3d->lastMousePos);
    
    // Update camera rotation based on mouse movement
    system3d->rotation -= delta.x * 0.01f;
    
    // Update vertical angle
    float verticalAngle = atan2f(system3d->camera.position.y, 
                                 sqrtf(system3d->camera.position.x * system3d->camera.position.x + 
                                       system3d->camera.position.z * system3d->camera.position.z));
    verticalAngle += delta.y * 0.01f;
    verticalAngle = fmaxf(-1.2f, fminf(1.2f, verticalAngle));
    
    float radius = 40.0f;
    float horizontalRadius = radius * cosf(verticalAngle);
    system3d->camera.position.x = sinf(system3d->rotation) * horizontalRadius;
    system3d->camera.position.z = cosf(system3d->rotation) * horizontalRadius;
    system3d->camera.position.y = radius * sinf(verticalAngle);
    
    system3d->lastMousePos = currentPos;
}