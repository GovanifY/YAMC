
#include "physics.h"
#include "item.h"
#include "main.h"
#include "util.h"
#include <math.h>

Chunk *find_chunk(int p, int q) {
    for (int i = 0; i < g->chunk_count; i++) {
        Chunk *chunk = g->chunks + i;
        if (chunk->p == p && chunk->q == q) {
            return chunk;
        }
    }
    return 0;
}

int chunk_distance(Chunk *chunk, int p, int q) {
    int dp = ABS(chunk->p - p);
    int dq = ABS(chunk->q - q);
    return MAX(dp, dq);
}

int chunk_visible(float planes[6][4], int p, int q, int miny, int maxy) {
    int x = p * CHUNK_SIZE - 1;
    int z = q * CHUNK_SIZE - 1;
    int d = CHUNK_SIZE + 1;
    float points[8][3] = { { x + 0, miny, z + 0 }, { x + d, miny, z + 0 },
                           { x + 0, miny, z + d }, { x + d, miny, z + d },
                           { x + 0, maxy, z + 0 }, { x + d, maxy, z + 0 },
                           { x + 0, maxy, z + d }, { x + d, maxy, z + d } };
    int n = g->ortho ? 4 : 6;
    for (int i = 0; i < n; i++) {
        int in = 0;
        int out = 0;
        for (int j = 0; j < 8; j++) {
            float d = planes[i][0] * points[j][0] +
                      planes[i][1] * points[j][1] +
                      planes[i][2] * points[j][2] + planes[i][3];
            if (d < 0) {
                out++;
            } else {
                in++;
            }
            if (in && out) {
                break;
            }
        }
        if (in == 0) {
            return 0;
        }
    }
    return 1;
}

int highest_block(float x, float z) {
    int result = -1;
    int nx = roundf(x);
    int nz = roundf(z);
    int p = chunked(x);
    int q = chunked(z);
    Chunk *chunk = find_chunk(p, q);
    if (chunk) {
        Map *map = &chunk->map;
        MAP_FOR_EACH(map, ex, ey, ez, ew) {
            if (is_obstacle(ew) && ex == nx && ez == nz) {
                result = MAX(result, ey);
            }
        }
        END_MAP_FOR_EACH;
    }
    return result;
}

int _hit_test(Map *map, float max_distance, int previous, float x, float y,
              float z, float vx, float vy, float vz, int *hx, int *hy,
              int *hz) {
    int m = 32;
    int px = 0;
    int py = 0;
    int pz = 0;
    for (int i = 0; i < max_distance * m; i++) {
        int nx = roundf(x);
        int ny = roundf(y);
        int nz = roundf(z);
        if (nx != px || ny != py || nz != pz) {
            int hw = map_get(map, nx, ny, nz);
            if (hw > 0) {
                if (previous) {
                    *hx = px;
                    *hy = py;
                    *hz = pz;
                } else {
                    *hx = nx;
                    *hy = ny;
                    *hz = nz;
                }
                return hw;
            }
            px = nx;
            py = ny;
            pz = nz;
        }
        x += vx / m;
        y += vy / m;
        z += vz / m;
    }
    return 0;
}

int hit_test(int previous, float x, float y, float z, float rx, float ry,
             int *bx, int *by, int *bz) {
    int result = 0;
    float best = 0;
    int p = chunked(x);
    int q = chunked(z);
    float vx, vy, vz;
    get_sight_vector(rx, ry, &vx, &vy, &vz);
    for (int i = 0; i < g->chunk_count; i++) {
        Chunk *chunk = g->chunks + i;
        if (chunk_distance(chunk, p, q) > 1) {
            continue;
        }
        int hx, hy, hz;
        int hw = _hit_test(&chunk->map, 8, previous, x, y, z, vx, vy, vz, &hx,
                           &hy, &hz);
        if (hw > 0) {
            float d =
                sqrtf(powf(hx - x, 2) + powf(hy - y, 2) + powf(hz - z, 2));
            if (best == 0 || d < best) {
                best = d;
                *bx = hx;
                *by = hy;
                *bz = hz;
                result = hw;
            }
        }
    }
    return result;
}

