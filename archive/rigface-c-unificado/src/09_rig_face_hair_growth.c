/* ==========================================================================
 * 09_rig_face_hair_growth.c   —   RIGFACE :: C UNIFICADO
 *
 * Copia canonica : src_raw/rig_face_hair_growth-1.c
 * Copias fundidas: 1
 * Funciones      : 12      Unidades injertadas: 0      Variantes: 0
 *
 * Copia canonica preservada verbatim. Ninguna funcion eliminada,
 * reducida ni omitida. Las divergencias de cuerpo bajo un mismo
 * nombre se conservan como <nombre>__vN con su procedencia.
 * ========================================================================== */
/* ============================================================================
 * rig_face_hair_growth.c
 *
 * RigCom :: Hair Growth Cycle Module (aditivo puro)
 *
 * Extiende el cabello ESTATICO (longitud fija, Marschner/Chiang en ng_hair.c)
 * con:
 *   1. Ciclo folicular real por hebra/mecha: anagen (crecimiento) -> catagen
 *      (regresion) -> telogen (reposo) -> exogen (caida) -> anagen de nuevo,
 *      con duraciones estocasticas por region (cuero cabelludo vs barba vs
 *      cejas, que tienen ciclos de duracion muy distinta en la realidad).
 *   2. Canas progresivas ligadas a la edad, con distribucion NO uniforme
 *      (sienes primero, luego coronilla, luego resto), coherente con
 *      RIG_TRAIT_HAIR_MELANIN_EUMELANIN/PHEOMELANIN del modulo de genetica.
 *   3. Generacion de GLSL fragment snippet que aplica el factor de canicie
 *      y el largo por-mecha al shader Marschner existente (aditivo).
 *   4. Serializacion binaria del estado folicular completo.
 * ==========================================================================*/

#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define RIG_HAIR_MAX_FOLLICLE_GROUPS 256
#define RIG_HAIR_GROWTH_MAGIC   0x48475231u /* "HGR1" */
#define RIG_HAIR_GROWTH_VERSION 1

typedef enum {
    RIG_FOLLICLE_ANAGEN = 0,   /* crecimiento activo */
    RIG_FOLLICLE_CATAGEN,      /* regresion, ~2-3 semanas */
    RIG_FOLLICLE_TELOGEN,      /* reposo, ~3 meses */
    RIG_FOLLICLE_EXOGEN        /* caida activa, transitorio */
} RigFollicleStage;

typedef enum {
    RIG_HAIR_REGION_SCALP = 0,
    RIG_HAIR_REGION_EYEBROW,
    RIG_HAIR_REGION_EYELASH,
    RIG_HAIR_REGION_BEARD,
    RIG_HAIR_REGION_BODY,
    RIG_HAIR_REGION_COUNT
} RigHairRegion;

/* Duraciones medias en dias por region (fuente: rangos fisiologicos tipicos
 * de ciclo piloso humano). Cuero cabelludo: anagen largo (años); cejas/
 * pestanas: anagen corto (semanas), por eso nunca alcanzan gran longitud. */
typedef struct {
    float anagen_days_mean;
    float anagen_days_stddev;
    float catagen_days_mean;
    float telogen_days_mean;
    float growth_rate_mm_per_day;
    float max_length_mm;
} RigHairRegionProfile;

static const RigHairRegionProfile RIG_HAIR_REGION_PROFILES[RIG_HAIR_REGION_COUNT] = {
    /* anagen_mean anagen_sd catagen telogen  rate    max_len */
    { 1460.0f,     400.0f,   21.0f,  100.0f, 0.35f,  900.0f  }, /* scalp: ~4 anios, 0.35mm/dia */
    {   60.0f,      15.0f,   14.0f,  90.0f,  0.16f,   12.0f  }, /* eyebrow */
    {   35.0f,      10.0f,   14.0f,  100.0f, 0.12f,    10.0f  }, /* eyelash */
    {  365.0f,     100.0f,   21.0f,  60.0f,  0.30f,   180.0f }, /* beard */
    {  180.0f,      60.0f,   21.0f,  80.0f,  0.20f,    15.0f  }  /* body */
};

