/* ============================================================================
 * rig_face_soft_body.c
 *
 * RigCom :: Secondary Soft-Tissue Motion Module (aditivo puro)
 *
 * El cuerpo Sovereign (sovereign_body.c) es rigido por hueso. Este modulo
 * agrega fisica secundaria real de tejido blando (jiggle de grasa/musculo)
 * sin tocar el esqueleto ni el skinning existentes:
 *
 *   1. Por cada region "jiggle" (pecho, gluteos, brazo, mejilla, papada,
 *      abdomen...) se simula una masa-resorte amortiguada en el marco NO
 *      inercial del hueso padre: cuando el hueso acelera, la masa blanda
 *      se retrasa por la fuerza pseudo-inercial -m*a_hueso, exactamente
 *      como se comporta tejido real.
 *   2. Integracion semi-implicita con sub-stepping (estable incluso con
 *      resortes rigidos y dt grande de framerate variable).
 *   3. Anisotropia por eje (mas blando vertical que lateral, tipico de
 *      pecho/gluteos) y clamp de desplazamiento maximo (evita artefactos).
 *   4. Mascara de influencia por vertice via falloff gaussiano desde el
 *      ancla de la region, para aplicar el desplazamiento como offset
 *      POST-skinning (no reemplaza el dual-quaternion skinning existente,
 *      se suma despues).
 *   5. Serializacion de configuracion + estado (para snapshots/resume).
 * ==========================================================================*/

#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>

#define RIG_JIGGLE_MAX_REGIONS   32
#define RIG_SOFTBODY_MAGIC       0x534f4231u /* "SOB1" */
#define RIG_SOFTBODY_VERSION     1
#define RIG_JIGGLE_SUBSTEPS      4

typedef struct { float x, y, z; } RigSBVec3;

static RigSBVec3 rig_sb_add__rig_dup_49551bdb(RigSBVec3 a, RigSBVec3 b) { RigSBVec3 r = {a.x+b.x,a.y+b.y,a.z+b.z}; return r; }
static RigSBVec3 rig_sb_sub__rig_dup_50ce2827(RigSBVec3 a, RigSBVec3 b) { RigSBVec3 r = {a.x-b.x,a.y-b.y,a.z-b.z}; return r; }
static RigSBVec3 rig_sb_scale__rig_dup_8dafb423(RigSBVec3 a, float s) { RigSBVec3 r = {a.x*s,a.y*s,a.z*s}; return r; }
static float rig_sb_len__rig_dup_3368f716(RigSBVec3 a) { return sqrtf(a.x*a.x+a.y*a.y+a.z*a.z); }

typedef struct {
    char  name[32];
    int   bone_index;             /* indice logico del hueso padre en el sistema del caller */
    RigSBVec3 anchor_local_mm;    /* offset del ancla de la region respecto al hueso, en su espacio local */
    RigSBVec3 stiffness;          /* k por eje (N/m equivalente, unidades consistentes con mass/damping) */
    RigSBVec3 damping;            /* c por eje */
    float mass;                   /* masa efectiva de la region (kg equivalente) */
    float max_displacement_mm;
    float influence_radius_mm;    /* radio de la mascara de falloff sobre la malla */
} RigJiggleRegionConfig;

typedef struct {
    RigSBVec3 displacement_mm;    /* desplazamiento actual respecto al ancla, en espacio local del hueso */
    RigSBVec3 velocity_mm_s;
    RigSBVec3 prev_bone_world_pos;
    RigSBVec3 prev_bone_world_vel;
    int   initialized;
} RigJiggleRegionState;

typedef struct {
    RigJiggleRegionConfig config[RIG_JIGGLE_MAX_REGIONS];
    RigJiggleRegionState  state[RIG_JIGGLE_MAX_REGIONS];
    int region_count;
} RigSoftBodySystem;

