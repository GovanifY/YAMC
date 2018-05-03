
#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include "config.h"
#include "map.h"
#include "threads.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define MAX_CHUNKS 8192
#define MAX_PLAYERS 128
#define MAX_BOTS 100
#define MAX_MOBS 100
#define WORKERS 4

#define WORKER_IDLE 0
#define WORKER_BUSY 1
#define WORKER_DONE 2

typedef struct {
    int p;
    int q;
    int load;
    Map *block_maps[3][3];
    Map *light_maps[3][3];
    int miny;
    int maxy;
    int faces;
    GLfloat *data;
} WorkerItem;

typedef struct {
    int index;
    int state;
    thrd_t thrd;
    mtx_t mtx;
    cnd_t cnd;
    WorkerItem item;
} Worker;

typedef struct {
    int x;
    int y;
    int z;
    int w;
} Block;

typedef struct {
    Map map;
    Map lights;
    int p;
    int q;
    int faces;
    int dirty;
    int miny;
    int maxy;
    int wood_cnt;
    int food_cnt;
    Block wood_blocks[MAX_WOOD];
    Block food_blocks[MAX_FOOD];
    GLuint buffer;
} Chunk;

typedef struct {
    float x;
    float y;
    float z;
    float rx;
    float ry;
    float t;
} State;

typedef struct {
    int id;
    State state;
    GLuint buffer;
} Player;

typedef struct {
    int id;
    State state;
    int target_type;
    int target_id;
    float delay;
    GLuint buffer;
} Bot;

typedef struct {
    int id;
    State state;
    int target_type;
    int target_id;
    float delay;
    GLuint buffer;
} Mob;

typedef struct {
    GLuint program;
    GLuint position;
    GLuint normal;
    GLuint uv;
    GLuint matrix;
    GLuint sampler;
    GLuint camera;
    GLuint timer;
    GLuint extra1;
    GLuint extra2;
    GLuint extra3;
    GLuint extra4;
} Attrib;

typedef struct {
    GLFWwindow *window;
    Worker workers[WORKERS];
    Chunk chunks[MAX_CHUNKS];
    int chunk_count;
    int create_radius;
    int render_radius;
    int delete_radius;
    Player players[MAX_PLAYERS];
    Bot bots[MAX_BOTS];
    Mob mobs[MAX_BOTS];
    int player_count;
    int bot_count;
    int mob_count;
    int width;
    int height;
    int flying;
    int item_index;
    int scale;
    int ortho;
    float fov;
    int mode;
    int mode_changed;
    int day_length;
    int time_changed;
    Block block0;
    Block block1;
    Block copy0;
    Block copy1;
} Model;

extern Model *g;
extern int water;
extern int water_changed;
extern int food;
extern int wood_total;
extern int trees;

void get_sight_vector(float rx, float ry, float *vx, float *vy, float *vz);
#endif
