#include "main.h"
#include "config.h"
#include "cube.h"
#include "draw.h"
#include "item.h"
#include "map.h"
#include "noise.h"
#include "physics.h"
#include "threads.h"
#include "util.h"
#include "world.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static Model model;
int water = 10;
int food = 1;
int trees = 1;
int water_changed = 0;
int wood_total = 0;
int gagne = 0;
Model *g = &model;

int get_scale_factor() {
    int window_width, window_height;
    int buffer_width, buffer_height;
    glfwGetWindowSize(g->window, &window_width, &window_height);
    glfwGetFramebufferSize(g->window, &buffer_width, &buffer_height);
    int result = buffer_width / window_width;
    result = MAX(1, result);
    result = MIN(2, result);
    return result;
}

void get_sight_vector(float rx, float ry, float *vx, float *vy, float *vz) {
    float m = cosf(ry);
    *vx = cosf(rx - RADIANS(90)) * m;
    *vy = sinf(ry);
    *vz = sinf(rx - RADIANS(90)) * m;
}

void get_motion_vector(int flying, int sz, int sx, float rx, float ry,
                       float *vx, float *vy, float *vz) {
    *vx = 0;
    *vy = 0;
    *vz = 0;
    if (!sz && !sx) {
        return;
    }
    float strafe = atan2f(sz, sx);
    if (flying) {
        float m = cosf(ry);
        float y = sinf(ry);
        if (sx) {
            if (!sz) {
                y = 0;
            }
            m = 1;
        }
        if (sz > 0) {
            y = -y;
        }
        *vx = cosf(rx + strafe) * m;
        *vy = y;
        *vz = sinf(rx + strafe) * m;
    } else {
        *vx = cosf(rx + strafe);
        *vy = 0;
        *vz = sinf(rx + strafe);
    }
}

Player *find_player(int id) {
    for (int i = 0; i < g->player_count; i++) {
        Player *player = g->players + i;
        if (player->id == id) {
            return player;
        }
    }
    return 0;
}

void update_bot(Bot *bot, float x, float y, float z, float rx, float ry) {
    State *s = &bot->state;
    s->x = x;
    s->y = y;
    s->z = z;
    s->rx = rx;
    s->ry = ry;
    del_buffer(bot->buffer);
    bot->buffer = gen_bot_buffer(s->x, s->y, s->z, s->rx, s->ry);
}
void update_mob(Mob *mob, float x, float y, float z, float rx, float ry) {
    State *s = &mob->state;
    s->x = x;
    s->y = y;
    s->z = z;
    s->rx = rx;
    s->ry = ry;
    del_buffer(mob->buffer);
    mob->buffer = gen_mob_buffer(s->x, s->y, s->z, s->rx, s->ry);
}

float player_player_distance(Player *p1, Player *p2) {
    State *s1 = &p1->state;
    State *s2 = &p2->state;
    float x = s2->x - s1->x;
    float y = s2->y - s1->y;
    float z = s2->z - s1->z;
    return sqrtf(x * x + y * y + z * z);
}

float player_crosshair_distance(Player *p1, Player *p2) {
    State *s1 = &p1->state;
    State *s2 = &p2->state;
    float d = player_player_distance(p1, p2);
    float vx, vy, vz;
    get_sight_vector(s1->rx, s1->ry, &vx, &vy, &vz);
    vx *= d;
    vy *= d;
    vz *= d;
    float px, py, pz;
    px = s1->x + vx;
    py = s1->y + vy;
    pz = s1->z + vz;
    float x = s2->x - px;
    float y = s2->y - py;
    float z = s2->z - pz;
    return sqrtf(x * x + y * y + z * z);
}

Player *player_crosshair(Player *player) {
    Player *result = 0;
    float threshold = RADIANS(5);
    float best = 0;
    for (int i = 0; i < g->player_count; i++) {
        Player *other = g->players + i;
        if (other == player) {
            continue;
        }
        float p = player_crosshair_distance(player, other);
        float d = player_player_distance(player, other);
        if (d < 96 && p / d < threshold) {
            if (best == 0 || d < best) {
                best = d;
                result = other;
            }
        }
    }
    return result;
}

void copy() {
    memcpy(&g->copy0, &g->block0, sizeof(Block));
    memcpy(&g->copy1, &g->block1, sizeof(Block));
}

