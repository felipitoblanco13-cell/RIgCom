/* ==========================================================================
 * 05_rig_face_export_arkit.c   —   RIGFACE :: C UNIFICADO
 *
 * Copia canonica : src_raw/rig_face_export_arkit-1.c
 * Copias fundidas: 1
 * Funciones      : 5      Unidades injertadas: 0      Variantes: 0
 *
 * Copia canonica preservada verbatim. Ninguna funcion eliminada,
 * reducida ni omitida. Las divergencias de cuerpo bajo un mismo
 * nombre se conservan como <nombre>__vN con su procedencia.
 * ========================================================================== */
/* ============================================================================
 * rig_face_export_arkit.c
 *
 * RigCom :: ARKit 52-Blendshape Export Module (aditivo puro)
 *
 * Traduce el estado de animacion interno (128 AUs de ng_anim.c + gaze de
 * eye_vergence.c + pose de mandibula) al set ESTANDAR de 52 blendshapes
 * de Apple ARKit (ARFaceAnchor.blendShapes), para interoperar con
 * pipelines de captura facial / motores externos (Unreal Live Link Face,
 * Unity ARKit Face Tracking, etc.) sin tocar ng_anim.c.
 *
 * Diseno desacoplado: NO asume el layout interno exacto del arreglo de
 * 128 AUs de ng_anim.c (no tengo ese header). En su lugar recibe un
 * callback au_lookup(userdata, au_number) que el caller implementa sobre
 * su propio arreglo real -- cero riesgo de desalineacion de memoria.
 *
 *   1. Tabla de correspondencia FACS AU -> blendshape ARKit (28 mapeos
 *      establecidos en la industria de animacion facial).
 *   2. Gaze continuo (no es un AU FACS discreto) desde eye_vergence.c
 *      -> eyeLookUp/Down/In/Out por ojo, con conversion angulo->peso.
 *   3. Pose de mandibula continua -> jawOpen/Left/Right/Forward.
 *   4. Export a JSON plano {nombre: peso}, formato compatible con
 *      ARFaceAnchor.blendShapes / Live Link Face.
 *   5. Serializacion binaria de una muestra (para grabar secuencias).
 * ==========================================================================*/

#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>

#define RIG_ARKIT_SHAPE_COUNT 52
#define RIG_ARKIT_MAGIC       0x414b4931u /* "AKI1" */
#define RIG_ARKIT_VERSION     1

typedef enum {
    RIG_ARKIT_browDownLeft = 0, RIG_ARKIT_browDownRight, RIG_ARKIT_browInnerUp,
    RIG_ARKIT_browOuterUpLeft, RIG_ARKIT_browOuterUpRight,
    RIG_ARKIT_cheekPuff, RIG_ARKIT_cheekSquintLeft, RIG_ARKIT_cheekSquintRight,
    RIG_ARKIT_eyeBlinkLeft, RIG_ARKIT_eyeBlinkRight,
    RIG_ARKIT_eyeLookDownLeft, RIG_ARKIT_eyeLookDownRight,
    RIG_ARKIT_eyeLookInLeft, RIG_ARKIT_eyeLookInRight,
    RIG_ARKIT_eyeLookOutLeft, RIG_ARKIT_eyeLookOutRight,
    RIG_ARKIT_eyeLookUpLeft, RIG_ARKIT_eyeLookUpRight,
    RIG_ARKIT_eyeSquintLeft, RIG_ARKIT_eyeSquintRight,
    RIG_ARKIT_eyeWideLeft, RIG_ARKIT_eyeWideRight,
    RIG_ARKIT_jawForward, RIG_ARKIT_jawLeft, RIG_ARKIT_jawOpen, RIG_ARKIT_jawRight,
    RIG_ARKIT_mouthClose, RIG_ARKIT_mouthDimpleLeft, RIG_ARKIT_mouthDimpleRight,
    RIG_ARKIT_mouthFrownLeft, RIG_ARKIT_mouthFrownRight, RIG_ARKIT_mouthFunnel,
    RIG_ARKIT_mouthLeft, RIG_ARKIT_mouthLowerDownLeft, RIG_ARKIT_mouthLowerDownRight,
    RIG_ARKIT_mouthPressLeft, RIG_ARKIT_mouthPressRight, RIG_ARKIT_mouthPucker,
    RIG_ARKIT_mouthRight, RIG_ARKIT_mouthRollLower, RIG_ARKIT_mouthRollUpper,
    RIG_ARKIT_mouthShrugLower, RIG_ARKIT_mouthShrugUpper,
    RIG_ARKIT_mouthSmileLeft, RIG_ARKIT_mouthSmileRight,
    RIG_ARKIT_mouthStretchLeft, RIG_ARKIT_mouthStretchRight,
    RIG_ARKIT_mouthUpperUpLeft, RIG_ARKIT_mouthUpperUpRight,
    RIG_ARKIT_noseSneerLeft, RIG_ARKIT_noseSneerRight,
    RIG_ARKIT_tongueOut
} RigARKitShapeIndex;

