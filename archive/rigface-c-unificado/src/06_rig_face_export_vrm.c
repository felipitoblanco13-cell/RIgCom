/* ==========================================================================
 * 06_rig_face_export_vrm.c   —   RIGFACE :: C UNIFICADO
 *
 * Copia canonica : src_raw/rig_face_export_vrm-1.c
 * Copias fundidas: 1
 * Funciones      : 7      Unidades injertadas: 0      Variantes: 0
 *
 * Copia canonica preservada verbatim. Ninguna funcion eliminada,
 * reducida ni omitida. Las divergencias de cuerpo bajo un mismo
 * nombre se conservan como <nombre>__vN con su procedencia.
 * ========================================================================== */
/* ============================================================================
 * rig_face_export_vrm.c
 *
 * RigCom :: VRM 1.0 Humanoid Export Module (aditivo puro)
 *
 * Genera el bloque de extension VRMC_vrm (JSON) que se inserta en el GLB
 * que YA produce sovereign_gltf.c, para que el avatar sea un VRM valido
 * (formato estandar de la VRM Consortium, usado por motores VTuber/
 * Unity/UniVRM/VSeeFace) sin modificar el exportador GLB existente.
 *
 *   1. Tabla completa de huesos "humanoid" del estandar VRM 1.0 (~54
 *      huesos: tronco, extremidades y los 3 segmentos de cada dedo).
 *   2. Mapeo desacoplado: el caller asocia sus propios indices de nodo
 *      del esqueleto (los que ya asigna sovereign_gltf.c al escribir el
 *      GLB) a cada hueso VRM -- este modulo no conoce la jerarquia osea
 *      interna de Sovereign, solo valida y serializa el mapeo.
 *   3. Validador de huesos requeridos (VRM exige un subconjunto minimo;
 *      el resto es opcional) -- devuelve exactamente cuales faltan.
 *   4. Presets de expresion estandar VRM (happy/angry/sad/relaxed/
 *      surprised + visemas aa/ih/ou/ee/oh + blink/look) con sus
 *      morph target binds.
 *   5. Generador del JSON real de la extension VRMC_vrm, listo para
 *      insertar en el campo "extensions" del GLB de sovereign_gltf.c.
 *   6. Serializacion del MAPEO hueso->nodo (no del JSON en si, que se
 *      regenera siempre), para no tener que remapear en cada export.
 * ==========================================================================*/

#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif

#include <string.h>
#include <stdio.h>

#define RIG_VRM_BONE_COUNT     55
#define RIG_VRM_MAGIC          0x56524d31u /* "VRM1" */
#define RIG_VRM_VERSION        1
#define RIG_VRM_JSON_MAX       32768
#define RIG_VRM_NO_NODE        (-1)

typedef enum {
    RIG_VRM_hips=0, RIG_VRM_spine, RIG_VRM_chest, RIG_VRM_upperChest, RIG_VRM_neck, RIG_VRM_head,
    RIG_VRM_leftEye, RIG_VRM_rightEye, RIG_VRM_jaw,
    RIG_VRM_leftUpperLeg, RIG_VRM_leftLowerLeg, RIG_VRM_leftFoot, RIG_VRM_leftToes,
    RIG_VRM_rightUpperLeg, RIG_VRM_rightLowerLeg, RIG_VRM_rightFoot, RIG_VRM_rightToes,
    RIG_VRM_leftShoulder, RIG_VRM_leftUpperArm, RIG_VRM_leftLowerArm, RIG_VRM_leftHand,
    RIG_VRM_rightShoulder, RIG_VRM_rightUpperArm, RIG_VRM_rightLowerArm, RIG_VRM_rightHand,
    RIG_VRM_leftThumbProximal, RIG_VRM_leftThumbIntermediate, RIG_VRM_leftThumbDistal,
    RIG_VRM_leftIndexProximal, RIG_VRM_leftIndexIntermediate, RIG_VRM_leftIndexDistal,
    RIG_VRM_leftMiddleProximal, RIG_VRM_leftMiddleIntermediate, RIG_VRM_leftMiddleDistal,
    RIG_VRM_leftRingProximal, RIG_VRM_leftRingIntermediate, RIG_VRM_leftRingDistal,
    RIG_VRM_leftLittleProximal, RIG_VRM_leftLittleIntermediate, RIG_VRM_leftLittleDistal,
    RIG_VRM_rightThumbProximal, RIG_VRM_rightThumbIntermediate, RIG_VRM_rightThumbDistal,
    RIG_VRM_rightIndexProximal, RIG_VRM_rightIndexIntermediate, RIG_VRM_rightIndexDistal,
    RIG_VRM_rightMiddleProximal, RIG_VRM_rightMiddleIntermediate, RIG_VRM_rightMiddleDistal,
    RIG_VRM_rightRingProximal, RIG_VRM_rightRingIntermediate, RIG_VRM_rightRingDistal,
    RIG_VRM_rightLittleProximal, RIG_VRM_rightLittleIntermediate, RIG_VRM_rightLittleDistal
} RigVRMBoneIndex;

