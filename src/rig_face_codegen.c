/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "rig_face_engine_v2.h"
#include "rig_noext_io.h"
#include "rig_noext_str.h"
#include "rig_syscall.h"

#define CG_APPEND(buf, sz, pos, ...)  do {                              \
    if ((pos) < (int)(sz)) {                                            \
        int _n = snprintf((buf)+(pos), (sz)-(pos), __VA_ARGS__);        \
        if (_n > 0) (pos) += _n;                                        \
    }                                                                   \
} while(0)

static void cg_timestamp__rig_variant_a2bee92d(char *buf, size_t sz)
{
    time_t t = time(NULL);
    struct tm *tm_info = gmtime(&t);
    if (tm_info)
        strftime(buf, sz, "%Y-%m-%dT%H:%M:%SZ", tm_info);
    else
        rl_snprintf(buf, sz, "1970-01-01T00:00:00Z");
    return 0;
}
int rig_codegen_params__rig_dup_94dd7414(const RigFaceParams *p,
                       const RigCodegenOptions *opt,
                       char *buf, size_t sz)
{
    if (!p || !buf || sz == 0) return -1;
    int pos = 0;
    const char *name = (opt && opt->object_name[0]) ? opt->object_name : "face";
    bool cmt  = (!opt || opt->include_comments);
    const char *I = (opt && opt->indent_size == 2) ? "  " : "    ";

    if (cmt) {
        char ts[32]; cg_timestamp__rig_variant_a2bee92d(ts, sizeof(ts));
        CG_APPEND(buf, sz, pos,
            "/*\n"
            " * %s_params — RigFaceParams generado por RigCom v24 THE SANTORIUM OF COMPILER CodeGen\n"
            " * Timestamp : %s\n"
            " * φ         : 1.6180339887498948482\n"
            " * Compilar  : rigc build %s --neon --lto\n"
            " */\n\n", name, ts, name);
    }

    CG_APPEND(buf, sz, pos,
        "static const RigFaceParams %s_params = {\n", name);

    if (cmt) CG_APPEND(buf, sz, pos, "%s/* ── Cráneo base ─────────────── */\n", I);
    CG_APPEND(buf, sz, pos,
        "%s.cranium_width       = %.6ff,\n"
        "%s.cranium_height      = %.6ff,\n"
        "%s.cranium_depth       = %.6ff,\n"
        "%s.face_phi_ratio      = %.10ff,  /* φ = %.6f */\n",
        I, p->cranium_width, I, p->cranium_height, I, p->cranium_depth,
        I, p->face_phi_ratio, 1.6180339887498948f);

    if (cmt) CG_APPEND(buf, sz, pos, "%s/* ── Tercios de Vitruvio ──────── */\n", I);
    CG_APPEND(buf, sz, pos,
        "%s.upper_third         = %.6ff,\n"
        "%s.mid_third           = %.6ff,\n"
        "%s.lower_third         = %.6ff,\n",
        I, p->upper_third, I, p->mid_third, I, p->lower_third);

    if (cmt) CG_APPEND(buf, sz, pos, "%s/* ── Órbitas ──────────────────── */\n", I);
    CG_APPEND(buf, sz, pos,
        "%s.interocular_dist    = %.6ff,\n"
        "%s.eye_width           = %.6ff,\n"
        "%s.orbital_depth       = %.6ff,\n"
        "%s.brow_protrusion     = %.6ff,\n",
        I, p->interocular_dist, I, p->eye_width,
        I, p->orbital_depth, I, p->brow_protrusion);

    if (cmt) CG_APPEND(buf, sz, pos, "%s/* ── Nariz (Canon de Burstone) ── */\n", I);
    CG_APPEND(buf, sz, pos,
        "%s.nose_length         = %.6ff,\n"
        "%s.nose_width          = %.6ff,\n"
        "%s.nasal_tip_proj      = %.6ff,\n"
        "%s.nasal_bridge_width  = %.6ff,\n",
        I, p->nose_length, I, p->nose_width,
        I, p->nasal_tip_proj, I, p->nasal_bridge_width);

    if (cmt) CG_APPEND(buf, sz, pos, "%s/* ── Boca · Maxilar · Mentón ──── */\n", I);
    CG_APPEND(buf, sz, pos,
        "%s.mouth_width         = %.6ff,\n"
        "%s.lip_thickness_upper = %.6ff,\n"
        "%s.lip_thickness_lower = %.6ff,\n"
        "%s.jaw_width           = %.6ff,\n"
        "%s.jaw_angle           = %.4ff,\n"
        "%s.chin_projection     = %.6ff,\n"
        "%s.chin_height         = %.6ff,\n",
        I, p->mouth_width, I, p->lip_thickness_upper, I, p->lip_thickness_lower,
        I, p->jaw_width, I, p->jaw_angle, I, p->chin_projection, I, p->chin_height);

    if (cmt) CG_APPEND(buf, sz, pos, "%s/* ── Pómulos · Cuello ─────────── */\n", I);
    CG_APPEND(buf, sz, pos,
        "%s.zygomatic_width     = %.6ff,\n"
        "%s.zygomatic_height    = %.6ff,\n"
        "%s.neck_width          = %.6ff,\n"
        "%s.neck_length         = %.6ff,\n",
        I, p->zygomatic_width, I, p->zygomatic_height,
        I, p->neck_width, I, p->neck_length);

    if (cmt) CG_APPEND(buf, sz, pos,
        "%s/* ── Etnología biofísica (Donner-Jensen SSS) ─── */\n", I);
    CG_APPEND(buf, sz, pos,
        "%s.melanin             = %.6ff,  /* 0=albino → 1=máx oscuro */\n"
        "%s.hemoglobin          = %.6ff,  /* vascularización visible  */\n"
        "%s.carotene            = %.6ff,  /* tono amarillo/ocre       */\n"
        "%s.age_factor          = %.6ff,  /* 0=joven → 1=anciano      */\n"
        "%s.gender_factor       = %.6ff,  /* 0=femenino → 1=masculino */\n"
        "};\n",
        I, p->melanin, I, p->hemoglobin, I, p->carotene,
        I, p->age_factor, I, p->gender_factor);

    if (cmt) {
        CG_APPEND(buf, sz, pos,
            "\n/* Crear malla desde los parámetros anteriores: */\n"
            "/* RigFaceMesh *%s = rig_face_create(&%s_params, 4); */\n"
            "/* RigFaceMeshV2 *%s_v2 = rig_face_v2_create(&%s_params, 4); */\n",
            name, name, name, name);
    }
    return pos;
}

