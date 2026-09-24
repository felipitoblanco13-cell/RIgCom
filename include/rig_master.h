#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"


int rig_master_add_light(RigMasterCtx *m, const RigLight *l);
int rig_master_art_clear(RigMasterCtx *m);
int rig_master_art_stroke(RigMasterCtx *m, float x, float y, float radius, float thickness_mm, float viscosity);
int rig_master_enable_aom(RigMasterCtx *m, float rf_power_w);
int rig_master_face_material(RigMasterCtx *m, uint32_t id, float melanin, float hemoglobin, float carotene, float age);
int rig_master_frame(RigMasterCtx *m, float dt);
int rig_master_init(RigMasterCtx *m, uint32_t w, uint32_t h);
int rig_master_load_presets(RigMasterCtx *m);
int rig_master_matrices(const RigMasterCtx *m, float proj[16], float view[16]);
int rig_master_mesh_to_volume(RigMasterCtx *m, const RigMesh *mesh, uint32_t density, AOMPoint *out, uint32_t max_points, uint32_t *out_count);
int rig_master_probe_ray(RigMasterCtx *m, const float ro_mm[3], const float rd[3], float speed, float dt, RigProbe *out);
int rig_master_probe_screen(RigMasterCtx *m, float sx, float sy, float speed, float dt, RigProbe *out);
int rig_master_selftest(RigMasterCtx *m, RigMasterAudit *A);
int rig_master_set_env(RigMasterCtx *m, RigEnvPreset p, uint32_t cube_size, uint32_t samples);
int rig_master_set_material(RigMasterCtx *m, uint32_t id, const RigMaterial *mat);
int rig_master_set_observer(RigMasterCtx *m, const float eye_mm[3]);
int rig_master_submit_mesh(RigMasterCtx *m, const RigMesh *mesh);
int rig_master_submit_volume(RigMasterCtx *m, AOMPoint *pts, uint32_t n, float x, float y, float z);
int rig_master_track_eyes(RigMasterCtx *m, const float eyeL[2], const float eyeR[2], float dt);
void rig_master_clear_meshes(RigMasterCtx *m);
void rig_master_destroy(RigMasterCtx *m);
