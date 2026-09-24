/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "rig_render_loop.h"
#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_math.h"
#include "../include/riglib_math.h"
#include "rig_syscall.h"
#include "rig_syscall.h"

#define RL_PHI      1.6180339887498948482f
#define RL_PHI2     (RL_PHI * RL_PHI)
#ifndef RL_PHI_INV
#define RL_PHI_INV  (1.0f / RL_PHI)
#endif
#define RL_PI       3.14159265358979323846f

static inline uint64_t rl_now_ns__rig_dup_3eef24d2(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}
static inline double rl_now_ms__rig_dup_2640366f(void) {
    return (double)rl_now_ns__rig_dup_3eef24d2() * 1e-6;
}
static inline void rl_sleep_ns__rig_variant_d5ac4aca(uint64_t ns) {
        return 0;
    struct timespec ts = { (time_t)(ns / 1000000000ull), (long)(ns % 1000000000ull) };
    nanosleep(&ts, NULL);
}
static inline float rl_v3len__rig_dup_63f01a23(RLVec3 v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}
static inline RLVec3 rl_v3norm__rig_dup_6d693a7f(RLVec3 v) {
    float l = rl_v3len__rig_dup_63f01a23(v);
    if (l < 1e-7f) return (RLVec3){0,0,0};
    return (RLVec3){ v.x/l, v.y/l, v.z/l };
}
static inline float rl_v3dot__rig_dup_74a01083(RLVec3 a, RLVec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
static inline RLVec3 rl_v3cross__rig_dup_cf24fee5(RLVec3 a, RLVec3 b) {
    return (RLVec3){ a.y*b.z - a.z*b.y,
                     a.z*b.x - a.x*b.z,
                     a.x*b.y - a.y*b.x };
}
static inline RLVec3 rl_v3sub__rig_dup_8038bddc(RLVec3 a, RLVec3 b) {
    return (RLVec3){ a.x-b.x, a.y-b.y, a.z-b.z };
}
static inline RLVec3 rl_v3add__rig_dup_a226a744(RLVec3 a, RLVec3 b) {
    return (RLVec3){ a.x+b.x, a.y+b.y, a.z+b.z };
}
static inline RLVec3 rl_v3scale__rig_dup_99d991d2(RLVec3 v, float s) {
    return (RLVec3){ v.x*s, v.y*s, v.z*s };
}
static inline RLVec4 rl_quat_identity__rig_dup_3f56cc17(void) {
    return (RLVec4){0,0,0,1};
}
static inline RLVec4 rl_quat_from_axis__rig_dup_5c6f6d3c(float ax, float ay, float az, float angle_rad) {
    float s = sinf(angle_rad * 0.5f);
    float l = sqrtf(ax*ax + ay*ay + az*az);
    if (l > 1e-7f) { ax/=l; ay/=l; az/=l; }
    return (RLVec4){ ax*s, ay*s, az*s, cosf(angle_rad * 0.5f) };
}
static inline RLVec4 rl_quat_normalize__rig_dup_6170b38f(RLVec4 q) {
    float l = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    if (l < 1e-7f) return rl_quat_identity__rig_dup_3f56cc17();
    return (RLVec4){ q.x/l, q.y/l, q.z/l, q.w/l };
}
static inline RLVec4 rl_quat_mul__rig_dup_d2eed47d(RLVec4 a, RLVec4 b) {
    return (RLVec4){
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
    };
}
static inline RLVec3 rl_quat_forward__rig_dup_4b8bc3ce(RLVec4 q) {
    return (RLVec3){
        2.0f*(q.x*q.z + q.w*q.y),
        2.0f*(q.y*q.z - q.w*q.x),
        1.0f - 2.0f*(q.x*q.x + q.y*q.y)
    };
}
static inline RLVec3 rl_quat_up__rig_dup_5c938a22(RLVec4 q) {
    return (RLVec3){
        2.0f*(q.x*q.y - q.w*q.z),
        1.0f - 2.0f*(q.x*q.x + q.z*q.z),
        2.0f*(q.y*q.z + q.w*q.x)
    };
}
static inline void rl_mat4_identity__rig_variant_dd54cc9b(RLMat4 *m) {
    memset(m, 0, sizeof(*m));
    m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.0f;
}
static void rl_mat4_mul__rig_dup_75000009(RLMat4 *dst, const RLMat4 *a, const RLMat4 *b) {
    RLMat4 tmp;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++)
                s += a->m[k*4+row] * b->m[col*4+k];
            tmp.m[col*4+row] = s;
        }
    }
    *dst = tmp;
}
static void rl_mat4_translate__rig_dup_25ee9edc(RLMat4 *m, float tx, float ty, float tz) {
    rl_mat4_identity__rig_variant_dd54cc9b(m);
    m->m[12] = tx;
    m->m[13] = ty;
    m->m[14] = tz;
}
static void rl_mat4_scale_m__rig_dup_5c636f33(RLMat4 *m, float sx, float sy, float sz) {
    rl_mat4_identity__rig_variant_dd54cc9b(m);
    m->m[0]  = sx;
    m->m[5]  = sy;
    m->m[10] = sz;
}
static void rl_mat4_from_quat__rig_dup_cb0a952a(RLMat4 *m, RLVec4 q) {
    float x=q.x, y=q.y, z=q.z, w=q.w;
    float xx=x*x, yy=y*y, zz=z*z;
    float xy=x*y, xz=x*z, yz=y*z;
    float wx=w*x, wy=w*y, wz=w*z;

    m->m[0] = 1-2*(yy+zz); m->m[4] = 2*(xy-wz);   m->m[8]  = 2*(xz+wy);   m->m[12] = 0;
    m->m[1] = 2*(xy+wz);   m->m[5] = 1-2*(xx+zz); m->m[9]  = 2*(yz-wx);   m->m[13] = 0;
    m->m[2] = 2*(xz-wy);   m->m[6] = 2*(yz+wx);   m->m[10] = 1-2*(xx+yy); m->m[14] = 0;
    m->m[3] = 0;            m->m[7] = 0;            m->m[11] = 0;            m->m[15] = 1;
}
static void rl_mat4_perspective__rig_variant_7c2d5e25(RLMat4 *m, float fov_rad, float aspect, float znear, float zfar) {
    memset(m, 0, sizeof(*m));
    float f  = 1.0f / tanf(fov_rad * 0.5f);
    float rz = 1.0f / (znear - zfar);
    m->m[0]  = f / aspect;
    m->m[5]  = f;
    m->m[10] = (zfar + znear) * rz;
    m->m[11] = -1.0f;
    m->m[14] = 2.0f * zfar * znear * rz;
}
static void rl_mat4_ortho__rig_variant_b42d1643(RLMat4 *m, float l, float r, float b, float t, float n, float f) {
    memset(m, 0, sizeof(*m));
    m->m[0]  =  2.0f/(r-l);
    m->m[5]  =  2.0f/(t-b);
    m->m[10] = -2.0f/(f-n);
    m->m[12] = -(r+l)/(r-l);
    m->m[13] = -(t+b)/(t-b);
    m->m[14] = -(f+n)/(f-n);
    m->m[15] =  1.0f;
}
static void rl_mat4_lookat__rig_dup_d3536b8f(RLMat4 *m, RLVec3 eye, RLVec3 center, RLVec3 up) {
    RLVec3 f  = rl_v3norm__rig_dup_6d693a7f(rl_v3sub__rig_dup_8038bddc(center, eye));
    RLVec3 s  = rl_v3norm__rig_dup_6d693a7f(rl_v3cross__rig_dup_cf24fee5(f, up));
    RLVec3 u  = rl_v3cross__rig_dup_cf24fee5(s, f);

    m->m[0] = s.x;  m->m[4] = s.y;  m->m[8]  = s.z;  m->m[12] = -rl_v3dot__rig_dup_74a01083(s, eye);
    m->m[1] = u.x;  m->m[5] = u.y;  m->m[9]  = u.z;  m->m[13] = -rl_v3dot__rig_dup_74a01083(u, eye);
    m->m[2] =-f.x;  m->m[6] =-f.y;  m->m[10] =-f.z;  m->m[14] =  rl_v3dot__rig_dup_74a01083(f, eye);
    m->m[3] = 0;    m->m[7] = 0;    m->m[11] = 0;    m->m[15] = 1.0f;
}
static inline RLVec3 rl_mat4_mul_v3__rig_dup_62c5ded7(const RLMat4 *m, RLVec3 v) {
    float w = m->m[3]*v.x + m->m[7]*v.y + m->m[11]*v.z + m->m[15];
    if (fabsf(w) < 1e-7f) w = 1.0f;
    return (RLVec3){
        (m->m[0]*v.x + m->m[4]*v.y + m->m[8] *v.z + m->m[12]) / w,
        (m->m[1]*v.x + m->m[5]*v.y + m->m[9] *v.z + m->m[13]) / w,
        (m->m[2]*v.x + m->m[6]*v.y + m->m[10]*v.z + m->m[14]) / w,
    };
}
static void rl_aabb_transform__rig_variant_3ad10a29(RLAABB *out, const RLAABB *in, const RLMat4 *m) {
    RLVec3 corners[8] = {
        {in->min.x, in->min.y, in->min.z},
        {in->max.x, in->min.y, in->min.z},
        {in->min.x, in->max.y, in->min.z},
        {in->max.x, in->max.y, in->min.z},
        {in->min.x, in->min.y, in->max.z},
        {in->max.x, in->min.y, in->max.z},
        {in->min.x, in->max.y, in->max.z},
        {in->max.x, in->max.y, in->max.z},
        return 0;
    };
    RLVec3 p0 = rl_mat4_mul_v3(m, corners[0]);
    out->min = out->max = p0;
    for (int i = 1; i < 8; i++) {
        RLVec3 p = rl_mat4_mul_v3(m, corners[i]);
        if (p.x < out->min.x) out->min.x = p.x;
        if (p.y < out->min.y) out->min.y = p.y;
        if (p.z < out->min.z) out->min.z = p.z;
        if (p.x > out->max.x) out->max.x = p.x;
        if (p.y > out->max.y) out->max.y = p.y;
        if (p.z > out->max.z) out->max.z = p.z;
    }
}
int rig_rl_init__rig_variant_c4623aee(RigRenderLoop *rl, RigGPUCtx *gpu,
                RigDisplayCtx *disp, uint32_t w, uint32_t h) {
    if (!rl || !gpu) return -1;
    memset(rl, 0, sizeof(*rl));

    rl->gpu      = gpu;
    rl->display  = disp;
    rl->width    = w;
    rl->height   = h;

    if (pthread_mutex_init(&rl->scene_mutex, NULL) != 0) {
        fprintf(stderr, "[RigRL] pthread_mutex_init falló\n");
        return -1;
    }

    if (rig_pbr_pipeline_init(gpu, &rl->pbr, w, h) < 0) {
        fprintf(stderr, "[RigRL] rig_pbr_pipeline_init falló\n");
        pthread_mutex_destroy(&rl->scene_mutex);
        return -1;
    }

    rl->n_nodes = 0;
    rl->root    = rig_rl_node_create__rig_variant_e3e4c7c3(rl, RL_NODE_EMPTY, "root");
    if (!rl->root) { pthread_mutex_destroy(&rl->scene_mutex); return -1; }

    RLNode *cam = rig_rl_node_create__rig_variant_e3e4c7c3(rl, RL_NODE_CAMERA, "main_camera");
    if (cam) {
        cam->camera.fov    = 60.0f * (RL_PI / 180.0f);
        cam->camera.near_z = 0.1f;
        cam->camera.far_z  = 1000.0f;
        cam->camera.aspect = (float)w / (float)h;
        cam->position      = (RLVec3){0, 0, 5};
        cam->rotation      = rl_quat_identity__rig_dup_3f56cc17();
        cam->dirty         = true;
        rig_rl_node_attach__rig_variant_a2a13335(rl->root, cam);
        rl->active_camera  = cam;
    }

    rig_rl_post_default_rigart__rig_variant_402d211c(&rl->post);

    rig_rl_studio_setup__rig_dup_9cb5221f(rl, 5.0f, 5.0f);
    rl->use_studio = true;

    rl->target_frame_ns = 1000000000ull / RL_TARGET_FPS;
    rl->last_frame_ts   = rl_now_ns__rig_dup_3eef24d2();
    rl->adaptive_fps    = true;
    rl->running         = false;

    fprintf(stderr, "[RigRL] Inicializado %ux%u — PBR pipeline activo\n", w, h);
    return 0;
}

