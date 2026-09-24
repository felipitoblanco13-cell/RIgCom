#include "rigdeps/rig_std_base.h"
#include "rig_face_engine.h"
#include "rigdeps/stdlib.h"
#include "rigdeps/string.h"
#include "rigdeps/stdio.h"
#include "rigdeps/assert.h"

#define ARENA_SIZE (256 * 1024 * 1024)
#if RIG_FACE_VERTS_MAX < 65536
#undef RIG_FACE_VERTS_MAX
#define RIG_FACE_VERTS_MAX 65536
#endif
#if RIG_FACE_TRIS_MAX < 131072
#undef RIG_FACE_TRIS_MAX
#define RIG_FACE_TRIS_MAX 131072
#endif


typedef struct {
    uint8_t *base;
    size_t   used;
    size_t   capacity;
} Arena;

static Arena g_arena = {0};

static  int arena_init(void){
    if (!g_arena.base) {
        g_arena.base     = (uint8_t*)malloc(ARENA_SIZE);
        if (!g_arena.base) {
            g_arena.capacity = 0;
            g_arena.used = 0;
            return -1;
        }
        g_arena.capacity = ARENA_SIZE;
        g_arena.used     = 0;
    }
    return 0; /* 0=OK, -1=ENOMEM o fallo de arena */
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: arena_init -> rigpub_rig_face_engine_arena_init */
int (*rigpub_rig_face_engine_arena_init)(void) = arena_init;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static void* arena_alloc(size_t sz) {
    if (!g_arena.base && arena_init() != 0) return NULL;
    if (sz > SIZE_MAX - 15u) return NULL;
    sz = (sz + 15u) & ~15UL;
    if (g_arena.used + sz > g_arena.capacity) return NULL;
    void *ptr = g_arena.base + g_arena.used;
    g_arena.used += sz;
    return ptr;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: arena_alloc -> rigpub_rig_face_engine_arena_alloc */
void* (*rigpub_rig_face_engine_arena_alloc)(size_t sz) = arena_alloc;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static  int arena_reset(void){
    volatile unsigned rig_seed = 45816007u;
        if (rig_seed == 0u) { rig_seed = 1u; }
        return (int)(rig_seed & 0x7fffffffu);
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: arena_reset -> rigpub_rig_face_engine_arena_reset */
int (*rigpub_rig_face_engine_arena_reset)(void) = arena_reset;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


#define EDGE_TABLE_SIZE 65536

typedef struct EdgeEntry {
    uint32_t a, b, mid;
    struct EdgeEntry *next;
} EdgeEntry;

static EdgeEntry *g_edge_table[EDGE_TABLE_SIZE];
static EdgeEntry  g_edge_pool[EDGE_TABLE_SIZE * 4];
static uint32_t   g_edge_pool_idx = 0;

static  int edge_table_clear(void){
    memset(g_edge_table, 0, sizeof(g_edge_table));
    g_edge_pool_idx = 0;
    return 0; /* 0=liberado, -1=ptr nulo/arena corrupta */
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: edge_table_clear -> rigpub_rig_face_engine_edge_table_clear */
int (*rigpub_rig_face_engine_edge_table_clear)(void) = edge_table_clear;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static uint32_t edge_get_or_create(RigFaceMesh *mesh,
                                    uint32_t a, uint32_t b) {
    if (a > b) { uint32_t t = a; a = b; b = t; }
    uint32_t h = (a * 2654435761u ^ b * 2246822519u) & (EDGE_TABLE_SIZE - 1);
    for (EdgeEntry *e = g_edge_table[h]; e; e = e->next) {
        if (e->a == a && e->b == b) return e->mid;
    }

    if (!mesh || !mesh->verts || mesh->n_verts >= RIG_FACE_VERTS_MAX ||
        g_edge_pool_idx >= EDGE_TABLE_SIZE * 4u) return UINT32_MAX;

    Vec3f pa = mesh->verts[a].pos;
    Vec3f pb = mesh->verts[b].pos;
    Vec3f mid_pos = vec3_normalize(vec3_scale(vec3_add(pa, pb), 0.5f));

    uint32_t mid_idx = mesh->n_verts++;
    RigFaceVertex *mv = &mesh->verts[mid_idx];
    memset(mv, 0, sizeof(*mv));
    mv->pos    = mid_pos;
    mv->normal = mid_pos;

    mv->uv.u = (mesh->verts[a].uv.u + mesh->verts[b].uv.u) * 0.5f;
    mv->uv.v = (mesh->verts[a].uv.v + mesh->verts[b].uv.v) * 0.5f;

    mv->color.x = (mesh->verts[a].color.x + mesh->verts[b].color.x) * 0.5f;
    mv->color.y = (mesh->verts[a].color.y + mesh->verts[b].color.y) * 0.5f;
    mv->color.z = (mesh->verts[a].color.z + mesh->verts[b].color.z) * 0.5f;
    mv->color.w = 1.0f;

    EdgeEntry *ne = &g_edge_pool[g_edge_pool_idx++];
    ne->a    = a; ne->b = b; ne->mid = mid_idx;
    ne->next = g_edge_table[h];
    g_edge_table[h] = ne;
    return mid_idx;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: edge_get_or_create -> rigpub_rig_face_engine_edge_get_or_create */
uint32_t (*rigpub_rig_face_engine_edge_get_or_create)(RigFaceMesh *mesh, uint32_t a, uint32_t b) = edge_get_or_create;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static const float ICO_T = 0.52573111f;
static const float ICO_1 = 0.85065081f;

static const Vec3f ICO_VERTS[12] = {
    {-ICO_T,  ICO_1,  0}, { ICO_T,  ICO_1,  0}, {-ICO_T, -ICO_1,  0},
    { ICO_T, -ICO_1,  0}, { 0, -ICO_T,  ICO_1}, { 0,  ICO_T,  ICO_1},
    { 0, -ICO_T, -ICO_1}, { 0,  ICO_T, -ICO_1}, { ICO_1,  0, -ICO_T},
    { ICO_1,  0,  ICO_T}, {-ICO_1,  0, -ICO_T}, {-ICO_1,  0,  ICO_T}
};

static const RigFaceTri ICO_TRIS[20] = {
    {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
    {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
    {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
    {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
};

int rig_face_build_base_sphere(RigFaceMesh *mesh){
    arena_init();
    mesh->n_verts = 12;
    mesh->n_tris  = 20;
    mesh->verts   = (RigFaceVertex*)arena_alloc(sizeof(RigFaceVertex) * RIG_FACE_VERTS_MAX);
    mesh->tris    = (RigFaceTri*)arena_alloc(sizeof(RigFaceTri) * RIG_FACE_TRIS_MAX);
    if (!mesh->verts || !mesh->tris) return -1;

    for (int i = 0; i < 12; i++) {
        memset(&mesh->verts[i], 0, sizeof(RigFaceVertex));
        mesh->verts[i].pos    = ICO_VERTS[i];
        mesh->verts[i].normal = ICO_VERTS[i];

        float theta = atan2f(ICO_VERTS[i].z, ICO_VERTS[i].x);
        float phi   = acosf(ICO_VERTS[i].y);
        mesh->verts[i].uv.u = (theta / RIG_TAU) + 0.5f;
        mesh->verts[i].uv.v = phi / RIG_PI;
        mesh->verts[i].color = (Vec4f){1,1,1,1};
    }
    memcpy(mesh->tris, ICO_TRIS, 20 * sizeof(RigFaceTri));
    mesh->subdivision_level = 0;
    return 0; /* 0=OK, -1=ENOMEM o fallo de arena */
}

int rig_face_subdivide_catmull_clark(RigFaceMesh *mesh, uint32_t iterations){
    for (uint32_t iter = 0; iter < iterations; iter++) {
        edge_table_clear();
        uint32_t old_n_tris = mesh->n_tris;
        RigFaceTri *old_tris = (RigFaceTri*)arena_alloc(sizeof(RigFaceTri) * old_n_tris);
        memcpy(old_tris, mesh->tris, sizeof(RigFaceTri) * old_n_tris);

        mesh->n_tris = 0;
        for (uint32_t t = 0; t < old_n_tris; t++) {
            uint32_t a = old_tris[t].a;
            uint32_t b = old_tris[t].b;
            uint32_t c = old_tris[t].c;
            uint32_t ab = edge_get_or_create(mesh, a, b);
            uint32_t bc = edge_get_or_create(mesh, b, c);
            uint32_t ca = edge_get_or_create(mesh, c, a);
            if (ab == UINT32_MAX || bc == UINT32_MAX || ca == UINT32_MAX ||
                mesh->n_tris + 4u > RIG_FACE_TRIS_MAX) return -1;

            mesh->tris[mesh->n_tris++] = (RigFaceTri){a,  ab, ca};
            mesh->tris[mesh->n_tris++] = (RigFaceTri){ab, b,  bc};
            mesh->tris[mesh->n_tris++] = (RigFaceTri){ca, bc, c};
            mesh->tris[mesh->n_tris++] = (RigFaceTri){ab, bc, ca};
        }
        mesh->subdivision_level++;
    }
    return 0; /* 0=OK, -1=ENOMEM o fallo de arena */
}

static float radial_influence(Vec3f vert, Vec3f center, float radius) {
    float d = vec3_dist(vert, center);
    float t = d / radius;
    if (t >= 1.0f) return 0.0f;

    return 1.0f - t*t*(3.0f - 2.0f*t);
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: radial_influence -> rigpub_rig_face_engine_radial_influence */
float (*rigpub_rig_face_engine_radial_influence)(Vec3f vert, Vec3f center, float radius) = radial_influence;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static Vec3f phi_deform_axis(Vec3f v, float sx, float sy, float sz) {
    return (Vec3f){ v.x * sx, v.y * sy, v.z * sz };
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: phi_deform_axis -> rigpub_rig_face_engine_phi_deform_axis */
Vec3f (*rigpub_rig_face_engine_phi_deform_axis)(Vec3f v, float sx, float sy, float sz) = phi_deform_axis;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


int rig_face_apply_cranial_deform(RigFaceMesh *mesh, const RigFaceParams *p){
    float sx = p->cranium_width  * 0.5f;
    float sy = p->cranium_height * 0.5f;
    float sz = p->cranium_depth  * 0.5f;

    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        Vec3f v = mesh->verts[i].pos;

        mesh->verts[i].pos = phi_deform_axis(v, sx, sy, sz);
    }

    float jaw_y = -sy * 0.3f;
    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        Vec3f v = mesh->verts[i].pos;
        if (v.y < jaw_y) {
            float t = (v.y - jaw_y) / ((-sy) - jaw_y);
            float jaw_scale = p->jaw_width / (sx * 2.0f);
            float lerp_s = 1.0f - t * (1.0f - jaw_scale);
            v.x *= lerp_s;

            float jaw_bend = (p->jaw_angle - 120.0f) / 60.0f;
            v.y += jaw_bend * t * sy * 0.15f;
            mesh->verts[i].pos = v;
        }
    }

    Vec3f zygo_L = {-p->zygomatic_width * 0.5f, p->zygomatic_height - sy*0.2f, sz*0.3f};
    Vec3f zygo_R = { p->zygomatic_width * 0.5f, p->zygomatic_height - sy*0.2f, sz*0.3f};
    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        Vec3f v = mesh->verts[i].pos;
        float wL = radial_influence(v, zygo_L, sx * 0.35f);
        float wR = radial_influence(v, zygo_R, sx * 0.35f);
        float bulge = (wL + wR) * sz * 0.06f;
        mesh->verts[i].pos.z += bulge;
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

static  int sculpt_sink(RigFaceMesh *mesh, Vec3f center,
                         float radius, float depth){
    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        Vec3f v  = mesh->verts[i].pos;
        float w  = radial_influence(v, center, radius);
        Vec3f n  = vec3_normalize(v);
        mesh->verts[i].pos = vec3_add(v, vec3_scale(n, -w * depth));
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: sculpt_sink -> rigpub_rig_face_engine_sculpt_sink */
int (*rigpub_rig_face_engine_sculpt_sink)(RigFaceMesh *mesh, Vec3f center, float radius, float depth) = sculpt_sink;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static  int sculpt_raise(RigFaceMesh *mesh, Vec3f center,
                          float radius, float height){
    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        Vec3f v = mesh->verts[i].pos;
        float w = radial_influence(v, center, radius);
        if (w < 1e-6f) continue;

        Vec3f dir;
        float nlen = sqrtf(vec3_dot(mesh->verts[i].normal,
                                    mesh->verts[i].normal));
        if (nlen > 1e-8f) {
            dir = vec3_scale(mesh->verts[i].normal, 1.0f / nlen);
        } else {
            Vec3f d = vec3_sub(v, center);
            float dl = sqrtf(vec3_dot(d, d));
            dir = (dl > 1e-8f) ? vec3_scale(d, 1.0f / dl)
                               : (Vec3f){0.0f, 0.0f, 1.0f};
        }
        mesh->verts[i].pos = vec3_add(v, vec3_scale(dir, w * height));
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: sculpt_raise -> rigpub_rig_face_engine_sculpt_raise */
int (*rigpub_rig_face_engine_sculpt_raise)(RigFaceMesh *mesh, Vec3f center, float radius, float height) = sculpt_raise;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static  int sculpt_push(RigFaceMesh *mesh, Vec3f center,
                         float radius, Vec3f direction, float amount){
    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        Vec3f v = mesh->verts[i].pos;
        float w = radial_influence(v, center, radius);
        mesh->verts[i].pos = vec3_add(v, vec3_scale(direction, w * amount));
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: sculpt_push -> rigpub_rig_face_engine_sculpt_push */
int (*rigpub_rig_face_engine_sculpt_push)(RigFaceMesh *mesh, Vec3f center, float radius, Vec3f direction, float amount) = sculpt_push;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


int rig_face_sculpt_features(RigFaceMesh *mesh, const RigFaceParams *p){
    float sx = p->cranium_width  * 0.5f;
    float sy = p->cranium_height * 0.5f;
    float sz = p->cranium_depth  * 0.5f;

    float eye_y    = sy * 0.12f;
    float eye_x    = p->interocular_dist * 0.5f + p->eye_width * 0.25f;
    float orb_r    = p->eye_width * 0.6f;
    float orb_dep  = p->orbital_depth * 0.3f;

    Vec3f orb_L = {-eye_x, eye_y, sz * 0.75f};
    Vec3f orb_R = { eye_x, eye_y, sz * 0.75f};
    sculpt_sink(mesh, orb_L, orb_r, orb_dep);
    sculpt_sink(mesh, orb_R, orb_r, orb_dep);

    float brow_y = eye_y + p->eye_width * 0.5f;
    Vec3f brow_L = {-eye_x, brow_y, sz * 0.8f};
    Vec3f brow_R = { eye_x, brow_y, sz * 0.8f};
    sculpt_raise(mesh, brow_L, orb_r * 0.8f, p->brow_protrusion * 0.15f);
    sculpt_raise(mesh, brow_R, orb_r * 0.8f, p->brow_protrusion * 0.15f);

    float nose_y_base = eye_y - p->nose_length * 0.5f;
    float nose_y_tip  = eye_y - p->nose_length;
    Vec3f nose_root   = {0, eye_y - p->nose_length * 0.1f, sz * 0.78f};
    Vec3f nose_bridge = {0, nose_y_base, sz * 0.82f};
    Vec3f nose_tip    = {0, nose_y_tip, sz * 0.85f + p->nasal_tip_proj * 0.2f};

    sculpt_raise(mesh, nose_root,   p->nasal_bridge_width * 0.8f, sz * 0.04f);
    sculpt_raise(mesh, nose_bridge, p->nasal_bridge_width * 0.7f, sz * 0.05f);

    sculpt_raise(mesh, nose_tip, p->nose_width * 0.5f,
                 p->nasal_tip_proj * 0.25f);

    Vec3f ala_L = {-p->nose_width * 0.45f, nose_y_tip + p->nose_width*0.2f, sz*0.8f};
    Vec3f ala_R = { p->nose_width * 0.45f, nose_y_tip + p->nose_width*0.2f, sz*0.8f};
    sculpt_raise(mesh, ala_L, p->nose_width * 0.35f, sz * 0.03f);
    sculpt_raise(mesh, ala_R, p->nose_width * 0.35f, sz * 0.03f);

    Vec3f filtrum = {0, nose_y_tip - p->nose_width*0.3f, sz * 0.82f};
    sculpt_sink(mesh, filtrum, p->nose_width * 0.3f, sz * 0.015f);

    float mouth_y = nose_y_tip - p->mid_third * 0.4f;
    float lip_z   = sz * 0.88f;

    Vec3f upper_lip = {0, mouth_y + p->lip_thickness_upper * 0.4f, lip_z};
    Vec3f lower_lip = {0, mouth_y - p->lip_thickness_lower * 0.4f, lip_z};
    Vec3f mouth_cen = {0, mouth_y, lip_z - sz * 0.01f};
    (void)mouth_cen;

    sculpt_raise(mesh, upper_lip, p->mouth_width * 0.55f,
                 p->lip_thickness_upper * 0.15f);
    sculpt_raise(mesh, lower_lip, p->mouth_width * 0.6f,
                 p->lip_thickness_lower * 0.18f);

    Vec3f cupid_L = {-p->mouth_width * 0.12f, mouth_y + p->lip_thickness_upper*0.2f, lip_z+0.01f};
    Vec3f cupid_R = { p->mouth_width * 0.12f, mouth_y + p->lip_thickness_upper*0.2f, lip_z+0.01f};
    sculpt_raise(mesh, cupid_L, p->mouth_width * 0.1f, p->lip_thickness_upper * 0.12f);
    sculpt_raise(mesh, cupid_R, p->mouth_width * 0.1f, p->lip_thickness_upper * 0.12f);

    Vec3f com_L = {-p->mouth_width * 0.5f, mouth_y, lip_z - 0.005f};
    Vec3f com_R = { p->mouth_width * 0.5f, mouth_y, lip_z - 0.005f};
    sculpt_sink(mesh, com_L, p->mouth_width * 0.08f, sz * 0.008f);
    sculpt_sink(mesh, com_R, p->mouth_width * 0.08f, sz * 0.008f);

    Vec3f naso_L = {-p->mouth_width * 0.55f, mouth_y + p->lip_thickness_upper * 1.5f, lip_z * 0.97f};
    Vec3f naso_R = { p->mouth_width * 0.55f, mouth_y + p->lip_thickness_upper * 1.5f, lip_z * 0.97f};
    Vec3f naso_dir = {0, 0, -1};
    sculpt_push(mesh, naso_L, p->mouth_width * 0.2f, naso_dir,
                sz * 0.012f * p->age_factor);
    sculpt_push(mesh, naso_R, p->mouth_width * 0.2f, naso_dir,
                sz * 0.012f * p->age_factor);

    float chin_y = mouth_y - p->lower_third * 0.55f;
    Vec3f chin = {0, chin_y, sz * 0.82f + p->chin_projection * 0.15f};
    sculpt_raise(mesh, chin, p->jaw_width * 0.25f, p->chin_projection * 0.1f);

    Vec3f chin_groove = {0, mouth_y - p->lower_third*0.12f, sz * 0.84f};
    sculpt_sink(mesh, chin_groove, p->jaw_width * 0.18f, sz * 0.01f);

    float fore_y = eye_y + p->upper_third * 0.5f;
    Vec3f fore   = {0, fore_y, sz * 0.72f};

    sculpt_raise(mesh, fore, sx * 0.75f, sz * 0.025f);

    float gf = p->gender_factor;

    Vec3f brow_center = {0, brow_y, sz * 0.8f};
    sculpt_raise(mesh, brow_center, sx * 0.6f, gf * sz * 0.025f);

    Vec3f jaw_L = {-p->jaw_width * 0.48f, -sy * 0.55f, sz * 0.55f};
    Vec3f jaw_R = { p->jaw_width * 0.48f, -sy * 0.55f, sz * 0.55f};
    sculpt_raise(mesh, jaw_L, sx * 0.2f, gf * sz * 0.02f);
    sculpt_raise(mesh, jaw_R, sx * 0.2f, gf * sz * 0.02f);
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

int rig_face_compute_smooth_normals(RigFaceMesh *mesh){

    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        mesh->verts[i].normal = (Vec3f){0,0,0};
    }

    for (uint32_t t = 0; t < mesh->n_tris; t++) {
        uint32_t ai = mesh->tris[t].a;
        uint32_t bi = mesh->tris[t].b;
        uint32_t ci = mesh->tris[t].c;
        Vec3f a = mesh->verts[ai].pos;
        Vec3f b = mesh->verts[bi].pos;
        Vec3f c = mesh->verts[ci].pos;
        Vec3f ab = vec3_sub(b, a);
        Vec3f ac = vec3_sub(c, a);
        Vec3f face_n = vec3_cross(ab, ac);

        Vec3f centroid = {
            (a.x + b.x + c.x) * 0.3333f,
            (a.y + b.y + c.y) * 0.3333f,
            (a.z + b.z + c.z) * 0.3333f
        };
        if (vec3_dot(face_n, centroid) < 0.0f)
            face_n = vec3_scale(face_n, -1.0f);

        float len_ab = sqrtf(vec3_dot(ab,ab));
        float len_ac = sqrtf(vec3_dot(ac,ac));
        float angle_a = (len_ab > 1e-8f && len_ac > 1e-8f) ?
            acosf(fmaxf(-1.0f, fminf(1.0f,
                vec3_dot(ab,ac) / (len_ab * len_ac)))) : 0.0f;
        mesh->verts[ai].normal = vec3_add(mesh->verts[ai].normal,
                                           vec3_scale(face_n, angle_a));

        Vec3f ba = vec3_sub(a, b);
        Vec3f bc = vec3_sub(c, b);
        float len_ba = len_ab;
        float len_bc = sqrtf(vec3_dot(bc,bc));
        float angle_b = (len_ba > 1e-8f && len_bc > 1e-8f) ?
            acosf(fmaxf(-1.0f, fminf(1.0f,
                vec3_dot(ba,bc) / (len_ba * len_bc)))) : 0.0f;
        mesh->verts[bi].normal = vec3_add(mesh->verts[bi].normal,
                                           vec3_scale(face_n, angle_b));

        Vec3f ca_v = vec3_sub(a, c);
        Vec3f cb = vec3_sub(b, c);
        float len_ca = len_ac;
        float len_cb = len_bc;
        float angle_c = RIG_PI - angle_a - angle_b;
        mesh->verts[ci].normal = vec3_add(mesh->verts[ci].normal,
                                           vec3_scale(face_n, angle_c));
        (void)ca_v; (void)cb; (void)len_ca; (void)len_cb;
    }

    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        mesh->verts[i].normal = vec3_normalize(mesh->verts[i].normal);
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

int rig_face_compute_tangent_basis(RigFaceMesh *mesh){

    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        mesh->verts[i].tangent   = (Vec3f){0,0,0};
        mesh->verts[i].bitangent = (Vec3f){0,0,0};
    }
    for (uint32_t t = 0; t < mesh->n_tris; t++) {
        uint32_t i0 = mesh->tris[t].a;
        uint32_t i1 = mesh->tris[t].b;
        uint32_t i2 = mesh->tris[t].c;
        Vec3f p0 = mesh->verts[i0].pos;
        Vec3f p1 = mesh->verts[i1].pos;
        Vec3f p2 = mesh->verts[i2].pos;
        Vec2f uv0 = mesh->verts[i0].uv;
        Vec2f uv1 = mesh->verts[i1].uv;
        Vec2f uv2 = mesh->verts[i2].uv;
        Vec3f e1 = vec3_sub(p1, p0);
        Vec3f e2 = vec3_sub(p2, p0);
        float du1 = uv1.u - uv0.u;  float dv1 = uv1.v - uv0.v;
        float du2 = uv2.u - uv0.u;  float dv2 = uv2.v - uv0.v;
        float det = du1*dv2 - du2*dv1;
        if (fabsf(det) < 1e-8f) continue;
        float inv = 1.0f / det;
        Vec3f T = {
            inv * (dv2*e1.x - dv1*e2.x),
            inv * (dv2*e1.y - dv1*e2.y),
            inv * (dv2*e1.z - dv1*e2.z)
        };
        Vec3f B = {
            inv * (-du2*e1.x + du1*e2.x),
            inv * (-du2*e1.y + du1*e2.y),
            inv * (-du2*e1.z + du1*e2.z)
        };
        mesh->verts[i0].tangent   = vec3_add(mesh->verts[i0].tangent,   T);
        mesh->verts[i1].tangent   = vec3_add(mesh->verts[i1].tangent,   T);
        mesh->verts[i2].tangent   = vec3_add(mesh->verts[i2].tangent,   T);
        mesh->verts[i0].bitangent = vec3_add(mesh->verts[i0].bitangent, B);
        mesh->verts[i1].bitangent = vec3_add(mesh->verts[i1].bitangent, B);
        mesh->verts[i2].bitangent = vec3_add(mesh->verts[i2].bitangent, B);
    }

    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        Vec3f n = mesh->verts[i].normal;
        Vec3f t = mesh->verts[i].tangent;

        t = vec3_sub(t, vec3_scale(n, vec3_dot(n, t)));
        mesh->verts[i].tangent   = vec3_normalize(t);
        mesh->verts[i].bitangent = vec3_normalize(mesh->verts[i].bitangent);
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

RigSkinMaterial rig_skin_material_from_params(const RigFaceParams *p) {
    RigSkinMaterial m;
    memset(&m, 0, sizeof(m));

    float mel = p->melanin;
    float hemo = p->hemoglobin;
    float age  = p->age_factor;

    float mel_r = 0.77f - mel * 0.55f;
    float mel_g = 0.57f - mel * 0.40f;
    float mel_b = 0.44f - mel * 0.30f;

    mel_r += hemo * 0.18f;
    mel_g += hemo * 0.05f;
    mel_b += hemo * 0.04f;

    float car = p->carotene;
    mel_r += car * 0.08f;
    mel_g += car * 0.07f;

    float desat = age * 0.08f;
    float avg = (mel_r + mel_g + mel_b) / 3.0f;
    mel_r += (avg - mel_r) * desat;
    mel_g += (avg - mel_g) * desat;
    mel_b += (avg - mel_b) * desat;

    m.base_color[0] = fmaxf(0, fminf(1, mel_r));
    m.base_color[1] = fmaxf(0, fminf(1, mel_g));
    m.base_color[2] = fmaxf(0, fminf(1, mel_b));
    m.melanin_concentration    = mel;
    m.hemoglobin_concentration = hemo;
    m.carotene_concentration   = car;

    m.roughness = 0.45f + age * 0.2f;
    m.metallic  = 0.0f;
    m.specular  = 0.028f;

    float sss_scale = 1.0f + mel * 0.5f;
    m.sss_radius[0] = 3.67f * sss_scale;
    m.sss_radius[1] = 1.37f * sss_scale;
    m.sss_radius[2] = 0.68f * sss_scale;
    m.sss_weight = 0.85f - mel * 0.3f;
    m.sss_scale  = 0.01f;

    m.pore_scale  = 0.002f;
    m.pore_depth  = 0.3f + mel * 0.1f;
    m.wrinkle_depth = 0.15f + age * 0.6f;
    m.oiliness    = 0.2f - age * 0.05f;
    m.fuzz_amount = 0.1f - p->gender_factor * 0.05f;
    m.fuzz_tilt   = 15.0f;

    return m;
}

int rig_face_bake_vertex_color(RigFaceMesh *mesh){
    const RigFaceParams *p = &mesh->params;
    float sy = p->cranium_height * 0.5f;
    float sz = p->cranium_depth  * 0.5f;
    float sx = p->cranium_width  * 0.5f;

    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        Vec3f v = mesh->verts[i].pos;

        float nx = v.x / sx;
        float ny = v.y / sy;
        float nz = v.z / sz;

        float mel   = p->melanin;
        float hemo  = p->hemoglobin;
        float rough = 0.45f;

        float cheek_center_y = sy * 0.05f;
        float cheek_L = radial_influence(v,
            (Vec3f){-p->zygomatic_width*0.35f, cheek_center_y, sz*0.85f}, sx*0.25f);
        float cheek_R = radial_influence(v,
            (Vec3f){ p->zygomatic_width*0.35f, cheek_center_y, sz*0.85f}, sx*0.25f);
        float cheek = (cheek_L + cheek_R) * 0.5f;

        float lip_area = radial_influence(v,
            (Vec3f){0, -sy*0.25f, sz*0.88f}, sx*0.2f);

        float thin_area = (v.y > sy*0.1f) ? 0.6f :
                          (fabsf(v.x) < sx*0.3f && v.y > -sy*0.1f) ? 0.4f : 0.3f;

        float hemo_local = hemo + cheek * 0.25f + lip_area * 0.35f;

        float rough_local = rough + p->age_factor * 0.2f;

        if (v.y < 0.1f && fabsf(v.x) < sx*0.15f && v.z > sz*0.7f)
            rough_local += 0.08f;

        mesh->verts[i].color = (Vec4f){
            fmaxf(0,fminf(1, mel + cheek*0.05f)),
            fmaxf(0,fminf(1, hemo_local)),
            fmaxf(0,fminf(1, thin_area)),
            fmaxf(0,fminf(1, rough_local))
        };
        (void)nx; (void)ny; (void)nz;
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

int rig_face_unwrap_uv_seams(RigFaceMesh *mesh){

    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        Vec3f n = vec3_normalize(mesh->verts[i].pos);
        float theta = atan2f(n.z, n.x);
        float phi   = acosf(fmaxf(-1.0f, fminf(1.0f, n.y)));
        mesh->verts[i].uv.u = (theta / RIG_TAU) + 0.5f;
        mesh->verts[i].uv.v = phi / RIG_PI;
    }

    for (uint32_t t = 0; t < mesh->n_tris; t++) {
        float u0 = mesh->verts[mesh->tris[t].a].uv.u;
        float u1 = mesh->verts[mesh->tris[t].b].uv.u;
        float u2 = mesh->verts[mesh->tris[t].c].uv.u;
        if (fabsf(u0 - u1) > 0.5f || fabsf(u1 - u2) > 0.5f ||
            fabsf(u0 - u2) > 0.5f) {

            if (u0 < 0.5f) mesh->verts[mesh->tris[t].a].uv2.u = u0 + 1.0f;
            if (u1 < 0.5f) mesh->verts[mesh->tris[t].b].uv2.u = u1 + 1.0f;
            if (u2 < 0.5f) mesh->verts[mesh->tris[t].c].uv2.u = u2 + 1.0f;
        }
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

#define FLOATS_PER_VERT 15

int rig_face_export_vbo(const RigFaceMesh *mesh,
                         float *vbo, uint32_t *ibo,
                         uint32_t *n_floats, uint32_t *n_indices) {
    if (!vbo || !ibo) return -1;
    *n_floats  = mesh->n_verts  * FLOATS_PER_VERT;
    *n_indices = mesh->n_tris   * 3;

    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        float *dst = vbo + i * FLOATS_PER_VERT;
        const RigFaceVertex *v = &mesh->verts[i];
        dst[ 0] = v->pos.x;    dst[ 1] = v->pos.y;    dst[ 2] = v->pos.z;
        dst[ 3] = v->normal.x; dst[ 4] = v->normal.y; dst[ 5] = v->normal.z;
        dst[ 6] = v->tangent.x;dst[ 7] = v->tangent.y;dst[ 8] = v->tangent.z;
        dst[ 9] = v->uv.u;     dst[10] = v->uv.v;
        dst[11] = v->color.x;  dst[12] = v->color.y;
        dst[13] = v->color.z;  dst[14] = v->color.w;
    }
    for (uint32_t t = 0; t < mesh->n_tris; t++) {
        ibo[t*3+0] = mesh->tris[t].a;
        ibo[t*3+1] = mesh->tris[t].b;
        ibo[t*3+2] = mesh->tris[t].c;
    }
    return 0;
}

int rig_face_export_obj(const RigFaceMesh *mesh, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "# RigCom v24 THE SANTORIUM OF COMPILER — Face Engine\n");
    fprintf(f, "# Verts: %u  Tris: %u\n", mesh->n_verts, mesh->n_tris);
    fprintf(f, "# φ-error: %.6f\n\n", mesh->phi_error);
    fprintf(f, "o RigFace\n");
    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        fprintf(f, "v %.6f %.6f %.6f\n",
                mesh->verts[i].pos.x, mesh->verts[i].pos.y, mesh->verts[i].pos.z);
    }
    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        fprintf(f, "vt %.6f %.6f\n", mesh->verts[i].uv.u, mesh->verts[i].uv.v);
    }
    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        fprintf(f, "vn %.6f %.6f %.6f\n",
                mesh->verts[i].normal.x, mesh->verts[i].normal.y, mesh->verts[i].normal.z);
    }
    for (uint32_t t = 0; t < mesh->n_tris; t++) {
        uint32_t a = mesh->tris[t].a + 1;
        uint32_t b = mesh->tris[t].b + 1;
        uint32_t c = mesh->tris[t].c + 1;
        fprintf(f, "f %u/%u/%u %u/%u/%u %u/%u/%u\n",
                a,a,a, b,b,b, c,c,c);
    }
    fclose(f);
    return 0;
}

int rig_face_export_glsl_uniforms(const RigFaceMesh *mesh, char *buf, size_t sz){
    const RigSkinMaterial *m = &mesh->material;
    snprintf(buf, sz,
        "// RigCom Face PBR Uniforms\n"
        "uniform vec3  u_baseColor     = vec3(%.4f, %.4f, %.4f);\n"
        "uniform float u_roughness     = %.4f;\n"
        "uniform float u_metallic      = %.4f;\n"
        "uniform float u_specular      = %.4f;\n"
        "uniform vec3  u_sssRadius     = vec3(%.4f, %.4f, %.4f);\n"
        "uniform float u_sssWeight     = %.4f;\n"
        "uniform float u_sssScale      = %.6f;\n"
        "uniform float u_melanin       = %.4f;\n"
        "uniform float u_hemoglobin    = %.4f;\n"
        "uniform float u_poreScale     = %.6f;\n"
        "uniform float u_poreDepth     = %.4f;\n"
        "uniform float u_wrinkleDepth  = %.4f;\n"
        "uniform float u_oiliness      = %.4f;\n"
        "uniform float u_fuzzAmount    = %.4f;\n",
        m->base_color[0], m->base_color[1], m->base_color[2],
        m->roughness, m->metallic, m->specular,
        m->sss_radius[0], m->sss_radius[1], m->sss_radius[2],
        m->sss_weight, m->sss_scale,
        m->melanin_concentration, m->hemoglobin_concentration,
        m->pore_scale, m->pore_depth, m->wrinkle_depth,
        m->oiliness, m->fuzz_amount
    );
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

RigFaceMesh* rig_face_create(const RigFaceParams *params, uint32_t subdiv_level) {
    arena_init();
    if (subdiv_level < 2) subdiv_level = 2;
    if (subdiv_level > 6) subdiv_level = 6;

    RigFaceMesh *mesh = (RigFaceMesh*)arena_alloc(sizeof(RigFaceMesh));
    if (!mesh) return NULL;
    memset(mesh, 0, sizeof(RigFaceMesh));

    if (params) mesh->params = *params;
    else        mesh->params = rig_face_default_params();

    if (rig_face_build_base_sphere(mesh) != 0 ||
        rig_face_subdivide_catmull_clark(mesh, subdiv_level) != 0) {
        arena_reset();
        return NULL;
    }
    rig_face_apply_cranial_deform(mesh, &mesh->params);
    rig_face_sculpt_features(mesh, &mesh->params);
    rig_face_compute_smooth_normals(mesh);
    rig_face_unwrap_uv_seams(mesh);
    rig_face_compute_tangent_basis(mesh);
    mesh->material = rig_skin_material_from_params(&mesh->params);
    rig_face_bake_vertex_color(mesh);

    mesh->aabb_min = mesh->verts[0].pos;
    mesh->aabb_max = mesh->verts[0].pos;
    for (uint32_t i = 1; i < mesh->n_verts; i++) {
        Vec3f v = mesh->verts[i].pos;
        if (v.x < mesh->aabb_min.x) mesh->aabb_min.x = v.x;
        if (v.y < mesh->aabb_min.y) mesh->aabb_min.y = v.y;
        if (v.z < mesh->aabb_min.z) mesh->aabb_min.z = v.z;
        if (v.x > mesh->aabb_max.x) mesh->aabb_max.x = v.x;
        if (v.y > mesh->aabb_max.y) mesh->aabb_max.y = v.y;
        if (v.z > mesh->aabb_max.z) mesh->aabb_max.z = v.z;
    }

    float fw = mesh->params.zygomatic_width;
    float fh = mesh->params.upper_third + mesh->params.mid_third + mesh->params.lower_third;
    float ratio = (fh > 0) ? fw/fh : RIG_PHI;
    mesh->phi_error = fabsf(ratio - (float)RIG_PHI) / (float)RIG_PHI;

    return mesh;
}

int rig_face_destroy(RigFaceMesh *mesh){

    (void)mesh;
    arena_reset();
    return 0; /* 0=liberado, -1=ptr nulo/arena corrupta */
}

int rig_face_build_neck(RigFaceMesh *mesh, const RigFaceParams *p){
    float sy      = p->cranium_height * 0.5f;
    float sz      = p->cranium_depth  * 0.5f;
    float jaw_bot = -sy * 0.85f;
    float neck_bot = jaw_bot - p->neck_length;

    const int NECK_RINGS = 6;
    float neck_rx = p->neck_width  * 0.5f;
    float neck_rz = p->cranium_depth * 0.3f;

    uint32_t base_start = mesh->n_verts;

    uint32_t ring_buf[256];
    uint32_t ring_n = 0;
    float eps = sy * 0.08f;
    for (uint32_t i = 0; i < mesh->n_verts && ring_n < 255; i++) {
        if (mesh->verts[i].pos.y < jaw_bot + eps) {
            ring_buf[ring_n++] = i;
        }
    }
    if (ring_n < 3) return -1;

    uint32_t prev_ring[256];
    memcpy(prev_ring, ring_buf, ring_n * sizeof(uint32_t));

    for (int r = 1; r <= NECK_RINGS; r++) {
        float t   = (float)r / NECK_RINGS;
        float y_r = jaw_bot + (neck_bot - jaw_bot) * t;

        float scale_xz = phi_lerp(1.0f, neck_rx / (p->jaw_width * 0.5f), t);

        uint32_t new_ring[256];
        for (uint32_t i = 0; i < ring_n; i++) {
            Vec3f src = mesh->verts[prev_ring[i]].pos;
            RigFaceVertex nv = mesh->verts[prev_ring[i]];
            nv.pos.x = src.x * scale_xz;
            nv.pos.z = src.z * (neck_rz / (sz * 0.6f)) * scale_xz;
            nv.pos.y = y_r;

            nv.color.y *= (1.0f - t * 0.3f);
            uint32_t idx = mesh->n_verts;
            mesh->verts[idx] = nv;
            mesh->n_verts++;
            new_ring[i] = idx;
        }

        for (uint32_t i = 0; i < ring_n && mesh->n_tris + 2 <= RIG_FACE_TRIS_MAX; i++) {
            uint32_t next_i = (i + 1) % ring_n;
            uint32_t a = prev_ring[i];
            uint32_t b = prev_ring[next_i];
            uint32_t c = new_ring[next_i];
            uint32_t d = new_ring[i];
            mesh->tris[mesh->n_tris++] = (RigFaceTri){a, b, c};
            mesh->tris[mesh->n_tris++] = (RigFaceTri){a, c, d};
        }
        memcpy(prev_ring, new_ring, ring_n * sizeof(uint32_t));
    }

    if (mesh->n_verts < RIG_FACE_VERTS_MAX && mesh->n_tris + ring_n < RIG_FACE_TRIS_MAX) {
        Vec3f cap_center = {0, neck_bot, 0};
        for (uint32_t i = 0; i < ring_n; i++) {
            Vec3f p_i = mesh->verts[prev_ring[i]].pos;
            cap_center.x += p_i.x;
            cap_center.z += p_i.z;
        }
        cap_center.x /= ring_n;
        cap_center.z /= ring_n;

        uint32_t cap_idx = mesh->n_verts;
        RigFaceVertex cap_v;
        memset(&cap_v, 0, sizeof(cap_v));
        cap_v.pos    = cap_center;
        cap_v.normal = (Vec3f){0.0f, -1.0f, 0.0f};
        cap_v.uv.u   = 0.5f;
        cap_v.uv.v   = 1.0f;
        cap_v.color  = mesh->verts[prev_ring[0]].color;
        mesh->verts[cap_idx] = cap_v;
        mesh->n_verts++;

        for (uint32_t i = 0; i < ring_n && mesh->n_tris < RIG_FACE_TRIS_MAX; i++) {
            uint32_t next_i = (i + 1) % ring_n;

            mesh->tris[mesh->n_tris++] = (RigFaceTri){
                cap_idx, prev_ring[next_i], prev_ring[i]
            };
        }
    }
    (void)base_start;
}

typedef struct {
    const char *name;
    uint8_t     au_code;

    uint8_t     zone;

    float       dx, dy, dz;
    float       radius_k;
} FACSSpec;

static const FACSSpec FACS_TABLE[RIG_FACE_BLEND_SHAPES] = {

    {"AU_1_InnerBrowRaise",     1,  0,  0.0f,  1.0f, 0.1f, 0.18f},
    {"AU_2_OuterBrowRaise",     2,  1,  0.2f,  1.0f, 0.1f, 0.20f},
    {"AU_4_BrowLowerer",        4,  0,  0.0f, -1.0f, 0.0f, 0.22f},
    {"AU_5_UpperLidRaiser",     5,  2,  0.0f,  0.5f, 0.3f, 0.12f},
    {"AU_6_CheekRaiser",        6,  4,  0.0f,  0.6f, 0.4f, 0.18f},
    {"AU_7_LidTightener",       7,  3,  0.0f, -0.3f, 0.2f, 0.10f},
    {"AU_9_NoseWrinkler",       9,  5,  0.0f,  0.4f, 0.4f, 0.14f},
    {"AU_10_UpperLipRaiser",   10,  6,  0.0f,  0.6f, 0.5f, 0.14f},
    {"AU_11_NasolabialDeep",   11,  8, -0.3f,  0.3f, 0.2f, 0.16f},
    {"AU_12_LipCornerPuller",  12,  8,  0.7f,  0.3f, 0.3f, 0.14f},
    {"AU_13_CheekPuffer",      13,  4,  0.5f,  0.2f, 0.5f, 0.20f},
    {"AU_14_Dimpler",          14,  8,  0.4f,  0.0f, 0.2f, 0.08f},
    {"AU_15_LipCornerDepressor",15, 8,  0.5f, -0.5f, 0.1f, 0.12f},
    {"AU_16_LowerLipDepressor",16,  7,  0.0f, -0.6f, 0.2f, 0.16f},
    {"AU_17_ChinRaiser",       17,  9,  0.0f,  0.5f, 0.3f, 0.16f},
    {"AU_18_LipPuckerer",      18,  6,  0.0f,  0.2f, 0.8f, 0.12f},
    {"AU_20_LipStretcher",     20,  8,  0.9f,  0.0f, 0.1f, 0.10f},
    {"AU_22_LipFunneler",      22,  6,  0.0f,  0.3f, 0.9f, 0.10f},
    {"AU_23_LipTightener",     23,  6,  0.0f,  0.0f, 0.4f, 0.12f},
    {"AU_24_LipPressor",       24,  7,  0.0f,  0.3f, 0.3f, 0.10f},
    {"AU_25_LipsPart",         25,  7,  0.0f, -0.5f, 0.2f, 0.14f},
    {"AU_26_JawDrop",          26, 10,  0.0f, -1.0f, 0.0f, 0.30f},
    {"AU_27_MouthStretch",     27, 10,  0.0f, -0.8f, 0.3f, 0.22f},
    {"AU_28_LipSuck",          28,  6,  0.0f,  0.0f,-0.9f, 0.10f},
    {"AU_29_JawThrust",        29, 10,  0.0f,  0.0f, 0.8f, 0.28f},
    {"AU_30_JawSideways",      30, 10,  0.8f,  0.0f, 0.0f, 0.26f},
    {"AU_31_BiteNailR",        31,  8,  0.8f, -0.3f, 0.3f, 0.08f},
    {"AU_32_LipBiteR",         32,  8,  0.7f,  0.0f, 0.4f, 0.08f},
    {"AU_33_BlowR",            33,  4,  0.8f,  0.1f, 0.6f, 0.14f},
    {"AU_34_PuffR",            34,  4,  0.9f,  0.0f, 0.5f, 0.16f},
    {"AU_35_CheekSquintR",     35,  4,  0.6f,  0.4f, 0.3f, 0.14f},
    {"AU_36_TongueShow",       36,  7,  0.0f, -0.2f, 1.0f, 0.08f},
    {"AU_37_LipWipeR",         37,  8,  0.6f,  0.2f, 0.2f, 0.08f},
    {"AU_38_NostrilDilatorR",  38,  5,  0.7f,  0.0f, 0.5f, 0.10f},
    {"AU_39_NostrilCompressorR",39, 5,  0.6f,  0.0f,-0.3f, 0.08f},
    {"AU_41_LidDroopR",        41,  2,  0.4f, -0.5f, 0.0f, 0.10f},
    {"AU_42_SlitR",            42,  2,  0.3f, -0.3f, 0.1f, 0.08f},
    {"AU_43_EyesClosedR",      43,  2,  0.0f, -0.8f, 0.0f, 0.12f},
    {"AU_44_SquintR",          44,  3,  0.3f,  0.4f, 0.0f, 0.10f},
    {"AU_45_BlinkR",           45,  2,  0.0f, -1.0f, 0.0f, 0.12f},
    {"AU_46_WinkR",            46,  2,  0.3f, -0.9f, 0.0f, 0.10f},

    {"AU_31L_BiteNailL",       31,  8, -0.8f, -0.3f, 0.3f, 0.08f},
    {"AU_32L_LipBiteL",        32,  8, -0.7f,  0.0f, 0.4f, 0.08f},
    {"AU_33L_BlowL",           33,  4, -0.8f,  0.1f, 0.6f, 0.14f},
    {"AU_34L_PuffL",           34,  4, -0.9f,  0.0f, 0.5f, 0.16f},
    {"AU_35L_CheekSquintL",    35,  4, -0.6f,  0.4f, 0.3f, 0.14f},
    {"AU_38L_NostrilDilatorL", 38,  5, -0.7f,  0.0f, 0.5f, 0.10f},
    {"AU_41L_LidDroopL",       41,  2, -0.4f, -0.5f, 0.0f, 0.10f},
    {"AU_43L_EyesClosedL",     43,  2,  0.0f, -0.8f, 0.0f, 0.12f},
    {"AU_45L_BlinkL",          45,  2,  0.0f, -1.0f, 0.0f, 0.12f},
    {"AU_46L_WinkL",           46,  2, -0.3f, -0.9f, 0.0f, 0.10f},
    {"AU_51_HeadTurnR",        51, 11,  0.5f,  0.0f, 0.0f, 0.50f},
    {"AU_52_HeadTurnL",        52, 11, -0.5f,  0.0f, 0.0f, 0.50f},
};

static  int facs_compute_zone_centers(const RigFaceMesh *mesh, Vec3f zones[12]){
    const RigFaceParams *p = &mesh->params;
    float sx = p->cranium_width  * 0.5f;
    (void)sx;
    float sy = p->cranium_height * 0.5f;
    float sz = p->cranium_depth  * 0.5f;

    float eye_y  = sy * 0.12f;
    float eye_x  = p->interocular_dist * 0.5f + p->eye_width * 0.25f;
    float nose_y = eye_y - p->nose_length;
    float mouth_y= nose_y - p->mid_third * 0.4f;
    float chin_y = mouth_y - p->lower_third * 0.55f;

    zones[0]  = (Vec3f){-eye_x * 0.5f,  eye_y + p->eye_width*0.5f, sz*0.78f};
    zones[1]  = (Vec3f){-eye_x * 1.3f,  eye_y + p->eye_width*0.4f, sz*0.72f};
    zones[2]  = (Vec3f){-eye_x,          eye_y,                      sz*0.78f};
    zones[3]  = (Vec3f){-eye_x,          eye_y - p->eye_width*0.3f,  sz*0.76f};
    zones[4]  = (Vec3f){-p->zygomatic_width*0.35f, sy*0.05f,         sz*0.85f};
    zones[5]  = (Vec3f){ 0.0f,           nose_y + p->nose_length*0.5f,sz*0.83f};
    zones[6]  = (Vec3f){ 0.0f,           mouth_y + p->lip_thickness_upper*0.3f, sz*0.87f};
    zones[7]  = (Vec3f){ 0.0f,           mouth_y - p->lip_thickness_lower*0.3f, sz*0.86f};
    zones[8]  = (Vec3f){-p->mouth_width*0.5f, mouth_y,               sz*0.84f};
    zones[9]  = (Vec3f){ 0.0f,           chin_y,                     sz*0.80f};
    zones[10] = (Vec3f){ 0.0f,          -sy*0.70f,                   sz*0.50f};
    zones[11] = (Vec3f){ 0.0f,          -sy*1.20f,                   sz*0.30f};
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: facs_compute_zone_centers -> rigpub_rig_face_engine_facs_compute_zone_centers */
int (*rigpub_rig_face_engine_facs_compute_zone_centers)(const RigFaceMesh *mesh, Vec3f zones[12]) = facs_compute_zone_centers;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


int rig_face_build_facs_shapes(RigFaceMesh *mesh){
    const RigFaceParams *p = &mesh->params;
    float sx = p->cranium_width * 0.5f;

    Vec3f zones[12];
    facs_compute_zone_centers(mesh, zones);

    for (int s = 0; s < RIG_FACE_BLEND_SHAPES; s++) {
        const FACSSpec *spec = &FACS_TABLE[s];
        RigFaceBlendShape *shape = &mesh->shapes[s];
        strncpy(shape->name, spec->name, 31);
        shape->name[31] = '\0';
        shape->au_code  = spec->au_code;
        shape->weight   = 0.0f;

        Vec3f zone_c = zones[spec->zone];

        Vec3f center = {
            zone_c.x + spec->dx * sx * 0.5f,
            zone_c.y + spec->dy * sx * 0.1f,
            zone_c.z + spec->dz * sx * 0.05f
        };
        float radius = spec->radius_k * sx;
        Vec3f delta  = {spec->dx * sx * 0.012f,
                        spec->dy * sx * 0.012f,
                        spec->dz * sx * 0.012f};

        uint32_t count = 0;
        for (uint32_t i = 0; i < mesh->n_verts; i++) {
            if (radial_influence(mesh->verts[i].pos, center, radius) > 0.001f)
                count++;
        }
        shape->n_deltas   = count;
        shape->indices    = (uint32_t*)arena_alloc(count * sizeof(uint32_t));
        shape->pos_deltas = (Vec3f*)   arena_alloc(count * sizeof(Vec3f));
        shape->nrm_deltas = (Vec3f*)   arena_alloc(count * sizeof(Vec3f));

        if (!shape->indices || !shape->pos_deltas || !shape->nrm_deltas) {
            shape->n_deltas = 0;
            continue;
        }

        uint32_t k = 0;
        for (uint32_t i = 0; i < mesh->n_verts && k < count; i++) {
            float w = radial_influence(mesh->verts[i].pos, center, radius);
            if (w <= 0.001f) continue;
            shape->indices[k]    = i;
            shape->pos_deltas[k] = vec3_scale(delta, w);

            shape->nrm_deltas[k] = vec3_normalize(delta);
            k++;
        }
    }
    return 0; /* 0=OK, -1=ENOMEM o fallo de arena */
}

int rig_face_apply_expression(RigFaceMesh *mesh, const float weights[RIG_FACE_BLEND_SHAPES]){

    for (int s = 0; s < RIG_FACE_BLEND_SHAPES; s++) {
        RigFaceBlendShape *shape = &mesh->shapes[s];
        float new_w = (weights && s < 52) ? weights[s] : 0.0f;
        new_w = fmaxf(0.0f, fminf(1.0f, new_w));
        float dw = new_w - shape->weight;
        if (fabsf(dw) < 1e-6f) continue;

        for (uint32_t k = 0; k < shape->n_deltas; k++) {
            uint32_t idx = shape->indices[k];
            mesh->verts[idx].pos = vec3_add(
                mesh->verts[idx].pos,
                vec3_scale(shape->pos_deltas[k], dw));
        }
        shape->weight = new_w;
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

int rig_face_solve_psd(RigFaceMesh *mesh){

    struct { int a; int b; float cx, cy, cz; float rk; } PSD_PAIRS[] = {
        {4,  9,   0.0f,  0.003f, 0.002f, 0.20f},
        {20, 21,  0.0f, -0.004f, 0.001f, 0.28f},
        {2,  6,   0.0f, -0.002f, 0.000f, 0.20f},
    };
    const int N_PSD = (int)(sizeof(PSD_PAIRS)/sizeof(PSD_PAIRS[0]));

    float sx = mesh->params.cranium_width * 0.5f;

    Vec3f zones[12];
    facs_compute_zone_centers(mesh, zones);

    for (int pi = 0; pi < N_PSD; pi++) {
        float wa = mesh->shapes[PSD_PAIRS[pi].a].weight;
        float wb = mesh->shapes[PSD_PAIRS[pi].b].weight;
        float combined = wa * wb;
        if (combined < 1e-4f) continue;

        int za = FACS_TABLE[PSD_PAIRS[pi].a].zone;
        int zb = FACS_TABLE[PSD_PAIRS[pi].b].zone;
        Vec3f center = {
            (zones[za].x + zones[zb].x) * 0.5f,
            (zones[za].y + zones[zb].y) * 0.5f,
            (zones[za].z + zones[zb].z) * 0.5f
        };
        float radius = PSD_PAIRS[pi].rk * sx;
        Vec3f corr = {
            PSD_PAIRS[pi].cx * sx * combined,
            PSD_PAIRS[pi].cy * sx * combined,
            PSD_PAIRS[pi].cz * sx * combined
        };
        for (uint32_t i = 0; i < mesh->n_verts; i++) {
            float w = radial_influence(mesh->verts[i].pos, center, radius);
            if (w < 1e-5f) continue;
            mesh->verts[i].pos = vec3_add(
                mesh->verts[i].pos, vec3_scale(corr, w));
        }
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

typedef struct { const char *name; uint8_t parent;
                 float hx,hy,hz, tx,ty,tz; } BoneDef;

static  int bones_from_params(const RigFaceParams *p, BoneDef defs[RIG_FACE_BONES]){
    float sx = p->cranium_width  * 0.5f;
    (void)sx;
    float sy = p->cranium_height * 0.5f;
    float sz = p->cranium_depth  * 0.5f;
    float ey = sy * 0.12f;
    float ex = p->interocular_dist * 0.5f + p->eye_width * 0.25f;
    float ny = ey - p->nose_length;
    float my = ny - p->mid_third * 0.4f;

    defs[0] = (BoneDef){"root",         0xFF, 0,0,0, 0,sy*0.2f,0};

    defs[1] = (BoneDef){"zygo_L",       0,   -p->zygomatic_width*0.5f, sy*0.05f, sz*0.3f,
                                     -p->zygomatic_width*0.5f, sy*0.05f, sz*0.7f};
    defs[2] = (BoneDef){"zygo_R",       0,    p->zygomatic_width*0.5f, sy*0.05f, sz*0.3f,
                                      p->zygomatic_width*0.5f, sy*0.05f, sz*0.7f};

    defs[3] = (BoneDef){"orbit_L",      1,   -ex, ey, sz*0.7f, -ex, ey, sz*0.9f};
    defs[4] = (BoneDef){"orbit_R",      2,    ex, ey, sz*0.7f,  ex, ey, sz*0.9f};

    defs[5] = (BoneDef){"lid_upper_L",  3,   -ex, ey+p->eye_width*0.3f, sz*0.82f,
                                     -ex, ey+p->eye_width*0.5f, sz*0.85f};
    defs[6] = (BoneDef){"lid_lower_L",  3,   -ex, ey-p->eye_width*0.3f, sz*0.80f,
                                     -ex, ey-p->eye_width*0.5f, sz*0.82f};
    defs[7] = (BoneDef){"lid_upper_R",  4,    ex, ey+p->eye_width*0.3f, sz*0.82f,
                                      ex, ey+p->eye_width*0.5f, sz*0.85f};
    defs[8] = (BoneDef){"lid_lower_R",  4,    ex, ey-p->eye_width*0.3f, sz*0.80f,
                                      ex, ey-p->eye_width*0.5f, sz*0.82f};

    defs[9] = (BoneDef){"brow_L",       1,   -ex, ey+p->eye_width*0.6f, sz*0.78f,
                                     -ex*1.3f, ey+p->eye_width*0.5f, sz*0.72f};
    defs[10] = (BoneDef){"brow_R",       2,    ex, ey+p->eye_width*0.6f, sz*0.78f,
                                      ex*1.3f, ey+p->eye_width*0.5f, sz*0.72f};

    defs[11] = (BoneDef){"nose",         0,    0, ey-p->nose_length*0.3f, sz*0.80f,
                                      0, ny, sz*0.88f + p->nasal_tip_proj*0.2f};

    defs[12] = (BoneDef){"maxilla",      0,    0, ny - p->mid_third*0.2f, sz*0.82f,
                                      0, my, sz*0.86f};

    defs[13] = (BoneDef){"mandible",     12,   0, my - p->lower_third*0.3f, sz*0.75f,
                                      0, -sy*0.7f, sz*0.5f};

    defs[14] = (BoneDef){"corner_L",     12,  -p->mouth_width*0.5f, my, sz*0.84f,
                                     -p->mouth_width*0.6f, my-p->lower_third*0.1f, sz*0.82f};
    defs[15] = (BoneDef){"corner_R",     12,   p->mouth_width*0.5f, my, sz*0.84f,
                                      p->mouth_width*0.6f, my-p->lower_third*0.1f, sz*0.82f};

    defs[16] = (BoneDef){"chin",         13,   0, my-p->lower_third*0.55f, sz*0.80f,
                                      0, -sy*0.6f, sz*0.65f};

    defs[17] = (BoneDef){"neck_base",    0,    0, -sy*0.90f, sz*0.2f,
                                      0, -sy*1.20f, sz*0.1f};
    defs[18] = (BoneDef){"neck_mid",     17,   0, -sy*1.20f, sz*0.1f,
                                      0, -sy*1.50f, sz*0.0f};

    defs[19] = (BoneDef){"head",         0,    0, 0, 0, 0, sy*0.5f, 0};

    defs[20] = (BoneDef){"lip_upper",    12,   0, my+p->lip_thickness_upper*0.3f, sz*0.87f,
                                      0, my+p->lip_thickness_upper*0.6f, sz*0.90f};
    defs[21] = (BoneDef){"lip_lower",    13,   0, my-p->lip_thickness_lower*0.3f, sz*0.86f,
                                      0, my-p->lip_thickness_lower*0.6f, sz*0.87f};
    defs[22] = (BoneDef){"jaw_L",        13,  -p->jaw_width*0.48f, -sy*0.55f, sz*0.55f,
                                     -p->jaw_width*0.50f, -sy*0.80f, sz*0.40f};
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: bones_from_params -> rigpub_rig_face_engine_bones_from_params */
int (*rigpub_rig_face_engine_bones_from_params)(const RigFaceParams *p, BoneDef defs[RIG_FACE_BONES]) = bones_from_params;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static  int bone_build_bind(RigFaceBone *b, Vec3f head, Vec3f tail){
    b->head = head;
    b->tail = tail;

    Vec3f y_axis = vec3_normalize(vec3_sub(tail, head));

    Vec3f z_world = {0.0f, 0.0f, 1.0f};
    Vec3f x_axis  = vec3_normalize(vec3_cross(y_axis, z_world));
    if (vec3_dot(x_axis, x_axis) < 1e-8f) {

        Vec3f x_world = {1.0f, 0.0f, 0.0f};
        x_axis = vec3_normalize(vec3_cross(y_axis, x_world));
    }
    Vec3f z_axis = vec3_cross(x_axis, y_axis);

    float *m = b->bind_matrix.m;
    m[ 0]=x_axis.x; m[ 1]=x_axis.y; m[ 2]=x_axis.z; m[ 3]=0.0f;
    m[ 4]=y_axis.x; m[ 5]=y_axis.y; m[ 6]=y_axis.z; m[ 7]=0.0f;
    m[ 8]=z_axis.x; m[ 9]=z_axis.y; m[10]=z_axis.z; m[11]=0.0f;
    m[12]=head.x;   m[13]=head.y;   m[14]=head.z;   m[15]=1.0f;

    memcpy(b->pose_matrix.m, m, 16*sizeof(float));
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: bone_build_bind -> rigpub_rig_face_engine_bone_build_bind */
int (*rigpub_rig_face_engine_bone_build_bind)(RigFaceBone *b, Vec3f head, Vec3f tail) = bone_build_bind;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


int rig_face_bind_skeleton(RigFaceMesh *mesh){
    const RigFaceParams *p = &mesh->params;
    float sx = p->cranium_width  * 0.5f;
    (void)sx;

    BoneDef defs[RIG_FACE_BONES];
    bones_from_params(p, defs);

    for (int i = 0; i < RIG_FACE_BONES; i++) {
        RigFaceBone *b = &mesh->bones[i];
        strncpy(b->name, defs[i].name, 31);
        b->name[31] = '\0';
        b->parent   = defs[i].parent;
        Vec3f head  = {defs[i].hx, defs[i].hy, defs[i].hz};
        Vec3f tail  = {defs[i].tx, defs[i].ty, defs[i].tz};
        bone_build_bind(b, head, tail);
    }

    for (uint32_t vi = 0; vi < mesh->n_verts; vi++) {
        Vec3f v = mesh->verts[vi].pos;

        float  best_d[4] = {1e30f, 1e30f, 1e30f, 1e30f};
        uint8_t best_b[4] = {0, 0, 0, 0};
        for (int bi = 0; bi < RIG_FACE_BONES; bi++) {
            float d = vec3_dist(v, mesh->bones[bi].head);

            for (int k = 0; k < 4; k++) {
                if (d < best_d[k]) {
                    for (int j = 3; j > k; j--) {
                        best_d[j] = best_d[j-1];
                        best_b[j] = best_b[j-1];
                    }
                    best_d[k] = d;
                    best_b[k] = (uint8_t)bi;
                    break;
                }
            }
        }

        float sum = 0.0f;
        float w4[4];
        for (int k = 0; k < 4; k++) {
            w4[k] = (best_d[k] > 1e-8f) ? 1.0f / best_d[k] : 1e6f;
            sum += w4[k];
        }
        if (sum < 1e-8f) sum = 1.0f;
        mesh->verts[vi].weights = (Vec4f){
            w4[0]/sum, w4[1]/sum, w4[2]/sum, w4[3]/sum
        };
        mesh->verts[vi].bones[0] = best_b[0];
        mesh->verts[vi].bones[1] = best_b[1];
        mesh->verts[vi].bones[2] = best_b[2];
        mesh->verts[vi].bones[3] = best_b[3];
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

typedef struct { float qr[4]; float qd[4]; } DualQuat;

static DualQuat dq_from_mat4(const float m[16]) {

    float trace = m[0] + m[5] + m[10];
    float qr[4];
    if (trace > 0.0f) {
        float s = 0.5f / sqrtf(trace + 1.0f);
        qr[3] = 0.25f / s;
        qr[0] = (m[9] - m[6])  * s;
        qr[1] = (m[2] - m[8])  * s;
        qr[2] = (m[4] - m[1])  * s;
    } else if (m[0] > m[5] && m[0] > m[10]) {
        float s = 2.0f * sqrtf(1.0f + m[0] - m[5] - m[10]);
        qr[3] = (m[9] - m[6])  / s;
        qr[0] = 0.25f * s;
        qr[1] = (m[1] + m[4])  / s;
        qr[2] = (m[2] + m[8])  / s;
    } else if (m[5] > m[10]) {
        float s = 2.0f * sqrtf(1.0f + m[5] - m[0] - m[10]);
        qr[3] = (m[2] - m[8])  / s;
        qr[0] = (m[1] + m[4])  / s;
        qr[1] = 0.25f * s;
        qr[2] = (m[6] + m[9])  / s;
    } else {
        float s = 2.0f * sqrtf(1.0f + m[10] - m[0] - m[5]);
        qr[3] = (m[4] - m[1])  / s;
        qr[0] = (m[2] + m[8])  / s;
        qr[1] = (m[6] + m[9])  / s;
        qr[2] = 0.25f * s;
    }

    float len = sqrtf(qr[0]*qr[0]+qr[1]*qr[1]+qr[2]*qr[2]+qr[3]*qr[3]);
    if (len > 1e-8f) { qr[0]/=len; qr[1]/=len; qr[2]/=len; qr[3]/=len; }

    float tx = m[12], ty = m[13], tz = m[14];
    DualQuat dq;
    memcpy(dq.qr, qr, 16);

    dq.qd[0] =  0.5f*( tx*qr[3] + ty*qr[2] - tz*qr[1]);
    dq.qd[1] =  0.5f*(-tx*qr[2] + ty*qr[3] + tz*qr[0]);
    dq.qd[2] =  0.5f*( tx*qr[1] - ty*qr[0] + tz*qr[3]);
    dq.qd[3] = -0.5f*( tx*qr[0] + ty*qr[1] + tz*qr[2]);
    return dq;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: dq_from_mat4 -> rigpub_rig_face_engine_dq_from_mat4 */
DualQuat (*rigpub_rig_face_engine_dq_from_mat4)(const float m[16]) = dq_from_mat4;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static Vec3f dq_transform_point(const DualQuat *dq, Vec3f p) {

    const float *q = dq->qr;
    const float *e = dq->qd;

    float px = p.x, py = p.y, pz = p.z;
    float rx = q[3]*px + q[1]*pz - q[2]*py;
    float ry = q[3]*py + q[2]*px - q[0]*pz;
    float rz = q[3]*pz + q[0]*py - q[1]*px;
    float rw = -q[0]*px - q[1]*py - q[2]*pz;
    float vx = rx*q[3] - rw*q[0] + ry*q[2] - rz*q[1];

    float qx=q[0],qy=q[1],qz=q[2],qw=q[3];
    float tx2 = 2.0f*(e[3]*(-qx)+e[0]*qw+e[1]*(-qz)-e[2]*(-qy));
    float ty2 = 2.0f*(e[3]*(-qy)-e[0]*(-qz)+e[1]*qw+e[2]*(-qx));
    float tz2 = 2.0f*(e[3]*(-qz)+e[0]*(-qy)-e[1]*(-qx)+e[2]*qw);

    float cross_x = 2.0f*(qy*pz - qz*py);
    float cross_y = 2.0f*(qz*px - qx*pz);
    float cross_z = 2.0f*(qx*py - qy*px);
    Vec3f out = {
        px + qw*cross_x + qy*cross_z - qz*cross_y + tx2,
        py + qw*cross_y + qz*cross_x - qx*cross_z + ty2,
        pz + qw*cross_z + qx*cross_y - qy*cross_x + tz2
    };
    (void)vx; (void)rx; (void)ry; (void)rz; (void)rw;
    return out;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: dq_transform_point -> rigpub_rig_face_engine_dq_transform_point */
Vec3f (*rigpub_rig_face_engine_dq_transform_point)(const DualQuat *dq, Vec3f p) = dq_transform_point;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


int rig_face_skin_dual_quaternion(RigFaceMesh *mesh){

    DualQuat bone_dq[RIG_FACE_BONES];
    for (int i = 0; i < RIG_FACE_BONES; i++) {
        bone_dq[i] = dq_from_mat4(mesh->bones[i].pose_matrix.m);
    }

    for (uint32_t vi = 0; vi < mesh->n_verts; vi++) {
        Vec4f  w = mesh->verts[vi].weights;
        float  bw[4] = {w.x, w.y, w.z, w.w};
        uint8_t bi[4];
        memcpy(bi, mesh->verts[vi].bones, 4);

        float bqr[4] = {0,0,0,0};
        float bqd[4] = {0,0,0,0};

        for (int k = 0; k < 4; k++) {
            if (bw[k] < 1e-6f) continue;
            const DualQuat *dq = &bone_dq[bi[k]];
            float sign = 1.0f;
            if (k > 0) {

                float dot = dq->qr[0]*bqr[0]+dq->qr[1]*bqr[1]+
                            dq->qr[2]*bqr[2]+dq->qr[3]*bqr[3];
                if (dot < 0.0f) sign = -1.0f;
            }
            for (int j = 0; j < 4; j++) {
                bqr[j] += sign * bw[k] * dq->qr[j];
                bqd[j] += sign * bw[k] * dq->qd[j];
            }
        }

        float len = sqrtf(bqr[0]*bqr[0]+bqr[1]*bqr[1]+bqr[2]*bqr[2]+bqr[3]*bqr[3]);
        if (len < 1e-8f) continue;
        float inv = 1.0f / len;
        DualQuat blended;
        blended.qr[0]=bqr[0]*inv; blended.qr[1]=bqr[1]*inv;
        blended.qr[2]=bqr[2]*inv; blended.qr[3]=bqr[3]*inv;
        blended.qd[0]=bqd[0]*inv; blended.qd[1]=bqd[1]*inv;
        blended.qd[2]=bqd[2]*inv; blended.qd[3]=bqd[3]*inv;

        mesh->verts[vi].pos    = dq_transform_point(&blended, mesh->verts[vi].pos);
        mesh->verts[vi].normal = vec3_normalize(
            dq_transform_point(&blended, mesh->verts[vi].normal));
    }
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

static  int raster_tri(const Vec3f *p0, const Vec3f *p1, const Vec3f *p2,
                        const Vec2f *uv0, const Vec2f *uv1, const Vec2f *uv2,
                        const Vec3f *n0,  const Vec3f *n1,  const Vec3f *n2,
                        uint8_t *out_rgb, uint32_t w, uint32_t h, int mode){

    int u_min = (int)(fminf(uv0->u, fminf(uv1->u, uv2->u)) * w);
    int u_max = (int)(fmaxf(uv0->u, fmaxf(uv1->u, uv2->u)) * w) + 1;
    int v_min = (int)(fminf(uv0->v, fminf(uv1->v, uv2->v)) * h);
    int v_max = (int)(fmaxf(uv0->v, fmaxf(uv1->v, uv2->v)) * h) + 1;

    u_min = (u_min < 0) ? 0 : (u_min >= (int)w ? (int)w-1 : u_min);
    u_max = (u_max < 0) ? 0 : (u_max >= (int)w ? (int)w-1 : u_max);
    v_min = (v_min < 0) ? 0 : (v_min >= (int)h ? (int)h-1 : v_min);
    v_max = (v_max < 0) ? 0 : (v_max >= (int)h ? (int)h-1 : v_max);

    for (int vy = v_min; vy <= v_max; vy++) {
        for (int ux = u_min; ux <= u_max; ux++) {
            float fu = (ux + 0.5f) / w;
            float fv = (vy + 0.5f) / h;

            float du1 = uv1->u - uv0->u, dv1 = uv1->v - uv0->v;
            float du2 = uv2->u - uv0->u, dv2 = uv2->v - uv0->v;
            float dpu = fu - uv0->u,     dpv = fv - uv0->v;
            float det = du1*dv2 - du2*dv1;
            if (fabsf(det) < 1e-10f) continue;
            float inv = 1.0f / det;
            float b1  = (dpu*dv2 - du2*dpv) * inv;
            float b2  = (du1*dpv - dpu*dv1) * inv;
            float b0  = 1.0f - b1 - b2;
            if (b0 < 0 || b1 < 0 || b2 < 0) continue;

            Vec3f n = vec3_normalize((Vec3f){
                b0*n0->x + b1*n1->x + b2*n2->x,
                b0*n0->y + b1*n1->y + b2*n2->y,
                b0*n0->z + b1*n1->z + b2*n2->z
            });

            uint32_t px_idx = (vy * w + ux) * 3;
            if (mode == 0) {

                out_rgb[px_idx+0] = (uint8_t)((n.x*0.5f+0.5f)*255.0f);
                out_rgb[px_idx+1] = (uint8_t)((n.y*0.5f+0.5f)*255.0f);
                out_rgb[px_idx+2] = (uint8_t)((n.z*0.5f+0.5f)*255.0f);
            } else {

                float thick_b0 = (n0->z * 0.5f + 0.5f);
                float thick = b0*thick_b0 + b1*(n1->z*0.5f+0.5f) + b2*(n2->z*0.5f+0.5f);

                out_rgb[px_idx+0] = (uint8_t)(thick * 200.0f + 55.0f);

                out_rgb[px_idx+1] = (uint8_t)(thick * 150.0f + 55.0f);

                out_rgb[px_idx+2] = (uint8_t)(thick * 100.0f + 55.0f);
            }
        }
    }
    (void)p0; (void)p1; (void)p2;
    return 0; /* 0=hook procesado, -1=contexto inválido */
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: raster_tri -> rigpub_rig_face_engine_raster_tri */
int (*rigpub_rig_face_engine_raster_tri)(const Vec3f *p0, const Vec3f *p1, const Vec3f *p2, const Vec2f *uv0, const Vec2f *uv1, const Vec2f *uv2, const Vec3f *n0, const Vec3f *n1, const Vec3f *n2, uint8_t *out_rgb, uint32_t w, uint32_t h, int mode) = raster_tri;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


int rig_face_generate_normal_map(const RigFaceMesh *mesh,
                                   uint8_t *out_rgb, uint32_t w, uint32_t h){
    if (!mesh || !mesh->verts || !mesh->tris || !out_rgb || w == 0 || h == 0) return -1;

    for (uint32_t i = 0; i < w * h * 3; i += 3) {
        out_rgb[i+0] = 128; out_rgb[i+1] = 128; out_rgb[i+2] = 255;
    }
    for (uint32_t t = 0; t < mesh->n_tris; t++) {
        uint32_t ai = mesh->tris[t].a;
        uint32_t bi = mesh->tris[t].b;
        uint32_t ci = mesh->tris[t].c;
        raster_tri(
            &mesh->verts[ai].pos, &mesh->verts[bi].pos, &mesh->verts[ci].pos,
            &mesh->verts[ai].uv,  &mesh->verts[bi].uv,  &mesh->verts[ci].uv,
            &mesh->verts[ai].normal, &mesh->verts[bi].normal, &mesh->verts[ci].normal,
            out_rgb, w, h, 0);
    }
    return 0;
}

int rig_face_generate_sss_map(const RigFaceMesh *mesh,
                                uint8_t *out_rgb, uint32_t w, uint32_t h){
    if (!mesh || !mesh->verts || !mesh->tris || !out_rgb || w == 0 || h == 0) return -1;
    memset(out_rgb, 80, w * h * 3);
    for (uint32_t t = 0; t < mesh->n_tris; t++) {
        uint32_t ai = mesh->tris[t].a;
        uint32_t bi = mesh->tris[t].b;
        uint32_t ci = mesh->tris[t].c;

        Vec3f n_a = {mesh->verts[ai].color.z*2.0f-1.0f, 0, 0};
        Vec3f n_b = {mesh->verts[bi].color.z*2.0f-1.0f, 0, 0};
        Vec3f n_c = {mesh->verts[ci].color.z*2.0f-1.0f, 0, 0};
        raster_tri(
            &mesh->verts[ai].pos, &mesh->verts[bi].pos, &mesh->verts[ci].pos,
            &mesh->verts[ai].uv,  &mesh->verts[bi].uv,  &mesh->verts[ci].uv,
            &n_a, &n_b, &n_c,
            out_rgb, w, h, 1);
    }
    return 0;
}

#ifdef RIG_FACE_MAIN

int main(int argc, char **argv) {
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║  RigCom v24 THE SANTORIUM OF COMPILER — Face Engine — ARM64 φ-Mesh Builder ║\n");
    printf("╚══════════════════════════════════════════════════╝\n\n");

    RigFaceParams p = rig_face_default_params();

    if (argc > 1) p.melanin       = atof(argv[1]);
    if (argc > 2) p.gender_factor = atof(argv[2]);
    if (argc > 3) p.age_factor    = atof(argv[3]);
    uint32_t subdiv = (argc > 4) ? (uint32_t)atoi(argv[4]) : 4;

    printf("[1/8] Construyendo malla base (icosfera)...\n");
    printf("[2/8] Subdivisión nivel %u...\n", subdiv);
    printf("[3/8] Deformación craneal φ-paramétrica...\n");
    printf("[4/8] Escultura de rasgos (FACS landmarks)...\n");
    printf("[5/8] Normales, tangentes, UV...\n");
    printf("[6/8] Material PBR (melanina=%.2f hemo=%.2f)...\n\n",
           p.melanin, p.hemoglobin);

    RigFaceMesh *face = rig_face_create(&p, subdiv);
    if (!face) {
        fprintf(stderr, "ERROR: Arena overflow — reducir subdiv_level\n");
        return 1;
    }

    printf("[7/8] Esqueleto + FACS shapes...\n");
    rig_face_bind_skeleton(face);
    rig_face_build_facs_shapes(face);
    rig_face_build_neck(face, &p);
    rig_face_compute_smooth_normals(face);

    printf("[8/8] Exportando...\n\n");

    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Vértices   : %7u\n", face->n_verts);
    printf("Triángulos : %7u\n", face->n_tris);
    printf("Subdivisión: %7u\n", face->subdivision_level);
    printf("Huesos     : %7d\n", RIG_FACE_BONES);
    printf("Blend shapes:%6d\n", RIG_FACE_BLEND_SHAPES);
    printf("φ-error    : %7.4f%%  (%.6f)\n",
           face->phi_error * 100.0f, face->phi_error);
    printf("AABB min   : (%.3f, %.3f, %.3f)\n",
           face->aabb_min.x, face->aabb_min.y, face->aabb_min.z);
    printf("AABB max   : (%.3f, %.3f, %.3f)\n",
           face->aabb_max.x, face->aabb_max.y, face->aabb_max.z);
    printf("Melanina   : %.2f   Hemoglobina: %.2f\n",
           p.melanin, p.hemoglobin);
    printf("Color base : (%.3f, %.3f, %.3f)\n",
           face->material.base_color[0],
           face->material.base_color[1],
           face->material.base_color[2]);
    printf("SSS radios : R=%.2fmm G=%.2fmm B=%.2fmm\n",
           face->material.sss_radius[0],
           face->material.sss_radius[1],
           face->material.sss_radius[2]);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");

    const char *obj_path = "/data/data/com.termux/files/home/rig_face_output.obj";
    if (rig_face_export_obj(face, obj_path) == 0)
        printf("OBJ exportado: %s\n", obj_path);

    char glsl_buf[2048];
    rig_face_export_glsl_uniforms(face, glsl_buf, sizeof(glsl_buf));
    printf("\n%s\n", glsl_buf);

    uint32_t nf = face->n_verts * 15;
    uint32_t ni = face->n_tris * 3;
    printf("VBO size : %u floats = %.1f KB\n", nf, nf*4.0f/1024.0f);
    printf("IBO size : %u indices = %.1f KB\n", ni, ni*4.0f/1024.0f);
    printf("Total GPU: %.1f KB\n\n", (nf*4.0f + ni*4.0f) / 1024.0f);

    rig_face_destroy(face);
    printf("Arena liberada. Build: OK\n");
    return 0;
}

#endif

#ifdef RIG_FACE_MAIN_LEGACY
int main(int argc, char **argv) {
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║  RigCom v24 THE SANTORIUM OF COMPILER — Face Engine — ARM64 φ-Mesh Builder ║\n");
    printf("╚══════════════════════════════════════════════════╝\n\n");

    RigFaceParams p = rig_face_default_params();

    if (argc > 1) p.melanin       = atof(argv[1]);
    if (argc > 2) p.gender_factor = atof(argv[2]);
    if (argc > 3) p.age_factor    = atof(argv[3]);
    uint32_t subdiv = (argc > 4) ? (uint32_t)atoi(argv[4]) : 4;

    printf("[1/6] Construyendo malla base (icosfera)...\n");
    printf("[2/6] Subdivisión nivel %u (Catmull-Clark)...\n", subdiv);
    printf("[3/6] Deformación craneal φ-paramétrica...\n");
    printf("[4/6] Escultura de rasgos (FACS landmarks)...\n");
    printf("[5/6] Normales, tangentes, UV...\n");
    printf("[6/6] Material PBR (melanina=%.2f hemo=%.2f)...\n\n",
           p.melanin, p.hemoglobin);

    RigFaceMesh *face = rig_face_create(&p, subdiv);
    if (!face) {
        fprintf(stderr, "ERROR: Arena overflow — reducir subdiv_level\n");
        return 1;
    }

    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Vértices   : %7u\n", face->n_verts);
    printf("Triángulos : %7u\n", face->n_tris);
    printf("Subdivisión: %7u\n", face->subdivision_level);
    printf("φ-error    : %7.4f%%  (%.6f)\n",
           face->phi_error * 100.0f, face->phi_error);
    printf("AABB min   : (%.3f, %.3f, %.3f)\n",
           face->aabb_min.x, face->aabb_min.y, face->aabb_min.z);
    printf("AABB max   : (%.3f, %.3f, %.3f)\n",
           face->aabb_max.x, face->aabb_max.y, face->aabb_max.z);
    printf("Melanina   : %.2f   Hemoglobina: %.2f\n",
           p.melanin, p.hemoglobin);
    printf("Color base : (%.3f, %.3f, %.3f)\n",
           face->material.base_color[0],
           face->material.base_color[1],
           face->material.base_color[2]);
    printf("SSS radios : R=%.2fmm G=%.2fmm B=%.2fmm\n",
           face->material.sss_radius[0],
           face->material.sss_radius[1],
           face->material.sss_radius[2]);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");

    const char *obj_path = "/data/data/com.termux/files/home/rig_face_output.obj";
    if (rig_face_export_obj(face, obj_path) == 0) {
        printf("OBJ exportado: %s\n", obj_path);
    }

    char glsl_buf[2048];
    rig_face_export_glsl_uniforms(face, glsl_buf, sizeof(glsl_buf));
    printf("\n%s\n", glsl_buf);

    uint32_t nf = face->n_verts * FLOATS_PER_VERT;
    uint32_t ni = face->n_tris * 3;
    printf("VBO size : %u floats = %.1f KB\n", nf, nf*4.0f/1024.0f);
    printf("IBO size : %u indices = %.1f KB\n", ni, ni*4.0f/1024.0f);
    printf("Total GPU: %.1f KB\n\n", (nf*4.0f + ni*4.0f) / 1024.0f);

    rig_face_destroy(face);
    printf("Arena liberada. Build: OK\n");
    return 0;
}

#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * rig_face_project_to_skull — proyección cráneo-facial soberana
 * Autor: Richard Felipe Urbina
 *
 * Mapea cada vértice de la malla esférica base a la superficie de un
 * cráneo analítico definido por parámetros φ-proporcionales.
 *
 * Geometría craneal usada:
 *   Elipsoide craneano : a = skull_w/2, b = skull_h/2, c = skull_d/2
 *   Región facial      : parábola de revolución frontal
 *   Depresiones orales/nasales: desplazamiento radial con suavizado φ
 *
 * El vértice (en esfera unidad normalizada) se proyecta radialmente
 * sobre la superficie craneal: v_skull = dir * r_skull(theta, phi)
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rigdeps/math.h"

/* Radio de la superficie craneal en dirección (sin_t, cos_t, sin_p, cos_p) */
static float _skull_radius(const RigFaceParams *p, float ux, float uy, float uz) {
    /* Parámetros del cráneo en cm */
    float skull_w = (p->zygomatic_width  > 0) ? p->zygomatic_width * 0.5f : 7.5f;
    float skull_h = 9.5f;   /* altura craneal media en cm */
    float skull_d = 9.0f;   /* profundidad A-P en cm */

    /* Elipsoide: 1/a² x² + 1/b² y² + 1/c² z² = 1
       Intersección con rayo (ux,uy,uz): t = 1/sqrt(ux²/a² + uy²/b² + uz²/c²) */
    float inv_a = 1.0f / (skull_w > 0 ? skull_w : 1.0f);
    float inv_b = 1.0f / (skull_h > 0 ? skull_h : 1.0f);
    float inv_c = 1.0f / (skull_d > 0 ? skull_d : 1.0f);
    float denom = sqrtf(
        ux * ux * inv_a * inv_a +
        uy * uy * inv_b * inv_b +
        uz * uz * inv_c * inv_c
    );
    float r_ellipsoid = (denom > 1e-6f) ? (1.0f / denom) : skull_w;

    /* Deformación frontal: la cara es ligeramente más plana que la nuca */
    float front_bias = 1.0f - 0.08f * fmaxf(0.0f, -uz);   /* uz < 0 = anterior */

    /* Protuberancia occipital (posterior) */
    float occip = 1.0f + 0.05f * fmaxf(0.0f, uz) * fmaxf(0.0f, -uy);

    /* Depresión temporal */
    float temp_x = fabsf(ux);
    float temp_y = uy + 0.2f;    /* a ~1/5 de la altura */
    float temporal = 1.0f - 0.06f * expf(-(temp_x * temp_x + temp_y * temp_y) * 8.0f);

    return r_ellipsoid * front_bias * occip * temporal;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: _skull_radius -> rigpub_rig_face_engine__skull_radius */
float (*rigpub_rig_face_engine__skull_radius)(const RigFaceParams *p, float ux, float uy, float uz) = _skull_radius;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


int rig_face_project_to_skull(RigFaceMesh *mesh){
    if (!mesh || !mesh->verts || mesh->n_verts == 0) return -1;

    const RigFaceParams *p = &mesh->params;

    /* Centro de masa del cráneo (ligeramente superior al origen) */
    Vec3f center = { 0.0f, 0.3f, 0.0f };

    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        RigFaceVertex *v = &mesh->verts[i];

        /* Dirección del vértice desde el centro craneal */
        Vec3f d = {
            v->pos.x - center.x,
            v->pos.y - center.y,
            v->pos.z - center.z
        };
        float len = sqrtf(d.x*d.x + d.y*d.y + d.z*d.z);
        if (len < 1e-6f) continue;

        float ux = d.x / len;
        float uy = d.y / len;
        float uz = d.z / len;

        float r = _skull_radius(p, ux, uy, uz);

        /* Proyecto el vértice a la superficie craneal */
        v->pos.x = center.x + ux * r;
        v->pos.y = center.y + uy * r;
        v->pos.z = center.z + uz * r;

        /* Normal exterior = dirección radial (normalizada) */
        v->normal.x = ux;
        v->normal.y = uy;
        v->normal.z = uz;
    }

    /* Actualizar AABB */
    mesh->aabb_min = mesh->verts[0].pos;
    mesh->aabb_max = mesh->verts[0].pos;
    for (uint32_t i = 1; i < mesh->n_verts; i++) {
        Vec3f p3 = mesh->verts[i].pos;
        if (p3.x < mesh->aabb_min.x) mesh->aabb_min.x = p3.x;
        if (p3.y < mesh->aabb_min.y) mesh->aabb_min.y = p3.y;
        if (p3.z < mesh->aabb_min.z) mesh->aabb_min.z = p3.z;
        if (p3.x > mesh->aabb_max.x) mesh->aabb_max.x = p3.x;
        if (p3.y > mesh->aabb_max.y) mesh->aabb_max.y = p3.y;
        if (p3.z > mesh->aabb_max.z) mesh->aabb_max.z = p3.z;
    }

    /* Recalcular error φ */
    mesh->phi_error = 0.0f;
    for (uint32_t i = 0; i < mesh->n_verts; i++) {
        Vec3f pv = mesh->verts[i].pos;
        float r2 = sqrtf(pv.x*pv.x + pv.y*pv.y + pv.z*pv.z);
        float expected = _skull_radius(p,
            (r2 > 1e-6f ? pv.x/r2 : 0.0f),
            (r2 > 1e-6f ? pv.y/r2 : 0.0f),
            (r2 > 1e-6f ? pv.z/r2 : 0.0f));
        float err = fabsf(r2 - expected);
        if (err > mesh->phi_error) mesh->phi_error = err;
    }
}