void paste() {
    Block *c1 = &g->copy1;
    Block *c2 = &g->copy0;
    Block *p1 = &g->block1;
    Block *p2 = &g->block0;
    int scx = SIGN(c2->x - c1->x);
    int scz = SIGN(c2->z - c1->z);
    int spx = SIGN(p2->x - p1->x);
    int spz = SIGN(p2->z - p1->z);
    int oy = p1->y - c1->y;
    int dx = ABS(c2->x - c1->x);
    int dz = ABS(c2->z - c1->z);
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x <= dx; x++) {
            for (int z = 0; z <= dz; z++) {
                int w = get_block(c1->x + x * scx, y, c1->z + z * scz);
                builder_block(p1->x + x * spx, y + oy, p1->z + z * spz, w);
            }
        }
    }
}

void array(Block *b1, Block *b2, int xc, int yc, int zc) {
    if (b1->w != b2->w) {
        return;
    }
    int w = b1->w;
    int dx = b2->x - b1->x;
    int dy = b2->y - b1->y;
    int dz = b2->z - b1->z;
    xc = dx ? xc : 1;
    yc = dy ? yc : 1;
    zc = dz ? zc : 1;
    for (int i = 0; i < xc; i++) {
        int x = b1->x + dx * i;
        for (int j = 0; j < yc; j++) {
            int y = b1->y + dy * j;
            for (int k = 0; k < zc; k++) {
                int z = b1->z + dz * k;
                builder_block(x, y, z, w);
            }
        }
    }
}

void cube(Block *b1, Block *b2, int fill) {
    if (b1->w != b2->w) {
        return;
    }
    int w = b1->w;
    int x1 = MIN(b1->x, b2->x);
    int y1 = MIN(b1->y, b2->y);
    int z1 = MIN(b1->z, b2->z);
    int x2 = MAX(b1->x, b2->x);
    int y2 = MAX(b1->y, b2->y);
    int z2 = MAX(b1->z, b2->z);
    int a = (x1 == x2) + (y1 == y2) + (z1 == z2);
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            for (int z = z1; z <= z2; z++) {
                if (!fill) {
                    int n = 0;
                    n += x == x1 || x == x2;
                    n += y == y1 || y == y2;
                    n += z == z1 || z == z2;
                    if (n <= a) {
                        continue;
                    }
                }
                builder_block(x, y, z, w);
            }
        }
    }
}

void sphere(Block *center, int radius, int fill, int fx, int fy, int fz) {
    static const float offsets[8][3] = {
        { -0.5, -0.5, -0.5 }, { -0.5, -0.5, 0.5 }, { -0.5, 0.5, -0.5 },
        { -0.5, 0.5, 0.5 },   { 0.5, -0.5, -0.5 }, { 0.5, -0.5, 0.5 },
        { 0.5, 0.5, -0.5 },   { 0.5, 0.5, 0.5 }
    };
    int cx = center->x;
    int cy = center->y;
    int cz = center->z;
    int w = center->w;
    for (int x = cx - radius; x <= cx + radius; x++) {
        if (fx && x != cx) {
            continue;
        }
        for (int y = cy - radius; y <= cy + radius; y++) {
            if (fy && y != cy) {
                continue;
            }
            for (int z = cz - radius; z <= cz + radius; z++) {
                if (fz && z != cz) {
                    continue;
                }
                int inside = 0;
                int outside = fill;
                for (int i = 0; i < 8; i++) {
                    float dx = x + offsets[i][0] - cx;
                    float dy = y + offsets[i][1] - cy;
                    float dz = z + offsets[i][2] - cz;
                    float d = sqrtf(dx * dx + dy * dy + dz * dz);
                    if (d < radius) {
                        inside = 1;
                    } else {
                        outside = 1;
                    }
                }
                if (inside && outside) {
                    builder_block(x, y, z, w);
                }
            }
        }
    }
}