int rig_codegen_rigscript_face__rig_dup_ade1334b(const RigFaceArchetype *arch, char *buf, size_t sz)
{
    if (!arch || !buf || sz == 0) return -1;
    int pos = 0;
    char ts[32]; cg_timestamp__rig_variant_a2bee92d(ts, sizeof(ts));
    const RigFaceParams *p = &arch->params;

    CG_APPEND(buf, sz, pos,
        "// %s — RigScript Face Definition\n"
        "// Generated by RigCom v24 THE SANTORIUM OF COMPILER CodeGen · %s\n"
        "// φ = 1.6180339887498948482 · Solo la vida prevalecerá.\n\n"
        "@module face_%s\n"
        "@version 2.0\n"
        "@target arm64\n\n",
        arch->name, ts, arch->codename);

    CG_APPEND(buf, sz, pos,
        "// ── Constantes del arquetipo ─────────────────────────────────\n"
        "const PHI       : f32 = 1.6180339887f;\n"
        "const SCHUMANN  : f32 = 7.83f;  // Hz\n"
        "const PHI_REF   : f32 = %.10ff;\n\n", arch->phi_reference);

    CG_APPEND(buf, sz, pos,
        "// ── Función de construcción facial ──────────────────────────\n"
        "@export\n"
        "fn build_%s(subdiv: u32 = %u) -> *RigFaceMeshV2 {\n",
        arch->codename, arch->recommended_subdiv);

    CG_APPEND(buf, sz, pos,
        "    let params = RigFaceParams {\n"
        "        cranium_width       : %.6f,\n"
        "        cranium_height      : %.6f,\n"
        "        cranium_depth       : %.6f,\n"
        "        face_phi_ratio      : %.6f,\n"
        "        upper_third         : %.6f,\n"
        "        mid_third           : %.6f,\n"
        "        lower_third         : %.6f,\n"
        "        interocular_dist    : %.6f,\n"
        "        eye_width           : %.6f,\n"
        "        orbital_depth       : %.6f,\n"
        "        brow_protrusion     : %.6f,\n"
        "        nose_length         : %.6f,\n"
        "        nose_width          : %.6f,\n"
        "        nasal_tip_proj      : %.6f,\n"
        "        nasal_bridge_width  : %.6f,\n"
        "        mouth_width         : %.6f,\n"
        "        lip_thickness_upper : %.6f,\n"
        "        lip_thickness_lower : %.6f,\n"
        "        jaw_width           : %.6f,\n"
        "        jaw_angle           : %.4f,\n"
        "        chin_projection     : %.6f,\n"
        "        chin_height         : %.6f,\n"
        "        zygomatic_width     : %.6f,\n"
        "        zygomatic_height    : %.6f,\n"
        "        neck_width          : %.6f,\n"
        "        neck_length         : %.6f,\n"
        "        melanin             : %.6f,\n"
        "        hemoglobin          : %.6f,\n"
        "        carotene            : %.6f,\n"
        "        age_factor          : %.6f,\n"
        "        gender_factor       : %.6f,\n"
        "    };\n\n",
        p->cranium_width, p->cranium_height, p->cranium_depth,
        p->face_phi_ratio,
        p->upper_third, p->mid_third, p->lower_third,
        p->interocular_dist, p->eye_width, p->orbital_depth, p->brow_protrusion,
        p->nose_length, p->nose_width, p->nasal_tip_proj, p->nasal_bridge_width,
        p->mouth_width, p->lip_thickness_upper, p->lip_thickness_lower,
        p->jaw_width, p->jaw_angle, p->chin_projection, p->chin_height,
        p->zygomatic_width, p->zygomatic_height,
        p->neck_width, p->neck_length,
        p->melanin, p->hemoglobin, p->carotene, p->age_factor, p->gender_factor);

    CG_APPEND(buf, sz, pos,
        "    let mesh = rig_face_v2_create(&params, subdiv);\n"
        "    if mesh == null { return null; }\n\n");

    if (arch->build_ears) {
        CG_APPEND(buf, sz, pos,
            "    rig_face_build_ear_geometry(mesh, false);  // oreja izquierda\n"
            "    rig_face_build_ear_geometry(mesh, true);   // oreja derecha\n"
            "    rig_face_attach_ears(mesh);\n");
    }
    if (arch->build_neck) {
        CG_APPEND(buf, sz, pos,
            "    rig_face_build_neck(&mesh.base, &params);\n");
    }
    if (arch->build_skeleton) {
        CG_APPEND(buf, sz, pos,
            "    rig_face_bind_skeleton(&mesh.base);\n");
    }
    if (arch->build_shapes) {
        CG_APPEND(buf, sz, pos,
            "    rig_face_build_facs_shapes(&mesh.base);\n");
    }

    CG_APPEND(buf, sz, pos,
        "\n"
        "    // Micro-detalle v2\n"
        "    rig_face_build_pore_system(mesh, 1.0);\n"
        "    rig_face_build_wrinkle_lines(mesh, params.age_factor, 0.0);\n"
        "    rig_face_build_vascular_tree(mesh);\n"
        "    rig_face_sculpt_lips_v2(mesh);\n\n"
        "    rig_face_compute_smooth_normals(&mesh.base);\n"
        "    rig_face_compute_tangent_basis(&mesh.base);\n\n"
        "    mesh.archetype_id  = %d;\n"
        "    mesh.has_archetype = true;\n"
        "    mesh.version       = 2;\n"
        "    mesh.phi_coherence = %.10f;\n\n"
        "    return mesh;\n"
        "}\n\n",
        (int)arch->id, 1.0f / arch->phi_reference);

    CG_APPEND(buf, sz, pos,
        "// ── Aplicar expresión FACS ────────────────────────────────────\n"
        "@export\n"
        "fn apply_expression_%s(mesh: *RigFaceMeshV2, weights: [f32; 52]) -> void {\n"
        "    rig_face_apply_expression(&mesh.base, weights.ptr);\n"
        "    rig_face_solve_psd(&mesh.base);\n"
        "    rig_face_compute_smooth_normals(&mesh.base);\n"
        "}\n\n",
        arch->codename);

    CG_APPEND(buf, sz, pos,
        "// ── Export VBO para WebGL / Vulkan ───────────────────────────\n"
        "@export\n"
        "fn export_vbo_%s(mesh: *RigFaceMeshV2,\n"
        "                  vbo: *f32, ibo: *u32,\n"
        "                  n_floats: *u32, n_indices: *u32) -> i32 {\n"
        "    return rig_face_v2_export_vbo(mesh, vbo, ibo, n_floats, n_indices);\n"
        "}\n",
        arch->codename);

    return pos;
}

