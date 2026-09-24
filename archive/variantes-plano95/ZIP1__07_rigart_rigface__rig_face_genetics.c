/* ============================================================================
 * rig_face_genetics.c
 *
 * RigCom :: Genetics / Parametric Identity Space Module (aditivo puro)
 *
 * 1. Motor de herencia mendeliana real: cada rasgo tiene un factor de
 *    dominancia por progenitor (no lerp uniforme), seleccion estocastica
 *    de alelo + mutacion gaussiana acotada.
 * 2. Espacio humano parametrico via PCA real sobre un conjunto base de
 *    genomas (poblado por el caller desde los 36 arquetipos reales de
 *    rig_face_archetypes.c -- este modulo NO inventa esos valores, opera
 *    sobre lo que se le cargue, para no arriesgar un solo bit de tus datos
 *    reales).
 * 3. Descomposicion propia (autovalores/autovectores) via Jacobi completo,
 *    sin dependencias externas (no LAPACK/BLAS).
 * 4. Proyeccion, reconstruccion y muestreo aleatorio dentro del espacio de
 *    variacion humana real (no arquetipos discretos interpolados a pares).
 * 5. Serializacion binaria + export/import de texto (formato linea=valor)
 *    para un genoma individual.
 * ==========================================================================*/

#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define RIG_GENOME_TRAIT_COUNT   48
#define RIG_GENOME_MAX_BASIS     64
#define RIG_GENETICS_MAGIC       0x47454e31u /* "GEN1" */
#define RIG_GENETICS_VERSION     1
#define RIG_GENOME_NAME_LEN      32

/* Indices nombrados de rasgos (orden fijo, documentado para que el caller
 * sepa exactamente que campo de sus structs reales mapea a cada indice). */
enum {
    RIG_TRAIT_CRANIUM_WIDTH_RATIO = 0,
    RIG_TRAIT_CRANIUM_HEIGHT_RATIO,
    RIG_TRAIT_FACE_THIRD_UPPER,
    RIG_TRAIT_FACE_THIRD_MID,
    RIG_TRAIT_FACE_THIRD_LOWER,
    RIG_TRAIT_INTERPUPILLARY_DIST,
    RIG_TRAIT_CANTHAL_TILT,
    RIG_TRAIT_EYE_APERTURE,
    RIG_TRAIT_ORBIT_DEPTH,
    RIG_TRAIT_BROW_RIDGE_PROMINENCE,
    RIG_TRAIT_NOSE_BRIDGE_HEIGHT,
    RIG_TRAIT_NOSE_WIDTH,
    RIG_TRAIT_NOSE_TIP_ROTATION,
    RIG_TRAIT_NASOLABIAL_ANGLE,
    RIG_TRAIT_LIP_FULLNESS_UPPER,
    RIG_TRAIT_LIP_FULLNESS_LOWER,
    RIG_TRAIT_PHILTRUM_LENGTH,
    RIG_TRAIT_JAW_GONIAL_ANGLE,
    RIG_TRAIT_JAW_WIDTH,
    RIG_TRAIT_CHIN_PROJECTION,
    RIG_TRAIT_CHEEKBONE_PROMINENCE,
    RIG_TRAIT_CHEEKBONE_WIDTH,
    RIG_TRAIT_EAR_ROTATION,
    RIG_TRAIT_EAR_LENGTH,
    RIG_TRAIT_EAR_HELIX_CURL,
    RIG_TRAIT_MELANIN,
    RIG_TRAIT_HEMOGLOBIN,
    RIG_TRAIT_CAROTENE,
    RIG_TRAIT_SKIN_ROUGHNESS,
    RIG_TRAIT_IRIS_MELANIN,
    RIG_TRAIT_IRIS_CRYPT_DENSITY,
    RIG_TRAIT_HAIR_MELANIN_EUMELANIN,
    RIG_TRAIT_HAIR_MELANIN_PHEOMELANIN,
    RIG_TRAIT_HAIR_CURL_RADIUS,
    RIG_TRAIT_HAIR_DENSITY,
    RIG_TRAIT_BROW_THICKNESS,
    RIG_TRAIT_STATURE_RATIO,
    RIG_TRAIT_SHOULDER_WIDTH_RATIO,
    RIG_TRAIT_LIMB_LENGTH_RATIO,
    RIG_TRAIT_HAND_SIZE_RATIO,
    RIG_TRAIT_NECK_LENGTH_RATIO,
    RIG_TRAIT_BODY_FAT_DISTRIBUTION,
    RIG_TRAIT_MUSCLE_MASS_RATIO,
    RIG_TRAIT_VOICE_FORMANT_SHIFT,
    RIG_TRAIT_FACIAL_ASYMMETRY,
    RIG_TRAIT_DIMPLE_TENDENCY,
    RIG_TRAIT_FRECKLE_DENSITY,
    RIG_TRAIT_RESERVED_1,
    RIG_TRAIT_RESERVED_2
};

