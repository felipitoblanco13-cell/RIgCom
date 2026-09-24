#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"


float rig_h_brushed(const float p[3], float freq, const float dir[3]);
float rig_h_crackle(const float p[3], float freq);
float rig_h_fbm(const float p[3], float freq);
float rig_h_fiber(const float p[3], float freq, const float dir[3]);
float rig_h_hammered(const float p[3], float freq);
float rig_h_knurl(const float p[3], float freq, const float dir[3]);
float rig_h_pores(const float p[3], float freq);
float rig_h_scales(const float p[3], float freq);
float rig_h_weave(const float p[3], float freq, const float dir[3]);
float rig_height3d(const float p[3], RigHeightFn fn, float freq, const float dir[3]);
float rig_height3d_local_roughness(const float p[3], RigHeightFn fn, float freq, float radius);
float rig_impasto_height(const RigImpastoField *f, float x, float y);
int rig_impasto_add(RigImpastoField *f, float x, float y, float r, float h, float visc);
int rig_impasto_init(RigImpastoField *f);
void rig_height3d_normal(const float p[3], RigHeightFn fn, float freq, const float dir[3], float eps, float out_n[3]);
