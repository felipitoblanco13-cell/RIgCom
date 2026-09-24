/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "rig_v17_preamble.h"
#include "rig_face_engine_v2.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_math.h"
#include "../include/riglib_math.h"
#include "rig_noext_io.h"
#include "rig_syscall.h"
#define V2_PHI          1.6180339887498948482f
#define V2_PHI_INV      0.6180339887498948482f
#define V2_SCHUMANN     7.83f
#define V2_PI           3.14159265358979323846f
#define FLOATS_PER_VERT 15
extern RigFaceMesh* rig_face_create(const RigFaceParams *params,
                                     uint32_t subdiv_level);
extern int          rig_face_destroy(RigFaceMesh *mesh);
extern int          rig_face_export_vbo(const RigFaceMesh *mesh,
                                         float *vbo, uint32_t *ibo,
                                         uint32_t *n_floats,
                                         uint32_t *n_indices);
static inline float v2_noise(uint32_t seed, float x, float y)
{
    uint32_t h = seed;
    h ^= (uint32_t)(x * 73856093.0f);
    h ^= (uint32_t)(y * 19349663.0f);
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
    RigFaceMesh *base = rig_face_create(params, subdiv);
    if (!base) return NULL;
    RigFaceMeshV2 *m = (RigFaceMeshV2*)calloc(1, sizeof(RigFaceMeshV2));
    if (!m) { rig_face_destroy(base); return NULL; }
    memcpy(&m->base, base, sizeof(RigFaceMesh));
    /* base y sus buffers pertenecen al arena del motor; V2 conserva esa propiedad. */
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
    if (!m) return -1;
    free(m->pore_normal_map);
    free(m->vascular_map);
    free(m->wrinkle_normal_map);
    free(m->thermal_map);
    free(m->ear_left_verts);
    free(m->ear_left_tris);
    free(m->ear_right_verts);
    free(m->ear_right_tris);
    /* verts/tris provienen del arena global; no son bloques malloc independientes. */
    rig_face_destroy(&m->base);
    memset(m, 0, sizeof(*m));
    free(m);
    return 0;
}
int rig_face_build_pore_system(RigFaceMeshV2 *m, float density_scale)
{
    if (!m || density_scale <= 0.0f) return -1;
    uint32_t w = 512, h = 512;
    m->texture_width  = w;
    m->texture_height = h;
    free(m->pore_normal_map);
    m->pore_normal_map = (uint8_t*)calloc(w * h * 4, 1);
    if (!m->pore_normal_map) return -1;
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
    return 0;
}
int rig_face_build_vascular_tree(RigFaceMeshV2 *m)
{
    if (!m) return -1;
    uint32_t w = 512, h = 512;
    m->texture_width  = m->texture_width  ? m->texture_width  : w;
    m->texture_height = m->texture_height ? m->texture_height : h;
    w = m->texture_width; h = m->texture_height;
    free(m->vascular_map);
    m->vascular_map = (uint8_t*)calloc(w * h * 4, 1);
    if (!m->vascular_map) return -1;
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
    return 0;
}
int rig_face_build_wrinkle_lines(RigFaceMeshV2 *m, float age,
                                   float expression)
{
    if (!m) return -1;
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
    if (!m->wrinkle_normal_map) return -1;
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
    return 0;
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
    if (!m) return -1;
    if (age_years <  0.0f) age_years =  0.0f;
    if (age_years > 100.0f) age_years = 100.0f;
    const RigFaceArchetype *young = rig_archetype_get__rig_dup_49442146(RIG_ARCH_AGE_YOUNG_25);
    const RigFaceArchetype *elder = rig_archetype_get__rig_dup_49442146(RIG_ARCH_AGE_ELDER_70);
    if (!young || !elder) return -1;
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
    return 0;
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
    if (!m) return -1;
    RigFaceIrisDetail *ir = right ? &m->iris_right : &m->iris_left;
    ir->pupil_radius = fmaxf(0.20f, fminf(0.60f, ir->pupil_radius));
    (void)right;
    return 0;
}
int rig_face_bake_iris_texture(const RigFaceIrisDetail *iris,
                                 uint8_t *out_rgba, uint32_t w, uint32_t h)
{
    if (!iris || !out_rgba || w == 0 || h == 0) return -1;
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
    return 0;
}
int rig_iris_set_pupil_dilation(RigFaceIrisDetail *iris, float lux)
{
    if (!iris) return -1;
    float dil = 0.60f - lux * 0.35f;
    iris->pupil_radius = fmaxf(1.0f, fminf(5.5f, dil * 6.5f));
    return 0;
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
    if (!m) return -1;
    RigFaceEarParams *ep = right ? &m->ear_right_params : &m->ear_left_params;
    uint32_t n_verts = 128, n_tris = 220;
    RigFaceVertex **pverts = right ? &m->ear_right_verts : &m->ear_left_verts;
    RigFaceTri    **ptris  = right ? &m->ear_right_tris  : &m->ear_left_tris;
    uint32_t *pnv = right ? &m->ear_right_n_verts : &m->ear_left_n_verts;
    uint32_t *pnt = right ? &m->ear_right_n_tris  : &m->ear_left_n_tris;
    free(*pverts); free(*ptris);
    *pverts = (RigFaceVertex*)calloc(n_verts, sizeof(RigFaceVertex));
    *ptris  = (RigFaceTri*)   calloc(n_tris,  sizeof(RigFaceTri));
    if (!*pverts || !*ptris) { free(*pverts); free(*ptris); *pverts = NULL; *ptris = NULL; return -1; }
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
    return 0;
}
int rig_face_attach_ears(RigFaceMeshV2 *m)
{
    if (!m) return -1;
    if (!m->ear_left_verts)  rig_face_build_ear_geometry(m, false);
    if (!m->ear_right_verts && rig_face_build_ear_geometry(m, true) != 0) return -1;
    return 0;
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
    if (!m || !m->base.verts) return -1;
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
    return 0;
}
int rig_face_bake_lip_color_map(const RigFaceMeshV2 *m,
                                  uint8_t *out_rgba, uint32_t w, uint32_t h)
{
    if (!m || !out_rgba || w == 0 || h == 0) return -1;
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
    return 0;
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
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
int rig_face_bake_vascular_map(RigFaceMeshV2 *m, uint32_t w, uint32_t h)
{
    m->texture_width = w; m->texture_height = h;
    rig_face_build_vascular_tree(m);
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
int rig_face_bake_wrinkle_normal_map(RigFaceMeshV2 *m, uint32_t w, uint32_t h)
{
    m->texture_width = w; m->texture_height = h;
    rig_face_build_wrinkle_lines(m, m->base.params.age_factor, 0.0f);
    return 0; /* 0=hook procesado, -1=contexto inválido */
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
                                   const RigHairRegionParams *hair){
    if (!mesh || !hair) return -1;
    /* Acceso a la malla base */
    RigFaceMesh *base = &mesh->base;
    if (!base->verts || base->n_verts == 0) return -1;
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
    return 0;
}