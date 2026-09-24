#include "rigdeps/rig_std_base.h"
#include "rig_v17_preamble.h"
#include "rig_face_engine_v2.h"
#include "rigdeps/stdlib.h"
#include "rigdeps/string.h"
#include "rigdeps/math.h"
#include "../include/riglib_math.h"
#include "rigdeps/stdio.h"
#include "rigdeps/time.h"

#define V2_PHI          1.6180339887498948482f
#define V2_PHI_INV      0.6180339887498948482f
#define V2_SCHUMANN     7.83f
#define V2_PI           3.14159265358979323846f
#define FLOATS_PER_VERT 15

extern RigFaceMesh* rig_face_create(const RigFaceParams *params,
                                     rl_u32 subdiv_level);
extern int          rig_face_destroy(RigFaceMesh *mesh);
extern int          rig_face_export_vbo(const RigFaceMesh *mesh,
                                         float *vbo, rl_u32 *ibo,
                                         rl_u32 *n_floats,
                                         rl_u32 *n_indices);

int rig_face_v2_destroy__rig_variant_a8a25bc2(RigFaceMeshV2 *m)
{
    if (!m) return -1;

    rl_free(m->pore_normal_map);
    rl_free(m->vascular_map);
    rl_free(m->wrinkle_normal_map);
    rl_free(m->thermal_map);
    rl_free(m->ear_left_verts);
    rl_free(m->ear_left_tris);
    rl_free(m->ear_right_verts);
    rl_free(m->ear_right_tris);

    /* verts/tris provienen del arena global; no son bloques malloc independientes. */
    rig_face_destroy(&m->base);
    rl_memset(m, 0, sizeof(*m));
    rl_free(m);
    return 0;
}

int rig_face_build_pore_system__rig_variant_82fb034d(RigFaceMeshV2 *m, float density_scale)
{
    if (!m || density_scale <= 0.0f) return -1;

    rl_u32 w = 512, h = 512;
    m->texture_width  = w;
    m->texture_height = h;

    rl_free(m->pore_normal_map);
    m->pore_normal_map = (rl_u8*)rl_calloc(w * h * 4, 1);
    if (!m->pore_normal_map) return -1;

    float pore_radius = (0.008f / density_scale);
    float cell_size   = pore_radius * (float)w;

    for (rl_u32 y = 0; y < h; y++) {
        for (rl_u32 x = 0; x < w; x++) {
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;

            float cell_u = fmodf(u / pore_radius, 1.0f);
            float cell_v = fmodf(v / pore_radius, 1.0f);

            float dist_sq = 1.0f;
            for (int dj = -1; dj <= 1; dj++) {
                for (int di = -1; di <= 1; di++) {
                    float jitter_u = v2_noise__rig_variant_2ce05560(0xBEEF, cell_u + di, cell_v + dj);
                    float jitter_v = v2_noise__rig_variant_2ce05560(0xDEAD, cell_u + di, cell_v + dj);
                    float du = cell_u - (floorf(cell_u) + di + jitter_u);
                    float dv = cell_v - (floorf(cell_v) + dj + jitter_v);
                    float d  = du*du + dv*dv;
                    if (d < dist_sq) dist_sq = d;
                }
            }

            float dist = sqrtf(dist_sq);
            float pore_depth = m->base.material.pore_depth > 0.0f
                               ? m->base.material.pore_depth : 0.04f;

            float nx = 0.0f, ny = 0.0f;
            if (dist < 0.35f) {
                float t = dist / 0.35f;
                float bump = pore_depth * (1.0f - t*t);
                nx = -2.0f * (u - floorf(u/pore_radius)*pore_radius) * bump;
                ny = -2.0f * (v - floorf(v/pore_radius)*pore_radius) * bump;
            }
            float nz = sqrtf(fmaxf(0.0f, 1.0f - nx*nx - ny*ny));

            rl_u8 *px = m->pore_normal_map + (y*w + x)*4;
            px[0] = (rl_u8)((nx*0.5f + 0.5f) * 255.0f);
            px[1] = (rl_u8)((ny*0.5f + 0.5f) * 255.0f);
            px[2] = (rl_u8)((nz*0.5f + 0.5f) * 255.0f);
            px[3] = (rl_u8)(fmaxf(0.0f, 1.0f - dist*3.0f) * 255.0f);
        }
    }
    (void)cell_size;
    return 0;
}

