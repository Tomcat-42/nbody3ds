#pragma once

#include <citro3d.h>

#include "camera.h"
#include "physics.h"

void render_init(void);
void render_frame(const struct Simulation *sim, const struct Camera *cam);
void render_fini(void);
