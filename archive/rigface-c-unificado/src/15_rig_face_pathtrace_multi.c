/* ==========================================================================
 * 15_rig_face_pathtrace_multi.c   —   RIGFACE :: C UNIFICADO
 *
 * Copia canonica : src_raw/rig_face_pathtrace_multi-1.c
 * Copias fundidas: 2
 * Funciones      : 23      Unidades injertadas: 0      Variantes: 0
 *
 * Copia canonica preservada verbatim. Ninguna funcion eliminada,
 * reducida ni omitida. Las divergencias de cuerpo bajo un mismo
 * nombre se conservan como <nombre>__vN con su procedencia.
 * ========================================================================== */
/* ============================================================================
 * rig_face_pathtrace_multi.c
 *
 * RigCom :: Multi-Character Path Tracer Scene Module (aditivo puro, TLAS)
 *
 * sovereign_pathtrace.c ya construye un BVH (BLAS) para UNA malla/material.
 * Este modulo agrega la jerarquia de nivel superior (TLAS sobre BLAS,
 * arquitectura de dos niveles estandar en ray tracing de produccion:
 * RTX, PBRT, Embree) que permite escenas con MULTIPLES personajes/
 * instancias compartiendo iluminacion, sin tocar ni conocer la
 * representacion interna del BVH existente.
 *
 * Desacoplado via callback: cada instancia expone su propio BLAS a traves
 * de una funcion blas_intersect(ctx, ray_local) que el caller implementa
 * envolviendo su codigo real de sovereign_pathtrace.c -- este modulo solo
 * conoce cajas delimitadoras (AABB) y transformos, nunca triangulos.
 *
 *   1. Algebra de matrices 4x4 (inversa general via Gauss-Jordan, para
 *      soportar escala no uniforme por instancia, no solo rotacion+
 *      traslacion rigida).
 *   2. Construccion de TLAS por particion de mediana en el eje mas largo
 *      (BVH top-down clasico, igual principio que un BLAS pero sobre
 *      cajas de instancia en vez de triangulos).
 *   3. Recorrido de rayo mas cercano (closest-hit) transformando el rayo
 *      a espacio local de cada instancia candidata, y transformando el
 *      resultado de vuelta a espacio mundo (normal via inversa-transpuesta,
 *      correcto incluso con escala no uniforme).
 *   4. Rayo de sombra (any-hit, corte temprano) para test de oclusion
 *      entre instancias -- necesario para que un personaje proyecte
 *      sombra sobre otro, capacidad que no existe con un unico mesh.
 *   5. Lista de luces compartida (puntual/direccional) evaluada contra
 *      la escena completa.
 * ==========================================================================*/

#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif

#include <math.h>
#include <string.h>
#include <float.h>

#define RIG_PT_MAX_INSTANCES   256
#define RIG_PT_MAX_TLAS_NODES  (RIG_PT_MAX_INSTANCES * 2)
#define RIG_PT_MAX_LIGHTS      16
#define RIG_PT_STACK_SIZE      64

typedef struct { float x, y, z; } RigPTVec3;
typedef struct { float m[16]; } RigPTMat4; /* row-major, m[row*4+col] */

typedef struct { RigPTVec3 origin, dir; float t_min, t_max; } RigPTRay;
typedef struct {
    int hit;
    float t;
    RigPTVec3 position;   /* espacio local del BLAS que reporto el hit */
    RigPTVec3 normal;     /* espacio local */
    int material_id;
} RigPTHit;

typedef struct {
    RigPTVec3 position;
    RigPTVec3 normal;      /* espacio mundo */
    float t;
    int instance_index;
    int material_id;
} RigPTWorldHit;

typedef struct { RigPTVec3 min, max; } RigPTAABB;

typedef int (*RigPTBlasIntersectFn)(void *blas_ctx, RigPTRay local_ray, RigPTHit *out_hit);
typedef int (*RigPTBlasOccludedFn)(void *blas_ctx, RigPTRay local_ray);

typedef struct {
    RigPTMat4 transform;
    RigPTMat4 inverse_transform;
    RigPTAABB local_aabb;
    RigPTAABB world_aabb;
    void *blas_ctx;
    RigPTBlasIntersectFn intersect_fn;
    RigPTBlasOccludedFn occluded_fn; /* puede ser NULL: se usa intersect_fn como fallback */
    int material_id;
} RigPTInstance;

