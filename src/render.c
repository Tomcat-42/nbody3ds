#include "render.h"

#include <math.h>

#include "vshader_shbin.h"

static constexpr u32 CLEAR_COLOR = 0x080810FF;

static constexpr u32 DISPLAY_TRANSFER_FLAGS =
    GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);

static constexpr int GLOW_SEGMENTS = 12;
static constexpr float INNER_RADIUS_FRAC = 0.6f;
static constexpr int VERTS_PER_BODY = GLOW_SEGMENTS * 9;
static constexpr int MAX_VERTICES = MAX_BODIES * VERTS_PER_BODY;
static constexpr float AMBIENT = 0.25f;
static constexpr float LIGHT_DX = 0.707f;
static constexpr float LIGHT_DY = 0.707f;

struct Vertex {
  float pos[3];
  float uv[2];
  float color[4];
};

static DVLB_s *shader_dvlb;
static shaderProgram_s program;
static s8 u_loc_projection, u_loc_model_view;
static C3D_Mtx projection;
static C3D_RenderTarget *target;
static struct Vertex *vbo_data;

static float sin_table[GLOW_SEGMENTS];
static float cos_table[GLOW_SEGMENTS];

void render_init(void) {
  for (int i = 0; i < GLOW_SEGMENTS; i++) {
    float angle = 2.0f * (float)M_PI * (float)i / GLOW_SEGMENTS;
    sin_table[i] = sinf(angle);
    cos_table[i] = cosf(angle);
  }

  target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
  C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

  shader_dvlb = DVLB_ParseFile((u32 *)vshader_shbin, vshader_shbin_size);
  shaderProgramInit(&program);
  shaderProgramSetVsh(&program, &shader_dvlb->DVLE[0]);
  C3D_BindProgram(&program);

  u_loc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
  u_loc_model_view = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");

  C3D_AttrInfo *attr_info = C3D_GetAttrInfo();
  AttrInfo_Init(attr_info);
  AttrInfo_AddLoader(attr_info, 0, GPU_FLOAT, 3);
  AttrInfo_AddLoader(attr_info, 1, GPU_FLOAT, 2);
  AttrInfo_AddLoader(attr_info, 2, GPU_FLOAT, 4);

  vbo_data = linearAlloc(MAX_VERTICES * sizeof(struct Vertex));

  C3D_BufInfo *buf_info = C3D_GetBufInfo();
  BufInfo_Init(buf_info);
  BufInfo_Add(buf_info, vbo_data, sizeof(struct Vertex), 3, 0x210);

  Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(60.0f), C3D_AspectRatioTop, 0.1f, 200.0f, false);
}

static void set_vertex(struct Vertex *v, float px, float py, float pz, float r, float g, float b,
                       float a) {
  *v = (struct Vertex){
      .pos = {px, py, pz},
      .color = {r, g, b, a},
  };
}

static float shade_for_angle(int seg) {
  float dot = cos_table[seg] * LIGHT_DX + sin_table[seg] * LIGHT_DY;
  return AMBIENT + (1.0f - AMBIENT) * (dot * 0.5f + 0.5f);
}

static void add_glow(struct Vertex *verts, int *idx, float cx, float cy, float cz, float radius,
                     const C3D_FVec *right, const C3D_FVec *up, float r, float g, float b,
                     float a) {
  float inner_r = radius * INNER_RADIUS_FRAC;

  float center_shade = AMBIENT + (1.0f - AMBIENT) * 0.8f;
  float cr = r * center_shade;
  float cg = g * center_shade;
  float cb = b * center_shade;

  for (int i = 0; i < GLOW_SEGMENTS; i++) {
    int next = (i + 1) % GLOW_SEGMENTS;

    float ex0 = cos_table[i] * right->x + sin_table[i] * up->x;
    float ey0 = cos_table[i] * right->y + sin_table[i] * up->y;
    float ez0 = cos_table[i] * right->z + sin_table[i] * up->z;

    float ex1 = cos_table[next] * right->x + sin_table[next] * up->x;
    float ey1 = cos_table[next] * right->y + sin_table[next] * up->y;
    float ez1 = cos_table[next] * right->z + sin_table[next] * up->z;

    float s0 = shade_for_angle(i);
    float s1 = shade_for_angle(next);

    float r0 = r * s0, g0 = g * s0, b0 = b * s0;
    float r1 = r * s1, g1 = g * s1, b1 = b * s1;

    set_vertex(&verts[(*idx)++], cx, cy, cz, cr, cg, cb, a);
    set_vertex(&verts[(*idx)++], cx + ex0 * inner_r, cy + ey0 * inner_r, cz + ez0 * inner_r, r0, g0,
               b0, a);
    set_vertex(&verts[(*idx)++], cx + ex1 * inner_r, cy + ey1 * inner_r, cz + ez1 * inner_r, r1, g1,
               b1, a);

    set_vertex(&verts[(*idx)++], cx + ex0 * inner_r, cy + ey0 * inner_r, cz + ez0 * inner_r, r0, g0,
               b0, a);
    set_vertex(&verts[(*idx)++], cx + ex0 * radius, cy + ey0 * radius, cz + ez0 * radius, r0, g0,
               b0, 0.0f);
    set_vertex(&verts[(*idx)++], cx + ex1 * radius, cy + ey1 * radius, cz + ez1 * radius, r1, g1,
               b1, 0.0f);

    set_vertex(&verts[(*idx)++], cx + ex0 * inner_r, cy + ey0 * inner_r, cz + ez0 * inner_r, r0, g0,
               b0, a);
    set_vertex(&verts[(*idx)++], cx + ex1 * radius, cy + ey1 * radius, cz + ez1 * radius, r1, g1,
               b1, 0.0f);
    set_vertex(&verts[(*idx)++], cx + ex1 * inner_r, cy + ey1 * inner_r, cz + ez1 * inner_r, r1, g1,
               b1, a);
  }
}

void render_frame(const struct Simulation *sim, const struct Camera *cam) {
  C3D_FVec right, up;
  camera_get_right_up(cam, &right, &up);

  int vert_count = 0;
  for (int i = 0; i < sim->count; i++) {
    const struct Body *b = &sim->bodies[i];

    float r = (float)((b->color >> 0) & 0xFF) / 255.0f;
    float g = (float)((b->color >> 8) & 0xFF) / 255.0f;
    float bl = (float)((b->color >> 16) & 0xFF) / 255.0f;
    float a = (float)((b->color >> 24) & 0xFF) / 255.0f;

    add_glow(vbo_data, &vert_count, b->x, b->y, b->z, b->radius, &right, &up, r, g, bl, a);
  }

  GSPGPU_FlushDataCache(vbo_data, vert_count * sizeof(struct Vertex));

  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

  C3D_RenderTargetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
  C3D_FrameDrawOn(target);

  C3D_BindProgram(&program);

  C3D_Mtx view;
  camera_get_view(cam, &view);

  C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, u_loc_projection, &projection);
  C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, u_loc_model_view, &view);

  C3D_TexEnv *env = C3D_GetTexEnv(0);
  C3D_TexEnvInit(env);
  C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
  C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

  C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
                 GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
  C3D_DepthTest(true, GPU_ALWAYS, GPU_WRITE_COLOR);
  C3D_CullFace(GPU_CULL_NONE);

  C3D_DrawArrays(GPU_TRIANGLES, 0, vert_count);

  C3D_FrameEnd(0);
}

void render_fini(void) {
  linearFree(vbo_data);
  shaderProgramFree(&program);
  DVLB_Free(shader_dvlb);
}
