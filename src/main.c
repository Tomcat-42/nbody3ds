#include <3ds.h>

#include <citro3d.h>

#include <stdio.h>

#include "camera.h"
#include "physics.h"
#include "render.h"

static struct Simulation sim;
static struct Camera cam;
static PrintConsole bottom_console;

int main(void) {
  gfxInitDefault();
  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

  consoleInit(GFX_BOTTOM, &bottom_console);

  render_init();
  camera_init(&cam);
  sim_init(&sim);

  u64 last_time = osGetTime();

  while (aptMainLoop()) {
    hidScanInput();
    u32 k_down = hidKeysDown();

    if (k_down & KEY_START)
      break;
    if (k_down & KEY_A)
      sim_add_body(&sim);
    if (k_down & KEY_B)
      sim_reset(&sim);
    if (k_down & KEY_DUP)
      sim_add_bodies(&sim, 10);
    if (k_down & KEY_DDOWN)
      sim_remove_bodies(&sim, 10);
    if (k_down & KEY_DRIGHT)
      sim_add_bodies(&sim, 50);
    if (k_down & KEY_DLEFT)
      sim_remove_bodies(&sim, 50);

    camera_update(&cam);
    sim_step(&sim);
    render_frame(&sim, &cam);

    u64 now = osGetTime();
    float fps = 1000.0f / (float)(now - last_time + 1);
    last_time = now;

    consoleSelect(&bottom_console);
    printf("\x1b[0;0H");
    printf(" N-Body 3DS Simulation\n");
    printf(" ---------------------\n");
    printf(" Bodies: %d/%d\n", sim.count, MAX_BODIES);
    printf(" FPS:    %.1f\n", fps);
    printf("\n");
    printf(" Controls:\n");
    printf(" Circle Pad  - Camera orbit\n");
    printf(" L/R         - Zoom\n");
    printf(" A           - Add 1 body\n");
    printf(" D-Up/Down   - +/- 10 bodies\n");
    printf(" D-Left/Right- +/- 50 bodies\n");
    printf(" B           - Reset sim\n");
    printf(" START       - Exit\n");
  }

  render_fini();
  C3D_Fini();
  gfxExit();
  return 0;
}
