/* ============================================================================
 * rig_face_dentition.c
 *
 * RigCom :: Per-Tooth Dental Arcade Module (aditivo puro)
 *
 * Reemplaza la boca de "6 cajas planas" (v2.c avatar assembly) por una
 * arcada dental completa de 32 piezas permanentes, numeracion FDI real
 * (11-18, 21-28, 31-38, 41-48), con:
 *
 *   1. Geometria real por diente via lathe (superficie de revolucion) con
 *      perfil especifico por clase dentaria (incisivo/canino/premolar/molar),
 *      seccion transversal eliptica (mesiodistal != bucolingual), cuspides
 *      reales en premolares/molares via perturbacion radial armonica.
 *   2. Raices anatomicamente plausibles (mono/bi/trirradiculares segun
 *      clase), con abanico de raices en molares.
 *   3. Colocacion a lo largo de una curva de arco dental parametrica
 *      (arco superior e inferior independientes).
 *   4. Maloclusion real: rotacion/traslacion rigida POR DIENTE (rotacion,
 *      apinamiento, version, impaccion parcial), no solo el arco ideal.
 *   5. Tincion (staining) por diente con patron realista (mas intenso
 *      cervical/interproximal), como color por-vertice.
 *   6. Snippet GLSL/JS para reemplazar el material plano de dientes actual.
 *   7. Serializacion del genoma dental (definicion parametrica, NO la
 *      malla derivada -- la malla se regenera siempre desde la definicion).
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
#define RIG_DENT_TOOTH_COUNT      32
#define RIG_DENT_RADIAL_SEGMENTS  12
#define RIG_DENT_CROWN_RINGS      8
#define RIG_DENT_ROOT_RINGS       6
#define RIG_DENT_MAX_ROOTS        3
#define RIG_DENT_MAX_VERTS        16384
#define RIG_DENT_MAX_INDICES      65536
#define RIG_DENTITION_MAGIC       0x44454e31u /* "DEN1" */
#define RIG_DENTITION_VERSION     1
typedef struct { float x, y, z; } RigDentVec3;
typedef enum {
    RIG_TOOTH_CENTRAL_INCISOR = 0,
    RIG_TOOTH_LATERAL_INCISOR,
    RIG_TOOTH_CANINE,
    RIG_TOOTH_PREMOLAR_1,
    RIG_TOOTH_PREMOLAR_2,
    RIG_TOOTH_MOLAR_1,
    RIG_TOOTH_MOLAR_2,
    RIG_TOOTH_MOLAR_3,
    RIG_TOOTH_CLASS_COUNT
} RigToothClass;
/* Perfil de radio normalizado (6 puntos de control, h=0 cervical .. h=1
 * borde incisal/oclusal) y dimensiones medias reales de anatomia dental
 * adulta (mm). Estos son promedios anatomicos estandar de referencia. */
