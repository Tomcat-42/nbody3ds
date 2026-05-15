#pragma once

#include <3ds.h>

static constexpr int MAX_BODIES = 256;

struct Body {
  float x, y, z;
  float vx, vy, vz;
  float ax, ay, az;
  float mass;
  float radius;
  u32 color;
};

struct Simulation {
  struct Body bodies[MAX_BODIES];
  int count;
};

void sim_init(struct Simulation *sim);
void sim_step(struct Simulation *sim);
void sim_add_body(struct Simulation *sim);
void sim_add_bodies(struct Simulation *sim, int count);
void sim_remove_bodies(struct Simulation *sim, int count);
void sim_reset(struct Simulation *sim);
