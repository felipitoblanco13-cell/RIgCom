#pragma once
/* GEN — rig_math.h reconstruido: reexporta la matemática del ecosistema. */
#include "../riglib_math.h"

#ifndef RIG_PI
#define RIG_PI 3.14159265358979323846f
#endif
#ifndef RIG_TAU
#define RIG_TAU 6.28318530717958647692f
#endif

typedef struct { union { struct { float u, v; }; struct { float x, y; }; }; } RigUV;