int rig_face_build_vascular_tree__rig_variant_7e94afb2(RigFaceMeshV2 *m)
{
    if (!m) return -1;

    rl_u32 w = 512, h = 512;
    m->texture_width  = m->texture_width  ? m->texture_width  : w;
    m->texture_height = m->texture_height ? m->texture_height : h;
    w = m->texture_width; h = m->texture_height;

    rl_free(m->vascular_map);
    m->vascular_map = (rl_u8*)rl_calloc(w * h * 4, 1);
    if (!m->vascular_map) return -1;

    float hemo  = m->base.params.hemoglobin;
    float age   = m->base.params.age_factor;

    for (rl_u32 y = 0; y < h; y++) {
        for (rl_u32 x = 0; x < w; x++) {
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;

            float n  = v2_simplex__rig_dup_380be2e4(u * 8.0f,          v * 8.0f         ) * 0.50f
                     + v2_simplex__rig_dup_380be2e4(u * 8.0f * V2_PHI,  v * 8.0f * V2_PHI) * 0.30f
                     + v2_simplex__rig_dup_380be2e4(u * 8.0f * V2_PHI*V2_PHI,
                                  v * 8.0f * V2_PHI*V2_PHI)              * 0.20f;
            n = n * 0.5f + 0.5f;

            float vessel = n * hemo * (0.5f + age * 0.5f);
            vessel = fminf(vessel, 1.0f);

            rl_u8 *px = m->vascular_map + (y*w + x)*4;
            px[0] = (rl_u8)(vessel * 255.0f);
            px[1] = (rl_u8)(hemo   * 255.0f);
            px[2] = (rl_u8)(m->base.params.melanin * 255.0f);
            px[3] = (rl_u8)(fminf(vessel * 1.5f, 1.0f) * 255.0f);
        }
    }
    return 0;
}

int rig_face_build_wrinkle_lines__rig_variant_d4e63079(RigFaceMeshV2 *m, float age,
                                   float expression)
{
    if (!m) return -1;

    if (age        < 0.0f) age        = 0.0f;
    if (age        > 1.0f) age        = 1.0f;
    if (expression < 0.0f) expression = 0.0f;
    if (expression > 1.0f) expression = 1.0f;

    rl_u32 w = 512, h = 512;
    m->texture_width  = m->texture_width  ? m->texture_width  : w;
    m->texture_height = m->texture_height ? m->texture_height : h;
    w = m->texture_width; h = m->texture_height;

    rl_free(m->wrinkle_normal_map);
    m->wrinkle_normal_map = (rl_u8*)rl_calloc(w * h * 4, 1);
    if (!m->wrinkle_normal_map) return -1;

    float depth = m->base.material.wrinkle_depth > 0.0f
                  ? m->base.material.wrinkle_depth
                  : 0.05f;
    float age_depth = depth * (age * 0.8f + expression * 0.2f);

    for (rl_u32 y = 0; y < h; y++) {
        for (rl_u32 x = 0; x < w; x++) {
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;

            float line_forehead  = sinf(v * V2_PI * 6.0f);
            float line_naso      = sinf((u*0.6f + v*0.4f) * V2_PI * 4.0f);
            float line_lateral   = v2_simplex__rig_dup_380be2e4(u*12.0f, v*12.0f);

            float combined = (line_forehead * 0.40f
                            + line_naso     * 0.35f
                            + line_lateral  * 0.25f) * age_depth;

            float fade = fmaxf(0.0f, (age - 0.2f) / 0.8f);
            combined  *= fade;

            float nx = combined * sinf(u * V2_PI * 8.0f);
            float ny = combined * cosf(v * V2_PI * 8.0f);
            float nz = sqrtf(fmaxf(0.0f, 1.0f - nx*nx - ny*ny));

            rl_u8 *px = m->wrinkle_normal_map + (y*w + x)*4;
            px[0] = (rl_u8)((nx*0.5f + 0.5f) * 255.0f);
            px[1] = (rl_u8)((ny*0.5f + 0.5f) * 255.0f);
            px[2] = (rl_u8)((nz*0.5f + 0.5f) * 255.0f);
            px[3] = (rl_u8)(fabsf(combined) * 255.0f);
        }
    }
    return 0;
}

