#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"

typedef struct { const char *name; float alb[3]; float metallic, roughness, ior; float aniso; RigTangentFlow flow; float sheen[3], sheen_r; float trans, absorb[3], thick, disp; float coat, coat_r; float iri, iri_nm; RigHeightFn hfn; float hscale, hfreq; float sss; } RigPreset;

float rig_hfn_spatial_freq(RigHeightFn fn);
float rig_hfn_tactile_noise(RigHeightFn fn);
int rig_material_default(RigMaterial *m);
int rig_material_derive_haptics(RigMaterial *m);
int rig_material_pack_gpu(const RigMaterial *m, float out[32]);
int rig_material_preset(RigMaterial *m, uint32_t index);
uint32_t rig_material_preset_count(void);