int hit_test_face(Player *player, int *x, int *y, int *z, int *face) {
    State *s = &player->state;
    int w = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, x, y, z);
    if (is_obstacle(w)) {
        int hx, hy, hz;
        hit_test(1, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        int dx = hx - *x;
        int dy = hy - *y;
        int dz = hz - *z;
        if (dx == -1 && dy == 0 && dz == 0) {
            *face = 0;
            return 1;
        }
        if (dx == 1 && dy == 0 && dz == 0) {
            *face = 1;
            return 1;
        }
        if (dx == 0 && dy == 0 && dz == -1) {
            *face = 2;
            return 1;
        }
        if (dx == 0 && dy == 0 && dz == 1) {
            *face = 3;
            return 1;
        }
        if (dx == 0 && dy == 1 && dz == 0) {
            int degrees = roundf(DEGREES(atan2f(s->x - hx, s->z - hz)));
            if (degrees < 0) {
                degrees += 360;
            }
            int top = ((degrees + 45) / 90) % 4;
            *face = 4 + top;
            return 1;
        }
    }
    return 0;
}

int collide(int height, float *x, float *y, float *z) {
    int result = 0;
    int p = chunked(*x);
    int q = chunked(*z);
    Chunk *chunk = find_chunk(p, q);
    if (!chunk) {
        return result;
    }
    Map *map = &chunk->map;
    int nx = roundf(*x);
    int ny = roundf(*y);
    int nz = roundf(*z);
    float px = *x - nx;
    float py = *y - ny;
    float pz = *z - nz;
    float pad = 0.25;
    for (int dy = 0; dy < height; dy++) {
        if (px < -pad && is_obstacle(map_get(map, nx - 1, ny - dy, nz))) {
            *x = nx - pad;
        }
        if (px > pad && is_obstacle(map_get(map, nx + 1, ny - dy, nz))) {
            *x = nx + pad;
        }
        if (py < -pad && is_obstacle(map_get(map, nx, ny - dy - 1, nz))) {
            *y = ny - pad;
            result = 1;
        }
        if (py > pad && is_obstacle(map_get(map, nx, ny - dy + 1, nz))) {
            *y = ny + pad;
            result = 1;
        }
        if (pz < -pad && is_obstacle(map_get(map, nx, ny - dy, nz - 1))) {
            *z = nz - pad;
        }
        if (pz > pad && is_obstacle(map_get(map, nx, ny - dy, nz + 1))) {
            *z = nz + pad;
        }
    }
    return result;
}

int collide_side(int height, float *x, float *y, float *z) {
    int result = 0;
    int p = chunked(*x);
    int q = chunked(*z);
    Chunk *chunk = find_chunk(p, q);
    if (!chunk) {
        return result;
    }
    Map *map = &chunk->map;
    int nx = roundf(*x);
    int ny = roundf(*y);
    int nz = roundf(*z);
    for (int dy = 0; dy < height; dy++) {
        if (is_obstacle(map_get(map, nx - 1, ny - dy, nz))) {
            result = 1;
        }
        if (is_obstacle(map_get(map, nx + 1, ny - dy, nz))) {
            result = 1;
        }
        if (is_obstacle(map_get(map, nx, ny - dy, nz - 1))) {
            result = 1;
        }
        if (is_obstacle(map_get(map, nx, ny - dy, nz + 1))) {
            result = 1;
        }
    }
    return result;
}
int player_intersects_block(int height, float x, float y, float z, int hx,
                            int hy, int hz) {
    int nx = roundf(x);
    int ny = roundf(y);
    int nz = roundf(z);
    for (int i = 0; i < height; i++) {
        if (nx == hx && ny - i == hy && nz == hz) {
            return 1;
        }
    }
    return 0;
}
