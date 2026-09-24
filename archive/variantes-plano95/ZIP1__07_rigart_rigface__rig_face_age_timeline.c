/* ============================================================================
 * rig_face_age_timeline.c
 *
 * RigCom :: Continuous Age Timeline Module (aditivo puro)
 *
 * Los arquetipos de estudio de edad en archetypes.c son snapshots
 * DISCRETOS (infante/nino/adolescente/joven/maduro/anciano). Este modulo
 * los convierte en una curva CONTINUA real, permitiendo muestrear
 * cualquier parametro antropometrico a una edad arbitraria (ej. 33.7
 * anios), no solo saltar entre las 6 fotos fijas.
 *
 *   1. Interpolacion cubica monotona (Fritsch-Carlson, el mismo metodo
 *      detras de PCHIP en Matlab/SciPy) EN VEZ de Catmull-Rom clasico.
 *      Es una eleccion deliberada: un spline que puede tener "overshoot"
 *      generaria proporciones anatomicas IMPOSIBLES entre dos edades
 *      reales (ej. una mandibula momentaneamente mas ancha a los 40 que
 *      tanto a los 25 como a los 70). Fritsch-Carlson garantiza que la
 *      curva nunca exceda el rango de sus propios puntos vecinos.
 *   2. Muestreo de valor Y de tasa de cambio (derivada) en una edad dada
 *      -- util para animar la VELOCIDAD actual de un proceso (ej. que
 *      tan rapido esta profundizando una arruga en este momento).
 *   3. Clamping en los bordes (edades fuera del rango definido devuelven
 *      el valor del extremo mas cercano, sin extrapolar sin control).
 *   4. Serializacion de la timeline completa (keyframes crudos; la
 *      curva se recalcula siempre desde ahi, nunca se serializa
 *      "horneada").
 * ==========================================================================*/

#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>

#define RIG_AGE_MAX_KEYFRAMES  16
#define RIG_AGE_MAX_PARAMS     48
#define RIG_AGE_MAGIC          0x41474531u /* "AGE1" */
#define RIG_AGE_VERSION        1

typedef struct {
    float age_years;
    float params[RIG_AGE_MAX_PARAMS];
} RigAgeKeyframe;

typedef struct {
    RigAgeKeyframe keyframes[RIG_AGE_MAX_KEYFRAMES];
    int keyframe_count;
    int param_count;
    /* tangentes de Fritsch-Carlson, una por parametro por keyframe,
     * calculadas por rig_age_timeline_finalize__rig_dup_17ea2fc9() */
    float tangents[RIG_AGE_MAX_KEYFRAMES][RIG_AGE_MAX_PARAMS];
    int finalized;
} RigAgeTimeline;

/* -------------------------------------------------------------------------
 * Construccion
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_age_timeline_init__rig_dup_cdd7784f(RigAgeTimeline *tl, int param_count) {
    if (!tl || param_count <= 0 || param_count > RIG_AGE_MAX_PARAMS) return -1;
    memset(tl, 0, sizeof(*tl));
    tl->param_count = param_count;
    return 0;
}

/* inserta manteniendo orden ascendente de edad (requisito del spline) */
RIGCOM_PUBLIC int rig_age_timeline_add_keyframe__rig_dup_18f64e2a(RigAgeTimeline *tl, float age_years, const float *params) {
    if (!tl || !params) return -1;
    if (tl->keyframe_count >= RIG_AGE_MAX_KEYFRAMES) return -2;

    int insert_at = tl->keyframe_count;
    for (int i = 0; i < tl->keyframe_count; i++) {
        if (age_years < tl->keyframes[i].age_years) { insert_at = i; break; }
        if (fabsf(age_years - tl->keyframes[i].age_years) < 1e-4f) return -3; /* edad duplicada */
    }
    for (int i = tl->keyframe_count; i > insert_at; i--) tl->keyframes[i] = tl->keyframes[i-1];

    tl->keyframes[insert_at].age_years = age_years;
    memcpy(tl->keyframes[insert_at].params, params, sizeof(float) * (size_t)tl->param_count);
    tl->keyframe_count++;
    tl->finalized = 0; /* hay que recalcular tangentes */
    return insert_at;
}

/* -------------------------------------------------------------------------
 * Fritsch-Carlson: calcula tangentes que garantizan monotonicidad local
 * (nunca overshoot mas alla de los valores vecinos), por parametro.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_age_timeline_finalize__rig_dup_17ea2fc9(RigAgeTimeline *tl) {
    if (!tl) return -1;
    int n = tl->keyframe_count;
    if (n < 2) { tl->finalized = 1; return 0; } /* con 0-1 puntos no hay curva que ajustar */

    for (int p = 0; p < tl->param_count; p++) {
        float delta[RIG_AGE_MAX_KEYFRAMES];
        for (int i = 0; i < n - 1; i++) {
            float dx = tl->keyframes[i+1].age_years - tl->keyframes[i].age_years;
            float dy = tl->keyframes[i+1].params[p] - tl->keyframes[i].params[p];
            delta[i] = (dx > 1e-6f) ? (dy / dx) : 0.0f;
        }

        float m[RIG_AGE_MAX_KEYFRAMES];
        m[0] = delta[0];
        m[n-1] = delta[n-2];
        for (int i = 1; i < n - 1; i++) {
            if (delta[i-1] * delta[i] <= 0.0f) {
                m[i] = 0.0f; /* cambio de signo local: tangente plana, evita oscilar */
            } else {
                m[i] = 0.5f * (delta[i-1] + delta[i]);
            }
        }

        /* correccion Fritsch-Carlson: restringe alpha^2+beta^2 <= 9 en cada
         * intervalo para garantizar monotonicidad estricta */
        for (int i = 0; i < n - 1; i++) {
            if (fabsf(delta[i]) < 1e-8f) { m[i] = 0.0f; m[i+1] = 0.0f; continue; }
            float alpha = m[i] / delta[i];
            float beta = m[i+1] / delta[i];
            float s2 = alpha*alpha + beta*beta;
            if (s2 > 9.0f) {
                float tau = 3.0f / sqrtf(s2);
                m[i] = tau * alpha * delta[i];
                m[i+1] = tau * beta * delta[i];
            }
        }

        for (int i = 0; i < n; i++) tl->tangents[i][p] = m[i];
    }
    tl->finalized = 1;
    return 0;
}