int rig_codegen_archetype__rig_dup_76d1695b(RigArchetypeID id,
                           const RigCodegenOptions *opt,
                           char *buf, size_t sz)
{
    const RigFaceArchetype *arch = rig_archetype_get(id);
    if (!arch || !buf || sz == 0) return -1;

    if (opt && opt->target == RIG_CODEGEN_RIGSCRIPT) {
        return rig_codegen_rigscript_face__rig_dup_ade1334b(arch, buf, sz);
    }

    if (opt && opt->target == RIG_CODEGEN_HEADER) {
        return rig_codegen_all_archetypes_header__rig_dup_2b02366f(buf, sz);
    }

    int pos = 0;
    const RigFaceParams *p = &arch->params;

    CG_APPEND(buf, sz, pos,
        "{\n"
        "  \"cmd\"         : \"rigart_face_build\",\n"
        "  \"archetype\"   : \"%s\",\n"
        "  \"archetype_id\": %d,\n"
        "  \"name\"        : \"%s\",\n"
        "  \"category\"    : \"%s\",\n"
        "  \"phi_ref\"     : %.10f,\n"
        "  \"melanin\"     : %.6f,\n"
        "  \"hemoglobin\"  : %.6f,\n"
        "  \"carotene\"    : %.6f,\n"
        "  \"age\"         : %.6f,\n"
        "  \"gender\"      : %.6f,\n"
        "  \"subdiv\"      : %u,\n"
        "  \"neck\"        : %s,\n"
        "  \"skeleton\"    : %s,\n"
        "  \"shapes\"      : %s,\n"
        "  \"cw\"          : %.6f,\n"
        "  \"ch\"          : %.6f,\n"
        "  \"cd\"          : %.6f,\n"
        "  \"jaw_w\"       : %.6f,\n"
        "  \"jaw_a\"       : %.4f,\n"
        "  \"nose_l\"      : %.6f,\n"
        "  \"nose_w\"      : %.6f,\n"
        "  \"iod\"         : %.6f,\n"
        "  \"zygo_w\"      : %.6f,\n"
        "  \"mouth_w\"     : %.6f,\n"
        "  \"neck_w\"      : %.6f,\n"
        "  \"neck_l\"      : %.6f,\n"
        "  \"chin_p\"      : %.6f,\n"
        "  \"tags\"        : \"%s\"\n"
        "}",
        arch->codename, (int)arch->id,
        arch->name,
        rig_archetype_category_name(arch->category),
        (double)arch->phi_reference,
        (double)p->melanin, (double)p->hemoglobin, (double)p->carotene,
        (double)p->age_factor, (double)p->gender_factor,
        arch->recommended_subdiv,
        arch->build_neck     ? "true" : "false",
        arch->build_skeleton ? "true" : "false",
        arch->build_shapes   ? "true" : "false",
        (double)p->cranium_width, (double)p->cranium_height, (double)p->cranium_depth,
        (double)p->jaw_width, (double)p->jaw_angle,
        (double)p->nose_length, (double)p->nose_width,
        (double)p->interocular_dist,
        (double)p->zygomatic_width, (double)p->mouth_width,
        (double)p->neck_width, (double)p->neck_length,
        (double)p->chin_projection,
        arch->tags);

    return pos;
}