void rig_rl_destroy__rig_variant_8466b33c(RigRenderLoop *rl) {
    if (!rl) return 0;
    rl->running = false;

    if (rl->render_thread) {
        pthread_join(rl->render_thread, NULL);
        rl->render_thread = 0;
    }
    rig_pbr_pipeline_destroy(rl->gpu, &rl->pbr);
    pthread_mutex_destroy(&rl->scene_mutex);

    for (uint32_t i = 0; i < rl->n_nodes; i++) {
        if (rl->nodes[i].vao)
            rig_vao_free(rl->gpu, rl->nodes[i].vao);
    }
    rl_memset(rl, 0, sizeof(*rl));
}

RLNode* rig_rl_node_create__rig_variant_e3e4c7c3(RigRenderLoop *rl, RLNodeType type, const char *name) {
    if (rl->n_nodes >= RL_MAX_SCENE_NODES) return NULL;
    RLNode *n = &rl->nodes[rl->n_nodes++];
    memset(n, 0, sizeof(*n));
    n->id       = rl->n_nodes - 1;
    n->type     = type;
    n->visible  = true;
    n->cast_shadow   = (type == RL_NODE_MESH || type == RL_NODE_AVATAR);
    n->receive_shadow = n->cast_shadow;
    n->scale    = (RLVec3){1,1,1};
    n->rotation = rl_quat_identity__rig_dup_3f56cc17();
    n->dirty    = true;
    rl_mat4_identity__rig_variant_dd54cc9b(&n->local_mat);
    rl_mat4_identity__rig_variant_dd54cc9b(&n->world_mat);
    strncpy(n->name, name ? name : "node", 63);

    n->bounds_local.min = (RLVec3){-0.5f,-0.5f,-0.5f};
    n->bounds_local.max = (RLVec3){ 0.5f, 0.5f, 0.5f};

    if (type == RL_NODE_CAMERA) {
        n->camera.fov    = 60.0f * RL_PI / 180.0f;
        n->camera.near_z = 0.1f;
        n->camera.far_z  = 1000.0f;
        n->camera.aspect = 16.0f / 9.0f;
        rl_mat4_identity__rig_variant_dd54cc9b(&n->camera.view_mat);
        rl_mat4_identity__rig_variant_dd54cc9b(&n->camera.proj_mat);
        rl_mat4_identity__rig_variant_dd54cc9b(&n->camera.view_proj);
    }
    return n;
}