void cylinder(Block *b1, Block *b2, int radius, int fill) {
    if (b1->w != b2->w) {
        return;
    }
    int w = b1->w;
    int x1 = MIN(b1->x, b2->x);
    int y1 = MIN(b1->y, b2->y);
    int z1 = MIN(b1->z, b2->z);
    int x2 = MAX(b1->x, b2->x);
    int y2 = MAX(b1->y, b2->y);
    int z2 = MAX(b1->z, b2->z);
    int fx = x1 != x2;
    int fy = y1 != y2;
    int fz = z1 != z2;
    if (fx + fy + fz != 1) {
        return;
    }
    Block block = { x1, y1, z1, w };
    if (fx) {
        for (int x = x1; x <= x2; x++) {
            block.x = x;
            sphere(&block, radius, fill, 1, 0, 0);
        }
    }
    if (fy) {
        for (int y = y1; y <= y2; y++) {
            block.y = y;
            sphere(&block, radius, fill, 0, 1, 0);
        }
    }
    if (fz) {
        for (int z = z1; z <= z2; z++) {
            block.z = z;
            sphere(&block, radius, fill, 0, 0, 1);
        }
    }
}

void on_light() {
    State *s = &g->players->state;
    int hx, hy, hz;
    int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    if (hy > 0 && hy < 256 && is_destructable(hw)) {}
}