typedef struct {
    RigHairRegion region;
    RigFollicleStage stage;
    float stage_age_days;
    float stage_duration_days;   /* duracion asignada (estocastica) para esta pasada del ciclo */
    float current_length_mm;
    float u, v;                  /* posicion UV en el scalp/region map */
    unsigned int rng_state;
    int   active;                /* 0 = foliculo inactivo permanentemente (ej. alopecia cicatricial) */
} RigFollicleGroup;

typedef struct {
    RigFollicleGroup groups[RIG_HAIR_MAX_FOLLICLE_GROUPS];
    int group_count;

    float age_years;             /* edad del sujeto, alimenta encanecimiento */
    float graying_onset_age;     /* edad a la que empieza a encanecer (variable genetica) */
    float graying_rate;          /* velocidad de progreso una vez iniciado (0..1 por año, tipico 0.05-0.15) */
    float graying_temple_bias;   /* que tan adelantadas van sienes vs resto, 0..1 */
} RigHairGrowthState;

/* -------------------------------------------------------------------------
 * PRNG local
 * ---------------------------------------------------------------------- */
static unsigned int rig_hair_xorshift32(unsigned int *state) {
    unsigned int x = *state ? *state : 0x1234567u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *state = x;
    return x;
}
static float rig_hair_rand01(unsigned int *state) {
    return (float)(rig_hair_xorshift32(state) & 0x00FFFFFFu) / (float)0x01000000u;
}
static float rig_hair_randn(unsigned int *state) {
    float u1 = rig_hair_rand01(state); if (u1 < 1e-7f) u1 = 1e-7f;
    float u2 = rig_hair_rand01(state);
    return sqrtf(-2.0f * logf(u1)) * cosf(6.283185307f * u2);
}
static float rig_hair_clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

/* -------------------------------------------------------------------------
 * Inicializacion
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_hair_growth_state_init(RigHairGrowthState *st, unsigned int seed) {
    if (!st) return -1;
    memset(st, 0, sizeof(*st));
    unsigned int rng = seed ? seed : 0xC0FFEEu;
    st->age_years = 25.0f;
    st->graying_onset_age = 28.0f + rig_hair_rand01(&rng) * 20.0f; /* 28-48, variabilidad genetica */
    st->graying_rate = 0.06f + rig_hair_rand01(&rng) * 0.06f;      /* 0.06 - 0.12 /anio */
    st->graying_temple_bias = 0.6f + rig_hair_rand01(&rng) * 0.3f;
    return 0;
}

RIGCOM_PUBLIC int rig_hair_follicle_add(RigHairGrowthState *st, RigHairRegion region,
                                         float u, float v, unsigned int seed) {
    if (!st) return -1;
    if (st->group_count >= RIG_HAIR_MAX_FOLLICLE_GROUPS) return -2;
    if (region < 0 || region >= RIG_HAIR_REGION_COUNT) return -3;

    RigFollicleGroup *g = &st->groups[st->group_count++];
    memset(g, 0, sizeof(*g));
    g->region = region;
    g->u = u; g->v = v;
    g->rng_state = seed ? seed : (unsigned int)(st->group_count * 2654435761u);
    g->active = 1;

    /* fase inicial aleatoria dentro del ciclo, para que no todas las hebras
     * nazcan sincronizadas (asi es realmente el cabello humano: mosaico) */
    const RigHairRegionProfile *prof = &RIG_HAIR_REGION_PROFILES[region];
    float roll = rig_hair_rand01(&g->rng_state);
    float total_cycle = prof->anagen_days_mean + prof->catagen_days_mean + prof->telogen_days_mean;
    float point = roll * total_cycle;

    if (point < prof->anagen_days_mean) {
        g->stage = RIG_FOLLICLE_ANAGEN;
        g->stage_duration_days = prof->anagen_days_mean + rig_hair_randn(&g->rng_state) * prof->anagen_days_stddev;
        if (g->stage_duration_days < 10.0f) g->stage_duration_days = 10.0f;
        g->stage_age_days = point;
        g->current_length_mm = point * prof->growth_rate_mm_per_day;
    } else if (point < prof->anagen_days_mean + prof->catagen_days_mean) {
        g->stage = RIG_FOLLICLE_CATAGEN;
        g->stage_duration_days = prof->catagen_days_mean;
        g->stage_age_days = point - prof->anagen_days_mean;
        g->current_length_mm = prof->anagen_days_mean * prof->growth_rate_mm_per_day;
    } else {
        g->stage = RIG_FOLLICLE_TELOGEN;
        g->stage_duration_days = prof->telogen_days_mean;
        g->stage_age_days = point - prof->anagen_days_mean - prof->catagen_days_mean;
        g->current_length_mm = prof->anagen_days_mean * prof->growth_rate_mm_per_day;
    }
    if (g->current_length_mm > prof->max_length_mm) g->current_length_mm = prof->max_length_mm;
    return st->group_count - 1;
}