typedef struct {
    float v[RIG_GENOME_TRAIT_COUNT];
} RigGenomeVector;

typedef struct {
    char name[RIG_GENOME_NAME_LEN];
    RigGenomeVector genome;
    /* dominancia por rasgo: 0 = totalmente recesivo, 1 = totalmente
     * dominante, 0.5 = codominante (blend). Permite modelar rasgos
     * mendelianos discretos junto a rasgos poligenicos continuos en el
     * mismo vector, cada uno con su propia dinamica de herencia. */
    float dominance[RIG_GENOME_TRAIT_COUNT];
} RigGenomeIndividual;

typedef struct {
    RigGenomeVector items[RIG_GENOME_MAX_BASIS];
    char names[RIG_GENOME_MAX_BASIS][RIG_GENOME_NAME_LEN];
    int count;
} RigGenomeBasisSet;

typedef struct {
    RigGenomeVector mean;
    float eigenvectors[RIG_GENOME_TRAIT_COUNT][RIG_GENOME_TRAIT_COUNT]; /* columnas = autovectores */
    float eigenvalues[RIG_GENOME_TRAIT_COUNT];
    int trait_count;
    int component_count; /* autovectores con autovalor > umbral, ordenados desc */
} RigGenomePCASpace;

/* -------------------------------------------------------------------------
 * PRNG autocontenido (xorshift32) + Box-Muller para mutacion/muestreo,
 * para no depender de rand() global (determinismo reproducible por seed).
 * ---------------------------------------------------------------------- */