/* -------------------------------------------------------------------------
 * Evaluacion de Hermite cubico monotono en una edad arbitraria
 * ---------------------------------------------------------------------- */
static void rig_age_hermite_eval__rig_dup_0dd19fc0(float y0, float y1, float m0, float m1, float t,
                                  float *out_val, float *out_deriv, float dx) {
    float t2 = t*t, t3 = t2*t;
    float h00 = 2*t3 - 3*t2 + 1;
    float h10 = t3 - 2*t2 + t;
    float h01 = -2*t3 + 3*t2;
    float h11 = t3 - t2;
    *out_val = h00*y0 + h10*dx*m0 + h01*y1 + h11*dx*m1;

    if (out_deriv) {
        float dh00 = 6*t2 - 6*t;
        float dh10 = 3*t2 - 4*t + 1;
        float dh01 = -6*t2 + 6*t;
        float dh11 = 3*t2 - 2*t;
        float d_dt = dh00*y0 + dh10*dx*m0 + dh01*y1 + dh11*dx*m1;
        *out_deriv = (dx > 1e-6f) ? (d_dt / dx) : 0.0f; /* regla de la cadena: d/d(edad) = d/dt * dt/d(edad) = d/dt / dx */
    }
}

RIGCOM_PUBLIC int rig_age_timeline_sample__rig_dup_04555b23(RigAgeTimeline *tl, float age_years,
                                           float *out_params, float *out_rates_or_null) {
    if (!tl || !out_params) return -1;
    if (!tl->finalized) rig_age_timeline_finalize__rig_dup_17ea2fc9(tl);
    int n = tl->keyframe_count;
    if (n == 0) return -2;

    if (n == 1 || age_years <= tl->keyframes[0].age_years) {
        memcpy(out_params, tl->keyframes[0].params, sizeof(float) * (size_t)tl->param_count);
        if (out_rates_or_null) memset(out_rates_or_null, 0, sizeof(float) * (size_t)tl->param_count);
        return 0;
    }
    if (age_years >= tl->keyframes[n-1].age_years) {
        memcpy(out_params, tl->keyframes[n-1].params, sizeof(float) * (size_t)tl->param_count);
        if (out_rates_or_null) memset(out_rates_or_null, 0, sizeof(float) * (size_t)tl->param_count);
        return 0;
    }

    int seg = 0;
    for (int i = 0; i < n - 1; i++) {
        if (age_years >= tl->keyframes[i].age_years && age_years <= tl->keyframes[i+1].age_years) { seg = i; break; }
    }

    float x0 = tl->keyframes[seg].age_years, x1 = tl->keyframes[seg+1].age_years;
    float dx = x1 - x0;
    float t = (dx > 1e-6f) ? (age_years - x0) / dx : 0.0f;

    for (int p = 0; p < tl->param_count; p++) {
        float y0 = tl->keyframes[seg].params[p], y1 = tl->keyframes[seg+1].params[p];
        float m0 = tl->tangents[seg][p], m1 = tl->tangents[seg+1][p];
        float val, deriv;
        rig_age_hermite_eval__rig_dup_0dd19fc0(y0, y1, m0, m1, t, &val, &deriv, dx);
        out_params[p] = val;
        if (out_rates_or_null) out_rates_or_null[p] = deriv;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Serializacion (keyframes crudos; la curva se recalcula siempre al cargar)
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int payload_size;
} RigAgeFileHeader;

RIGCOM_PUBLIC int rig_age_timeline_save__rig_dup_5d29028c(const RigAgeTimeline *tl, FILE *fp) {
    if (!tl || !fp) return -1;
    RigAgeFileHeader hdr = { RIG_AGE_MAGIC, RIG_AGE_VERSION, (unsigned int)sizeof(RigAgeTimeline) };
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (fwrite(tl, sizeof(RigAgeTimeline), 1, fp) != 1) return -3;
    return 0;
}

RIGCOM_PUBLIC int rig_age_timeline_load__rig_dup_94893bdb(RigAgeTimeline *tl, FILE *fp) {
    if (!tl || !fp) return -1;
    RigAgeFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (hdr.magic != RIG_AGE_MAGIC) return -3;
    if (hdr.version != RIG_AGE_VERSION) return -4;
    if (hdr.payload_size != (unsigned int)sizeof(RigAgeTimeline)) return -5;
    if (fread(tl, sizeof(RigAgeTimeline), 1, fp) != 1) return -6;
    tl->finalized = 0; /* se recalculan tangentes tras cargar, nunca se confia en datos horneados */
    return 0;
}