int rig_age_apply_to_mesh__rig_variant_9e98e555(RigFaceMeshV2 *m, float age_years)
{
    if (!m) return -1;
    if (age_years <  0.0f) age_years =  0.0f;
    if (age_years > 100.0f) age_years = 100.0f;

    const RigFaceArchetype *young = rig_archetype_get(RIG_ARCH_AGE_YOUNG_25);
    const RigFaceArchetype *elder = rig_archetype_get(RIG_ARCH_AGE_ELDER_70);
    if (!young || !elder) return -1;

    float t = (age_years - 25.0f) / 45.0f;
    t = fmaxf(0.0f, fminf(1.0f, t));

    float t_smooth = t * t * (3.0f - 2.0f * t);

    const float *py = (const float*)&young->params;
    const float *pe = (const float*)&elder->params;
    float       *pp = (float*)&m->base.params;
    rl_size       n  = sizeof(RigFaceParams) / sizeof(float);
    for (rl_size i = 0; i < n; i++)
        pp[i] = py[i] + (pe[i] - py[i]) * t_smooth;

    m->base.params.age_factor = t_smooth;

    m->base.material.roughness     = 0.45f + t_smooth * 0.25f;
    m->base.material.sss_weight    = 0.45f - t_smooth * 0.10f;
    m->base.material.wrinkle_depth = 0.02f + t_smooth * 0.12f;
    m->base.material.oiliness      = 0.30f - t_smooth * 0.15f;
    m->base.material.pore_scale    = 0.8f  + t_smooth * 0.4f;

    m->age_progression.milestones[0].ptosis_upper     = t_smooth * 0.35f;
    m->age_progression.milestones[0].jowl_factor      = t_smooth * 0.45f;
    m->age_progression.milestones[0].nasolabial_depth = t_smooth * 0.55f;
    m->age_progression.current_age = age_years;

    if (m->base.verts && m->base.n_verts > 0) {
        float sag = t_smooth * 0.04f;
        for (rl_u32 i = 0; i < m->base.n_verts; i++) {
            RigFaceVertex *v = &m->base.verts[i];

            if (v->pos.y < 0.0f)
                v->pos.y -= sag * fabsf(v->pos.y);
        }
    }

    m->phi_coherence = V2_PHI_INV * (1.0f - t_smooth * 0.1f);
    return 0;
}

int rig_face_build_iris_geometry__rig_variant_860a5ffb(RigFaceMeshV2 *m, rl_bool right)
{
    if (!m) return -1;

    RigFaceIrisDetail *ir = right ? &m->iris_right : &m->iris_left;

    ir->pupil_radius = fmaxf(0.20f, fminf(0.60f, ir->pupil_radius));
    (void)right;
    return 0;
}

int rig_face_bake_iris_texture__rig_variant_4af75347(const RigFaceIrisDetail *iris,
                                 rl_u8 *out_rgba, rl_u32 w, rl_u32 h)
{
    if (!iris || !out_rgba || w == 0 || h == 0) return -1;
    for (rl_u32 y = 0; y < h; y++) {
        for (rl_u32 x = 0; x < w; x++) {
            float u  = (float)x / (float)w * 2.0f - 1.0f;
            float v  = (float)y / (float)h * 2.0f - 1.0f;
            float r  = sqrtf(u*u + v*v);
            float a  = atan2f(v, u);
            rl_u8 *px = out_rgba + (y*w + x)*4;
            if (r > iris->iris_radius) {

                px[0] = 255; px[1] = 255; px[2] = 255; px[3] = 255;
            } else if (r < iris->pupil_radius) {

                px[0] = 5; px[1] = 5; px[2] = 5; px[3] = 255;
            } else {

                float cell = v2_noise__rig_variant_2ce05560(0x1234, r * iris->crypt_density * 64.0f,
                                      a * iris->crypt_density * 64.0f);
                float c0 = iris->iris_color[0] * cell;
                float c1 = iris->iris_color[1] * cell;
                float c2 = iris->iris_color[2] * cell;
                px[0] = (rl_u8)(fminf(c0, 1.0f) * 255.0f);
                px[1] = (rl_u8)(fminf(c1, 1.0f) * 255.0f);
                px[2] = (rl_u8)(fminf(c2, 1.0f) * 255.0f);
                px[3] = 255;
            }
        }
    }
    return 0;
}