typedef struct {
    RigPTAABB bounds;
    int left, right;    /* indices de hijos en el arreglo de nodos, -1 si es hoja */
    int instance_index;  /* valido solo si es hoja (left==-1) */
} RigPTTLASNode;

typedef enum { RIG_PT_LIGHT_POINT=0, RIG_PT_LIGHT_DIRECTIONAL } RigPTLightType;
typedef struct {
    RigPTLightType type;
    RigPTVec3 position_or_dir; /* posicion si POINT, direccion (hacia la luz) si DIRECTIONAL */
    RigPTVec3 color;
    float intensity;
} RigPTLight;

typedef struct {
    RigPTInstance instances[RIG_PT_MAX_INSTANCES];
    int instance_count;
    RigPTTLASNode nodes[RIG_PT_MAX_TLAS_NODES];
    int node_count;
    int root_node;
    RigPTLight lights[RIG_PT_MAX_LIGHTS];
    int light_count;
} RigPTScene;

/* -------------------------------------------------------------------------
 * Vectores
 * ---------------------------------------------------------------------- */
static RigPTVec3 ptv_add(RigPTVec3 a, RigPTVec3 b){RigPTVec3 r={a.x+b.x,a.y+b.y,a.z+b.z};return r;}
static RigPTVec3 ptv_sub(RigPTVec3 a, RigPTVec3 b){RigPTVec3 r={a.x-b.x,a.y-b.y,a.z-b.z};return r;}
static RigPTVec3 ptv_scale(RigPTVec3 a, float s){RigPTVec3 r={a.x*s,a.y*s,a.z*s};return r;}
static float ptv_dot(RigPTVec3 a, RigPTVec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static float ptv_len(RigPTVec3 a){return sqrtf(ptv_dot(a,a));}
static RigPTVec3 ptv_norm(RigPTVec3 a){float l=ptv_len(a); if(l<1e-8f){RigPTVec3 z={0,0,1};return z;} return ptv_scale(a,1.0f/l);}

/* -------------------------------------------------------------------------
 * Matrices 4x4: identidad, multiplicacion, transformacion de punto/
 * direccion, e inversa general via Gauss-Jordan con pivoteo parcial
 * (soporta escala no uniforme, no solo transformos rigidos).
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC RigPTMat4 rig_pt_mat4_identity(void) {
    RigPTMat4 m; memset(&m, 0, sizeof(m));
    m.m[0]=1; m.m[5]=1; m.m[10]=1; m.m[15]=1;
    return m;
}

RIGCOM_PUBLIC RigPTMat4 rig_pt_mat4_trs(RigPTVec3 t, RigPTVec3 euler_deg, RigPTVec3 scale) {
    float rx = euler_deg.x * 0.017453293f, ry = euler_deg.y * 0.017453293f, rz = euler_deg.z * 0.017453293f;
    float cx=cosf(rx),sx=sinf(rx), cy=cosf(ry),sy=sinf(ry), cz=cosf(rz),sz=sinf(rz);
    /* R = Rz * Ry * Rx, aplicada a columnas de escala */
    float r00 = cy*cz,               r01 = -cy*sz,              r02 = sy;
    float r10 = sx*sy*cz + cx*sz,    r11 = -sx*sy*sz + cx*cz,   r12 = -sx*cy;
    float r20 = -cx*sy*cz + sx*sz,   r21 = cx*sy*sz + sx*cz,    r22 = cx*cy;

    RigPTMat4 m = rig_pt_mat4_identity();
    m.m[0]=r00*scale.x; m.m[1]=r01*scale.y; m.m[2]=r02*scale.z; m.m[3]=t.x;
    m.m[4]=r10*scale.x; m.m[5]=r11*scale.y; m.m[6]=r12*scale.z; m.m[7]=t.y;
    m.m[8]=r20*scale.x; m.m[9]=r21*scale.y; m.m[10]=r22*scale.z; m.m[11]=t.z;
    return m;
}