void on_key(GLFWwindow *window, int key, int scancode, int action, int mods) {
    int exclusive =
        glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    if (action == GLFW_RELEASE) {
        return;
    }
    if (action != GLFW_PRESS) {
        return;
    }
    if (key == GLFW_KEY_ESCAPE) {
        if (exclusive) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if (key == CRAFT_KEY_FLY) {
        g->flying = !g->flying;
    }
    if (key >= '1' && key <= '9') {
        g->item_index = key - '1';
    }
    if (key == '0') {
        g->item_index = 9;
    }
    if (key == CRAFT_KEY_ITEM_NEXT) {
        g->item_index = (g->item_index + 1) % item_count;
    }
    if (key == CRAFT_KEY_ITEM_PREV) {
        g->item_index--;
        if (g->item_index < 0) {
            g->item_index = item_count - 1;
        }
    }
}

void on_scroll(GLFWwindow *window, double xdelta, double ydelta) {
    static double ypos = 0;
    ypos += ydelta;
    if (ypos < -SCROLL_THRESHOLD) {
        g->item_index = (g->item_index + 1) % item_count;
        ypos = 0;
    }
    if (ypos > SCROLL_THRESHOLD) {
        g->item_index--;
        if (g->item_index < 0) {
            g->item_index = item_count - 1;
        }
        ypos = 0;
    }
}

void on_mouse_button(GLFWwindow *window, int button, int action, int mods) {
    if (action != GLFW_PRESS) {
        return;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void create_window() {
    int window_width = WINDOW_WIDTH;
    int window_height = WINDOW_HEIGHT;
    GLFWmonitor *monitor = NULL;
    if (FULLSCREEN) {
        int mode_count;
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *modes = glfwGetVideoModes(monitor, &mode_count);
        window_width = modes[mode_count - 1].width;
        window_height = modes[mode_count - 1].height;
    }
    g->window =
        glfwCreateWindow(window_width, window_height, "YAMC", monitor, NULL);
}

void handle_mouse_input() {
    int exclusive =
        glfwGetInputMode(g->window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    static double px = 0;
    static double py = 0;
    State *s = &g->players->state;
    if (exclusive && (px || py)) {
        double mx, my;
        glfwGetCursorPos(g->window, &mx, &my);
        float m = 0.0025;
        s->rx += (mx - px) * m;
        if (INVERT_MOUSE) {
            s->ry += (my - py) * m;
        } else {
            s->ry -= (my - py) * m;
        }
        if (s->rx < 0) {
            s->rx += RADIANS(360);
        }
        if (s->rx >= RADIANS(360)) {
            s->rx -= RADIANS(360);
        }
        s->ry = MAX(s->ry, -RADIANS(90));
        s->ry = MIN(s->ry, RADIANS(90));
        px = mx;
        py = my;
    } else {
        glfwGetCursorPos(g->window, &px, &py);
    }
}

void handle_movement(double dt) {
    static float dy = 0;
    State *s = &g->players->state;
    int sz = 0;
    int sx = 0;
    float m = dt * 1.0;
    g->ortho = glfwGetKey(g->window, CRAFT_KEY_ORTHO) ? 64 : 0;
    g->fov = glfwGetKey(g->window, CRAFT_KEY_ZOOM) ? 15 : 65;
    if (glfwGetKey(g->window, CRAFT_KEY_FORWARD))
        sz--;
    if (glfwGetKey(g->window, CRAFT_KEY_BACKWARD))
        sz++;
    if (glfwGetKey(g->window, CRAFT_KEY_LEFT))
        sx--;
    if (glfwGetKey(g->window, CRAFT_KEY_RIGHT))
        sx++;
    if (glfwGetKey(g->window, GLFW_KEY_LEFT))
        s->rx -= m;
    if (glfwGetKey(g->window, GLFW_KEY_RIGHT))
        s->rx += m;
    if (glfwGetKey(g->window, GLFW_KEY_UP))
        s->ry += m;
    if (glfwGetKey(g->window, GLFW_KEY_DOWN))
        s->ry -= m;
    float vx, vy, vz;
    get_motion_vector(g->flying, sz, sx, s->rx, s->ry, &vx, &vy, &vz);
    if (glfwGetKey(g->window, CRAFT_KEY_JUMP)) {
        if (g->flying) {
            vy = 1;
        } else if (dy == 0) {
            dy = 8;
        }
    }
    float speed = g->flying ? 20 : 5;
    int estimate =
        roundf(sqrtf(powf(vx * speed, 2) + powf(vy * speed + ABS(dy) * 2, 2) +
                     powf(vz * speed, 2)) *
               dt * 8);
    int step = MAX(8, estimate);
    float ut = dt / step;
    vx = vx * ut * speed;
    vy = vy * ut * speed;
    vz = vz * ut * speed;
    for (int i = 0; i < step; i++) {
        if (g->flying) {
            dy = 0;
        }

        else {
            dy -= ut * 25;
            dy = MAX(dy, -250);
        }
        s->x += vx;
        s->y += vy + dy * ut;
        s->z += vz;
        if (collide(2, &s->x, &s->y, &s->z)) {
            dy = 0;
        }
    }
    if (s->y < 0) {
        s->y = highest_block(s->x, s->z) + 2;
    }
}

void delete_all_players() {
    for (int i = 0; i < g->player_count; i++) {
        Player *player = g->players + i;
        del_buffer(player->buffer);
    }
    g->player_count = 0;
}

void handle_bot_movement(Bot *bot, double dt) {
    static float dy = 0;
    State *s = &bot->state;

    int p = chunked(s->x);
    int q = chunked(s->z);

    int ox = 0;
    int oz = 0;
    Chunk *chnk = find_chunk(p, q);
    // si ils ne sont pas dans le champs de vision ça sert a rien de simuler
    if (!chnk) {
        return;
    }
    bot->delay -= dt;
    if (bot->target_type == 0 && wood_total < WANTED_WOOD) {
        if (rand() % 2 == 0) {
            if (chnk->wood_cnt > 0) {
                printf("bot %d assigned tree %d, out of %d, coordinates: %d, "
                       "%d, %d\n",
                       bot->id, chnk->wood_cnt - 1, chnk->wood_cnt,
                       chnk->wood_blocks[chnk->wood_cnt - 1].x,
                       chnk->wood_blocks[chnk->wood_cnt - 1].y,
                       chnk->wood_blocks[chnk->wood_cnt - 1].z);
                bot->target_type = 1;
                bot->target_id = chnk->wood_cnt - 1;
                chnk->wood_cnt--;
            }
        } else {
            if (chnk->food_cnt > 0) {
                printf("bot %d assigned food %d, out of %d, coordinates: %d, "
                       "%d, %d\n",
                       bot->id, chnk->food_cnt - 1, chnk->food_cnt,
                       chnk->food_blocks[chnk->food_cnt - 1].x,
                       chnk->food_blocks[chnk->food_cnt - 1].y,
                       chnk->food_blocks[chnk->food_cnt - 1].z);
                bot->target_type = 2;
                bot->target_id = chnk->food_cnt - 1;
                chnk->food_cnt--;
            }
        }
    }

    int px = (int)s->x % CHUNK_SIZE;
    if (px < 0) {
        px += CHUNK_SIZE;
    }
    // int py = floorf(s->y);
    int pz = (int)s->z % CHUNK_SIZE;
    if (pz < 0) {
        pz += CHUNK_SIZE;
    }
    /*printf("I am the bot %d! I am at x: %f, y: %f, z: %f, chunk %d, %d, "
           "relative coordinates: %d, %d, %d!\n",
           bot->id, s->x, s->y, s->z, p, q, px, py, pz);*/
    if (wood_total >= WANTED_WOOD) {
        if ((abs(px - ox) < 2) && (abs(pz - oz) < 2)) {
            printf("Gagné!\n");
            gagne = 1;
            return;
        }
    }

    if (bot->target_type == 1) {

        ox = chnk->wood_blocks[bot->target_id].x;
        oz = chnk->wood_blocks[bot->target_id].z;
        if ((abs(px - ox) < 2) && (abs(pz - oz) < 2)) {
            chnk->wood_blocks[bot->target_id].w = 1;
            bot->target_type = 0;
            dirty_chunk(chnk);
            wood_total++;
        }
    }

    if (bot->target_type == 2) {

        ox = chnk->food_blocks[bot->target_id].x;
        oz = chnk->food_blocks[bot->target_id].z;
        if ((abs(px - ox) < 2) && (abs(pz - oz) < 2)) {
            chnk->food_blocks[bot->target_id].w = 1;
            bot->target_type = 0;
            dirty_chunk(chnk);
        }
    }

    // printf("Assigned coordinates: %d, %d\n", ox, oz);

    int sz = 0;
    int sx = 0;
    int flying = 0;

    if (bot->target_type == 0 && wood_total < WANTED_WOOD) {
        sz++;
    } else {
        if (px < ox) {
            sx++;
        }
        if (px > ox) {
            sx--;
        }
        if (pz < oz) {
            sz++;
        }
        if (pz > oz) {
            sz--;
        }
    }

    float vx, vy, vz;
    get_motion_vector(flying, sz, sx, s->rx, s->ry, &vx, &vy, &vz);
    if (collide_side(1, &s->x, &s->y, &s->z)) {
        if (dy == 0 && bot->delay < 0) {
            dy = 16;
            bot->delay = 2;
        }
    }

    float speed = flying ? 20 : 5;
    int estimate =
        roundf(sqrtf(powf(vx * speed, 2) + powf(vy * speed + ABS(dy) * 2, 2) +
                     powf(vz * speed, 2)) *
               dt * 8);
    int step = MAX(8, estimate);
    float ut = dt / step;
    vx = vx * ut * speed;
    vy = vy * ut * speed;
    vz = vz * ut * speed;

    for (int i = 0; i < step; i++) {
        if (flying) {
            dy = 0;
        } else {
            dy -= ut * 25;
            dy = MAX(dy, -250);
            if (dy < 0) {
                dy = -8;
            }
        }
        s->x += vx;
        s->y += vy + dy * ut;
        s->z += vz;

        if (collide(1, &s->x, &s->y, &s->z)) {
            dy = 0;
        }
    }
    if (s->y < 0) {
        s->y = highest_block(s->x, s->z) + 2;
    }
}

static void delete_bot(int id) {
    int count;
    Bot *other;
    Bot *bot = &g->bots[id];
    count = g->bot_count;
    del_buffer(bot->buffer);
    other = g->bots + (--count);
    memcpy(bot, other, sizeof(Player));
    g->bot_count = count;
}
void handle_mob_movement(Mob *mob, double dt) {
    static float dy = 0;
    State *s = &mob->state;

    int ox = 0;
    int oz = 0;
    mob->delay -= dt;
    if (mob->target_type == 0 && g->bot_count != 0) {
        int target = rand() % g->bot_count;
        State *st = &g->bots[target].state;
        printf("mob %d assigned bot %d, coordinates: %f, %f, %f\n", mob->id,
               g->bots[target].id, st->x, st->y, st->z);
        mob->target_type = 1;
        mob->target_id = target;
    }
    if (mob->target_type == 0 && g->bot_count == 0) {
        printf("Perdu!\n");
        gagne = 1;
        return;
    }

    if (mob->target_type == 1) {
        State *st = &g->bots[mob->target_id].state;
        ox = st->x;
        oz = st->z;
        if ((abs(s->x - ox) < 2) && (abs(s->z - oz) < 2)) {
            delete_bot(mob->target_id);
            mob->target_type = 0;
        }
    }

    // printf("Assigned coordinates: %d, %d\n", ox, oz);

    int sz = 0;
    int sx = 0;
    int flying = 0;

    if (s->x < ox) {
        sx++;
    }
    if (s->x > ox) {
        sx--;
    }
    if (s->z < oz) {
        sz++;
    }
    if (s->z > oz) {
        sz--;
    }

    float vx, vy, vz;
    get_motion_vector(flying, sz, sx, s->rx, s->ry, &vx, &vy, &vz);
    if (collide_side(1, &s->x, &s->y, &s->z)) {
        if (dy == 0 && mob->delay < 0) {
            dy = 16;
            mob->delay = 2;
        }
    }

    float speed = flying ? 20 : 2;
    int estimate =
        roundf(sqrtf(powf(vx * speed, 2) + powf(vy * speed + ABS(dy) * 2, 2) +
                     powf(vz * speed, 2)) *
               dt * 8);
    int step = MAX(8, estimate);
    float ut = dt / step;
    vx = vx * ut * speed;
    vy = vy * ut * speed;
    vz = vz * ut * speed;

    for (int i = 0; i < step; i++) {
        if (flying) {
            dy = 0;
        } else {
            dy -= ut * 25;
            dy = MAX(dy, -250);
            if (dy < 0) {
                dy = -8;
            }
        }
        s->x += vx;
        s->y += vy + dy * ut;
        s->z += vz;

        if (collide(1, &s->x, &s->y, &s->z)) {
            dy = 0;
        }
    }
    if (s->y < 0) {
        s->y = highest_block(s->x, s->z) + 2;
    }
}

void reset_model() {
    memset(g->chunks, 0, sizeof(Chunk) * MAX_CHUNKS);
    g->chunk_count = 0;
    memset(g->players, 0, sizeof(Player) * MAX_PLAYERS);
    g->player_count = 0;
    g->flying = 0;
    g->item_index = 0;
    g->day_length = DAY_LENGTH;
    glfwSetTime(g->day_length / 3.0);
    g->time_changed = 1;
}

int main(int argc, char **argv) {
    // on initialise le rng
    srand(time(NULL));
    seed(rand());

    // on crée la fenêtre principale et les events
    if (!glfwInit()) {
        return -1;
    }
    create_window();
    if (!g->window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(g->window);
    glfwSwapInterval(VSYNC);
    glfwSetInputMode(g->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(g->window, on_key);
    glfwSetMouseButtonCallback(g->window, on_mouse_button);
    glfwSetScrollCallback(g->window, on_scroll);

    // on initialise glew
    if (glewInit() != GLEW_OK) {
        return -1;
    }

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glLogicOp(GL_INVERT);
    glClearColor(0, 0, 0, 1);

    // on charge les textures pour ogl
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    load_png_texture("textures/texture.png");

    GLuint sky;
    glGenTextures(1, &sky);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    load_png_texture("textures/sky.png");

    // on charge les shaders
    Attrib block_attrib = { 0 };
    Attrib sky_attrib = { 0 };
    GLuint program;

    program = load_program("shaders/block_vertex.glsl",
                           "shaders/block_fragment.glsl");
    block_attrib.program = program;
    block_attrib.position = glGetAttribLocation(program, "position");
    block_attrib.normal = glGetAttribLocation(program, "normal");
    block_attrib.uv = glGetAttribLocation(program, "uv");
    block_attrib.matrix = glGetUniformLocation(program, "matrix");
    block_attrib.sampler = glGetUniformLocation(program, "sampler");
    block_attrib.extra1 = glGetUniformLocation(program, "sky_sampler");
    block_attrib.extra2 = glGetUniformLocation(program, "daylight");
    block_attrib.extra3 = glGetUniformLocation(program, "fog_distance");
    block_attrib.extra4 = glGetUniformLocation(program, "ortho");
    block_attrib.camera = glGetUniformLocation(program, "camera");
    block_attrib.timer = glGetUniformLocation(program, "timer");

    program =
        load_program("shaders/sky_vertex.glsl", "shaders/sky_fragment.glsl");
    sky_attrib.program = program;
    sky_attrib.position = glGetAttribLocation(program, "position");
    sky_attrib.normal = glGetAttribLocation(program, "normal");
    sky_attrib.uv = glGetAttribLocation(program, "uv");
    sky_attrib.matrix = glGetUniformLocation(program, "matrix");
    sky_attrib.sampler = glGetUniformLocation(program, "sampler");
    sky_attrib.timer = glGetUniformLocation(program, "timer");

    g->create_radius = CREATE_CHUNK_RADIUS;
    g->render_radius = RENDER_CHUNK_RADIUS;
    g->delete_radius = DELETE_CHUNK_RADIUS;

    // on initialise les threads
    for (int i = 0; i < WORKERS; i++) {
        Worker *worker = g->workers + i;
        worker->index = i;
        worker->state = WORKER_IDLE;
        mtx_init(&worker->mtx, mtx_plain);
        cnd_init(&worker->cnd);
        thrd_create(&worker->thrd, worker_run, worker);
    }

    int running = 1;
    while (running) {

        reset_model();
        GLuint sky_buffer = gen_sky_buffer();

        Player *me = g->players;
        State *s = &g->players->state;
        me->id = 0;
        me->buffer = 0;
        g->player_count = 1;

        force_chunks(me);
        s->y = highest_block(s->x, s->z) + 2;

        // on crée les bots
        g->bot_count = 5;
        for (int i = 0; i < g->bot_count; i++) {
            g->bots[i].id = i;
            g->bots[i].buffer = 0;
            update_bot(&g->bots[i], s->x + i + 1, s->y, s->z, s->rx, s->ry);
        }
        g->mob_count = 5;
        for (int i = 0; i < g->mob_count; i++) {
            g->mobs[i].id = i;
            g->mobs[i].buffer = 0;
            update_mob(&g->mobs[i], s->x + 30 + i + g->bot_count + 1, s->y + 10,
                       s->z, s->rx, s->ry);
        }

        // boucle principale
        double previous = glfwGetTime();
        while (1) {
            g->scale = get_scale_factor();
            glfwGetFramebufferSize(g->window, &g->width, &g->height);
            glViewport(0, 0, g->width, g->height);

            double now = glfwGetTime();
            double dt = now - previous;
            dt = MIN(dt, 0.2);
            dt = MAX(dt, 0.0);
            previous = now;

            // on maj les résultats des events
            handle_mouse_input();

            handle_movement(dt);

            // on fait bouger les bots
            for (int i = 0; i < g->bot_count; i++) {
                handle_bot_movement(g->bots + i, dt);
            }
            for (int i = 0; i < g->mob_count; i++) {
                handle_mob_movement(g->mobs + i, dt);
            }

            delete_chunks();
            del_buffer(me->buffer);
            me->buffer = gen_player_buffer(s->x, s->y, s->z, s->rx, s->ry);

            // on maj la position des bots
            for (int i = 0; i < g->bot_count; i++) {
                State *s = &g->bots[i].state;
                update_bot(&g->bots[i], s->x, s->y, s->z, s->rx, s->ry);
            }
            for (int i = 0; i < g->mob_count; i++) {
                State *s = &g->mobs[i].state;
                update_mob(&g->mobs[i], s->x, s->y, s->z, s->rx, s->ry);
            }

            Player *player = g->players;

            // on calcule le rendu
            glClear(GL_COLOR_BUFFER_BIT);
            glClear(GL_DEPTH_BUFFER_BIT);
            render_sky(&sky_attrib, player, sky_buffer);
            glClear(GL_DEPTH_BUFFER_BIT);
            render_players(&block_attrib, player);
            render_chunks(&block_attrib, player);
            render_bots(&block_attrib);
            render_mobs(&block_attrib);

            glClear(GL_DEPTH_BUFFER_BIT);

            // on marque les chunks a recalculer tous les jours
            if (water_changed == 1) {
                for (int i = 0; i < MAX_CHUNKS; i++) {
                    Chunk *chunk = g->chunks + i;
                    dirty_chunk(chunk);
                }
                water_changed = 0;
            }

            // vsync + maj events
            glfwSwapBuffers(g->window);
            glfwPollEvents();
            if (glfwWindowShouldClose(g->window)) {
                running = 0;
                break;
            }
            if (g->mode_changed) {
                g->mode_changed = 0;
                break;
            }
            if (gagne) {
                running = 0;
                break;
            }
            if (water >= 50) {
                printf("Perdu!\n");
                running = 0;
                break;
            }
        }

        del_buffer(sky_buffer);
        delete_all_chunks();
        delete_all_players();
    }

    glfwTerminate();
    return 0;
}