/* -------------------------------------------------------------------------
 * Gestion de regiones
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_softbody_init__rig_dup_f1da9ea6(RigSoftBodySystem *sys) {
    if (!sys) return -1;
    memset(sys, 0, sizeof(*sys));
    return 0;
}

RIGCOM_PUBLIC int rig_softbody_region_add__rig_dup_7ba3a8be(RigSoftBodySystem *sys, const char *name, int bone_index,
                                           RigSBVec3 anchor_local_mm, RigSBVec3 stiffness,
                                           RigSBVec3 damping, float mass,
                                           float max_displacement_mm, float influence_radius_mm) {
    if (!sys || !name) return -1;
    if (sys->region_count >= RIG_JIGGLE_MAX_REGIONS) return -2;
    if (mass <= 0.0f) return -3;

    RigJiggleRegionConfig *c = &sys->config[sys->region_count];
    memset(c, 0, sizeof(*c));
    strncpy(c->name, name, sizeof(c->name) - 1);
    c->bone_index = bone_index;
    c->anchor_local_mm = anchor_local_mm;
    c->stiffness = stiffness;
    c->damping = damping;
    c->mass = mass;
    c->max_displacement_mm = max_displacement_mm;
    c->influence_radius_mm = influence_radius_mm;

    memset(&sys->state[sys->region_count], 0, sizeof(RigJiggleRegionState));
    sys->region_count++;
    return sys->region_count - 1;
}

/* Presets de conveniencia con rangos anatomicamente razonables (el caller
 * puede sobreescribir cualquier campo despues de agregarlos). */
RIGCOM_PUBLIC int rig_softbody_region_add_preset_chest__rig_dup_c99dd53c(RigSoftBodySystem *sys, int bone_index, int is_right) {
    RigSBVec3 anchor = { is_right ? 60.0f : -60.0f, 0.0f, 40.0f };
    RigSBVec3 k = { 180.0f, 90.0f, 140.0f };   /* mas blando verticalmente (y) */
    RigSBVec3 c = { 14.0f, 9.0f, 12.0f };
    char name[32]; snprintf(name, 32, "chest_%s", is_right ? "R" : "L");
    return rig_softbody_region_add__rig_dup_7ba3a8be(sys, name, bone_index, anchor, k, c, 0.9f, 35.0f, 90.0f);
}
RIGCOM_PUBLIC int rig_softbody_region_add_preset_glute__rig_dup_18e3509f(RigSoftBodySystem *sys, int bone_index, int is_right) {
    RigSBVec3 anchor = { is_right ? 50.0f : -50.0f, -30.0f, -20.0f };
    RigSBVec3 k = { 220.0f, 130.0f, 200.0f };
    RigSBVec3 c = { 16.0f, 11.0f, 15.0f };
    char name[32]; snprintf(name, 32, "glute_%s", is_right ? "R" : "L");
    return rig_softbody_region_add__rig_dup_7ba3a8be(sys, name, bone_index, anchor, k, c, 1.2f, 30.0f, 110.0f);
}
RIGCOM_PUBLIC int rig_softbody_region_add_preset_belly__rig_dup_cd373c9e(RigSoftBodySystem *sys, int bone_index) {
    RigSBVec3 anchor = { 0.0f, -20.0f, 30.0f };
    RigSBVec3 k = { 150.0f, 100.0f, 130.0f };
    RigSBVec3 c = { 13.0f, 10.0f, 12.0f };
    return rig_softbody_region_add__rig_dup_7ba3a8be(sys, "belly", bone_index, anchor, k, c, 1.0f, 40.0f, 120.0f);
}
RIGCOM_PUBLIC int rig_softbody_region_add_preset_cheek__rig_dup_7cf33d44(RigSoftBodySystem *sys, int bone_index, int is_right) {
    RigSBVec3 anchor = { is_right ? 35.0f : -35.0f, 10.0f, 60.0f };
    RigSBVec3 k = { 340.0f, 300.0f, 320.0f }; /* mucho mas rigido: tejido facial */
    RigSBVec3 c = { 22.0f, 20.0f, 21.0f };
    char name[32]; snprintf(name, 32, "cheek_%s", is_right ? "R" : "L");
    return rig_softbody_region_add__rig_dup_7ba3a8be(sys, name, bone_index, anchor, k, c, 0.08f, 6.0f, 35.0f);
}
RIGCOM_PUBLIC int rig_softbody_region_add_preset_upper_arm__rig_dup_014a3768(RigSoftBodySystem *sys, int bone_index, int is_right) {
    RigSBVec3 anchor = { is_right ? 90.0f : -90.0f, 0.0f, 0.0f };
    RigSBVec3 k = { 200.0f, 200.0f, 160.0f };
    RigSBVec3 c = { 15.0f, 15.0f, 13.0f };
    char name[32]; snprintf(name, 32, "upper_arm_%s", is_right ? "R" : "L");
    return rig_softbody_region_add__rig_dup_7ba3a8be(sys, name, bone_index, anchor, k, c, 0.5f, 20.0f, 70.0f);
}