static RigPTVec3 rig_pt_mat4_mul_point(const RigPTMat4 *m, RigPTVec3 p) {
    RigPTVec3 r;
    r.x = m->m[0]*p.x + m->m[1]*p.y + m->m[2]*p.z + m->m[3];
    r.y = m->m[4]*p.x + m->m[5]*p.y + m->m[6]*p.z + m->m[7];
    r.z = m->m[8]*p.x + m->m[9]*p.y + m->m[10]*p.z + m->m[11];
    return r;
}
static RigPTVec3 rig_pt_mat4_mul_dir(const RigPTMat4 *m, RigPTVec3 p) {
    RigPTVec3 r;
    r.x = m->m[0]*p.x + m->m[1]*p.y + m->m[2]*p.z;
    r.y = m->m[4]*p.x + m->m[5]*p.y + m->m[6]*p.z;
    r.z = m->m[8]*p.x + m->m[9]*p.y + m->m[10]*p.z;
    return r;
}

/* inversa general 4x4 via Gauss-Jordan con pivoteo parcial. Devuelve 0 en
 * exito, -1 si la matriz es singular (no invertible). */
RIGCOM_PUBLIC int rig_pt_mat4_inverse(const RigPTMat4 *in, RigPTMat4 *out) {
    float a[4][8];
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) a[r][c] = in->m[r*4+c];
        for (int c = 0; c < 4; c++) a[r][4+c] = (r==c) ? 1.0f : 0.0f;
    }
    for (int col = 0; col < 4; col++) {
        int pivot = col;
        float best = fabsf(a[col][col]);
        for (int r = col+1; r < 4; r++) if (fabsf(a[r][col]) > best) { best = fabsf(a[r][col]); pivot = r; }
        if (best < 1e-9f) return -1;
        if (pivot != col) for (int c = 0; c < 8; c++) { float t=a[col][c]; a[col][c]=a[pivot][c]; a[pivot][c]=t; }

        float diag = a[col][col];
        for (int c = 0; c < 8; c++) a[col][c] /= diag;
        for (int r = 0; r < 4; r++) {
            if (r == col) continue;
            float factor = a[r][col];
            for (int c = 0; c < 8; c++) a[r][c] -= factor * a[col][c];
        }
    }
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) out->m[r*4+c] = a[r][4+c];
    return 0;
}

/* transforma una normal correctamente bajo escala no uniforme: se necesita
 * la inversa-transpuesta de la parte 3x3, no la matriz directa. */
static RigPTVec3 rig_pt_transform_normal(const RigPTMat4 *inverse_transform, RigPTVec3 n_local) {
    const float *inv = inverse_transform->m;
    RigPTVec3 r;
    /* inversa-transpuesta: usar columnas de 'inv' como filas */
    r.x = inv[0]*n_local.x + inv[4]*n_local.y + inv[8]*n_local.z;
    r.y = inv[1]*n_local.x + inv[5]*n_local.y + inv[9]*n_local.z;
    r.z = inv[2]*n_local.x + inv[6]*n_local.y + inv[10]*n_local.z;
    return ptv_norm(r);
}

/* -------------------------------------------------------------------------
 * AABB
 * ---------------------------------------------------------------------- */
static RigPTAABB rig_pt_aabb_union(RigPTAABB a, RigPTAABB b) {
    RigPTAABB r;
    r.min.x = a.min.x < b.min.x ? a.min.x : b.min.x;
    r.min.y = a.min.y < b.min.y ? a.min.y : b.min.y;
    r.min.z = a.min.z < b.min.z ? a.min.z : b.min.z;
    r.max.x = a.max.x > b.max.x ? a.max.x : b.max.x;
    r.max.y = a.max.y > b.max.y ? a.max.y : b.max.y;
    r.max.z = a.max.z > b.max.z ? a.max.z : b.max.z;
    return r;
}

static int rig_pt_aabb_hit(RigPTAABB box, RigPTRay ray, float t_max_current) {
    float t0 = ray.t_min, t1 = t_max_current;
    float *o = &ray.origin.x, *d = &ray.dir.x, *bmin = &box.min.x, *bmax = &box.max.x;
    for (int a = 0; a < 3; a++) {
        float inv_d = 1.0f / d[a];
        float near_t = (bmin[a] - o[a]) * inv_d;
        float far_t  = (bmax[a] - o[a]) * inv_d;
        if (inv_d < 0.0f) { float tmp = near_t; near_t = far_t; far_t = tmp; }
        t0 = near_t > t0 ? near_t : t0;
        t1 = far_t < t1 ? far_t : t1;
        if (t1 <= t0) return 0;
    }
    return 1;
}