static const char *RIG_VRM_BONE_NAMES[RIG_VRM_BONE_COUNT] = {
    "hips","spine","chest","upperChest","neck","head","leftEye","rightEye","jaw",
    "leftUpperLeg","leftLowerLeg","leftFoot","leftToes",
    "rightUpperLeg","rightLowerLeg","rightFoot","rightToes",
    "leftShoulder","leftUpperArm","leftLowerArm","leftHand",
    "rightShoulder","rightUpperArm","rightLowerArm","rightHand",
    "leftThumbProximal","leftThumbIntermediate","leftThumbDistal",
    "leftIndexProximal","leftIndexIntermediate","leftIndexDistal",
    "leftMiddleProximal","leftMiddleIntermediate","leftMiddleDistal",
    "leftRingProximal","leftRingIntermediate","leftRingDistal",
    "leftLittleProximal","leftLittleIntermediate","leftLittleDistal",
    "rightThumbProximal","rightThumbIntermediate","rightThumbDistal",
    "rightIndexProximal","rightIndexIntermediate","rightIndexDistal",
    "rightMiddleProximal","rightMiddleIntermediate","rightMiddleDistal",
    "rightRingProximal","rightRingIntermediate","rightRingDistal",
    "rightLittleProximal","rightLittleIntermediate","rightLittleDistal"
};

/* huesos exigidos por la especificacion VRM 1.0 (los demas son opcionales) */
static const int RIG_VRM_REQUIRED_BONES[] = {
    RIG_VRM_hips, RIG_VRM_spine, RIG_VRM_head,
    RIG_VRM_leftUpperLeg, RIG_VRM_leftLowerLeg, RIG_VRM_leftFoot,
    RIG_VRM_rightUpperLeg, RIG_VRM_rightLowerLeg, RIG_VRM_rightFoot,
    RIG_VRM_leftUpperArm, RIG_VRM_leftLowerArm, RIG_VRM_leftHand,
    RIG_VRM_rightUpperArm, RIG_VRM_rightLowerArm, RIG_VRM_rightHand
};
#define RIG_VRM_REQUIRED_COUNT (int)(sizeof(RIG_VRM_REQUIRED_BONES)/sizeof(RIG_VRM_REQUIRED_BONES[0]))

typedef struct {
    int node_index[RIG_VRM_BONE_COUNT]; /* indice de nodo glTF asignado por sovereign_gltf.c, o RIG_VRM_NO_NODE */
} RigVRMBoneMap;

typedef struct {
    float happy, angry, sad, relaxed, surprised;
    float aa, ih, ou, ee, oh;         /* visemas */
    float blink, blink_left, blink_right;
    float look_up, look_down, look_left, look_right;
} RigVRMExpressionState;

/* -------------------------------------------------------------------------
 * Mapeo de huesos
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_vrm_bonemap_init(RigVRMBoneMap *map) {
    if (!map) return -1;
    for (int i = 0; i < RIG_VRM_BONE_COUNT; i++) map->node_index[i] = RIG_VRM_NO_NODE;
    return 0;
}

RIGCOM_PUBLIC int rig_vrm_bonemap_set(RigVRMBoneMap *map, RigVRMBoneIndex bone, int gltf_node_index) {
    if (!map || bone < 0 || bone >= RIG_VRM_BONE_COUNT) return -1;
    map->node_index[bone] = gltf_node_index;
    return 0;
}

/* Devuelve 0 si estan todos los huesos requeridos; si no, escribe hasta
 * max_missing nombres faltantes en out_missing_names y devuelve cuantos
 * faltan en total (aunque excedan max_missing). */
RIGCOM_PUBLIC int rig_vrm_bonemap_validate(const RigVRMBoneMap *map, const char **out_missing_names, int max_missing) {
    if (!map) return -1;
    int missing_count = 0;
    for (int i = 0; i < RIG_VRM_REQUIRED_COUNT; i++) {
        int bone = RIG_VRM_REQUIRED_BONES[i];
        if (map->node_index[bone] == RIG_VRM_NO_NODE) {
            if (out_missing_names && missing_count < max_missing) {
                out_missing_names[missing_count] = RIG_VRM_BONE_NAMES[bone];
            }
            missing_count++;
        }
    }
    return missing_count;
}

/* -------------------------------------------------------------------------
 * Generacion del JSON de la extension VRMC_vrm (spec VRM 1.0)
 * ---------------------------------------------------------------------- */
static int rig_vrm_emit_expression_preset(char *buf, int max_len, int written,
                                           const char *name, float weight,
                                           const char *bind_node_target, int morph_target_index,
                                           int is_last) {
    return written + snprintf(buf + written, (size_t)(max_len - written),
        "    \"%s\": {\n"
        "      \"morphTargetBinds\": [ { \"node\": %s, \"index\": %d, \"weight\": %.4f } ],\n"
        "      \"isBinary\": false,\n"
        "      \"overrideBlink\": \"none\",\n"
        "      \"overrideLookAt\": \"none\",\n"
        "      \"overrideMouth\": \"none\"\n"
        "    }%s\n", name, bind_node_target, morph_target_index, weight, is_last ? "" : ",");
}