typedef struct {
    float radius_profile[6];   /* radio relativo (se escala por mesiodistal/2) en h=0,0.2,0.4,0.6,0.8,1.0 */
    float crown_height_mm;
    float mesiodistal_mm;
    float buccolingual_mm;
    float root_length_mm;
    int   root_count;
    int   cusp_count;          /* 0 = borde incisal liso (incisivos), 1 = cuspide unica (canino) */
    float cusp_amplitude;      /* 0..1, relativo al radio */
} RigToothClassProfile;
static const RigToothClassProfile RIG_TOOTH_PROFILES[RIG_TOOTH_CLASS_COUNT] = {
    /* radius_profile[6]                      crown  mesio bucco root  roots cusps amp */
    { {0.55f,0.75f,0.90f,0.95f,0.70f,0.30f},   10.5f, 8.5f, 7.0f, 13.0f, 1, 0, 0.0f },  /* incisivo central */
    { {0.50f,0.68f,0.82f,0.88f,0.65f,0.25f},    9.0f, 6.5f, 6.0f, 13.0f, 1, 0, 0.0f },  /* incisivo lateral */
    { {0.55f,0.72f,0.85f,0.80f,0.55f,0.10f},   10.0f, 7.5f, 8.0f, 17.0f, 1, 1, 0.35f }, /* canino */
    { {0.60f,0.80f,0.92f,0.95f,0.85f,0.55f},    8.5f, 7.0f, 9.0f, 14.0f, 1, 2, 0.22f }, /* premolar 1 */
    { {0.58f,0.78f,0.90f,0.93f,0.82f,0.55f},    8.0f, 7.0f, 9.0f, 14.0f, 1, 2, 0.18f }, /* premolar 2 */
    { {0.65f,0.85f,0.95f,0.98f,0.90f,0.60f},    7.5f,10.5f,11.0f, 13.0f, 3, 4, 0.28f }, /* molar 1 (3 raices sup./2 inf. -> usamos 3 max) */
    { {0.65f,0.85f,0.95f,0.97f,0.88f,0.58f},    7.0f,10.0f,10.5f, 12.5f, 3, 4, 0.25f }, /* molar 2 */
    { {0.62f,0.80f,0.90f,0.92f,0.80f,0.50f},    6.5f, 9.0f, 9.5f, 11.0f, 2, 3, 0.20f }  /* molar 3 (cordal) */
};
typedef struct {
    int   fdi_number;       /* numeracion FDI real, ej. 11, 26, 48 */
    int   tooth_class;      /* RigToothClass */
    int   is_upper;
    int   arch_slot;        /* 0 = linea media, hasta 7 = mas distal (cordal) */
    int   is_right_side;
    /* --- geometria/genoma editable, se serializa --- */
    float size_scale;             /* 1.0 = tamano promedio de clase */
    float rotation_deg[3];        /* maloclusion: rotacion rigida x/y/z */
    float translation_mm[3];      /* maloclusion: traslacion rigida x/y/z respecto a posicion ideal de arco */
    float stain_level;            /* 0..1 */
    float stain_color_rgb[3];
    int   present;                /* 0 = agenesia / extraido, no genera geometria */
} RigToothInstance;
typedef struct {
    RigToothInstance teeth[RIG_DENT_TOOTH_COUNT];
    float upper_arch_width_mm;
    float upper_arch_depth_mm;
    float lower_arch_width_mm;
    float lower_arch_depth_mm;
    float enamel_base_color_rgb[3];
} RigDentalArcade;
typedef struct {
    float positions[RIG_DENT_MAX_VERTS][3];
    float normals[RIG_DENT_MAX_VERTS][3];
    float stain_color[RIG_DENT_MAX_VERTS][3];
    int   fdi_of_vertex[RIG_DENT_MAX_VERTS];
    int   vertex_count;
    unsigned int indices[RIG_DENT_MAX_INDICES];
    int   index_count;
} RigDentalMesh;
/* -------------------------------------------------------------------------
 * Definicion FDI estandar: 32 posiciones = 4 cuadrantes x 8 posiciones
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_dentition_arcade_init_standard__rig_dup_305ecb42(RigDentalArcade *arc) {
    if (!arc) return -1;
    memset(arc, 0, sizeof(*arc));
    arc->upper_arch_width_mm = 60.0f;
    arc->upper_arch_depth_mm = 45.0f;
    arc->lower_arch_width_mm = 56.0f;
    arc->lower_arch_depth_mm = 42.0f;
    arc->enamel_base_color_rgb[0] = 0.92f;
    arc->enamel_base_color_rgb[1] = 0.90f;
    arc->enamel_base_color_rgb[2] = 0.82f;
    static const int class_by_slot[8] = {
        RIG_TOOTH_CENTRAL_INCISOR, RIG_TOOTH_LATERAL_INCISOR, RIG_TOOTH_CANINE,
        RIG_TOOTH_PREMOLAR_1, RIG_TOOTH_PREMOLAR_2,
        RIG_TOOTH_MOLAR_1, RIG_TOOTH_MOLAR_2, RIG_TOOTH_MOLAR_3
    };
    int idx = 0;
    /* cuadrante 1: superior derecho, FDI 11-18 */
    for (int slot = 0; slot < 8; slot++, idx++) {
        RigToothInstance *t = &arc->teeth[idx];
        t->fdi_number = 11 + slot; t->tooth_class = class_by_slot[slot];
        t->is_upper = 1; t->is_right_side = 1; t->arch_slot = slot;
        t->size_scale = 1.0f; t->present = 1;
        t->stain_color_rgb[0] = 0.55f; t->stain_color_rgb[1] = 0.42f; t->stain_color_rgb[2] = 0.18f;
    }
    /* cuadrante 2: superior izquierdo, FDI 21-28 */
    for (int slot = 0; slot < 8; slot++, idx++) {
        RigToothInstance *t = &arc->teeth[idx];
        t->fdi_number = 21 + slot; t->tooth_class = class_by_slot[slot];
        t->is_upper = 1; t->is_right_side = 0; t->arch_slot = slot;
        t->size_scale = 1.0f; t->present = 1;
        t->stain_color_rgb[0] = 0.55f; t->stain_color_rgb[1] = 0.42f; t->stain_color_rgb[2] = 0.18f;
    }
    /* cuadrante 3: inferior izquierdo, FDI 31-38 */
    for (int slot = 0; slot < 8; slot++, idx++) {
        RigToothInstance *t = &arc->teeth[idx];
        t->fdi_number = 31 + slot; t->tooth_class = class_by_slot[slot];
        t->is_upper = 0; t->is_right_side = 0; t->arch_slot = slot;
        t->size_scale = 1.0f; t->present = 1;
        t->stain_color_rgb[0] = 0.55f; t->stain_color_rgb[1] = 0.42f; t->stain_color_rgb[2] = 0.18f;
    }
    /* cuadrante 4: inferior derecho, FDI 41-48 */
    for (int slot = 0; slot < 8; slot++, idx++) {
        RigToothInstance *t = &arc->teeth[idx];
        t->fdi_number = 41 + slot; t->tooth_class = class_by_slot[slot];
        t->is_upper = 0; t->is_right_side = 1; t->arch_slot = slot;
        t->size_scale = 1.0f; t->present = 1;
        t->stain_color_rgb[0] = 0.55f; t->stain_color_rgb[1] = 0.42f; t->stain_color_rgb[2] = 0.18f;
    }
    return 0;
}
/* -------------------------------------------------------------------------
 * Curva de arco dental: parabola generalizada. slot 0 = incisivo central
 * (linea media), slot 7 = cordal (mas posterior). t en [0,1] a lo largo
 * del semiarco; devuelve posicion local (x=lateral, z=antero-posterior)
 * y angulo de orientacion (normal hacia afuera del arco).
 * ---------------------------------------------------------------------- */