/* -------------------------------------------------------------------------
 * Avance del ciclo folicular. dt en dias (fraccional permitido).
 * ---------------------------------------------------------------------- */
static void rig_hair_advance_one(RigFollicleGroup *g, const RigHairRegionProfile *prof, float dt) {
    if (!g->active) return;
    g->stage_age_days += dt;

    if (g->stage == RIG_FOLLICLE_ANAGEN) {
        g->current_length_mm += prof->growth_rate_mm_per_day * dt;
        if (g->current_length_mm > prof->max_length_mm) g->current_length_mm = prof->max_length_mm;
        if (g->stage_age_days >= g->stage_duration_days) {
            g->stage = RIG_FOLLICLE_CATAGEN;
            g->stage_age_days = 0.0f;
            g->stage_duration_days = prof->catagen_days_mean;
        }
    } else if (g->stage == RIG_FOLLICLE_CATAGEN) {
        /* leve retraccion durante catagen (~10% de la longitud) */
        float shrink = prof->growth_rate_mm_per_day * 0.15f * dt;
        g->current_length_mm -= shrink;
        if (g->current_length_mm < 0.0f) g->current_length_mm = 0.0f;
        if (g->stage_age_days >= g->stage_duration_days) {
            g->stage = RIG_FOLLICLE_TELOGEN;
            g->stage_age_days = 0.0f;
            g->stage_duration_days = prof->telogen_days_mean;
        }
    } else if (g->stage == RIG_FOLLICLE_TELOGEN) {
        if (g->stage_age_days >= g->stage_duration_days) {
            g->stage = RIG_FOLLICLE_EXOGEN;
            g->stage_age_days = 0.0f;
            g->stage_duration_days = 3.0f + rig_hair_rand01(&g->rng_state) * 7.0f; /* caida: dias */
        }
    } else { /* EXOGEN */
        if (g->stage_age_days >= g->stage_duration_days) {
            g->current_length_mm = 0.0f;
            g->stage = RIG_FOLLICLE_ANAGEN;
            g->stage_age_days = 0.0f;
            g->stage_duration_days = prof->anagen_days_mean +
                rig_hair_randn(&g->rng_state) * prof->anagen_days_stddev;
            if (g->stage_duration_days < 10.0f) g->stage_duration_days = 10.0f;
        }
    }
}

