#include "camera.h"

#include <3ds.h>

#include <math.h>

static constexpr float CAM_SPEED = 0.03f;
static constexpr float ZOOM_SPEED = 0.5f;
static constexpr float MIN_DISTANCE = 5.0f;
static constexpr float MAX_DISTANCE = 80.0f;
static constexpr float PHI_MIN = 0.1f;

void camera_init(struct Camera *cam) {
  *cam = (struct Camera){
      .phi = M_PI / 3.0f,
      .distance = 30.0f,
  };
}

void camera_update(struct Camera *cam) {
  circlePosition cpad = {};
  hidCircleRead(&cpad);

  float dx = (float)cpad.dx / 150.0f;
  float dy = (float)cpad.dy / 150.0f;

  if (fabsf(dx) > 0.1f)
    cam->theta += dx * CAM_SPEED;
  if (fabsf(dy) > 0.1f)
    cam->phi -= dy * CAM_SPEED;

  if (cam->phi < PHI_MIN)
    cam->phi = PHI_MIN;
  float phi_max = (float)M_PI - 0.1f;
  if (cam->phi > phi_max)
    cam->phi = phi_max;

  u32 held = hidKeysHeld();
  if (held & KEY_L)
    cam->distance -= ZOOM_SPEED;
  if (held & KEY_R)
    cam->distance += ZOOM_SPEED;

  if (cam->distance < MIN_DISTANCE)
    cam->distance = MIN_DISTANCE;
  if (cam->distance > MAX_DISTANCE)
    cam->distance = MAX_DISTANCE;
}

void camera_get_view(const struct Camera *cam, C3D_Mtx *view) {
  float cx = cam->distance * sinf(cam->phi) * cosf(cam->theta);
  float cy = cam->distance * cosf(cam->phi);
  float cz = cam->distance * sinf(cam->phi) * sinf(cam->theta);

  C3D_FVec eye = FVec3_New(cam->target_x + cx, cam->target_y + cy, cam->target_z + cz);
  C3D_FVec target = FVec3_New(cam->target_x, cam->target_y, cam->target_z);
  C3D_FVec up = FVec3_New(0.0f, 1.0f, 0.0f);

  Mtx_LookAt(view, eye, target, up, false);
}

void camera_get_right_up(const struct Camera *cam, C3D_FVec *right, C3D_FVec *up) {
  C3D_Mtx view;
  camera_get_view(cam, &view);

  right->x = view.r[0].x;
  right->y = view.r[0].y;
  right->z = view.r[0].z;
  right->w = 0.0f;

  up->x = view.r[1].x;
  up->y = view.r[1].y;
  up->z = view.r[1].z;
  up->w = 0.0f;
}