static void rig_dent_arch_position__rig_dup_d2ecbb76(float arch_width_mm, float arch_depth_mm,
                                    int slot, int is_right_side,
                                    RigDentVec3 *out_pos, float *out_yaw_deg) {
    /* posicion angular a lo largo de una media elipse, 0=frente, PI/2=lateral */
    float t = (float)slot / 7.0f;               /* 0..1 */
    float theta = t * 1.65f;                     /* hasta ~94 grados: cubre hasta el 3er molar */
    float a = arch_width_mm * 0.5f;
    float b = arch_depth_mm;
    float x = a * sinf(theta);
    float z = -b * (1.0f - cosf(theta));         /* z negativo = hacia posterior */
    if (!is_right_side) x = -x;
    out_pos->x = x;
    out_pos->y = 0.0f;
    out_pos->z = z;
    /* yaw: la normal de la superficie del arco en ese punto, aproximada
     * como la derivada de la curva rotada 90 grados */
    float dtheta = 0.01f;
    float x2 = a * sinf(theta + dtheta);
    float z2 = -b * (1.0f - cosf(theta + dtheta));
    if (!is_right_side) x2 = -x2;
    float tangent_x = x2 - x;
    float tangent_z = z2 - z;
    float yaw = atan2f(tangent_x, tangent_z) * 57.29577951f;
    *out_yaw_deg = yaw + (is_right_side ? 90.0f : -90.0f);
}
/* -------------------------------------------------------------------------
 * Generacion de un diente individual (corona + raiz) por lathe, con
 * cuspides armonicas y maloclusion rigida aplicada. Devuelve cuantos
 * vertices/indices agrego, o negativo si no entra en los buffers.
 * ---------------------------------------------------------------------- */