static const char *RIG_ARKIT_SHAPE_NAMES[RIG_ARKIT_SHAPE_COUNT] = {
    "browDownLeft","browDownRight","browInnerUp","browOuterUpLeft","browOuterUpRight",
    "cheekPuff","cheekSquintLeft","cheekSquintRight",
    "eyeBlinkLeft","eyeBlinkRight",
    "eyeLookDownLeft","eyeLookDownRight","eyeLookInLeft","eyeLookInRight",
    "eyeLookOutLeft","eyeLookOutRight","eyeLookUpLeft","eyeLookUpRight",
    "eyeSquintLeft","eyeSquintRight","eyeWideLeft","eyeWideRight",
    "jawForward","jawLeft","jawOpen","jawRight",
    "mouthClose","mouthDimpleLeft","mouthDimpleRight","mouthFrownLeft","mouthFrownRight",
    "mouthFunnel","mouthLeft","mouthLowerDownLeft","mouthLowerDownRight",
    "mouthPressLeft","mouthPressRight","mouthPucker","mouthRight",
    "mouthRollLower","mouthRollUpper","mouthShrugLower","mouthShrugUpper",
    "mouthSmileLeft","mouthSmileRight","mouthStretchLeft","mouthStretchRight",
    "mouthUpperUpLeft","mouthUpperUpRight","noseSneerLeft","noseSneerRight",
    "tongueOut"
};

typedef struct {
    int au_number;
    int shape_index;   /* RigARKitShapeIndex */
    float weight;       /* factor de escala, tipicamente 1.0; <1 si un AU bilateral se reparte */
} RigARKitAUMap;

/* Correspondencia FACS -> ARKit establecida en la industria de animacion
 * facial / retargeting (mismo tipo de tabla que usan MetaHuman/Live Link). */
static const RigARKitAUMap RIG_ARKIT_AU_TABLE[] = {
    { 1,  RIG_ARKIT_browInnerUp,        1.0f },
    { 2,  RIG_ARKIT_browOuterUpLeft,    1.0f },
    { 2,  RIG_ARKIT_browOuterUpRight,   1.0f },
    { 4,  RIG_ARKIT_browDownLeft,       1.0f },
    { 4,  RIG_ARKIT_browDownRight,      1.0f },
    { 5,  RIG_ARKIT_eyeWideLeft,        1.0f },
    { 5,  RIG_ARKIT_eyeWideRight,       1.0f },
    { 6,  RIG_ARKIT_cheekSquintLeft,    1.0f },
    { 6,  RIG_ARKIT_cheekSquintRight,   1.0f },
    { 7,  RIG_ARKIT_eyeSquintLeft,      1.0f },
    { 7,  RIG_ARKIT_eyeSquintRight,     1.0f },
    { 9,  RIG_ARKIT_noseSneerLeft,      1.0f },
    { 9,  RIG_ARKIT_noseSneerRight,     1.0f },
    { 10, RIG_ARKIT_mouthUpperUpLeft,   1.0f },
    { 10, RIG_ARKIT_mouthUpperUpRight,  1.0f },
    { 12, RIG_ARKIT_mouthSmileLeft,     1.0f },
    { 12, RIG_ARKIT_mouthSmileRight,    1.0f },
    { 14, RIG_ARKIT_mouthDimpleLeft,    1.0f },
    { 14, RIG_ARKIT_mouthDimpleRight,   1.0f },
    { 15, RIG_ARKIT_mouthFrownLeft,     1.0f },
    { 15, RIG_ARKIT_mouthFrownRight,    1.0f },
    { 16, RIG_ARKIT_mouthLowerDownLeft, 1.0f },
    { 16, RIG_ARKIT_mouthLowerDownRight,1.0f },
    { 17, RIG_ARKIT_mouthShrugLower,    1.0f },
    { 18, RIG_ARKIT_mouthPucker,        1.0f },
    { 20, RIG_ARKIT_mouthStretchLeft,   1.0f },
    { 20, RIG_ARKIT_mouthStretchRight,  1.0f },
    { 22, RIG_ARKIT_mouthFunnel,        1.0f },
    { 23, RIG_ARKIT_mouthPressLeft,     1.0f },
    { 23, RIG_ARKIT_mouthPressRight,    1.0f },
    { 24, RIG_ARKIT_mouthRollUpper,     1.0f },
    { 24, RIG_ARKIT_mouthRollLower,     1.0f },
    { 28, RIG_ARKIT_mouthRollLower,     0.6f },
    { 33, RIG_ARKIT_cheekPuff,          1.0f },
    { 36, RIG_ARKIT_tongueOut,          1.0f },
    { 43, RIG_ARKIT_eyeBlinkLeft,       1.0f },
    { 43, RIG_ARKIT_eyeBlinkRight,      1.0f },
    { 45, RIG_ARKIT_eyeBlinkLeft,       1.0f },
    { 45, RIG_ARKIT_eyeBlinkRight,      1.0f },
};
#define RIG_ARKIT_AU_TABLE_COUNT (int)(sizeof(RIG_ARKIT_AU_TABLE) / sizeof(RIG_ARKIT_AU_TABLE[0]))