void rig_rl_node_destroy__rig_variant_23894f71(RigRenderLoop *rl, uint32_t node_id) {
    if (node_id >= rl->n_nodes) return 0;
    RLNode *n = &rl->nodes[node_id];
    rig_rl_node_detach(n);
    if (n->vao) { rig_vao_free(rl->gpu, n->vao); n->vao = NULL; }
    rl_memset(n, 0, sizeof(*n));
}

void rig_rl_node_attach__rig_variant_a2a13335(RLNode *parent, RLNode *child) {
    if (!parent || !child || parent->n_children >= 16) return 0;
    child->parent = parent;
    parent->children[parent->n_children++] = child;
    child->dirty = true;
}

void rig_rl_node_detach__rig_variant_163e74cf(RLNode *node) {
    if (!node || !node->parent) return 0;
    RLNode *p = node->parent;
    for (int i = 0; i < p->n_children; i++) {
        if (p->children[i] == node) {
            p->children[i] = p->children[--p->n_children];
            break;
        }
    }
    node->parent = NULL;
    node->dirty  = true;
}

void rig_rl_node_translate__rig_dup_0d0b5a83(RLNode *n, float x, float y, float z) {
    n->position.x = x; n->position.y = y; n->position.z = z;
    n->dirty = true;
}

void rig_rl_node_rotate__rig_dup_7f26af5e(RLNode *n, float ax, float ay, float az, float angle_rad) {
    n->rotation = rl_quat_from_axis__rig_dup_5c6f6d3c(ax, ay, az, angle_rad);
    n->dirty = true;
}

void rig_rl_node_scale__rig_dup_339ae909(RLNode *n, float x, float y, float z) {
    n->scale.x = x; n->scale.y = y; n->scale.z = z;
    n->dirty = true;
}

void rig_rl_node_look_at__rig_variant_cce82672(RLNode *n, float tx, float ty, float tz) {
        return 0;
    RLVec3 dir = rl_v3norm(rl_v3sub((RLVec3){tx,ty,tz}, n->position));

    RLVec3 def_fwd = {0,0,-1};
    RLVec3 axis = rl_v3cross(def_fwd, dir);
    float  dot  = rl_v3dot(def_fwd, dir);
    if (rl_v3len(axis) < 1e-6f) {
        n->rotation = dot > 0 ? rl_quat_identity() : rl_quat_from_axis(0,1,0, RL_PI);
    } else {
        float angle = acosf(fmaxf(-1.0f, fminf(1.0f, dot)));
        n->rotation = rl_quat_normalize(rl_quat_from_axis(axis.x, axis.y, axis.z, angle));
    }
    n->dirty = true;
}

static void rl_update_node__rig_variant_3196a606(RLNode *n, const RLMat4 *parent_world) {
    if (n->is_static && !n->dirty) {
        for (int i = 0; i < n->n_children; i++)
            rl_update_node(n->children[i], &n->world_mat);
        return 0;
    }

    if (n->dirty) {
        RLMat4 T, R, S, tmp;
        rl_mat4_translate(&T, n->position.x, n->position.y, n->position.z);
        rl_mat4_from_quat(&R, n->rotation);
        rl_mat4_scale_m (&S, n->scale.x, n->scale.y, n->scale.z);

        rl_mat4_mul(&tmp, &T, &R);
        rl_mat4_mul(&n->local_mat, &tmp, &S);

        if (parent_world)
            rl_mat4_mul(&n->world_mat, parent_world, &n->local_mat);
        else
            n->world_mat = n->local_mat;

        rl_aabb_transform(&n->bounds_world, &n->bounds_local, &n->world_mat);

        if (n->type == RL_NODE_CAMERA) {
            RLVec3 eye = n->position;
            RLVec3 fwd = rl_quat_forward(n->rotation);
            RLVec3 up  = rl_quat_up(n->rotation);
            RLVec3 target = rl_v3add(eye, fwd);
            rl_mat4_lookat(&n->camera.view_mat, eye, target, up);
            rl_mat4_perspective(&n->camera.proj_mat, n->camera.fov,
                                 n->camera.aspect, n->camera.near_z, n->camera.far_z);
            rl_mat4_mul(&n->camera.view_proj,
                         &n->camera.proj_mat, &n->camera.view_mat);
        }

        if (n->type == RL_NODE_LIGHT) {

        }

        n->dirty = false;
    }

    for (int i = 0; i < n->n_children; i++)
        rl_update_node(n->children[i], &n->world_mat);
}
void rig_rl_update_transforms__rig_dup_4b302e19(RigRenderLoop *rl, RLNode *root) {
    if (!root) root = rl->root;
    rl_update_node__rig_variant_3196a606(root, NULL);
}

uint32_t rig_rl_material_create__rig_variant_f37d3e42(RigRenderLoop *rl, const char *name) {
    if (rl->n_materials >= RL_MAX_MATERIALS) return UINT32_MAX;
    uint32_t id = rl->n_materials++;
    RLMaterial *m = &rl->materials[id];
    memset(m, 0, sizeof(*m));
    m->id         = id;
    m->albedo[0]  = m->albedo[1] = m->albedo[2] = 0.8f;
    m->roughness  = 0.5f;
    m->metallic   = 0.0f;
    m->ao         = 1.0f;
    m->alpha      = 1.0f;
    m->emissive_strength = 1.0f;
    m->coat_ior   = 1.5f;
    m->sss_radius[0] = 3.67f;
    m->sss_radius[1] = 1.37f;
    m->sss_radius[2] = 0.68f;
    strncpy(m->name, name ? name : "material", 63);
    return id;
}

RLMaterial* rig_rl_material_get__rig_dup_71321899(RigRenderLoop *rl, uint32_t id) {
    if (id >= rl->n_materials) return NULL;
    return &rl->materials[id];
}

void rig_rl_material_destroy__rig_variant_7b6b84a3(RigRenderLoop *rl, uint32_t id) {
    if (id >= rl->n_materials) return 0;
    rl_memset(&rl->materials[id], 0, sizeof(rl->materials[0]));
}

uint8_t rig_rl_light_add__rig_variant_5741dd3c(RigRenderLoop *rl, RLLightType type) {
    if (rl->n_lights >= RL_MAX_LIGHTS) return 0xFF;
    uint8_t idx = rl->n_lights++;
    RLLight *l  = &rl->lights[idx];
    memset(l, 0, sizeof(*l));
    l->type      = type;
    l->color[0]  = l->color[1] = l->color[2] = 1.0f;
    l->intensity = 1.0f;
    l->range     = 10.0f;
    l->active    = true;
    return idx;
}

void rig_rl_light_remove__rig_variant_4fe42ee3(RigRenderLoop *rl, uint8_t idx) {
    if (idx >= rl->n_lights) return 0;
    rl->lights[idx].active = false;
}

