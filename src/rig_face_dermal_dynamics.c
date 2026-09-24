/* ============================================================================
 * rig_face_dermal_dynamics.c
 *
 * RigCom :: Dermal Dynamics Module (amplitud de capacidades, aditivo puro)
 *
 * Extiende la bioquimica ESTATICA de piel (melanin/hemoglobin/carotene ya
 * existentes en engine_v2.c / ng_skin.c) con:
 *   1. Estado reactivo en el tiempo (rubor, palidez, sudor, piloereccion)
 *   2. Heridas/cicatrices procedurales con progresion temporal real
 *   3. Rasterizado real de heridas a un atlas RGBA8 (no solo eval puntual)
 *   4. Generacion de codigo GLSL (fragment) que aplica los deltas en shader,
 *      en el mismo estilo de rig_face_ng_skin_shader (uniforms + snippet)
 *   5. Runtime JS que actualiza los uniforms cada frame desde el estado C
 *   6. Serializacion binaria (save/load) del estado completo, con magic+version
 *
 * No modifica ningun archivo existente del motor.
 * ==========================================================================*/
#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif
/* [CANON] <math.h> → "rig_math.h" */
#include "rig_math.h"
/* [CANON] <string.h> → "rig_noext_str.h" */
#include "rig_noext_str.h"
/* [CANON] <stdlib.h> → "rig_noext_mem.h" */
#include "rig_noext_mem.h"
/* [CANON] <stdio.h> → "rig_noext_io.h" */
#include "rig_noext_io.h"
#define RIG_DERMAL_MAX_WOUNDS    32
#define RIG_DERMAL_MAX_REGIONS   16
#define RIG_DERMAL_MAGIC         0x44455231u /* "DER1" */
#define RIG_DERMAL_VERSION       1
typedef enum {
    RIG_WOUND_FRESH = 0,
    RIG_WOUND_CLOT,
    RIG_WOUND_GRANULATING,
    RIG_WOUND_SCAR_YOUNG,
    RIG_WOUND_SCAR_MATURE
} RigWoundStage;
typedef struct {
    float u, v;
    float radius_mm;
    float depth_mm;
    float age_days;
    float keloid_bias;
    int   active;
} RigDermalWound;
typedef struct {
    float sweat_density[RIG_DERMAL_MAX_REGIONS];
    float piloerection_density[RIG_DERMAL_MAX_REGIONS];
} RigDermalRegionMap;
typedef struct {
    float emotion_arousal;
    float emotion_valence;
    float ambient_temp_c;
    float core_temp_c;
    float exertion;
    float blush_level;
    float pallor_level;
    float sweat_level;
    float goosebump_level;
    RigDermalRegionMap region_map;
    RigDermalWound wounds[RIG_DERMAL_MAX_WOUNDS];
    int wound_count;
} RigDermalState;
/* Resultado de generacion de codigo, mismo patron que RigArtResultNG:
 * strings de tamaño fijo generoso, propiedad del caller (sin malloc oculto). */
#define RIG_DERMAL_SRC_MAX 16384
typedef struct {
    char glsl_fragment_snippet[RIG_DERMAL_SRC_MAX];
    char js_runtime_snippet[RIG_DERMAL_SRC_MAX];
    int  glsl_len;
    int  js_len;
} RigDermalArtResult;
/* -------------------------------------------------------------------------
 * Utilidades numericas
 * ---------------------------------------------------------------------- */