static unsigned int rig_genetics_xorshift32__rig_dup_8e640fec(unsigned int *state) {
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static float rig_genetics_rand01__rig_dup_14b342ca(unsigned int *state) {
    return (float)(rig_genetics_xorshift32__rig_dup_8e640fec(state) & 0x00FFFFFFu) / (float)0x01000000u;
}

static float rig_genetics_randn__rig_dup_5a25f176(unsigned int *state) {
    /* Box-Muller */
    float u1 = rig_genetics_rand01__rig_dup_14b342ca(state);
    float u2 = rig_genetics_rand01__rig_dup_14b342ca(state);
    if (u1 < 1e-7f) u1 = 1e-7f;
    return sqrtf(-2.0f * logf(u1)) * cosf(6.283185307f * u2);
}

static float rig_genetics_clampf__rig_dup_ddb2743b(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* -------------------------------------------------------------------------
 * Basis set management
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_genetics_basis_init__rig_dup_333f496c(RigGenomeBasisSet *set) {
    if (!set) return -1;
    memset(set, 0, sizeof(*set));
    return 0;
}

RIGCOM_PUBLIC int rig_genetics_basis_add__rig_dup_1c037ca1(RigGenomeBasisSet *set, const char *name,
                                          const RigGenomeVector *vec) {
    if (!set || !name || !vec) return -1;
    if (set->count >= RIG_GENOME_MAX_BASIS) return -2;
    strncpy(set->names[set->count], name, RIG_GENOME_NAME_LEN - 1);
    set->names[set->count][RIG_GENOME_NAME_LEN - 1] = '\0';
    set->items[set->count] = *vec;
    set->count++;
    return set->count - 1;
}

/* -------------------------------------------------------------------------
 * Herencia mendeliana con dominancia por rasgo + mutacion gaussiana acotada
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_genetics_inherit__rig_dup_576360ed(const RigGenomeIndividual *parent_a,
                                        const RigGenomeIndividual *parent_b,
                                        unsigned int seed,
                                        float mutation_sigma,
                                        RigGenomeIndividual *out_child) {
    if (!parent_a || !parent_b || !out_child) return -1;
    unsigned int rng = seed ? seed : 0x9E3779B9u;

    memset(out_child, 0, sizeof(*out_child));
    snprintf(out_child->name, RIG_GENOME_NAME_LEN, "child_of_%.10s_%.10s",
             parent_a->name, parent_b->name);

    for (int i = 0; i < RIG_GENOME_TRAIT_COUNT; i++) {
        float da = parent_a->dominance[i];
        float db = parent_b->dominance[i];
        float va = parent_a->genome.v[i];
        float vb = parent_b->genome.v[i];

        float total_dom = da + db;
        float p_a; /* probabilidad de que el alelo de A se exprese */
        if (total_dom < 1e-6f) {
            p_a = 0.5f;
        } else {
            p_a = da / total_dom;
        }

        float roll = rig_genetics_rand01__rig_dup_14b342ca(&rng);
        float base_value;
        float codominance = 1.0f - fabsf(da - db); /* alto cuando ambos son similares -> blend */

        if (codominance > 0.6f) {
            /* codominante: mezcla real ponderada por un pequeno sesgo aleatorio,
             * no un promedio ciego -- introduce variabilidad entre hermanos. */
            float w = rig_genetics_clampf__rig_dup_ddb2743b(0.5f + (roll - 0.5f) * 0.4f, 0.0f, 1.0f);
            base_value = va * w + vb * (1.0f - w);
        } else {
            /* seleccion discreta de alelo ponderada por dominancia relativa */
            base_value = (roll < p_a) ? va : vb;
        }

        float mutated = base_value + rig_genetics_randn__rig_dup_5a25f176(&rng) * mutation_sigma;
        out_child->genome.v[i] = mutated;

        /* la dominancia del hijo para ese rasgo hereda tambien, con un sesgo
         * hacia el promedio de los padres mas una pequena deriva */
        float dom_child = 0.5f * (da + db) + rig_genetics_randn__rig_dup_5a25f176(&rng) * 0.03f;
        out_child->dominance[i] = rig_genetics_clampf__rig_dup_ddb2743b(dom_child, 0.0f, 1.0f);
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Covarianza
 * ---------------------------------------------------------------------- */
static void rig_genetics_compute_mean__rig_dup_4c2a6f73(const RigGenomeBasisSet *set, RigGenomeVector *out_mean) {
    memset(out_mean, 0, sizeof(*out_mean));
    if (set->count == 0) return;
    for (int k = 0; k < set->count; k++)
        for (int i = 0; i < RIG_GENOME_TRAIT_COUNT; i++)
            out_mean->v[i] += set->items[k].v[i];
    for (int i = 0; i < RIG_GENOME_TRAIT_COUNT; i++)
        out_mean->v[i] /= (float)set->count;
}

static void rig_genetics_compute_covariance__rig_dup_c1337f4c(const RigGenomeBasisSet *set,
                                             const RigGenomeVector *mean,
                                             float cov[RIG_GENOME_TRAIT_COUNT][RIG_GENOME_TRAIT_COUNT]) {
    int n = RIG_GENOME_TRAIT_COUNT;
    memset(cov, 0, sizeof(float) * n * n);
    if (set->count < 2) return;

    for (int k = 0; k < set->count; k++) {
        float centered[RIG_GENOME_TRAIT_COUNT];
        for (int i = 0; i < n; i++) centered[i] = set->items[k].v[i] - mean->v[i];
        for (int i = 0; i < n; i++)
            for (int j = i; j < n; j++)
                cov[i][j] += centered[i] * centered[j];
    }
    float denom = (float)(set->count - 1);
    for (int i = 0; i < n; i++) {
        for (int j = i; j < n; j++) {
            cov[i][j] /= denom;
            cov[j][i] = cov[i][j];
        }
    }
}

/* -------------------------------------------------------------------------
 * Descomposicion Jacobi (autovalores + autovectores) de matriz simetrica.
 * Metodo ciclico clasico con umbral decreciente. Completo, no aproximado.
 * ---------------------------------------------------------------------- */
static void rig_genetics_jacobi_eigen__rig_dup_a02d0647(float a[RIG_GENOME_TRAIT_COUNT][RIG_GENOME_TRAIT_COUNT],
                                       float eigenvalues[RIG_GENOME_TRAIT_COUNT],
                                       float eigenvectors[RIG_GENOME_TRAIT_COUNT][RIG_GENOME_TRAIT_COUNT],
                                       int n, int max_sweeps) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            eigenvectors[i][j] = (i == j) ? 1.0f : 0.0f;

    for (int sweep = 0; sweep < max_sweeps; sweep++) {
        float off = 0.0f;
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++)
                off += a[i][j] * a[i][j];
        if (off < 1e-12f) break;

        for (int p = 0; p < n; p++) {
            for (int q = p + 1; q < n; q++) {
                if (fabsf(a[p][q]) < 1e-14f) continue;

                float theta = (a[q][q] - a[p][p]) / (2.0f * a[p][q]);
                float sign_t = (theta >= 0.0f) ? 1.0f : -1.0f;
                float t = sign_t / (fabsf(theta) + sqrtf(theta * theta + 1.0f));
                float c = 1.0f / sqrtf(t * t + 1.0f);
                float s = t * c;

                float app = a[p][p] - t * a[p][q];
                float aqq = a[q][q] + t * a[p][q];

                for (int k = 0; k < n; k++) {
                    if (k == p || k == q) continue;
                    float akp = a[k][p];
                    float akq = a[k][q];
                    a[k][p] = a[p][k] = c * akp - s * akq;
                    a[k][q] = a[q][k] = s * akp + c * akq;
                }
                a[p][p] = app;
                a[q][q] = aqq;
                a[p][q] = 0.0f;
                a[q][p] = 0.0f;

                for (int k = 0; k < n; k++) {
                    float vkp = eigenvectors[k][p];
                    float vkq = eigenvectors[k][q];
                    eigenvectors[k][p] = c * vkp - s * vkq;
                    eigenvectors[k][q] = s * vkp + c * vkq;
                }
            }
        }
    }
    for (int i = 0; i < n; i++) eigenvalues[i] = a[i][i];
}

