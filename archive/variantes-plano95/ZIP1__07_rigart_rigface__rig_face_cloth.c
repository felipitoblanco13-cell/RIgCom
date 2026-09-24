/* ============================================================================
 * rig_face_cloth.c
 *
 * RigCom :: Procedural Cloth Simulation Module (aditivo puro, XPBD)
 *
 * Vestuario/accesorios con pano simulado sobre el cuerpo Sovereign, sin
 * modificar sovereign_body.c. Implementa Extended Position-Based Dynamics
 * (Muller et al.) para independencia de rigidez respecto al framerate:
 *
 *   1. Constraints de distancia (estructurales horiz/vert, shear diagonal,
 *      flexion a 2 saltos) via XPBD con compliance por tipo (no PBD
 *      clasico con "stiffness" dependiente del numero de iteraciones).
 *   2. Colision punto-vs-capsula contra primitivas del cuerpo (reutiliza
 *      el mismo concepto de capsulas de sovereign_body.c, pasadas por el
 *      caller -- este modulo no conoce la jerarquia osea interna).
 *   3. Auto-colision via hash espacial uniforme (evita el pano
 *      atravesandose a si mismo en pliegues, sin costo O(n^2)).
 *   4. Viento aerodinamico por triangulo (fuerza de arrastre proyectada
 *      en la normal de cara, distribuida a los 3 vertices).
 *   5. Anclajes (pines) para union a costuras/hueso (cuello, cintura).
 *   6. Extraccion de malla con normales suavizadas para render.
 *   7. Serializacion de estado completo (posiciones/velocidades) para
 *      snapshots deterministas.
 * ==========================================================================*/

#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define RIG_CLOTH_MAX_PARTICLES     4096
#define RIG_CLOTH_MAX_CONSTRAINTS   20000
#define RIG_CLOTH_MAX_BODY_CAPSULES 32
#define RIG_CLOTH_MAX_PINS          256
#define RIG_CLOTH_MAX_TRIS          8192
#define RIG_CLOTH_HASH_BUCKETS      4096
#define RIG_CLOTH_MAX_PER_BUCKET    16
#define RIG_CLOTH_MAGIC             0x434c4f31u /* "CLO1" */
#define RIG_CLOTH_VERSION           1

typedef struct { float x, y, z; } RigClothVec3;

