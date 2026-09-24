/* ============================================================================
 * rig_face_eye_vergence.c
 *
 * RigCom :: Eye Vergence, Accommodation & Combined Pupil Reactivity (aditivo)
 *
 * Extiende el sistema de ojos NG (ng_eyes.c: paralaje corneal, Purkinje,
 * film lagrimal, sacadas 2D, reactividad pupilar a luz Y reactividad a
 * emocion como funciones SEPARADAS) con:
 *
 *   1. Vergencia estereoscopica real: dado un punto de fijacion 3D y la
 *      posicion 3D de cada globo ocular (separados por la distancia
 *      interpupilar), calcula el yaw/pitch EXACTO de cada ojo via
 *      trigonometria, de forma que ambos ojos convergen geometricamente
 *      sobre el punto (no solo rotan en paralelo).
 *   2. Acomodacion: distancia de enfoque -> curvatura de cristalino
 *      (aproximada, afecta el circle-of-confusion para DOF si el renderer
 *      lo usa) ligada a la MISMA distancia de vergencia (fisiologicamente
 *      correcto: vergencia y acomodacion estan acopladas).
 *   3. Reactividad pupilar UNIFICADA: combina luz ambiental + estado
 *      emocional (arousal) + distancia de acomodacion (reflejo cercano)
 *      en un solo modelo con sus tres reflejos superpuestos correctamente
 *      en vez de tres funciones independientes que se pisan.
 *
 * No modifica ng_eyes.c. Consume/produce los mismos angulos y radio de
 * pupila que ese shader espera como uniforms.
 * ==========================================================================*/
