#ifndef SRC_DRAW_H_
#define SRC_DRAW_H_

#include "main.h"

GLuint gen_crosshair_buffer();
GLuint gen_wireframe_buffer(float x, float y, float z, float n);
GLuint gen_sky_buffer();
GLuint gen_cube_buffer(float x, float y, float z, float n, int w);
GLuint gen_plant_buffer(float x, float y, float z, float n, int w);
GLuint gen_player_buffer(float x, float y, float z, float rx, float ry);

void draw_triangles_3d_ao(Attrib *attrib, GLuint buffer, int count);
void draw_triangles_3d(Attrib *attrib, GLuint buffer, int count);
void draw_triangles_2d(Attrib *attrib, GLuint buffer, int count);

void draw_lines(Attrib *attrib, GLuint buffer, int components, int count);
void draw_chunk(Attrib *attrib, Chunk *chunk);
void draw_item(Attrib *attrib, GLuint buffer, int count);
void draw_cube(Attrib *attrib, GLuint buffer);
void draw_plant(Attrib *attrib, GLuint buffer);
void draw_player(Attrib *attrib, Player *player);

int render_chunks(Attrib *attrib, Player *player);
void render_players(Attrib *attrib, Player *player);
void render_sky(Attrib *attrib, Player *player, GLuint buffer);
void render_wireframe(Attrib *attrib, Player *player);
void render_crosshairs(Attrib *attrib);

void check_workers();
void force_chunks(Player *player);
void ensure_chunks_worker(Player *player, Worker *worker);
void ensure_chunks(Player *player);

int worker_run(void *arg);

void load_chunk(WorkerItem *item);

void request_chunk(int p, int q);

void init_chunk(Chunk *chunk, int p, int q);

void create_chunk(Chunk *chunk, int p, int q);

void delete_chunks();

void delete_all_chunks();

#define XZ_SIZE (CHUNK_SIZE * 3 + 2)
#define XZ_LO (CHUNK_SIZE)
#define XZ_HI (CHUNK_SIZE * 2 + 1)
#define Y_SIZE 258
#define XYZ(x, y, z) ((y)*XZ_SIZE * XZ_SIZE + (x)*XZ_SIZE + (z))
#define XZ(x, z) ((x)*XZ_SIZE + (z))

void light_fill(char *opaque, char *light, int x, int y, int z, int w,
                int force);

void compute_chunk(WorkerItem *item);

void generate_chunk(Chunk *chunk, WorkerItem *item);

void gen_chunk_buffer(Chunk *chunk);

void map_set_func(int x, int y, int z, int w, void *arg);

void dirty_chunk(Chunk *chunk);

void occlusion(char neighbors[27], char lights[27], float shades[27],
               float ao[6][4], float light[6][4]);

int has_lights(Chunk *chunk);

void toggle_light(int x, int y, int z);

void set_light(int p, int q, int x, int y, int z, int w);

void _set_block(int p, int q, int x, int y, int z, int w, int dirty);

void set_block(int x, int y, int z, int w);

void record_block(int x, int y, int z, int w);

int get_block(int x, int y, int z);

void builder_block(int x, int y, int z, int w);

void draw_bot(Attrib *attrib, Bot *bot);
GLuint gen_bot_buffer(float x, float y, float z, float rx, float ry);
void render_bots(Attrib *attrib);

void draw_mob(Attrib *attrib, Mob *mob);
GLuint gen_mob_buffer(float x, float y, float z, float rx, float ry);
void render_mobs(Attrib *attrib);

#endif