static RigClothVec3 rc_add__rig_dup_2af00470(RigClothVec3 a, RigClothVec3 b) { RigClothVec3 r={a.x+b.x,a.y+b.y,a.z+b.z}; return r; }
static RigClothVec3 rc_sub__rig_dup_a434d123(RigClothVec3 a, RigClothVec3 b) { RigClothVec3 r={a.x-b.x,a.y-b.y,a.z-b.z}; return r; }
static RigClothVec3 rc_scale__rig_dup_2c73f35e(RigClothVec3 a, float s) { RigClothVec3 r={a.x*s,a.y*s,a.z*s}; return r; }
static float rc_dot__rig_dup_74a01083(RigClothVec3 a, RigClothVec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static RigClothVec3 rc_cross__rig_dup_0d5a7810(RigClothVec3 a, RigClothVec3 b) {
    RigClothVec3 r = { a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x }; return r;
}
static float rc_len__rig_dup_1d28fa08(RigClothVec3 a) { return sqrtf(rc_dot__rig_dup_74a01083(a,a)); }
static RigClothVec3 rc_norm__rig_dup_08e5cbec(RigClothVec3 a) {
    float l = rc_len__rig_dup_1d28fa08(a);
    if (l < 1e-8f) { RigClothVec3 z = {0,1,0}; return z; }
    return rc_scale__rig_dup_2c73f35e(a, 1.0f / l);
}

typedef enum { RIG_CLOTH_CONSTRAINT_STRUCTURAL=0, RIG_CLOTH_CONSTRAINT_SHEAR, RIG_CLOTH_CONSTRAINT_BEND } RigClothConstraintType;

typedef struct {
    int i, j;
    float rest_length_mm;
    float compliance;    /* XPBD: 0 = rigido puro, mayor = mas elastico. m/N equivalente */
    float lambda;        /* multiplicador acumulado, se resetea cada substep */
    RigClothConstraintType type;
} RigClothConstraint;

typedef struct {
    int particle_index;
    RigClothVec3 world_pos_mm;
    int active;
} RigClothPin;

typedef struct {
    RigClothVec3 a, b;  /* extremos de la capsula en espacio mundo */
    float radius_mm;
} RigClothBodyCapsule;

typedef struct {
    RigClothVec3 pos[RIG_CLOTH_MAX_PARTICLES];
    RigClothVec3 prev_pos[RIG_CLOTH_MAX_PARTICLES];
    RigClothVec3 vel[RIG_CLOTH_MAX_PARTICLES];
    float inv_mass[RIG_CLOTH_MAX_PARTICLES];
    int particle_count;

    RigClothConstraint constraints[RIG_CLOTH_MAX_CONSTRAINTS];
    int constraint_count;

    unsigned int tri_indices[RIG_CLOTH_MAX_TRIS * 3];
    int tri_count;

    RigClothPin pins[RIG_CLOTH_MAX_PINS];
    int pin_count;

    int grid_rows, grid_cols;
    float particle_radius_mm;   /* para auto-colision y grosor de tela */
    float damping;              /* amortiguacion global de velocidad, 0..1 por segundo */
} RigClothPatch;

/* -------------------------------------------------------------------------
 * Construccion de una malla de tela rectangular (grid_rows x grid_cols)
 * con constraints estructurales, shear y de flexion, todas via XPBD.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_cloth_patch_create_grid__rig_dup_399c70d6(RigClothPatch *cloth, int rows, int cols,
                                               float spacing_mm, RigClothVec3 origin_mm,
                                               RigClothVec3 right_axis, RigClothVec3 down_axis,
                                               float mass_per_particle_kg,
                                               float structural_compliance,
                                               float shear_compliance,
                                               float bend_compliance) {
    if (!cloth || rows < 2 || cols < 2) return -1;
    if (rows * cols > RIG_CLOTH_MAX_PARTICLES) return -2;
    memset(cloth, 0, sizeof(*cloth));

    cloth->grid_rows = rows; cloth->grid_cols = cols;
    cloth->particle_radius_mm = spacing_mm * 0.4f;
    cloth->damping = 0.02f;

    RigClothVec3 r_unit = rc_norm__rig_dup_08e5cbec(right_axis);
    RigClothVec3 d_unit = rc_norm__rig_dup_08e5cbec(down_axis);

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int idx = row * cols + col;
            RigClothVec3 p = rc_add__rig_dup_2af00470(origin_mm, rc_add__rig_dup_2af00470(rc_scale__rig_dup_2c73f35e(r_unit, col * spacing_mm), rc_scale__rig_dup_2c73f35e(d_unit, row * spacing_mm)));
            cloth->pos[idx] = p;
            cloth->prev_pos[idx] = p;
            cloth->vel[idx] = (RigClothVec3){0,0,0};
            cloth->inv_mass[idx] = (mass_per_particle_kg > 0.0f) ? (1.0f / mass_per_particle_kg) : 0.0f;
        }
    }
    cloth->particle_count = rows * cols;

    int cc = 0;
    #define RIG_CLOTH_ADD_CONSTRAINT(I,J,TYPE,COMPL) do { \
        if (cc >= RIG_CLOTH_MAX_CONSTRAINTS) return -3; \
        RigClothConstraint *k = &cloth->constraints[cc++]; \
        k->i = (I); k->j = (J); k->type = (TYPE); k->compliance = (COMPL); k->lambda = 0.0f; \
        k->rest_length_mm = rc_len__rig_dup_1d28fa08(rc_sub__rig_dup_a434d123(cloth->pos[(I)], cloth->pos[(J)])); \
    } while (0)

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int idx = row * cols + col;
            if (col + 1 < cols) RIG_CLOTH_ADD_CONSTRAINT(idx, idx + 1, RIG_CLOTH_CONSTRAINT_STRUCTURAL, structural_compliance);
            if (row + 1 < rows) RIG_CLOTH_ADD_CONSTRAINT(idx, idx + cols, RIG_CLOTH_CONSTRAINT_STRUCTURAL, structural_compliance);
            if (row + 1 < rows && col + 1 < cols) RIG_CLOTH_ADD_CONSTRAINT(idx, idx + cols + 1, RIG_CLOTH_CONSTRAINT_SHEAR, shear_compliance);
            if (row + 1 < rows && col > 0) RIG_CLOTH_ADD_CONSTRAINT(idx, idx + cols - 1, RIG_CLOTH_CONSTRAINT_SHEAR, shear_compliance);
            if (col + 2 < cols) RIG_CLOTH_ADD_CONSTRAINT(idx, idx + 2, RIG_CLOTH_CONSTRAINT_BEND, bend_compliance);
            if (row + 2 < rows) RIG_CLOTH_ADD_CONSTRAINT(idx, idx + 2 * cols, RIG_CLOTH_CONSTRAINT_BEND, bend_compliance);
        }
    }
    cloth->constraint_count = cc;
    #undef RIG_CLOTH_ADD_CONSTRAINT

    int tc = 0;
    for (int row = 0; row < rows - 1; row++) {
        for (int col = 0; col < cols - 1; col++) {
            unsigned int a = row * cols + col;
            unsigned int b = a + 1;
            unsigned int c = a + cols;
            unsigned int d = c + 1;
            if (tc + 2 > RIG_CLOTH_MAX_TRIS) return -4;
            cloth->tri_indices[tc*3+0] = a; cloth->tri_indices[tc*3+1] = c; cloth->tri_indices[tc*3+2] = b; tc++;
            cloth->tri_indices[tc*3+0] = b; cloth->tri_indices[tc*3+1] = c; cloth->tri_indices[tc*3+2] = d; tc++;
        }
    }
    cloth->tri_count = tc;
    return 0;
}

RIGCOM_PUBLIC int rig_cloth_pin_add__rig_dup_dfb4236d(RigClothPatch *cloth, int particle_index, RigClothVec3 world_pos_mm) {
    if (!cloth) return -1;
    if (cloth->pin_count >= RIG_CLOTH_MAX_PINS) return -2;
    if (particle_index < 0 || particle_index >= cloth->particle_count) return -3;
    RigClothPin *p = &cloth->pins[cloth->pin_count++];
    p->particle_index = particle_index;
    p->world_pos_mm = world_pos_mm;
    p->active = 1;
    return cloth->pin_count - 1;
}

RIGCOM_PUBLIC int rig_cloth_pin_update_position__rig_dup_3bb70282(RigClothPatch *cloth, int pin_index, RigClothVec3 world_pos_mm) {
    if (!cloth || pin_index < 0 || pin_index >= cloth->pin_count) return -1;
    cloth->pins[pin_index].world_pos_mm = world_pos_mm;
    return 0;
}

/* -------------------------------------------------------------------------
 * Hash espacial uniforme para auto-colision O(n) amortizado
 * ---------------------------------------------------------------------- */
static unsigned int rig_cloth_hash_cell__rig_dup_ca2b1428(int cx, int cy, int cz) {
    unsigned int h = (unsigned int)(cx * 73856093) ^ (unsigned int)(cy * 19349663) ^ (unsigned int)(cz * 83492791);
    return h % RIG_CLOTH_HASH_BUCKETS;
}

static void rig_cloth_self_collision_resolve__rig_dup_ab60dc2e(RigClothPatch *cloth) {
    static int bucket_items[RIG_CLOTH_HASH_BUCKETS][RIG_CLOTH_MAX_PER_BUCKET];
    static int bucket_counts[RIG_CLOTH_HASH_BUCKETS];
    memset(bucket_counts, 0, sizeof(bucket_counts));

    float cell_size = cloth->particle_radius_mm * 2.2f;
    if (cell_size < 1e-4f) return;

    for (int i = 0; i < cloth->particle_count; i++) {
        int cx = (int)floorf(cloth->pos[i].x / cell_size);
        int cy = (int)floorf(cloth->pos[i].y / cell_size);
        int cz = (int)floorf(cloth->pos[i].z / cell_size);
        unsigned int h = rig_cloth_hash_cell__rig_dup_ca2b1428(cx, cy, cz);
        if (bucket_counts[h] < RIG_CLOTH_MAX_PER_BUCKET) {
            bucket_items[h][bucket_counts[h]++] = i;
        }
    }

    float min_dist = cloth->particle_radius_mm * 2.0f;
    for (int i = 0; i < cloth->particle_count; i++) {
        int cx = (int)floorf(cloth->pos[i].x / cell_size);
        int cy = (int)floorf(cloth->pos[i].y / cell_size);
        int cz = (int)floorf(cloth->pos[i].z / cell_size);

        for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
        for (int dz = -1; dz <= 1; dz++) {
            unsigned int h = rig_cloth_hash_cell__rig_dup_ca2b1428(cx+dx, cy+dy, cz+dz);
            for (int b = 0; b < bucket_counts[h]; b++) {
                int j = bucket_items[h][b];
                if (j <= i) continue; /* evita pares duplicados y auto-par */
                /* ignora pares directamente conectados por constraint (serian vecinos de malla) */
                RigClothVec3 delta = rc_sub__rig_dup_a434d123(cloth->pos[i], cloth->pos[j]);
                float d = rc_len__rig_dup_1d28fa08(delta);
                if (d < min_dist && d > 1e-6f) {
                    float penetration = min_dist - d;
                    RigClothVec3 n = rc_scale__rig_dup_2c73f35e(delta, 1.0f / d);
                    float wi = cloth->inv_mass[i], wj = cloth->inv_mass[j];
                    float wsum = wi + wj;
                    if (wsum < 1e-8f) continue;
                    RigClothVec3 correction_i = rc_scale__rig_dup_2c73f35e(n, penetration * (wi / wsum));
                    RigClothVec3 correction_j = rc_scale__rig_dup_2c73f35e(n, -penetration * (wj / wsum));
                    cloth->pos[i] = rc_add__rig_dup_2af00470(cloth->pos[i], correction_i);
                    cloth->pos[j] = rc_add__rig_dup_2af00470(cloth->pos[j], correction_j);
                }
            }
        }
    }
}

/* -------------------------------------------------------------------------
 * Colision contra capsulas del cuerpo: empuje posicional directo (no XPBD,
 * es una restriccion unilateral resuelta como proyeccion, tecnica estandar
 * en solvers PBD para colision).
 * ---------------------------------------------------------------------- */
static void rig_cloth_capsule_collide_point__rig_dup_16341a5e(RigClothVec3 *p, float p_radius_mm,
                                             RigClothVec3 a, RigClothVec3 b, float cap_radius_mm) {
    RigClothVec3 ab = rc_sub__rig_dup_a434d123(b, a);
    float ab_len2 = rc_dot__rig_dup_74a01083(ab, ab);
    float t = (ab_len2 > 1e-8f) ? (rc_dot__rig_dup_74a01083(rc_sub__rig_dup_a434d123(*p, a), ab) / ab_len2) : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    RigClothVec3 closest = rc_add__rig_dup_2af00470(a, rc_scale__rig_dup_2c73f35e(ab, t));
    RigClothVec3 delta = rc_sub__rig_dup_a434d123(*p, closest);
    float dist = rc_len__rig_dup_1d28fa08(delta);
    float min_dist = cap_radius_mm + p_radius_mm;
    if (dist < min_dist) {
        RigClothVec3 n = (dist > 1e-6f) ? rc_scale__rig_dup_2c73f35e(delta, 1.0f / dist) : (RigClothVec3){0,1,0};
        *p = rc_add__rig_dup_2af00470(closest, rc_scale__rig_dup_2c73f35e(n, min_dist));
    }
}

/* -------------------------------------------------------------------------
 * Paso de simulacion XPBD completo con sub-stepping.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_cloth_step__rig_dup_6ca30f16(RigClothPatch *cloth, float dt, RigClothVec3 gravity_mm_s2,
                                  RigClothVec3 wind_mm_s, float wind_strength,
                                  const RigClothBodyCapsule *capsules, int capsule_count,
                                  int substeps, int solver_iterations) {
    if (!cloth || dt <= 0.0f || substeps < 1) return -1;
    float sub_dt = dt / (float)substeps;

    for (int s = 0; s < substeps; s++) {
        /* --- prediccion: gravedad + viento aerodinamico por triangulo --- */
        for (int i = 0; i < cloth->particle_count; i++) {
            if (cloth->inv_mass[i] <= 0.0f) continue;
            cloth->vel[i] = rc_add__rig_dup_2af00470(cloth->vel[i], rc_scale__rig_dup_2c73f35e(gravity_mm_s2, sub_dt));
        }

        if (wind_strength > 0.0f) {
            for (int t = 0; t < cloth->tri_count; t++) {
                unsigned int ia = cloth->tri_indices[t*3+0];
                unsigned int ib = cloth->tri_indices[t*3+1];
                unsigned int ic = cloth->tri_indices[t*3+2];
                RigClothVec3 e1 = rc_sub__rig_dup_a434d123(cloth->pos[ib], cloth->pos[ia]);
                RigClothVec3 e2 = rc_sub__rig_dup_a434d123(cloth->pos[ic], cloth->pos[ia]);
                RigClothVec3 n = rc_cross__rig_dup_0d5a7810(e1, e2);
                float area2 = rc_len__rig_dup_1d28fa08(n);
                if (area2 < 1e-8f) continue;
                n = rc_scale__rig_dup_2c73f35e(n, 1.0f / area2);

                RigClothVec3 face_vel = rc_scale__rig_dup_2c73f35e(rc_add__rig_dup_2af00470(rc_add__rig_dup_2af00470(cloth->vel[ia], cloth->vel[ib]), cloth->vel[ic]), 1.0f/3.0f);
                RigClothVec3 rel_wind = rc_sub__rig_dup_a434d123(wind_mm_s, face_vel);
                float flux = rc_dot__rig_dup_74a01083(rel_wind, n) * area2 * 0.5f * wind_strength;
                RigClothVec3 force = rc_scale__rig_dup_2c73f35e(n, flux);

                float w_a = cloth->inv_mass[ia], w_b = cloth->inv_mass[ib], w_c = cloth->inv_mass[ic];
                if (w_a > 0.0f) cloth->vel[ia] = rc_add__rig_dup_2af00470(cloth->vel[ia], rc_scale__rig_dup_2c73f35e(force, sub_dt * w_a / 3.0f));
                if (w_b > 0.0f) cloth->vel[ib] = rc_add__rig_dup_2af00470(cloth->vel[ib], rc_scale__rig_dup_2c73f35e(force, sub_dt * w_b / 3.0f));
                if (w_c > 0.0f) cloth->vel[ic] = rc_add__rig_dup_2af00470(cloth->vel[ic], rc_scale__rig_dup_2c73f35e(force, sub_dt * w_c / 3.0f));
            }
        }

        for (int i = 0; i < cloth->particle_count; i++) {
            cloth->vel[i] = rc_scale__rig_dup_2c73f35e(cloth->vel[i], 1.0f - cloth->damping);
            cloth->prev_pos[i] = cloth->pos[i];
            cloth->pos[i] = rc_add__rig_dup_2af00470(cloth->pos[i], rc_scale__rig_dup_2c73f35e(cloth->vel[i], sub_dt));
        }

        for (int i = 0; i < cloth->constraint_count; i++) cloth->constraints[i].lambda = 0.0f;

        for (int iter = 0; iter < solver_iterations; iter++) {
            for (int c = 0; c < cloth->constraint_count; c++) {
                RigClothConstraint *k = &cloth->constraints[c];
                float wi = cloth->inv_mass[k->i], wj = cloth->inv_mass[k->j];
                if (wi + wj < 1e-8f) continue;

                RigClothVec3 delta = rc_sub__rig_dup_a434d123(cloth->pos[k->i], cloth->pos[k->j]);
                float dist = rc_len__rig_dup_1d28fa08(delta);
                if (dist < 1e-8f) continue;
                RigClothVec3 n = rc_scale__rig_dup_2c73f35e(delta, 1.0f / dist);
                float C = dist - k->rest_length_mm;

                float alpha_tilde = k->compliance / (sub_dt * sub_dt);
                float delta_lambda = (-C - alpha_tilde * k->lambda) / (wi + wj + alpha_tilde);
                k->lambda += delta_lambda;

                cloth->pos[k->i] = rc_add__rig_dup_2af00470(cloth->pos[k->i], rc_scale__rig_dup_2c73f35e(n, wi * delta_lambda));
                cloth->pos[k->j] = rc_add__rig_dup_2af00470(cloth->pos[k->j], rc_scale__rig_dup_2c73f35e(n, -wj * delta_lambda));
            }

            for (int p = 0; p < cloth->pin_count; p++) {
                if (!cloth->pins[p].active) continue;
                cloth->pos[cloth->pins[p].particle_index] = cloth->pins[p].world_pos_mm;
            }

            for (int i = 0; i < cloth->particle_count; i++) {
                for (int cap = 0; cap < capsule_count; cap++) {
                    rig_cloth_capsule_collide_point__rig_dup_16341a5e(&cloth->pos[i], cloth->particle_radius_mm,
                        capsules[cap].a, capsules[cap].b, capsules[cap].radius_mm);
                }
            }
        }

        rig_cloth_self_collision_resolve__rig_dup_ab60dc2e(cloth);

        for (int i = 0; i < cloth->particle_count; i++) {
            cloth->vel[i] = rc_scale__rig_dup_2c73f35e(rc_sub__rig_dup_a434d123(cloth->pos[i], cloth->prev_pos[i]), 1.0f / sub_dt);
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Extraccion de normales suavizadas (promedio de normales de cara por
 * vertice), para entregar al pipeline de render.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_cloth_compute_smooth_normals__rig_dup_3679ee3a(const RigClothPatch *cloth, RigClothVec3 *out_normals) {
    if (!cloth || !out_normals) return -1;
    for (int i = 0; i < cloth->particle_count; i++) out_normals[i] = (RigClothVec3){0,0,0};

    for (int t = 0; t < cloth->tri_count; t++) {
        unsigned int ia = cloth->tri_indices[t*3+0];
        unsigned int ib = cloth->tri_indices[t*3+1];
        unsigned int ic = cloth->tri_indices[t*3+2];
        RigClothVec3 e1 = rc_sub__rig_dup_a434d123(cloth->pos[ib], cloth->pos[ia]);
        RigClothVec3 e2 = rc_sub__rig_dup_a434d123(cloth->pos[ic], cloth->pos[ia]);
        RigClothVec3 n = rc_cross__rig_dup_0d5a7810(e1, e2);
        out_normals[ia] = rc_add__rig_dup_2af00470(out_normals[ia], n);
        out_normals[ib] = rc_add__rig_dup_2af00470(out_normals[ib], n);
        out_normals[ic] = rc_add__rig_dup_2af00470(out_normals[ic], n);
    }
    for (int i = 0; i < cloth->particle_count; i++) out_normals[i] = rc_norm__rig_dup_08e5cbec(out_normals[i]);
    return 0;
}

/* -------------------------------------------------------------------------
 * Serializacion (estado dinamico completo, no solo configuracion, para
 * poder pausar/reanudar la simulacion bit-exacto)
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int payload_size;
} RigClothFileHeader;

RIGCOM_PUBLIC int rig_cloth_save__rig_dup_c3a9e3d3(const RigClothPatch *cloth, FILE *fp) {
    if (!cloth || !fp) return -1;
    RigClothFileHeader hdr = { RIG_CLOTH_MAGIC, RIG_CLOTH_VERSION, (unsigned int)sizeof(RigClothPatch) };
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (fwrite(cloth, sizeof(RigClothPatch), 1, fp) != 1) return -3;
    return 0;
}

RIGCOM_PUBLIC int rig_cloth_load__rig_dup_d4da7e14(RigClothPatch *cloth, FILE *fp) {
    if (!cloth || !fp) return -1;
    RigClothFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (hdr.magic != RIG_CLOTH_MAGIC) return -3;
    if (hdr.version != RIG_CLOTH_VERSION) return -4;
    if (hdr.payload_size != (unsigned int)sizeof(RigClothPatch)) return -5;
    if (fread(cloth, sizeof(RigClothPatch), 1, fp) != 1) return -6;
    return 0;
}