static void rig_genetics_sort_eigen_desc__rig_dup_5dc5c3fd(float eigenvalues[RIG_GENOME_TRAIT_COUNT],
                                          float eigenvectors[RIG_GENOME_TRAIT_COUNT][RIG_GENOME_TRAIT_COUNT],
                                          int n) {
    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++)
            if (eigenvalues[j] > eigenvalues[best]) best = j;
        if (best != i) {
            float tmp = eigenvalues[i]; eigenvalues[i] = eigenvalues[best]; eigenvalues[best] = tmp;
            for (int k = 0; k < n; k++) {
                float tv = eigenvectors[k][i];
                eigenvectors[k][i] = eigenvectors[k][best];
                eigenvectors[k][best] = tv;
            }
        }
    }
}

RIGCOM_PUBLIC int rig_genetics_pca_compute__rig_dup_52334d6b(const RigGenomeBasisSet *set, RigGenomePCASpace *out_space) {
    if (!set || !out_space) return -1;
    if (set->count < 3) return -2; /* covarianza no confiable con menos de 3 muestras */

    memset(out_space, 0, sizeof(*out_space));
    out_space->trait_count = RIG_GENOME_TRAIT_COUNT;

    rig_genetics_compute_mean__rig_dup_4c2a6f73(set, &out_space->mean);

    static float cov[RIG_GENOME_TRAIT_COUNT][RIG_GENOME_TRAIT_COUNT];
    rig_genetics_compute_covariance__rig_dup_c1337f4c(set, &out_space->mean, cov);

    rig_genetics_jacobi_eigen__rig_dup_a02d0647(cov, out_space->eigenvalues, out_space->eigenvectors,
                               RIG_GENOME_TRAIT_COUNT, 100);
    rig_genetics_sort_eigen_desc__rig_dup_5dc5c3fd(out_space->eigenvalues, out_space->eigenvectors, RIG_GENOME_TRAIT_COUNT);

    int comp = 0;
    float total = 0.0f;
    for (int i = 0; i < RIG_GENOME_TRAIT_COUNT; i++)
        if (out_space->eigenvalues[i] > 0.0f) total += out_space->eigenvalues[i];

    float cumulative = 0.0f;
    for (int i = 0; i < RIG_GENOME_TRAIT_COUNT; i++) {
        if (out_space->eigenvalues[i] <= 1e-8f) break;
        cumulative += out_space->eigenvalues[i];
        comp++;
        if (total > 0.0f && cumulative / total > 0.995f) break; /* 99.5% varianza explicada */
    }
    out_space->component_count = comp > 0 ? comp : 1;
    return 0;
}

RIGCOM_PUBLIC int rig_genetics_pca_project__rig_dup_4dfdbb88(const RigGenomePCASpace *space,
                                            const RigGenomeVector *vec,
                                            float *out_coeffs /* [component_count] */) {
    if (!space || !vec || !out_coeffs) return -1;
    float centered[RIG_GENOME_TRAIT_COUNT];
    for (int i = 0; i < space->trait_count; i++)
        centered[i] = vec->v[i] - space->mean.v[i];

    for (int c = 0; c < space->component_count; c++) {
        float dot = 0.0f;
        for (int i = 0; i < space->trait_count; i++)
            dot += centered[i] * space->eigenvectors[i][c];
        out_coeffs[c] = dot;
    }
    return 0;
}

