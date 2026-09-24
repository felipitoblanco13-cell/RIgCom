/* ============================================================================
 * rig_face_vision_retarget.c
 *
 * RigCom :: Live Vision -> FACS Retargeting Bridge (aditivo puro)
 *
 * Cierra el circulo entre las dos lineas mas potentes del motor: alimenta
 * los 68 landmarks detectados/trackeados por sovereign_vision.c directo a
 * los pesos de Action Units que consume ng_anim.c, en tiempo real, sin
 * modificar ninguno de los dos archivos.
 *
 * Layout de landmarks: esquema estandar iBUG-68 (36-41 ojo derecho,
 * 42-47 ojo izquierdo, 17-21/22-26 cejas, 48-67 boca, 0-16 mandibula,
 * 27-35 nariz) -- el mismo esquema publico que ya produce vision.c.
 *
 *   1. Calibracion neutral (captura de referencia en reposo) para que las
 *      metricas geometricas sean invariantes a la anatomia individual.
 *   2. Extraccion geometrica de AUs (EAR para parpadeo, MAR para
 *      apertura de boca, desplazamiento de cejas/comisuras normalizado
 *      por distancia interocular) -- tecnica estandar de retargeting
 *      geometrico, no una red neuronal (complementa, no reemplaza, al
 *      MLP de deteccion de piel que ya existe en vision.c).
 *   3. Suavizado temporal (EMA) + rechazo de outliers por velocidad
 *      maxima, imprescindible para tracking de camara real (ruidoso
 *      frame a frame).
 *   4. Salida dispersa {numero_AU, intensidad} para que el caller (con
 *      su propio header real de ng_anim.h) mapee a su arreglo interno
 *      sin que este modulo necesite conocer su layout exacto.
 *   5. Serializacion de la calibracion neutral (evita recalibrar cada
 *      sesion).
 * ==========================================================================*/

#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>

#define RIG_RETARGET_LANDMARK_COUNT  68
#define RIG_RETARGET_MAX_AU_OUT      32
#define RIG_RETARGET_MAGIC           0x52455431u /* "RET1" */
#define RIG_RETARGET_VERSION         1

typedef struct { float x, y; } RigLandmark2D;

typedef struct {
    RigLandmark2D pts[RIG_RETARGET_LANDMARK_COUNT];
} RigLandmarkSet68;

typedef struct {
    int au_number;      /* numero FACS estandar, ej. 1,2,4,6,9,12,15,25,26,45 */
    float intensity;    /* 0..1 */
} RigAUSample;

typedef struct {
    RigLandmarkSet68 neutral;
    float interocular_dist;      /* distancia 36-45 en calibracion neutral, referencia de escala */
    float neutral_ear_right, neutral_ear_left;
    float neutral_mar;
    float neutral_face_height;   /* 27 (puente nasal) a 8 (menton), referencia vertical */
    int   calibrated;
} RigRetargetCalibration;

typedef struct {
    RigAUSample smoothed[RIG_RETARGET_MAX_AU_OUT];
    int smoothed_count;
    RigLandmarkSet68 prev_raw;         /* para rechazo de outliers por velocidad */
    int has_prev;
    float smoothing_tau_s;
    float max_landmark_velocity_per_s; /* en unidades normalizadas por interocular_dist */
} RigRetargetRuntime;

/* -------------------------------------------------------------------------
 * Utilidades geometricas
 * ---------------------------------------------------------------------- */