int rig_iris_set_pupil_dilation__rig_variant_acdf855e(RigFaceIrisDetail *iris, float lux)
{
    if (!iris) return -1;

    float dil = 0.60f - lux * 0.35f;
    iris->pupil_radius = fmaxf(1.0f, fminf(5.5f, dil * 6.5f));
    return 0;
}

int rig_face_build_ear_geometry__rig_variant_95219d0a(RigFaceMeshV2 *m, rl_bool right)
{
    if (!m) return -1;
    RigFaceEarParams *ep = right ? &m->ear_right_params : &m->ear_left_params;
    rl_u32 n_verts = 128, n_tris = 220;

    RigFaceVertex **pverts = right ? &m->ear_right_verts : &m->ear_left_verts;
    RigFaceTri    **ptris  = right ? &m->ear_right_tris  : &m->ear_left_tris;
    rl_u32 *pnv = right ? &m->ear_right_n_verts : &m->ear_left_n_verts;
    rl_u32 *pnt = right ? &m->ear_right_n_tris  : &m->ear_left_n_tris;

    rl_free(*pverts); rl_free(*ptris);
    *pverts = (RigFaceVertex*)rl_calloc(n_verts, sizeof(RigFaceVertex));
    *ptris  = (RigFaceTri*)   rl_calloc(n_tris,  sizeof(RigFaceTri));
    if (!*pverts || !*ptris) { rl_free(*pverts); rl_free(*ptris); *pverts = NULL; *ptris = NULL; return -1; }

    *pnv = n_verts; *pnt = n_tris;

    float sign = right ? 1.0f : -1.0f;
    float lx   = sign * (m->base.params.cranium_width * 0.5f + 0.5f);

    for (rl_u32 i = 0; i < n_verts; i++) {
        float t   = (float)i / (float)n_verts;
        float ang = t * V2_PHI * V2_PI * 2.0f;
        float r   = ep->ear_height * 0.5f * (0.6f + 0.4f * sinf(t * V2_PI));

        (*pverts)[i].pos.x = lx + sinf(ang) * ep->ear_width * 0.3f;
        (*pverts)[i].pos.y = r * cosf(ang * 0.5f) - ep->ear_height * 0.2f;
        (*pverts)[i].pos.z = r * sinf(ang * 0.5f) * ep->helix_width * 0.5f;

        (*pverts)[i].normal.x = sinf(ang);
        (*pverts)[i].normal.y = cosf(ang);
        (*pverts)[i].normal.z = 0.1f;

        (*pverts)[i].uv.x = t;
        (*pverts)[i].uv.y = 0.5f + 0.5f * sinf(ang);
    }

    rl_u32 ti = 0;
    for (rl_u32 i = 0; i < n_verts - 2 && ti < n_tris; i++, ti++) {
        (*ptris)[ti].a = i;
        (*ptris)[ti].b = i + 1;
        (*ptris)[ti].c = (i + 2) % n_verts;
    }
    (void)ep;
}

int rig_face_attach_ears__rig_variant_e0a090c7(RigFaceMeshV2 *m)
{
    if (!m) return -1;
    if (!m->ear_left_verts)  rig_face_build_ear_geometry__rig_variant_95219d0a(m, false);
    if (!m->ear_right_verts && rig_face_build_ear_geometry__rig_variant_95219d0a(m, true) != 0) return -1;
    return 0;
}

int rig_face_sculpt_lips_v2__rig_variant_ddb41423(RigFaceMeshV2 *m)
{
    if (!m || !m->base.verts) return -1;

    const RigFaceLipDetail *lip = &m->lip;
    float my = -(m->base.params.lower_third * 0.3f);
    for (rl_u32 i = 0; i < m->base.n_verts; i++) {
        RigFaceVertex *v = &m->base.verts[i];
        float dy = v->pos.y - my;
        if (fabsf(dy) < lip->upper_lip_height * 0.1f &&
            fabsf(v->pos.x) < m->base.params.mouth_width * 0.6f) {
            v->pos.z += lip->vermilion_height * 0.005f
                      * expf(-v->pos.x*v->pos.x * 0.1f)
                      * expf(-dy*dy * 2.0f);
        }
    }
    return 0;
}