int rig_codegen_glsl_skin_shader__rig_dup_1c81da77(const RigFaceMeshV2 *mesh, char *buf, size_t sz)
{
    if (!mesh || !buf || sz == 0) return -1;
    int pos = 0;
    const RigSkinMaterial *mat = &mesh->base.material;
    char ts[32]; cg_timestamp__rig_variant_a2bee92d(ts, sizeof(ts));

    CG_APPEND(buf, sz, pos,
        "// ── RigCom v24 THE SANTORIUM OF COMPILER — PBR Skin Shader ────────────────────────────\n"
        "// Generated: %s\n"
        "// φ = 1.6180339887498948482\n"
        "// Modelo: Donner-Jensen (2005) + Chiang SSS (2016)\n\n"
        "#version 300 es\n"
        "precision highp float;\n\n"
        "// ── PBR Uniforms ─────────────────────────────────────────────\n"
        "uniform vec3  u_albedo;          // (%.4f, %.4f, %.4f)\n"
        "uniform float u_roughness;       // %.4f\n"
        "uniform float u_metallic;        // %.4f (piel ≈ 0)\n"
        "uniform float u_specular;        // %.4f (F0 fresnel)\n\n",
        ts,
        mat->base_color[0], mat->base_color[1], mat->base_color[2],
        mat->roughness, mat->metallic, mat->specular);

    CG_APPEND(buf, sz, pos,
        "// ── Subsurface Scattering (SSS) Chiang 2016 ─────────────────\n"
        "uniform float u_sss_weight;      // %.4f\n"
        "uniform float u_sss_scale;       // %.4f\n"
        "uniform vec3  u_sss_radius;      // (%.4f, %.4f, %.4f) mm RGB\n\n"
        "// Concentraciones biofísicas (Donner-Jensen)\n"
        "uniform float u_melanin;         // %.4f  eumelanina\n"
        "uniform float u_hemoglobin;      // %.4f  oxihemoglobina\n"
        "uniform float u_carotene;        // %.4f  caroteno\n\n",
        mat->sss_weight, mat->sss_scale,
        mat->sss_radius[0], mat->sss_radius[1], mat->sss_radius[2],
        mat->melanin_concentration, mat->hemoglobin_concentration,
        mat->carotene_concentration);

    CG_APPEND(buf, sz, pos,
        "// ── Micro-detalle ────────────────────────────────────────────\n"
        "uniform float u_pore_scale;      // %.4f\n"
        "uniform float u_pore_depth;      // %.4f\n"
        "uniform float u_wrinkle_depth;   // %.4f\n"
        "uniform float u_oiliness;        // %.4f\n"
        "uniform float u_fuzz_amount;     // %.4f\n\n"
        "// ── Samplers ─────────────────────────────────────────────────\n"
        "uniform sampler2D u_albedo_map;\n"
        "uniform sampler2D u_normal_map;\n"
        "uniform sampler2D u_roughness_map;\n"
        "uniform sampler2D u_sss_map;       // SSS thickness map\n"
        "uniform sampler2D u_pore_normal;   // Normal map de poros\n"
        "uniform sampler2D u_wrinkle_normal;// Normal map de arrugas\n"
        "uniform sampler2D u_vascular_map;  // Vascularización\n\n",
        mat->pore_scale, mat->pore_depth, mat->wrinkle_depth,
        mat->oiliness, mat->fuzz_amount);

    CG_APPEND(buf, sz, pos,
        "// ── Inputs ───────────────────────────────────────────────────\n"
        "in vec3 v_pos;\n"
        "in vec3 v_normal;\n"
        "in vec3 v_tangent;\n"
        "in vec3 v_bitangent;\n"
        "in vec2 v_uv;\n"
        "in vec4 v_color;    // vertex color: melanina/SSS por región\n\n"
        "out vec4 fragColor;\n\n"
        "// ── Constantes φ ─────────────────────────────────────────────\n"
        "const float PHI       = 1.6180339887;\n"
        "const float PHI_INV   = 0.6180339887;\n"
        "const float SCHUMANN  = 7.83;  // Hz Resonancia Schumann\n"
        "const float PI        = 3.14159265359;\n\n");

    CG_APPEND(buf, sz, pos,
        "// ── Fresnel Schlick ──────────────────────────────────────────\n"
        "vec3 fresnelSchlick(float cosTheta, vec3 F0) {\n"
        "    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);\n"
        "}\n\n"
        "// ── GGX Normal Distribution ───────────────────────────────────\n"
        "float ggxNDF(vec3 N, vec3 H, float roughness) {\n"
        "    float a  = roughness * roughness;\n"
        "    float a2 = a * a;\n"
        "    float NdotH  = max(dot(N, H), 0.0);\n"
        "    float NdotH2 = NdotH * NdotH;\n"
        "    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);\n"
        "    return a2 / (PI * denom * denom);\n"
        "}\n\n"
        "// ── Schlick-GGX Geometry ──────────────────────────────────────\n"
        "float schlickGGX(float NdotV, float roughness) {\n"
        "    float r = (roughness + 1.0);\n"
        "    float k = (r * r) / 8.0;\n"
        "    return NdotV / (NdotV * (1.0 - k) + k);\n"
        "}\n\n");

    CG_APPEND(buf, sz, pos,
        "// ── SSS aproximación (Jimenez 2010 + modificación RigCom) ────\n"
        "vec3 sss_approx(vec3 albedo, float thickness, vec3 sss_radius) {\n"
        "    // Perfil de difusión por capas (epidermis + dermis + subcut.)\n"
        "    vec3 scatter = albedo * exp(-thickness * (1.0 / sss_radius));\n"
        "    // Componente de retro-dispersión (backward scatter)\n"
        "    float back   = exp(-thickness * PHI_INV);\n"
        "    return mix(scatter, albedo * back * vec3(1.0, 0.72, 0.46), 0.3);\n"
        "}\n\n"
        "// ── Main ──────────────────────────────────────────────────────\n"
        "void main() {\n"
        "    vec2 uv = v_uv;\n\n"
        "    // 1. Normal combinada (mapa + poros + arrugas)\n"
        "    vec3 nrm_map  = texture(u_normal_map,    uv).rgb * 2.0 - 1.0;\n"
        "    vec3 pore_nrm = texture(u_pore_normal,   uv * 8.0).rgb * 2.0 - 1.0;\n"
        "    vec3 wrkl_nrm = texture(u_wrinkle_normal,uv).rgb * 2.0 - 1.0;\n"
        "    vec3 N_ts = normalize(nrm_map\n"
        "                + pore_nrm  * u_pore_depth\n"
        "                + wrkl_nrm  * u_wrinkle_depth);\n"
        "    mat3 TBN = mat3(normalize(v_tangent),\n"
        "                    normalize(v_bitangent),\n"
        "                    normalize(v_normal));\n"
        "    vec3 N = normalize(TBN * N_ts);\n\n"
        "    // 2. Albedo con corrección de melanina\n"
        "    vec4 albedo_smp = texture(u_albedo_map, uv);\n"
        "    vec3 albedo = u_albedo * albedo_smp.rgb;\n"
        "    // Modular con vertex color (bake regional melanina/SSS)\n"
        "    albedo *= v_color.rgb;\n\n"
        "    // 3. Roughness con mapa de poros anisótropo\n"
        "    float rgh = u_roughness * texture(u_roughness_map, uv).r;\n"
        "    // Oiliness → specular anisótropo\n"
        "    rgh = mix(rgh, rgh * PHI_INV, u_oiliness);\n\n"
        "    // 4. SSS via mapa de espesor\n"
        "    float thickness   = texture(u_sss_map, uv).r;\n"
        "    vec3  vascular    = texture(u_vascular_map, uv).rgb;\n"
        "    vec3  sss_color   = sss_approx(albedo, thickness, u_sss_radius);\n"
        "    // Añadir hemoglobina de la red vascular\n"
        "    sss_color += vascular * u_hemoglobin * vec3(0.8, 0.1, 0.1) * 0.15;\n\n"
        "    // 5. BRDF PBR Cook-Torrance\n"
        "    // (luz ambiente simplificada — reemplazar con IBL en producción)\n"
        "    vec3 L = normalize(vec3(0.5, 1.0, 0.8));\n"
        "    vec3 V = normalize(-v_pos);\n"
        "    vec3 H = normalize(V + L);\n"
        "    float NdotL = max(dot(N, L), 0.0);\n"
        "    float NdotV = max(dot(N, V), 0.0);\n"
        "    vec3 F0 = mix(vec3(u_specular), albedo, u_metallic);\n"
        "    vec3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);\n"
        "    float D = ggxNDF(N, H, rgh);\n"
        "    float G = schlickGGX(NdotV, rgh) * schlickGGX(NdotL, rgh);\n"
        "    vec3  spec  = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);\n"
        "    vec3  kD    = (1.0 - F) * (1.0 - u_metallic);\n"
        "    vec3  diff  = kD * albedo / PI;\n\n"
        "    // 6. Composición final con SSS\n"
        "    vec3 direct = (diff + spec) * NdotL * vec3(1.0, 0.98, 0.95);\n"
        "    vec3 ambient = vec3(0.03) * albedo + sss_color * u_sss_weight * 0.5;\n"
        "    vec3 color   = direct + ambient;\n\n"
        "    // 7. Tone mapping (ACES) + gamma\n"
        "    color = color * (color + 0.0245786) /\n"
        "            (color * (0.983729 * color + 0.4329510) + 0.238081);\n"
        "    color = pow(color, vec3(1.0 / 2.2));\n\n"
        "    fragColor = vec4(color, 1.0);\n"
        "}\n");

    return pos;
}