static RigPTAABB rig_pt_instance_world_aabb(const RigPTInstance *inst) {
    RigPTVec3 corners[8];
    RigPTVec3 mn = inst->local_aabb.min, mx = inst->local_aabb.max;
    corners[0]=(RigPTVec3){mn.x,mn.y,mn.z}; corners[1]=(RigPTVec3){mx.x,mn.y,mn.z};
    corners[2]=(RigPTVec3){mn.x,mx.y,mn.z}; corners[3]=(RigPTVec3){mx.x,mx.y,mn.z};
    corners[4]=(RigPTVec3){mn.x,mn.y,mx.z}; corners[5]=(RigPTVec3){mx.x,mn.y,mx.z};
    corners[6]=(RigPTVec3){mn.x,mx.y,mx.z}; corners[7]=(RigPTVec3){mx.x,mx.y,mx.z};

    RigPTAABB world;
    world.min = world.max = rig_pt_mat4_mul_point(&inst->transform, corners[0]);
    for (int i = 1; i < 8; i++) {
        RigPTVec3 p = rig_pt_mat4_mul_point(&inst->transform, corners[i]);
        if (p.x < world.min.x) world.min.x = p.x;
        if (p.x > world.max.x) world.max.x = p.x;
        if (p.y < world.min.y) world.min.y = p.y;
        if (p.y > world.max.y) world.max.y = p.y;
        if (p.z < world.min.z) world.min.z = p.z;
        if (p.z > world.max.z) world.max.z = p.z;
    }
    return world;
}

/* -------------------------------------------------------------------------
 * Gestion de escena e instancias
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_pt_scene_init(RigPTScene *scene) {
    if (!scene) return -1;
    memset(scene, 0, sizeof(*scene));
    scene->root_node = -1;
    return 0;
}

RIGCOM_PUBLIC int rig_pt_scene_add_instance(RigPTScene *scene, RigPTMat4 transform,
                                             RigPTAABB local_aabb, void *blas_ctx,
                                             RigPTBlasIntersectFn intersect_fn,
                                             RigPTBlasOccludedFn occluded_fn_or_null,
                                             int material_id) {
    if (!scene || !intersect_fn) return -1;
    if (scene->instance_count >= RIG_PT_MAX_INSTANCES) return -2;

    RigPTInstance *inst = &scene->instances[scene->instance_count];
    inst->transform = transform;
    if (rig_pt_mat4_inverse(&transform, &inst->inverse_transform) != 0) return -3; /* transform singular */
    inst->local_aabb = local_aabb;
    inst->blas_ctx = blas_ctx;
    inst->intersect_fn = intersect_fn;
    inst->occluded_fn = occluded_fn_or_null;
    inst->material_id = material_id;
    inst->world_aabb = rig_pt_instance_world_aabb(inst);

    scene->instance_count++;
    return scene->instance_count - 1;
}

RIGCOM_PUBLIC int rig_pt_scene_add_light(RigPTScene *scene, RigPTLight light) {
    if (!scene) return -1;
    if (scene->light_count >= RIG_PT_MAX_LIGHTS) return -2;
    scene->lights[scene->light_count++] = light;
    return scene->light_count - 1;
}

/* -------------------------------------------------------------------------
 * Construccion del TLAS: particion de mediana recursiva top-down sobre el
 * eje mas largo de la caja acumulada, igual principio que un BVH de
 * triangulos pero aplicado a N cajas de instancia (N tipicamente pequeno:
 * personajes en una escena, no millones de triangulos).
 * ---------------------------------------------------------------------- */