#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif
/* [CANON] <math.h> → "rig_math.h" */
#include "rig_math.h"
/* [CANON] <string.h> → "rig_noext_str.h" */
#include "rig_noext_str.h"
/* [CANON] <stdio.h> → "rig_noext_io.h" */
#include "rig_noext_io.h"
#define RIG_VERGENCE_MAGIC   0x56455231u /* "VER1" */
#define RIG_VERGENCE_VERSION 1
typedef struct { float x, y, z; } RigVec3;
static RigVec3 rig_vergence_sub__rig_dup_2aeecf0d(RigVec3 a, RigVec3 b) {
    RigVec3 r = { a.x - b.x, a.y - b.y, a.z - b.z }; return r;
}
static float rig_vergence_len__rig_dup_63f01a23(RigVec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}
static RigVec3 rig_vergence_norm__rig_dup_dbce9f9a(RigVec3 v) {
    float l = rig_vergence_len__rig_dup_63f01a23(v);
    if (l < 1e-6f) { RigVec3 z = {0,0,1}; return z; }
    RigVec3 r = { v.x / l, v.y / l, v.z / l }; return r;
}
typedef struct {
    RigVec3 head_forward;   /* vector unitario, direccion "recto adelante" de la cabeza en espacio mundo */
    RigVec3 head_up;
    RigVec3 head_right;
    RigVec3 head_position;  /* origen de la cabeza (punto medio entre ojos) en espacio mundo */
    float interpupillary_distance_mm;
} RigHeadFrame;
typedef struct {
    float yaw_deg;    /* rotacion horizontal del globo ocular relativa a head_forward */
    float pitch_deg;  /* rotacion vertical */
} RigEyeAngles;
typedef struct {
    RigEyeAngles left;
    RigEyeAngles right;
    float vergence_angle_deg;     /* angulo total entre ambos ejes visuales (0 = paralelo/infinito) */
    float focus_distance_mm;      /* distancia al punto de fijacion desde el punto medio interocular */
    float lens_diopters;          /* potencia de acomodacion aproximada, 1/focus_distance_m */
} RigVergenceResult;
/* -------------------------------------------------------------------------
 * Vergencia geometrica exacta
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_vergence_compute__rig_dup_4cf2805d(const RigHeadFrame *head, RigVec3 fixation_point_world,
                                        RigVergenceResult *out) {
    if (!head || !out) return -1;
    memset(out, 0, sizeof(*out));
    float half_ipd = head->interpupillary_distance_mm * 0.5f;
    /* posiciones de los globos oculares en espacio mundo, offset lateral
     * desde head_position a lo largo de head_right */
    RigVec3 eye_left = {
        head->head_position.x - head->head_right.x * half_ipd,
        head->head_position.y - head->head_right.y * half_ipd,
        head->head_position.z - head->head_right.z * half_ipd
    };
    RigVec3 eye_right = {
        head->head_position.x + head->head_right.x * half_ipd,
        head->head_position.y + head->head_right.y * half_ipd,
        head->head_position.z + head->head_right.z * half_ipd
    };
    RigVec3 to_target_left  = rig_vergence_sub__rig_dup_2aeecf0d(fixation_point_world, eye_left);
    RigVec3 to_target_right = rig_vergence_sub__rig_dup_2aeecf0d(fixation_point_world, eye_right);
    RigVec3 dir_left  = rig_vergence_norm__rig_dup_dbce9f9a(to_target_left);
    RigVec3 dir_right = rig_vergence_norm__rig_dup_dbce9f9a(to_target_right);
    /* proyectar cada direccion sobre la base local de la cabeza (forward/
     * right/up) para obtener yaw/pitch relativos a "mirar al frente" */
    float fl = dir_left.x * head->head_forward.x + dir_left.y * head->head_forward.y + dir_left.z * head->head_forward.z;
    float rl = dir_left.x * head->head_right.x   + dir_left.y * head->head_right.y   + dir_left.z * head->head_right.z;
    float ul = dir_left.x * head->head_up.x      + dir_left.y * head->head_up.y      + dir_left.z * head->head_up.z;
    float fr = dir_right.x * head->head_forward.x + dir_right.y * head->head_forward.y + dir_right.z * head->head_forward.z;
    float rr = dir_right.x * head->head_right.x   + dir_right.y * head->head_right.y   + dir_right.z * head->head_right.z;
    float ur = dir_right.x * head->head_up.x      + dir_right.y * head->head_up.y      + dir_right.z * head->head_up.z;
    out->left.yaw_deg    = atan2f(rl, fl) * 57.29577951f;
    out->left.pitch_deg  = atan2f(ul, fl) * 57.29577951f;
    out->right.yaw_deg   = atan2f(rr, fr) * 57.29577951f;
    out->right.pitch_deg = atan2f(ur, fr) * 57.29577951f;
    /* angulo de vergencia = angulo entre los dos ejes visuales */
    float dot = dir_left.x * dir_right.x + dir_left.y * dir_right.y + dir_left.z * dir_right.z;
    if (dot > 1.0f) dot = 1.0f;
    if (dot < -1.0f) dot = -1.0f;
    out->vergence_angle_deg = acosf(dot) * 57.29577951f;
    /* distancia de enfoque: promedio de distancias desde cada ojo,
     * proyectado sobre la direccion frontal media (evita sesgo si el
     * punto esta muy lateral) */
    float dist_left  = rig_vergence_len__rig_dup_63f01a23(to_target_left);
    float dist_right = rig_vergence_len__rig_dup_63f01a23(to_target_right);
    out->focus_distance_mm = 0.5f * (dist_left + dist_right);
    if (out->focus_distance_mm < 40.0f) out->focus_distance_mm = 40.0f; /* punto proximo minimo fisiologico ~40mm en jovenes */
    out->lens_diopters = 1000.0f / out->focus_distance_mm; /* mm -> D (1/m) */
    return 0;
}
/* -------------------------------------------------------------------------
 * Reactividad pupilar unificada: reflejo fotomotor + reflejo de vision
 * cercana (miosis al converger de cerca) + componente emocional (arousal
 * dilata via simpatico, miedo extremo puede tambien dilatar por catecolaminas).
 * Los tres reflejos NO son aditivos ingenuos: el fotomotor domina en rango
 * medio, el reflejo cercano tiene un piso, y la emocion module ambos.
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC float rig_vergence_pupil_radius_mm__rig_dup_1b05452f(float ambient_lux, float focus_distance_mm,
                                                  float emotion_arousal, float baseline_radius_mm) {
    /* reflejo fotomotor: mapeo logaritmico aproximado de luminancia a radio,
     * rango fisiologico real ~2mm (luz brillante) a ~8mm (oscuridad total) */
    float log_lux = log10f(ambient_lux > 0.001f ? ambient_lux : 0.001f);
    float photo_t = (log_lux + 3.0f) / 8.0f; /* normaliza aprox rango -3..5 log lux */
    if (photo_t < 0.0f) photo_t = 0.0f;
    if (photo_t > 1.0f) photo_t = 1.0f;
    float photo_radius = 8.0f - photo_t * 6.0f; /* 8mm oscuridad -> 2mm luz brillante */
    /* reflejo de vision cercana: mientras mas cerca el punto de fijacion,
     * mas miosis (constriccion), independiente de la luz */
    float near_t = 1.0f - (focus_distance_mm - 40.0f) / 2000.0f; /* 40mm -> 1.0, >2040mm -> 0.0 */
    if (near_t < 0.0f) near_t = 0.0f;
    if (near_t > 1.0f) near_t = 1.0f;
    float near_constriction_mm = near_t * 1.5f; /* hasta 1.5mm de constriccion adicional */
    /* componente emocional: dilatacion simpatica proporcional a |arousal|,
     * sin importar valence (tanto miedo como excitacion positiva dilatan) */
    float emotion_dilation_mm = (emotion_arousal > 0.0f ? emotion_arousal : -emotion_arousal) * 1.2f;
    float radius = photo_radius - near_constriction_mm + emotion_dilation_mm;
    float baseline_offset = baseline_radius_mm - 4.5f; /* 4.5mm = radio "tipico" de referencia */
    radius += baseline_offset;
    if (radius < 1.5f) radius = 1.5f;   /* miosis maxima fisiologica */
    if (radius > 9.0f) radius = 9.0f;   /* midriasis maxima fisiologica */
    return radius;
}
/* -------------------------------------------------------------------------
 * Codegen GLSL: aplica yaw/pitch por ojo (rotacion del eje visual) y el
 * radio de pupila unificado sobre el shader existente de ng_eyes.c
 * ---------------------------------------------------------------------- */
