#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <emscripten/emscripten.h>
#include <raylib.h>

#define GRID_WIDTH 80
#define GRID_HEIGHT 60
#define PIPE_SEGMENTS 5
#define MAX_COLORS 7

typedef enum {
    DIR_UP = 0,
    DIR_RIGHT = 1,
    DIR_DOWN = 2,
    DIR_LEFT = 3
} Direction;

typedef enum {
    PIPE_NONE = 0,
    PIPE_HORIZONTAL = 1,
    PIPE_VERTICAL = 2,
    PIPE_CORNER_TL = 3,
    PIPE_CORNER_TR = 4,
    PIPE_CORNER_BR = 5,
    PIPE_CORNER_BL = 6
} PipeType;

typedef struct {
    unsigned char type;
    unsigned char color;
} Cell;

typedef struct {
    int x;
    int y;
    Direction dir;
    int color;
    int steps;
} Pipe;

static Cell grid[GRID_HEIGHT][GRID_WIDTH];
static Pipe pipes[10];
static int numPipes = 0;
static int cellSize = 10;
static int speed = 30;
static int thickness = 3;
static int frameCounter = 0;
static Color pipeColors[MAX_COLORS];

static Direction getRandomDirection() {
    return (Direction)(rand() % 4);
}

static void tryAddDirection(Direction dirs[], int *count, Direction dir, int x, int y) {
    int nx = x, ny = y;
    switch (dir) {
        case DIR_UP:    ny--; break;
        case DIR_RIGHT: nx++; break;
        case DIR_DOWN:  ny++; break;
        case DIR_LEFT:  nx--; break;
    }
    
    if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && 
        grid[ny][nx].type == PIPE_NONE) {
        dirs[(*count)++] = dir;
    }
}

static Direction chooseNewDirection(int x, int y, Direction currentDir) {
    Direction possibleDirs[4];
    int dirCount = 0;
    
    tryAddDirection(possibleDirs, &dirCount, currentDir, x, y);
    
    for (int i = 0; i < 4; i++) {
        if (i != currentDir && i != ((currentDir + 2) % 4)) {
            tryAddDirection(possibleDirs, &dirCount, i, x, y);
        }
    }
    
    if (dirCount == 0) {
        for (int i = 0; i < 4; i++) {
            if (i != ((currentDir + 2) % 4)) {
                tryAddDirection(possibleDirs, &dirCount, i, x, y);
            }
        }
    }
    
    if (dirCount == 0) return getRandomDirection();
    return possibleDirs[rand() % dirCount];
}

static PipeType getPipeType(Direction from, Direction to) {
    if ((from == DIR_UP && to == DIR_DOWN) || (from == DIR_DOWN && to == DIR_UP)) {
        return PIPE_VERTICAL;
    }
    if ((from == DIR_LEFT && to == DIR_RIGHT) || (from == DIR_RIGHT && to == DIR_LEFT)) {
        return PIPE_HORIZONTAL;
    }
    
    if ((from == DIR_UP && to == DIR_RIGHT) || (from == DIR_RIGHT && to == DIR_UP)) {
        return PIPE_CORNER_BR;
    }
    if ((from == DIR_RIGHT && to == DIR_DOWN) || (from == DIR_DOWN && to == DIR_RIGHT)) {
        return PIPE_CORNER_BL;
    }
    if ((from == DIR_DOWN && to == DIR_LEFT) || (from == DIR_LEFT && to == DIR_DOWN)) {
        return PIPE_CORNER_TL;
    }
    if ((from == DIR_LEFT && to == DIR_UP) || (from == DIR_UP && to == DIR_LEFT)) {
        return PIPE_CORNER_TR;
    }
    
    return PIPE_HORIZONTAL;
}

static void initPipe(Pipe *pipe) {
    pipe->x = rand() % GRID_WIDTH;
    pipe->y = rand() % GRID_HEIGHT;
    pipe->dir = getRandomDirection();
    pipe->color = rand() % MAX_COLORS;
    pipe->steps = 0;
}

static void updatePipe(Pipe *pipe) {
    if (pipe->steps >= PIPE_SEGMENTS) {
        Direction newDir = chooseNewDirection(pipe->x, pipe->y, pipe->dir);
        Direction from = (Direction)((pipe->dir + 2) % 4);
        
        grid[pipe->y][pipe->x].type = getPipeType(from, newDir);
        grid[pipe->y][pipe->x].color = pipe->color;
        
        switch (pipe->dir) {
            case DIR_UP:    pipe->y--; break;
            case DIR_RIGHT: pipe->x++; break;
            case DIR_DOWN:  pipe->y++; break;
            case DIR_LEFT:  pipe->x--; break;
        }
        
        if (pipe->x < 0 || pipe->x >= GRID_WIDTH || 
            pipe->y < 0 || pipe->y >= GRID_HEIGHT || 
            grid[pipe->y][pipe->x].type != PIPE_NONE) {
            initPipe(pipe);
        } else {
            pipe->dir = newDir;
            pipe->steps = 0;
        }
    } else {
        pipe->steps++;
    }
}