int rig_codegen_all_archetypes_header__rig_dup_2b02366f(char *buf, size_t sz)
{
    if (!buf || sz == 0) return -1;
    int pos = 0;
    char ts[32]; cg_timestamp__rig_variant_a2bee92d(ts, sizeof(ts));

    CG_APPEND(buf, sz, pos,
        "/*\n"
        " * rig_face_archetypes_gen.h — Auto-generado por RigCom v24 THE SANTORIUM OF COMPILER CodeGen\n"
        " * Timestamp: %s\n"
        " * 36 arquetipos de rostro — ARM64 / Termux\n"
        " * φ = 1.6180339887498948482 · Solo la vida prevalecerá.\n"
        " */\n\n"
        "#ifndef RIG_FACE_ARCHETYPES_GEN_H\n"
        "#define RIG_FACE_ARCHETYPES_GEN_H\n\n"
        "#include \"rig_face_engine_v2.h\"\n\n"
        "/* ── IDs de arquetipo enumerados ────────────────────────────── */\n",
        ts);

    for (int i = 0; i < RIG_ARCH_COUNT; i++) {
        const RigFaceArchetype *a = rig_archetype_get((RigArchetypeID)i);
        if (!a) continue;
        char upper[64];
        int ui = 0;
        for (const char *c = a->codename; *c && ui < 63; c++, ui++) {
            upper[ui] = (*c >= 'a' && *c <= 'z') ? (*c - 32) : *c;
        }
        upper[ui] = '\0';
        CG_APPEND(buf, sz, pos,
            "#define RIG_ARCH_%s_ID  %d  /* %s */\n",
            upper, i, a->name);
    }

    CG_APPEND(buf, sz, pos,
        "\n/* ── Lookup por nombre ─────────────────────────────────────── */\n"
        "/* rig_archetype_get(RIG_ARCH_NORDIC_ID) → const RigFaceArchetype* */\n\n"
        "/* ── Tabla de φ-referencias ─────────────────────────────────── */\n"
        "static const float rig_arch_phi_table[%d] = {\n", RIG_ARCH_COUNT);

    for (int i = 0; i < RIG_ARCH_COUNT; i++) {
        const RigFaceArchetype *a = rig_archetype_get((RigArchetypeID)i);
        CG_APPEND(buf, sz, pos, "    %.10ff%s  /* [%02d] %s */\n",
            a ? a->phi_reference : 0.0f,
            (i < RIG_ARCH_COUNT - 1) ? "," : " ",
            i, a ? a->name : "?");
    }
    CG_APPEND(buf, sz, pos, "};\n\n#endif /* RIG_FACE_ARCHETYPES_GEN_H */\n");
    return pos;
}