static float rig_dent_lerp__rig_dup_2b396547(float a, float b, float t) { return a + (b - a) * t; }
static float rig_dent_profile_radius__rig_dup_24bec5e8(const RigToothClassProfile *prof, float h) {
    /* h en [0,1], 6 puntos de control en 0,0.2,0.4,0.6,0.8,1.0 */
    float scaled = h * 5.0f;
    int i0 = (int)scaled; if (i0 > 4) i0 = 4; if (i0 < 0) i0 = 0;
    int i1 = i0 + 1;
    float local_t = scaled - (float)i0;
    /* suavizado coseno entre puntos de control para perfil organico */
    float smooth_t = 0.5f - 0.5f * cosf(local_t * 3.14159265f);
    return rig_dent_lerp__rig_dup_2b396547(prof->radius_profile[i0], prof->radius_profile[i1], smooth_t);
}
static int rig_dent_generate_tooth__rig_dup_577d2c8b(const RigToothInstance *tooth, RigDentalMesh *mesh,
                                    RigDentVec3 world_pos, float world_yaw_deg,
                                    float enamel_rgb[3]) {
    const RigToothClassProfile *prof = &RIG_TOOTH_PROFILES[tooth->tooth_class];
    float mesio = prof->mesiodistal_mm * 0.5f * tooth->size_scale;
    float bucco = prof->buccolingual_mm * 0.5f * tooth->size_scale;
    float crown_h = prof->crown_height_mm * tooth->size_scale;
    float root_len = prof->root_length_mm * tooth->size_scale;
    float yaw = (world_yaw_deg + tooth->rotation_deg[1]) * 0.017453293f;
    float pitch = tooth->rotation_deg[0] * 0.017453293f;
    float roll = tooth->rotation_deg[2] * 0.017453293f;
    float cy = cosf(yaw), sy = sinf(yaw);
    float cp = cosf(pitch), sp = sinf(pitch);
    float cr = cosf(roll), sr = sinf(roll);
    int base_vertex = mesh->vertex_count;
    int rings_total = RIG_DENT_CROWN_RINGS + RIG_DENT_ROOT_RINGS * (prof->root_count > 0 ? 1 : 0);
    int verts_needed = (rings_total + 1) * RIG_DENT_RADIAL_SEGMENTS;
    if (base_vertex + verts_needed >= RIG_DENT_MAX_VERTS) return -1;
    /* --- anillos de corona: h de 0 (cervical) a 1 (oclusal/incisal) --- */
    for (int ring = 0; ring <= RIG_DENT_CROWN_RINGS; ring++) {
        float h = (float)ring / (float)RIG_DENT_CROWN_RINGS;
        float base_r = rig_dent_profile_radius__rig_dup_24bec5e8(prof, h);
        float y_local = crown_h * h;
        for (int seg = 0; seg < RIG_DENT_RADIAL_SEGMENTS; seg++) {
            float theta = 2.0f * 3.14159265f * (float)seg / (float)RIG_DENT_RADIAL_SEGMENTS;
            float cusp_mod = 1.0f;
            if (prof->cusp_count > 0) {
                float cusp_region = h > 0.65f ? (h - 0.65f) / 0.35f : 0.0f;
                cusp_mod = 1.0f + prof->cusp_amplitude * cusp_region *
                           cosf((float)prof->cusp_count * theta);
            }
            float rx = base_r * mesio * cusp_mod;
            float rz = base_r * bucco * cusp_mod;
            float lx = rx * cosf(theta);
            float lz = rz * sinf(theta);
            float ly = y_local;
            /* rotacion local pitch/roll (inclinacion del diente) luego yaw
             * (orientacion a lo largo del arco), luego traslacion a
             * posicion de arco + offset de maloclusion */
            float y1 = ly * cp - lz * sp;
            float z1 = ly * sp + lz * cp;
            float x1 = lx * cr - y1 * sr;
            float y2 = lx * sr + y1 * cr;
            float wx = x1 * cy + z1 * sy;
            float wz = -x1 * sy + z1 * cy;
            float wy = y2;
            int vi = mesh->vertex_count;
            mesh->positions[vi][0] = world_pos.x + tooth->translation_mm[0] + wx;
            mesh->positions[vi][1] = world_pos.y + tooth->translation_mm[1] + wy;
            mesh->positions[vi][2] = world_pos.z + tooth->translation_mm[2] + wz;
            /* normal aproximada: direccion radial en el plano meridiano,
             * inclinada por la pendiente local del perfil (dr/dh) */
            float h_eps = 0.02f;
            float r_next = rig_dent_profile_radius__rig_dup_24bec5e8(prof, h + h_eps > 1.0f ? 1.0f : h + h_eps);
            float slope = (r_next - base_r) / h_eps;
            RigDentVec3 n_local = { cosf(theta), -slope * 0.4f, sinf(theta) };
            float nlen = sqrtf(n_local.x*n_local.x + n_local.y*n_local.y + n_local.z*n_local.z);
            if (nlen < 1e-6f) nlen = 1.0f;
            n_local.x /= nlen; n_local.y /= nlen; n_local.z /= nlen;
            float ny1 = n_local.y * cp - n_local.z * sp;
            float nz1 = n_local.y * sp + n_local.z * cp;
            float nx1 = n_local.x * cr - ny1 * sr;
            float ny2 = n_local.x * sr + ny1 * cr;
            float nwx = nx1 * cy + nz1 * sy;
            float nwz = -nx1 * sy + nz1 * cy;
            mesh->normals[vi][0] = nwx;
            mesh->normals[vi][1] = ny2;
            mesh->normals[vi][2] = nwz;
            float stain_bias = (h < 0.15f) ? (1.0f - h / 0.15f) : 0.0f; /* mas mancha cervical */
            float stain_amt = tooth->stain_level * (0.3f + 0.7f * stain_bias);
            mesh->stain_color[vi][0] = rig_dent_lerp__rig_dup_2b396547(enamel_rgb[0], tooth->stain_color_rgb[0], stain_amt);
            mesh->stain_color[vi][1] = rig_dent_lerp__rig_dup_2b396547(enamel_rgb[1], tooth->stain_color_rgb[1], stain_amt);
            mesh->stain_color[vi][2] = rig_dent_lerp__rig_dup_2b396547(enamel_rgb[2], tooth->stain_color_rgb[2], stain_amt);
            mesh->fdi_of_vertex[vi] = tooth->fdi_number;
            mesh->vertex_count++;
        }
    }
    /* --- indices de la corona (quads -> 2 triangulos) --- */
    for (int ring = 0; ring < RIG_DENT_CROWN_RINGS; ring++) {
        for (int seg = 0; seg < RIG_DENT_RADIAL_SEGMENTS; seg++) {
            int seg_next = (seg + 1) % RIG_DENT_RADIAL_SEGMENTS;
            unsigned int a = base_vertex + ring * RIG_DENT_RADIAL_SEGMENTS + seg;
            unsigned int b = base_vertex + ring * RIG_DENT_RADIAL_SEGMENTS + seg_next;
            unsigned int c = base_vertex + (ring + 1) * RIG_DENT_RADIAL_SEGMENTS + seg_next;
            unsigned int d = base_vertex + (ring + 1) * RIG_DENT_RADIAL_SEGMENTS + seg;
            if (mesh->index_count + 6 >= RIG_DENT_MAX_INDICES) return -2;
            mesh->indices[mesh->index_count++] = a;
            mesh->indices[mesh->index_count++] = b;
            mesh->indices[mesh->index_count++] = c;
            mesh->indices[mesh->index_count++] = a;
            mesh->indices[mesh->index_count++] = c;
            mesh->indices[mesh->index_count++] = d;
        }
    }
    /* --- raiz(ces): cada raiz es un cono tapering independiente, offset
     * lateralmente si root_count > 1 (bifurcacion/trifurcacion real) --- */
    int root_count = prof->root_count > RIG_DENT_MAX_ROOTS ? RIG_DENT_MAX_ROOTS : prof->root_count;
    float cervical_r = rig_dent_profile_radius__rig_dup_24bec5e8(prof, 0.0f);
    for (int r = 0; r < root_count; r++) {
        float root_offset_x_mm = 0.0f;
        if (root_count > 1) {
            float spread = mesio * 0.5f;
            root_offset_x_mm = spread * ((float)r / (float)(root_count - 1) * 2.0f - 1.0f);
        }
        int root_base_vertex = mesh->vertex_count;
        if (root_base_vertex + (RIG_DENT_ROOT_RINGS + 1) * RIG_DENT_RADIAL_SEGMENTS >= RIG_DENT_MAX_VERTS) return -3;
        for (int ring = 0; ring <= RIG_DENT_ROOT_RINGS; ring++) {
            float h = (float)ring / (float)RIG_DENT_ROOT_RINGS;
            float root_r = cervical_r * (1.0f - h) * 0.85f; /* tapering hacia el apice */
            float y_local = -root_len * h; /* hacia abajo/apical, y negativo local */
            for (int seg = 0; seg < RIG_DENT_RADIAL_SEGMENTS; seg++) {
                float theta = 2.0f * 3.14159265f * (float)seg / (float)RIG_DENT_RADIAL_SEGMENTS;
                float lx = root_r * mesio * cosf(theta) * 0.6f + root_offset_x_mm;
                float lz = root_r * bucco * sinf(theta) * 0.6f;
                float ly = y_local;
                float y1 = ly * cp - lz * sp;
                float z1 = ly * sp + lz * cp;
                float x1 = lx * cr - y1 * sr;
                float y2 = lx * sr + y1 * cr;
                float wx = x1 * cy + z1 * sy;
                float wz = -x1 * sy + z1 * cy;
                int vi = mesh->vertex_count;
                mesh->positions[vi][0] = world_pos.x + tooth->translation_mm[0] + wx;
                mesh->positions[vi][1] = world_pos.y + tooth->translation_mm[1] + y2;
                mesh->positions[vi][2] = world_pos.z + tooth->translation_mm[2] + wz;
                mesh->normals[vi][0] = cosf(theta) * cy;
                mesh->normals[vi][1] = 0.3f;
                mesh->normals[vi][2] = sinf(theta) * cy;
                mesh->stain_color[vi][0] = 0.85f; mesh->stain_color[vi][1] = 0.78f; mesh->stain_color[vi][2] = 0.65f; /* cemento radicular */
                mesh->fdi_of_vertex[vi] = tooth->fdi_number;
                mesh->vertex_count++;
            }
        }
        for (int ring = 0; ring < RIG_DENT_ROOT_RINGS; ring++) {
            for (int seg = 0; seg < RIG_DENT_RADIAL_SEGMENTS; seg++) {
                int seg_next = (seg + 1) % RIG_DENT_RADIAL_SEGMENTS;
                unsigned int a = root_base_vertex + ring * RIG_DENT_RADIAL_SEGMENTS + seg;
                unsigned int b = root_base_vertex + ring * RIG_DENT_RADIAL_SEGMENTS + seg_next;
                unsigned int c = root_base_vertex + (ring + 1) * RIG_DENT_RADIAL_SEGMENTS + seg_next;
                unsigned int d = root_base_vertex + (ring + 1) * RIG_DENT_RADIAL_SEGMENTS + seg;
                if (mesh->index_count + 6 >= RIG_DENT_MAX_INDICES) return -4;
                mesh->indices[mesh->index_count++] = a;
                mesh->indices[mesh->index_count++] = b;
                mesh->indices[mesh->index_count++] = c;
                mesh->indices[mesh->index_count++] = a;
                mesh->indices[mesh->index_count++] = c;
                mesh->indices[mesh->index_count++] = d;
            }
        }
    }
    return 0;
}
/* -------------------------------------------------------------------------
 * Generacion de la arcada completa (32 piezas, o menos si hay agenesias)
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_dentition_generate_arcade__rig_dup_fe4d1ac6(const RigDentalArcade *arc, RigDentalMesh *out_mesh) {
    if (!arc || !out_mesh) return -1;
    memset(out_mesh, 0, sizeof(*out_mesh));
    for (int i = 0; i < RIG_DENT_TOOTH_COUNT; i++) {
        const RigToothInstance *t = &arc->teeth[i];
        if (!t->present) continue;
        float width = t->is_upper ? arc->upper_arch_width_mm : arc->lower_arch_width_mm;
        float depth = t->is_upper ? arc->upper_arch_depth_mm : arc->lower_arch_depth_mm;
        RigDentVec3 pos; float yaw;
        rig_dent_arch_position__rig_dup_d2ecbb76(width, depth, t->arch_slot, t->is_right_side, &pos, &yaw);
        pos.y = t->is_upper ? 4.0f : -4.0f; /* separacion vertical arco superior/inferior en reposo */
        float enamel_copy[3] = { arc->enamel_base_color_rgb[0], arc->enamel_base_color_rgb[1], arc->enamel_base_color_rgb[2] };
        int rc = rig_dent_generate_tooth__rig_dup_577d2c8b(t, out_mesh, pos, yaw, enamel_copy);
        if (rc != 0) return rc; /* buffer lleno: se detiene limpio, sin corromper datos parciales */
    }
    return 0;
}
/* -------------------------------------------------------------------------
 * Codegen GLSL/JS: color por-vertice de tincion sobre el esmalte base
 * ---------------------------------------------------------------------- */