static int rig_pt_tlas_build_recursive(RigPTScene *scene, int *indices, int count) {
    int node_idx = scene->node_count++;
    RigPTTLASNode *node = &scene->nodes[node_idx];

    RigPTAABB bounds = scene->instances[indices[0]].world_aabb;
    for (int i = 1; i < count; i++) bounds = rig_pt_aabb_union(bounds, scene->instances[indices[i]].world_aabb);
    node->bounds = bounds;

    if (count == 1) {
        node->left = -1; node->right = -1; node->instance_index = indices[0];
        return node_idx;
    }

    RigPTVec3 extent = ptv_sub(bounds.max, bounds.min);
    int axis = 0;
    if (extent.y > extent.x && extent.y > extent.z) axis = 1;
    else if (extent.z > extent.x && extent.z > extent.y) axis = 2;

    /* ordenamiento por insercion simple sobre el centro de cada caja en el
     * eje elegido (N pequeno: suficiente, evita dependencias externas) */
    for (int i = 1; i < count; i++) {
        int key = indices[i];
        RigPTAABB kb = scene->instances[key].world_aabb;
        float key_center = axis==0 ? (kb.min.x+kb.max.x)*0.5f : axis==1 ? (kb.min.y+kb.max.y)*0.5f : (kb.min.z+kb.max.z)*0.5f;
        int j = i - 1;
        while (j >= 0) {
            RigPTAABB jb = scene->instances[indices[j]].world_aabb;
            float j_center = axis==0 ? (jb.min.x+jb.max.x)*0.5f : axis==1 ? (jb.min.y+jb.max.y)*0.5f : (jb.min.z+jb.max.z)*0.5f;
            if (j_center <= key_center) break;
            indices[j+1] = indices[j]; j--;
        }
        indices[j+1] = key;
    }

    int mid = count / 2;
    int left_idx = rig_pt_tlas_build_recursive(scene, indices, mid);
    int right_idx = rig_pt_tlas_build_recursive(scene, indices + mid, count - mid);
    /* nota: los indices de nodo hijo se guardan DESPUES de construirlos
     * porque el arreglo scene->nodes puede haberse reubicado logicamente
     * (no fisicamente: es un arreglo estatico, pero el orden de insercion
     * si cambia) -- se re-obtiene el puntero por seguridad. */
    scene->nodes[node_idx].left = left_idx;
    scene->nodes[node_idx].right = right_idx;
    scene->nodes[node_idx].instance_index = -1;
    return node_idx;
}

RIGCOM_PUBLIC int rig_pt_scene_build_tlas(RigPTScene *scene) {
    if (!scene) return -1;
    if (scene->instance_count == 0) return -2;
    scene->node_count = 0;

    static int indices[RIG_PT_MAX_INSTANCES];
    for (int i = 0; i < scene->instance_count; i++) indices[i] = i;

    scene->root_node = rig_pt_tlas_build_recursive(scene, indices, scene->instance_count);
    return 0;
}

