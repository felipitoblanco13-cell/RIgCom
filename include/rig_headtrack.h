#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"


float rig_oneeuro_step(RigOneEuro *f, float x, float dt);
int rig_headtrack_default_cal(RigScreenCal *c);
int rig_headtrack_init(RigObserver *o);
int rig_headtrack_lost(RigObserver *o, const RigScreenCal *cal, float dt);
int rig_headtrack_update(RigObserver *o, const RigScreenCal *cal, const float eyeL_px[2], const float eyeR_px[2], float dt);
int rig_offaxis_model_scale(float s, float out[16]);
int rig_offaxis_projection(const RigScreenCal *cal, const float eye_mm[3], float near_mm, float far_mm, float out[16]);
int rig_offaxis_view(const float eye_mm[3], float out[16]);
void rig_oneeuro_init(RigOneEuro *f, float min_cutoff, float beta, float d_cutoff);
