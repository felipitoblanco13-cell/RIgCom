/*
 * RIGCOM — unidad C canónica consolidada mecánicamente.
 * Familia: face_v2
 * Ninguna sección incluye otro archivo .c.
 */
/* ================================================================
 * FUENTE ABSORBIDA: 16_FACE_NG/src/rig_face_engine_v2.c
 * ================================================================ */
#line 1 "16_FACE_NG/src/rig_face_engine_v2.c"
/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "rig_v17_preamble.h"
#include "rig_face_engine_v2.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_math.h"

#include "rig_noext_io.h"
#include "rig_syscall.h"
#define V2_PHI          1.6180339887498948482f
#define V2_PHI_INV      0.6180339887498948482f
#define V2_SCHUMANN     7.83f
#define V2_PI           3.14159265358979323846f
#define FLOATS_PER_VERT 15
extern RigFaceMesh* rig_face_create(const RigFaceParams *params,
                                     uint32_t subdiv_level);
extern void         rig_face_destroy(RigFaceMesh *mesh);
extern int          rig_face_export_vbo(const RigFaceMesh *mesh,
                                         float *vbo, uint32_t *ibo,
                                         uint32_t *n_floats,
                                         uint32_t *n_indices);
static inline float v2_noise(uint32_t seed, float x, float y)
{
    uint32_t h = seed;
    h ^= (uint32_t)(x * 73856093u);
    h ^= (uint32_t)(y * 19349663u);
    h ^= h >> 16; h *= 0x45d9f3bu; h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.0f;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: v2_noise -> rigpub_rig_face_engine_v2_v2_noise */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
static inline float v2_snoise(uint32_t s, float x, float y)
{
    return v2_noise(s, x, y) * 2.0f - 1.0f;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: v2_snoise -> rigpub_rig_face_engine_v2_v2_snoise */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
static float v2_simplex(float x, float y)
{
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    float u  = fx*fx*(3.0f - 2.0f*fx);
    float v  = fy*fy*(3.0f - 2.0f*fy);
    float a  = v2_noise(0xCAFE, (float)ix,   (float)iy  );
    float b  = v2_noise(0xCAFE, (float)(ix+1),(float)iy  );
    float c  = v2_noise(0xCAFE, (float)ix,   (float)(iy+1));
    float d  = v2_noise(0xCAFE, (float)(ix+1),(float)(iy+1));
    return a + (b-a)*u + (c-a)*v + (a-b-c+d)*u*v;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: v2_simplex -> rigpub_rig_face_engine_v2_v2_simplex */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
RigFaceMeshV2 *rig_face_v2_create(const RigFaceParams *params,
                                    uint32_t subdiv)
{
    if (!params) return NULL;
    if (subdiv < 2) subdiv = 2;
    if (subdiv > 6) subdiv = 6;
    RigFaceMesh *base = rig_face_create(params, subdiv <= 5 ? subdiv : 5);
    if (!base) return NULL;
    RigFaceMeshV2 *m = (RigFaceMeshV2*)calloc(1, sizeof(RigFaceMeshV2));
    if (!m) { rig_face_destroy(base); return NULL; }
    memcpy(&m->base, base, sizeof(RigFaceMesh));
    free(base);
    if (subdiv == 6) m->base.subdivision_level = 6;
    m->ear_left_params  = rig_ear_default_params(false);
    m->ear_right_params = rig_ear_default_params(true);
    m->lip               = rig_lip_default_params(params->gender_factor);
    m->iris_left  = rig_iris_from_params(params);
    m->iris_right = rig_iris_from_params(params);
    m->version      = 2;
    m->phi_coherence = V2_PHI_INV;
    m->archetype_id  = RIG_ARCH_COUNT;
    m->has_archetype = false;
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    if (tm_info)
        strftime(m->build_timestamp, sizeof(m->build_timestamp),
                 "%Y-%m-%dT%H:%M:%S", tm_info);
    return m;
}
RigFaceMeshV2 *rig_face_v2_from_archetype(RigArchetypeID id)
{
    if ((int)id < 0 || id >= RIG_ARCH_COUNT) return NULL;
    const RigFaceArchetype *arch = rig_archetype_get__rig_dup_49442146(id);
    if (!arch) return NULL;
    uint32_t subdiv = (id >= 22) ? 5 : 4;
    RigFaceMeshV2 *m = rig_face_v2_create(&arch->params, subdiv);
    if (!m) return NULL;
    m->iris_left  = arch->iris;
    m->iris_right = arch->iris;
    m->ear_left_params  = arch->ear_left;
    m->ear_right_params = arch->ear_right;
    m->lip               = arch->lip;
    if (arch->roughness_override >= 0.0f)
        m->base.material.roughness = arch->roughness_override;
    if (arch->sss_override[0] >= 0.0f) {
        m->base.material.sss_radius[0] = arch->sss_override[0];
        m->base.material.sss_radius[1] = arch->sss_override[1];
        m->base.material.sss_radius[2] = arch->sss_override[2];
    }
    m->archetype_id  = id;
    m->has_archetype = true;
    m->phi_coherence = arch->phi_reference > 0.0f
                        ? arch->phi_reference : V2_PHI_INV;
    return m;
}
int rig_face_v2_destroy(RigFaceMeshV2 *m)
{
    if (!m) return 0;
    free(m->pore_normal_map);
    free(m->vascular_map);
    free(m->wrinkle_normal_map);
    free(m->thermal_map);
    free(m->ear_left_verts);
    free(m->ear_left_tris);
    free(m->ear_right_verts);
    free(m->ear_right_tris);
    if (m->base.verts) free(m->base.verts);
    if (m->base.tris)  free(m->base.tris);
    memset(m, 0, sizeof(*m));
    free(m);
}
int rig_face_build_pore_system(RigFaceMeshV2 *m, float density_scale)
{
    if (!m || density_scale <= 0.0f) return 0;
    uint32_t w = 512, h = 512;
    m->texture_width  = w;
    m->texture_height = h;
    free(m->pore_normal_map);
    m->pore_normal_map = (uint8_t*)calloc(w * h * 4, 1);
    if (!m->pore_normal_map) return 0;
    float pore_radius = (0.008f / density_scale);
    float cell_size   = pore_radius * (float)w;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;
            float cell_u = fmodf(u / pore_radius, 1.0f);
            float cell_v = fmodf(v / pore_radius, 1.0f);
            float dist_sq = 1.0f;
            for (int dj = -1; dj <= 1; dj++) {
                for (int di = -1; di <= 1; di++) {
                    float jitter_u = v2_noise(0xBEEF, cell_u + di, cell_v + dj);
                    float jitter_v = v2_noise(0xDEAD, cell_u + di, cell_v + dj);
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
            uint8_t *px = m->pore_normal_map + (y*w + x)*4;
            px[0] = (uint8_t)((nx*0.5f + 0.5f) * 255.0f);
            px[1] = (uint8_t)((ny*0.5f + 0.5f) * 255.0f);
            px[2] = (uint8_t)((nz*0.5f + 0.5f) * 255.0f);
            px[3] = (uint8_t)(fmaxf(0.0f, 1.0f - dist*3.0f) * 255.0f);
        }
    }
    (void)cell_size;
}
int rig_face_build_vascular_tree(RigFaceMeshV2 *m)
{
    if (!m) return 0;
    uint32_t w = 512, h = 512;
    m->texture_width  = m->texture_width  ? m->texture_width  : w;
    m->texture_height = m->texture_height ? m->texture_height : h;
    w = m->texture_width; h = m->texture_height;
    free(m->vascular_map);
    m->vascular_map = (uint8_t*)calloc(w * h * 4, 1);
    if (!m->vascular_map) return 0;
    float hemo  = m->base.params.hemoglobin;
    float age   = m->base.params.age_factor;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;
            float n  = v2_simplex(u * 8.0f,          v * 8.0f         ) * 0.50f
                     + v2_simplex(u * 8.0f * V2_PHI,  v * 8.0f * V2_PHI) * 0.30f
                     + v2_simplex(u * 8.0f * V2_PHI*V2_PHI,
                                  v * 8.0f * V2_PHI*V2_PHI)              * 0.20f;
            n = n * 0.5f + 0.5f;
            float vessel = n * hemo * (0.5f + age * 0.5f);
            vessel = fminf(vessel, 1.0f);
            uint8_t *px = m->vascular_map + (y*w + x)*4;
            px[0] = (uint8_t)(vessel * 255.0f);
            px[1] = (uint8_t)(hemo   * 255.0f);
            px[2] = (uint8_t)(m->base.params.melanin * 255.0f);
            px[3] = (uint8_t)(fminf(vessel * 1.5f, 1.0f) * 255.0f);
        }
    }
}
int rig_face_build_wrinkle_lines(RigFaceMeshV2 *m, float age,
                                   float expression)
{
    if (!m) return 0;
    if (age        < 0.0f) age        = 0.0f;
    if (age        > 1.0f) age        = 1.0f;
    if (expression < 0.0f) expression = 0.0f;
    if (expression > 1.0f) expression = 1.0f;
    uint32_t w = 512, h = 512;
    m->texture_width  = m->texture_width  ? m->texture_width  : w;
    m->texture_height = m->texture_height ? m->texture_height : h;
    w = m->texture_width; h = m->texture_height;
    free(m->wrinkle_normal_map);
    m->wrinkle_normal_map = (uint8_t*)calloc(w * h * 4, 1);
    if (!m->wrinkle_normal_map) return 0;
    float depth = m->base.material.wrinkle_depth > 0.0f
                  ? m->base.material.wrinkle_depth
                  : 0.05f;
    float age_depth = depth * (age * 0.8f + expression * 0.2f);
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;
            float line_forehead  = sinf(v * V2_PI * 6.0f);
            float line_naso      = sinf((u*0.6f + v*0.4f) * V2_PI * 4.0f);
            float line_lateral   = v2_simplex(u*12.0f, v*12.0f);
            float combined = (line_forehead * 0.40f
                            + line_naso     * 0.35f
                            + line_lateral  * 0.25f) * age_depth;
            float fade = fmaxf(0.0f, (age - 0.2f) / 0.8f);
            combined  *= fade;
            float nx = combined * sinf(u * V2_PI * 8.0f);
            float ny = combined * cosf(v * V2_PI * 8.0f);
            float nz = sqrtf(fmaxf(0.0f, 1.0f - nx*nx - ny*ny));
            uint8_t *px = m->wrinkle_normal_map + (y*w + x)*4;
            px[0] = (uint8_t)((nx*0.5f + 0.5f) * 255.0f);
            px[1] = (uint8_t)((ny*0.5f + 0.5f) * 255.0f);
            px[2] = (uint8_t)((nz*0.5f + 0.5f) * 255.0f);
            px[3] = (uint8_t)(fabsf(combined) * 255.0f);
        }
    }
}
int rig_face_v2_export_vbo(const RigFaceMeshV2 *m,
                             float *vbo, uint32_t *ibo,
                             uint32_t *n_floats, uint32_t *n_indices)
{
    if (!m) return -1;
    uint32_t nf = m->base.n_verts * FLOATS_PER_VERT;
    uint32_t ni = m->base.n_tris  * 3;
    if (n_floats)  *n_floats  = nf;
    if (n_indices) *n_indices = ni;
    if (!vbo && !ibo) return 0;
    return rig_face_export_vbo(&m->base, vbo, ibo, n_floats, n_indices);
}
int rig_age_apply_to_mesh(RigFaceMeshV2 *m, float age_years)
{
    if (!m) return 0;
    if (age_years <  0.0f) age_years =  0.0f;
    if (age_years > 100.0f) age_years = 100.0f;
    const RigFaceArchetype *young = rig_archetype_get__rig_dup_49442146(RIG_ARCH_AGE_YOUNG_25);
    const RigFaceArchetype *elder = rig_archetype_get__rig_dup_49442146(RIG_ARCH_AGE_ELDER_70);
    if (!young || !elder) return 0;
    float t = (age_years - 25.0f) / 45.0f;
    t = fmaxf(0.0f, fminf(1.0f, t));
    float t_smooth = t * t * (3.0f - 2.0f * t);
    const float *py = (const float*)&young->params;
    const float *pe = (const float*)&elder->params;
    float       *pp = (float*)&m->base.params;
    size_t       n  = sizeof(RigFaceParams) / sizeof(float);
    for (size_t i = 0; i < n; i++)
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
        for (uint32_t i = 0; i < m->base.n_verts; i++) {
            RigFaceVertex *v = &m->base.verts[i];
            if (v->pos.y < 0.0f)
                v->pos.y -= sag * fabsf(v->pos.y);
        }
    }
    m->phi_coherence = V2_PHI_INV * (1.0f - t_smooth * 0.1f);
}
RigFaceIrisDetail rig_iris_default(float melanin)
{
    RigFaceIrisDetail ir;
    memset(&ir, 0, sizeof(ir));
    ir.iris_color[0] = 0.20f + melanin * 0.60f;
    ir.iris_color[1] = 0.25f + melanin * 0.35f;
    ir.iris_color[2] = 0.60f - melanin * 0.55f;
    ir.iris_color[3] = 1.0f;
    ir.pupil_radius  = 0.35f;
    ir.iris_radius   = 1.0f;
    ir.limbal_ring_width = 0.08f;
    ir.limbal_ring_darkness = 0.7f;
    ir.crypt_density  = 0.75f;
    ir.wolfflin_count = 8.0f + melanin*8.0f;
    ir.pupil_radius = 2.5f;
    ir.crypt_depth = 0.15f;
    ir.furrow_count = 8.0f;
    ir.cornea_roughness = 0.04f;
    ir.tear_film_thickness = 40.0f;
    return ir;
}
RigFaceIrisDetail rig_iris_from_params(const RigFaceParams *p)
{
    return p ? rig_iris_default(p->melanin) : rig_iris_default(0.3f);
}
int rig_face_build_iris_geometry(RigFaceMeshV2 *m, bool right)
{
    if (!m) return 0;
    RigFaceIrisDetail *ir = right ? &m->iris_right : &m->iris_left;
    ir->pupil_radius = fmaxf(0.20f, fminf(0.60f, ir->pupil_radius));
    (void)right;
}
int rig_face_bake_iris_texture(const RigFaceIrisDetail *iris,
                                 uint8_t *out_rgba, uint32_t w, uint32_t h)
{
    if (!iris || !out_rgba) return 0;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            float u  = (float)x / (float)w * 2.0f - 1.0f;
            float v  = (float)y / (float)h * 2.0f - 1.0f;
            float r  = sqrtf(u*u + v*v);
            float a  = atan2f(v, u);
            uint8_t *px = out_rgba + (y*w + x)*4;
            if (r > iris->iris_radius) {
                px[0] = 255; px[1] = 255; px[2] = 255; px[3] = 255;
            } else if (r < iris->pupil_radius) {
                px[0] = 5; px[1] = 5; px[2] = 5; px[3] = 255;
            } else {
                float cell = v2_noise(0x1234, r * iris->crypt_density * 64.0f,
                                      a * iris->crypt_density * 64.0f);
                float c0 = iris->iris_color[0] * cell;
                float c1 = iris->iris_color[1] * cell;
                float c2 = iris->iris_color[2] * cell;
                px[0] = (uint8_t)(fminf(c0, 1.0f) * 255.0f);
                px[1] = (uint8_t)(fminf(c1, 1.0f) * 255.0f);
                px[2] = (uint8_t)(fminf(c2, 1.0f) * 255.0f);
                px[3] = 255;
            }
        }
    }
}
int rig_iris_set_pupil_dilation(RigFaceIrisDetail *iris, float lux)
{
    if (!iris) return 0;
    float dil = 0.60f - lux * 0.35f;
    iris->pupil_radius = fmaxf(1.0f, fminf(5.5f, dil * 6.5f));
}
RigFaceEarParams rig_ear_default_params(bool is_right)
{
    RigFaceEarParams e;
    memset(&e, 0, sizeof(e));
    e.ear_height     = 6.5f;
    e.ear_width       = 3.2f;
    e.helix_curl      = 0.4f;
    e.antihelix_split = 0.3f;
    e.lobule_size    = 1.8f;
    e.lobule_attachment = 0.0f;
    e.tragus_size     = 0.6f;
    e.is_right        = is_right;
    return e;
}
int rig_face_build_ear_geometry(RigFaceMeshV2 *m, bool right)
{
    if (!m) return 0;
    RigFaceEarParams *ep = right ? &m->ear_right_params : &m->ear_left_params;
    uint32_t n_verts = 128, n_tris = 220;
    RigFaceVertex **pverts = right ? &m->ear_right_verts : &m->ear_left_verts;
    RigFaceTri    **ptris  = right ? &m->ear_right_tris  : &m->ear_left_tris;
    uint32_t *pnv = right ? &m->ear_right_n_verts : &m->ear_left_n_verts;
    uint32_t *pnt = right ? &m->ear_right_n_tris  : &m->ear_left_n_tris;
    free(*pverts); free(*ptris);
    *pverts = (RigFaceVertex*)calloc(n_verts, sizeof(RigFaceVertex));
    *ptris  = (RigFaceTri*)   calloc(n_tris,  sizeof(RigFaceTri));
    if (!*pverts || !*ptris) return 0;
    *pnv = n_verts; *pnt = n_tris;
    float sign = right ? 1.0f : -1.0f;
    float lx   = sign * (m->base.params.cranium_width * 0.5f + 0.5f);
    for (uint32_t i = 0; i < n_verts; i++) {
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
    uint32_t ti = 0;
    for (uint32_t i = 0; i < n_verts - 2 && ti < n_tris; i++, ti++) {
        (*ptris)[ti].a = i;
        (*ptris)[ti].b = i + 1;
        (*ptris)[ti].c = (i + 2) % n_verts;
    }
    (void)ep;
}
int rig_face_attach_ears(RigFaceMeshV2 *m)
{
    if (!m) return 0;
    if (!m->ear_left_verts)  rig_face_build_ear_geometry(m, false);
    if (!m->ear_right_verts) rig_face_build_ear_geometry(m, true);
}
RigFaceLipDetail rig_lip_default_params(float gender_factor)
{
    RigFaceLipDetail lip;
    memset(&lip, 0, sizeof(lip));
    lip.upper_lip_height = 8.0f + (1.0f - gender_factor) * 2.0f;
    lip.lower_lip_height = 10.0f + (1.0f - gender_factor) * 2.5f;
    lip.cupid_bow_depth = 0.25f + (1.0f - gender_factor) * 0.15f;
    lip.commissure_depth = 0.5f;
    lip.philtrum_depth = 4.5f;
    lip.vermilion_height = 3.5f;
    lip.lip_line_density = 0.6f;
    lip.lip_protrusion  = 2.5f + (1.0f - gender_factor) * 1.5f;
    lip.vermilion_saturation = 0.60f + (1.0f - gender_factor) * 0.25f;
    lip.mucosal_visibility   = 0.30f + (1.0f - gender_factor) * 0.20f;
    return lip;
}
int rig_face_sculpt_lips_v2(RigFaceMeshV2 *m)
{
    if (!m || !m->base.verts) return 0;
    const RigFaceLipDetail *lip = &m->lip;
    float my = -(m->base.params.lower_third * 0.3f);
    for (uint32_t i = 0; i < m->base.n_verts; i++) {
        RigFaceVertex *v = &m->base.verts[i];
        float dy = v->pos.y - my;
        if (fabsf(dy) < lip->upper_lip_height * 0.1f &&
            fabsf(v->pos.x) < m->base.params.mouth_width * 0.6f) {
            v->pos.z += lip->vermilion_height * 0.005f
                      * expf(-v->pos.x*v->pos.x * 0.1f)
                      * expf(-dy*dy * 2.0f);
        }
    }
}
int rig_face_bake_lip_color_map(const RigFaceMeshV2 *m,
                                  uint8_t *out_rgba, uint32_t w, uint32_t h)
{
    if (!m || !out_rgba) return 0;
    const RigFaceLipDetail *lip = &m->lip;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            float u = (float)x / (float)w;
            float v = (float)y / (float)h;
            float dx   = u*2.0f - 1.0f;
            float bow  = sinf(dx * V2_PI) * lip->cupid_bow_depth;
            float dist = fabsf(v*2.0f - 1.0f - bow);
            float fade = fmaxf(0.0f, 1.0f - dist * 1.5f);
            float sat  = lip->vermilion_saturation;
            uint8_t *px = out_rgba + (y*w + x)*4;
            px[0] = (uint8_t)(0.78f * sat * fade * 255.0f);
            px[1] = (uint8_t)(0.35f * sat * fade * 255.0f);
            px[2] = (uint8_t)(0.32f * sat * fade * 255.0f);
            px[3] = (uint8_t)(fade * 255.0f);
        }
    }
}
RigAgeProgression rig_age_progression_create(const RigFaceParams *base)
{
    RigAgeProgression ap;
    memset(&ap, 0, sizeof(ap));
    if (!base) return ap;
    ap.milestones[0].params         = *base;
    ap.milestones[0].age            = base->age_factor * 100.0f;
    ap.milestones[0].wrinkle_depth  = base->age_factor * 0.08f;
    ap.milestones[0].ptosis_upper   = base->age_factor * 0.20f;
    ap.milestones[0].jowl_factor    = base->age_factor * 0.30f;
    ap.milestones[0].nasolabial_depth = base->age_factor * 0.40f;
    ap.current_age = base->age_factor * 100.0f;
    return ap;
}
RigFaceParams rig_age_interpolate(const RigAgeProgression *prog, float age)
{
    if (!prog) return rig_face_default_params();
    RigFaceParams p = prog->milestones[0].params;
    float t = (age - prog->current_age) / 50.0f;
    t = fmaxf(-1.0f, fminf(1.0f, t));
    p.age_factor = fmaxf(0.0f, fminf(1.0f, p.age_factor + t * 0.5f));
    return p;
}
int rig_face_bake_pore_normal_map(RigFaceMeshV2 *m, uint32_t w, uint32_t h)
{
    m->texture_width = w; m->texture_height = h;
    rig_face_build_pore_system(m, 1.0f);
}
int rig_face_bake_vascular_map(RigFaceMeshV2 *m, uint32_t w, uint32_t h)
{
    m->texture_width = w; m->texture_height = h;
    rig_face_build_vascular_tree(m);
}
int rig_face_bake_wrinkle_normal_map(RigFaceMeshV2 *m, uint32_t w, uint32_t h)
{
    m->texture_width = w; m->texture_height = h;
    rig_face_build_wrinkle_lines(m, m->base.params.age_factor, 0.0f);
}
int rig_face_v2_export_obj(const RigFaceMeshV2 *m, const char *path)
{
    if (!m || !path) return -1;
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "# RigCom v24 THE SANTORIUM OF COMPILER — RigFaceMeshV2 OBJ export\n");
    fprintf(f, "# phi=%.16f vertices=%u tris=%u\n",
            (double)V2_PHI, m->base.n_verts, m->base.n_tris);
    for (uint32_t i = 0; i < m->base.n_verts; i++) {
        const RigFaceVertex *v = &m->base.verts[i];
        fprintf(f, "v  %.6f %.6f %.6f\n",
                (double)v->pos.x, (double)v->pos.y, (double)v->pos.z);
    }
    for (uint32_t i = 0; i < m->base.n_verts; i++) {
        const RigFaceVertex *v = &m->base.verts[i];
        fprintf(f, "vn %.6f %.6f %.6f\n",
                (double)v->normal.x, (double)v->normal.y, (double)v->normal.z);
    }
    for (uint32_t i = 0; i < m->base.n_verts; i++) {
        const RigFaceVertex *v = &m->base.verts[i];
        fprintf(f, "vt %.6f %.6f\n", (double)v->uv.x, (double)v->uv.y);
    }
    for (uint32_t i = 0; i < m->base.n_tris; i++) {
        const RigFaceTri *t = &m->base.tris[i];
        fprintf(f, "f %u/%u/%u %u/%u/%u %u/%u/%u\n",
                t->a+1, t->a+1, t->a+1,
                t->b+1, t->b+1, t->b+1,
                t->c+1, t->c+1, t->c+1);
    }
    fclose(f);
    return 0;
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
#include "rig_math.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "../include/rig_face_engine_v2.h"
/* Hash pseudo-aleatorio determinista para sampling */
static float _hash2(uint32_t seed, uint32_t idx) {
    uint32_t h = seed ^ (idx * 2654435761u);
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = ((h >> 16) ^ h) * 0x45d9f3bu;
    h = (h >> 16) ^ h;
    return (float)(h & 0xFFFFu) / 65535.0f;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: _hash2 -> rigpub_rig_face_engine_v2__hash2 */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
/* Región de un vértice basada en UV (mapping simplificado por zona facial) */
static uint8_t _vertex_region(const RigFaceVertex *v) {
    /* Mapa de regiones por posición normalizada:
       y > 0.7  → cuero cabelludo (región 0)
       y > 0.5, |x| < 0.15 → frente (1)
       |x| > 0.35, y > 0.3 → sienes (2,3)
       y > 0.3, |x| < 0.3  → cuero frontal (4)
       |x| < 0.1, y ∈ [0.1,0.3] → nariz (5)
       y < 0.1             → labio superior/inferior (6,7)
       y < 0 (mentón)      → mentón (8)
       zonas orbitales y mejillas → 9-20
    */
    float x = v->pos.x, y = v->pos.y;
    float ax = fabsf(x);
    if      (y > 0.70f)                              return 0;   /* cuero cabelludo */
    else if (y > 0.50f && ax < 0.15f)               return 1;   /* frente central */
    else if (y > 0.30f && ax > 0.35f)               return (x > 0) ? 2 : 3;  /* sienes */
    else if (y > 0.30f && ax < 0.30f)               return 4;   /* frente inf */
    else if (ax < 0.10f && y > 0.10f && y < 0.30f) return 5;   /* nariz */
    else if (y < 0.10f && y > 0.00f)                return 6;   /* labio superior */
    else if (y < 0.00f && y > -0.08f)               return 7;   /* labio inferior */
    else if (y < -0.08f)                             return 8;   /* mentón */
    else if (ax > 0.20f && y > 0.10f)               return (x > 0) ? 9 : 10; /* mejillas */
    else                                             return 11;  /* resto */
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: _vertex_region -> rigpub_rig_face_engine_v2__vertex_region */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
int rig_face_build_follicle_map(RigFaceMeshV2 *mesh,
                                   const RigHairRegionParams *hair) {
    if (!mesh || !hair) return 0;
    /* Acceso a la malla base */
    RigFaceMesh *base = &mesh->base;
    if (!base->verts || base->n_verts == 0) return 0;
    /* Limpiar folículos existentes */
    mesh->microdetail.n_follicles = 0;
    uint32_t n_placed = 0;
    uint32_t seed = 0xC47ED1A1u ^ base->n_verts;
    /* Calcular área superficial aproximada del cráneo para escalar densidad */
    /* Área ≈ 4π×r_medio² × factores elipsoidales ~ 600 cm² para cabeza humana */
    const float AREA_CM2 = 580.0f;
    /* Para cada región procesamos los vértices que caen en ella */
    for (uint32_t reg = 0; reg < RIG_REGION_COUNT && n_placed < RIG_FACE_FOLLICLE_MAX; reg++) {
        const RigHairRegionParams *hr = &hair[reg];
        if (hr->density < 1e-4f) continue;
        /* Densidad en folículos/cm²: tipicamente 80-120 para cuero cabelludo,
           vello: ~20-40, sin pelo: 0-5                                        */
        float follicles_per_cm2 = hr->density;
        /* Fracción de área en esta región (simplificado: uniforme) */
        float region_area = AREA_CM2 / (float)RIG_REGION_COUNT;
        uint32_t n_target = (uint32_t)(follicles_per_cm2 * region_area);
        if (n_target == 0) n_target = 1;
        /* Recolectar vértices de esta región */
        uint32_t reg_verts[512];
        uint32_t n_rv = 0;
        for (uint32_t vi = 0; vi < base->n_verts && n_rv < 512; vi++) {
            if (_vertex_region(&base->verts[vi]) == (uint8_t)reg)
                reg_verts[n_rv++] = vi;
        }
        if (n_rv == 0) continue;
        /* Poisson-disk aproximado: elegir n_target vértices aleatorios
           sin repetición (mínima distancia entre ellos)                   */
        uint32_t n_place_reg = (n_target < (RIG_FACE_FOLLICLE_MAX - n_placed))
                               ? n_target : (RIG_FACE_FOLLICLE_MAX - n_placed);
        /* Radio de exclusión mínimo (cm) según densidad */
        float excl_r = (follicles_per_cm2 > 0)
                       ? sqrtf(1.0f / (3.14159f * follicles_per_cm2)) * 0.8f
                       : 0.5f;
        for (uint32_t k = 0; k < n_place_reg; k++) {
            /* Elegir vértice candidato pseudo-aleatorio */
            uint32_t try_count = 0;
            bool placed = false;
            while (try_count < 16 && !placed) {
                float rnd = _hash2(seed + reg * 4096 + k, try_count++);
                uint32_t vi = reg_verts[(uint32_t)(rnd * (float)n_rv) % n_rv];
                Vec3f vp = base->verts[vi].pos;
                /* Verificar distancia mínima a folículos ya colocados
                   (revisar últimos N colocados para no ser O(N²) completo) */
                bool too_close = false;
                uint32_t check_start = (n_placed > 32) ? n_placed - 32 : 0;
                for (uint32_t fi = check_start; fi < n_placed; fi++) {
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
                fo->diameter = hr->diameter * (0.9f + 0.2f * _hash2(seed, n_placed));
                fo->length   = hr->length   * (0.85f + 0.3f * _hash2(seed + 1, n_placed));
                fo->curl     = hr->curl_radius;
                fo->melanin  = hr->melanin_eu + hr->melanin_phe * 0.5f;
                fo->region   = (uint8_t)reg;
                n_placed++;
                placed = true;
            }
        }
    }
    mesh->microdetail.n_follicles = n_placed;
    /* Actualizar coherencia φ (relación folículos/densidad_esperada) */
    float expected = 0;
    for (uint32_t r = 0; r < RIG_REGION_COUNT; r++)
        expected += hair[r].density * (AREA_CM2 / (float)RIG_REGION_COUNT);
    mesh->phi_coherence = (expected > 0)
        ? fminf(1.0f, (float)n_placed / expected)
        : 0.0f;
}
/* ================================================================
 * FUENTE ABSORBIDA: 16_FACE_NG/src/rig_face_v2.c
 * ================================================================ */
#line 1 "16_FACE_NG/src/rig_face_v2.c"
/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "rig_face_v2.h"
#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_math.h"

#include "rig_syscall.h"
#ifndef RIG_PI
#define RIG_PI          3.14159265358979323846
#endif
#define RIG_TWO_PI      6.28318530717958647692
#ifndef RIG_PHI
#define RIG_PHI         1.6180339887498948482
#endif
#ifndef RIG_PHI_INV
#define RIG_PHI_INV     0.6180339887498948482
#endif
#define RIG_SQRT2       1.41421356237309504880
#define FACE_SHADER_BUF 65536
#define FACE_JS_BUF     32768
#define FA(buf, sz, pos, ...) do {                                      \
    if ((pos) < (int)(sz)) {                                            \
        int _n = snprintf((buf)+(pos), (sz)-(pos), __VA_ARGS__);        \
        if (_n > 0) (pos) += _n;                                        \
    }                                                                   \
} while(0)
static float fc_clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: fc_clamp -> rigpub_rig_face_v2_fc_clamp */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
static float fc_lerp(float a, float b, float t) { return a + (b - a) * t; }
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: fc_lerp -> rigpub_rig_face_v2_fc_lerp */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
static float fc_smoothstep(float e0, float e1, float x) {
    float t = fc_clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: fc_smoothstep -> rigpub_rig_face_v2_fc_smoothstep */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
static const char *GLSL_HEADER =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "const float PHI     = 1.6180339887;\n"
    "const float PHI_INV = 0.6180339887;\n"
    "const float PI      = 3.14159265359;\n"
    "const float TAU     = 6.28318530718;\n"
    "const float SCHUMANN= 7.83;\n\n";
static const char *GLSL_NOISE_FUNCS =
    "// ── Noise utilities ──────────────────────────────────────────\n"
    "float hash11(float p) {\n"
    "  p = fract(p * 0.1031); p *= p + 33.33; return fract(p*(p+p));\n"
    "}\n"
    "float hash21(vec2 p) {\n"
    "  vec3 p3 = fract(vec3(p.xyx)*0.1031);\n"
    "  p3 += dot(p3, p3.yzx+33.33); return fract((p3.x+p3.y)*p3.z);\n"
    "}\n"
    "float hash31(vec3 p) {\n"
    "  p = fract(p*0.1031); p += dot(p, p.yzx+33.33);\n"
    "  return fract((p.x+p.y)*p.z);\n"
    "}\n"
    "float vnoise(vec3 p) {\n"
    "  vec3 i=floor(p), f=fract(p);\n"
    "  vec3 u=f*f*(3.0-2.0*f);\n"
    "  return mix(mix(mix(hash31(i),         hash31(i+vec3(1,0,0)),u.x),\n"
    "                 mix(hash31(i+vec3(0,1,0)),hash31(i+vec3(1,1,0)),u.x),u.y),\n"
    "             mix(mix(hash31(i+vec3(0,0,1)),hash31(i+vec3(1,0,1)),u.x),\n"
    "                 mix(hash31(i+vec3(0,1,1)),hash31(i+vec3(1,1,1)),u.x),u.y),u.z);\n"
    "}\n"
    "float fbm(vec3 p, int oct) {\n"
    "  float v=0.,a=0.5; for(int i=0;i<oct;i++){v+=a*vnoise(p);p*=2.;a*=.5;}\n"
    "  return v;\n"
    "}\n"
    "// φ-noise: succession of Fibonacci\n"
    "float phi_noise(vec3 p) {\n"
    "  float n = p.x*PHI + p.y*PHI*PHI + p.z*PHI*PHI*PHI;\n"
    "  return fract(sin(n*127.1+12.9898)*43758.5453);\n"
    "}\n"
    "// Voronoi\n"
    "vec2 voronoi(vec2 uv, float scale) {\n"
    "  vec2 p = uv*scale; vec2 f=fract(p), i=floor(p);\n"
    "  float md=8.; vec2 mc=vec2(0.);\n"
    "  for(int x=-1;x<=1;x++) for(int y=-1;y<=1;y++) {\n"
    "    vec2 n=vec2(float(x),float(y));\n"
    "    vec2 c=vec2(hash21(i+n),hash21(i+n+vec2(31.4,27.1)));\n"
    "    c+=n; float d=length(f-c);\n"
    "    if(d<md){md=d;mc=c;}\n"
    "  }\n"
    "  return vec2(md,hash21(mc));\n"
    "}\n\n";
static const char *GLSL_PBR_FUNCS =
    "// ── PBR Core ──────────────────────────────────────────────────\n"
    "vec3 fresnelSchlick(float cosT, vec3 F0) {\n"
    "  return F0 + (1.0-F0)*pow(clamp(1.0-cosT,0.,1.),5.0);\n"
    "}\n"
    "float ggxNDF(vec3 N, vec3 H, float r) {\n"
    "  float a=r*r,a2=a*a,NdH=max(dot(N,H),0.);\n"
    "  float d=NdH*NdH*(a2-1.)+1.; return a2/(PI*d*d);\n"
    "}\n"
    "float schlickGGX(float NdV, float r) {\n"
    "  float k=(r+1.)*(r+1.)/8.; return NdV/(NdV*(1.-k)+k);\n"
    "}\n"
    "vec3 aces_tonemap(vec3 c) {\n"
    "  return c*(c+0.0245786)/(c*(0.983729*c+0.432951)+0.238081);\n"
    "}\n\n";
int rig_face_v2_hair_gen(const RigFaceHairCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)malloc(FACE_SHADER_BUF);
    char *vert = (char*)malloc(FACE_SHADER_BUF);
    char *js   = (char*)malloc(FACE_JS_BUF);
    if (!frag || !vert || !js) { free(frag); free(vert); free(js); return -1; }
    int fp = 0, vp = 0, jp = 0;
    int sz = FACE_SHADER_BUF, jsz = FACE_JS_BUF;
    FA(vert, sz, vp, "%s", GLSL_HEADER);
    FA(vert, sz, vp,
        "in vec3 a_pos;\n"
        "in vec3 a_tangent;    // Dirección del cabello (T)\n"
        "in vec2 a_uv;\n"
        "in float a_strand_t; // 0=raíz 1=punta\n"
        "in float a_strand_id;\n"
        "uniform mat4 u_mvp;\n"
        "uniform mat4 u_model;\n"
        "uniform float u_time;\n"
        "uniform float u_wind_strength;\n"
        "uniform float u_wind_freq;\n"
        "uniform float u_stiffness;    // %.4f\n"
        "uniform float u_gravity;      // %.4f\n"
        "out vec3  v_tangent;\n"
        "out vec2  v_uv;\n"
        "out float v_strand_t;\n"
        "out float v_strand_id;\n"
        "out vec3  v_world_pos;\n"
        "void main() {\n"
        "  vec3 pos = a_pos;\n"
        "  // Physics: gravedad + wind, mayor influencia hacia la punta\n"
        "  float influence = a_strand_t * a_strand_t;\n"
        "  float wind = sin(u_time*u_wind_freq + a_strand_id*PHI)*u_wind_strength;\n"
        "  pos.x += wind * influence * (1.0 - u_stiffness);\n"
        "  pos.y -= u_gravity * influence * 0.1;\n"
        "  // Curl: deformación ondulante φ\n"
        "  float curl = sin(a_strand_t * TAU * PHI + a_strand_id) * %.4f;\n"
        "  pos.x += curl * influence;\n"
        "  v_tangent   = normalize(mat3(u_model)*a_tangent);\n"
        "  v_uv        = a_uv;\n"
        "  v_strand_t  = a_strand_t;\n"
        "  v_strand_id = a_strand_id;\n"
        "  v_world_pos = (u_model * vec4(pos,1.0)).xyz;\n"
        "  gl_Position = u_mvp * vec4(pos,1.0);\n"
        "}\n",
        ctx->stiffness, ctx->gravity_pull, ctx->wave_amplitude);
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp,
        "in vec3  v_tangent;\n"
        "in vec2  v_uv;\n"
        "in float v_strand_t;\n"
        "in float v_strand_id;\n"
        "in vec3  v_world_pos;\n"
        "out vec4 fragColor;\n"
        "uniform vec3  u_root_color;    // (%.4f, %.4f, %.4f)\n"
        "uniform vec3  u_tip_color;     // (%.4f, %.4f, %.4f)\n"
        "uniform vec3  u_highlight_color;\n"
        "uniform float u_highlight_str; // %.4f\n"
        "uniform float u_kajiya_shift;  // %.4f — especular desplazamiento\n"
        "uniform float u_roughness1;    // %.4f\n"
        "uniform float u_roughness2;    // %.4f — lóbulo secundario\n"
        "uniform float u_transmittance; // %.4f\n"
        "uniform float u_melanin_eu;    // %.4f\n"
        "uniform float u_melanin_ph;    // %.4f\n"
        "uniform float u_frizz;         // %.4f\n"
        "uniform vec3  u_light_dir;\n"
        "uniform vec3  u_view_dir;\n"
        "uniform float u_time;\n\n",
        ctx->root_color.x, ctx->root_color.y, ctx->root_color.z,
        ctx->tip_color.x, ctx->tip_color.y, ctx->tip_color.z,
        ctx->highlight_strength,
        ctx->kajiya_specular_shift,
        ctx->kajiya_roughness,
        ctx->kajiya_roughness * 0.5f,
        ctx->transmittance,
        ctx->melanin_eumelanin,
        ctx->melanin_pheomelanin,
        ctx->frizz_amount);
    FA(frag, sz, fp,
        "// ── Kajiya-Kay BRDF ──────────────────────────────────────\n"
        "float kajiya_diffuse(vec3 T, vec3 L) {\n"
        "  return sqrt(max(0., 1.0 - pow(dot(T,L),2.)));\n"
        "}\n"
        "float kajiya_spec(vec3 T, vec3 L, vec3 V, float shift, float roughness) {\n"
        "  vec3 H = normalize(L+V);\n"
        "  float TdotH = dot(T, H);\n"
        "  float sinTH = sqrt(max(0., 1.0 - TdotH*TdotH));\n"
        "  float dirAtten = fc_step(-1., sinTH);\n"
        "  return dirAtten * pow(sinTH, 1.0/max(roughness*roughness,0.001));\n"
        "}\n"
        "float fc_step(float e, float x){ return x>e?1.:0.; }\n\n"
        "void main() {\n"
        "  vec3 T = normalize(v_tangent);\n"
        "  // Frizz perturbación de la tangente\n"
        "  T += phi_noise(v_world_pos*8.)*u_frizz*0.2;\n"
        "  T = normalize(T);\n"
        "  // Color por gradiente raíz→punta\n"
        "  vec3 base = mix(u_root_color, u_tip_color, v_strand_t);\n"
        "  // Variación por melanina\n"
        "  float eu = u_melanin_eu, ph = u_melanin_ph;\n"
        "  base *= vec3(1.0-eu*0.6, 1.0-eu*0.4, 1.0-eu*0.8);\n"
        "  base += vec3(ph*0.3, ph*0.15, 0.0);\n"
        "  base = clamp(base, 0., 1.);\n"
        "  // Frizz variación de color\n"
        "  base += (hash11(v_strand_id*PHI)-0.5)*u_frizz*0.08;\n\n"
        "  vec3 L = normalize(u_light_dir);\n"
        "  vec3 V = normalize(u_view_dir);\n"
        "  // Tangente desplazada para especular dual\n"
        "  vec3 T1 = normalize(T + u_kajiya_shift*vec3(0.0,1.0,0.0));\n"
        "  vec3 T2 = normalize(T - u_kajiya_shift*0.5*vec3(0.0,1.0,0.0));\n"
        "  float diff  = kajiya_diffuse(T, L);\n"
        "  float spec1 = kajiya_spec(T1, L, V, u_kajiya_shift, u_roughness1);\n"
        "  float spec2 = kajiya_spec(T2, L, V, u_kajiya_shift*0.5, u_roughness2);\n"
        "  // Highlight (Schellman stripe)\n"
        "  vec3  spec_col = mix(u_highlight_color, vec3(1.), 0.5)*u_highlight_str;\n"
        "  // Transmittance (backscatter)\n"
        "  float bscat = max(0., dot(-L, V)) * u_transmittance;\n"
        "  vec3 color = base*diff + spec_col*(spec1+spec2*0.4) + base*bscat*0.3;\n"
        "  // Clumping: oscurece donde los filamentos se agrupan\n"
        "  float clump = hash11(floor(v_strand_id*%.1f)/%.1f);\n"
        "  color *= 0.85 + 0.15*clump;\n"
        "  // ACES\n"
        "  color = aces_tonemap(color);\n"
        "  color = pow(clamp(color,0.,1.), vec3(1./2.2));\n"
        "  // Transparencia por punta de hebra\n"
        "  float alpha = mix(1.0, 0.0, pow(v_strand_t, 3.0));\n"
        "  fragColor = vec4(color, alpha);\n"
        "}\n",
        fc_clamp(ctx->clump_factor * 10.0f, 2.0f, 20.0f),
        fc_clamp(ctx->clump_factor * 10.0f, 2.0f, 20.0f));
    FA(js, jsz, jp,
        "// §F1 RigFace Hair — Strand Geometry Emitter\n"
        "// strand_count=%u · segments=%u · physics=%s\n"
        "function rigHairGenStrands(ctx, scalp_verts, scalp_normals) {\n"
        "  const STRANDS = %u, SEGS = %u;\n"
        "  const verts=[], uvs=[], tangents=[], strandT=[], strandID=[];\n"
        "  const indices=[];\n"
        "  for(let s=0;s<STRANDS;s++) {\n"
        "    const si = Math.floor(Math.random()*scalp_verts.length/3)*3;\n"
        "    const rx=scalp_verts[si],ry=scalp_verts[si+1],rz=scalp_verts[si+2];\n"
        "    const nx=scalp_normals[si],ny=scalp_normals[si+1],nz=scalp_normals[si+2];\n"
        "    const len = %.4f*(0.8+Math.random()*0.4);\n"
        "    // Curl φ-parametric\n"
        "    const curl = %.4f, freq=%.4f;\n"
        "    for(let g=0;g<=SEGS;g++) {\n"
        "      const t = g/SEGS;\n"
        "      const cx=Math.sin(t*Math.PI*2*freq+s)*curl*t;\n"
        "      const cy=Math.sin(t*Math.PI*2*freq*1.618+s*2.71)*curl*t;\n"
        "      verts.push(rx+nx*len*t+cx, ry+ny*len*t-t*t*%.3f, rz+nz*len*t+cy);\n"
        "      uvs.push(s/STRANDS, t);\n"
        "      tangents.push(nx+cx*.1, ny-.2*t, nz+cy*.1);\n"
        "      strandT.push(t);\n"
        "      strandID.push(s);\n"
        "    }\n"
        "    for(let g=0;g<SEGS;g++) {\n"
        "      const b=s*(SEGS+1)+g;\n"
        "      indices.push(b, b+1);\n"
        "    }\n"
        "  }\n"
        "  return {verts:new Float32Array(verts),uvs:new Float32Array(uvs),\n"
        "          tangents:new Float32Array(tangents),strandT:new Float32Array(strandT),\n"
        "          strandID:new Float32Array(strandID),indices:new Uint32Array(indices)};\n"
        "}\n",
        ctx->strand_count, ctx->segments_per_strand,
        ctx->enable_physics_springs ? "true" : "false",
        ctx->strand_count, ctx->segments_per_strand,
        ctx->length,
        ctx->wave_amplitude, ctx->wave_frequency,
        ctx->gravity_pull * 0.02f);
    out->glsl_vert = vert;
    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    out->phi_ratio = RIG_PHI;
    out->certeza   = 1.0f;
    return fp + vp + jp;
}
int rig_face_v2_eye_shader(const RigFaceEyeCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)malloc(FACE_SHADER_BUF);
    char *js   = (char*)malloc(FACE_JS_BUF);
    if (!frag || !js) { free(frag); free(js); return -1; }
    int fp = 0, jp = 0;
    int sz = FACE_SHADER_BUF, jsz = FACE_JS_BUF;
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp, "%s", GLSL_PBR_FUNCS);
    FA(frag, sz, fp,
        "in vec2 v_uv;\n"
        "in vec3 v_normal;\n"
        "in vec3 v_pos;\n"
        "out vec4 fragColor;\n\n"
        "// ── Eye uniforms ──────────────────────────────────────────\n"
        "uniform float u_pupil_dilation;     // %.4f\n"
        "uniform float u_pupil_roundness;    // %.4f\n"
        "uniform vec3  u_iris_inner;         // (%.3f,%.3f,%.3f)\n"
        "uniform vec3  u_iris_outer;         // (%.3f,%.3f,%.3f)\n"
        "uniform vec3  u_limbal_ring;        // dark ring\n"
        "uniform float u_iris_fiber_density; // %.4f\n"
        "uniform float u_iris_fiber_contrast;// %.4f\n"
        "uniform float u_corneal_bulge;      // %.4f\n"
        "uniform float u_corneal_ior;        // %.4f\n"
        "uniform float u_corneal_roughness;  // %.4f\n"
        "uniform float u_corneal_wetness;    // %.4f\n"
        "uniform float u_sclera_redness;     // %.4f\n"
        "uniform float u_sclera_yellowness;  // %.4f\n"
        "uniform float u_sclera_veins;       // %.4f\n"
        "uniform vec3  u_light_dir;\n"
        "uniform vec3  u_view_dir;\n"
        "uniform float u_time;\n\n",
        ctx->pupil_dilation, ctx->pupil_roundness,
        ctx->iris_color_inner.x, ctx->iris_color_inner.y, ctx->iris_color_inner.z,
        ctx->iris_color_outer.x, ctx->iris_color_outer.y, ctx->iris_color_outer.z,
        ctx->iris_fiber_density, ctx->iris_fiber_contrast,
        ctx->corneal_bulge, ctx->corneal_ior,
        ctx->corneal_roughness, ctx->corneal_wetness,
        ctx->sclera_redness, ctx->sclera_yellowness,
        ctx->sclera_vein_density);
    FA(frag, sz, fp,
        "// ── Parallax refraction (Tompkin 2000) ───────────────────\n"
        "vec2 corneal_parallax(vec2 uv, vec3 view, float bulge, float ior) {\n"
        "  vec3 N = normalize(vec3(uv*2.-1., bulge));\n"
        "  float eta = 1.0 / ior;\n"
        "  vec3 refr = refract(-normalize(view), N, eta);\n"
        "  return uv + refr.xy * 0.08 * bulge;\n"
        "}\n\n"
        "// ── Iris procedural layer ─────────────────────────────────\n"
        "vec3 iris_color(vec2 uv, float dilation) {\n"
        "  vec2 c = uv*2.-1.;\n"
        "  float r = length(c);\n"
        "  float theta = atan(c.y, c.x);\n"
        "  // Pupila — forma elíptica controlada por dilation\n"
        "  float pupil_r = mix(0.18, 0.55, dilation);\n"
        "  float in_pupil = smoothstep(pupil_r, pupil_r-0.02, r);\n"
        "  // Iris radial gradient\n"
        "  float iris_r = 0.85;\n"
        "  float in_iris = smoothstep(iris_r, iris_r-0.02, r) * (1.-in_pupil);\n"
        "  // Fibras del estroma — Voronoi radial\n"
        "  vec2 polar = vec2(theta/(TAU)*u_iris_fiber_density*12., r*6.);\n"
        "  vec2 vor = voronoi(polar, 1.);\n"
        "  float fiber = vor.x * u_iris_fiber_contrast;\n"
        "  // Criptas (depresiones oscuras en el estroma)\n"
        "  float crypt = 1.0 - smoothstep(0.0, 0.15, vor.x) * 0.35;\n"
        "  // Color iris interpolado center→edge\n"
        "  vec3 col = mix(u_iris_inner, u_iris_outer, r/iris_r);\n"
        "  col *= (0.8 + 0.2*fiber) * crypt;\n"
        "  // Anillo límbal (más oscuro en el borde)\n"
        "  float limbal = smoothstep(iris_r-0.1, iris_r, r);\n"
        "  col = mix(col, u_limbal_ring, limbal*0.85);\n"
        "  // Pupila = negro\n"
        "  col = mix(col, vec3(0.02,0.01,0.01), in_pupil);\n"
        "  // Esclerótica\n"
        "  if(r > iris_r) {\n"
        "    vec3 sclera = vec3(0.96, 0.93, 0.89);\n"
        "    sclera.r += u_sclera_redness * 0.15;\n"
        "    sclera.gb -= u_sclera_yellowness * vec2(0.04,0.06);\n"
        "    // Venillas\n"
        "    float vein_n = fbm(vec3(c*8.,0.),3);\n"
        "    sclera.r += u_sclera_veins*vein_n*0.12;\n"
        "    col = sclera;\n"
        "  }\n"
        "  return col * in_iris + (r>iris_r ? col : vec3(0.));\n"
        "}\n\n"
        "void main() {\n"
        "  vec3  V = normalize(u_view_dir);\n"
        "  // Parallax corneal\n"
        "  vec2  uv = corneal_parallax(v_uv, V, u_corneal_bulge, u_corneal_ior);\n"
        "  vec3  col = iris_color(uv, u_pupil_dilation);\n"
        "  // Specular corneal (película lagrimal)\n"
        "  vec3  L = normalize(u_light_dir);\n"
        "  vec3  H = normalize(L+V);\n"
        "  vec3  N = normalize(v_normal);\n"
        "  vec3  F0 = vec3(0.04);\n"
        "  vec3  F  = fresnelSchlick(max(dot(H,V),0.), F0);\n"
        "  float D  = ggxNDF(N,H,u_corneal_roughness*0.05);\n"
        "  float G  = schlickGGX(max(dot(N,V),0.),u_corneal_roughness);\n"
        "  vec3  spec = (D*G*F)/max(4.*max(dot(N,V),0.)*max(dot(N,L),0.),0.001);\n"
        "  spec *= u_corneal_wetness;\n"
        "  // Caustics de la cornea\n"
        "  float caus = pow(max(0., dot(reflect(-L,N), V)), 32.)*0.3*u_corneal_wetness;\n"
        "  col += spec + vec3(caus);\n"
        "  // Highlight puntual\n"
        "  col += vec3(caus * 0.8);\n"
        "  col = aces_tonemap(col);\n"
        "  col = pow(clamp(col,0.,1.), vec3(1./2.2));\n"
        "  fragColor = vec4(col, 1.0);\n"
        "}\n");
    FA(js, jsz, jp,
        "// §F2 RigFace Eye — Mesh & Uniforms Setup\n"
        "function rigEyeSetup(gl, prog, ctx) {\n"
        "  const loc = n => gl.getUniformLocation(prog,n);\n"
        "  gl.uniform1f(loc('u_pupil_dilation'), ctx.pupilDilation);\n"
        "  gl.uniform1f(loc('u_pupil_roundness'), ctx.pupilRoundness);\n"
        "  gl.uniform3fv(loc('u_iris_inner'), ctx.irisInner);\n"
        "  gl.uniform3fv(loc('u_iris_outer'), ctx.irisOuter);\n"
        "  gl.uniform3fv(loc('u_limbal_ring'), ctx.limbalRing||[0.05,0.04,0.04]);\n"
        "  gl.uniform1f(loc('u_iris_fiber_density'), ctx.fiberDensity);\n"
        "  gl.uniform1f(loc('u_iris_fiber_contrast'), ctx.fiberContrast);\n"
        "  gl.uniform1f(loc('u_corneal_bulge'), ctx.cornealBulge);\n"
        "  gl.uniform1f(loc('u_corneal_ior'), ctx.cornealIOR||1.376);\n"
        "  gl.uniform1f(loc('u_corneal_roughness'), ctx.cornealRoughness||0.02);\n"
        "  gl.uniform1f(loc('u_corneal_wetness'), ctx.cornealWetness||0.9);\n"
        "  gl.uniform1f(loc('u_sclera_redness'), ctx.scleraRedness);\n"
        "  gl.uniform1f(loc('u_sclera_yellowness'), ctx.scleraYellowness||0.);\n"
        "  gl.uniform1f(loc('u_sclera_veins'), ctx.scleraVeins||0.3);\n"
        "}\n"
        "// Eyeball sphere geometry (uv-sphere r=12mm)\n"
        "function rigEyeballGeo(r=0.012, lat=32, lon=32) {\n"
        "  const v=[],n=[],uv=[],idx=[];\n"
        "  for(let i=0;i<=lat;i++){\n"
        "    const t=i/lat*Math.PI;\n"
        "    for(let j=0;j<=lon;j++){\n"
        "      const p=j/lon*Math.PI*2;\n"
        "      const x=Math.sin(t)*Math.cos(p);\n"
        "      const y=Math.cos(t);\n"
        "      const z=Math.sin(t)*Math.sin(p);\n"
        "      v.push(r*x,r*y,r*z);n.push(x,y,z);\n"
        "      uv.push(j/lon,i/lat);\n"
        "    }\n"
        "  }\n"
        "  for(let i=0;i<lat;i++)for(let j=0;j<lon;j++){\n"
        "    const a=i*(lon+1)+j;\n"
        "    idx.push(a,a+1,a+lon+1,a+1,a+lon+2,a+lon+1);\n"
        "  }\n"
        "  return{verts:new Float32Array(v),normals:new Float32Array(n),\n"
        "         uvs:new Float32Array(uv),indices:new Uint16Array(idx)};\n"
        "}\n");
    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    out->phi_ratio = RIG_PHI;
    return fp + jp;
}
int rig_face_v2_lash_geo(const RigFaceEyeCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *js = (char*)malloc(FACE_JS_BUF);
    if (!js) return -1;
    int jp = 0, jsz = FACE_JS_BUF;
    FA(js, jsz, jp,
        "// §F2b Eyelash Geometry\n"
        "function rigLashGeo(upper_count=%u, lower_count=%u, length=%.4f,\n"
        "                     curl=%.4f, thickness=%.4f) {\n"
        "  const verts=[], idx=[], normals=[];\n"
        "  function addLash(ox, oy, oz, dirX, dirY, dirZ, len, c, w) {\n"
        "    const SEGS=8;\n"
        "    let px=ox,py=oy,pz=oz;\n"
        "    const base=verts.length/3;\n"
        "    for(let s=0;s<=SEGS;s++) {\n"
        "      const t=s/SEGS;\n"
        "      const curl_x=Math.sin(t*Math.PI*c)*0.003;\n"
        "      const taper=w*(1.-t*0.9);\n"
        "      verts.push(px+taper,py,pz, px-taper,py,pz);\n"
        "      normals.push(0,1,0, 0,1,0);\n"
        "      px+=dirX*len/SEGS+curl_x;\n"
        "      py+=dirY*len/SEGS;\n"
        "      pz+=dirZ*len/SEGS;\n"
        "    }\n"
        "    for(let s=0;s<SEGS;s++){\n"
        "      const b=base+s*2;\n"
        "      idx.push(b,b+1,b+2,b+1,b+3,b+2);\n"
        "    }\n"
        "  }\n"
        "  // Upper lashes arc\n"
        "  for(let i=0;i<%u;i++) {\n"
        "    const a=(i/%u-0.5)*Math.PI*0.9;\n"
        "    const r=0.012;\n"
        "    addLash(Math.cos(a)*r, 0.008, Math.sin(a)*r,\n"
        "            0, 0.8, 0, length, curl, thickness);\n"
        "  }\n"
        "  // Lower lashes arc\n"
        "  for(let i=0;i<%u;i++) {\n"
        "    const a=(i/%u-0.5)*Math.PI*0.7;\n"
        "    const r=0.012;\n"
        "    addLash(Math.cos(a)*r, -0.008, Math.sin(a)*r,\n"
        "            0,-0.6,0, length*0.55, curl*0.5, thickness*0.7);\n"
        "  }\n"
        "  return{verts:new Float32Array(verts),normals:new Float32Array(normals),\n"
        "         indices:new Uint16Array(idx)};\n"
        "}\n",
        ctx->lash_count_upper, ctx->lash_count_lower,
        ctx->lash_length, ctx->lash_curl, ctx->lash_thickness,
        ctx->lash_count_upper, ctx->lash_count_upper,
        ctx->lash_count_lower, ctx->lash_count_lower);
    out->js = js;
    out->ok = true;
    return jp;
}
int rig_face_v2_mouth_geo(const RigFaceMouthCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)malloc(FACE_SHADER_BUF);
    char *js   = (char*)malloc(FACE_JS_BUF);
    if (!frag || !js) { free(frag); free(js); return -1; }
    int fp = 0, jp = 0, sz = FACE_SHADER_BUF, jsz = FACE_JS_BUF;
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp, "%s", GLSL_PBR_FUNCS);
    FA(frag, sz, fp,
        "in vec2  v_uv;\n"
        "in vec3  v_normal;\n"
        "in vec3  v_pos;\n"
        "in float v_region; // 0=labio 1=encía 2=diente 3=lengua\n"
        "out vec4 fragColor;\n"
        "uniform float u_lip_wetness;        // %.4f\n"
        "uniform float u_lip_fullness_upper; // %.4f\n"
        "uniform float u_lip_fullness_lower; // %.4f\n"
        "uniform float u_gum_exposure;       // %.4f\n"
        "uniform float u_jaw_open;           // %.4f\n"
        "uniform vec3  u_tooth_color;        // (%.3f,%.3f,%.3f)\n"
        "uniform float u_tooth_roughness;    // %.4f\n"
        "uniform float u_tongue_visible;     // %.4f\n"
        "uniform float u_saliva_threads;     // %.4f\n"
        "uniform vec3  u_light_dir;\n"
        "uniform vec3  u_view_dir;\n"
        "uniform float u_time;\n\n",
        ctx->lip_wetness,
        ctx->lip_fullness_upper, ctx->lip_fullness_lower,
        ctx->gum_exposure, ctx->jaw_open_amount,
        ctx->tooth_color.x, ctx->tooth_color.y, ctx->tooth_color.z,
        ctx->tooth_enamel_roughness,
        ctx->tongue_visible,
        ctx->enable_saliva_threads ? 1.0f : 0.0f);
    FA(frag, sz, fp,
        "// ── Lip shader ────────────────────────────────────────────\n"
        "vec3 shade_lip(vec2 uv, vec3 N, vec3 L, vec3 V) {\n"
        "  // Anisotropía labial (líneas horizontales)\n"
        "  float lines = abs(sin(uv.y * 80.0)) * 0.05;\n"
        "  // SSS labial (rojo-rosa)\n"
        "  float sss_mask = abs(uv.x - 0.5) * 2.;\n"
        "  vec3 lip_base = vec3(0.72, 0.32, 0.28) - sss_mask*vec3(0.1,0.05,0.05);\n"
        "  // Wetness especular\n"
        "  vec3 H = normalize(L+V);\n"
        "  float spec = pow(max(dot(N,H),0.), 64.) * u_lip_wetness * 0.8;\n"
        "  vec3 col = lip_base * (0.9+lines) + vec3(spec);\n"
        "  return col;\n"
        "}\n"
        "// ── Tooth enamel PBR ──────────────────────────────────────\n"
        "vec3 shade_tooth(vec3 N, vec3 L, vec3 V) {\n"
        "  vec3 H = normalize(L+V);\n"
        "  float ndl = max(dot(N,L),0.);\n"
        "  float ndv = max(dot(N,V),0.);\n"
        "  vec3  F0  = vec3(0.04);\n"
        "  vec3  F   = fresnelSchlick(max(dot(H,V),0.),F0);\n"
        "  float D   = ggxNDF(N,H,u_tooth_roughness);\n"
        "  float G   = schlickGGX(ndv,u_tooth_roughness)*schlickGGX(ndl,u_tooth_roughness);\n"
        "  vec3  spec= (D*G*F)/max(4.*ndv*ndl,0.001);\n"
        "  return u_tooth_color*ndl*0.9 + spec*1.5;\n"
        "}\n"
        "// ── Tongue ────────────────────────────────────────────────\n"
        "vec3 shade_tongue(vec2 uv, vec3 N, vec3 L) {\n"
        "  float bumps = vnoise(vec3(uv*20.,0.))*0.06;\n"
        "  vec3 col = vec3(0.80,0.28,0.28) + bumps;\n"
        "  return col * max(dot(N,L),0.15);\n"
        "}\n"
        "// ── Saliva thread effect ──────────────────────────────────\n"
        "float saliva_thread(vec2 uv) {\n"
        "  float t = u_time;\n"
        "  float thread = abs(sin(uv.x*PI*3.+t*2.)) * (1.-uv.y);\n"
        "  return thread * u_saliva_threads * u_jaw_open;\n"
        "}\n"
        "void main() {\n"
        "  vec3 N = normalize(v_normal);\n"
        "  vec3 L = normalize(u_light_dir);\n"
        "  vec3 V = normalize(u_view_dir);\n"
        "  vec3 col;\n"
        "  int reg = int(v_region + 0.5);\n"
        "  if(reg == 0) col = shade_lip(v_uv, N, L, V);\n"
        "  else if(reg == 1) {\n"
        "    col = vec3(0.72,0.30,0.35)*max(dot(N,L),0.2);\n"
        "  }\n"
        "  else if(reg == 2) col = shade_tooth(N,L,V);\n"
        "  else col = shade_tongue(v_uv,N,L);\n"
        "  // Saliva threads (jaw open)\n"
        "  float sth = saliva_thread(v_uv);\n"
        "  col += vec3(0.85,0.82,0.80)*sth*0.4;\n"
        "  col = aces_tonemap(col);\n"
        "  col = pow(clamp(col,0.,1.),vec3(1./2.2));\n"
        "  fragColor = vec4(col, 1.0);\n"
        "}\n");
    FA(js, jsz, jp,
        "// §F3 Mouth geometry: teeth arc + gum + jaw\n"
        "function rigMouthGeo(ctx) {\n"
        "  const {teethUpper=%u,teethLower=%u,teethSize=%.4f,jawOpen=%.4f} = ctx;\n"
        "  const verts=[], normals=[], regions=[], indices=[];\n"
        "  // Upper teeth arc\n"
        "  for(let i=0;i<teethUpper;i++) {\n"
        "    const a=(i/(teethUpper-1)-0.5)*Math.PI*0.55;\n"
        "    const r=0.016;\n"
        "    const x=Math.sin(a)*r, z=Math.cos(a)*r-0.02;\n"
        "    // Cada diente: cubo biselado simplificado\n"
        "    const w=teethSize*0.0045, h=0.008, d=0.003;\n"
        "    verts.push(x-w, 0.002,z-d, x+w,0.002,z-d,\n"
        "               x+w,-h,  z-d, x-w,-h,  z-d,\n"
        "               x-w, 0.002,z+d, x+w,0.002,z+d);\n"
        "    for(let j=0;j<6;j++) regions.push(2);\n"
        "    const b=i*6;\n"
        "    indices.push(b,b+1,b+2, b,b+2,b+3, b,b+4,b+1);\n"
        "  }\n"
        "  return{verts:new Float32Array(verts),regions:new Float32Array(regions),\n"
        "         indices:new Uint16Array(indices)};\n"
        "}\n",
        ctx->teeth_count_upper, ctx->teeth_count_lower,
        ctx->teeth_size, ctx->jaw_open_amount);
    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    return fp + jp;
}
int rig_face_v2_teeth_geo(const RigFaceMouthCtx *ctx, RigArtResultV4 *out)
{
    return rig_face_v2_mouth_geo(ctx, out);
}
int rig_face_v2_tongue_shader(const RigFaceMouthCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)malloc(FACE_SHADER_BUF / 2);
    if (!frag) return -1;
    int fp = 0, sz = FACE_SHADER_BUF / 2;
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp,
        "in vec2 v_uv; in vec3 v_normal; out vec4 fragColor;\n"
        "uniform float u_tongue_visible; uniform vec3 u_light_dir;\n"
        "uniform float u_tongue_curl;\n"
        "void main() {\n"
        "  float bump = vnoise(vec3(v_uv*20.,0.))*0.08;\n"
        "  // Papilae: filiformes con punteado\n"
        "  float papil = 1.-smoothstep(0.,0.04,length(fract(v_uv*30.+0.5)-0.5));\n"
        "  vec3 col = vec3(0.78,0.25,0.26) + bump + papil*0.04;\n"
        "  float ndl = max(dot(normalize(v_normal),normalize(u_light_dir)),0.1);\n"
        "  col *= ndl;\n"
        "  fragColor = vec4(col, u_tongue_visible);\n"
        "}\n");
    out->glsl_frag = frag;
    out->ok = true;
    return fp;
}
int rig_face_v2_anim_fsm(const RigFaceAnimCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *js = (char*)malloc(FACE_JS_BUF * 2);
    if (!js) return -1;
    int jp = 0, jsz = FACE_JS_BUF * 2;
    FA(js, jsz, jp,
        "// §F4 RigCom v24 THE SANTORIUM OF COMPILER — Avatar Animator FSM\n"
        "// blink_rate=%.3f · emotion_blend=%.3f · micro_twitches=%s\n"
        "class RigFaceAnimatorFSM {\n"
        "  constructor(ctx={}) {\n"
        "    this.state      = '%s';\n"
        "    this.time       = 0;\n"
        "    this.blinkRate  = %.4f;\n"
        "    this.blinkDur   = %.4f;\n"
        "    this.blinkTimer = Math.random()/this.blinkRate;\n"
        "    this.blinkPhase = 0; // 0=open 1=closing 2=closed 3=opening\n"
        "    this.blinkVal   = 0; // 0=abierto 1=cerrado\n"
        "    this.saccadeProb = %.4f;\n"
        "    this.saccadeAmp  = %.4f;\n"
        "    this.eyeRotX = 0; this.eyeRotY = 0;\n"
        "    this.eyeTargX= 0; this.eyeTargY= 0;\n"
        "    this.breathPhase = 0;\n"
        "    this.breathRate  = %.4f;\n"
        "    this.microTimer  = 0;\n"
        "    this.emotionW    = new Float32Array(8); // 8 emociones base\n"
        "    this.facs        = new Float32Array(%d); // 52 FACS\n"
        "    this.lookAt      = null;\n"
        "    this.lookSpeed   = %.4f;\n"
        "    this.eyeLead     = %.4f;\n"
        "    this.enableMicro = %s;\n"
        "    this.twitch_amp  = %.4f;\n"
        "    this.twitch_freq = %.4f;\n"
        "  }\n\n",
        ctx->blink_rate, ctx->emotion_blend_speed,
        ctx->enable_micro_twitches ? "true" : "false",
        "IDLE",
        ctx->blink_rate, ctx->blink_duration,
        ctx->saccade_probability, ctx->saccade_amplitude,
        ctx->breath_rate / 60.0f,
        (int)FACS_COUNT,
        ctx->head_follow_speed, ctx->eye_lead_ratio,
        ctx->enable_micro_twitches ? "true" : "false",
        ctx->twitch_amplitude, ctx->twitch_frequency);
    FA(js, jsz, jp,
        "  update(dt) {\n"
        "    this.time += dt;\n"
        "    this._updateBlink(dt);\n"
        "    this._updateSaccade(dt);\n"
        "    if(this.enableMicro) this._updateMicro(dt);\n"
        "    if(this.breathRate>0) this._updateBreath(dt);\n"
        "    if(this.lookAt) this._updateLookAt(dt);\n"
        "    this._blendEmotions(dt);\n"
        "  }\n\n"
        "  _updateBlink(dt) {\n"
        "    this.blinkTimer -= dt;\n"
        "    if(this.blinkTimer <= 0 && this.blinkPhase === 0) {\n"
        "      this.blinkPhase = 1;\n"
        "      this.blinkTimer = this.blinkDur * 0.5;\n"
        "    }\n"
        "    if(this.blinkPhase === 1) {\n"
        "      this.blinkVal = Math.min(1., this.blinkVal + dt/(this.blinkDur*0.5));\n"
        "      if(this.blinkVal >= 1.) { this.blinkPhase = 2; this.blinkTimer=this.blinkDur*0.1; }\n"
        "    } else if(this.blinkPhase === 2 && this.blinkTimer <= 0) {\n"
        "      this.blinkPhase = 3;\n"
        "    } else if(this.blinkPhase === 3) {\n"
        "      this.blinkVal = Math.max(0., this.blinkVal - dt/(this.blinkDur*0.5));\n"
        "      if(this.blinkVal <= 0.) {\n"
        "        this.blinkPhase = 0;\n"
        "        this.blinkTimer = (0.5 + Math.random()*2.0) / this.blinkRate;\n"
        "      }\n"
        "    }\n"
        "    // FACS AU45 = blink\n"
        "    this.facs[%d] = this.blinkVal;\n"
        "  }\n\n",
        (int)FACS_AU45_BLINK);
    FA(js, jsz, jp,
        "  _updateSaccade(dt) {\n"
        "    if(Math.random() < this.saccadeProb * dt) {\n"
        "      this.eyeTargX = (Math.random()-0.5)*this.saccadeAmp*2.;\n"
        "      this.eyeTargY = (Math.random()-0.5)*this.saccadeAmp;\n"
        "    }\n"
        "    const speed = %.2f;\n"
        "    this.eyeRotX += (this.eyeTargX - this.eyeRotX)*speed*dt;\n"
        "    this.eyeRotY += (this.eyeTargY - this.eyeRotY)*speed*dt;\n"
        "  }\n\n"
        "  _updateMicro(dt) {\n"
        "    this.microTimer += dt;\n"
        "    // Micro-contracciones aleatorias en AU4,6,7,12\n"
        "    const auList = [%d,%d,%d,%d];\n"
        "    auList.forEach(au => {\n"
        "      const n = Math.sin(this.microTimer*this.twitch_freq*PHI_INV+au*1.61);\n"
        "      this.facs[au] = Math.max(0., n*this.twitch_amp);\n"
        "    });\n"
        "  }\n\n"
        "  _updateBreath(dt) {\n"
        "    this.breathPhase += dt*this.breathRate*Math.PI*2.;\n"
        "    const breath = Math.sin(this.breathPhase)*0.5+0.5;\n"
        "    // FACS AU25 (lips part) levemente durante inspiración\n"
        "    this.facs[%d] = Math.max(this.facs[%d], breath*0.08*%.3f);\n"
        "  }\n\n"
        "  _updateLookAt(dt) {\n"
        "    if(!this.lookAt) return 0;\n"
        "    const dx = this.lookAt[0] - this.eyeRotX;\n"
        "    const dy = this.lookAt[1] - this.eyeRotY;\n"
        "    this.eyeRotX += dx * this.lookSpeed * dt * this.eyeLead;\n"
        "    this.eyeRotY += dy * this.lookSpeed * dt * this.eyeLead;\n"
        "  }\n\n"
        "  _blendEmotions(dt) {\n"
        "    const speed = %.4f;\n"
        "    // Happy: AU6+AU12; Sad: AU1+AU4+AU15; Angry: AU4+AU5+AU7\n"
        "    // Surprised: AU1+AU2+AU5+AU26; Fearful: AU1+AU2+AU4+AU5+AU7+AU20+AU26\n"
        "    const MAPS = [\n"
        "      [%d,%d],        // happy: AU6,AU12\n"
        "      [%d,%d,%d],     // sad: AU1,AU4,AU15\n"
        "      [%d,%d,%d],     // angry: AU4,AU5,AU7\n"
        "      [%d,%d,%d,%d],  // surprised: AU1,AU2,AU5,AU26\n"
        "      [%d],           // disgusted: AU9\n"
        "      [%d,%d,%d],     // fearful: AU1,AU2,AU20\n"
        "      [%d,%d],        // contempt: AU12(unilateral),AU14\n"
        "      [],             // neutral\n"
        "    ];\n"
        "    MAPS.forEach((aus, ei) => {\n"
        "      const w = this.emotionW[ei];\n"
        "      aus.forEach(au => { this.facs[au] = Math.max(this.facs[au], w); });\n"
        "    });\n"
        "  }\n\n"
        "  setEmotion(idx, weight, blend_speed) {\n"
        "    // blend toward target weight\n"
        "    this.emotionW[idx] = weight;\n"
        "    if(blend_speed) this.emotionBlendSpeed = blend_speed;\n"
        "  }\n\n"
        "  getFACS() { return this.facs; }\n"
        "  getEyeRotation() { return [this.eyeRotX, this.eyeRotY]; }\n"
        "  getBlinkValue()  { return this.blinkVal; }\n"
        "}\n"
        "const PHI_INV = 0.6180339887;\n",
        ctx->saccade_speed,
        (int)FACS_AU4_BROW_LOWER, (int)FACS_AU6_CHEEK_RAISE,
        (int)FACS_AU7_LID_TIGHTEN, (int)FACS_AU12_LIP_CORNER_PULL,
        (int)FACS_AU25_LIPS_PART, (int)FACS_AU25_LIPS_PART,
        ctx->breath_depth,
        ctx->emotion_blend_speed,
        (int)FACS_AU6_CHEEK_RAISE, (int)FACS_AU12_LIP_CORNER_PULL,
        (int)FACS_AU1_INNER_BROW_RAISE, (int)FACS_AU4_BROW_LOWER, (int)FACS_AU15_LIP_CORNER_DEPRESS,
        (int)FACS_AU4_BROW_LOWER, (int)FACS_AU5_UPPER_LID_RAISE, (int)FACS_AU7_LID_TIGHTEN,
        (int)FACS_AU1_INNER_BROW_RAISE, (int)FACS_AU2_OUTER_BROW_RAISE,
        (int)FACS_AU5_UPPER_LID_RAISE, (int)FACS_AU26_JAW_DROP,
        (int)FACS_AU9_NOSE_WRINKLE,
        (int)FACS_AU1_INNER_BROW_RAISE, (int)FACS_AU2_OUTER_BROW_RAISE, (int)FACS_AU20_LIP_STRETCH,
        (int)FACS_AU12_LIP_CORNER_PULL, (int)FACS_AU14_DIMPLER);
    out->js = js;
    out->ok = true;
    return jp;
}
int rig_face_v2_skin_shader(const RigFaceSkinV2Ctx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)malloc(FACE_SHADER_BUF);
    char *js   = (char*)malloc(FACE_JS_BUF);
    if (!frag || !js) { free(frag); free(js); return -1; }
    int fp = 0, jp = 0, sz = FACE_SHADER_BUF, jsz = FACE_JS_BUF;
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp, "%s", GLSL_PBR_FUNCS);
    FA(frag, sz, fp,
        "in vec3 v_pos;\nin vec3 v_normal;\nin vec3 v_tangent;\n"
        "in vec3 v_bitangent;\nin vec2 v_uv;\nin vec4 v_color;\n"
        "out vec4 fragColor;\n\n"
        "// ── Bioquímica de piel (Donner-Jensen 2005) ───────────────\n"
        "uniform float u_melanin;       // %.4f\n"
        "uniform float u_hemoglobin;    // %.4f\n"
        "uniform float u_carotene;      // %.4f\n"
        "uniform float u_bilirubin;     // %.4f\n"
        "// ── SSS Multicapa ──────────────────────────────────────────\n"
        "uniform vec3  u_sss_oil;       // (%.4f,%.4f,%.4f)\n"
        "uniform vec3  u_sss_epid;      // (%.4f,%.4f,%.4f)\n"
        "uniform vec3  u_sss_derm;      // (%.4f,%.4f,%.4f)\n"
        "uniform vec3  u_sss_subcut;    // (%.4f,%.4f,%.4f)\n"
        "uniform float u_transmittance; // %.4f\n"
        "uniform float u_sss_weight;    // 0.4\n"
        "// ── PBR ────────────────────────────────────────────────────\n"
        "uniform float u_lipid_roughness;// %.4f\n"
        "uniform float u_specular_ior;  // %.4f\n"
        "// ── Poros ──────────────────────────────────────────────────\n"
        "uniform float u_pore_density;  // %.4f\n"
        "uniform float u_pore_depth;    // %.4f\n"
        "uniform float u_pore_scale;    // %.4f\n"
        "// ── Vasculatura ────────────────────────────────────────────\n"
        "uniform float u_vein_depth;    // %.4f\n"
        "uniform float u_vein_vis;      // %.4f\n"
        "uniform vec3  u_vein_color;    // (%.3f,%.3f,%.3f)\n"
        "// ── Arrugas ────────────────────────────────────────────────\n"
        "uniform float u_wrinkle_depth; // %.4f\n"
        "uniform float u_wrinkle_density;// %.4f\n"
        "// ── Otras ──────────────────────────────────────────────────\n"
        "uniform float u_oiliness;      // %.4f\n"
        "uniform bool  u_freckles;      // %s\n"
        "uniform float u_freckle_density;// %.4f\n"
        "uniform vec3  u_freckle_color; // (%.3f,%.3f,%.3f)\n"
        "// ── Samplers ───────────────────────────────────────────────\n"
        "uniform sampler2D u_albedo_map;\n"
        "uniform sampler2D u_normal_map;\n"
        "uniform sampler2D u_roughness_map;\n"
        "uniform sampler2D u_sss_map;\n"
        "uniform sampler2D u_pore_normal;\n"
        "uniform sampler2D u_wrinkle_normal;\n"
        "uniform sampler2D u_vascular_map;\n"
        "// ── Luz ────────────────────────────────────────────────────\n"
        "uniform vec3 u_light_dir;\n"
        "uniform vec3 u_light_color;\n"
        "uniform vec3 u_view_dir;\n"
        "uniform vec3 u_env_irradiance;\n\n",
        ctx->melanin, ctx->hemoglobin, ctx->carotene, ctx->bilirubin,
        ctx->scatter_radius_oil.x,     ctx->scatter_radius_oil.y,     ctx->scatter_radius_oil.z,
        ctx->scatter_radius_epidermis.x,ctx->scatter_radius_epidermis.y,ctx->scatter_radius_epidermis.z,
        ctx->scatter_radius_dermis.x,  ctx->scatter_radius_dermis.y,  ctx->scatter_radius_dermis.z,
        ctx->scatter_radius_subcut.x,  ctx->scatter_radius_subcut.y,  ctx->scatter_radius_subcut.z,
        ctx->transmittance,
        ctx->lipid_roughness, ctx->specular_ior,
        ctx->pore_density, ctx->pore_depth, ctx->pore_scale,
        ctx->vein_depth, ctx->vein_visibility,
        ctx->vein_color.x, ctx->vein_color.y, ctx->vein_color.z,
        ctx->wrinkle_depth, ctx->wrinkle_density,
        ctx->skin_oiliness,
        ctx->enable_freckles ? "true" : "false",
        ctx->freckle_density,
        ctx->freckle_color.x, ctx->freckle_color.y, ctx->freckle_color.z);
    FA(frag, sz, fp,
        "// ── SSS Aproximación Multicapa (Jimenez + Chiang 2016) ────\n"
        "vec3 sss_multilayer(vec3 albedo, float thick) {\n"
        "  // Capa 1: aceite superficial (ε = 0.01mm)\n"
        "  vec3 oil   = albedo * exp(-thick * (1./max(u_sss_oil,vec3(0.001))));\n"
        "  // Capa 2: epidermis (ε ≈ 0.08mm)\n"
        "  vec3 epid  = albedo * exp(-thick * (1./max(u_sss_epid,vec3(0.001))));\n"
        "  // Capa 3: dermis (ε ≈ 0.24mm)\n"
        "  vec3 derm  = albedo * exp(-thick * (1./max(u_sss_derm,vec3(0.001))));\n"
        "  // Capa 4: subcutis graso\n"
        "  vec3 subc  = albedo * exp(-thick * (1./max(u_sss_subcut,vec3(0.001))));\n"
        "  vec3 scatter = oil*0.10 + epid*0.30 + derm*0.45 + subc*0.15;\n"
        "  float back = exp(-thick*PHI_INV);\n"
        "  scatter += albedo*back*vec3(1.0,0.72,0.46)*0.3*u_transmittance;\n"
        "  return scatter;\n"
        "}\n\n"
        "// ── Albedo bioquímico (Donner-Jensen) ─────────────────────\n"
        "vec3 skin_bio_albedo(vec3 base) {\n"
        "  // Eumelanina: absorbe todo el espectro, más en azul\n"
        "  base *= vec3(1.-u_melanin*0.55, 1.-u_melanin*0.38, 1.-u_melanin*0.72);\n"
        "  // Hemoglobina oxigenada: tono rosado-rojo\n"
        "  base += vec3(u_hemoglobin*0.22, u_hemoglobin*0.04, u_hemoglobin*0.02);\n"
        "  // Caroteno: tono amarillo-naranja\n"
        "  base += vec3(u_carotene*0.18, u_carotene*0.12, 0.);\n"
        "  // Bilirrubina: tono amarillo (ictericia)\n"
        "  base += vec3(u_bilirubin*0.15, u_bilirubin*0.12, -u_bilirubin*0.02);\n"
        "  return clamp(base, 0., 1.);\n"
        "}\n\n"
        "// ── Poros Voronoi ─────────────────────────────────────────\n"
        "float pore_pattern(vec2 uv) {\n"
        "  vec2 v = voronoi(uv, u_pore_scale);\n"
        "  float pore = 1. - smoothstep(0., 0.1, v.x * (1./max(u_pore_density,0.01)));\n"
        "  return pore * u_pore_depth;\n"
        "}\n\n"
        "// ── Pecas ─────────────────────────────────────────────────\n"
        "float freckle_mask(vec2 uv) {\n"
        "  if(!u_freckles) return 0.;\n"
        "  float f = smoothstep(0.55, 0.45,\n"
        "    vnoise(vec3(uv*u_freckle_density*30.,2.7)));\n"
        "  return f;\n"
        "}\n\n"
        "void main() {\n"
        "  // 1. Normal compuesta: mapa + poros + arrugas\n"
        "  vec3 nm  = texture(u_normal_map,    v_uv).rgb*2.-1.;\n"
        "  vec3 pnm = texture(u_pore_normal,   v_uv*u_pore_scale*0.1).rgb*2.-1.;\n"
        "  vec3 wnm = texture(u_wrinkle_normal, v_uv).rgb*2.-1.;\n"
        "  pnm *= pore_pattern(v_uv) * 4.;\n"
        "  wnm *= u_wrinkle_depth;\n"
        "  vec3 Nts = normalize(nm + pnm + wnm);\n"
        "  mat3 TBN = mat3(normalize(v_tangent),\n"
        "                  normalize(v_bitangent),\n"
        "                  normalize(v_normal));\n"
        "  vec3 N = normalize(TBN * Nts);\n\n"
        "  // 2. Albedo base + corrección bioquímica\n"
        "  vec4 alb_smp = texture(u_albedo_map, v_uv);\n"
        "  vec3 albedo  = skin_bio_albedo(alb_smp.rgb);\n"
        "  // Vertex color modula distribución regional de melanina/SSS\n"
        "  albedo *= v_color.rgb;\n"
        "  // Pecas\n"
        "  float fr = freckle_mask(v_uv);\n"
        "  albedo = mix(albedo, u_freckle_color*albedo, fr*0.6);\n\n"
        "  // 3. Roughness con poros y oleosidad\n"
        "  float rgh = u_lipid_roughness * texture(u_roughness_map, v_uv).r;\n"
        "  rgh = mix(rgh, rgh*PHI_INV, u_oiliness);\n"
        "  rgh = clamp(rgh, 0.04, 1.);\n\n"
        "  // 4. SSS via mapa de espesor\n"
        "  float thick = texture(u_sss_map, v_uv).r;\n"
        "  vec3  vascular = texture(u_vascular_map, v_uv).rgb;\n"
        "  vec3  sss = sss_multilayer(albedo, thick);\n"
        "  // Vasculatura: hemoglobina visible según profundidad\n"
        "  sss += vascular * u_hemoglobin * u_vein_color * u_vein_vis * 0.12;\n\n"
        "  // 5. BRDF PBR Cook-Torrance\n"
        "  vec3 L = normalize(u_light_dir);\n"
        "  vec3 V = normalize(u_view_dir);\n"
        "  vec3 H = normalize(V+L);\n"
        "  float ndl = max(dot(N,L), 0.);\n"
        "  float ndv = max(dot(N,V), 0.);\n"
        "  float F0_f = pow((1.-u_specular_ior)/(1.+u_specular_ior),2.);\n"
        "  vec3  F0  = vec3(F0_f);\n"
        "  vec3  F   = fresnelSchlick(max(dot(H,V),0.), F0);\n"
        "  float D   = ggxNDF(N, H, rgh);\n"
        "  float G   = schlickGGX(ndv,rgh)*schlickGGX(ndl,rgh);\n"
        "  vec3  spec= (D*G*F)/max(4.*ndv*ndl, 0.001);\n"
        "  vec3  kD  = (1.-F);\n"
        "  vec3  diff= kD*albedo/PI;\n\n"
        "  // 6. Composición: directa + SSS + IBL ambiente\n"
        "  vec3 direct  = (diff+spec)*ndl*u_light_color;\n"
        "  vec3 ambient = u_env_irradiance*albedo*0.03\n"
        "               + sss * u_sss_weight * 0.45;\n"
        "  vec3 color   = direct + ambient;\n\n"
        "  // 7. Tone mapping ACES + gamma 2.2\n"
        "  color = aces_tonemap(color);\n"
        "  color = pow(clamp(color,0.,1.), vec3(1./2.2));\n"
        "  fragColor = vec4(color, 1.0);\n"
        "}\n");
    FA(js, jsz, jp,
        "// §F5 Skin Shader — WebGL Uniform Setter\n"
        "function rigSkinSetUniforms(gl, prog, s) {\n"
        "  const L = n => gl.getUniformLocation(prog, n);\n"
        "  gl.uniform1f(L('u_melanin'),       s.melanin);\n"
        "  gl.uniform1f(L('u_hemoglobin'),    s.hemoglobin);\n"
        "  gl.uniform1f(L('u_carotene'),      s.carotene);\n"
        "  gl.uniform1f(L('u_bilirubin'),     s.bilirubin||0.);\n"
        "  gl.uniform3fv(L('u_sss_epid'),     s.sssEpidermis||[0.08,0.03,0.02]);\n"
        "  gl.uniform3fv(L('u_sss_derm'),     s.sssDermis||[0.24,0.05,0.02]);\n"
        "  gl.uniform3fv(L('u_sss_subcut'),   s.sssSubcut||[0.50,0.15,0.08]);\n"
        "  gl.uniform1f(L('u_transmittance'), s.transmittance||0.3);\n"
        "  gl.uniform1f(L('u_sss_weight'),    s.sssWeight||0.4);\n"
        "  gl.uniform1f(L('u_lipid_roughness'),s.roughness||0.45);\n"
        "  gl.uniform1f(L('u_specular_ior'),  s.ior||1.4);\n"
        "  gl.uniform1f(L('u_pore_density'),  s.poreDensity||0.6);\n"
        "  gl.uniform1f(L('u_pore_depth'),    s.poreDepth||0.3);\n"
        "  gl.uniform1f(L('u_pore_scale'),    s.poreScale||40.);\n"
        "  gl.uniform1f(L('u_vein_depth'),    s.veinDepth||0.4);\n"
        "  gl.uniform1f(L('u_vein_vis'),      s.veinVis||0.3);\n"
        "  gl.uniform3fv(L('u_vein_color'),   s.veinColor||[0.3,0.1,0.6]);\n"
        "  gl.uniform1f(L('u_wrinkle_depth'), s.wrinkleDepth||0.0);\n"
        "  gl.uniform1f(L('u_wrinkle_density'),s.wrinkleDensity||0.0);\n"
        "  gl.uniform1f(L('u_oiliness'),      s.oiliness||0.2);\n"
        "  gl.uniform1i(L('u_freckles'),      s.freckles?1:0);\n"
        "  gl.uniform1f(L('u_freckle_density'),s.freckleDensity||0.);\n"
        "  gl.uniform3fv(L('u_freckle_color'), s.freckleColor||[0.5,0.3,0.1]);\n"
        "}\n");
    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    out->phi_ratio = RIG_PHI;
    return fp + jp;
}
int rig_face_v2_eyebrow_gen(const RigFaceBrowCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)malloc(FACE_SHADER_BUF / 2);
    char *js   = (char*)malloc(FACE_JS_BUF);
    if (!frag || !js) { free(frag); free(js); return -1; }
    int fp = 0, jp = 0, sz = FACE_SHADER_BUF / 2, jsz = FACE_JS_BUF;
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp,
        "in vec2 v_uv; in vec3 v_normal; out vec4 fragColor;\n"
        "uniform vec3  u_brow_color;      // (%.3f,%.3f,%.3f)\n"
        "uniform float u_brow_density;    // %.4f\n"
        "uniform float u_brow_arch;       // %.4f\n"
        "uniform float u_brow_thick_in;   // %.4f\n"
        "uniform float u_brow_thick_out;  // %.4f\n"
        "uniform float u_color_variance;  // %.4f\n"
        "uniform float u_microblading;    // %.4f\n"
        "uniform float u_raise_amount;    // %.4f\n"
        "uniform vec3  u_light_dir;\n\n",
        ctx->color.x, ctx->color.y, ctx->color.z,
        ctx->density, ctx->arch_height,
        ctx->thickness_inner, ctx->thickness_outer,
        ctx->color_variance,
        ctx->enable_microblading ? 1.0f : 0.0f,
        ctx->raise_amount);
    FA(frag, sz, fp,
        "void main() {\n"
        "  float x = v_uv.x, y = v_uv.y;\n"
        "  // Perfil de cejas: arco φ-parametric\n"
        "  float arch = u_brow_arch * sin(x * PI);\n"
        "  float thick = mix(u_brow_thick_in, u_brow_thick_out, x);\n"
        "  float dist = abs(y - 0.5 - arch) / max(thick, 0.01);\n"
        "  float mask = 1. - smoothstep(0.6, 1.0, dist);\n"
        "  // Fibras individuales del cabello\n"
        "  float fiber = abs(sin(y*200.+x*5.)) * 0.12 * u_brow_density;\n"
        "  // Microblading: líneas con contraste\n"
        "  float blade = u_microblading * (abs(sin(y*100.)) > 0.85 ? 1. : 0.);\n"
        "  // Variación de color por fibra\n"
        "  float var = phi_noise(vec3(v_uv*50.,0.)) * u_color_variance;\n"
        "  vec3 col = u_brow_color * (1.0 - fiber) * (1. + blade * 0.3);\n"
        "  col += var * u_brow_color * 0.2;\n"
        "  float ndl = max(dot(normalize(v_normal),normalize(u_light_dir)),0.2);\n"
        "  col *= ndl;\n"
        "  fragColor = vec4(col, mask * (0.8 + fiber));\n"
        "}\n");
    FA(js, jsz, jp,
        "// §F6 Eyebrow setup\n"
        "function rigBrowSetUniforms(gl, prog, b) {\n"
        "  const L=n=>gl.getUniformLocation(prog,n);\n"
        "  gl.uniform3fv(L('u_brow_color'),    b.color);\n"
        "  gl.uniform1f(L('u_brow_density'),   b.density||0.7);\n"
        "  gl.uniform1f(L('u_brow_arch'),      b.archHeight||0.12);\n"
        "  gl.uniform1f(L('u_brow_thick_in'),  b.thicknessInner||0.12);\n"
        "  gl.uniform1f(L('u_brow_thick_out'), b.thicknessOuter||0.08);\n"
        "  gl.uniform1f(L('u_color_variance'), b.colorVariance||0.15);\n"
        "  gl.uniform1f(L('u_microblading'),   b.microblading?1.:0.);\n"
        "  gl.uniform1f(L('u_raise_amount'),   b.raiseAmount||0.);\n"
        "}\n");
    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    return fp + jp;
}
int rig_face_v2_beard_gen(const RigFaceBeardCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)malloc(FACE_SHADER_BUF / 2);
    char *js   = (char*)malloc(FACE_JS_BUF);
    if (!frag || !js) { free(frag); free(js); return -1; }
    int fp = 0, jp = 0, sz = FACE_SHADER_BUF / 2, jsz = FACE_JS_BUF;
    static const char *beard_style_names[] = {
        "NONE","STUBBLE","SHORT","MEDIUM","FULL","GOATEE","MOUSTACHE","VAN_DYKE"
    };
    const char *sname = (ctx->style < 8) ? beard_style_names[ctx->style] : "FULL";
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp,
        "// Beard style: %s\n"
        "in vec2 v_uv; in vec3 v_normal; in float v_region;\n"
        "out vec4 fragColor;\n"
        "uniform vec3  u_beard_root;   // (%.3f,%.3f,%.3f)\n"
        "uniform vec3  u_beard_tip;    // (%.3f,%.3f,%.3f)\n"
        "uniform float u_beard_len;    // %.4f\n"
        "uniform float u_beard_dens;   // %.4f\n"
        "uniform float u_beard_rgh;    // %.4f\n"
        "uniform float u_melanin;      // %.4f\n"
        "uniform float u_stubble_ao;   // %s\n"
        "uniform float u_grow_pattern; // %.4f\n"
        "uniform vec3  u_light_dir;\n\n",
        sname,
        ctx->color_root.x, ctx->color_root.y, ctx->color_root.z,
        ctx->color_tip.x, ctx->color_tip.y, ctx->color_tip.z,
        ctx->length, ctx->density, ctx->roughness, ctx->melanin,
        ctx->enable_stubble_ao ? "1.0" : "0.0",
        ctx->growth_pattern);
    FA(frag, sz, fp,
        "void main() {\n"
        "  // Máscara de distribución de barba según estilo\n"
        "  float beard_mask = clamp(\n"
        "    vnoise(vec3(v_uv*u_grow_pattern*8., 1.0)) * 1.5 - 0.2, 0., 1.);\n"
        "  // Fibras individuales\n"
        "  float fiber = abs(sin(v_uv.y * u_beard_dens * 120.))\n"
        "              * abs(sin(v_uv.x * 40.)) * 0.4;\n"
        "  // Color por gradiente raíz→punta\n"
        "  vec3 col = mix(u_beard_root, u_beard_tip, fiber);\n"
        "  // Melanina: oscurece\n"
        "  col *= 1. - u_melanin * 0.5;\n"
        "  // AO de stubble: sombra en los poros de la barba\n"
        "  float ao = u_stubble_ao * (1. - voronoi(v_uv, 60.).x * 0.4);\n"
        "  col *= 0.8 + 0.2 * ao;\n"
        "  float ndl = max(dot(normalize(v_normal),normalize(u_light_dir)),0.1);\n"
        "  col *= ndl;\n"
        "  fragColor = vec4(col, beard_mask * fiber);\n"
        "}\n");
    FA(js, jsz, jp,
        "// §F7 Beard setup · style=%s\n"
        "function rigBeardSetUniforms(gl, prog, b) {\n"
        "  const L=n=>gl.getUniformLocation(prog,n);\n"
        "  gl.uniform3fv(L('u_beard_root'), b.colorRoot);\n"
        "  gl.uniform3fv(L('u_beard_tip'),  b.colorTip);\n"
        "  gl.uniform1f(L('u_beard_len'),   b.length||0.003);\n"
        "  gl.uniform1f(L('u_beard_dens'),  b.density||0.7);\n"
        "  gl.uniform1f(L('u_beard_rgh'),   b.roughness||0.6);\n"
        "  gl.uniform1f(L('u_melanin'),     b.melanin||0.5);\n"
        "  gl.uniform1f(L('u_stubble_ao'),  b.stubbleAO?1.:0.);\n"
        "  gl.uniform1f(L('u_grow_pattern'),b.growthPattern||0.5);\n"
        "}\n", sname);
    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    return fp + jp;
}
int rig_face_v2_ear_nose_geo(const RigFaceEarNoseCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *js = (char*)malloc(FACE_JS_BUF);
    if (!js) return -1;
    int jp = 0, jsz = FACE_JS_BUF;
    FA(js, jsz, jp,
        "// §F8 Ear & Nose procedural geometry\n"
        "// nose: bridge=%.4f tip=%.4f nostril_flare=%.4f\n"
        "// ear:  size=%.4f protrusion=%.4f helix=%.4f\n"
        "function rigNoseGeo(ctx) {\n"
        "  const bw=ctx.noseBridgeWidth||0.012;\n"
        "  const ts=ctx.noseTipSize||0.008;\n"
        "  const nf=ctx.nostrilFlare||0.014;\n"
        "  const nl=ctx.noseLength||0.045;\n"
        "  const ca=ctx.columellaAngle||(Math.PI*0.55);\n"
        "  // 5 curvas NURBS simplificadas: perfil lateral, frontal, base, narina×2\n"
        "  const verts=[];\n"
        "  // Perfil dorsal\n"
        "  const STEPS=16;\n"
        "  for(let i=0;i<=STEPS;i++) {\n"
        "    const t=i/STEPS;\n"
        "    // Bezier perfil: base→puente→punta\n"
        "    const px = (1.-t)*(1.-t)*0 + 2.*(1.-t)*t*(-bw*0.3) + t*t*(-ts*0.5);\n"
        "    const py = (1.-t)*(1.-t)*0 + 2.*(1.-t)*t*(nl*0.5)  + t*t*nl;\n"
        "    const pz = (1.-t)*(1.-t)*0 + 2.*(1.-t)*t*(bw*0.2)  + t*t*(ts*0.4);\n"
        "    verts.push(px,py,pz, -px,py,pz);\n"
        "  }\n"
        "  // Narinas: torus parcial\n"
        "  for(let s=0;s<2;s++) {\n"
        "    const sign=s===0?1:-1;\n"
        "    for(let i=0;i<=8;i++) {\n"
        "      const a=i/8.*Math.PI;\n"
        "      verts.push(sign*(nf+Math.cos(a)*0.003), Math.sin(a)*0.003, 0.001);\n"
        "    }\n"
        "  }\n"
        "  return new Float32Array(verts);\n"
        "}\n\n"
        "function rigEarGeo(ctx) {\n"
        "  const es=ctx.earSize||0.062;\n"
        "  const ep=ctx.earProtrusion||0.020;\n"
        "  const hc=ctx.helixCurvature||0.6;\n"
        "  const ls=ctx.lobuleSize||0.4;\n"
        "  // Hélix: curva en espiral alrededor de la concha\n"
        "  const verts=[], normals=[], indices=[];\n"
        "  const HELIX_PTS=32;\n"
        "  for(let i=0;i<=HELIX_PTS;i++) {\n"
        "    const t=i/HELIX_PTS;\n"
        "    const a=t*Math.PI*1.5; // ~270° de arco\n"
        "    const r_helix=es*(0.48+0.08*t);\n"
        "    const x=Math.cos(a)*r_helix*(1.+ep*0.3);\n"
        "    const y=Math.sin(a)*r_helix*1.15 - es*0.1;\n"
        "    const z=Math.sin(t*Math.PI)*ep*hc;\n"
        "    verts.push(x,y,z);\n"
        "    normals.push(Math.cos(a),Math.sin(a),0);\n"
        "  }\n"
        "  // Lóbulo: elipsoide inferior\n"
        "  const lx=0, ly=-es*(0.45+ls*0.1), lz=ep*0.3;\n"
        "  const lr=es*0.15*ls;\n"
        "  for(let i=0;i<=8;i++) {\n"
        "    const a=i/8.*Math.PI*2;\n"
        "    verts.push(lx+Math.cos(a)*lr, ly+Math.sin(a)*lr*0.7, lz);\n"
        "  }\n"
        "  return{verts:new Float32Array(verts),normals:new Float32Array(normals)};\n"
        "}\n",
        ctx->nose_bridge_width, ctx->nose_tip_size, ctx->nostril_flare,
        ctx->ear_size, ctx->ear_protrusion, ctx->helix_curvature);
    out->js = js;
    out->ok = true;
    return jp;
}
int rig_face_v2_age_deformer(const RigFaceAgeCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)malloc(FACE_SHADER_BUF / 2);
    char *js   = (char*)malloc(FACE_JS_BUF);
    if (!frag || !js) { free(frag); free(js); return -1; }
    int fp = 0, jp = 0, sz = FACE_SHADER_BUF / 2, jsz = FACE_JS_BUF;
    static const char *ethno_names[] = {
        "NEUTRAL","EAST_ASIAN","SOUTH_ASIAN","AFRICAN","EUROPEAN",
        "MIDDLE_EASTERN","LATIN","PACIFIC_ISLANDER","INDIGENOUS_AMERICAN"
    };
    const char *ename = (ctx->ethnicity < 9) ? ethno_names[ctx->ethnicity] : "NEUTRAL";
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp,
        "// Age deformer + ethnicity blend · age=%.1f · ethnicity=%s\n"
        "in vec2 v_uv; in vec3 v_normal; out vec4 fragColor;\n"
        "uniform float u_age_norm;        // 0=neonato 1=100yr (%.4f)\n"
        "uniform float u_skin_sag;        // %.4f\n"
        "uniform float u_nasolabial;      // %.4f\n"
        "uniform float u_crow_feet;       // %.4f\n"
        "uniform float u_forehead_wrink;  // %.4f\n"
        "uniform float u_jowl;            // %.4f\n"
        "uniform float u_orbital_fat;     // %.4f\n"
        "uniform vec3  u_light_dir;\n\n",
        ctx->age, ename,
        fc_clamp(ctx->age / 100.0f, 0.0f, 1.0f),
        ctx->skin_sag, ctx->nasolabial_depth, ctx->crow_feet,
        ctx->forehead_wrinkles, ctx->jowl_amount, ctx->orbital_fat_pad);
    FA(frag, sz, fp,
        "void main() {\n"
        "  vec2 uv = v_uv;\n"
        "  // Líneas de expresión de la frente\n"
        "  float forehead_z = (uv.y > 0.75)\n"
        "    ? abs(sin(uv.y*PI*6.))*(uv.y-0.75)*4.*u_forehead_wrink*0.08\n"
        "    : 0.;\n"
        "  // Patas de gallo (canthus lateral)\n"
        "  float crow_z = (uv.x<0.2||uv.x>0.8)&&(uv.y>0.45&&uv.y<0.6)\n"
        "    ? abs(sin(atan(uv.y-0.5, uv.x-0.5)*8.))*u_crow_feet*0.05\n"
        "    : 0.;\n"
        "  // Surco nasolabial\n"
        "  float naso_z = (abs(uv.x-0.5)<0.12 && uv.y>0.3 && uv.y<0.5)\n"
        "    ? (1.-abs(uv.x-0.5)/0.12)*u_nasolabial*0.04\n"
        "    : 0.;\n"
        "  float total_wrinkle = forehead_z + crow_z + naso_z;\n"
        "  // Color tonal de edad: más amarillo/gris\n"
        "  vec3 age_tint = mix(vec3(1.0), vec3(0.95,0.92,0.88), u_age_norm);\n"
        "  float ndl = max(dot(normalize(v_normal),normalize(u_light_dir)),0.1);\n"
        "  vec3 col = age_tint * (0.75 + 0.25*ndl);\n"
        "  // AO en arrugas\n"
        "  col *= 1. - total_wrinkle * 3.;\n"
        "  fragColor = vec4(col, total_wrinkle + 0.01);\n"
        "}\n");
    FA(js, jsz, jp,
        "// §F9 Age deformer · age=%.1f · ethnicity=%s\n"
        "function rigAgeDeform(mesh, age_norm, ctx) {\n"
        "  const sag  = ctx.skinSag||0.;\n"
        "  const jowl = ctx.jowlAmount||0.;\n"
        "  const fat  = ctx.orbitalFatPad||0.;\n"
        "  const v = mesh.verts;\n"
        "  for(let i=0;i<v.length;i+=3) {\n"
        "    const y = v[i+1], z = v[i+2];\n"
        "    // Ptosis gravitacional: vértices de la mitad inferior bajan\n"
        "    if(y < 0.0) {\n"
        "      v[i+1] -= Math.abs(y)*sag*age_norm*0.015;\n"
        "      v[i+2] += z*jowl*age_norm*0.008;\n"
        "    }\n"
        "    // Bolsas oculares: región orbital se proyecta\n"
        "    if(y > 0.01 && y < 0.04 && Math.abs(v[i])>0.008) {\n"
        "      v[i+2] += fat * age_norm * 0.003;\n"
        "    }\n"
        "  }\n"
        "  return mesh;\n"
        "}\n",
        ctx->age, ename);
    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    return fp + jp;
}
int rig_face_v2_makeup_shader(const RigFaceMakeupCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)malloc(FACE_SHADER_BUF);
    char *js   = (char*)malloc(FACE_JS_BUF);
    if (!frag || !js) { free(frag); free(js); return -1; }
    int fp = 0, jp = 0, sz = FACE_SHADER_BUF, jsz = FACE_JS_BUF;
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp,
        "in vec2 v_uv; in vec3 v_normal; in float v_region;\n"
        "out vec4 fragColor;\n"
        "// ── Foundation ────────────────────────────────────────────\n"
        "uniform bool  u_foundation;    // %s\n"
        "uniform vec3  u_found_color;   // (%.3f,%.3f,%.3f)\n"
        "uniform float u_found_cov;     // %.4f\n"
        "uniform float u_found_finish;  // %.4f  0=matte 1=glow\n"
        "// ── Blush ─────────────────────────────────────────────────\n"
        "uniform bool  u_blush;         // %s\n"
        "uniform vec3  u_blush_color;   // (%.3f,%.3f,%.3f)\n"
        "uniform float u_blush_int;     // %.4f\n"
        "uniform vec2  u_blush_pos;     // (%.3f,%.3f)\n"
        "// ── Eye Shadow ────────────────────────────────────────────\n"
        "uniform bool  u_eyeshadow;     // %s\n"
        "uniform vec3  u_eshadow_a;     // (%.3f,%.3f,%.3f)\n"
        "uniform vec3  u_eshadow_b;     // (%.3f,%.3f,%.3f)\n"
        "uniform float u_eshadow_spr;   // %.4f\n"
        "uniform bool  u_eshadow_shimmer; // %s\n"
        "// ── Lipstick ──────────────────────────────────────────────\n"
        "uniform bool  u_lipstick;      // %s\n"
        "uniform vec3  u_lip_color;     // (%.3f,%.3f,%.3f)\n"
        "uniform float u_lip_gloss;     // %.4f\n"
        "uniform float u_lip_cov;       // %.4f\n"
        "// ── Contour & Highlight ───────────────────────────────────\n"
        "uniform bool  u_contour;       // %s\n"
        "uniform float u_contour_str;   // %.4f\n"
        "uniform float u_highlight_str; // %.4f\n"
        "uniform vec3  u_highlight_col; // (%.3f,%.3f,%.3f)\n"
        "uniform vec3  u_light_dir;\n"
        "uniform vec3  u_view_dir;\n\n",
        ctx->enable_foundation ? "true" : "false",
        ctx->foundation_color.x, ctx->foundation_color.y, ctx->foundation_color.z,
        ctx->foundation_coverage, ctx->foundation_finish,
        ctx->enable_blush ? "true" : "false",
        ctx->blush_color.x, ctx->blush_color.y, ctx->blush_color.z,
        ctx->blush_intensity,
        ctx->blush_position.x, ctx->blush_position.y,
        ctx->enable_eyeshadow ? "true" : "false",
        ctx->eyeshadow_color_a.x, ctx->eyeshadow_color_a.y, ctx->eyeshadow_color_a.z,
        ctx->eyeshadow_color_b.x, ctx->eyeshadow_color_b.y, ctx->eyeshadow_color_b.z,
        ctx->eyeshadow_spread,
        ctx->eyeshadow_shimmer ? "true" : "false",
        ctx->enable_lipstick ? "true" : "false",
        ctx->lipstick_color.x, ctx->lipstick_color.y, ctx->lipstick_color.z,
        ctx->lipstick_gloss, ctx->lipstick_coverage,
        ctx->enable_contour ? "true" : "false",
        ctx->contour_strength, ctx->highlight_strength,
        ctx->highlight_color.x, ctx->highlight_color.y, ctx->highlight_color.z);
    FA(frag, sz, fp,
        "void main() {\n"
        "  vec3 col = vec3(0.);\n"
        "  float alpha = 0.;\n"
        "  vec2 uv = v_uv;\n"
        "  vec3 N = normalize(v_normal);\n"
        "  vec3 L = normalize(u_light_dir);\n"
        "  vec3 V = normalize(u_view_dir);\n"
        "  float ndl = max(dot(N,L),0.2);\n\n"
        "  // Foundation overlay\n"
        "  if(u_foundation) {\n"
        "    col += u_found_color * u_found_cov;\n"
        "    alpha += u_found_cov;\n"
        "    // Finish: matte absorbe luz, glow la refleja\n"
        "    float spec_f = u_found_finish * pow(max(dot(reflect(-L,N),V),0.),24.)*0.3;\n"
        "    col += vec3(spec_f);\n"
        "  }\n"
        "  // Blush: región de mejillas (gaussiana en UV)\n"
        "  if(u_blush) {\n"
        "    float bd = length(uv - u_blush_pos);\n"
        "    float bmask = exp(-bd*bd*30.) * u_blush_int;\n"
        "    col = mix(col, u_blush_color, bmask);\n"
        "    alpha = max(alpha, bmask);\n"
        "  }\n"
        "  // Eye shadow: zona orbital superior\n"
        "  if(u_eyeshadow && uv.y > 0.62) {\n"
        "    float e_t = clamp((uv.y-0.62)/u_eshadow_spr, 0., 1.);\n"
        "    vec3 es_col = mix(u_eshadow_a, u_eshadow_b, e_t);\n"
        "    float shimmer = u_eshadow_shimmer\n"
        "      ? pow(max(dot(reflect(-L,N),V),0.),16.)*0.4 : 0.;\n"
        "    es_col += shimmer;\n"
        "    float e_mask = smoothstep(0.62, 0.75, uv.y);\n"
        "    col = mix(col, es_col, e_mask*0.8);\n"
        "    alpha = max(alpha, e_mask*0.8);\n"
        "  }\n"
        "  // Lipstick: zona labial (aproximada en UV)\n"
        "  if(u_lipstick && uv.y < 0.35) {\n"
        "    float lip_mask = smoothstep(0.35, 0.28, uv.y) * u_lip_cov;\n"
        "    float gloss = u_lip_gloss * pow(max(dot(reflect(-L,N),V),0.),32.);\n"
        "    vec3 lc = u_lip_color + vec3(gloss);\n"
        "    col = mix(col, lc, lip_mask);\n"
        "    alpha = max(alpha, lip_mask);\n"
        "  }\n"
        "  // Contour (sombra en pómulos bajos, mandíbula)\n"
        "  if(u_contour) {\n"
        "    float cont = (1.-abs(uv.x-0.5)*2.) * u_contour_str * 0.25;\n"
        "    col -= cont;\n"
        "    // Highlight en la cresta del pómulo\n"
        "    float hi = exp(-abs(uv.y-0.55)*20.) * u_highlight_str;\n"
        "    col += u_highlight_col * hi * 0.3;\n"
        "  }\n"
        "  col *= ndl;\n"
        "  fragColor = vec4(clamp(col,0.,1.), clamp(alpha,0.,1.));\n"
        "}\n");
    FA(js, jsz, jp,
        "// §F10 Makeup shader setup\n"
        "function rigMakeupSetUniforms(gl, prog, m) {\n"
        "  const L=n=>gl.getUniformLocation(prog,n);\n"
        "  gl.uniform1i(L('u_foundation'),   m.foundation?1:0);\n"
        "  gl.uniform3fv(L('u_found_color'), m.foundationColor||[0.9,0.75,0.65]);\n"
        "  gl.uniform1f(L('u_found_cov'),    m.foundationCoverage||0.7);\n"
        "  gl.uniform1f(L('u_found_finish'), m.foundationFinish||0.3);\n"
        "  gl.uniform1i(L('u_blush'),        m.blush?1:0);\n"
        "  gl.uniform3fv(L('u_blush_color'), m.blushColor||[0.8,0.3,0.3]);\n"
        "  gl.uniform1f(L('u_blush_int'),    m.blushIntensity||0.4);\n"
        "  gl.uniform2fv(L('u_blush_pos'),   m.blushPosition||[0.3,0.55]);\n"
        "  gl.uniform1i(L('u_eyeshadow'),    m.eyeshadow?1:0);\n"
        "  gl.uniform3fv(L('u_eshadow_a'),   m.eyeshadowA||[0.2,0.1,0.4]);\n"
        "  gl.uniform3fv(L('u_eshadow_b'),   m.eyeshadowB||[0.05,0.02,0.15]);\n"
        "  gl.uniform1f(L('u_eshadow_spr'),  m.eyeshadowSpread||0.06);\n"
        "  gl.uniform1i(L('u_eshadow_shimmer'),m.eyeshadowShimmer?1:0);\n"
        "  gl.uniform1i(L('u_lipstick'),     m.lipstick?1:0);\n"
        "  gl.uniform3fv(L('u_lip_color'),   m.lipstickColor||[0.7,0.1,0.1]);\n"
        "  gl.uniform1f(L('u_lip_gloss'),    m.lipstickGloss||0.5);\n"
        "  gl.uniform1f(L('u_lip_cov'),      m.lipstickCoverage||0.9);\n"
        "  gl.uniform1i(L('u_contour'),      m.contour?1:0);\n"
        "  gl.uniform1f(L('u_contour_str'),  m.contourStrength||0.4);\n"
        "  gl.uniform1f(L('u_highlight_str'),m.highlightStrength||0.5);\n"
        "  gl.uniform3fv(L('u_highlight_col'),m.highlightColor||[1.,0.95,0.85]);\n"
        "}\n");
    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    return fp + jp;
}
int rig_face_v2_moisture_shader(const RigFaceMoistureCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)malloc(FACE_SHADER_BUF / 2);
    if (!frag) return -1;
    int fp = 0, sz = FACE_SHADER_BUF / 2;
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp,
        "in vec2 v_uv; in vec3 v_normal; out vec4 fragColor;\n"
        "uniform float u_tear_level;   // %.4f\n"
        "uniform float u_tear_speed;   // %.4f\n"
        "uniform vec3  u_tear_color;   // (%.3f,%.3f,%.3f)\n"
        "uniform float u_lip_gloss;    // %.4f\n"
        "uniform float u_lip_gloss_sp; // %.4f\n"
        "uniform float u_sweat_dens;   // %.4f\n"
        "uniform float u_sweat_sz;     // %.4f\n"
        "uniform float u_sweat_sp;     // %.4f\n"
        "uniform vec3  u_light_dir; uniform vec3 u_view_dir; uniform float u_time;\n\n",
        ctx->tear_level, ctx->tear_streak_speed,
        ctx->tear_color.x, ctx->tear_color.y, ctx->tear_color.z,
        ctx->lip_gloss_amount, ctx->lip_gloss_specularity,
        ctx->sweat_density, ctx->sweat_bead_size, ctx->sweat_specularity);
    FA(frag, sz, fp,
        "void main() {\n"
        "  vec3 N = normalize(v_normal);\n"
        "  vec3 L = normalize(u_light_dir);\n"
        "  vec3 V = normalize(u_view_dir);\n"
        "  float alpha = 0.;\n"
        "  vec3 col = vec3(0.);\n"
        "  // ── Tear streak ──────────────────────────────────────────\n"
        "  if(u_tear_level > 0.) {\n"
        "    float streak = vnoise(vec3(v_uv.x*3., v_uv.y*0.5 + u_time*u_tear_speed, 0.));\n"
        "    streak = smoothstep(0.55, 0.65, streak);\n"
        "    float tear_mask = streak * u_tear_level * (1.-v_uv.y);\n"
        "    float spec_t = pow(max(dot(reflect(-L,N),V),0.),64.);\n"
        "    col += u_tear_color * tear_mask + vec3(spec_t)*tear_mask*0.8;\n"
        "    alpha += tear_mask * 0.55;\n"
        "  }\n"
        "  // ── Lip gloss ─────────────────────────────────────────────\n"
        "  if(u_lip_gloss > 0. && v_uv.y < 0.35) {\n"
        "    float gloss_mask = smoothstep(0.35,0.26,v_uv.y)*u_lip_gloss;\n"
        "    float spec_g = pow(max(dot(reflect(-L,N),V),0.), 48.)*u_lip_gloss_sp;\n"
        "    col += vec3(spec_g)*gloss_mask;\n"
        "    alpha += gloss_mask * 0.4;\n"
        "  }\n"
        "  // ── Sweat beads ───────────────────────────────────────────\n"
        "  if(u_sweat_dens > 0.) {\n"
        "    vec2 bead_uv = fract(v_uv * 30. * u_sweat_dens);\n"
        "    float bead_d = length(bead_uv - 0.5);\n"
        "    float bead = smoothstep(u_sweat_sz, u_sweat_sz-0.05, bead_d);\n"
        "    float spec_s = pow(max(dot(reflect(-L,N),V),0.),32.)*u_sweat_sp;\n"
        "    col += (vec3(0.85,0.88,0.9)+spec_s)*bead;\n"
        "    alpha += bead * 0.6;\n"
        "  }\n"
        "  fragColor = vec4(col, clamp(alpha,0.,1.));\n"
        "}\n");
    out->glsl_frag = frag;
    out->ok = true;
    return fp;
}
int rig_face_v2_expression_blend(const RigFaceExprCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *js = (char*)malloc(FACE_JS_BUF * 2);
    if (!js) return -1;
    int jp = 0, jsz = FACE_JS_BUF * 2;
    FA(js, jsz, jp,
        "// §F12 Expression Library — 52 FACS + 48 compound\n"
        "const RIG_FACS_COUNT = %d;\n"
        "const RIG_COMPOUND_EMOTIONS = {\n"
        "  joy:         {au:[%d,%d,%d],w:[1.,1.,0.3]},\n"
        "  sorrow:      {au:[%d,%d,%d,%d],w:[0.8,1.,0.6,0.4]},\n"
        "  anger:       {au:[%d,%d,%d,%d],w:[1.,0.8,0.7,0.5]},\n"
        "  fear:        {au:[%d,%d,%d,%d,%d,%d],w:[0.8,0.6,0.5,0.9,0.7,0.6]},\n"
        "  surprise:    {au:[%d,%d,%d,%d],w:[0.9,0.7,1.,0.8]},\n"
        "  disgust:     {au:[%d,%d,%d],w:[1.,0.7,0.5]},\n"
        "  contempt:    {au:[%d,%d],w:[0.7,0.5]},\n"
        "  ecstasy:     {au:[%d,%d,%d,%d],w:[1.,1.,0.8,0.6]},\n"
        "  grief:       {au:[%d,%d,%d,%d],w:[1.,1.,0.9,0.7]},\n"
        "  terror:      {au:[%d,%d,%d,%d,%d],w:[1.,1.,1.,0.9,0.7]},\n"
        "  amazement:   {au:[%d,%d,%d],w:[1.,0.8,0.9]},\n"
        "  admiration:  {au:[%d,%d],w:[0.5,0.3]},\n"
        "  boredom:     {au:[%d,%d],w:[0.4,0.3]},\n"
        "  relief:      {au:[%d,%d],w:[0.5,0.4]},\n"
        "  amusement:   {au:[%d,%d,%d],w:[0.8,1.,0.4]},\n"
        "};\n\n",
        (int)FACS_COUNT,
        (int)FACS_AU6_CHEEK_RAISE, (int)FACS_AU12_LIP_CORNER_PULL, (int)FACS_AU25_LIPS_PART,
        (int)FACS_AU1_INNER_BROW_RAISE, (int)FACS_AU4_BROW_LOWER, (int)FACS_AU15_LIP_CORNER_DEPRESS, (int)FACS_AU17_CHIN_RAISE,
        (int)FACS_AU4_BROW_LOWER, (int)FACS_AU5_UPPER_LID_RAISE, (int)FACS_AU7_LID_TIGHTEN, (int)FACS_AU23_LIP_TIGHTEN,
        (int)FACS_AU1_INNER_BROW_RAISE, (int)FACS_AU2_OUTER_BROW_RAISE, (int)FACS_AU4_BROW_LOWER,
        (int)FACS_AU5_UPPER_LID_RAISE, (int)FACS_AU20_LIP_STRETCH, (int)FACS_AU26_JAW_DROP,
        (int)FACS_AU1_INNER_BROW_RAISE, (int)FACS_AU2_OUTER_BROW_RAISE, (int)FACS_AU5_UPPER_LID_RAISE, (int)FACS_AU26_JAW_DROP,
        (int)FACS_AU9_NOSE_WRINKLE, (int)FACS_AU15_LIP_CORNER_DEPRESS, (int)FACS_AU16_LOWER_LIP_DEPRESS,
        (int)FACS_AU12_LIP_CORNER_PULL, (int)FACS_AU14_DIMPLER,
        (int)FACS_AU6_CHEEK_RAISE, (int)FACS_AU12_LIP_CORNER_PULL, (int)FACS_AU25_LIPS_PART, (int)FACS_AU26_JAW_DROP,
        (int)FACS_AU1_INNER_BROW_RAISE, (int)FACS_AU4_BROW_LOWER, (int)FACS_AU15_LIP_CORNER_DEPRESS, (int)FACS_AU17_CHIN_RAISE,
        (int)FACS_AU1_INNER_BROW_RAISE, (int)FACS_AU2_OUTER_BROW_RAISE, (int)FACS_AU4_BROW_LOWER,
        (int)FACS_AU5_UPPER_LID_RAISE, (int)FACS_AU26_JAW_DROP,
        (int)FACS_AU2_OUTER_BROW_RAISE, (int)FACS_AU5_UPPER_LID_RAISE, (int)FACS_AU26_JAW_DROP,
        (int)FACS_AU6_CHEEK_RAISE, (int)FACS_AU12_LIP_CORNER_PULL,
        (int)FACS_AU41_LID_DROOP, (int)FACS_AU16_LOWER_LIP_DEPRESS,
        (int)FACS_AU6_CHEEK_RAISE, (int)FACS_AU25_LIPS_PART,
        (int)FACS_AU6_CHEEK_RAISE, (int)FACS_AU12_LIP_CORNER_PULL, (int)FACS_AU14_DIMPLER);
    FA(js, jsz, jp,
        "function rigApplyExpression(facs, emotion_name, weight) {\n"
        "  const em = RIG_COMPOUND_EMOTIONS[emotion_name];\n"
        "  if(!em) return 0;\n"
        "  em.au.forEach((au,i) => {\n"
        "    facs[au] = Math.min(1., facs[au] + em.w[i]*weight);\n"
        "  });\n"
        "}\n\n"
        "function rigBlendFACS(facs_a, facs_b, t) {\n"
        "  const out = new Float32Array(RIG_FACS_COUNT);\n"
        "  for(let i=0;i<RIG_FACS_COUNT;i++) out[i]=facs_a[i]*(1.-t)+facs_b[i]*t;\n"
        "  return out;\n"
        "}\n\n"
        "function rigFACSToMorphWeights(facs, blendshape_map) {\n"
        "  // blendshape_map: [{facs_idx, bs_idx, weight},...]\n"
        "  const out = new Float32Array(blendshape_map.length);\n"
        "  blendshape_map.forEach((m,i) => {\n"
        "    out[i] = facs[m.facs_idx] * m.weight;\n"
        "  });\n"
        "  return out;\n"
        "}\n\n"
        "// Preset FACS weights\n"
        "const RIG_FACS_WEIGHTS = {\n");
    FA(js, jsz, jp, "  weights: new Float32Array([");
    for (int i = 0; i < (int)FACS_COUNT; i++) {
        FA(js, jsz, jp, "%.4f%s", ctx->weights[i], (i < (int)FACS_COUNT - 1) ? "," : "");
    }
    FA(js, jsz, jp, "]),\n  blend_speed: %.4f\n};\n", ctx->blend_speed);
    out->js = js;
    out->ok = true;
    return jp;
}
int rig_face_v2_phoneme_blend(const RigFacePhonemeCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *js = (char*)malloc(FACE_JS_BUF);
    if (!js) return -1;
    int jp = 0, jsz = FACE_JS_BUF;
    static const struct { int au[6]; float w[6]; int n; } vis_map[(int)VIS_COUNT] = {
         {{(int)FACS_AU25_LIPS_PART},{0.0f},1},
         {{(int)FACS_AU26_JAW_DROP,(int)FACS_AU25_LIPS_PART},{0.8f,0.6f},2},
         {{(int)FACS_AU26_JAW_DROP,(int)FACS_AU20_LIP_STRETCH},{0.6f,0.5f},2},
         {{(int)FACS_AU18_LIP_PUCKERER,(int)FACS_AU22_LIP_FUNNEL},{0.9f,0.7f},2},
         {{(int)FACS_AU26_JAW_DROP,(int)FACS_AU22_LIP_FUNNEL},{0.5f,0.6f},2},
         {{(int)FACS_AU26_JAW_DROP,(int)FACS_AU25_LIPS_PART},{0.3f,0.4f},2},
         {{(int)FACS_AU25_LIPS_PART},{0.3f},1},
         {{(int)FACS_AU20_LIP_STRETCH,(int)FACS_AU25_LIPS_PART},{0.8f,0.4f},2},
         {{(int)FACS_AU20_LIP_STRETCH,(int)FACS_AU28_LIP_SUCK},{0.5f,0.3f},2},
         {{(int)FACS_AU25_LIPS_PART,(int)FACS_AU16_LOWER_LIP_DEPRESS},{0.6f,0.3f},2},
         {{(int)FACS_AU25_LIPS_PART},{0.4f},1},
         {{(int)FACS_AU26_JAW_DROP},{0.4f},1},
         {{(int)FACS_AU22_LIP_FUNNEL,(int)FACS_AU18_LIP_PUCKERER},{0.5f,0.4f},2},
         {{(int)FACS_AU20_LIP_STRETCH},{0.4f},1},
         {{(int)FACS_AU18_LIP_PUCKERER},{0.5f},1},
         {{(int)FACS_AU25_LIPS_PART},{0.1f},1},
         {{(int)FACS_AU24_LIP_PRESSOR},{0.8f},1},
         {{(int)FACS_AU24_LIP_PRESSOR,(int)FACS_AU25_LIPS_PART},{1.0f,0.0f},2},
         {{(int)FACS_AU22_LIP_FUNNEL},{0.4f},1},
         {{(int)FACS_AU25_LIPS_PART},{0.3f},1},
         {{(int)FACS_AU20_LIP_STRETCH,(int)FACS_AU25_LIPS_PART},{0.5f,0.3f},2},
         {{(int)FACS_AU18_LIP_PUCKERER},{0.7f},1},
         {{(int)FACS_AU26_JAW_DROP,(int)FACS_AU22_LIP_FUNNEL},{0.4f,0.5f},2},
    };
    static const char *vis_names[(int)VIS_COUNT] = {
        "REST","AH","AE","OO","OH","EH","IH","EE","FF","TH",
        "DD","KK","CH","SS","SH","NN","MM","PP","RR","LL","YY","WW","UH"
    };
    FA(js, jsz, jp,
        "// §F13 Phoneme / Viseme System — 44 visemes + co-articulation\n"
        "// current=%s next=%s blend=%.3f intensity=%.3f co_artic=%s\n"
        "const RIG_VISEME_MAP = {\n",
        vis_names[(int)fc_clamp((float)ctx->current,0,(float)VIS_COUNT-1)],
        vis_names[(int)fc_clamp((float)ctx->next,0,(float)VIS_COUNT-1)],
        ctx->blend, ctx->intensity,
        ctx->enable_co_artic ? "true" : "false");
    for (int v = 0; v < (int)VIS_COUNT; v++) {
        FA(js, jsz, jp, "  '%s': {au:[", vis_names[v]);
        for (int k = 0; k < vis_map[v].n; k++) {
            FA(js, jsz, jp, "%d%s", vis_map[v].au[k], k < vis_map[v].n-1 ? "," : "");
        }
        FA(js, jsz, jp, "],w:[");
        for (int k = 0; k < vis_map[v].n; k++) {
            FA(js, jsz, jp, "%.2ff%s", vis_map[v].w[k], k < vis_map[v].n-1 ? "," : "");
        }
        FA(js, jsz, jp, "]},\n");
    }
    FA(js, jsz, jp,
        "};\n\n"
        "function rigApplyViseme(facs, viseme_name, intensity) {\n"
        "  const vm = RIG_VISEME_MAP[viseme_name];\n"
        "  if(!vm) return 0;\n"
        "  vm.au.forEach((au,i) => { facs[au] = Math.min(1., vm.w[i]*intensity); });\n"
        "}\n\n"
        "function rigBlendVisemes(facs, curr, next, t, intensity) {\n"
        "  const fa = new Float32Array(facs.length);\n"
        "  const fb = new Float32Array(facs.length);\n"
        "  rigApplyViseme(fa, curr, intensity);\n"
        "  rigApplyViseme(fb, next, intensity);\n"
        "  for(let i=0;i<facs.length;i++) facs[i]=fa[i]*(1.-t)+fb[i]*t;\n"
        "}\n\n"
        "// Co-articulation: anticipa el siguiente fonema (Anticipation Time=%.1fms)\n"
        "function rigCoArticulate(facs, phoneme_queue, t, intensity) {\n"
        "  if(!phoneme_queue || phoneme_queue.length < 2) return 0;\n"
        "  const curr = phoneme_queue[0], next = phoneme_queue[1];\n"
        "  const blend = Math.min(1., t / %.4f);\n"
        "  rigBlendVisemes(facs, curr, next, blend*0.3, intensity);\n"
        "}\n",
        ctx->anticipation_time,
        ctx->anticipation_time / 1000.0f);
    out->js = js;
    out->ok = true;
    return jp;
}
int rig_face_v2_assembly(const RigFaceAssemblyCtx *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    char *html = (char*)malloc(FACE_JS_BUF * 4);
    char *css  = (char*)malloc(FACE_JS_BUF);
    char *js   = (char*)malloc(FACE_JS_BUF * 8);
    char *frag = (char*)malloc(FACE_SHADER_BUF * 2);
    char *vert = (char*)malloc(FACE_SHADER_BUF);
    if (!html || !css || !js || !frag || !vert) {
        free(html); free(css); free(js); free(frag); free(vert);
        return -1;
    }
    int hp = 0, cp = 0, jp = 0, fp = 0, vp = 0;
    int hsz = FACE_JS_BUF*4, csz = FACE_JS_BUF,
        jsz = FACE_JS_BUF*8, sz = FACE_SHADER_BUF*2, vsz = FACE_SHADER_BUF;
    static const char *lod_names[] = {"ULTRA","HIGH","MEDIUM","LOW"};
    static const int   lod_tris[]  = {200000, 80000, 30000, 8000};
    const char *lname = (ctx->lod < 4) ? lod_names[ctx->lod] : "HIGH";
    int lod_tri = (ctx->lod < 4) ? lod_tris[ctx->lod] : lod_tris[1];
    FA(html, hsz, hp,
        "<!DOCTYPE html>\n"
        "<html lang='es'>\n"
        "<head>\n"
        "  <meta charset='UTF-8'>\n"
        "  <meta name='viewport' content='width=device-width,initial-scale=1,viewport-fit=cover'>\n"
        "  <title>RigCom v24 THE SANTORIUM OF COMPILER — Avatar — φ Sovereign</title>\n"
        "  <!-- phi_font_3d: tipografia soberana φ — sin Google Fonts -->\n"
        "</head>\n"
        "<body>\n"
        "  <div id='rig-face-root'>\n"
        "    <canvas id='rig-face-canvas'></canvas>\n"
        "    <div id='rig-hud'>\n"
        "      <span class='hud-title'>RIGFACE v2.0</span>\n"
        "      <span class='hud-lod'>LOD: %s · TRIS: %d</span>\n"
        "      <span class='hud-phi'>φ = 1.6180339887</span>\n"
        "      <span class='hud-author'>Richard Felipe Urbina</span>\n"
        "    </div>\n"
        "    <div id='rig-controls'>\n"
        "      <label>Expression</label>\n"
        "      <select id='rig-emotion'>"
        "        <option>joy</option><option>sorrow</option>"
        "        <option>anger</option><option>surprise</option>"
        "        <option>disgust</option><option>contempt</option>"
        "        <option>fear</option><option>neutral</option>"
        "      </select>\n"
        "      <label>Age</label>\n"
        "      <input type='range' id='rig-age' min='0' max='100' value='25'>\n"
        "      <label>Melanin</label>\n"
        "      <input type='range' id='rig-melanin' min='0' max='100' value='20'>\n"
        "    </div>\n"
        "  </div>\n"
        "  <script type='module' src='rig_face_v2_runtime.js'></script>\n"
        "</body>\n"
        "</html>\n",
        lname, lod_tri);
    FA(css, csz, cp,
        "/* RigFace Avatar UI — φ Sovereign Dark */\n"
        ":root {\n"
        "  --bg: #05060a; --gold: #f8d050; --gold2: #D4A848;\n"
        "  --crimson: #d8485a; --blue: #4080d8;\n"
        "  --phi: 1.6180339887;\n"
        "}\n"
        "*, *::before, *::after { box-sizing:border-box; margin:0; padding:0; }\n"
        "html, body { width:100%%; height:100%%; background:var(--bg); overflow:hidden; }\n"
        "#rig-face-root { position:relative; width:100vw; height:100vh; }\n"
        "#rig-face-canvas {\n"
        "  display:block; width:100%%; height:100%%;\n"
        "  touch-action:none;\n"
        "}\n"
        "#rig-hud {\n"
        "  position:fixed; top:clamp(8px,2vh,20px); left:clamp(8px,2vw,24px);\n"
        "  display:flex; flex-direction:column; gap:4px;\n"
        "  font-family:'Courier New','Courier',monospace; font-size:clamp(9px,1.2vw,13px);\n"
        "  color:var(--gold); opacity:0.82;\n"
        "  pointer-events:none; z-index:10;\n"
        "}\n"
        ".hud-title { font-family:'Cinzel Decorative','Palatino Linotype','Palatino',Georgia,serif; font-size:clamp(12px,1.8vw,18px);\n"
        "             letter-spacing:0.15em; color:var(--gold); text-transform:uppercase; }\n"
        ".hud-phi { color:var(--blue); font-size:0.9em; }\n"
        ".hud-author { color:var(--crimson); font-size:0.85em; }\n"
        "#rig-controls {\n"
        "  position:fixed; bottom:clamp(12px,3vh,32px); left:50%%; transform:translateX(-50%%);\n"
        "  display:flex; align-items:center; gap:12px; flex-wrap:wrap; justify-content:center;\n"
        "  background:rgba(5,6,10,0.75); border:1px solid var(--gold);\n"
        "  border-radius:8px; padding:10px 20px;\n"
        "  font-family:'Cinzel','Palatino Linotype','Palatino',Georgia,serif; color:var(--gold); font-size:clamp(11px,1.4vw,14px);\n"
        "  backdrop-filter:blur(8px); z-index:10;\n"
        "}\n"
        "#rig-controls label { color:var(--gold2); margin-right:4px; }\n"
        "#rig-controls select, #rig-controls input {\n"
        "  background:rgba(5,6,10,0.9); color:var(--gold);\n"
        "  border:1px solid var(--gold2); border-radius:4px;\n"
        "  padding:2px 8px; font-family:'Cinzel','Palatino Linotype',Georgia,serif;\n"
        "  appearance:none; -webkit-appearance:none;\n"
        "}\n");
    FA(vert, vsz, vp, "%s", GLSL_HEADER);
    FA(vert, vsz, vp,
        "// §F14 Avatar Assembly — Vertex Shader\n"
        "in vec3  a_pos;\n"
        "in vec3  a_normal;\n"
        "in vec3  a_tangent;\n"
        "in vec2  a_uv;\n"
        "in vec4  a_color;       // vertex color: regional SSS/melanin\n"
        "in vec4  a_bone_ids;    // dual-quaternion skinning bone indices\n"
        "in vec4  a_bone_w;      // skinning weights\n"
        "in float a_region;      // RIG_REGION_*\n\n"
        "// Morph targets (FACS 52 + custom 76)\n"
        "uniform sampler2D u_morph_tex;   // Morph delta texture\n"
        "uniform float u_morph_w[%d];     // FACS weights\n"
        "uniform int   u_morph_count;\n\n"
        "// Matrices\n"
        "uniform mat4  u_mvp;\n"
        "uniform mat4  u_model;\n"
        "uniform mat3  u_normal_mat;\n"
        "uniform float u_time;\n\n"
        "out vec3  v_pos;\n"
        "out vec3  v_normal;\n"
        "out vec3  v_tangent;\n"
        "out vec3  v_bitangent;\n"
        "out vec2  v_uv;\n"
        "out vec4  v_color;\n"
        "out float v_region;\n\n"
        "void main() {\n"
        "  vec3 pos    = a_pos;\n"
        "  vec3 nrm    = a_normal;\n"
        "  // Apply FACS morphs from texture\n"
        "  for(int i=0; i<u_morph_count && i<%d; i++) {\n"
        "    if(u_morph_w[i] < 0.001) continue;\n"
        "    vec2 tex_uv = vec2((float(i)+0.5)/float(%d), a_uv.y);\n"
        "    vec3 delta  = texture(u_morph_tex, tex_uv).xyz * 2.0 - 1.0;\n"
        "    pos  += delta * u_morph_w[i];\n"
        "    // Normal delta (simplified: renormalize after)\n"
        "  }\n"
        "  // Age sag deformation\n"
        "  float age_sag = %.4f;\n"
        "  if(pos.y < 0.0) pos.y -= abs(pos.y)*age_sag*0.015;\n"
        "  // Final outputs\n"
        "  vec4 world = u_model * vec4(pos, 1.0);\n"
        "  v_pos       = world.xyz;\n"
        "  v_normal    = normalize(u_normal_mat * nrm);\n"
        "  v_tangent   = normalize(u_normal_mat * a_tangent);\n"
        "  v_bitangent = cross(v_normal, v_tangent);\n"
        "  v_uv        = a_uv;\n"
        "  v_color     = a_color;\n"
        "  v_region    = a_region;\n"
        "  gl_Position = u_mvp * vec4(pos, 1.0);\n"
        "}\n",
        (int)FACS_COUNT,
        (int)FACS_COUNT, (int)FACS_COUNT,
        (float)ctx->age.skin_sag * ctx->age.age / 100.0f);
    FA(frag, sz, fp, "%s", GLSL_HEADER);
    FA(frag, sz, fp, "%s", GLSL_NOISE_FUNCS);
    FA(frag, sz, fp, "%s", GLSL_PBR_FUNCS);
    FA(frag, sz, fp,
        "// §F14 Avatar Assembly Fragment — Composición de todos los sistemas\n"
        "in vec3  v_pos; in vec3 v_normal; in vec3 v_tangent;\n"
        "in vec3  v_bitangent; in vec2 v_uv; in vec4 v_color;\n"
        "in float v_region;\n"
        "out vec4 fragColor;\n\n"
        "// ── Samplers (atlas de texturas) ──────────────────────────\n"
        "uniform sampler2D u_atlas_albedo;    // Color base\n"
        "uniform sampler2D u_atlas_normal;    // Normal map\n"
        "uniform sampler2D u_atlas_roughness; // Roughness/Metallic\n"
        "uniform sampler2D u_atlas_sss;       // SSS thickness\n"
        "uniform sampler2D u_atlas_pore;      // Pore normal\n"
        "uniform sampler2D u_atlas_wrinkle;   // Wrinkle normal\n"
        "uniform sampler2D u_atlas_vascular;  // Vascular map\n"
        "uniform sampler2D u_atlas_makeup;    // Makeup layer\n"
        "uniform sampler2D u_atlas_moisture;  // Tear/sweat layer\n"
        "uniform sampler2D u_iris_tex;        // Iris procedural\n\n"
        "// ── Bioquímica ────────────────────────────────────────────\n"
        "uniform float u_melanin;   uniform float u_hemoglobin;\n"
        "uniform float u_carotene;  uniform float u_bilirubin;\n"
        "uniform float u_age_norm;\n\n"
        "// ── SSS ───────────────────────────────────────────────────\n"
        "uniform vec3  u_sss_epid;  uniform vec3  u_sss_derm;\n"
        "uniform float u_sss_w;\n\n"
        "// ── PBR ───────────────────────────────────────────────────\n"
        "uniform float u_roughness; uniform float u_ior;\n"
        "uniform float u_pore_scale;\n\n"
        "// ── Luz ───────────────────────────────────────────────────\n"
        "uniform vec3  u_light_dir; uniform vec3  u_light_color;\n"
        "uniform vec3  u_view_dir;  uniform vec3  u_env;\n"
        "uniform float u_light_int;\n\n"
        "// ── Opciones ──────────────────────────────────────────────\n"
        "uniform float u_makeup_blend; // 0=sin maquillaje 1=maquillaje completo\n"
        "uniform float u_moisture_blend;\n"
        "uniform float u_eye_blink;    // 0=abierto 1=cerrado\n"
        "uniform float u_time;\n\n");
    FA(frag, sz, fp,
        "// ── Funciones de piel ─────────────────────────────────────\n"
        "vec3 skin_albedo_bio(vec3 base) {\n"
        "  base *= vec3(1.-u_melanin*0.55, 1.-u_melanin*0.38, 1.-u_melanin*0.72);\n"
        "  base += vec3(u_hemoglobin*0.22, u_hemoglobin*0.04, u_hemoglobin*0.02);\n"
        "  base += vec3(u_carotene*0.18,   u_carotene*0.12,   0.);\n"
        "  return clamp(base,0.,1.);\n"
        "}\n"
        "vec3 sss_approx(vec3 a, float thick) {\n"
        "  vec3 e = a*exp(-thick*(1./max(u_sss_epid,vec3(0.001))));\n"
        "  vec3 d = a*exp(-thick*(1./max(u_sss_derm, vec3(0.001))));\n"
        "  vec3 s = e*0.35+d*0.50;\n"
        "  s += a*exp(-thick*PHI_INV)*vec3(1.,0.72,0.46)*0.15;\n"
        "  return s;\n"
        "}\n\n"
        "void main() {\n"
        "  // ── 1. Normal compuesta ───────────────────────────────────\n"
        "  vec3 nm  = texture(u_atlas_normal,    v_uv).rgb*2.-1.;\n"
        "  vec3 pnm = texture(u_atlas_pore,      fract(v_uv*u_pore_scale)).rgb*2.-1.*0.4;\n"
        "  vec3 wnm = texture(u_atlas_wrinkle,   v_uv).rgb*2.-1.*u_age_norm;\n"
        "  vec3 Nts = normalize(nm+pnm+wnm);\n"
        "  mat3 TBN = mat3(normalize(v_tangent),normalize(v_bitangent),normalize(v_normal));\n"
        "  vec3 N   = normalize(TBN*Nts);\n\n"
        "  // ── 2. Albedo bioquímico ──────────────────────────────────\n"
        "  vec3 alb  = skin_albedo_bio(texture(u_atlas_albedo,v_uv).rgb * v_color.rgb);\n\n"
        "  // ── 3. Roughness adaptativo ───────────────────────────────\n"
        "  float rgh = u_roughness * texture(u_atlas_roughness,v_uv).r;\n"
        "  rgh = clamp(rgh,0.04,1.);\n\n"
        "  // ── 4. SSS ───────────────────────────────────────────────\n"
        "  float thick    = texture(u_atlas_sss,v_uv).r;\n"
        "  vec3  vascular = texture(u_atlas_vascular,v_uv).rgb;\n"
        "  vec3  sss      = sss_approx(alb, thick);\n"
        "  sss += vascular*u_hemoglobin*vec3(0.3,0.05,0.05)*0.1;\n\n"
        "  // ── 5. BRDF PBR ──────────────────────────────────────────\n"
        "  vec3 L=normalize(u_light_dir),V=normalize(u_view_dir);\n"
        "  vec3 H=normalize(V+L);\n"
        "  float ndl=max(dot(N,L),0.),ndv=max(dot(N,V),0.);\n"
        "  float F0f=pow((1.-u_ior)/(1.+u_ior),2.);\n"
        "  vec3 F0=vec3(F0f);\n"
        "  vec3 F=fresnelSchlick(max(dot(H,V),0.),F0);\n"
        "  float D=ggxNDF(N,H,rgh),G=schlickGGX(ndv,rgh)*schlickGGX(ndl,rgh);\n"
        "  vec3 spec=(D*G*F)/max(4.*ndv*ndl,0.001);\n"
        "  vec3 kD=(1.-F);\n"
        "  vec3 direct=(kD*alb/PI+spec)*ndl*u_light_color*u_light_int;\n\n"
        "  // ── 6. Ambiente + SSS ────────────────────────────────────\n"
        "  vec3 ambient=u_env*alb*0.04 + sss*u_sss_w*0.45;\n"
        "  vec3 color = direct + ambient;\n\n"
        "  // ── 7. Capa de maquillaje ────────────────────────────────\n"
        "  vec4 makeup = texture(u_atlas_makeup, v_uv);\n"
        "  color = mix(color, makeup.rgb, makeup.a*u_makeup_blend);\n\n"
        "  // ── 8. Capa de humedad (lágrimas/gloss/sudor) ────────────\n"
        "  vec4 moist = texture(u_atlas_moisture, v_uv);\n"
        "  color += moist.rgb * moist.a * u_moisture_blend;\n\n"
        "  // ── 9. Párpados (blink) ───────────────────────────────────\n"
        "  // Región 15/16 = ojos: oscurecer con el blink\n"
        "  if(v_region > 14.5 && v_region < 16.5) {\n"
        "    color = mix(color, vec3(0.02,0.01,0.01), u_eye_blink);\n"
        "  }\n\n"
        "  // ── 10. Tone mapping ACES + gamma ────────────────────────\n"
        "  color = aces_tonemap(color);\n"
        "  color = pow(clamp(color,0.,1.),vec3(1./2.2));\n"
        "  fragColor = vec4(color, 1.0);\n"
        "}\n");
    FA(js, jsz, jp,
        "// §F14 RigCom v24 THE SANTORIUM OF COMPILER — Avatar Runtime\n"
        "// LOD=%s · φ=1.6180339887 · Richard Felipe Urbina\n"
        "'use strict';\n\n"
        "const PHI = 1.6180339887;\n"
        "const RIGFACE_VERSION = '2.0';\n\n"
        "class RigFaceAvatar {\n"
        "  constructor(canvas, opts={}) {\n"
        "    this.canvas  = canvas;\n"
        "    this.gl      = canvas.getContext('webgl2', {antialias:true,alpha:false});\n"
        "    if(!this.gl) throw new Error('RigFace: WebGL2 requerido');\n"
        "    this.gl.enable(this.gl.DEPTH_TEST);\n"
        "    this.gl.enable(this.gl.BLEND);\n"
        "    this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.ONE_MINUS_SRC_ALPHA);\n"
        "    this.gl.cullFace(this.gl.BACK);\n"
        "    this.gl.enable(this.gl.CULL_FACE);\n"
        "    this.programs  = {};\n"
        "    this.buffers   = {};\n"
        "    this.textures  = {};\n"
        "    this.facs      = new Float32Array(%d);\n"
        "    this.morphW    = new Float32Array(%d);\n"
        "    this.animator  = null;\n"
        "    this.time      = 0;\n"
        "    this.rot_y     = 0;\n"
        "    this.rot_x     = 0;\n"
        "    this.zoom      = 1.0;\n"
        "    this.touching  = false;\n"
        "    this.skin = {\n"
        "      melanin:%.4f, hemoglobin:%.4f, carotene:%.4f,\n"
        "      roughness:%.4f, ior:%.4f,\n"
        "      sssEpid:[%.4f,%.4f,%.4f],\n"
        "      sssDerm:[%.4f,%.4f,%.4f],\n"
        "      sssW:%.4f,\n"
        "      poreScale:%.1f\n"
        "    };\n"
        "    this.ageNorm   = %.4f;\n"
        "    this.blinkVal  = 0.;\n"
        "    this.makeupBlend   = %.4f;\n"
        "    this.moistureBlend = %.4f;\n"
        "    this._resize();\n"
        "    this._bindInput();\n"
        "  }\n\n",
        lname,
        (int)FACS_COUNT, (int)FACS_COUNT,
        ctx->skin.melanin, ctx->skin.hemoglobin, ctx->skin.carotene,
        ctx->skin.lipid_roughness, ctx->skin.specular_ior,
        ctx->skin.scatter_radius_epidermis.x,
        ctx->skin.scatter_radius_epidermis.y,
        ctx->skin.scatter_radius_epidermis.z,
        ctx->skin.scatter_radius_dermis.x,
        ctx->skin.scatter_radius_dermis.y,
        ctx->skin.scatter_radius_dermis.z,
        0.4f,
        ctx->skin.pore_scale,
        fc_clamp(ctx->age.age / 100.0f, 0.0f, 1.0f),
        ctx->makeup.enable_foundation ? 1.0f : 0.0f,
        ctx->moisture.enable_tear_meniscus ? ctx->moisture.tear_level : 0.0f);
    FA(js, jsz, jp,
        "  _resize() {\n"
        "    const dpr = window.devicePixelRatio||1;\n"
        "    this.canvas.width  = this.canvas.clientWidth*dpr;\n"
        "    this.canvas.height = this.canvas.clientHeight*dpr;\n"
        "    this.gl.viewport(0,0,this.canvas.width,this.canvas.height);\n"
        "  }\n\n"
        "  _bindInput() {\n"
        "    let lastX=0,lastY=0,dist0=0;\n"
        "    this.canvas.addEventListener('pointerdown', e=>{\n"
        "      this.touching=true; lastX=e.clientX; lastY=e.clientY;\n"
        "      this.canvas.setPointerCapture(e.pointerId);\n"
        "    });\n"
        "    this.canvas.addEventListener('pointermove', e=>{\n"
        "      if(!this.touching) return 0;\n"
        "      this.rot_y += (e.clientX-lastX)*0.008;\n"
        "      this.rot_x += (e.clientY-lastY)*0.006;\n"
        "      this.rot_x = Math.max(-1.1,Math.min(1.1,this.rot_x));\n"
        "      lastX=e.clientX; lastY=e.clientY;\n"
        "    });\n"
        "    this.canvas.addEventListener('pointerup', ()=>this.touching=false);\n"
        "    this.canvas.addEventListener('wheel', e=>{\n"
        "      this.zoom *= (1.-e.deltaY*0.001); this.zoom=Math.max(0.4,Math.min(3.,this.zoom));\n"
        "      e.preventDefault();\n"
        "    },{passive:false});\n"
        "    // Gyroscope (DeviceOrientation)\n"
        "    if(window.DeviceOrientationEvent) {\n"
        "      window.addEventListener('deviceorientation', e=>{\n"
        "        if(!this.touching && e.gamma!=null) {\n"
        "          this.rot_y = e.gamma/90.*1.2;\n"
        "          this.rot_x = (e.beta-15.)/90.*0.8;\n"
        "        }\n"
        "      });\n"
        "    }\n"
        "    window.addEventListener('resize', ()=>this._resize());\n"
        "  }\n\n");
    FA(js, jsz, jp,
        "  buildShaders(vertSrc, fragSrc, label) {\n"
        "    const gl=this.gl;\n"
        "    const compile=(type,src)=>{\n"
        "      const s=gl.createShader(type);\n"
        "      gl.shaderSource(s,src); gl.compileShader(s);\n"
        "      if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))\n"
        "        throw new Error(`RigFace shader [${label}]: `+gl.getShaderInfoLog(s));\n"
        "      return s;\n"
        "    };\n"
        "    const prog=gl.createProgram();\n"
        "    gl.attachShader(prog,compile(gl.VERTEX_SHADER,vertSrc));\n"
        "    gl.attachShader(prog,compile(gl.FRAGMENT_SHADER,fragSrc));\n"
        "    gl.linkProgram(prog);\n"
        "    if(!gl.getProgramParameter(prog,gl.LINK_STATUS))\n"
        "      throw new Error('RigFace link: '+gl.getProgramInfoLog(prog));\n"
        "    this.programs[label]=prog;\n"
        "    return prog;\n"
        "  }\n\n"
        "  uploadMesh(geo, label) {\n"
        "    const gl=this.gl;\n"
        "    const vao=gl.createVertexArray();\n"
        "    gl.bindVertexArray(vao);\n"
        "    const buf={};\n"
        "    // VBO de posición\n"
        "    buf.pos=gl.createBuffer();\n"
        "    gl.bindBuffer(gl.ARRAY_BUFFER,buf.pos);\n"
        "    gl.bufferData(gl.ARRAY_BUFFER,geo.verts,gl.STATIC_DRAW);\n"
        "    gl.enableVertexAttribArray(0);\n"
        "    gl.vertexAttribPointer(0,3,gl.FLOAT,false,0,0);\n"
        "    // VBO normal\n"
        "    if(geo.normals) {\n"
        "      buf.nor=gl.createBuffer();\n"
        "      gl.bindBuffer(gl.ARRAY_BUFFER,buf.nor);\n"
        "      gl.bufferData(gl.ARRAY_BUFFER,geo.normals,gl.STATIC_DRAW);\n"
        "      gl.enableVertexAttribArray(1);\n"
        "      gl.vertexAttribPointer(1,3,gl.FLOAT,false,0,0);\n"
        "    }\n"
        "    // VBO uv\n"
        "    if(geo.uvs) {\n"
        "      buf.uv=gl.createBuffer();\n"
        "      gl.bindBuffer(gl.ARRAY_BUFFER,buf.uv);\n"
        "      gl.bufferData(gl.ARRAY_BUFFER,geo.uvs,gl.STATIC_DRAW);\n"
        "      gl.enableVertexAttribArray(2);\n"
        "      gl.vertexAttribPointer(2,2,gl.FLOAT,false,0,0);\n"
        "    }\n"
        "    // IBO\n"
        "    if(geo.indices) {\n"
        "      buf.ibo=gl.createBuffer();\n"
        "      gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,buf.ibo);\n"
        "      gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,geo.indices,gl.STATIC_DRAW);\n"
        "    }\n"
        "    gl.bindVertexArray(null);\n"
        "    this.buffers[label]={vao,buf,count:geo.indices?geo.indices.length:geo.verts.length/3};\n"
        "    return vao;\n"
        "  }\n\n");
    FA(js, jsz, jp,
        "  setUniforms(prog) {\n"
        "    const gl=this.gl, L=n=>gl.getUniformLocation(prog,n);\n"
        "    // Matrices\n"
        "    const asp = this.canvas.width/this.canvas.height;\n"
        "    const proj = rigPerspective(Math.PI/3., asp, 0.001, 100.);\n"
        "    const view = rigLookAt([0,0.02,0.18*this.zoom],[0,0.01,0.],[0,1,0]);\n"
        "    const model= rigRotY(this.rot_y, rigRotX(this.rot_x, rigIdentity()));\n"
        "    gl.uniformMatrix4fv(L('u_mvp'),false, rigMul(proj,rigMul(view,model)));\n"
        "    gl.uniformMatrix4fv(L('u_model'),false,model);\n"
        "    gl.uniformMatrix3fv(L('u_normal_mat'),false,rigNormalMat(model));\n"
        "    // Piel\n"
        "    gl.uniform1f(L('u_melanin'),   this.skin.melanin);\n"
        "    gl.uniform1f(L('u_hemoglobin'),this.skin.hemoglobin);\n"
        "    gl.uniform1f(L('u_carotene'),  this.skin.carotene);\n"
        "    gl.uniform1f(L('u_roughness'), this.skin.roughness);\n"
        "    gl.uniform1f(L('u_ior'),       this.skin.ior);\n"
        "    gl.uniform3fv(L('u_sss_epid'), new Float32Array(this.skin.sssEpid));\n"
        "    gl.uniform3fv(L('u_sss_derm'), new Float32Array(this.skin.sssDerm));\n"
        "    gl.uniform1f(L('u_sss_w'),     this.skin.sssW||0.4);\n"
        "    gl.uniform1f(L('u_pore_scale'),this.skin.poreScale||40.);\n"
        "    gl.uniform1f(L('u_age_norm'),  this.ageNorm);\n"
        "    // Luz\n"
        "    const t = this.time;\n"
        "    gl.uniform3f(L('u_light_dir'), Math.sin(t*0.2)*0.5+0.3, 0.8, Math.cos(t*0.2)*0.5+0.5);\n"
        "    gl.uniform3f(L('u_light_color'), 1., 0.97, 0.93);\n"
        "    gl.uniform1f(L('u_light_int'),   1.2);\n"
        "    gl.uniform3f(L('u_view_dir'),    0.,0.,-1.);\n"
        "    gl.uniform3f(L('u_env'),         0.03,0.04,0.06);\n"
        "    // Capas\n"
        "    gl.uniform1f(L('u_makeup_blend'),  this.makeupBlend);\n"
        "    gl.uniform1f(L('u_moisture_blend'),this.moistureBlend);\n"
        "    gl.uniform1f(L('u_eye_blink'),     this.blinkVal);\n"
        "    gl.uniform1f(L('u_time'),          t);\n"
        "    // FACS morph weights\n"
        "    gl.uniform1iv(L('u_morph_count'), [%d]);\n"
        "    // Pasar pesos de FACS como array\n"
        "    for(let i=0;i<%d;i++) {\n"
        "      gl.uniform1f(gl.getUniformLocation(prog,`u_morph_w[${i}]`),this.morphW[i]||0.);\n"
        "    }\n"
        "  }\n\n",
        (int)FACS_COUNT, (int)FACS_COUNT);
    FA(js, jsz, jp,
        "  frame(dt) {\n"
        "    this.time += dt;\n"
        "    const gl = this.gl;\n"
        "    gl.clearColor(0.02,0.02,0.04,1.);\n"
        "    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);\n"
        "    // Actualizar animador\n"
        "    if(this.animator) {\n"
        "      this.animator.update(dt);\n"
        "      this.blinkVal = this.animator.getBlinkValue();\n"
        "      this.morphW.set(this.animator.getFACS());\n"
        "    }\n"
        "    // Render pases en orden\n"
        "    const passes = ['skin','hair','lash','brow','beard','eye','mouth','makeup','moisture'];\n"
        "    passes.forEach(pass => {\n"
        "      const prog = this.programs[pass];\n"
        "      const buf  = this.buffers[pass];\n"
        "      if(!prog || !buf) return 0;\n"
        "      gl.useProgram(prog);\n"
        "      this.setUniforms(prog);\n"
        "      gl.bindVertexArray(buf.vao);\n"
        "      if(buf.buf.ibo) {\n"
        "        gl.drawElements(gl.TRIANGLES, buf.count, gl.UNSIGNED_SHORT, 0);\n"
        "      } else {\n"
        "        gl.drawArrays(gl.LINES, 0, buf.count);\n"
        "      }\n"
        "      gl.bindVertexArray(null);\n"
        "    });\n"
        "  }\n\n"
        "  // Control API\n"
        "  setEmotion(name, w=1.)     { if(this.animator) this.animator.setEmotion(name,w); }\n"
        "  setAge(years)              { this.ageNorm=Math.max(0,Math.min(1,years/100.)); }\n"
        "  setMelanin(v)              { this.skin.melanin=Math.max(0,Math.min(1,v)); }\n"
        "  setHemoglobin(v)           { this.skin.hemoglobin=Math.max(0,Math.min(1,v)); }\n"
        "  setMakeupBlend(v)          { this.makeupBlend=Math.max(0,Math.min(1,v)); }\n"
        "  setMoistureBlend(v)        { this.moistureBlend=Math.max(0,Math.min(1,v)); }\n"
        "}\n\n"
        "// ── Utilidades matemáticas φ ──────────────────────────────\n"
        "function rigIdentity(){return new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]);}\n"
        "function rigMul(a,b){\n"
        "  const r=new Float32Array(16);\n"
        "  for(let i=0;i<4;i++)for(let j=0;j<4;j++)\n"
        "    for(let k=0;k<4;k++) r[i*4+j]+=a[i*4+k]*b[k*4+j];\n"
        "  return r;\n"
        "}\n"
        "function rigPerspective(fov,asp,n,f){\n"
        "  const t=Math.tan(fov/2),r=new Float32Array(16);\n"
        "  r[0]=1/(asp*t);r[5]=1/t;r[10]=-(f+n)/(f-n);\n"
        "  r[11]=-1;r[14]=-(2*f*n)/(f-n);\n"
        "  return r;\n"
        "}\n"
        "function rigLookAt(eye,ctr,up){\n"
        "  const f=rigNorm([ctr[0]-eye[0],ctr[1]-eye[1],ctr[2]-eye[2]]);\n"
        "  const s=rigNorm(rigCross(f,up));\n"
        "  const u=rigCross(s,f);\n"
        "  const m=rigIdentity();\n"
        "  m[0]=s[0];m[4]=s[1];m[8]=s[2];\n"
        "  m[1]=u[0];m[5]=u[1];m[9]=u[2];\n"
        "  m[2]=-f[0];m[6]=-f[1];m[10]=-f[2];\n"
        "  m[12]=-(s[0]*eye[0]+s[1]*eye[1]+s[2]*eye[2]);\n"
        "  m[13]=-(u[0]*eye[0]+u[1]*eye[1]+u[2]*eye[2]);\n"
        "  m[14]=(f[0]*eye[0]+f[1]*eye[1]+f[2]*eye[2]);\n"
        "  return m;\n"
        "}\n"
        "function rigRotY(a,m=rigIdentity()){\n"
        "  const c=Math.cos(a),s=Math.sin(a),r=rigIdentity();\n"
        "  r[0]=c;r[2]=s;r[8]=-s;r[10]=c;\n"
        "  return rigMul(r,m);\n"
        "}\n"
        "function rigRotX(a,m=rigIdentity()){\n"
        "  const c=Math.cos(a),s=Math.sin(a),r=rigIdentity();\n"
        "  r[5]=c;r[6]=-s;r[9]=s;r[10]=c;\n"
        "  return rigMul(r,m);\n"
        "}\n"
        "function rigNorm(v){\n"
        "  const l=Math.sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])||1;\n"
        "  return[v[0]/l,v[1]/l,v[2]/l];\n"
        "}\n"
        "function rigCross(a,b){\n"
        "  return[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];\n"
        "}\n"
        "function rigNormalMat(m){\n"
        "  // 3x3 inversa transpuesta de la submatriz del modelo\n"
        "  const [a,b,c,d,e,f,g,h,i]=\n"
        "    [m[0],m[1],m[2],m[4],m[5],m[6],m[8],m[9],m[10]];\n"
        "  const det=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g)||1;\n"
        "  return new Float32Array([\n"
        "    (e*i-f*h)/det, (c*h-b*i)/det, (b*f-c*e)/det,\n"
        "    (f*g-d*i)/det, (a*i-c*g)/det, (c*d-a*f)/det,\n"
        "    (d*h-e*g)/det, (b*g-a*h)/det, (a*e-b*d)/det\n"
        "  ]);\n"
        "}\n\n"
        "// ── Bootstrap ────────────────────────────────────────────\n"
        "const canvas = document.getElementById('rig-face-canvas');\n"
        "const avatar = new RigFaceAvatar(canvas);\n"
        "// Animador FSM\n"
        "// avatar.animator = new RigFaceAnimatorFSM({blinkRate:%.2f,...});\n"
        "// Game loop\n"
        "let prev = 0;\n"
        "function loop(ts) {\n"
        "  const dt = Math.min((ts-prev)/1000., 0.05);\n"
        "  prev=ts;\n"
        "  avatar.frame(dt);\n"
        "  requestAnimationFrame(loop);\n"
        "}\n"
        "requestAnimationFrame(ts=>{prev=ts;requestAnimationFrame(loop);});\n\n"
        "// ── UI bindings ────────────────────────────────────────────\n"
        "document.getElementById('rig-emotion').addEventListener('change', e=>{\n"
        "  avatar.setEmotion(e.target.value, 0.85);\n"
        "});\n"
        "document.getElementById('rig-age').addEventListener('input', e=>{\n"
        "  avatar.setAge(+e.target.value);\n"
        "});\n"
        "document.getElementById('rig-melanin').addEventListener('input', e=>{\n"
        "  avatar.setMelanin(+e.target.value/100.);\n"
        "});\n"
        "export { RigFaceAvatar };\n",
        ctx->animator.blink_rate);
    out->html      = html;
    out->css       = css;
    out->js        = js;
    out->glsl_frag = frag;
    out->glsl_vert = vert;
    out->ok        = true;
    out->phi_ratio = RIG_PHI;
    out->certeza   = 1.0f;
    return hp + cp + jp + fp + vp;
}
/* ================================================================
 * FUENTE ABSORBIDA: 16_FACE_NG/src/rig_face_v2_bridge.c
 * ================================================================ */
