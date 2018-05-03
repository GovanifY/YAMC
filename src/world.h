#ifndef _world_h_
#define _world_h_

typedef void (*world_func)(int, int, int, int, void *);

void create_world(int p, int q, world_func func, void *arg, int t, int food,
                  int trees);

float get_daylight();
float time_of_day();

#endif
