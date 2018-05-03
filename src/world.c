#include "world.h"
#include "config.h"
#include "main.h"
#include "noise.h"
#include "physics.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
int day_elapsed = 1;

void create_world(int p, int q, world_func func, void *arg, int t, int food,
                  int trees) {
    Chunk *chnk = find_chunk(p, q);
    int cnt_trees = 0;
    int cnt_food = 0;
    int pad = 1;
    for (int dx = -pad; dx < CHUNK_SIZE + pad; dx++) {
        for (int dz = -pad; dz < CHUNK_SIZE + pad; dz++) {
            int flag = 1;
            if (dx < 0 || dz < 0 || dx >= CHUNK_SIZE || dz >= CHUNK_SIZE) {
                flag = -1;
            }
            int x = p * CHUNK_SIZE + dx;
            int z = q * CHUNK_SIZE + dz;
            float f = simplex2(x * 0.01, z * 0.01, 4, 0.5, 2);
            float g = simplex2(-x * 0.01, -z * 0.01, 2, 0.9, 2);
            int mh = g * 32 + 16;
            int h = f * mh;
            int w = 1;

            if (h <= t) {
                // si le plafond est inférieur a la moyenne montagneuse on mets
                // de l'eau
                h = t;
                w = 24;
            }
            // sand and grass terrain
            for (int y = 0; y < h; y++) {
                func(x, y, z, w * flag, arg);
            }
            if (w == 1) {
                if (food) {
                    // herbe
                    if (simplex2(-x * 0.1, z * 0.1, 4, 0.8, 2) > 0.6) {
                        func(x, h, z, 17 * flag, arg);
                    }
                    // fleurs
                    if (simplex2(x * 0.05, -z * 0.05, 4, 0.8, 2) > 0.8) {
                        if (!(chnk->food_blocks[cnt_food].w == 1)) {
                            chnk->food_blocks[cnt_food].x = x;
                            chnk->food_blocks[cnt_food].y = h;
                            chnk->food_blocks[cnt_food].z = z;
                            cnt_food++;

                            int w =
                                18 + simplex2(x * 0.1, z * 0.1, 4, 0.8, 2) * 7;
                            func(x, h, z, w * flag, arg);
                        }
                    }
                }
                int ok = trees;
                if (dx - 4 < 0 || dz - 4 < 0 || dx + 4 >= CHUNK_SIZE ||
                    dz + 4 >= CHUNK_SIZE) {
                    ok = 0;
                }
                // arbres
                if (ok && simplex2(x, z, 6, 0.5, 2) > 0.84) {
                    if (!(chnk->wood_blocks[cnt_trees].w == 1)) {
                        chnk->wood_blocks[cnt_trees].x = x;
                        chnk->wood_blocks[cnt_trees].y = h + 3;
                        chnk->wood_blocks[cnt_trees].z = z;
                        cnt_trees++;
                        for (int y = h + 3; y < h + 8; y++) {
                            for (int ox = -3; ox <= 3; ox++) {
                                for (int oz = -3; oz <= 3; oz++) {
                                    int d = (ox * ox) + (oz * oz) +
                                            (y - (h + 4)) * (y - (h + 4));
                                    if (d < 11) {
                                        func(x + ox, y, z + oz, 15, arg);
                                    }
                                }
                            }
                        }
                        for (int y = h; y < h + 7; y++) {
                            func(x, y, z, 5, arg);
                        }
                    }
                }
            }
            // nuages
            for (int y = 64; y < 72; y++) {
                if (simplex3(x * 0.01, y * 0.1, z * 0.01, 8, 0.5, 2) > 0.75) {
                    func(x, y, z, 16 * flag, arg);
                }
            }
        }
    }
    chnk->wood_cnt = cnt_trees;
    chnk->food_cnt = cnt_food;
}

float time_of_day() {
    if (g->day_length <= 0) {
        return 0.5;
    }
    float t;
    t = glfwGetTime();
    t = t / g->day_length;
    t = t - (int)t;
    return t;
}

float get_daylight() {
    float timer = time_of_day();
    if (timer < 0.5) {
        day_elapsed = 1;
        float t = (timer - 0.25) * 100;
        return 1 / (1 + powf(2, -t));
    } else {
        // on recalcule le monde selon le nombre de jours écoulés
        if (timer > 0.9 && day_elapsed == 1) {
            water_changed = 1;
            water++;
            if (water % 3 == 0) {
                food = 0;
                trees = 1;
            } else {
                if (water % 3 == 1) {
                    trees = 0;
                    food = 1;
                } else {
                    trees = 1;
                    food = 1;
                }
            }

            day_elapsed = 0;
        }
        float t = (timer - 0.85) * 100;
        return 1 - 1 / (1 + powf(2, -t));
    }
}