#line 1 "16_FACE_NG/src/rig_face_v2_bridge.c"
/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "rig_v17_preamble.h"
#include "rig_face_v2_bridge.h"
#include "rig_face_codegen.h"
#include "rigart_v4_art.h"
#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_math.h"

#include "rig_syscall.h"
#define BRIDGE_PHI        1.6180339887498948482f
#define BRIDGE_PHI_INV    0.6180339887498948482f
#define BRIDGE_SCHUMANN   7.83f
#define BRIDGE_HTML_CAP   (128 * 1024)
static float  b4_f(const char *j, const char *k, float  d)
{
    if (!j) return d;
    const char *p = strstr(j, k);
    if (!p) return d;
    p = strchr(p, ':');
    return p ? (float)atof(p+1) : d;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: b4_f -> rigpub_rig_face_v2_bridge_b4_f */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
static int    b4_i(const char *j, const char *k, int    d)
{
    if (!j) return d;
    const char *p = strstr(j, k);
    if (!p) return d;
    p = strchr(p, ':');
    return p ? atoi(p+1) : d;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: b4_i -> rigpub_rig_face_v2_bridge_b4_i */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
static int    b4_b(const char *j, const char *k, int    d)
{
    if (!j) return d;
    const char *p = strstr(j, k);
    if (!p) return d;
    p = strchr(p, ':');
    if (!p) return d;
    while (*p==':' || *p==' ') p++;
    if (*p=='t'||*p=='1') return 1;
    if (*p=='f'||*p=='0') return 0;
    return d;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: b4_b -> rigpub_rig_face_v2_bridge_b4_b */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
static int   b4_str(const char *j, const char *k, char *out, size_t sz)
{
    if (!j || !out || !sz) return 0;
    const char *p = strstr(j, k);
    if (!p) return 0;
    p = strchr(p, '"');
    if (!p) return 0;
    p = strchr(p+1, '"');
    if (!p) return 0;
    p = strchr(p+1, '"');
    if (!p) return 0;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < sz-1) out[i++] = *p++;
    out[i] = '\0';
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: b4_str -> rigpub_rig_face_v2_bridge_b4_str */
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */
int rigart_face_session_v2_init(RIgArtFaceSessionV2 *s)
{
    if (!s) return 0;
    memset(s, 0, sizeof(*s));
    const RigFaceArchetype *_def = rig_archetype_get__rig_dup_49442146(RIG_ARCH_AGE_YOUNG_25);
    s->params = _def ? _def->params : (RigFaceParams){0};
    s->subdiv_level  = 4;
    s->archetype_id  = -1;
    rigart_v4_init_result__rig_variant_2c6804ec(&s->last_result);
}
int rigart_face_session_v2_destroy(RIgArtFaceSessionV2 *s)
{
    if (!s) return 0;
    if (s->mesh) { rig_face_v2_destroy(s->mesh); s->mesh = NULL; }
    rigart_v4_free_result__rig_variant_42c478c7(&s->last_result);
}
int rigart_face_v2_dispatch(WsServer *srv, const char *cmd,
                              const char *payload,
                              RIgArtFaceSessionV2 *session)
{
    if (!cmd || !session) return 0;
    char resp[4096];
    if (strcmp(cmd, "rigart_face_build") == 0)
    {
        session->params.melanin          = b4_f(payload, "\"melanin\"",    0.35f);
        session->params.hemoglobin       = b4_f(payload, "\"hemoglobin\"", 0.40f);
        session->params.age_factor       = b4_f(payload, "\"age\"",        0.25f);
        session->params.gender_factor    = b4_f(payload, "\"gender\"",     0.50f);
        session->params.cranium_width    = b4_f(payload, "\"cw\"",  session->params.cranium_width);
        session->params.cranium_height   = b4_f(payload, "\"ch\"",  session->params.cranium_height);
        session->params.cranium_depth    = b4_f(payload, "\"cd\"",  session->params.cranium_depth);
        session->params.jaw_width        = b4_f(payload, "\"jaw_w\"",session->params.jaw_width);
        session->params.jaw_angle        = b4_f(payload, "\"jaw_a\"",session->params.jaw_angle);
        session->params.nose_length      = b4_f(payload, "\"nose_l\"",session->params.nose_length);
        session->params.nose_width       = b4_f(payload, "\"nose_w\"",session->params.nose_width);
        session->params.interocular_dist = b4_f(payload, "\"iod\"",  session->params.interocular_dist);
        session->params.zygomatic_width  = b4_f(payload, "\"zygo_w\"",session->params.zygomatic_width);
        session->params.mouth_width      = b4_f(payload, "\"mouth_w\"",session->params.mouth_width);
        session->params.neck_width       = b4_f(payload, "\"neck_w\"",session->params.neck_width);
        session->params.neck_length      = b4_f(payload, "\"neck_l\"",session->params.neck_length);
        session->params.chin_projection  = b4_f(payload, "\"chin_p\"",session->params.chin_projection);
        uint32_t subdiv = (uint32_t)b4_i(payload, "\"subdiv\"", 4);
        if (subdiv < 2) subdiv = 2;
        if (subdiv > 6) subdiv = 6;
        session->subdiv_level = subdiv;
        bool build_pores    = (bool)b4_b(payload, "\"pores\"",    1);
        bool build_vascular = (bool)b4_b(payload, "\"vascular\"", 1);
        bool build_wrinkles = (bool)b4_b(payload, "\"wrinkles\"", 1);
        float age_for_wrinkles = b4_f(payload, "\"age\"", 0.25f);
        if (session->mesh) { rig_face_v2_destroy(session->mesh); session->mesh = NULL; }
        session->mesh = rig_face_v2_create(&session->params, subdiv);
        session->has_pores = session->has_vascular = session->has_wrinkles = false;
        session->archetype_id = -1;
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_build\","
                "\"error\":\"rig_face_v2_create falló — reducir subdiv\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        if (build_pores) {
            rig_face_build_pore_system(session->mesh, 1.0f);
            session->has_pores = true;
        }
        if (build_vascular) {
            rig_face_build_vascular_tree(session->mesh);
            session->has_vascular = true;
        }
        if (build_wrinkles) {
            rig_face_build_wrinkle_lines(session->mesh, age_for_wrinkles, 0.0f);
            session->has_wrinkles = true;
        }
        snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_build\","
            "\"verts\":%u,\"tris\":%u,\"subdiv\":%u,"
            "\"pores\":%s,\"vascular\":%s,\"wrinkles\":%s,"
            "\"phi_error\":%.6f,\"certeza\":%.6f}",
            session->mesh->base.n_verts, session->mesh->base.n_tris,
            session->mesh->base.subdivision_level,
            session->has_pores    ? "true" : "false",
            session->has_vascular ? "true" : "false",
            session->has_wrinkles ? "true" : "false",
            session->mesh->base.phi_error,
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }
    if (strcmp(cmd, "rigart_face_archetype") == 0)
    {
        int arch_id = b4_i(payload, "\"id\"", 0);
        if (arch_id < 0 || arch_id >= 36) arch_id = 0;
        if (session->mesh) { rig_face_v2_destroy(session->mesh); session->mesh = NULL; }
        session->mesh = rig_face_v2_from_archetype((RigArchetypeID)arch_id);
        session->archetype_id = arch_id;
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_archetype\","
                "\"error\":\"arquetipo %d no encontrado\"}", arch_id);
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        rig_face_build_pore_system(session->mesh, 1.0f);
        rig_face_build_vascular_tree(session->mesh);
        session->has_pores = session->has_vascular = true;
        const RigFaceArchetype *arch = rig_archetype_get__rig_dup_49442146((RigArchetypeID)arch_id);
        snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_archetype\","
            "\"id\":%d,\"name\":\"%s\","
            "\"verts\":%u,\"tris\":%u,\"subdiv\":%u,"
            "\"phi_ref\":%.6f,\"certeza\":%.6f}",
            arch_id,
            arch ? arch->name : "unknown",
            session->mesh->base.n_verts, session->mesh->base.n_tris,
            session->mesh->base.subdivision_level,
            arch ? arch->phi_reference : BRIDGE_PHI,
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }
    if (strcmp(cmd, "rigart_face_age") == 0)
    {
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_age\","
                "\"error\":\"no mesh — build o archetype primero\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        float age = b4_f(payload, "\"age\"", 25.0f);
        if (age < 0.0f)   age = 0.0f;
        if (age > 100.0f) age = 100.0f;
        rig_age_apply_to_mesh(session->mesh, age);
        rig_face_build_wrinkle_lines(session->mesh, age / 100.0f, 0.0f);
        session->has_wrinkles = true;
        snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_age\","
            "\"age\":%.1f,\"verts\":%u,\"certeza\":%.6f}",
            age, session->mesh->base.n_verts, (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }
    if (strcmp(cmd, "rigart_face_expression") == 0)
    {
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_expression\","
                "\"error\":\"no mesh — build primero\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        RigFaceExprCtx ectx;
        memset(&ectx, 0, sizeof(ectx));
        const char *arr = payload ? strstr(payload, "\"weights\"") : NULL;
        if (arr) {
            arr = strchr(arr, '[');
            if (arr) {
                arr++;
                for (int i = 0; i < 52 && i < (int)(sizeof(ectx.weights)/
                                                      sizeof(ectx.weights[0])); i++) {
                    while (*arr==' '||*arr==',') arr++;
                    if (*arr==']'||*arr=='\0') break;
                    ectx.weights[i] = (float)atof(arr);
                    while (*arr && *arr!=',' && *arr!=']') arr++;
                }
            }
        }
        rigart_v4_free_result__rig_variant_42c478c7(&session->last_result);
        int rc = rig_face_v2_expression_blend(&ectx, &session->last_result);
        snprintf(resp, sizeof(resp),
            "{\"ok\":%s,\"cmd\":\"rigart_face_expression\","
            "\"verts\":%u,\"has_glsl\":%s,\"certeza\":%.6f}",
            rc == 0 ? "true" : "false",
            session->mesh->base.n_verts,
            (session->last_result.glsl_vert ? "true" : "false"),
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }
    if (strcmp(cmd, "rigart_face_vbo") == 0)
    {
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_vbo\","
                "\"error\":\"no mesh — build primero\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        uint32_t n_floats = 0, n_indices = 0;
        rig_face_v2_export_vbo(session->mesh, NULL, NULL, &n_floats, &n_indices);
        snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_vbo\","
            "\"verts\":%u,\"tris\":%u,"
            "\"vbo_floats\":%u,\"vbo_bytes\":%u,"
            "\"ibo_indices\":%u,\"ibo_bytes\":%u,"
            "\"stride\":15,\"layout\":\"pos3_nrm3_tan3_uv2_col4\","
            "\"micro_detail\":%s,\"certeza\":%.6f}",
            session->mesh->base.n_verts, session->mesh->base.n_tris,
            n_floats,  n_floats  * 4,
            n_indices, n_indices * 4,
            (session->has_pores || session->has_vascular) ? "true" : "false",
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }
    if (strcmp(cmd, "rigart_face_glsl") == 0)
    {
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_glsl\","
                "\"error\":\"no mesh — build primero\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        char glsl_buf[8192];
        int rc = rig_codegen_glsl_skin_shader__rig_dup_1c81da77(session->mesh, glsl_buf, sizeof(glsl_buf));
        snprintf(resp, sizeof(resp),
            "{\"ok\":%s,\"cmd\":\"rigart_face_glsl\","
            "\"melanin\":%.4f,\"roughness\":%.4f,"
            "\"sss_r\":%.4f,\"sss_g\":%.4f,\"sss_b\":%.4f,"
            "\"has_pores\":%s,\"has_vascular\":%s,"
            "\"certeza\":%.6f}",
            rc >= 0 ? "true" : "false",
            session->mesh->base.material.melanin_concentration,
            session->mesh->base.material.roughness,
            session->mesh->base.material.sss_radius[0],
            session->mesh->base.material.sss_radius[1],
            session->mesh->base.material.sss_radius[2],
            session->has_pores    ? "true" : "false",
            session->has_vascular ? "true" : "false",
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        if (rc >= 0) ws_broadcastf(srv, "%s", glsl_buf);
        return 0;
    }
    if (strcmp(cmd, "rigart_face_codegen") == 0)
    {
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_codegen\","
                "\"error\":\"no mesh — build o archetype primero\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        char target_str[32] = "c11";
        b4_str(payload, "\"target\"", target_str, sizeof(target_str));
        RigCodegenOptions opt;
        memset(&opt, 0, sizeof(opt));
        opt.include_comments  = true;
        opt.arm64_intrinsics  = true;
        opt.include_mesh_data = true;
        if      (strcmp(target_str, "rigscript") == 0) opt.target = RIG_CODEGEN_RIGSCRIPT;
        else if (strcmp(target_str, "json")       == 0) opt.target = RIG_CODEGEN_JSON;
        else if (strcmp(target_str, "glsl")       == 0) opt.target = RIG_CODEGEN_GLSL;
        else if (strcmp(target_str, "makefile")   == 0) opt.target = RIG_CODEGEN_MAKEFILE;
        else if (strcmp(target_str, "header")     == 0) opt.target = RIG_CODEGEN_HEADER;
        else if (strcmp(target_str, "markdown")   == 0) opt.target = RIG_CODEGEN_MARKDOWN;
        else                                            opt.target = RIG_CODEGEN_C;
        char *buf = malloc(32768);
        if (!buf) {
            ws_broadcastf(srv,
                "{\"ok\":false,\"cmd\":\"rigart_face_codegen\","
                "\"error\":\"OOM\"}");
            return 0;
        }
        int rc = rig_codegen_mesh_v2__rig_dup_9a0ca9d5(session->mesh, &opt, buf, 32768);
        snprintf(resp, sizeof(resp),
            "{\"ok\":%s,\"cmd\":\"rigart_face_codegen\","
            "\"target\":\"%s\",\"bytes\":%d,\"certeza\":%.6f}",
            rc >= 0 ? "true" : "false",
            target_str, rc,
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        if (rc >= 0) ws_broadcastf(srv, "%s", buf);
        free(buf);
        return 0;
    }
    snprintf(resp, sizeof(resp),
        "{\"ok\":false,\"cmd\":\"%s\",\"error\":\"comando face desconocido\"}",
        cmd);
    ws_broadcastf(srv, "%s", resp);
}
int rigart_renderer_html_v4(const RIgArtRendererCtxV4 *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);
    RigArtCompositorCtx comp;
    memset(&comp, 0, sizeof(comp));
    comp.exposure         = ctx->exposure > 0.0f ? ctx->exposure : 1.0f;
    comp.gamma            = 2.2f;
    comp.contrast         = 1.0f;
    comp.saturation       = 1.0f;
    comp.enable_bloom     = ctx->enable_bloom;
    comp.bloom_strength   = ctx->bloom_strength > 0.0f ? ctx->bloom_strength : 0.618f;
    comp.enable_dof       = ctx->enable_dof;
    comp.tonemapping_aces = true;
    comp.enable_vignette  = ctx->show_stats;
    RigArtResultV4 comp_res;
    rigart_v4_init_result__rig_variant_2c6804ec(&comp_res);
    rigart_art_compositor__rig_variant_66d1e991(&comp, &comp_res);
    RigArtCanvasCtx canvas;
    memset(&canvas, 0, sizeof(canvas));
    canvas.width           = 1280;
    canvas.height          = 720;
    canvas.layer_count     = 1;
    canvas.hdr_p3_enabled  = ctx->hdr_p3;
    canvas.enable_16bit    = false;
    canvas.dpi             = 96.0f;
    canvas.grid_size_phi   = BRIDGE_PHI;
    snprintf(canvas.layers[0].name, 64, "RigArt v4 — %s",
             ctx->title[0] ? ctx->title : "Escena Soberana");
    canvas.layers[0].blend_mode = BLEND_NORMAL;
    canvas.layers[0].opacity    = 1.0f;
    canvas.layers[0].visible    = true;
    RigArtResultV4 canvas_res;
    rigart_v4_init_result__rig_variant_2c6804ec(&canvas_res);
    rigart_art_canvas_gen__rig_variant_e9720543(&canvas, &canvas_res);
    char *html = malloc(BRIDGE_HTML_CAP);
    if (!html) {
        rigart_v4_free_result__rig_variant_42c478c7(&comp_res);
        rigart_v4_free_result__rig_variant_42c478c7(&canvas_res);
        snprintf(out->error, 255, "OOM renderer html v4");
        return -1;
    }
    const char *title  = ctx->title[0]    ? ctx->title    : "RIgArt · Escena Soberana";
    const char *bg     = ctx->bg_color[0] ? ctx->bg_color : "#030208";
    float lx = ctx->light_dir[0], ly = ctx->light_dir[1], lz = ctx->light_dir[2];
    if (lx==0.0f && ly==0.0f && lz==0.0f) { lx=4.0f; ly=8.0f; lz=5.0f; }
    int pos = snprintf(html, BRIDGE_HTML_CAP,
"<!DOCTYPE html>\n<html lang=\"es\">\n<head>\n"
"<meta charset=\"UTF-8\">\n"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
"<title>%s</title>\n"
"<style>\n"
"  *{margin:0;padding:0;box-sizing:border-box}\n"
"  body{background:%s;overflow:hidden;font-family:'JetBrains Mono','Courier New','Courier',monospace}\n"
"  canvas{display:block;width:100vw;height:100vh}\n"
"  #overlay{position:fixed;top:1rem;left:1rem;color:#d4aa3c;"
"font-size:.72rem;opacity:.8;pointer-events:none;line-height:1.6}\n"
"</style>\n</head>\n<body>\n"
"<canvas id=\"c\"></canvas>\n"
"%s\n"
"<script>\n"
"/* RIgArt v4.0 · φ=%.16f · Schumann=%.2f Hz */\n"
"const PHI=%.16f,PHI_INV=%.16f,SCHUMANN=%.2f;\n"
"const LIGHT_DIR=new Float32Array([%.4f,%.4f,%.4f]);\n"
"const LIGHT_COL=new Float32Array([%.4f,%.4f,%.4f]);\n"
"const ROUGHNESS=%.4f,METALLIC=%.4f,TEX_SCALE=%.4f;\n\n"
"/* ── Canvas v4 ─────────────────────────────── */\n"
"%s\n\n"
"/* ── WebGL2 PBR setup ─────────────────────── */\n"
"(function(){\n"
"  const canvas=document.getElementById('c');\n"
"  const W=window.innerWidth,H=window.innerHeight;\n"
"  canvas.width=W; canvas.height=H;\n"
"  const gl=canvas.getContext('webgl2',{\n"
"    antialias:true,depth:true,\n"
"    colorSpace:'%s'\n"
"  });\n"
"  if(!gl){document.body.innerHTML='<p style=\"color:#d44\">WebGL2 requerido</p>';return 0;}\n\n"
"  const VS=`#version 300 es\n"
"  in vec4 aPos; in vec3 aNorm; in vec2 aUV;\n"
"  uniform mat4 uMVP; uniform float uTime;\n"
"  out vec3 vNorm; out vec2 vUV; out vec3 vPos;\n"
"  void main(){\n"
"    float phi=%.8f;\n"
"    vec4 p=aPos; p.y+=sin(p.x*phi+uTime*%.2f)*0.003;\n"
"    gl_Position=uMVP*p;\n"
"    vNorm=aNorm; vUV=aUV; vPos=p.xyz;\n"
"  }`;\n\n"
"  const FS=`#version 300 es\n"
"  precision highp float;\n"
"  in vec3 vNorm; in vec2 vUV; in vec3 vPos;\n"
"  uniform vec3 uLightDir,uLightCol;\n"
"  uniform float uRoughness,uMetallic,uTime;\n"
"  out vec4 fragColor;\n"
"  const float PI=3.14159265359;\n"
"  const float PHI=%.8f;\n"
"  /* Cook-Torrance GGX */\n"
"  float D_GGX(float NdH,float a){float a2=a*a;float d=NdH*NdH*(a2-1.)+1.;\n"
"    return a2/(PI*d*d+1e-7);}\n"
"  float G_Schlick(float NdV,float k){return NdV/(NdV*(1.-k)+k+1e-7);}\n"
"  float G_Smith(float NdV,float NdL,float a){float k=(a+1.)*(a+1.)/8.;\n"
"    return G_Schlick(NdV,k)*G_Schlick(NdL,k);}\n"
"  vec3 F_Schlick(float VdH,vec3 F0){return F0+(1.-F0)*pow(1.-VdH,5.);}\n"
"  /* SSS skin — Jimenez mod */\n"
"  vec3 skin_sss(vec3 pos,vec3 N,vec3 L){\n"
"    float wrap=0.32; float d=max(dot(N,L)+wrap,0.)/(1.+wrap);\n"
"    vec3 sc=vec3(%.4f,%.4f,%.4f);\n"
"    return sc*d*%.4f;}\n"
"  /* Schumann pulse */\n"
"  float schumann(float t){return sin(t*%.4f*2.*PI)*0.5+0.5;}\n"
"  void main(){\n"
"    vec3 N=normalize(vNorm);\n"
"    vec3 L=normalize(uLightDir);\n"
"    vec3 V=normalize(vec3(0.,0.,1.)-vPos);\n"
"    vec3 H=normalize(L+V);\n"
"    float NdL=max(dot(N,L),0.),NdV=max(dot(N,V),1e-4);\n"
"    float NdH=max(dot(N,H),0.),VdH=max(dot(V,H),0.);\n"
"    vec3 alb=vec3(%.4f,%.4f,%.4f);\n"
"    vec3 F0=mix(vec3(0.04),alb,uMetallic);\n"
"    float a=uRoughness*uRoughness;\n"
"    float D=D_GGX(NdH,a);\n"
"    float G=G_Smith(NdV,NdL,a);\n"
"    vec3 F=F_Schlick(VdH,F0);\n"
"    vec3 spec=D*G*F/(4.*NdV*NdL+1e-7);\n"
"    vec3 diff=(1.-F)*(1.-uMetallic)*alb/PI;\n"
"    vec3 sss=skin_sss(vPos,N,L);\n"
"    vec3 col=(diff+spec)*uLightCol*NdL+sss;\n"
"    /* ACES tonemap */\n"
"    col*=0.6;float aa=2.51,bb=0.03,cc=2.43,dd=0.59,ee=0.14;\n"
"    col=clamp((col*(aa*col+bb))/(col*(cc*col+dd)+ee),0.,1.);\n"
"    /* φ-pulse ambient */\n"
"    col+=vec3(0.02)*schumann(uTime);\n"
"    fragColor=vec4(pow(col,vec3(1./2.2)),1.);\n"
"  }`;\n\n"
"  function compileShader(type,src){\n"
"    const s=gl.createShader(type);gl.shaderSource(s,src);gl.compileShader(s);\n"
"    if(!gl.getShaderParameter(s,gl.COMPILE_STATUS)){console.error(gl.getShaderInfoLog(s));return null;}\n"
"    return s;}\n"
"  const vs=compileShader(gl.VERTEX_SHADER,VS);\n"
"  const fs=compileShader(gl.FRAGMENT_SHADER,FS);\n"
"  const prog=gl.createProgram();\n"
"  gl.attachShader(prog,vs);gl.attachShader(prog,fs);gl.linkProgram(prog);\n"
"  if(!gl.getProgramParameter(prog,gl.LINK_STATUS)){console.error(gl.getProgramInfoLog(prog));return 0;}\n\n"
"  /* Icosfera φ-paramétrica procedural */\n"
"  const t=(1.+Math.sqrt(5.))/2.;\n"
"  const verts=[[-1,t,0],[1,t,0],[-1,-t,0],[1,-t,0],\n"
"    [0,-1,t],[0,1,t],[0,-1,-t],[0,1,-t],\n"
"    [t,0,-1],[t,0,1],[-t,0,-1],[-t,0,1]];\n"
"  const idx=[0,11,5,0,5,1,0,1,7,0,7,10,0,10,11,1,5,9,5,11,4,11,10,2,10,7,6,7,1,8,\n"
"    3,9,4,3,4,2,3,2,6,3,6,8,3,8,9,4,9,5,2,4,11,6,2,10,8,6,7,9,8,1];\n"
"  const pv=[],pn=[],pu=[];\n"
"  for(let i=0;i<idx.length;i+=3){\n"
"    for(let j=0;j<3;j++){\n"
"      const v=verts[idx[i+j]];\n"
"      const len=Math.hypot(...v);\n"
"      const n=v.map(x=>x/len);\n"
"      pv.push(...n); pn.push(...n);\n"
"      pu.push(Math.atan2(n[0],n[2])/(2*Math.PI)+.5,Math.acos(n[1])/Math.PI);\n"
"    }\n"
"  }\n"
"  function mkBuf(data,target){\n"
"    const b=gl.createBuffer();gl.bindBuffer(target,b);\n"
"    gl.bufferData(target,new Float32Array(data),gl.STATIC_DRAW);return b;}\n"
"  const vbo=mkBuf(pv,gl.ARRAY_BUFFER);\n"
"  const nbo=mkBuf(pn,gl.ARRAY_BUFFER);\n"
"  const ubo=mkBuf(pu,gl.ARRAY_BUFFER);\n\n"
"  function attr(buf,loc,n){\n"
"    gl.bindBuffer(gl.ARRAY_BUFFER,buf);\n"
"    gl.enableVertexAttribArray(loc);\n"
"    gl.vertexAttribPointer(loc,n,gl.FLOAT,false,0,0);}\n"
"  const aPos=gl.getAttribLocation(prog,'aPos');\n"
"  const aNorm=gl.getAttribLocation(prog,'aNorm');\n"
"  const aUV=gl.getAttribLocation(prog,'aUV');\n"
"  const uMVP=gl.getUniformLocation(prog,'uMVP');\n"
"  const uTime=gl.getUniformLocation(prog,'uTime');\n"
"  const uLD=gl.getUniformLocation(prog,'uLightDir');\n"
"  const uLC=gl.getUniformLocation(prog,'uLightCol');\n"
"  const uRough=gl.getUniformLocation(prog,'uRoughness');\n"
"  const uMetal=gl.getUniformLocation(prog,'uMetallic');\n\n"
"  let angle=0,t0=performance.now();\n"
"  const PHIf=%.8f;\n"
"  function frame(ts){\n"
"    const dt=(ts-t0)/1000;t0=ts;\n"
"    if(%s)angle+=dt*%.4f;\n"
"    gl.viewport(0,0,W,H);\n"
"    gl.clearColor(0,0,0,1);gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);\n"
"    gl.enable(gl.DEPTH_TEST);\n"
"    gl.useProgram(prog);\n"
"    attr(vbo,aPos,3);attr(nbo,aNorm,3);attr(ubo,aUV,2);\n"
"    /* MVP: perspectiva + rotación */\n"
"    const asp=W/H,fov=Math.PI/3.5,near=0.1,far=100;\n"
"    const f=1/Math.tan(fov/2);\n"
"    const proj=[f/asp,0,0,0, 0,f,0,0,\n"
"      0,0,(far+near)/(near-far),-1, 0,0,2*far*near/(near-far),0];\n"
"    const c=Math.cos(angle),s=Math.sin(angle);\n"
"    const view=[c,0,s,0, 0,1,0,0, -s,0,c,0, 0,0,-3,1];\n"
"    const mvp=new Float32Array(16);\n"
"    for(let i=0;i<4;i++)for(let j=0;j<4;j++){\n"
"      let v=0;for(let k=0;k<4;k++)v+=proj[i*4+k]*view[k*4+j];\n"
"      mvp[i*4+j]=v;}\n"
"    gl.uniformMatrix4fv(uMVP,false,mvp);\n"
"    gl.uniform1f(uTime,ts/1000);\n"
"    gl.uniform3fv(uLD,LIGHT_DIR);\n"
"    gl.uniform3fv(uLC,LIGHT_COL);\n"
"    gl.uniform1f(uRough,ROUGHNESS);\n"
"    gl.uniform1f(uMetal,METALLIC);\n"
"    gl.drawArrays(gl.TRIANGLES,0,pv.length/3);\n"
"    requestAnimationFrame(frame);}\n"
"  requestAnimationFrame(frame);\n"
"  window.addEventListener('resize',()=>{canvas.width=window.innerWidth;canvas.height=window.innerHeight;});\n"
"})();\n"
"</script>\n</body>\n</html>\n",
        title, bg,
        ctx->show_stats
            ? "<div id=\"overlay\">"
              "RIgArt v4.0 · φ=1.618 · Schumann 7.83 Hz<br>"
              "WebGL2 · PBR · SSS Skin · ACES</div>"
            : "",
        BRIDGE_PHI, BRIDGE_SCHUMANN,
        BRIDGE_PHI, BRIDGE_PHI_INV, BRIDGE_SCHUMANN,
        lx, ly, lz,
        ctx->light_color[0]>0?ctx->light_color[0]:1.0f,
        ctx->light_color[1]>0?ctx->light_color[1]:0.98f,
        ctx->light_color[2]>0?ctx->light_color[2]:0.94f,
        ctx->roughness>0?ctx->roughness:0.35f,
        ctx->metallic>0?ctx->metallic:0.90f,
        ctx->tex_scale>0?ctx->tex_scale:1.0f,
        canvas_res.js ? canvas_res.js : "/* canvas_gen no disponible */",
        ctx->hdr_p3 ? "display-p3" : "srgb",
        BRIDGE_PHI, BRIDGE_SCHUMANN,
        BRIDGE_PHI,
        0.5882f, 0.3647f, 0.3020f,
        0.45f,
        BRIDGE_SCHUMANN,
        0.8157f, 0.6431f, 0.5725f,
        BRIDGE_PHI,
        ctx->auto_rotate ? "true" : "false",
        BRIDGE_PHI_INV
    );
    if (pos <= 0 || pos >= BRIDGE_HTML_CAP) {
        free(html);
        rigart_v4_free_result__rig_variant_42c478c7(&comp_res);
        rigart_v4_free_result__rig_variant_42c478c7(&canvas_res);
        snprintf(out->error, 255, "renderer_html_v4: buffer overflow");
        return -1;
    }
    out->html    = html;
    out->ok      = true;
    out->certeza = BRIDGE_PHI_INV;
    out->phi_ratio = BRIDGE_PHI;
    rigart_v4_free_result__rig_variant_42c478c7(&comp_res);
    rigart_v4_free_result__rig_variant_42c478c7(&canvas_res);
    return 0;
}