RIGCOM_PUBLIC int rig_hair_growth_update(RigHairGrowthState *st, float dt_days) {
    if (!st || dt_days <= 0.0f) return -1;
    st->age_years += dt_days / 365.25f;
    for (int i = 0; i < st->group_count; i++) {
        RigFollicleGroup *g = &st->groups[i];
        if (!g->active) continue;
        rig_hair_advance_one(g, &RIG_HAIR_REGION_PROFILES[g->region], dt_days);
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Canicie progresiva: sienes primero, luego coronilla, luego el resto.
 * Devuelve fraccion de canas 0..1 para un foliculo dado.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC float rig_hair_graying_fraction(const RigHairGrowthState *st, const RigFollicleGroup *g) {
    if (!st || !g) return 0.0f;
    if (st->age_years <= st->graying_onset_age) return 0.0f;

    float years_since_onset = st->age_years - st->graying_onset_age;
    float base_progress = rig_hair_clamp01(years_since_onset * st->graying_rate);

    /* distancia UV al centro de la sien (aprox u=0.15/0.85, v=0.45 en un
     * mapa scalp tipico); mas cerca de la sien = progresa antes */
    float temple_dist_left  = sqrtf((g->u - 0.15f) * (g->u - 0.15f) + (g->v - 0.45f) * (g->v - 0.45f));
    float temple_dist_right = sqrtf((g->u - 0.85f) * (g->u - 0.85f) + (g->v - 0.45f) * (g->v - 0.45f));
    float temple_dist = temple_dist_left < temple_dist_right ? temple_dist_left : temple_dist_right;
    float temple_proximity = rig_hair_clamp01(1.0f - temple_dist / 0.5f);

    float region_bias = 1.0f;
    if (g->region == RIG_HAIR_REGION_EYEBROW || g->region == RIG_HAIR_REGION_EYELASH) {
        region_bias = 0.4f; /* cejas/pestanas encanecen mas tarde que el cuero cabelludo */
    } else if (g->region == RIG_HAIR_REGION_BEARD) {
        region_bias = 1.15f; /* la barba suele encanecer ANTES que el cuero cabelludo */
    }

    float local_advance = base_progress * (1.0f + st->graying_temple_bias * temple_proximity) * region_bias;
    return rig_hair_clamp01(local_advance);
}

/* -------------------------------------------------------------------------
 * Codegen GLSL: aplica largo por-mecha (para geometria instanciada tipo
 * strand) y factor de canicie (mezcla con blanco/gris, reduce ambos
 * eumelanina y feomelanina proporcionalmente) sobre el shader Marschner
 * existente de ng_hair.c
 * ---------------------------------------------------------------------- */
#define RIG_HAIR_GROWTH_SRC_MAX 8192
RIGCOM_PUBLIC int rig_hair_growth_generate_shader(char *out_glsl, int max_len) {
    if (!out_glsl || max_len <= 0) return -1;
    int n = snprintf(out_glsl, (size_t)max_len,
        "// === rig_face_hair_growth :: fragment/vertex snippet (aditivo) ===\n"
        "// Se aplica ANTES del shading Marschner/Chiang existente:\n"
        "// 1) recorta la mecha a su longitud actual de ciclo\n"
        "// 2) mezcla la melanina base hacia blanco segun canicie\n"
        "attribute float a_hair_length_fraction;   // 0..1, largo actual / largo maximo\n"
        "attribute float a_hair_graying_fraction;  // 0..1, canicie local\n"
        "varying float v_hair_graying;\n"
        "\n"
        "void rig_hair_growth_vertex(inout vec3 strandPos, float strandParamT) {\n"
        "    // descarta geometria mas alla del largo actual (colapsa a raiz)\n"
        "    float clip = step(strandParamT, a_hair_length_fraction);\n"
        "    strandPos = mix(strandPos * 0.0, strandPos, clip);\n"
        "    v_hair_graying = a_hair_graying_fraction;\n"
        "}\n"
        "\n"
        "vec3 rig_hair_growth_apply_graying(vec3 eumelaninColor, vec3 pheomelaninColor) {\n"
        "    vec3 baseColor = eumelaninColor + pheomelaninColor;\n"
        "    vec3 grayColor = vec3(0.72, 0.71, 0.70); // canas: ligero tinte azulado-frio\n"
        "    return mix(baseColor, grayColor, clamp(v_hair_graying, 0.0, 1.0));\n"
        "}\n");
    if (n < 0 || n >= max_len) return -2;
    return n;
}

/* -------------------------------------------------------------------------
 * Serializacion binaria
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int payload_size;
} RigHairGrowthFileHeader;

RIGCOM_PUBLIC int rig_hair_growth_save(const RigHairGrowthState *st, FILE *fp) {
    if (!st || !fp) return -1;
    RigHairGrowthFileHeader hdr = { RIG_HAIR_GROWTH_MAGIC, RIG_HAIR_GROWTH_VERSION,
                                     (unsigned int)sizeof(RigHairGrowthState) };
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (fwrite(st, sizeof(RigHairGrowthState), 1, fp) != 1) return -3;
    return 0;
}

RIGCOM_PUBLIC int rig_hair_growth_load(RigHairGrowthState *st, FILE *fp) {
    if (!st || !fp) return -1;
    RigHairGrowthFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (hdr.magic != RIG_HAIR_GROWTH_MAGIC) return -3;
    if (hdr.version != RIG_HAIR_GROWTH_VERSION) return -4;
    if (hdr.payload_size != (unsigned int)sizeof(RigHairGrowthState)) return -5;
    if (fread(st, sizeof(RigHairGrowthState), 1, fp) != 1) return -6;
    return 0;
}