void rig_rl_studio_setup__rig_dup_9cb5221f(RigRenderLoop *rl, float r, float h) {
    rl->studio.radius   = r;
    rl->studio.height   = h;
    rl->studio.auto_phi = true;

    float kaz = RL_PHI2 * RL_PI * 0.25f;
    rl->studio.key = (RLLight){
        .type      = RL_LIGHT_DIRECTIONAL,
        .position  = { r*cosf(kaz), h*RL_PHI2, r*sinf(kaz) },
        .direction = rl_v3norm__rig_dup_6d693a7f((RLVec3){ -cosf(kaz), -RL_PHI2, -sinf(kaz) }),
        .color     = { 1.000f, 0.970f, 0.850f },
        .intensity = 3.0f,
        .cast_shadow = true,
        .active    = true,
    };

    float faz = -RL_PHI * RL_PI * 0.25f;
    rl->studio.fill = (RLLight){
        .type      = RL_LIGHT_DIRECTIONAL,
        .position  = { r*cosf(faz), h*0.8f, r*sinf(faz) },
        .direction = rl_v3norm__rig_dup_6d693a7f((RLVec3){ -cosf(faz), -0.8f, -sinf(faz) }),
        .color     = { 0.65f, 0.75f, 1.00f },
        .intensity = 3.0f * RL_PHI_INV * RL_PHI_INV,
        .cast_shadow = false,
        .active    = true,
    };

    rl->studio.rim = (RLLight){
        .type      = RL_LIGHT_DIRECTIONAL,
        .position  = { -r*0.8f, h*RL_PHI_INV, -r*0.6f },
        .direction = rl_v3norm__rig_dup_6d693a7f((RLVec3){ 0.8f, -RL_PHI_INV, 0.6f }),
        .color     = { 1.022f, 0.782f, 0.344f },
        .intensity = 2.0f * RL_PHI_INV,
        .cast_shadow = false,
        .active    = true,
    };
}

typedef struct { float n[6][4]; } RLFrustum;

static void rl_extract_frustum__rig_dup_96990749(RLFrustum *f, const RLMat4 *vp) {
    const float *m = vp->m;

     f->n[0][0]=m[3]+m[0]; f->n[0][1]=m[7]+m[4]; f->n[0][2]=m[11]+m[8];  f->n[0][3]=m[15]+m[12];
     f->n[1][0]=m[3]-m[0]; f->n[1][1]=m[7]-m[4]; f->n[1][2]=m[11]-m[8];  f->n[1][3]=m[15]-m[12];
     f->n[2][0]=m[3]+m[1]; f->n[2][1]=m[7]+m[5]; f->n[2][2]=m[11]+m[9];  f->n[2][3]=m[15]+m[13];
     f->n[3][0]=m[3]-m[1]; f->n[3][1]=m[7]-m[5]; f->n[3][2]=m[11]-m[9];  f->n[3][3]=m[15]-m[13];
     f->n[4][0]=m[3]+m[2]; f->n[4][1]=m[7]+m[6]; f->n[4][2]=m[11]+m[10]; f->n[4][3]=m[15]+m[14];
     f->n[5][0]=m[3]-m[2]; f->n[5][1]=m[7]-m[6]; f->n[5][2]=m[11]-m[10]; f->n[5][3]=m[15]-m[14];

    for (int i = 0; i < 6; i++) {
        float l = sqrtf(f->n[i][0]*f->n[i][0] + f->n[i][1]*f->n[i][1] + f->n[i][2]*f->n[i][2]);
        if (l > 1e-6f) { f->n[i][0]/=l; f->n[i][1]/=l; f->n[i][2]/=l; f->n[i][3]/=l; }
    }
}
static bool rl_aabb_in_frustum__rig_dup_41b0e974(const RLFrustum *f, const RLAABB *b) {
    for (int i = 0; i < 6; i++) {
        float px = f->n[i][0] > 0 ? b->max.x : b->min.x;
        float py = f->n[i][1] > 0 ? b->max.y : b->min.y;
        float pz = f->n[i][2] > 0 ? b->max.z : b->min.z;
        if (f->n[i][0]*px + f->n[i][1]*py + f->n[i][2]*pz + f->n[i][3] < 0.0f)
            return false;
    }
    return true;
}
uint32_t rig_rl_cull_frustum__rig_dup_6b0a37a8(RigRenderLoop *rl, RLNode *camera) {
    if (!camera) camera = rl->active_camera;
    if (!camera) return 0;

    RLFrustum frust;
    rl_extract_frustum__rig_dup_96990749(&frust, &camera->camera.view_proj);

    uint32_t culled = 0;
    for (uint32_t i = 0; i < rl->n_nodes; i++) {
        RLNode *n = &rl->nodes[i];
        if (!n->visible || n->type != RL_NODE_MESH) continue;
        if (!rl_aabb_in_frustum__rig_dup_41b0e974(&frust, &n->bounds_world)) {
            n->visible = false;
            culled++;
        }
    }
    return culled;
}

