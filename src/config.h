#ifndef _config_h_
#define _config_h_

// app parameters
#define FULLSCREEN 0
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define VSYNC 1
#define SCROLL_THRESHOLD 0.1
#define DAY_LENGTH 50
#define INVERT_MOUSE 0

// rendering options
#define SHOW_LIGHTS 1
#define SHOW_ITEM 0

// key bindings
#define CRAFT_KEY_FORWARD 'W'
#define CRAFT_KEY_BACKWARD 'S'
#define CRAFT_KEY_LEFT 'A'
#define CRAFT_KEY_RIGHT 'D'
#define CRAFT_KEY_JUMP GLFW_KEY_SPACE
#define CRAFT_KEY_FLY GLFW_KEY_TAB
#define CRAFT_KEY_ITEM_NEXT 'E'
#define CRAFT_KEY_ITEM_PREV 'R'
#define CRAFT_KEY_ZOOM GLFW_KEY_LEFT_SHIFT
#define CRAFT_KEY_ORTHO 'F'

// advanced parameters
#define CREATE_CHUNK_RADIUS 1
#define RENDER_CHUNK_RADIUS 1
#define DELETE_CHUNK_RADIUS 14
#define CHUNK_SIZE 64

#define MAX_WOOD 100
#define MAX_FOOD 200
#define WANTED_WOOD 1
#endif
