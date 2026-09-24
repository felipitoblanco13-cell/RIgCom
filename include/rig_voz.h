#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"

typedef struct { float dopamina; float serotonina; float cortisol; float oxitocina; float coherencia; } RigVozNQ;
typedef struct { float f0; float Rd; float Tp, Tn; float f1, f2, f3, f4; float jitter; float shimmer; } RigVozTracto;

void rig_voz_evento(RigVozCtx *ctx, VozEvento ev, WsServer *srv);
void rig_voz_free(RigVozCtx *ctx);
void rig_voz_synth(RigVozCtx *ctx, int16_t *out_pcm);