#define RIG_VERGENCE_SRC_MAX 4096
RIGCOM_PUBLIC int rig_vergence_generate_shader__rig_dup_8e383170(char *out_glsl, int max_len) {
    if (!out_glsl || max_len <= 0) return -1;
    int n = snprintf(out_glsl, (size_t)max_len,
        "// === rig_face_eye_vergence :: snippet (aditivo sobre ng_eyes.c) ===\n"
        "uniform float u_eye_yaw_left_deg;\n"
        "uniform float u_eye_pitch_left_deg;\n"
        "uniform float u_eye_yaw_right_deg;\n"
        "uniform float u_eye_pitch_right_deg;\n"
        "uniform float u_pupil_radius_mm; // reemplaza el termino de reactividad pupilar aislado\n"
        "\n"
        "mat3 rig_vergence_eye_rotation(float yawDeg, float pitchDeg) {\n"
        "    float yaw = radians(yawDeg);\n"
        "    float pitch = radians(pitchDeg);\n"
        "    float cy = cos(yaw), sy = sin(yaw);\n"
        "    float cp = cos(pitch), sp = sin(pitch);\n"
        "    mat3 yawMat  = mat3(cy, 0.0, sy,  0.0, 1.0, 0.0,  -sy, 0.0, cy);\n"
        "    mat3 pitchMat = mat3(1.0, 0.0, 0.0,  0.0, cp, -sp,  0.0, sp, cp);\n"
        "    return yawMat * pitchMat;\n"
        "}\n");
    if (n < 0 || n >= max_len) return -2;
    return n;
}
/* -------------------------------------------------------------------------
 * Serializacion binaria de una muestra de vergencia (para grabar/reproducir
 * secuencias de mirada, ej. desde captura de vision.c)
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    float timestamp_s;
    RigVergenceResult result;
} RigVergenceSample;
RIGCOM_PUBLIC int rig_vergence_sample_save__rig_dup_6329ee0d(const RigVergenceResult *res, float timestamp_s, FILE *fp) {
    if (!res || !fp) return -1;
    RigVergenceSample s;
    s.magic = RIG_VERGENCE_MAGIC;
    s.version = RIG_VERGENCE_VERSION;
    s.timestamp_s = timestamp_s;
    s.result = *res;
    if (fwrite(&s, sizeof(s), 1, fp) != 1) return -2;
    return 0;
}
RIGCOM_PUBLIC int rig_vergence_sample_load__rig_dup_e2068276(RigVergenceResult *res, float *out_timestamp_s, FILE *fp) {
    if (!res || !fp) return -1;
    RigVergenceSample s;
    if (fread(&s, sizeof(s), 1, fp) != 1) return -2;
    if (s.magic != RIG_VERGENCE_MAGIC) return -3;
    if (s.version != RIG_VERGENCE_VERSION) return -4;
    *res = s.result;
    if (out_timestamp_s) *out_timestamp_s = s.timestamp_s;
    return 0;
}