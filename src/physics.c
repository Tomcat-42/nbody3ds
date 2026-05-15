#include "physics.h"

#include <math.h>
#include <stdlib.h>

static constexpr float SOFTENING = 0.5f;
static constexpr float G_CONST = 1.0f;
static constexpr float DT = 0.016f;

static constexpr u32 BODY_COLORS[] = {
    0xFF4444FF, 0xFF44FF44, 0xFFFF4444, 0xFF44FFFF, 0xFFFF44FF, 0xFFFFFF44, 0xFFFF8844, 0xFF8844FF,
};

static float randf(float lo, float hi) {
  return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
}

static void compute_accelerations(struct Simulation *sim) {
  for (int i = 0; i < sim->count; i++) {
    sim->bodies[i].ax = 0.0f;
    sim->bodies[i].ay = 0.0f;
    sim->bodies[i].az = 0.0f;
  }

  for (int i = 0; i < sim->count; i++) {
    for (int j = i + 1; j < sim->count; j++) {
      float dx = sim->bodies[j].x - sim->bodies[i].x;
      float dy = sim->bodies[j].y - sim->bodies[i].y;
      float dz = sim->bodies[j].z - sim->bodies[i].z;

      float dist_sq = dx * dx + dy * dy + dz * dz + SOFTENING * SOFTENING;
      float inv_dist = 1.0f / sqrtf(dist_sq);
      float inv_dist3 = inv_dist * inv_dist * inv_dist;
      float force = G_CONST * inv_dist3;

      float fx = force * dx;
      float fy = force * dy;
      float fz = force * dz;

      sim->bodies[i].ax += fx * sim->bodies[j].mass;
      sim->bodies[i].ay += fy * sim->bodies[j].mass;
      sim->bodies[i].az += fz * sim->bodies[j].mass;

      sim->bodies[j].ax -= fx * sim->bodies[i].mass;
      sim->bodies[j].ay -= fy * sim->bodies[i].mass;
      sim->bodies[j].az -= fz * sim->bodies[i].mass;
    }
  }
}

static void spawn_orbiting_bodies(struct Simulation *sim, int start, int count) {
  float central_mass = sim->bodies[0].mass;

  for (int i = start; i < start + count && i < MAX_BODIES; i++) {
    float r = randf(5.0f, 15.0f);
    float angle = randf(0.0f, 2.0f * M_PI);
    float height = randf(-1.0f, 1.0f);

    struct Body *b = &sim->bodies[i];
    b->x = r * cosf(angle);
    b->y = height;
    b->z = r * sinf(angle);

    float orbital_v = sqrtf(G_CONST * central_mass / r);
    orbital_v *= randf(0.9f, 1.1f);

    b->vx = -orbital_v * sinf(angle);
    b->vy = 0.0f;
    b->vz = orbital_v * cosf(angle);

    b->ax = b->ay = b->az = 0.0f;
    b->mass = randf(0.1f, 1.0f);
    b->radius = 0.2f + 0.3f * (b->mass / 1.0f);
    b->color = BODY_COLORS[i % 8];

    sim->count = i + 1;
  }
}

void sim_init(struct Simulation *sim) {
  *sim = (struct Simulation){};
  srand(osGetTime());

  struct Body *central = &sim->bodies[0];
  *central = (struct Body){
      .mass = 100.0f,
      .radius = 1.0f,
      .color = 0xFFFFFFFF,
  };
  sim->count = 1;

  spawn_orbiting_bodies(sim, 1, 99);
  compute_accelerations(sim);
}

void sim_step(struct Simulation *sim) {
  for (int i = 0; i < sim->count; i++) {
    struct Body *b = &sim->bodies[i];
    b->vx += 0.5f * b->ax * DT;
    b->vy += 0.5f * b->ay * DT;
    b->vz += 0.5f * b->az * DT;
  }

  for (int i = 0; i < sim->count; i++) {
    struct Body *b = &sim->bodies[i];
    b->x += b->vx * DT;
    b->y += b->vy * DT;
    b->z += b->vz * DT;
  }

  compute_accelerations(sim);

  for (int i = 0; i < sim->count; i++) {
    struct Body *b = &sim->bodies[i];
    b->vx += 0.5f * b->ax * DT;
    b->vy += 0.5f * b->ay * DT;
    b->vz += 0.5f * b->az * DT;
  }
}

void sim_add_body(struct Simulation *sim) {
  if (sim->count >= MAX_BODIES)
    return;

  spawn_orbiting_bodies(sim, sim->count, 1);
}

void sim_add_bodies(struct Simulation *sim, int count) {
  int to_add = count;
  if (sim->count + to_add > MAX_BODIES)
    to_add = MAX_BODIES - sim->count;
  if (to_add <= 0)
    return;

  spawn_orbiting_bodies(sim, sim->count, to_add);
}

void sim_remove_bodies(struct Simulation *sim, int count) {
  sim->count -= count;
  if (sim->count < 1)
    sim->count = 1;
}

void sim_reset(struct Simulation *sim) {
  sim_init(sim);
}