/* -------------------------------------------------------------------------
 * Recorrido closest-hit: stack-based, transforma el rayo a espacio local
 * de cada instancia candidata, llama al BLAS del caller, y de vuelta a
 * espacio mundo con la instancia correcta.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_pt_scene_intersect(const RigPTScene *scene, RigPTRay world_ray, RigPTWorldHit *out_hit) {
    if (!scene || !out_hit || scene->root_node < 0) return -1;
    memset(out_hit, 0, sizeof(*out_hit));

    int stack[RIG_PT_STACK_SIZE];
    int sp = 0;
    stack[sp++] = scene->root_node;

    float closest_t = FLT_MAX;
    int found = 0;

    while (sp > 0) {
        int ni = stack[--sp];
        const RigPTTLASNode *node = &scene->nodes[ni];
        if (!rig_pt_aabb_hit(node->bounds, world_ray, closest_t)) continue;

        if (node->left < 0) {
            const RigPTInstance *inst = &scene->instances[node->instance_index];
            RigPTRay local_ray;
            local_ray.origin = rig_pt_mat4_mul_point(&inst->inverse_transform, world_ray.origin);
            local_ray.dir = rig_pt_mat4_mul_dir(&inst->inverse_transform, world_ray.dir);
            local_ray.t_min = world_ray.t_min;
            local_ray.t_max = closest_t;

            RigPTHit local_hit;
            if (inst->intersect_fn(inst->blas_ctx, local_ray, &local_hit) && local_hit.hit && local_hit.t < closest_t) {
                closest_t = local_hit.t;
                out_hit->t = local_hit.t;
                out_hit->position = rig_pt_mat4_mul_point(&inst->transform, local_hit.position);
                out_hit->normal = rig_pt_transform_normal(&inst->inverse_transform, local_hit.normal);
                out_hit->instance_index = node->instance_index;
                out_hit->material_id = local_hit.material_id;
                found = 1;
            }
        } else {
            if (sp < RIG_PT_STACK_SIZE - 1) { stack[sp++] = node->left; stack[sp++] = node->right; }
        }
    }
    return found;
}

/* -------------------------------------------------------------------------
 * Rayo de sombra: any-hit con corte temprano (no busca el mas cercano,
 * solo si hay CUALQUIER obstruccion) -- necesario para sombras entre
 * multiples instancias, mas eficiente que reusar closest-hit para esto.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_pt_scene_occluded(const RigPTScene *scene, RigPTRay world_ray) {
    if (!scene || scene->root_node < 0) return 0;

    int stack[RIG_PT_STACK_SIZE];
    int sp = 0;
    stack[sp++] = scene->root_node;

    while (sp > 0) {
        int ni = stack[--sp];
        const RigPTTLASNode *node = &scene->nodes[ni];
        if (!rig_pt_aabb_hit(node->bounds, world_ray, world_ray.t_max)) continue;

        if (node->left < 0) {
            const RigPTInstance *inst = &scene->instances[node->instance_index];
            RigPTRay local_ray;
            local_ray.origin = rig_pt_mat4_mul_point(&inst->inverse_transform, world_ray.origin);
            local_ray.dir = rig_pt_mat4_mul_dir(&inst->inverse_transform, world_ray.dir);
            local_ray.t_min = world_ray.t_min;
            local_ray.t_max = world_ray.t_max;

            if (inst->occluded_fn) {
                if (inst->occluded_fn(inst->blas_ctx, local_ray)) return 1;
            } else {
                RigPTHit h;
                if (inst->intersect_fn(inst->blas_ctx, local_ray, &h) && h.hit) return 1;
            }
        } else {
            if (sp < RIG_PT_STACK_SIZE - 1) { stack[sp++] = node->left; stack[sp++] = node->right; }
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Iluminacion directa compartida: evalua las luces de la escena contra un
 * punto/normal en espacio mundo, con test de oclusion multi-instancia.
 * Lambertiano simple; el shading de material avanzado sigue viviendo en
 * sovereign_pathtrace.c -- esto solo resuelve visibilidad+irradiancia.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC RigPTVec3 rig_pt_scene_direct_light(const RigPTScene *scene, RigPTVec3 world_pos, RigPTVec3 world_normal) {
    RigPTVec3 accum = {0,0,0};
    for (int i = 0; i < scene->light_count; i++) {
        const RigPTLight *L = &scene->lights[i];
        RigPTVec3 to_light;
        float dist = FLT_MAX;
        if (L->type == RIG_PT_LIGHT_DIRECTIONAL) {
            to_light = ptv_norm(L->position_or_dir);
        } else {
            RigPTVec3 delta = ptv_sub(L->position_or_dir, world_pos);
            dist = ptv_len(delta);
            to_light = dist > 1e-6f ? ptv_scale(delta, 1.0f/dist) : (RigPTVec3){0,1,0};
        }
        float ndotl = ptv_dot(world_normal, to_light);
        if (ndotl <= 0.0f) continue;

        RigPTRay shadow_ray;
        shadow_ray.origin = ptv_add(world_pos, ptv_scale(world_normal, 0.001f));
        shadow_ray.dir = to_light;
        shadow_ray.t_min = 0.0001f;
        shadow_ray.t_max = (dist == FLT_MAX) ? FLT_MAX : dist - 0.002f;

        if (rig_pt_scene_occluded(scene, shadow_ray)) continue;

        float atten = (L->type == RIG_PT_LIGHT_POINT) ? (1.0f / (dist*dist + 1e-4f)) : 1.0f;
        float k = L->intensity * ndotl * atten;
        accum = ptv_add(accum, ptv_scale(L->color, k));
    }
    return accum;
}