static float rig_dermal_clamp01__rig_dup_b0fbec52(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}
static float rig_dermal_lerp__rig_dup_2b396547(float a, float b, float t) {
    return a + (b - a) * t;
}
static float rig_dermal_approach__rig_dup_4f13e1a4(float current, float target, float dt, float tau) {
    if (tau <= 0.0001f) return target;
    float alpha = 1.0f - expf(-dt / tau);
    return current + (target - current) * alpha;
}
/* -------------------------------------------------------------------------
 * Inicializacion
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_dermal_state_init__rig_dup_11050991(RigDermalState *st) {
    if (!st) return -1;
    memset(st, 0, sizeof(*st));
    st->core_temp_c = 37.0f;
    st->ambient_temp_c = 22.0f;
    /* region 0=frente 1=mejillas 2=nariz 3=menton 4=cuello 5=orejas
     * 6=palmas 7=axilas 8=espalda 9=pecho 10=antebrazo
     * 11=cuero_cabelludo 12=labio_sup 13=parpados 14=hombros 15=torso */
    const float sweat_defaults[RIG_DERMAL_MAX_REGIONS] = {
        0.65f, 0.35f, 0.30f, 0.30f, 0.25f, 0.10f,
        0.90f, 0.85f, 0.40f, 0.35f, 0.30f,
        0.70f, 0.55f, 0.15f, 0.30f, 0.35f
    };
    const float pilo_defaults[RIG_DERMAL_MAX_REGIONS] = {
        0.20f, 0.15f, 0.10f, 0.15f, 0.30f, 0.10f,
        0.05f, 0.20f, 0.50f, 0.45f, 0.55f,
        0.05f, 0.05f, 0.05f, 0.45f, 0.50f
    };
    memcpy(st->region_map.sweat_density, sweat_defaults, sizeof(sweat_defaults));
    memcpy(st->region_map.piloerection_density, pilo_defaults, sizeof(pilo_defaults));
    return 0;
}
/* -------------------------------------------------------------------------
 * Simulacion temporal
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_dermal_update__rig_dup_87e0a536(RigDermalState *st, float dt) {
    if (!st || dt <= 0.0f) return -1;
    float shame_component = (st->emotion_valence < 0.0f) ? -st->emotion_valence : 0.0f;
    float arousal_pos     = (st->emotion_arousal > 0.0f) ? st->emotion_arousal : 0.0f;
    float blush_target = rig_dermal_clamp01__rig_dup_b0fbec52(0.6f * shame_component + 0.5f * arousal_pos
                          + 0.08f * (st->core_temp_c - 37.0f));
    int shock_mode = (st->emotion_arousal > 0.7f && st->emotion_valence < -0.5f);
    float pallor_target;
    if (shock_mode) {
        pallor_target = rig_dermal_clamp01__rig_dup_b0fbec52(0.5f + 0.5f * st->emotion_arousal);
    } else {
        float cold_component = rig_dermal_clamp01__rig_dup_b0fbec52((15.0f - st->ambient_temp_c) / 20.0f);
        pallor_target = rig_dermal_clamp01__rig_dup_b0fbec52(cold_component * 0.7f);
    }
    float heat_component = rig_dermal_clamp01__rig_dup_b0fbec52((st->ambient_temp_c - 26.0f) / 12.0f);
    float sweat_target = rig_dermal_clamp01__rig_dup_b0fbec52(0.7f * heat_component + 0.6f * st->exertion
                          + 0.25f * arousal_pos);
    float cold_shiver = rig_dermal_clamp01__rig_dup_b0fbec52((12.0f - st->ambient_temp_c) / 15.0f);
    float fear_chill   = shock_mode ? 0.6f : 0.0f;
    float goosebump_target = rig_dermal_clamp01__rig_dup_b0fbec52(cold_shiver + fear_chill - 0.5f * sweat_target);
    float blush_tau  = (blush_target  > st->blush_level)  ? 4.0f  : 12.0f;
    float sweat_tau  = (sweat_target  > st->sweat_level)  ? 20.0f : 60.0f;
    float pallor_tau = (pallor_target > st->pallor_level) ? 3.0f  : 15.0f;
    float goose_tau  = 1.5f;
    st->blush_level     = rig_dermal_approach__rig_dup_4f13e1a4(st->blush_level,     blush_target,     dt, blush_tau);
    st->pallor_level    = rig_dermal_approach__rig_dup_4f13e1a4(st->pallor_level,    pallor_target,    dt, pallor_tau);
    st->sweat_level     = rig_dermal_approach__rig_dup_4f13e1a4(st->sweat_level,     sweat_target,     dt, sweat_tau);
    st->goosebump_level = rig_dermal_approach__rig_dup_4f13e1a4(st->goosebump_level, goosebump_target, dt, goose_tau);
    for (int i = 0; i < st->wound_count; i++) {
        if (st->wounds[i].active) st->wounds[i].age_days += dt / 86400.0f;
    }
    return 0;
}
static RigWoundStage rig_dermal_wound_stage__rig_dup_4e20bea5(const RigDermalWound *w) {
    if (w->age_days < 1.0f)   return RIG_WOUND_FRESH;
    if (w->age_days < 4.0f)   return RIG_WOUND_CLOT;
    if (w->age_days < 14.0f)  return RIG_WOUND_GRANULATING;
    if (w->age_days < 180.0f) return RIG_WOUND_SCAR_YOUNG;
    return RIG_WOUND_SCAR_MATURE;
}
RIGCOM_PUBLIC int rig_dermal_wound_add__rig_dup_13fdb5bf(RigDermalState *st, float u, float v,
                                        float radius_mm, float depth_mm,
                                        float keloid_bias) {
    if (!st) return -1;
    if (st->wound_count >= RIG_DERMAL_MAX_WOUNDS) return -2;
    RigDermalWound *w = &st->wounds[st->wound_count++];
    w->u = u; w->v = v;
    w->radius_mm = radius_mm;
    w->depth_mm = depth_mm;
    w->age_days = 0.0f;
    w->keloid_bias = rig_dermal_clamp01__rig_dup_b0fbec52(keloid_bias);
    w->active = 1;
    return st->wound_count - 1;
}
RIGCOM_PUBLIC int rig_dermal_wound_eval__rig_dup_2bc4b896(const RigDermalWound *w,
                                         float *out_color_delta_rgb,
                                         float *out_height_mm) {
    if (!w || !out_color_delta_rgb || !out_height_mm) return -1;
    RigWoundStage stage = rig_dermal_wound_stage__rig_dup_4e20bea5(w);
    switch (stage) {
        case RIG_WOUND_FRESH:
            out_color_delta_rgb[0] = 0.55f; out_color_delta_rgb[1] = 0.02f; out_color_delta_rgb[2] = 0.02f;
            *out_height_mm = 0.6f * w->depth_mm;
            break;
        case RIG_WOUND_CLOT:
            out_color_delta_rgb[0] = 0.25f; out_color_delta_rgb[1] = 0.06f; out_color_delta_rgb[2] = 0.03f;
            *out_height_mm = 0.8f * w->depth_mm;
            break;
        case RIG_WOUND_GRANULATING: {
            float t = (w->age_days - 4.0f) / 10.0f;
            out_color_delta_rgb[0] = rig_dermal_lerp__rig_dup_2b396547(0.35f, 0.18f, t);
            out_color_delta_rgb[1] = rig_dermal_lerp__rig_dup_2b396547(0.08f, 0.06f, t);
            out_color_delta_rgb[2] = rig_dermal_lerp__rig_dup_2b396547(0.08f, 0.06f, t);
            *out_height_mm = rig_dermal_lerp__rig_dup_2b396547(0.5f, 0.15f, t) * w->depth_mm;
            break;
        }
        case RIG_WOUND_SCAR_YOUNG:
            out_color_delta_rgb[0] = 0.20f; out_color_delta_rgb[1] = 0.08f; out_color_delta_rgb[2] = 0.09f;
            *out_height_mm = 0.10f * w->depth_mm * (0.5f + w->keloid_bias);
            break;
        case RIG_WOUND_SCAR_MATURE:
        default: {
            float hyper = w->keloid_bias;
            out_color_delta_rgb[0] = rig_dermal_lerp__rig_dup_2b396547(-0.15f, 0.05f, hyper);
            out_color_delta_rgb[1] = rig_dermal_lerp__rig_dup_2b396547(-0.10f, -0.02f, hyper);
            out_color_delta_rgb[2] = rig_dermal_lerp__rig_dup_2b396547(-0.10f, -0.02f, hyper);
            *out_height_mm = 0.05f * w->depth_mm * hyper;
            break;
        }
    }
    return (int)stage;
}
RIGCOM_PUBLIC int rig_dermal_region_color_delta__rig_dup_8e9c13a5(const RigDermalState *st, int region_idx,
                                                 float *out_rgb) {
    if (!st || !out_rgb || region_idx < 0 || region_idx >= RIG_DERMAL_MAX_REGIONS) return -1;
    float blush = st->blush_level;
    float pallor = st->pallor_level;
    out_rgb[0] =  0.35f * blush - 0.20f * pallor;
    out_rgb[1] = -0.05f * blush - 0.05f * pallor;
    out_rgb[2] = -0.05f * blush + 0.03f * pallor;
    return 0;
}
RIGCOM_PUBLIC int rig_dermal_region_surface__rig_dup_45e90d7d(const RigDermalState *st, int region_idx,
                                             float *out_wetness, float *out_goosebump) {
    if (!st || !out_wetness || !out_goosebump || region_idx < 0 || region_idx >= RIG_DERMAL_MAX_REGIONS) return -1;
    *out_wetness   = rig_dermal_clamp01__rig_dup_b0fbec52(st->sweat_level * st->region_map.sweat_density[region_idx]);
    *out_goosebump = rig_dermal_clamp01__rig_dup_b0fbec52(st->goosebump_level * st->region_map.piloerection_density[region_idx]);
    return 0;
}
/* -------------------------------------------------------------------------
 * Rasterizado real de heridas a un atlas RGBA8 (no evaluacion puntual: esto
 * ESCRIBE pixeles). Formato: canal R/G/B = delta de color con offset +128
 * (signed remap), canal A = altura en mm * 100 (0..255 -> 0..2.55mm),
 * compatible como layer adicional del atlas empaquetado por el baker.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_dermal_wounds_rasterize__rig_dup_a053fca0(const RigDermalState *st,
                                               unsigned char *atlas_rgba8,
                                               int atlas_w, int atlas_h) {
    if (!st || !atlas_rgba8 || atlas_w <= 0 || atlas_h <= 0) return -1;
    for (int i = 0; i < st->wound_count; i++) {
        const RigDermalWound *w = &st->wounds[i];
        if (!w->active) continue;
        float color_delta[3];
        float height_mm;
        rig_dermal_wound_eval__rig_dup_2bc4b896(w, color_delta, &height_mm);
        /* radio en UV: se asume 1 unidad UV ~ 180mm de recorrido facial,
         * consistente con el rango antropometrico usado en archetypes.c */
        float radius_uv = w->radius_mm / 180.0f;
        int cx = (int)(w->u * atlas_w);
        int cy = (int)(w->v * atlas_h);
        int rx = (int)(radius_uv * atlas_w) + 1;
        int ry = (int)(radius_uv * atlas_h) + 1;
        int x0 = cx - rx; if (x0 < 0) x0 = 0;
        int x1 = cx + rx; if (x1 >= atlas_w) x1 = atlas_w - 1;
        int y0 = cy - ry; if (y0 < 0) y0 = 0;
        int y1 = cy + ry; if (y1 >= atlas_h) y1 = atlas_h - 1;
        for (int y = y0; y <= y1; y++) {
            for (int x = x0; x <= x1; x++) {
                float dx = (float)(x - cx) / (float)(rx > 0 ? rx : 1);
                float dy = (float)(y - cy) / (float)(ry > 0 ? ry : 1);
                float d2 = dx * dx + dy * dy;
                if (d2 > 1.0f) continue;
                /* falloff suave (smoothstep) hacia el borde */
                float falloff = 1.0f - d2;
                falloff = falloff * falloff * (3.0f - 2.0f * falloff);
                int idx = (y * atlas_w + x) * 4;
                unsigned char *px = &atlas_rgba8[idx];
                for (int c = 0; c < 3; c++) {
                    float base = ((float)px[c] - 128.0f) / 128.0f;
                    float blended = rig_dermal_lerp__rig_dup_2b396547(base, color_delta[c], falloff);
                    int q = (int)((blended * 128.0f) + 128.0f);
                    if (q < 0) q = 0;
                    if (q > 255) q = 255;
                    px[c] = (unsigned char)q;
                }
                float base_h = (float)px[3] / 100.0f;
                float blended_h = rig_dermal_lerp__rig_dup_2b396547(base_h, height_mm, falloff);
                int qh = (int)(blended_h * 100.0f);
                if (qh < 0) qh = 0;
                if (qh > 255) qh = 255;
                px[3] = (unsigned char)qh;
            }
        }
    }
    return 0;
}
/* -------------------------------------------------------------------------
 * Generacion de codigo GLSL + JS runtime (mismo patron que ng_skin.c /
 * ng_anim.c: snippets de texto que el ensamblador (assembly/v2_bridge)
 * inyecta en el shader final y en el runtime del visor HTML).
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_dermal_generate_shader__rig_dup_f38a286b(RigDermalArtResult *out) {
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    int n = snprintf(out->glsl_fragment_snippet, RIG_DERMAL_SRC_MAX,
        "// === rig_face_dermal_dynamics :: fragment snippet (aditivo) ===\n"
        "uniform float u_dermal_blush;\n"
        "uniform float u_dermal_pallor;\n"
        "uniform float u_dermal_sweat;\n"
        "uniform float u_dermal_goosebump;\n"
        "uniform sampler2D u_dermal_wound_atlas;\n"
        "\n"
        "vec3 rig_dermal_apply(vec3 baseColor, vec2 uv, vec3 N, float wetnessDensity, float piloDensity) {\n"
        "    vec3 color = baseColor;\n"
        "    color += vec3(0.35, -0.05, -0.05) * u_dermal_blush;\n"
        "    color += vec3(-0.20, -0.05,  0.03) * u_dermal_pallor;\n"
        "\n"
        "    vec4 wound = texture(u_dermal_wound_atlas, uv);\n"
        "    vec3 woundColorDelta = (wound.rgb - vec3(0.5019608)) * 2.0;\n"
        "    color += woundColorDelta;\n"
        "\n"
        "    float wetness = clamp(u_dermal_sweat * wetnessDensity, 0.0, 1.0);\n"
        "    /* el termino especular real se combina en el BRDF principal del\n"
        "     * shader de piel (ng_skin.c); aqui solo se expone el factor. */\n"
        "    return color;\n"
        "}\n"
        "\n"
        "float rig_dermal_specular_boost(float wetnessDensity) {\n"
        "    return mix(1.0, 3.2, clamp(u_dermal_sweat * wetnessDensity, 0.0, 1.0));\n"
        "}\n"
        "\n"
        "float rig_dermal_wound_height(vec2 uv) {\n"
        "    return texture(u_dermal_wound_atlas, uv).a * 2.55; /* mm */\n"
        "}\n");
    out->glsl_len = n;
    int m = snprintf(out->js_runtime_snippet, RIG_DERMAL_SRC_MAX,
        "// === rig_face_dermal_dynamics :: JS runtime driver (aditivo) ===\n"
        "function RigDermalRuntime(gl, program) {\n"
        "  this.gl = gl;\n"
        "  this.loc = {\n"
        "    blush:     gl.getUniformLocation(program, 'u_dermal_blush'),\n"
        "    pallor:    gl.getUniformLocation(program, 'u_dermal_pallor'),\n"
        "    sweat:     gl.getUniformLocation(program, 'u_dermal_sweat'),\n"
        "    goosebump: gl.getUniformLocation(program, 'u_dermal_goosebump')\n"
        "  };\n"
        "}\n"
        "RigDermalRuntime.prototype.update = function(state) {\n"
        "  var gl = this.gl;\n"
        "  gl.uniform1f(this.loc.blush,     state.blush_level);\n"
        "  gl.uniform1f(this.loc.pallor,    state.pallor_level);\n"
        "  gl.uniform1f(this.loc.sweat,     state.sweat_level);\n"
        "  gl.uniform1f(this.loc.goosebump, state.goosebump_level);\n"
        "};\n");
    out->js_len = m;
    if (n < 0 || n >= RIG_DERMAL_SRC_MAX) return -2;
    if (m < 0 || m >= RIG_DERMAL_SRC_MAX) return -3;
    return 0;
}
/* -------------------------------------------------------------------------
 * Serializacion binaria (save/load), formato con magic + version para
 * evolucion futura sin romper compatibilidad (cero perdida de estado).
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int payload_size;
} RigDermalFileHeader;
RIGCOM_PUBLIC int rig_dermal_state_save__rig_dup_702c1cf6(const RigDermalState *st, FILE *fp) {
    if (!st || !fp) return -1;
    RigDermalFileHeader hdr;
    hdr.magic = RIG_DERMAL_MAGIC;
    hdr.version = RIG_DERMAL_VERSION;
    hdr.payload_size = (unsigned int)sizeof(RigDermalState);
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (fwrite(st, sizeof(RigDermalState), 1, fp) != 1) return -3;
    return 0;
}
RIGCOM_PUBLIC int rig_dermal_state_load__rig_dup_2fb82a6c(RigDermalState *st, FILE *fp) {
    if (!st || !fp) return -1;
    RigDermalFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (hdr.magic != RIG_DERMAL_MAGIC) return -3;
    if (hdr.version != RIG_DERMAL_VERSION) return -4; /* migracion futura entra aqui */
    if (hdr.payload_size != (unsigned int)sizeof(RigDermalState)) return -5;
    if (fread(st, sizeof(RigDermalState), 1, fp) != 1) return -6;
    return 0;
}