RIGCOM_PUBLIC int rig_genetics_pca_reconstruct__rig_dup_eba90b9f(const RigGenomePCASpace *space,
                                                const float *coeffs,
                                                RigGenomeVector *out_vec) {
    if (!space || !coeffs || !out_vec) return -1;
    for (int i = 0; i < space->trait_count; i++)
        out_vec->v[i] = space->mean.v[i];

    for (int c = 0; c < space->component_count; c++)
        for (int i = 0; i < space->trait_count; i++)
            out_vec->v[i] += coeffs[c] * space->eigenvectors[i][c];
    return 0;
}

/* Muestrea una identidad nueva, valida dentro del espacio de variacion
 * humana real definido por el basis set (no un lerp entre dos arquetipos:
 * es un punto genuinamente nuevo del espacio de covarianza real). */
RIGCOM_PUBLIC int rig_genetics_pca_sample_random__rig_dup_7b4d5dd3(const RigGenomePCASpace *space,
                                                  unsigned int seed,
                                                  float sigma_scale,
                                                  RigGenomeVector *out_vec) {
    if (!space || !out_vec) return -1;
    unsigned int rng = seed ? seed : 0xA5A5A5A5u;
    float coeffs[RIG_GENOME_TRAIT_COUNT];

    for (int c = 0; c < space->component_count; c++) {
        float std_dev = sqrtf(space->eigenvalues[c] > 0.0f ? space->eigenvalues[c] : 0.0f);
        coeffs[c] = rig_genetics_randn__rig_dup_5a25f176(&rng) * std_dev * sigma_scale;
    }
    return rig_genetics_pca_reconstruct__rig_dup_eba90b9f(space, coeffs, out_vec);
}

/* -------------------------------------------------------------------------
 * Serializacion binaria de un genoma + basis set completo
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int payload_size;
} RigGeneticsFileHeader;

RIGCOM_PUBLIC int rig_genetics_basis_save__rig_dup_f2e05186(const RigGenomeBasisSet *set, FILE *fp) {
    if (!set || !fp) return -1;
    RigGeneticsFileHeader hdr = { RIG_GENETICS_MAGIC, RIG_GENETICS_VERSION,
                                  (unsigned int)sizeof(RigGenomeBasisSet) };
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (fwrite(set, sizeof(RigGenomeBasisSet), 1, fp) != 1) return -3;
    return 0;
}

RIGCOM_PUBLIC int rig_genetics_basis_load__rig_dup_4b09c52c(RigGenomeBasisSet *set, FILE *fp) {
    if (!set || !fp) return -1;
    RigGeneticsFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (hdr.magic != RIG_GENETICS_MAGIC) return -3;
    if (hdr.version != RIG_GENETICS_VERSION) return -4;
    if (hdr.payload_size != (unsigned int)sizeof(RigGenomeBasisSet)) return -5;
    if (fread(set, sizeof(RigGenomeBasisSet), 1, fp) != 1) return -6;
    return 0;
}

/* Export/import de texto plano (un valor por linea, orden fijo de indices)
 * para inspeccion humana / diffs en control de versiones. */
RIGCOM_PUBLIC int rig_genetics_vector_to_text__rig_dup_d37f3e4f(const RigGenomeVector *vec, FILE *fp) {
    if (!vec || !fp) return -1;
    fprintf(fp, "# rig_genome v%d (%d traits)\n", RIG_GENETICS_VERSION, RIG_GENOME_TRAIT_COUNT);
    for (int i = 0; i < RIG_GENOME_TRAIT_COUNT; i++)
        fprintf(fp, "%d %.8f\n", i, vec->v[i]);
    return 0;
}

RIGCOM_PUBLIC int rig_genetics_vector_from_text__rig_dup_457d09f2(RigGenomeVector *vec, FILE *fp) {
    if (!vec || !fp) return -1;
    char line[128];
    memset(vec, 0, sizeof(*vec));
    if (!fgets(line, sizeof(line), fp)) return -2; /* header */
    int idx; float val;
    int read_count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%d %f", &idx, &val) == 2 && idx >= 0 && idx < RIG_GENOME_TRAIT_COUNT) {
            vec->v[idx] = val;
            read_count++;
        }
    }
    return read_count;
}
