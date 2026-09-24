#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"


int rig_haptic_couple(const AOMEngine *aom, const RigMaterial *mats, uint32_t mat_count, const float hand_mm[3], float finger_speed_mm_s, float dt, RigHapticOut *out);