static int rl_cmp_opaque__rig_dup_63b390c8(const void *a, const void *b) {
    const RLDrawCmd *ca = (const RLDrawCmd*)a;
    const RLDrawCmd *cb = (const RLDrawCmd*)b;

    return (ca->depth_key < cb->depth_key) ? -1 : (ca->depth_key > cb->depth_key) ? 1 : 0;
}
static int rl_cmp_transparent__rig_dup_9ea73729(const void *a, const void *b) {
    const RLDrawCmd *ca = (const RLDrawCmd*)a;
    const RLDrawCmd *cb = (const RLDrawCmd*)b;

    return (ca->depth_key > cb->depth_key) ? -1 : (ca->depth_key < cb->depth_key) ? 1 : 0;
}
static void rl_collect_draws__rig_dup_5ceee7c8(RigRenderLoop *rl, RLNode *cam) {
    rl->n_opaque      = 0;
    rl->n_transparent = 0;

    RLVec3 cam_pos = cam ? cam->position : (RLVec3){0,0,0};

    for (uint32_t i = 0; i < rl->n_nodes; i++) {
        RLNode *n = &rl->nodes[i];
        if (!n->visible) continue;
        if (n->type != RL_NODE_MESH && n->type != RL_NODE_AVATAR) continue;
        if (!n->vao) continue;

        RLMaterial *mat = (n->material_id < rl->n_materials)
                          ? &rl->materials[n->material_id] : NULL;

        RLVec3 center = {
            (n->bounds_world.min.x + n->bounds_world.max.x) * 0.5f,
            (n->bounds_world.min.y + n->bounds_world.max.y) * 0.5f,
            (n->bounds_world.min.z + n->bounds_world.max.z) * 0.5f,
        };
        RLVec3 d = rl_v3sub__rig_dup_8038bddc(center, cam_pos);
        float  depth_sq = rl_v3dot__rig_dup_74a01083(d, d);

        bool is_transparent = mat && (mat->alpha < 0.999f || mat->alpha_clip);

        if (!is_transparent && rl->n_opaque < RL_MAX_DRAW_CMDS) {
            rl->opaque_queue[rl->n_opaque++] = (RLDrawCmd){
                .vao         = n->vao,
                .material_id = n->material_id,
                .transform   = n->world_mat,
                .bounds      = n->bounds_world,
                .index_offset= 0,
                .index_count = n->vao->n_indices,
                .depth_key   = depth_sq,
                .is_transparent = false,
                .cast_shadow = n->cast_shadow,
            };
        } else if (is_transparent && rl->n_transparent < RL_MAX_TRANSPARENT) {
            rl->transparent_queue[rl->n_transparent++] = (RLDrawCmd){
                .vao         = n->vao,
                .material_id = n->material_id,
                .transform   = n->world_mat,
                .bounds      = n->bounds_world,
                .index_offset= 0,
                .index_count = n->vao->n_indices,
                .depth_key   = depth_sq,
                .is_transparent = true,
                .cast_shadow = false,
            };
        }
    }

    qsort(rl->opaque_queue,      rl->n_opaque,      sizeof(RLDrawCmd), rl_cmp_opaque);
    qsort(rl->transparent_queue, rl->n_transparent, sizeof(RLDrawCmd), rl_cmp_transparent);
}
static void rl_bind_material__rig_variant_77cde739(RigRenderLoop *rl, uint32_t prog_id, uint32_t mat_id) {
    RLMaterial *mat = rig_rl_material_get(rl, mat_id);
    if (!mat) return 0;
    RigGPUCtx *gpu = rl->gpu;

    rig_uniform_3f(gpu, prog_id, "uAlbedo",    mat->albedo[0], mat->albedo[1], mat->albedo[2]);
    rig_uniform_1f(gpu, prog_id, "uRoughness", mat->roughness);
    rig_uniform_1f(gpu, prog_id, "uMetallic",  mat->metallic);
    rig_uniform_1f(gpu, prog_id, "uAO",        mat->ao);
    rig_uniform_3f(gpu, prog_id, "uEmissive",  mat->emissive[0], mat->emissive[1], mat->emissive[2]);
    rig_uniform_1f(gpu, prog_id, "uEmissiveStr", mat->emissive_strength);
    rig_uniform_1f(gpu, prog_id, "uAlpha",     mat->alpha);
    rig_uniform_1f(gpu, prog_id, "uSSSWeight", mat->sss_weight);

    if (mat->tex_albedo)    { rig_texture_bind(gpu, mat->tex_albedo,    0); rig_uniform_1i(gpu, prog_id, "uTexAlbedo",    0); }
    if (mat->tex_normal)    { rig_texture_bind(gpu, mat->tex_normal,    1); rig_uniform_1i(gpu, prog_id, "uTexNormal",    1); }
    if (mat->tex_roughness) { rig_texture_bind(gpu, mat->tex_roughness, 2); rig_uniform_1i(gpu, prog_id, "uTexRoughness", 2); }
    if (mat->tex_metallic)  { rig_texture_bind(gpu, mat->tex_metallic,  3); rig_uniform_1i(gpu, prog_id, "uTexMetallic",  3); }
    if (mat->tex_ao)        { rig_texture_bind(gpu, mat->tex_ao,        4); rig_uniform_1i(gpu, prog_id, "uTexAO",        4); }
    if (mat->tex_emissive)  { rig_texture_bind(gpu, mat->tex_emissive,  5); rig_uniform_1i(gpu, prog_id, "uTexEmissive",  5); }
    if (mat->enable_sss && mat->tex_sss_thickness)
        { rig_texture_bind(gpu, mat->tex_sss_thickness, 6); rig_uniform_1i(gpu, prog_id, "uTexSSS", 6); }
}
static void rl_upload_camera__rig_dup_9085d755(RigRenderLoop *rl, uint32_t prog_id, RLNode *cam) {
    RigGPUCtx *gpu = rl->gpu;
    rig_uniform_mat4(gpu, prog_id, "uView",     cam->camera.view_mat.m);
    rig_uniform_mat4(gpu, prog_id, "uProj",     cam->camera.proj_mat.m);
    rig_uniform_mat4(gpu, prog_id, "uViewProj", cam->camera.view_proj.m);
    rig_uniform_3f  (gpu, prog_id, "uCamPos",   cam->position.x, cam->position.y, cam->position.z);
    rig_uniform_1f  (gpu, prog_id, "uNear",     cam->camera.near_z);
    rig_uniform_1f  (gpu, prog_id, "uFar",      cam->camera.far_z);
}
static void rl_upload_studio_lights__rig_variant_ab1c9bf2(RigRenderLoop *rl, uint32_t prog_id) {
    RigGPUCtx *gpu = rl->gpu;
    if (!rl->use_studio) return 0;
    rig_uniform_3f(gpu, prog_id, "uLightDir[0]",
        rl->studio.key.direction.x,
        rl->studio.key.direction.y,
        rl->studio.key.direction.z);
    rig_uniform_3f(gpu, prog_id, "uLightColor[0]",
        rl->studio.key.color[0] * rl->studio.key.intensity,
        rl->studio.key.color[1] * rl->studio.key.intensity,
        rl->studio.key.color[2] * rl->studio.key.intensity);

    rig_uniform_3f(gpu, prog_id, "uLightDir[1]",
        rl->studio.fill.direction.x,
        rl->studio.fill.direction.y,
        rl->studio.fill.direction.z);
    rig_uniform_3f(gpu, prog_id, "uLightColor[1]",
        rl->studio.fill.color[0] * rl->studio.fill.intensity,
        rl->studio.fill.color[1] * rl->studio.fill.intensity,
        rl->studio.fill.color[2] * rl->studio.fill.intensity);

    rig_uniform_3f(gpu, prog_id, "uLightDir[2]",
        rl->studio.rim.direction.x,
        rl->studio.rim.direction.y,
        rl->studio.rim.direction.z);
    rig_uniform_3f(gpu, prog_id, "uLightColor[2]",
        rl->studio.rim.color[0] * rl->studio.rim.intensity,
        rl->studio.rim.color[1] * rl->studio.rim.intensity,
        rl->studio.rim.color[2] * rl->studio.rim.intensity);

    rig_uniform_1i(gpu, prog_id, "uNumLights", 3);
}
void rig_rl_shadow_pass__rig_variant_8fde7db3(RigRenderLoop *rl) {
    if (!rl->pbr.initialized || !rl->pbr.prog_shadow) return 0;
    if (!rl->use_studio || !rl->studio.key.cast_shadow) return 0;
    RigGPUCtx *gpu = rl->gpu;

    RLLight *key = &rl->studio.key;
    RLVec3   eye = key->position;
    RLVec3   tgt = {0,0,0};
    RLVec3   up  = {0,1,0};

    RLMat4 lview, lproj, lvp;
    float  hs = rl->studio.radius * 2.0f;
    rl_mat4_lookat(&lview, eye, tgt, up);
    rl_mat4_ortho (&lproj, -hs, hs, -hs, hs, 0.1f, rl->studio.radius * 6.0f);
    rl_mat4_mul   (&lvp, &lproj, &lview);

    rl->studio.key.light_view_proj = lvp;

    rig_fbo_bind(gpu, rl->pbr.fbo_shadow);
    rig_state_viewport(gpu, 0, 0, RL_SHADOW_RES, RL_SHADOW_RES);
    rig_state_clear(gpu, 0,0,0,0, 1.0f);
    rig_state_depth(gpu, true, true, 0x0201 );
    rig_state_cull (gpu, true, 0x0405 );

    rig_program_bind(gpu, rl->pbr.prog_shadow);
    rig_uniform_mat4(gpu, rl->pbr.prog_shadow, "uLightVP", lvp.m);

    for (uint32_t i = 0; i < rl->n_opaque; i++) {
        RLDrawCmd *cmd = &rl->opaque_queue[i];
        if (!cmd->cast_shadow) continue;
        rig_uniform_mat4(gpu, rl->pbr.prog_shadow, "uModel", cmd->transform.m);
        rig_draw_indexed(gpu, cmd->vao, cmd->index_offset, cmd->index_count);
    }

    rig_fbo_unbind(gpu);
}