static void drawPipeSegment(int x, int y, PipeType type, Color color, int offset) {
    int cx = x * cellSize + cellSize / 2;
    int cy = y * cellSize + cellSize / 2;
    int halfThick = thickness / 2;
    
    switch (type) {
        case PIPE_HORIZONTAL:
            DrawRectangle(x * cellSize, cy - halfThick, cellSize, thickness, color);
            break;
            
        case PIPE_VERTICAL:
            DrawRectangle(cx - halfThick, y * cellSize, thickness, cellSize, color);
            break;
            
        case PIPE_CORNER_TL:
            DrawRectangle(cx - halfThick, y * cellSize, thickness, cellSize/2 + halfThick, color);
            DrawRectangle(cx - halfThick, cy - halfThick, cellSize/2 + halfThick, thickness, color);
            break;
            
        case PIPE_CORNER_TR:
            DrawRectangle(cx - halfThick, y * cellSize, thickness, cellSize/2 + halfThick, color);
            DrawRectangle(x * cellSize, cy - halfThick, cellSize/2 + halfThick, thickness, color);
            break;
            
        case PIPE_CORNER_BR:
            DrawRectangle(cx - halfThick, cy - halfThick, thickness, cellSize/2 + halfThick, color);
            DrawRectangle(x * cellSize, cy - halfThick, cellSize/2 + halfThick, thickness, color);
            break;
            
        case PIPE_CORNER_BL:
            DrawRectangle(cx - halfThick, cy - halfThick, thickness, cellSize/2 + halfThick, color);
            DrawRectangle(cx - halfThick, cy - halfThick, cellSize/2 + halfThick, thickness, color);
            break;
            
        case PIPE_NONE:
        default:
            // Do nothing for empty cells
            break;
    }
}

static void drawPartialPipe(Pipe *pipe) {
    int progress = (pipe->steps * cellSize) / PIPE_SEGMENTS;
    int x = pipe->x;
    int y = pipe->y;
    int cx = x * cellSize + cellSize / 2;
    int cy = y * cellSize + cellSize / 2;
    int halfThick = thickness / 2;
    Color color = pipeColors[pipe->color];
    
    switch (pipe->dir) {
        case DIR_UP:
            DrawRectangle(cx - halfThick, cy - progress, thickness, progress, color);
            break;
        case DIR_RIGHT:
            DrawRectangle(cx, cy - halfThick, progress, thickness, color);
            break;
        case DIR_DOWN:
            DrawRectangle(cx - halfThick, cy, thickness, progress, color);
            break;
        case DIR_LEFT:
            DrawRectangle(cx - progress, cy - halfThick, progress, thickness, color);
            break;
    }
}

EMSCRIPTEN_KEEPALIVE
void pipes2d_init(int canvasWidth, int canvasHeight) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(canvasWidth, canvasHeight, "Pipes 2D");
    SetTargetFPS(60);
    
    // Initialize colors
    pipeColors[0] = (Color){255, 0, 0, 255};      // Red
    pipeColors[1] = (Color){0, 255, 0, 255};      // Green
    pipeColors[2] = (Color){0, 0, 255, 255};      // Blue
    pipeColors[3] = (Color){255, 255, 0, 255};    // Yellow
    pipeColors[4] = (Color){255, 0, 255, 255};    // Magenta
    pipeColors[5] = (Color){0, 255, 255, 255};    // Cyan
    pipeColors[6] = (Color){255, 165, 0, 255};    // Orange
    
    // Clear grid
    memset(grid, 0, sizeof(grid));
    
    // Initialize pipes
    numPipes = 3;
    for (int i = 0; i < numPipes; i++) {
        initPipe(&pipes[i]);
    }
}

EMSCRIPTEN_KEEPALIVE
void pipes2d_frame() {
    frameCounter++;
    
    // Update pipes based on speed
    if (frameCounter % (60 / speed) == 0) {
        for (int i = 0; i < numPipes; i++) {
            updatePipe(&pipes[i]);
        }
    }
    
    BeginDrawing();
    ClearBackground(BLACK);
    
    // Draw existing pipes
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x].type != PIPE_NONE) {
                drawPipeSegment(x, y, grid[y][x].type, pipeColors[grid[y][x].color], 0);
            }
        }
    }
    
    // Draw active pipe segments
    for (int i = 0; i < numPipes; i++) {
        drawPartialPipe(&pipes[i]);
    }
    
    EndDrawing();
}

EMSCRIPTEN_KEEPALIVE
void pipes2d_setSpeed(int newSpeed) {
    speed = newSpeed;
}

EMSCRIPTEN_KEEPALIVE
void pipes2d_setThickness(int newThickness) {
    thickness = newThickness;
}

EMSCRIPTEN_KEEPALIVE
void pipes2d_setPipeCount(int count) {
    if (count > 0 && count <= 10) {
        if (count > numPipes) {
            for (int i = numPipes; i < count; i++) {
                initPipe(&pipes[i]);
            }
        }
        numPipes = count;
    }
}

EMSCRIPTEN_KEEPALIVE
void pipes2d_resize(int width, int height) {
    SetWindowSize(width, height);
    cellSize = fmin(width / GRID_WIDTH, height / GRID_HEIGHT);
}

EMSCRIPTEN_KEEPALIVE
void pipes2d_cleanup() {
    CloseWindow();
}