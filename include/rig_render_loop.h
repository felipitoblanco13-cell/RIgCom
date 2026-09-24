#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"


RLMaterial* rig_rl_material_get__rig_dup_71321899(RigRenderLoop *rl, uint32_t id);
RLNode* rig_rl_node_create__rig_variant_e3e4c7c3(RigRenderLoop *rl, RLNodeType type, const char *name);
const RLFrameProfile* rig_rl_profile_current__rig_dup_95d9bae4(const RigRenderLoop *rl);
int rig_rl_init__rig_variant_c4623aee(RigRenderLoop *rl, RigGPUCtx *gpu, RigDisplayCtx *disp, uint32_t w, uint32_t h);
int rig_rl_load_hdri__rig_variant_d3c624da(RigRenderLoop *rl, const char *path);
int rig_rl_run__rig_dup_6863ea5a(RigRenderLoop *rl);
int rig_rl_run_thread__rig_variant_e7e96a8c(RigRenderLoop *rl);
int rig_rl_tick__rig_dup_2c36ac07(RigRenderLoop *rl);
uint32_t rig_rl_cull_frustum__rig_dup_6b0a37a8(RigRenderLoop *rl, RLNode *camera);
uint32_t rig_rl_material_create__rig_variant_f37d3e42(RigRenderLoop *rl, const char *name);
uint8_t rig_rl_light_add__rig_variant_5741dd3c(RigRenderLoop *rl, RLLightType type);
void rig_rl_destroy__rig_variant_8466b33c(RigRenderLoop *rl);
void rig_rl_geometry_pass__rig_variant_1a45db9c(RigRenderLoop *rl);
void rig_rl_light_remove__rig_variant_4fe42ee3(RigRenderLoop *rl, uint8_t idx);
void rig_rl_lighting_pass__rig_variant_d8768cb5(RigRenderLoop *rl);
void rig_rl_material_destroy__rig_variant_7b6b84a3(RigRenderLoop *rl, uint32_t id);
void rig_rl_node_attach__rig_variant_a2a13335(RLNode *parent, RLNode *child);
void rig_rl_node_destroy__rig_variant_23894f71(RigRenderLoop *rl, uint32_t node_id);
void rig_rl_node_detach__rig_variant_163e74cf(RLNode *node);
void rig_rl_node_look_at__rig_variant_cce82672(RLNode *n, float tx, float ty, float tz);
void rig_rl_node_rotate__rig_dup_7f26af5e(RLNode *n, float ax, float ay, float az, float angle_rad);
void rig_rl_node_scale__rig_dup_339ae909(RLNode *n, float x, float y, float z);
void rig_rl_node_translate__rig_dup_0d0b5a83(RLNode *n, float x, float y, float z);
void rig_rl_pause__rig_dup_8e65f851(RigRenderLoop *rl);
void rig_rl_post_default_cinema__rig_variant_64234fb3(RLPostConfig *cfg);
void rig_rl_post_default_clean__rig_variant_a7eba73d(RLPostConfig *cfg);
void rig_rl_post_default_rigart__rig_variant_402d211c(RLPostConfig *cfg);
void rig_rl_post_pass__rig_variant_d14552a2(RigRenderLoop *rl);
void rig_rl_profile_emit_json__rig_variant_3d96a41d(const RigRenderLoop *rl, char *out, size_t sz);
void rig_rl_resume__rig_dup_b853fdcd(RigRenderLoop *rl);
void rig_rl_set_hdri__rig_variant_a00bd1a9(RigRenderLoop *rl, uint32_t tex_id, float rot, float intensity);
void rig_rl_shadow_pass__rig_variant_8fde7db3(RigRenderLoop *rl);
void rig_rl_stop__rig_variant_9ac65b77(RigRenderLoop *rl);
void rig_rl_studio_setup__rig_dup_9cb5221f(RigRenderLoop *rl, float r, float h);
void rig_rl_update_transforms__rig_dup_4b302e19(RigRenderLoop *rl, RLNode *root);