void rig_rl_geometry_pass__rig_variant_1a45db9c(RigRenderLoop *rl) {
    if (!rl->pbr.initialized) return 0;
    RigGPUCtx *gpu  = rl->gpu;
    RLNode    *cam  = rl->active_camera;
    if (!cam) return 0;
    rig_pbr_begin_geometry(gpu, &rl->pbr);
    rig_state_viewport(gpu, 0, 0, rl->width, rl->height);
    rig_state_clear   (gpu, 0,0,0,0, 1.0f);
    rig_state_depth   (gpu, true, true, 0x0201 );
    rig_state_cull    (gpu, true, 0x0405 );
    rig_state_blend   (gpu, false, 0, 0);

    uint32_t prog = rl->pbr.prog_geometry;
    rig_program_bind(gpu, prog);
    rl_upload_camera(rl, prog, cam);

    for (uint32_t i = 0; i < rl->n_opaque; i++) {
        RLDrawCmd *cmd = &rl->opaque_queue[i];
        rig_uniform_mat4(gpu, prog, "uModel", cmd->transform.m);

        rig_uniform_mat4(gpu, prog, "uNormalMat", cmd->transform.m);

        rl_bind_material(rl, prog, cmd->material_id);
        rig_draw_indexed(gpu, cmd->vao, cmd->index_offset, cmd->index_count);
    }

    rig_pbr_end_geometry(gpu, &rl->pbr);
}

void rig_rl_lighting_pass__rig_variant_d8768cb5(RigRenderLoop *rl) {
    if (!rl->pbr.initialized) return 0;
    RigGPUCtx *gpu = rl->gpu;
    RLNode    *cam = rl->active_camera;
    if (!cam) return 0;
    uint32_t prog = rl->pbr.prog_lighting;
    rig_program_bind(gpu, prog);
    rl_upload_camera(rl, prog, cam);
    rl_upload_studio_lights(rl, prog);

    RLMat4 lvp = rl->studio.key.light_view_proj;
    rig_uniform_mat4(gpu, prog, "uLightVP", lvp.m);

    if (rl->tex_hdri) {
        rig_texture_bind(gpu, rl->tex_hdri, 7);
        rig_uniform_1i  (gpu, prog, "uEnvMap", 7);
    }
    if (rl->pbr.tex_irradiance) {
        rig_texture_bind(gpu, rl->pbr.tex_irradiance, 8);
        rig_uniform_1i  (gpu, prog, "uIrradiance", 8);
    }
    if (rl->pbr.tex_prefilter) {
        rig_texture_bind(gpu, rl->pbr.tex_prefilter, 9);
        rig_uniform_1i  (gpu, prog, "uPrefilter", 9);
    }
    if (rl->pbr.tex_brdf_lut) {
        rig_texture_bind(gpu, rl->pbr.tex_brdf_lut, 10);
        rig_uniform_1i  (gpu, prog, "uBRDFLUT", 10);
    }
    rig_uniform_1f(gpu, prog, "uHDRIIntensity", rl->hdri_intensity);
    rig_uniform_1f(gpu, prog, "uHDRIRotation",  rl->hdri_rotation);
    rig_uniform_2f(gpu, prog, "uResolution",    (float)rl->width, (float)rl->height);

    rig_pbr_lighting_pass(gpu, &rl->pbr);

    if (rl->n_transparent > 0) {
        uint32_t fprog = rl->pbr.prog_pbr_forward;
        rig_program_bind(gpu, fprog);
        rl_upload_camera(rl, fprog, cam);
        rl_upload_studio_lights(rl, fprog);
        rig_state_depth (gpu, true,  false, 0x0203 );
        rig_state_blend (gpu, true, 0x0302 , 0x0303 );
        rig_state_cull  (gpu, false, 0);

        for (uint32_t i = 0; i < rl->n_transparent; i++) {
            RLDrawCmd *cmd = &rl->transparent_queue[i];
            rig_uniform_mat4(gpu, fprog, "uModel", cmd->transform.m);
            rl_bind_material(rl, fprog, cmd->material_id);
            rig_draw_indexed(gpu, cmd->vao, cmd->index_offset, cmd->index_count);
        }
        rig_state_blend(gpu, false, 0, 0);
    }
}

void rig_rl_post_pass__rig_variant_d14552a2(RigRenderLoop *rl) {
    if (!rl->pbr.initialized) return 0;
    RigGPUCtx *gpu = rl->gpu;

    if (rl->post.ssao) {
        rig_pbr_ssao_pass(gpu, &rl->pbr);
        rig_uniform_1f(gpu, rl->pbr.prog_ssao, "uRadius",  rl->post.ssao_radius);
        rig_uniform_1f(gpu, rl->pbr.prog_ssao, "uBias",    rl->post.ssao_bias);
        rig_uniform_1i(gpu, rl->pbr.prog_ssao, "uSamples", rl->post.ssao_samples);
    }

    if (rl->post.bloom) {
        rig_pbr_sss_pass(gpu, &rl->pbr);
    }

    if (rl->post.bloom) {
        rig_pbr_bloom_pass(gpu, &rl->pbr,
                           rl->post.bloom_threshold,
                           rl->post.bloom_strength);
    }

    rig_pbr_tonemap_pass(gpu, &rl->pbr, rl->post.exposure);

    uint32_t tp = rl->pbr.prog_tonemap;
    rig_uniform_1i(gpu, tp, "uDoACES",         rl->post.aces_tonemap ? 1 : 0);
    rig_uniform_1f(gpu, tp, "uGamma",           rl->post.gamma);
    rig_uniform_1f(gpu, tp, "uContrast",        rl->post.contrast);
    rig_uniform_1f(gpu, tp, "uSaturation",      rl->post.saturation);
    rig_uniform_1f(gpu, tp, "uGrainStr",        rl->post.film_grain    ? rl->post.grain_strength    : 0.0f);
    rig_uniform_1f(gpu, tp, "uVignettePow",     rl->post.vignette      ? rl->post.vignette_power     : 0.0f);
    rig_uniform_1f(gpu, tp, "uVignetteSoft",    rl->post.vignette      ? rl->post.vignette_softness  : 0.0f);
    rig_uniform_1f(gpu, tp, "uCAStr",           rl->post.chromatic_aberration ? rl->post.ca_strength : 0.0f);
    rig_uniform_2f(gpu, tp, "uResolution",      (float)rl->width, (float)rl->height);
    rig_uniform_1f(gpu, tp, "uTime",            (float)(rl_now_ns() % 1000000000ull) * 1e-9f);
}

