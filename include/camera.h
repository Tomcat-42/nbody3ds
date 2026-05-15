#pragma once

#include <c3d/maths.h>
#include <c3d/types.h>

struct Camera {
  float theta;
  float phi;
  float distance;
  float target_x, target_y, target_z;
};

void camera_init(struct Camera *cam);
void camera_update(struct Camera *cam);
void camera_get_view(const struct Camera *cam, C3D_Mtx *view);
void camera_get_right_up(const struct Camera *cam, C3D_FVec *right, C3D_FVec *up);
