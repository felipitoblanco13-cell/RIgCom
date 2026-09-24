#pragma once
/* GEN — rigart_v4_art.h reconstruido: tipos públicos del módulo rigart v4. */
#include "rig_face_ng.h"
#include "rig_face_v2_bridge.h"

#define RIG_MAX_LAYERS 16
#ifndef RIG_SHADER_MAXBUF
#define RIG_SHADER_MAXBUF 262144
#endif

typedef enum {
    BLEND_NORMAL = 0,
    BLEND_MULTIPLY,
    BLEND_SCREEN,
    BLEND_OVERLAY,
    BLEND_DARKEN,
    BLEND_LIGHTEN,
    BLEND_COLOR,
    BLEND_ADD,
    BLEND_DIFFERENCE,
    BLEND_EXCLUSION,
    BLEND_SOFT
} RigBlendMode;

typedef struct {
    char          name[64];
    RigBlendMode  blend_mode;
    float         opacity;
    bool          visible;
    bool          locked;
} RigLayerDesc;

typedef struct {
    uint32_t      width;
    uint32_t      height;
    uint32_t      layer_count;
    bool          hdr_p3_enabled;
    bool          enable_16bit;
    float         dpi;
    float         grid_size_phi;
    RigLayerDesc  layers[RIG_MAX_LAYERS];
} RigArtCanvasCtx;

void rigart_v4_init_result__rig_variant_2c6804ec(RigArtResultV4 *res);
void rigart_v4_free_result__rig_variant_42c478c7(RigArtResultV4 *res);
int rigart_art_canvas_gen__rig_variant_e9720543(const RigArtCanvasCtx *ctx, RigArtResultV4 *out);
int rigart_art_compositor__rig_variant_66d1e991(const RigArtCompositorCtx *ctx, RigArtResultV4 *out);