typedef struct {
    float weights[RIG_ARKIT_SHAPE_COUNT];
} RigARKitFrame;

static float rig_arkit_clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

/* -------------------------------------------------------------------------
 * Computo principal: recorre la tabla AU->shape consultando el callback
 * del caller (desacoplado del layout real de 128 AUs), luego superpone
 * gaze continuo y pose de mandibula (que no son AUs FACS discretos).
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_arkit_compute(float (*au_lookup)(void *userdata, int au_number), void *userdata,
                                     float gaze_left_yaw_deg, float gaze_left_pitch_deg,
                                     float gaze_right_yaw_deg, float gaze_right_pitch_deg,
                                     float jaw_open01, float jaw_lateral_norm, float jaw_forward01,
                                     RigARKitFrame *out) {
    if (!au_lookup || !out) return -1;
    memset(out, 0, sizeof(*out));

    for (int i = 0; i < RIG_ARKIT_AU_TABLE_COUNT; i++) {
        const RigARKitAUMap *m = &RIG_ARKIT_AU_TABLE[i];
        float au_val = au_lookup(userdata, m->au_number);
        float contribution = rig_arkit_clamp01(au_val) * m->weight;
        if (contribution > out->weights[m->shape_index]) {
            out->weights[m->shape_index] = contribution; /* max, no suma: varios AUs pueden alimentar el mismo shape */
        }
    }

    /* gaze: +yaw = hacia afuera del centro segun convencion de eye_vergence.c
     * (positivo = hacia el lado derecho de la cabeza). Para el ojo izquierdo,
     * yaw positivo es "in" (hacia el centro); para el derecho, yaw positivo
     * es "out" (hacia afuera). Se normaliza sobre un rango de +-35 grados,
     * fisiologicamente razonable para el rango motor ocular. */
    #define RIG_ARKIT_GAZE_RANGE_DEG 35.0f
    float gl_yaw_n = gaze_left_yaw_deg / RIG_ARKIT_GAZE_RANGE_DEG;
    float gr_yaw_n = gaze_right_yaw_deg / RIG_ARKIT_GAZE_RANGE_DEG;
    float gl_pitch_n = gaze_left_pitch_deg / RIG_ARKIT_GAZE_RANGE_DEG;
    float gr_pitch_n = gaze_right_pitch_deg / RIG_ARKIT_GAZE_RANGE_DEG;

    out->weights[RIG_ARKIT_eyeLookInLeft]   = rig_arkit_clamp01(gl_yaw_n);
    out->weights[RIG_ARKIT_eyeLookOutLeft]  = rig_arkit_clamp01(-gl_yaw_n);
    out->weights[RIG_ARKIT_eyeLookOutRight] = rig_arkit_clamp01(gr_yaw_n);
    out->weights[RIG_ARKIT_eyeLookInRight]  = rig_arkit_clamp01(-gr_yaw_n);

    out->weights[RIG_ARKIT_eyeLookUpLeft]    = rig_arkit_clamp01(-gl_pitch_n);
    out->weights[RIG_ARKIT_eyeLookDownLeft]  = rig_arkit_clamp01(gl_pitch_n);
    out->weights[RIG_ARKIT_eyeLookUpRight]   = rig_arkit_clamp01(-gr_pitch_n);
    out->weights[RIG_ARKIT_eyeLookDownRight] = rig_arkit_clamp01(gr_pitch_n);
    #undef RIG_ARKIT_GAZE_RANGE_DEG

    /* mandibula: pose continua, no proviene de una tabla AU */
    out->weights[RIG_ARKIT_jawOpen] = rig_arkit_clamp01(jaw_open01);
    out->weights[RIG_ARKIT_jawLeft] = rig_arkit_clamp01(-jaw_lateral_norm);
    out->weights[RIG_ARKIT_jawRight] = rig_arkit_clamp01(jaw_lateral_norm);
    out->weights[RIG_ARKIT_jawForward] = rig_arkit_clamp01(jaw_forward01);

    /* mouthClose no tiene un AU FACS propio que lo respalde de forma
     * directa e inequivoca (AU24 ya se mapea a mouthRollUpper/Lower).
     * En vez de inventar una formula sin respaldo (ej. derivarlo como
     * "1 - jawOpen", que produciria falsos positivos con la boca abierta
     * pero los labios sellados, como al pronunciar /m/), se deja en 0.0
     * explicito: es una limitacion real y documentada, no un valor
     * inventado para que la tabla "se vea completa". Si mas adelante se
     * trackea AU24 con mayor resolucion (labios prensados vs sellados),
     * este campo puede alimentarse de ahi. */
    out->weights[RIG_ARKIT_mouthClose] = 0.0f;

    return 0;
}