RIGCOM_PUBLIC int rig_vrm_generate_extension_json(const RigVRMBoneMap *map,
                                                   const RigVRMExpressionState *expr,
                                                   int mesh_node_index,
                                                   char *out_json, int max_len) {
    if (!map || !expr || !out_json || max_len <= 0) return -1;

    const char *missing[8];
    int missing_count = rig_vrm_bonemap_validate(map, missing, 8);
    if (missing_count > 0) return -2; /* el caller debe completar el mapeo antes de exportar */

    int w = 0;
    w += snprintf(out_json + w, (size_t)(max_len - w),
        "{\n  \"specVersion\": \"1.0\",\n  \"humanoid\": {\n    \"humanBones\": {\n");

    for (int i = 0; i < RIG_VRM_BONE_COUNT; i++) {
        if (map->node_index[i] == RIG_VRM_NO_NODE) continue; /* opcional no mapeado: se omite, no se inventa */
        if (w >= max_len - 64) return -3;
        w += snprintf(out_json + w, (size_t)(max_len - w),
            "      \"%s\": { \"node\": %d }%s\n",
            RIG_VRM_BONE_NAMES[i], map->node_index[i],
            (i == RIG_VRM_BONE_COUNT - 1) ? "" : ",");
    }
    /* nota: la coma final de la ultima entrada realmente escrita se corrige
     * abajo con un segundo paso simple, para JSON valido sin logica fragil
     * de "es el ultimo" cuando hay huesos opcionales salteados. */

    w += snprintf(out_json + w, (size_t)(max_len - w), "    }\n  },\n");

    char node_str[16];
    snprintf(node_str, sizeof(node_str), "%d", mesh_node_index);

    w += snprintf(out_json + w, (size_t)(max_len - w), "  \"expressions\": {\n    \"preset\": {\n");
    struct { const char *name; float weight; int idx; } presets[] = {
        {"happy", expr->happy, 0}, {"angry", expr->angry, 1}, {"sad", expr->sad, 2},
        {"relaxed", expr->relaxed, 3}, {"surprised", expr->surprised, 4},
        {"aa", expr->aa, 5}, {"ih", expr->ih, 6}, {"ou", expr->ou, 7},
        {"ee", expr->ee, 8}, {"oh", expr->oh, 9},
        {"blink", expr->blink, 10}, {"blinkLeft", expr->blink_left, 11}, {"blinkRight", expr->blink_right, 12},
        {"lookUp", expr->look_up, 13}, {"lookDown", expr->look_down, 14},
        {"lookLeft", expr->look_left, 15}, {"lookRight", expr->look_right, 16}
    };
    int preset_count = (int)(sizeof(presets) / sizeof(presets[0]));
    for (int i = 0; i < preset_count; i++) {
        if (w >= max_len - 256) return -4;
        w = rig_vrm_emit_expression_preset(out_json, max_len, w, presets[i].name, presets[i].weight,
                                            node_str, presets[i].idx, i == preset_count - 1);
    }
    w += snprintf(out_json + w, (size_t)(max_len - w), "    }\n  }\n}\n");

    if (w < 0 || w >= max_len) return -5;

    /* --- segunda pasada: eliminar comas colgantes antes de "}" que hayan
     * quedado por huesos opcionales omitidos, dejando JSON estrictamente
     * valido en vez de asumir que el ultimo indice escrito siempre existe. */
    for (int i = 0; i < w - 1; i++) {
        if (out_json[i] == ',') {
            int j = i + 1;
            while (j < w && (out_json[j] == '\n' || out_json[j] == ' ')) j++;
            if (j < w && out_json[j] == '}') {
                /* desplaza el resto del buffer un caracter a la izquierda, borrando la coma */
                memmove(out_json + i, out_json + i + 1, (size_t)(w - i - 1));
                w--;
                out_json[w] = '\0';
            }
        }
    }
    return w;
}

/* -------------------------------------------------------------------------
 * Serializacion del mapeo hueso->nodo (evita remapear en cada export)
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int payload_size;
} RigVRMFileHeader;

RIGCOM_PUBLIC int rig_vrm_bonemap_save(const RigVRMBoneMap *map, FILE *fp) {
    if (!map || !fp) return -1;
    RigVRMFileHeader hdr = { RIG_VRM_MAGIC, RIG_VRM_VERSION, (unsigned int)sizeof(RigVRMBoneMap) };
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (fwrite(map, sizeof(RigVRMBoneMap), 1, fp) != 1) return -3;
    return 0;
}

RIGCOM_PUBLIC int rig_vrm_bonemap_load(RigVRMBoneMap *map, FILE *fp) {
    if (!map || !fp) return -1;
    RigVRMFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (hdr.magic != RIG_VRM_MAGIC) return -3;
    if (hdr.version != RIG_VRM_VERSION) return -4;
    if (hdr.payload_size != (unsigned int)sizeof(RigVRMBoneMap)) return -5;
    if (fread(map, sizeof(RigVRMBoneMap), 1, fp) != 1) return -6;
    return 0;
}