int rig_face_bake_lip_color_map__rig_variant_bb2339a3(const RigFaceMeshV2 *m,
                                  rl_u8 *out_rgba, rl_u32 w, rl_u32 h)
{
    if (!m || !out_rgba || w == 0 || h == 0) return -1;
    const RigFaceLipDetail *lip = &m->lip;
    for (rl_u32 y = 0; y < h; y++) {
        for (rl_u32 x = 0; x < w; x++) {
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;

            float dx   = u*2.0f - 1.0f;
            float bow  = sinf(dx * V2_PI) * lip->cupid_bow_depth;
            float dist = fabsf(v*2.0f - 1.0f - bow);
            float fade = fmaxf(0.0f, 1.0f - dist * 1.5f);
            float sat  = lip->vermilion_saturation;
            rl_u8 *px = out_rgba + (y*w + x)*4;
            px[0] = (rl_u8)(0.78f * sat * fade * 255.0f);
            px[1] = (rl_u8)(0.35f * sat * fade * 255.0f);
            px[2] = (rl_u8)(0.32f * sat * fade * 255.0f);
            px[3] = (rl_u8)(fade * 255.0f);
        }
    }
    return 0;
}

int rig_face_bake_pore_normal_map__rig_variant_df9af220(RigFaceMeshV2 *m, rl_u32 w, rl_u32 h)
{
    m->texture_width = w; m->texture_height = h;
    rig_face_build_pore_system__rig_variant_82fb034d(m, 1.0f);
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
int rig_face_bake_vascular_map__rig_variant_f170bd7c(RigFaceMeshV2 *m, rl_u32 w, rl_u32 h)
{
    m->texture_width = w; m->texture_height = h;
    rig_face_build_vascular_tree__rig_variant_7e94afb2(m);
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
int rig_face_bake_wrinkle_normal_map__rig_variant_62b0f6c1(RigFaceMeshV2 *m, rl_u32 w, rl_u32 h)
{
    m->texture_width = w; m->texture_height = h;
    rig_face_build_wrinkle_lines__rig_variant_d4e63079(m, m->base.params.age_factor, 0.0f);
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

/* ═══════════════════════════════════════════════════════════════════════════
 * rig_face_build_follicle_map — mapa de folículos pilosos soberano
 * Autor: Richard Felipe Urbina
 *
 * Genera la distribución de folículos (raíz + dirección + propiedades) sobre
 * la superficie de la malla v2, siguiendo los parámetros de la región capilar.
 *
 * Algoritmo:
 *   1. Para cada región i en RIG_REGION_COUNT, leer hair[i]
 *   2. Calcular densidad de folículos (folículos/cm²) → nF para esa región
 *   3. Distribuir usando Poisson-disk sampling aproximado sobre vértices UV
 *   4. Para cada folículo: raíz = vértice más cercano, dirección = normal + hair.dir
 *   5. Propiedades de fibra desde RigHairRegionParams
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rigdeps/math.h"
#include "rigdeps/stdlib.h"
#include "rigdeps/string.h"
#include "../include/rig_face_engine_v2.h"
#include "../include/rig_math.h"

int rig_face_build_follicle_map__rig_variant_f66916e7(RigFaceMeshV2 *mesh,
                                   const RigHairRegionParams *hair){
    if (!mesh || !hair) return -1;

    /* Acceso a la malla base */
    RigFaceMesh *base = &mesh->base;
    if (!base->verts || base->n_verts == 0) return -1;

    /* Limpiar folículos existentes */
    mesh->microdetail.n_follicles = 0;

    rl_u32 n_placed = 0;
    rl_u32 seed = 0xC47ED1A1u ^ base->n_verts;

    /* Calcular área superficial aproximada del cráneo para escalar densidad */
    /* Área ≈ 4π×r_medio² × factores elipsoidales ~ 600 cm² para cabeza humana */
    const float AREA_CM2 = 580.0f;

    /* Para cada región procesamos los vértices que caen en ella */
    for (rl_u32 reg = 0; reg < RIG_REGION_COUNT && n_placed < RIG_FACE_FOLLICLE_MAX; reg++) {
        const RigHairRegionParams *hr = &hair[reg];
        if (hr->density < 1e-4f) continue;

        /* Densidad en folículos/cm²: tipicamente 80-120 para cuero cabelludo,
           vello: ~20-40, sin pelo: 0-5                                        */
        float follicles_per_cm2 = hr->density;
        /* Fracción de área en esta región (simplificado: uniforme) */
        float region_area = AREA_CM2 / (float)RIG_REGION_COUNT;
        rl_u32 n_target = (rl_u32)(follicles_per_cm2 * region_area);
        if (n_target == 0) n_target = 1;

        /* Recolectar vértices de esta región */
        rl_u32 reg_verts[512];
        rl_u32 n_rv = 0;
        for (rl_u32 vi = 0; vi < base->n_verts && n_rv < 512; vi++) {
            if (_vertex_region__rig_dup_0cf07b3f(&base->verts[vi]) == (rl_u8)reg)
                reg_verts[n_rv++] = vi;
        }
        if (n_rv == 0) continue;

        /* Poisson-disk aproximado: elegir n_target vértices aleatorios
           sin repetición (mínima distancia entre ellos)                   */
        rl_u32 n_place_reg = (n_target < (RIG_FACE_FOLLICLE_MAX - n_placed))
                               ? n_target : (RIG_FACE_FOLLICLE_MAX - n_placed);
        /* Radio de exclusión mínimo (cm) según densidad */
        float excl_r = (follicles_per_cm2 > 0)
                       ? sqrtf(1.0f / (3.14159f * follicles_per_cm2)) * 0.8f
                       : 0.5f;

        for (rl_u32 k = 0; k < n_place_reg; k++) {
            /* Elegir vértice candidato pseudo-aleatorio */
            rl_u32 try_count = 0;
            rl_bool placed = false;
            while (try_count < 16 && !placed) {
                float rnd = _hash2__rig_variant_6ca546f1(seed + reg * 4096 + k, try_count++);
                rl_u32 vi = reg_verts[(rl_u32)(rnd * (float)n_rv) % n_rv];
                Vec3f vp = base->verts[vi].pos;

                /* Verificar distancia mínima a folículos ya colocados
                   (revisar últimos N colocados para no ser O(N²) completo) */
                rl_bool too_close = false;
                rl_u32 check_start = (n_placed > 32) ? n_placed - 32 : 0;
                for (rl_u32 fi = check_start; fi < n_placed; fi++) {
                    Vec3f fp = mesh->microdetail.follicles[fi].root;
                    float dx = vp.x - fp.x, dy = vp.y - fp.y, dz = vp.z - fp.z;
                    float d2 = dx*dx + dy*dy + dz*dz;
                    if (d2 < excl_r * excl_r) { too_close = true; break; }
                }
                if (too_close) continue;

                /* Colocar folículo */
                RigFollicle *fo = &mesh->microdetail.follicles[n_placed];
                fo->root      = vp;
                /* Dirección: normal del vértice rotada hacia hair.primary_direction */
                Vec3f n3 = base->verts[vi].normal;
                Vec3f dir = hr->primary_direction;
                float blend = hr->direction_scatter;
                fo->direction.x = n3.x * blend + dir.x * (1.0f - blend);
                fo->direction.y = n3.y * blend + dir.y * (1.0f - blend);
                fo->direction.z = n3.z * blend + dir.z * (1.0f - blend);
                /* Normalizar dirección */
                float dlen = sqrtf(fo->direction.x * fo->direction.x +
                                    fo->direction.y * fo->direction.y +
                                    fo->direction.z * fo->direction.z);
                if (dlen > 1e-6f) {
                    fo->direction.x /= dlen;
                    fo->direction.y /= dlen;
                    fo->direction.z /= dlen;
                }
                /* Propiedades de la fibra */
                fo->diameter = hr->diameter * (0.9f + 0.2f * _hash2__rig_variant_6ca546f1(seed, n_placed));
                fo->length   = hr->length   * (0.85f + 0.3f * _hash2__rig_variant_6ca546f1(seed + 1, n_placed));
                fo->curl     = hr->curl_radius;
                fo->melanin  = hr->melanin_eu + hr->melanin_ph * 0.5f;
                fo->region   = (rl_u8)reg;
                n_placed++;
                placed = true;
            }
        }
    }

    mesh->microdetail.n_follicles = n_placed;

    /* Actualizar coherencia φ (relación folículos/densidad_esperada) */
    float expected = 0;
    for (rl_u32 r = 0; r < RIG_REGION_COUNT; r++)
        expected += hair[r].density * (AREA_CM2 / (float)RIG_REGION_COUNT);
    mesh->phi_coherence = (expected > 0)
        ? fminf(1.0f, (float)n_placed / expected)
        : 0.0f;
    return 0;
}