/* -------------------------------------------------------------------------
 * Export JSON plano: {"browInnerUp": 0.42, "eyeBlinkLeft": 0.0, ...}
 * Formato compatible con ARFaceAnchor.blendShapes / Live Link Face.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_arkit_export_json(const RigARKitFrame *frame, char *out_json, int max_len) {
    if (!frame || !out_json || max_len <= 0) return -1;
    int written = 0;
    written += snprintf(out_json + written, (size_t)(max_len - written), "{\n");
    for (int i = 0; i < RIG_ARKIT_SHAPE_COUNT; i++) {
        if (written >= max_len - 1) return -2;
        written += snprintf(out_json + written, (size_t)(max_len - written),
            "  \"%s\": %.6f%s\n", RIG_ARKIT_SHAPE_NAMES[i], frame->weights[i],
            (i < RIG_ARKIT_SHAPE_COUNT - 1) ? "," : "");
    }
    written += snprintf(out_json + written, (size_t)(max_len - written), "}\n");
    if (written < 0 || written >= max_len) return -3;
    return written;
}

/* -------------------------------------------------------------------------
 * Serializacion binaria de un frame (para grabar secuencias de captura)
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    float timestamp_s;
    RigARKitFrame frame;
} RigARKitFileFrame;

RIGCOM_PUBLIC int rig_arkit_frame_save(const RigARKitFrame *frame, float timestamp_s, FILE *fp) {
    if (!frame || !fp) return -1;
    RigARKitFileFrame f;
    f.magic = RIG_ARKIT_MAGIC; f.version = RIG_ARKIT_VERSION; f.timestamp_s = timestamp_s; f.frame = *frame;
    if (fwrite(&f, sizeof(f), 1, fp) != 1) return -2;
    return 0;
}

RIGCOM_PUBLIC int rig_arkit_frame_load(RigARKitFrame *frame, float *out_timestamp_s, FILE *fp) {
    if (!frame || !fp) return -1;
    RigARKitFileFrame f;
    if (fread(&f, sizeof(f), 1, fp) != 1) return -2;
    if (f.magic != RIG_ARKIT_MAGIC) return -3;
    if (f.version != RIG_ARKIT_VERSION) return -4;
    *frame = f.frame;
    if (out_timestamp_s) *out_timestamp_s = f.timestamp_s;
    return 0;
}