int rig_codegen_makefile__rig_dup_d9e2e66a(const char *project_name, char *buf, size_t sz)
{
    if (!project_name || !buf || sz == 0) return -1;
    int pos = 0;
    char ts[32]; cg_timestamp__rig_variant_a2bee92d(ts, sizeof(ts));

    CG_APPEND(buf, sz, pos,
        "# ══════════════════════════════════════════════════════════════\n"
        "# %s — Makefile ARM64 standalone\n"
        "# Generado por RigCom v24 THE SANTORIUM OF COMPILER CodeGen · %s\n"
        "# Target: Honor 400 / Kirin 9000S · Termux · ARM64\n"
        "# φ = 1.6180339887498948482 · Solo la vida prevalecerá.\n"
        "# ══════════════════════════════════════════════════════════════\n\n"
        "CC_TERMUX = /data/data/com.termux/files/usr/bin/gcc\n"
        "ifneq ($(wildcard $(CC_TERMUX)),)\n"
        "    CC = $(CC_TERMUX)\n"
        "else\n"
        "    CC = gcc\n"
        "endif\n\n"
        "CFLAGS = -std=c11 -O3 -Wall -fPIC \\\n"
        "         -march=armv8.2-a+simd -mtune=cortex-a76 \\\n"
        "         -ffast-math -funroll-loops \\\n"
        "         -D_POSIX_C_SOURCE=200809L \\\n"
        "         -DRIG_V2 -DRIG_FACE_STANDALONE\n\n"
        "LDFLAGS = -lm -lpthread\n\n"
        "SRC = src/rig_face_engine.c \\\n"
        "      src/rig_face_microdetail.c \\\n"
        "      src/rig_face_archetypes.c \\\n"
        "      src/rig_face_codegen.c\n\n"
        "INC = -Iinclude\n\n"
        "TARGET = build/%s\n\n"
        "all: $(TARGET)\n\n"
        "$(TARGET): $(SRC)\n"
        "\t@mkdir -p build\n"
        "\t$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(LDFLAGS)\n"
        "\t@echo \"✓ $(TARGET) · ARM64 φ-build OK\"\n\n"
        "# Subdivisión 6 — ultra detalle (40962 vértices)\n"
        "ultra: $(SRC)\n"
        "\t@mkdir -p build\n"
        "\t$(CC) $(CFLAGS) $(INC) -DRIG_SUBDIV_DEFAULT=6 \\\n"
        "\t      -o build/%s_ultra $(SRC) $(LDFLAGS)\n\n"
        "# Standalone con SafeStack + CFI\n"
        "secure: $(SRC)\n"
        "\t$(CC) $(CFLAGS) $(INC) \\\n"
        "\t      -fsanitize=safe-stack -fsanitize=cfi \\\n"
        "\t      -o build/%s_secure $(SRC) $(LDFLAGS)\n\n"
        "clean:\n"
        "\trm -rf build/\n\n"
        "list:\n"
        "\t./build/%s --list-archetypes\n\n"
        ".PHONY: all ultra secure clean list\n",
        project_name, ts, project_name,
        project_name, project_name, project_name);

    return pos;
}

