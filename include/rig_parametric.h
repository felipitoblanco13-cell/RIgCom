#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"


float rig_parametric_envelope(RigParametric *p, float s);
float rig_parametric_gain_for(float f);
float rig_parametric_preemphasis(RigParametric *p, float x);
float rig_parametric_spl(float P1, float f_mod, float S, float z);
int rig_friction_init(RigFrictionState *s, uint32_t seed);
int rig_friction_render(RigFrictionState *s, const RigHapticSig *sig, float speed, float sr, float *out, uint32_t n);
int rig_parametric_check(const RigHapticSig *sig, float carrier_pa, RigTriSenseCheck *out);
int rig_parametric_encode(RigParametric *p, RigFrictionState *fs, const RigHapticSig *sig, float speed, float *out, uint32_t n);
int rig_parametric_init(RigParametric *p, float sr, float max_pa);