/* -------------------------------------------------------------------------
 * Integracion fisica: masa-resorte amortiguada en el marco no inercial
 * del hueso padre, con sub-stepping semi-implicito para estabilidad.
 *
 *   m * x'' = -k*x - c*x' - m*a_hueso_local
 *
 * bone_world_pos: posicion mundial ACTUAL del hueso padre de cada region
 * (arreglo indexado por region, ya resuelto por el caller desde su propio
 * esqueleto -- este modulo no conoce la jerarquia osea, solo consume
 * posiciones resueltas, para no acoplarse a la representacion interna del
 * esqueleto existente).
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_softbody_update__rig_dup_86746c3c(RigSoftBodySystem *sys, const RigSBVec3 *bone_world_pos,
                                       int bone_world_pos_count, float dt) {
    if (!sys || !bone_world_pos || dt <= 0.0f) return -1;

    for (int i = 0; i < sys->region_count; i++) {
        RigJiggleRegionConfig *cfg = &sys->config[i];
        RigJiggleRegionState *st = &sys->state[i];

        if (cfg->bone_index < 0 || cfg->bone_index >= bone_world_pos_count) continue;
        RigSBVec3 bone_pos = bone_world_pos[cfg->bone_index];

        if (!st->initialized) {
            st->prev_bone_world_pos = bone_pos;
            st->prev_bone_world_vel = (RigSBVec3){0,0,0};
            st->displacement_mm = (RigSBVec3){0,0,0};
            st->velocity_mm_s = (RigSBVec3){0,0,0};
            st->initialized = 1;
            continue;
        }

        RigSBVec3 bone_vel = rig_sb_scale__rig_dup_8dafb423(rig_sb_sub__rig_dup_50ce2827(bone_pos, st->prev_bone_world_pos), 1.0f / dt);
        RigSBVec3 bone_accel = rig_sb_scale__rig_dup_8dafb423(rig_sb_sub__rig_dup_50ce2827(bone_vel, st->prev_bone_world_vel), 1.0f / dt);

        float sub_dt = dt / (float)RIG_JIGGLE_SUBSTEPS;
        for (int s = 0; s < RIG_JIGGLE_SUBSTEPS; s++) {
            RigSBVec3 spring_force = {
                -cfg->stiffness.x * st->displacement_mm.x,
                -cfg->stiffness.y * st->displacement_mm.y,
                -cfg->stiffness.z * st->displacement_mm.z
            };
            RigSBVec3 damping_force = {
                -cfg->damping.x * st->velocity_mm_s.x,
                -cfg->damping.y * st->velocity_mm_s.y,
                -cfg->damping.z * st->velocity_mm_s.z
            };
            RigSBVec3 inertial_force = rig_sb_scale__rig_dup_8dafb423(bone_accel, -cfg->mass);

            RigSBVec3 total_force = rig_sb_add__rig_dup_49551bdb(rig_sb_add__rig_dup_49551bdb(spring_force, damping_force), inertial_force);
            RigSBVec3 accel = rig_sb_scale__rig_dup_8dafb423(total_force, 1.0f / cfg->mass);

            /* semi-implicito: actualiza velocidad primero, luego posicion con la nueva velocidad */
            st->velocity_mm_s = rig_sb_add__rig_dup_49551bdb(st->velocity_mm_s, rig_sb_scale__rig_dup_8dafb423(accel, sub_dt));
            st->displacement_mm = rig_sb_add__rig_dup_49551bdb(st->displacement_mm, rig_sb_scale__rig_dup_8dafb423(st->velocity_mm_s, sub_dt));
        }

        /* clamp de seguridad: evita explosion numerica ante aceleraciones extremas */
        float disp_len = rig_sb_len__rig_dup_3368f716(st->displacement_mm);
        if (disp_len > cfg->max_displacement_mm && disp_len > 1e-6f) {
            float scale = cfg->max_displacement_mm / disp_len;
            st->displacement_mm = rig_sb_scale__rig_dup_8dafb423(st->displacement_mm, scale);
            st->velocity_mm_s = rig_sb_scale__rig_dup_8dafb423(st->velocity_mm_s, 0.7f); /* disipa energia extra en el clamp */
        }

        st->prev_bone_world_pos = bone_pos;
        st->prev_bone_world_vel = bone_vel;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Aplicacion a un vertice: offset ponderado por falloff gaussiano desde
 * el ancla de la region, en espacio mundo (post-skinning, aditivo puro).
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_softbody_eval_vertex_offset__rig_dup_76e23a82(const RigSoftBodySystem *sys, int region_index,
                                                   RigSBVec3 vertex_world_pos_mm,
                                                   RigSBVec3 region_anchor_world_pos_mm,
                                                   RigSBVec3 *out_offset_mm) {
    if (!sys || !out_offset_mm || region_index < 0 || region_index >= sys->region_count) return -1;
    const RigJiggleRegionConfig *cfg = &sys->config[region_index];
    const RigJiggleRegionState *st = &sys->state[region_index];

    RigSBVec3 delta = rig_sb_sub__rig_dup_50ce2827(vertex_world_pos_mm, region_anchor_world_pos_mm);
    float dist = rig_sb_len__rig_dup_3368f716(delta);
    float sigma = cfg->influence_radius_mm * 0.5f;
    float weight = expf(-(dist * dist) / (2.0f * sigma * sigma + 1e-6f));

    *out_offset_mm = rig_sb_scale__rig_dup_8dafb423(st->displacement_mm, weight);
    return 0;
}

/* -------------------------------------------------------------------------
 * Serializacion (config + estado, para snapshot/resume determinista)
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int payload_size;
} RigSoftBodyFileHeader;

RIGCOM_PUBLIC int rig_softbody_save__rig_dup_8cd25567(const RigSoftBodySystem *sys, FILE *fp) {
    if (!sys || !fp) return -1;
    RigSoftBodyFileHeader hdr = { RIG_SOFTBODY_MAGIC, RIG_SOFTBODY_VERSION,
                                   (unsigned int)sizeof(RigSoftBodySystem) };
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (fwrite(sys, sizeof(RigSoftBodySystem), 1, fp) != 1) return -3;
    return 0;
}

RIGCOM_PUBLIC int rig_softbody_load__rig_dup_68cb42cc(RigSoftBodySystem *sys, FILE *fp) {
    if (!sys || !fp) return -1;
    RigSoftBodyFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (hdr.magic != RIG_SOFTBODY_MAGIC) return -3;
    if (hdr.version != RIG_SOFTBODY_VERSION) return -4;
    if (hdr.payload_size != (unsigned int)sizeof(RigSoftBodySystem)) return -5;
    if (fread(sys, sizeof(RigSoftBodySystem), 1, fp) != 1) return -6;
    return 0;
}