int rig_codegen_mesh_v2__rig_dup_9a0ca9d5(const RigFaceMeshV2 *mesh,
                         const RigCodegenOptions *opt,
                         char *buf, size_t sz)
{
    if (!mesh || !buf || sz == 0) return -1;

    if (opt && opt->target == RIG_CODEGEN_MARKDOWN) {
        int pos = 0;
        char ts[32]; cg_timestamp__rig_variant_a2bee92d(ts, sizeof(ts));
        const RigFaceMesh *m = &mesh->base;

        CG_APPEND(buf, sz, pos,
            "# RigCom v24 THE SANTORIUM OF COMPILER — Face Mesh — Documentación Técnica\n\n"
            "**Generado:** %s  \n"
            "**φ:** `1.6180339887498948482`  \n"
            "**Versión motor:** 2.0  \n\n"
            "## Geometría\n\n"
            "| Parámetro | Valor |\n"
            "|-----------|-------|\n"
            "| Vértices  | %u |\n"
            "| Triángulos| %u |\n"
            "| Subdivisión | %u |\n"
            "| φ-error   | %.6f%% |\n"
            "| AABB min  | (%.3f, %.3f, %.3f) |\n"
            "| AABB max  | (%.3f, %.3f, %.3f) |\n\n",
            ts, m->n_verts, m->n_tris, m->subdivision_level,
            m->phi_error * 100.0f,
            m->aabb_min.x, m->aabb_min.y, m->aabb_min.z,
            m->aabb_max.x, m->aabb_max.y, m->aabb_max.z);

        CG_APPEND(buf, sz, pos,
            "## Material PBR — Piel (Donner-Jensen SSS)\n\n"
            "| Propiedad | Valor |\n"
            "|-----------|-------|\n"
            "| Color base RGB | (%.3f, %.3f, %.3f) |\n"
            "| Roughness | %.4f |\n"
            "| SSS radius | (R=%.2fmm G=%.2fmm B=%.2fmm) |\n"
            "| SSS weight | %.4f |\n"
            "| Melanina | %.4f |\n"
            "| Hemoglobina | %.4f |\n"
            "| Poros scale | %.4f |\n\n",
            m->material.base_color[0], m->material.base_color[1], m->material.base_color[2],
            m->material.roughness,
            m->material.sss_radius[0], m->material.sss_radius[1], m->material.sss_radius[2],
            m->material.sss_weight,
            m->material.melanin_concentration,
            m->material.hemoglobin_concentration,
            m->material.pore_scale);

        CG_APPEND(buf, sz, pos,
            "## Micro-detalle v2.0\n\n"
            "| Módulo | Estado | Elementos |\n"
            "|--------|--------|----------|\n"
            "| Poros | ✓ | %u |\n"
            "| Arrugas | ✓ | %u |\n"
            "| Vasos | ✓ | %u |\n"
            "| Folículos | ✓ | %u |\n\n"
            "## GPU\n\n"
            "| Recurso | Tamaño |\n"
            "|---------|--------|\n"
            "| VBO | %.1f KB |\n"
            "| IBO | %.1f KB |\n"
            "| Total GPU | %.1f KB |\n\n"
            "---\n*RigCom v24 THE SANTORIUM OF COMPILER — Face Engine · φ = 1.618 · Solo la vida prevalecerá.*\n",
            mesh->microdetail.n_pores,
            mesh->microdetail.n_wrinkles,
            mesh->microdetail.n_vessels,
            mesh->microdetail.n_follicles,
            (float)(m->n_verts * 15 * 4) / 1024.0f,
            (float)(m->n_tris * 3 * 4) / 1024.0f,
            (float)((m->n_verts * 15 + m->n_tris * 3) * 4) / 1024.0f);

        return pos;
    }

    return rig_codegen_params__rig_dup_94dd7414(&mesh->base.params, opt, buf, sz);
}