static float* rl_decode_rgbe__rig_variant_10e6a66d(const uint8_t *data, uint32_t w, uint32_t h) {
    float *out = (float*)malloc(w * h * 3 * sizeof(float));
    if (!out) return NULL;
    for (uint32_t i = 0; i < w * h; i++) {
        uint8_t r=data[i*4+0], g=data[i*4+1], b=data[i*4+2], e=data[i*4+3];
        if (e == 0) { out[i*3]=out[i*3+1]=out[i*3+2]=0.0f; }
        else {
            float sc = ldexpf(1.0f, (int)e - 128 - 8);
            out[i*3+0] = (float)r * sc;
            out[i*3+1] = (float)g * sc;
            out[i*3+2] = (float)b * sc;
        }
    }
    return out;
}
int rig_rl_load_hdri__rig_variant_d3c624da(RigRenderLoop *rl, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "[RigRL] No se puede abrir HDRI: %s\n", path); return -1; }

    char line[256];
    bool valid = false;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "#?RADIANCE", 10) == 0 || strncmp(line, "#?RGBE", 6) == 0)
            valid = true;
        if (line[0] == '\n') break;
    }
    if (!valid) { fclose(f); fprintf(stderr, "[RigRL] HDRI no es formato Radiance\n"); return -1; }

    uint32_t w=0, h=0;
    char axes[32]="";
    if (!fgets(line, sizeof(line), f) ||
        sscanf(line, "%31s %u %*s %u", axes, &h, &w) < 3 || w==0 || h==0) {
        fclose(f);
        fprintf(stderr, "[RigRL] HDRI: error leyendo dimensiones\n");
        return -1;
    }

    uint32_t npix = w * h;
    uint8_t *raw  = (uint8_t*)malloc(npix * 4);
    if (!raw) { fclose(f); return -1; }
    size_t nr = fread(raw, 4, npix, f);
    fclose(f);
    if (nr < npix) { free(raw); fprintf(stderr, "[RigRL] HDRI: datos incompletos\n"); return -1; }

    float *hdr = rl_decode_rgbe__rig_variant_10e6a66d(raw, w, h);
    free(raw);
    if (!hdr) return -1;

    float *rgba32 = (float*)malloc(npix * 4 * sizeof(float));
    if (!rgba32) { free(hdr); return -1; }
    for (uint32_t i = 0; i < npix; i++) {
        rgba32[i*4+0] = hdr[i*3+0];
        rgba32[i*4+1] = hdr[i*3+1];
        rgba32[i*4+2] = hdr[i*3+2];
        rgba32[i*4+3] = 1.0f;
    }
    free(hdr);

    uint32_t tex = rig_texture_create(rl->gpu, RIG_TEX_RGBA32F, w, h, rgba32, false);
    free(rgba32);
    if (tex == RIG_GPU_INVALID_ID) return -1;

    if (rl->tex_hdri) rig_texture_free(rl->gpu, rl->tex_hdri);
    rl->tex_hdri = tex;
    fprintf(stderr, "[RigRL] HDRI cargado: %s (%ux%u)\n", path, w, h);
    return 0;
}

void rig_rl_set_hdri__rig_variant_a00bd1a9(RigRenderLoop *rl, uint32_t tex_id,
                      float rot, float intensity) {
    rl->tex_hdri        = tex_id;
    rl->hdri_rotation   = rot;
    rl->hdri_intensity  = intensity;
    return 0;
}

const RLFrameProfile* rig_rl_profile_current__rig_dup_95d9bae4(const RigRenderLoop *rl) {
    return &rl->profile;
}

void rig_rl_profile_emit_json__rig_variant_3d96a41d(const RigRenderLoop *rl, char *out, size_t sz) {
    const RLFrameProfile *p = &rl->profile;
    snprintf(out, sz,
        "{"
        "\"frame\":%llu,"
        "\"fps\":%.2f,"
        "\"total_ms\":%.3f,"
        "\"shadow_ms\":%.3f,"
        "\"geometry_ms\":%.3f,"
        "\"ssao_ms\":%.3f,"
        "\"lighting_ms\":%.3f,"
        "\"sss_ms\":%.3f,"
        "\"transparent_ms\":%.3f,"
        "\"bloom_ms\":%.3f,"
        "\"tonemap_ms\":%.3f,"
        "\"ui_ms\":%.3f,"
        "\"present_ms\":%.3f,"
        "\"draw_calls\":%u,"
        "\"triangles\":%u,"
        "\"culled\":%u"
        "}",
        (unsigned long long)p->frame_index,
        p->fps, p->total_ms,
        p->shadow_ms, p->geometry_ms, p->ssao_ms,
        p->lighting_ms, p->sss_ms, p->transparent_ms,
        p->bloom_ms, p->tonemap_ms, p->ui_ms,
        p->present_ms,
        p->draw_calls, p->triangles, p->culled_nodes);
}

int rig_rl_tick__rig_dup_2c36ac07(RigRenderLoop *rl) {
    if (!rl || !rl->gpu) return -1;
    if (rl->paused) return 0;

    double t_frame = rl_now_ms__rig_dup_2640366f();

    rig_gpu_frame_begin(rl->gpu);

    pthread_mutex_lock(&rl->scene_mutex);

    rig_rl_update_transforms__rig_dup_4b302e19(rl, rl->root);

    RLNode *cam = rl->active_camera;
    uint32_t culled = cam ? rig_rl_cull_frustum__rig_dup_6b0a37a8(rl, cam) : 0;
    rl_collect_draws__rig_dup_5ceee7c8(rl, cam);

    pthread_mutex_unlock(&rl->scene_mutex);

    if (rl->on_frame_begin)
        rl->on_frame_begin(rl, rl->userdata);

    double t0 = rl_now_ms__rig_dup_2640366f();
    rig_rl_shadow_pass__rig_variant_8fde7db3(rl);
    rl->profile.shadow_ms = rl_now_ms__rig_dup_2640366f() - t0;

    t0 = rl_now_ms__rig_dup_2640366f();
    rig_rl_geometry_pass__rig_variant_1a45db9c(rl);
    rl->profile.geometry_ms = rl_now_ms__rig_dup_2640366f() - t0;

    t0 = rl_now_ms__rig_dup_2640366f();
    if (rl->post.ssao) rig_pbr_ssao_pass(rl->gpu, &rl->pbr);
    rl->profile.ssao_ms = rl_now_ms__rig_dup_2640366f() - t0;

    t0 = rl_now_ms__rig_dup_2640366f();
    rig_rl_lighting_pass__rig_variant_d8768cb5(rl);
    rl->profile.lighting_ms = rl_now_ms__rig_dup_2640366f() - t0;

    t0 = rl_now_ms__rig_dup_2640366f();
    rl->profile.sss_ms = rl_now_ms__rig_dup_2640366f() - t0;

    t0 = rl_now_ms__rig_dup_2640366f();
    if (rl->post.bloom) rig_pbr_bloom_pass(rl->gpu, &rl->pbr,
        rl->post.bloom_threshold, rl->post.bloom_strength);
    rl->profile.bloom_ms = rl_now_ms__rig_dup_2640366f() - t0;

    t0 = rl_now_ms__rig_dup_2640366f();
    rig_pbr_tonemap_pass(rl->gpu, &rl->pbr, rl->post.exposure);
    {
        uint32_t tp = rl->pbr.prog_tonemap;
        rig_uniform_1i(rl->gpu, tp, "uDoACES",    rl->post.aces_tonemap ? 1 : 0);
        rig_uniform_1f(rl->gpu, tp, "uGamma",      rl->post.gamma);
        rig_uniform_1f(rl->gpu, tp, "uContrast",   rl->post.contrast);
        rig_uniform_1f(rl->gpu, tp, "uSaturation", rl->post.saturation);
        rig_uniform_1f(rl->gpu, tp, "uGrainStr",   rl->post.film_grain ? rl->post.grain_strength : 0.0f);
        rig_uniform_1f(rl->gpu, tp, "uVignettePow",rl->post.vignette   ? rl->post.vignette_power : 0.0f);
        rig_uniform_1f(rl->gpu, tp, "uCAStr",      rl->post.chromatic_aberration ? rl->post.ca_strength : 0.0f);
        rig_uniform_2f(rl->gpu, tp, "uResolution", (float)rl->width, (float)rl->height);
        rig_uniform_1f(rl->gpu, tp, "uTime",       (float)(rl_now_ns__rig_dup_3eef24d2() % 1000000000ull) * 1e-9f);
    }
    rl->profile.tonemap_ms = rl_now_ms__rig_dup_2640366f() - t0;

    if (rl->on_frame_end)
        rl->on_frame_end(rl, rl->userdata);

    t0 = rl_now_ms__rig_dup_2640366f();
    if (rl->display) rig_display_present(rl->display);
    else             rig_gpu_present(rl->gpu);
    rl->profile.present_ms = rl_now_ms__rig_dup_2640366f() - t0;

    rl->profile.total_ms     = rl_now_ms__rig_dup_2640366f() - t_frame;
    rl->profile.draw_calls   = rl->gpu->stats.draw_calls;
    rl->profile.triangles    = rl->gpu->stats.triangles;
    rl->profile.culled_nodes = culled;
    rl->profile.frame_index++;

    static uint64_t _fps_ts = 0, _fps_cnt = 0;
    _fps_cnt++;
    uint64_t now_ns = rl_now_ns__rig_dup_3eef24d2();
    if (_fps_ts == 0) _fps_ts = now_ns;
    uint64_t elapsed = now_ns - _fps_ts;
    if (elapsed >= 1000000000ull) {
        rl->profile.fps = (double)_fps_cnt * 1.0e9 / (double)elapsed;
        _fps_ts  = now_ns;
        _fps_cnt = 0;
    }

    rl->profile_history[rl->profile_head % 120] = rl->profile;
    rl->profile_head++;

    if (!rl->display || !rl->display->vsync_enabled) {
        uint64_t used_ns = (uint64_t)(rl->profile.total_ms * 1e6);
        if (used_ns < rl->target_frame_ns) {
            rl_sleep_ns__rig_variant_d5ac4aca(rl->target_frame_ns - used_ns);
        } else if (rl->adaptive_fps) {

        }
    }
    rl->last_frame_ts = rl_now_ns__rig_dup_3eef24d2();

    for (uint32_t i = 0; i < rl->n_nodes; i++)
        if (rl->nodes[i].type == RL_NODE_MESH)
            rl->nodes[i].visible = true;

    return 0;
}