static float rig_retarget_dist__rig_dup_928bc8a7(RigLandmark2D a, RigLandmark2D b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}
static float rig_retarget_clamp01__rig_dup_f32c8dec(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static RigLandmark2D rig_retarget_mid__rig_dup_a8f5e249(RigLandmark2D a, RigLandmark2D b) {
    RigLandmark2D r = { (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f }; return r;
}

/* -------------------------------------------------------------------------
 * Calibracion neutral
 * ---------------------------------------------------------------------- */
static float rig_retarget_ear__rig_dup_07d732af(const RigLandmarkSet68 *lm, int p1, int p2, int p3, int p4, int p5, int p6) {
    float vert1 = rig_retarget_dist__rig_dup_928bc8a7(lm->pts[p2], lm->pts[p6]);
    float vert2 = rig_retarget_dist__rig_dup_928bc8a7(lm->pts[p3], lm->pts[p5]);
    float horiz = rig_retarget_dist__rig_dup_928bc8a7(lm->pts[p1], lm->pts[p4]);
    if (horiz < 1e-6f) return 0.3f;
    return (vert1 + vert2) / (2.0f * horiz);
}

static float rig_retarget_mar__rig_dup_6c1a8553(const RigLandmarkSet68 *lm) {
    /* boca interior: 60 esq. izq, 61,62,63 sup, 64 esq. der, 65,66,67 inf
     * (esquema iBUG-68 estandar). Usamos 62 (centro sup interior) y 66
     * (centro inf interior) contra la apertura de comisuras 60-64. */
    float vert = rig_retarget_dist__rig_dup_928bc8a7(lm->pts[62], lm->pts[66]);
    float horiz = rig_retarget_dist__rig_dup_928bc8a7(lm->pts[60], lm->pts[64]);
    if (horiz < 1e-6f) return 0.0f;
    return vert / horiz;
}

RIGCOM_PUBLIC int rig_retarget_calibrate_neutral__rig_dup_be4a5eb3(RigRetargetCalibration *cal, const RigLandmarkSet68 *neutral_pose) {
    if (!cal || !neutral_pose) return -1;
    memset(cal, 0, sizeof(*cal));
    cal->neutral = *neutral_pose;
    cal->interocular_dist = rig_retarget_dist__rig_dup_928bc8a7(neutral_pose->pts[36], neutral_pose->pts[45]);
    if (cal->interocular_dist < 1e-4f) return -2; /* landmarks degenerados */

    cal->neutral_ear_right = rig_retarget_ear__rig_dup_07d732af(neutral_pose, 36,37,38,39,40,41);
    cal->neutral_ear_left  = rig_retarget_ear__rig_dup_07d732af(neutral_pose, 42,43,44,45,46,47);
    cal->neutral_mar = rig_retarget_mar__rig_dup_6c1a8553(neutral_pose);
    cal->neutral_face_height = rig_retarget_dist__rig_dup_928bc8a7(neutral_pose->pts[27], neutral_pose->pts[8]);
    cal->calibrated = 1;
    return 0;
}

RIGCOM_PUBLIC int rig_retarget_runtime_init__rig_dup_ec751c5b(RigRetargetRuntime *rt, float smoothing_tau_s) {
    if (!rt) return -1;
    memset(rt, 0, sizeof(*rt));
    rt->smoothing_tau_s = smoothing_tau_s > 0.0f ? smoothing_tau_s : 0.08f;
    rt->max_landmark_velocity_per_s = 8.0f; /* en unidades de interocular_dist/s, generoso pero acota glitches */
    return 0;
}

/* -------------------------------------------------------------------------
 * Filtro de outliers: si un punto salta mas de lo fisicamente plausible en
 * un frame, se limita su desplazamiento (no se descarta el frame entero,
 * para no perder informacion de los demas puntos que si son validos).
 * ---------------------------------------------------------------------- */
static void rig_retarget_filter_outliers__rig_dup_c427d7e0(RigRetargetRuntime *rt, const RigLandmarkSet68 *raw,
                                          float interocular_dist, float dt,
                                          RigLandmarkSet68 *out_filtered) {
    if (!rt->has_prev) {
        *out_filtered = *raw;
        rt->prev_raw = *raw;
        rt->has_prev = 1;
        return;
    }
    float max_step = rt->max_landmark_velocity_per_s * interocular_dist * dt;
    for (int i = 0; i < RIG_RETARGET_LANDMARK_COUNT; i++) {
        float dx = raw->pts[i].x - rt->prev_raw.pts[i].x;
        float dy = raw->pts[i].y - rt->prev_raw.pts[i].y;
        float step = sqrtf(dx*dx + dy*dy);
        if (step > max_step && step > 1e-6f) {
            float scale = max_step / step;
            out_filtered->pts[i].x = rt->prev_raw.pts[i].x + dx * scale;
            out_filtered->pts[i].y = rt->prev_raw.pts[i].y + dy * scale;
        } else {
            out_filtered->pts[i] = raw->pts[i];
        }
    }
    rt->prev_raw = *out_filtered;
}

static float rig_retarget_smooth_towards__rig_dup_7d1d6729(RigRetargetRuntime *rt, int au_number, float target, float dt) {
    for (int i = 0; i < rt->smoothed_count; i++) {
        if (rt->smoothed[i].au_number == au_number) {
            float alpha = 1.0f - expf(-dt / rt->smoothing_tau_s);
            rt->smoothed[i].intensity += (target - rt->smoothed[i].intensity) * alpha;
            return rt->smoothed[i].intensity;
        }
    }
    if (rt->smoothed_count < RIG_RETARGET_MAX_AU_OUT) {
        rt->smoothed[rt->smoothed_count].au_number = au_number;
        rt->smoothed[rt->smoothed_count].intensity = target;
        rt->smoothed_count++;
        return target;
    }
    return target; /* tabla llena: devuelve sin suavizar en vez de perder el AU */
}

/* -------------------------------------------------------------------------
 * Extraccion geometrica principal: landmarks crudos -> AUs suavizados
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_retarget_compute_au_weights__rig_dup_68a9040f(const RigRetargetCalibration *cal,
                                                   RigRetargetRuntime *rt,
                                                   const RigLandmarkSet68 *raw_landmarks,
                                                   float dt,
                                                   RigAUSample *out_samples,
                                                   int max_out) {
    if (!cal || !rt || !raw_landmarks || !out_samples || max_out <= 0) return -1;
    if (!cal->calibrated) return -2;

    float cur_interocular = rig_retarget_dist__rig_dup_928bc8a7(raw_landmarks->pts[36], raw_landmarks->pts[45]);
    if (cur_interocular < 1e-4f) return -3;

    RigLandmarkSet68 lm;
    rig_retarget_filter_outliers__rig_dup_c427d7e0(rt, raw_landmarks, cur_interocular, dt > 0.0f ? dt : (1.0f/30.0f), &lm);

    float scale_ratio = cal->interocular_dist / cur_interocular; /* normaliza distancia camara/zoom */

    /* --- AU45: parpadeo (bilateral, se reporta como un solo AU con el
     * promedio de ambos ojos, estandar en anotacion FACS manual) --- */
    float ear_r = rig_retarget_ear__rig_dup_07d732af(&lm, 36,37,38,39,40,41);
    float ear_l = rig_retarget_ear__rig_dup_07d732af(&lm, 42,43,44,45,46,47);
    float ear_avg = 0.5f * (ear_r + ear_l);
    float neutral_ear_avg = 0.5f * (cal->neutral_ear_right + cal->neutral_ear_left);
    float ear_closed_floor = 0.06f;
    float blink_intensity = rig_retarget_clamp01__rig_dup_f32c8dec((neutral_ear_avg - ear_avg) / (neutral_ear_avg - ear_closed_floor + 1e-6f));

    /* --- AU1/AU2: elevacion de ceja interna/externa ---
     * IMPORTANTE: la referencia del ojo debe ser un punto que NO se mueva
     * al parpadear, o el cierre del ojo se leeria como elevacion de ceja.
     * Los parpados (37,38,40,41) se mueven al parpadear; las COMISURAS
     * del ojo (36,39 y 42,45) son estructuralmente estables durante el
     * parpadeo (solo cambian si el ojo entero se desplaza), por eso se
     * usan aqui en vez de los puntos de parpado. */
    RigLandmark2D brow_inner_r = lm.pts[21], brow_inner_l = lm.pts[22];
    RigLandmark2D brow_outer_r = lm.pts[17], brow_outer_l = lm.pts[26];
    RigLandmark2D eye_ref_r = rig_retarget_mid__rig_dup_a8f5e249(lm.pts[36], lm.pts[39]);
    RigLandmark2D eye_ref_l = rig_retarget_mid__rig_dup_a8f5e249(lm.pts[42], lm.pts[45]);

    float inner_gap_cur = 0.5f * ((eye_ref_r.y - brow_inner_r.y) + (eye_ref_l.y - brow_inner_l.y));
    RigLandmark2D neutral_eye_ref_r = rig_retarget_mid__rig_dup_a8f5e249(cal->neutral.pts[36], cal->neutral.pts[39]);
    RigLandmark2D neutral_eye_ref_l = rig_retarget_mid__rig_dup_a8f5e249(cal->neutral.pts[42], cal->neutral.pts[45]);
    float inner_gap_neutral = 0.5f * ((neutral_eye_ref_r.y - cal->neutral.pts[21].y) + (neutral_eye_ref_l.y - cal->neutral.pts[22].y));

    float outer_gap_cur = 0.5f * ((eye_ref_r.y - brow_outer_r.y) + (eye_ref_l.y - brow_outer_l.y));
    float outer_gap_neutral = 0.5f * ((neutral_eye_ref_r.y - cal->neutral.pts[17].y) + (neutral_eye_ref_l.y - cal->neutral.pts[26].y));

    float au1_intensity = rig_retarget_clamp01__rig_dup_f32c8dec(((inner_gap_cur * scale_ratio) - inner_gap_neutral) / (cal->interocular_dist * 0.15f));
    float au2_intensity = rig_retarget_clamp01__rig_dup_f32c8dec(((outer_gap_cur * scale_ratio) - outer_gap_neutral) / (cal->interocular_dist * 0.15f));

    /* --- AU12/AU15: comisuras (sonrisa / deprimidas)
     * IMPORTANTE: la referencia debe ser un punto INDEPENDIENTE de las
     * comisuras que se estan midiendo. Usar el punto medio de las propias
     * comisuras (48,54) como referencia se autocancela: si ambas suben lo
     * mismo (sonrisa simetrica), la diferencia contra su propio promedio
     * da cero. Se usa la base nasal (landmark 33), estable e independiente
     * de la boca, como ancla vertical. */
    float corner_lift_r = (lm.pts[33].y - lm.pts[48].y) * scale_ratio;
    float corner_lift_l = (lm.pts[33].y - lm.pts[54].y) * scale_ratio;
    float neutral_lift_r = cal->neutral.pts[33].y - cal->neutral.pts[48].y;
    float neutral_lift_l = cal->neutral.pts[33].y - cal->neutral.pts[54].y;
    float lift_delta = 0.5f * ((corner_lift_r - neutral_lift_r) + (corner_lift_l - neutral_lift_l));

    float au12_intensity = rig_retarget_clamp01__rig_dup_f32c8dec(lift_delta / (cal->interocular_dist * 0.12f));
    float au15_intensity = rig_retarget_clamp01__rig_dup_f32c8dec(-lift_delta / (cal->interocular_dist * 0.12f));

    /* --- AU25/AU26: labios separados / mandibula caida --- */
    float mar_cur = rig_retarget_mar__rig_dup_6c1a8553(&lm);
    float au25_intensity = rig_retarget_clamp01__rig_dup_f32c8dec((mar_cur - cal->neutral_mar) / 0.35f);

    float face_height_cur = rig_retarget_dist__rig_dup_928bc8a7(lm.pts[27], lm.pts[8]) * scale_ratio;
    float jaw_delta = face_height_cur - cal->neutral_face_height;
    float au26_intensity = rig_retarget_clamp01__rig_dup_f32c8dec(jaw_delta / (cal->neutral_face_height * 0.18f + 1e-6f));

    int n = 0;
    #define RIG_RETARGET_EMIT(AU, VAL) do { \
        if (n < max_out) { \
            out_samples[n].au_number = (AU); \
            out_samples[n].intensity = rig_retarget_smooth_towards__rig_dup_7d1d6729(rt, (AU), (VAL), dt > 0.0f ? dt : (1.0f/30.0f)); \
            n++; \
        } \
    } while (0)

    RIG_RETARGET_EMIT(45, blink_intensity);
    RIG_RETARGET_EMIT(1, au1_intensity);
    RIG_RETARGET_EMIT(2, au2_intensity);
    RIG_RETARGET_EMIT(12, au12_intensity);
    RIG_RETARGET_EMIT(15, au15_intensity);
    RIG_RETARGET_EMIT(25, au25_intensity);
    RIG_RETARGET_EMIT(26, au26_intensity);
    #undef RIG_RETARGET_EMIT

    return n;
}

/* -------------------------------------------------------------------------
 * Serializacion de la calibracion neutral
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int payload_size;
} RigRetargetFileHeader;

RIGCOM_PUBLIC int rig_retarget_calibration_save__rig_dup_dfbc8571(const RigRetargetCalibration *cal, FILE *fp) {
    if (!cal || !fp) return -1;
    RigRetargetFileHeader hdr = { RIG_RETARGET_MAGIC, RIG_RETARGET_VERSION,
                                   (unsigned int)sizeof(RigRetargetCalibration) };
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (fwrite(cal, sizeof(RigRetargetCalibration), 1, fp) != 1) return -3;
    return 0;
}

RIGCOM_PUBLIC int rig_retarget_calibration_load__rig_dup_209c9320(RigRetargetCalibration *cal, FILE *fp) {
    if (!cal || !fp) return -1;
    RigRetargetFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (hdr.magic != RIG_RETARGET_MAGIC) return -3;
    if (hdr.version != RIG_RETARGET_VERSION) return -4;
    if (hdr.payload_size != (unsigned int)sizeof(RigRetargetCalibration)) return -5;
    if (fread(cal, sizeof(RigRetargetCalibration), 1, fp) != 1) return -6;
    return 0;
}