#define RIG_DENT_SRC_MAX 4096
RIGCOM_PUBLIC int rig_dentition_generate_shader__rig_dup_8e383170(char *out_glsl, int max_len) {
    if (!out_glsl || max_len <= 0) return -1;
    int n = snprintf(out_glsl, (size_t)max_len,
        "// === rig_face_dentition :: snippet (reemplaza material plano de dientes) ===\n"
        "attribute vec3 a_tooth_stain_color;\n"
        "varying vec3 v_tooth_color;\n"
        "void rig_dentition_vertex() { v_tooth_color = a_tooth_stain_color; }\n"
        "vec3 rig_dentition_fragment(vec3 subsurfaceTint) {\n"
        "    return v_tooth_color * (0.85 + 0.15 * subsurfaceTint);\n"
        "}\n");
    if (n < 0 || n >= max_len) return -2;
    return n;
}
/* -------------------------------------------------------------------------
 * Serializacion del genoma dental (definicion parametrica de las 32 piezas)
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int payload_size;
} RigDentitionFileHeader;
RIGCOM_PUBLIC int rig_dentition_arcade_save__rig_dup_dba567cf(const RigDentalArcade *arc, FILE *fp) {
    if (!arc || !fp) return -1;
    RigDentitionFileHeader hdr = { RIG_DENTITION_MAGIC, RIG_DENTITION_VERSION,
                                    (unsigned int)sizeof(RigDentalArcade) };
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (fwrite(arc, sizeof(RigDentalArcade), 1, fp) != 1) return -3;
    return 0;
}
RIGCOM_PUBLIC int rig_dentition_arcade_load__rig_dup_9683ba0f(RigDentalArcade *arc, FILE *fp) {
    if (!arc || !fp) return -1;
    RigDentitionFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (hdr.magic != RIG_DENTITION_MAGIC) return -3;
    if (hdr.version != RIG_DENTITION_VERSION) return -4;
    if (hdr.payload_size != (unsigned int)sizeof(RigDentalArcade)) return -5;
    if (fread(arc, sizeof(RigDentalArcade), 1, fp) != 1) return -6;
    return 0;
}