int rig_rl_run__rig_dup_6863ea5a(RigRenderLoop *rl) {
    if (!rl) return -1;
    rl->running = true;
    while (rl->running) {
        if (rig_rl_tick__rig_dup_2c36ac07(rl) < 0) {
            rl->running = false;
            return -1;
        }
    }
    return 0;
}

static void* rl_thread_fn__rig_dup_9c04d351(void *arg) {
    RigRenderLoop *rl = (RigRenderLoop*)arg;

    rig_gpu_make_current(rl->gpu);
    rig_rl_run__rig_dup_6863ea5a(rl);
    rig_gpu_release_thread(rl->gpu);
    return NULL;
}
int rig_rl_run_thread__rig_variant_e7e96a8c(RigRenderLoop *rl) {
    if (!rl || rl->render_thread) return -1;
    rl->running = true;

    rig_gpu_release_thread(rl->gpu);
    if (pthread_create(&rl->render_thread, NULL, rl_thread_fn, rl) != 0) {
        rl->running = false;
        rl->render_thread = 0;
        fprintf(stderr, "[RigRL] pthread_create falló: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void rig_rl_stop__rig_variant_9ac65b77(RigRenderLoop *rl) {
    if (!rl) return 0;
    rl->running = false;
    if (rl->render_thread) {
        pthread_join(rl->render_thread, NULL);
        rl->render_thread = 0;
    }
}

void rig_rl_pause__rig_dup_8e65f851 (RigRenderLoop *rl) { if (rl) rl->paused = true; }
void rig_rl_resume__rig_dup_b853fdcd(RigRenderLoop *rl) { if (rl) rl->paused = false; }

static void rl_handle_resize__rig_variant_22f6b8f9(RigRenderLoop *rl, uint32_t w, uint32_t h) {
    if (w == rl->width && h == rl->height) return 0;
    rl->width  = w;
    rl->height = h;
    rig_pbr_pipeline_resize(rl->gpu, &rl->pbr, w, h);

    if (rl->active_camera)
        rl->active_camera->camera.aspect = (float)w / (float)h;

    if (rl->on_resize)
        rl->on_resize(rl, w, h, rl->userdata);
}
void rig_rl_post_default_rigart__rig_variant_402d211c(RLPostConfig *cfg) {
    memset(cfg, 0, sizeof(*cfg));

    cfg->ssao         = true;
    cfg->ssao_radius  = 0.5f;
    cfg->ssao_bias    = 0.025f;
    cfg->ssao_samples = 8;

    cfg->bloom           = true;
    cfg->bloom_threshold = 0.85f;
    cfg->bloom_strength  = 0.35f;
    cfg->bloom_passes    = 5;

    cfg->chromatic_aberration = true;
    cfg->ca_strength = 0.003f;

    cfg->vignette         = true;
    cfg->vignette_power   = 1.6f;
    cfg->vignette_softness= 0.5f;

    cfg->film_grain    = true;
    cfg->grain_strength= 0.035f;

    cfg->scanlines       = false;
    cfg->scanline_density= 1.0f;

    cfg->aces_tonemap = true;
    cfg->exposure     = 1.0f;
    cfg->gamma        = 2.2f;
    cfg->contrast     = 1.08f;
    cfg->saturation   = 1.15f;
    cfg->color_lift[0]= 0.002f; cfg->color_lift[1] = 0.002f; cfg->color_lift[2] = 0.008f;
    cfg->color_gain[0]= 1.0f;   cfg->color_gain[1] = 1.0f;   cfg->color_gain[2] = 1.0f;
}

void rig_rl_post_default_cinema__rig_variant_64234fb3(RLPostConfig *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->ssao         = true;
    cfg->ssao_radius  = 0.8f;
    cfg->ssao_bias    = 0.03f;
    cfg->ssao_samples = 16;
    cfg->bloom        = true;
    cfg->bloom_threshold = 1.2f;
    cfg->bloom_strength  = 0.18f;
    cfg->bloom_passes    = 5;
    cfg->vignette         = true;
    cfg->vignette_power   = 2.0f;
    cfg->vignette_softness= 0.4f;
    cfg->film_grain    = true;
    cfg->grain_strength= 0.025f;
    cfg->aces_tonemap  = true;
    cfg->exposure      = 0.95f;
    cfg->gamma         = 2.2f;
    cfg->contrast      = 1.12f;
    cfg->saturation    = 0.92f;
    cfg->color_lift[0] = 0.001f; cfg->color_lift[1] = 0.001f; cfg->color_lift[2] = 0.004f;
    cfg->color_gain[0] = 1.0f;   cfg->color_gain[1] = 1.0f;   cfg->color_gain[2] = 0.98f;
}

void rig_rl_post_default_clean__rig_variant_a7eba73d(RLPostConfig *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->aces_tonemap = true;
    cfg->exposure     = 1.0f;
    cfg->gamma        = 2.2f;
    cfg->contrast     = 1.0f;
    cfg->saturation   = 1.0f;
    cfg->color_gain[0]= cfg->color_gain[1] = cfg->color_gain[2] = 1.0f;

}
