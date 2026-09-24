#include "rigdeps/rig_std_base.h"
#include "rig_v17_preamble.h"
#include "rig_face_v2_bridge.h"
#include "rigdeps/stdio.h"
#include "rigdeps/stdlib.h"
#include "rigdeps/string.h"
#include "rigdeps/math.h"
#include "../include/riglib_math.h"
#include "rigdeps/time.h"

#define BRIDGE_PHI        1.6180339887498948482f
#define BRIDGE_PHI_INV    0.6180339887498948482f
#define BRIDGE_SCHUMANN   7.83f
#define BRIDGE_HTML_CAP   (128 * 1024)

static float  b4_f(const char *j, const char *k, float  d)
{
    if (!j) return d;
    const char *p = strstr(j, k);
    if (!p) return d;
    p = strchr(p, ':');
    return p ? (float)atof(p+1) : d;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: b4_f -> rigpub_rig_face_v2_bridge_b4_f */
float (*rigpub_rig_face_v2_bridge_b4_f)(const char *j, const char *k, float d) = b4_f;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */

static int    b4_i(const char *j, const char *k, int    d)
{
    if (!j) return d;
    const char *p = strstr(j, k);
    if (!p) return d;
    p = strchr(p, ':');
    return p ? atoi(p+1) : d;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: b4_i -> rigpub_rig_face_v2_bridge_b4_i */
int (*rigpub_rig_face_v2_bridge_b4_i)(const char *j, const char *k, int d) = b4_i;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */

static int    b4_b(const char *j, const char *k, int    d)
{
    if (!j) return d;
    const char *p = strstr(j, k);
    if (!p) return d;
    p = strchr(p, ':');
    if (!p) return d;
    while (*p==':' || *p==' ') p++;
    if (*p=='t'||*p=='1') return 1;
    if (*p=='f'||*p=='0') return 0;
    return d;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: b4_b -> rigpub_rig_face_v2_bridge_b4_b */
int (*rigpub_rig_face_v2_bridge_b4_b)(const char *j, const char *k, int d) = b4_b;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */

static size_t   b4_str(const char *j, const char *k, char *out, size_t sz)
{
    if (!j || !out || !sz) return 0;
    const char *p = strstr(j, k);
    if (!p) return 0;
    p = strchr(p, '"');
    if (!p) return 0;
    p = strchr(p+1, '"');
    if (!p) return 0;
    p = strchr(p+1, '"');
    if (!p) return 0;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < sz-1) out[i++] = *p++;
    out[i] = '\0';
    return i;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: b4_str -> rigpub_rig_face_v2_bridge_b4_str */
size_t (*rigpub_rig_face_v2_bridge_b4_str)(const char *j, const char *k, char *out, size_t sz) = b4_str;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


int rigart_face_session_v2_init(RIgArtFaceSessionV2 *s)
{
    if (!s) return -1;
    memset(s, 0, sizeof(*s));
    const RigFaceArchetype *_def = rig_archetype_get(RIG_ARCH_AGE_YOUNG_25);
    s->params = _def ? _def->params : (RigFaceParams){0};
    s->subdiv_level  = 4;
    s->archetype_id  = -1;
    rigart_v4_init_result(&s->last_result);
    return 0;
}

int rigart_face_session_v2_destroy(RIgArtFaceSessionV2 *s)
{
    if (!s) return -1;
    if (s->mesh) { rig_face_v2_destroy(s->mesh); s->mesh = NULL; }
    rigart_v4_free_result(&s->last_result);
    return 0;
}

int rigart_face_v2_dispatch(WsServer *srv, const char *cmd,
                              const char *payload,
                              RIgArtFaceSessionV2 *session)
{
    if (!cmd || !session) return -1;
    char resp[4096];

    if (strcmp(cmd, "rigart_face_build") == 0)
    {
        session->params.melanin          = b4_f(payload, "\"melanin\"",    0.35f);
        session->params.hemoglobin       = b4_f(payload, "\"hemoglobin\"", 0.40f);
        session->params.age_factor       = b4_f(payload, "\"age\"",        0.25f);
        session->params.gender_factor    = b4_f(payload, "\"gender\"",     0.50f);
        session->params.cranium_width    = b4_f(payload, "\"cw\"",  session->params.cranium_width);
        session->params.cranium_height   = b4_f(payload, "\"ch\"",  session->params.cranium_height);
        session->params.cranium_depth    = b4_f(payload, "\"cd\"",  session->params.cranium_depth);
        session->params.jaw_width        = b4_f(payload, "\"jaw_w\"",session->params.jaw_width);
        session->params.jaw_angle        = b4_f(payload, "\"jaw_a\"",session->params.jaw_angle);
        session->params.nose_length      = b4_f(payload, "\"nose_l\"",session->params.nose_length);
        session->params.nose_width       = b4_f(payload, "\"nose_w\"",session->params.nose_width);
        session->params.interocular_dist = b4_f(payload, "\"iod\"",  session->params.interocular_dist);
        session->params.zygomatic_width  = b4_f(payload, "\"zygo_w\"",session->params.zygomatic_width);
        session->params.mouth_width      = b4_f(payload, "\"mouth_w\"",session->params.mouth_width);
        session->params.neck_width       = b4_f(payload, "\"neck_w\"",session->params.neck_width);
        session->params.neck_length      = b4_f(payload, "\"neck_l\"",session->params.neck_length);
        session->params.chin_projection  = b4_f(payload, "\"chin_p\"",session->params.chin_projection);

        uint32_t subdiv = (uint32_t)b4_i(payload, "\"subdiv\"", 4);
        if (subdiv < 2) subdiv = 2;
        if (subdiv > 6) subdiv = 6;
        session->subdiv_level = subdiv;

        bool build_pores    = (bool)b4_b(payload, "\"pores\"",    1);
        bool build_vascular = (bool)b4_b(payload, "\"vascular\"", 1);
        bool build_wrinkles = (bool)b4_b(payload, "\"wrinkles\"", 1);
        float age_for_wrinkles = b4_f(payload, "\"age\"", 0.25f);

        if (session->mesh) { rig_face_v2_destroy(session->mesh); session->mesh = NULL; }
        session->mesh = rig_face_v2_create(&session->params, subdiv);
        session->has_pores = session->has_vascular = session->has_wrinkles = false;
        session->archetype_id = -1;

        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_build\","
                "\"error\":\"rig_face_v2_create falló — reducir subdiv\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }

        if (build_pores) {
            rig_face_build_pore_system(session->mesh, 1.0f);
            session->has_pores = true;
        }
        if (build_vascular) {
            rig_face_build_vascular_tree(session->mesh);
            session->has_vascular = true;
        }
        if (build_wrinkles) {
            rig_face_build_wrinkle_lines(session->mesh, age_for_wrinkles, 0.0f);
            session->has_wrinkles = true;
        }

        snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_build\","
            "\"verts\":%u,\"tris\":%u,\"subdiv\":%u,"
            "\"pores\":%s,\"vascular\":%s,\"wrinkles\":%s,"
            "\"phi_error\":%.6f,\"certeza\":%.6f}",
            session->mesh->base.n_verts, session->mesh->base.n_tris,
            session->mesh->base.subdivision_level,
            session->has_pores    ? "true" : "false",
            session->has_vascular ? "true" : "false",
            session->has_wrinkles ? "true" : "false",
            session->mesh->base.phi_error,
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }

    if (strcmp(cmd, "rigart_face_archetype") == 0)
    {
        int arch_id = b4_i(payload, "\"id\"", 0);
        if (arch_id < 0 || arch_id >= 36) arch_id = 0;

        if (session->mesh) { rig_face_v2_destroy(session->mesh); session->mesh = NULL; }
        session->mesh = rig_face_v2_from_archetype((RigArchetypeID)arch_id);
        session->archetype_id = arch_id;

        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_archetype\","
                "\"error\":\"arquetipo %d no encontrado\"}", arch_id);
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }

        rig_face_build_pore_system(session->mesh, 1.0f);
        rig_face_build_vascular_tree(session->mesh);
        session->has_pores = session->has_vascular = true;

        const RigFaceArchetype *arch = rig_archetype_get((RigArchetypeID)arch_id);
        snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_archetype\","
            "\"id\":%d,\"name\":\"%s\","
            "\"verts\":%u,\"tris\":%u,\"subdiv\":%u,"
            "\"phi_ref\":%.6f,\"certeza\":%.6f}",
            arch_id,
            arch ? arch->name : "unknown",
            session->mesh->base.n_verts, session->mesh->base.n_tris,
            session->mesh->base.subdivision_level,
            arch ? arch->phi_reference : BRIDGE_PHI,
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }

    if (strcmp(cmd, "rigart_face_age") == 0)
    {
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_age\","
                "\"error\":\"no mesh — build o archetype primero\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        float age = b4_f(payload, "\"age\"", 25.0f);
        if (age < 0.0f)   age = 0.0f;
        if (age > 100.0f) age = 100.0f;

        rig_age_apply_to_mesh(session->mesh, age);

        rig_face_build_wrinkle_lines(session->mesh, age / 100.0f, 0.0f);
        session->has_wrinkles = true;

        snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_age\","
            "\"age\":%.1f,\"verts\":%u,\"certeza\":%.6f}",
            age, session->mesh->base.n_verts, (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }

    if (strcmp(cmd, "rigart_face_expression") == 0)
    {
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_expression\","
                "\"error\":\"no mesh — build primero\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }

        RigFaceExprCtx ectx;
        memset(&ectx, 0, sizeof(ectx));

        const char *arr = payload ? strstr(payload, "\"weights\"") : NULL;
        if (arr) {
            arr = strchr(arr, '[');
            if (arr) {
                arr++;
                for (int i = 0; i < 52 && i < (int)(sizeof(ectx.weights)/
                                                      sizeof(ectx.weights[0])); i++) {
                    while (*arr==' '||*arr==',') arr++;
                    if (*arr==']'||*arr=='\0') break;
                    ectx.weights[i] = (float)atof(arr);
                    while (*arr && *arr!=',' && *arr!=']') arr++;
                }
            }
        }

        rigart_v4_free_result(&session->last_result);
        int rc = rig_face_v2_expression_blend(&ectx, &session->last_result);

        snprintf(resp, sizeof(resp),
            "{\"ok\":%s,\"cmd\":\"rigart_face_expression\","
            "\"verts\":%u,\"has_glsl\":%s,\"certeza\":%.6f}",
            rc == 0 ? "true" : "false",
            session->mesh->base.n_verts,
            (session->last_result.glsl_vert ? "true" : "false"),
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }

    if (strcmp(cmd, "rigart_face_vbo") == 0)
    {
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_vbo\","
                "\"error\":\"no mesh — build primero\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        uint32_t n_floats = 0, n_indices = 0;

        rig_face_v2_export_vbo(session->mesh, NULL, NULL, &n_floats, &n_indices);

        snprintf(resp, sizeof(resp),
            "{\"ok\":true,\"cmd\":\"rigart_face_vbo\","
            "\"verts\":%u,\"tris\":%u,"
            "\"vbo_floats\":%u,\"vbo_bytes\":%u,"
            "\"ibo_indices\":%u,\"ibo_bytes\":%u,"
            "\"stride\":15,\"layout\":\"pos3_nrm3_tan3_uv2_col4\","
            "\"micro_detail\":%s,\"certeza\":%.6f}",
            session->mesh->base.n_verts, session->mesh->base.n_tris,
            n_floats,  n_floats  * 4,
            n_indices, n_indices * 4,
            (session->has_pores || session->has_vascular) ? "true" : "false",
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        return 0;
    }

    if (strcmp(cmd, "rigart_face_glsl") == 0)
    {
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_glsl\","
                "\"error\":\"no mesh — build primero\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }
        char glsl_buf[8192];
        int rc = rig_codegen_glsl_skin_shader(session->mesh, glsl_buf, sizeof(glsl_buf));

        snprintf(resp, sizeof(resp),
            "{\"ok\":%s,\"cmd\":\"rigart_face_glsl\","
            "\"melanin\":%.4f,\"roughness\":%.4f,"
            "\"sss_r\":%.4f,\"sss_g\":%.4f,\"sss_b\":%.4f,"
            "\"has_pores\":%s,\"has_vascular\":%s,"
            "\"certeza\":%.6f}",
            rc >= 0 ? "true" : "false",
            session->mesh->base.material.melanin_concentration,
            session->mesh->base.material.roughness,
            session->mesh->base.material.sss_radius[0],
            session->mesh->base.material.sss_radius[1],
            session->mesh->base.material.sss_radius[2],
            session->has_pores    ? "true" : "false",
            session->has_vascular ? "true" : "false",
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        if (rc >= 0) ws_broadcastf(srv, "%s", glsl_buf);
        return 0;
    }

    if (strcmp(cmd, "rigart_face_codegen") == 0)
    {
        if (!session->mesh) {
            snprintf(resp, sizeof(resp),
                "{\"ok\":false,\"cmd\":\"rigart_face_codegen\","
                "\"error\":\"no mesh — build o archetype primero\"}");
            ws_broadcastf(srv, "%s", resp);
            return 0;
        }

        char target_str[32] = "c11";
        b4_str(payload, "\"target\"", target_str, sizeof(target_str));

        RigCodegenOptions opt;
        memset(&opt, 0, sizeof(opt));
        opt.include_comments  = true;
        opt.arm64_intrinsics  = true;
        opt.include_mesh_data = true;

        if      (strcmp(target_str, "rigscript") == 0) opt.target = RIG_CODEGEN_RIGSCRIPT;
        else if (strcmp(target_str, "json")       == 0) opt.target = RIG_CODEGEN_JSON;
        else if (strcmp(target_str, "glsl")       == 0) opt.target = RIG_CODEGEN_GLSL;
        else if (strcmp(target_str, "makefile")   == 0) opt.target = RIG_CODEGEN_MAKEFILE;
        else if (strcmp(target_str, "header")     == 0) opt.target = RIG_CODEGEN_HEADER;
        else if (strcmp(target_str, "markdown")   == 0) opt.target = RIG_CODEGEN_MARKDOWN;
        else                                            opt.target = RIG_CODEGEN_C11;

        char *buf = malloc(32768);
        if (!buf) {
            ws_broadcastf(srv,
                "{\"ok\":false,\"cmd\":\"rigart_face_codegen\","
                "\"error\":\"OOM\"}");
            return 0;
        }
        int rc = rig_codegen_mesh_v2(session->mesh, &opt, buf, 32768);

        snprintf(resp, sizeof(resp),
            "{\"ok\":%s,\"cmd\":\"rigart_face_codegen\","
            "\"target\":\"%s\",\"bytes\":%d,\"certeza\":%.6f}",
            rc >= 0 ? "true" : "false",
            target_str, rc,
            (double)BRIDGE_PHI_INV);
        ws_broadcastf(srv, "%s", resp);
        if (rc >= 0) ws_broadcastf(srv, "%s", buf);
        free(buf);
        return 0;
    }

    snprintf(resp, sizeof(resp),
        "{\"ok\":false,\"cmd\":\"%s\",\"error\":\"comando face desconocido\"}",
        cmd);
    ws_broadcastf(srv, "%s", resp);
    return 0;
}

int rigart_renderer_html_v4(const RIgArtRendererCtxV4 *ctx, RigArtResultV4 *out)
{
    if (!ctx || !out) return -1;
    rigart_v4_init_result(out);

    RigArtCompositorCtx comp;
    memset(&comp, 0, sizeof(comp));
    comp.exposure         = ctx->exposure > 0.0f ? ctx->exposure : 1.0f;
    comp.gamma            = 2.2f;
    comp.contrast         = 1.0f;
    comp.saturation       = 1.0f;
    comp.enable_bloom     = ctx->enable_bloom;
    comp.bloom_strength   = ctx->bloom_strength > 0.0f ? ctx->bloom_strength : 0.618f;
    comp.enable_dof       = ctx->enable_dof;
    comp.tonemapping_aces = true;
    comp.enable_vignette  = ctx->show_stats;

    RigArtResultV4 comp_res;
    rigart_v4_init_result(&comp_res);
    rigart_art_compositor(&comp, &comp_res);

    RigArtCanvasCtx canvas;
    memset(&canvas, 0, sizeof(canvas));
    canvas.width           = 1280;
    canvas.height          = 720;
    canvas.layer_count     = 1;
    canvas.hdr_p3_enabled  = ctx->hdr_p3;
    canvas.enable_16bit    = false;
    canvas.dpi             = 96.0f;
    canvas.grid_size_phi   = BRIDGE_PHI;
    snprintf(canvas.layers[0].name, 64, "RigArt v4 — %.48s",
             ctx->title[0] ? ctx->title : "Escena Soberana");
    canvas.layers[0].blend_mode = BLEND_NORMAL;
    canvas.layers[0].opacity    = 1.0f;
    canvas.layers[0].visible    = true;

    RigArtResultV4 canvas_res;
    rigart_v4_init_result(&canvas_res);
    rigart_art_canvas_gen(&canvas, &canvas_res);

    char *html = malloc(BRIDGE_HTML_CAP);
    if (!html) {
        rigart_v4_free_result(&comp_res);
        rigart_v4_free_result(&canvas_res);
        snprintf(out->error, 255, "OOM renderer html v4");
        return -1;
    }

    const char *title  = ctx->title[0]    ? ctx->title    : "RIgArt · Escena Soberana";
    const char *bg     = ctx->bg_color[0] ? ctx->bg_color : "#030208";
    float lx = ctx->light_dir[0], ly = ctx->light_dir[1], lz = ctx->light_dir[2];
    if (lx==0.0f && ly==0.0f && lz==0.0f) { lx=4.0f; ly=8.0f; lz=5.0f; }

    int pos = snprintf(html, BRIDGE_HTML_CAP,
"<!DOCTYPE html>\n<html lang=\"es\">\n<head>\n"
"<meta charset=\"UTF-8\">\n"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
"<title>%s</title>\n"
"<style>\n"
"  *{margin:0;padding:0;box-sizing:border-box}\n"
"  body{background:%s;overflow:hidden;font-family:'JetBrains Mono','Courier New','Courier',monospace}\n"
"  canvas{display:block;width:100vw;height:100vh}\n"
"  #overlay{position:fixed;top:1rem;left:1rem;color:#d4aa3c;"
"font-size:.72rem;opacity:.8;pointer-events:none;line-height:1.6}\n"
"</style>\n</head>\n<body>\n"
"<canvas id=\"c\"></canvas>\n"
"%s\n"
"<script>\n"
"/* RIgArt v4.0 · φ=%.16f · Schumann=%.2f Hz */\n"
"const PHI=%.16f,PHI_INV=%.16f,SCHUMANN=%.2f;\n"
"const LIGHT_DIR=new Float32Array([%.4f,%.4f,%.4f]);\n"
"const LIGHT_COL=new Float32Array([%.4f,%.4f,%.4f]);\n"
"const ROUGHNESS=%.4f,METALLIC=%.4f,TEX_SCALE=%.4f;\n\n"
"/* ── Canvas v4 ─────────────────────────────── */\n"
"%s\n\n"
"/* ── WebGL2 PBR setup ─────────────────────── */\n"
"(function(){\n"
"  const canvas=document.getElementById('c');\n"
"  const W=window.innerWidth,H=window.innerHeight;\n"
"  canvas.width=W; canvas.height=H;\n"
"  const gl=canvas.getContext('webgl2',{\n"
"    antialias:true,depth:true,\n"
"    colorSpace:'%s'\n"
"  });\n"
"  if(!gl){document.body.innerHTML='<p style=\"color:#d44\">WebGL2 requerido</p>';return;}\n\n"
"  const VS=`#version 300 es\n"
"  in vec4 aPos; in vec3 aNorm; in vec2 aUV;\n"
"  uniform mat4 uMVP; uniform float uTime;\n"
"  out vec3 vNorm; out vec2 vUV; out vec3 vPos;\n"
"  void main(){\n"
"    float phi=%.8f;\n"
"    vec4 p=aPos; p.y+=sin(p.x*phi+uTime*%.2f)*0.003;\n"
"    gl_Position=uMVP*p;\n"
"    vNorm=aNorm; vUV=aUV; vPos=p.xyz;\n"
"  }`;\n\n"
"  const FS=`#version 300 es\n"
"  precision highp float;\n"
"  in vec3 vNorm; in vec2 vUV; in vec3 vPos;\n"
"  uniform vec3 uLightDir,uLightCol;\n"
"  uniform float uRoughness,uMetallic,uTime;\n"
"  out vec4 fragColor;\n"
"  const float PI=3.14159265359;\n"
"  const float PHI=%.8f;\n"
"  /* Cook-Torrance GGX */\n"
"  float D_GGX(float NdH,float a){float a2=a*a;float d=NdH*NdH*(a2-1.)+1.;\n"
"    return a2/(PI*d*d+1e-7);}\n"
"  float G_Schlick(float NdV,float k){return NdV/(NdV*(1.-k)+k+1e-7);}\n"
"  float G_Smith(float NdV,float NdL,float a){float k=(a+1.)*(a+1.)/8.;\n"
"    return G_Schlick(NdV,k)*G_Schlick(NdL,k);}\n"
"  vec3 F_Schlick(float VdH,vec3 F0){return F0+(1.-F0)*pow(1.-VdH,5.);}\n"
"  /* SSS skin — Jimenez mod */\n"
"  vec3 skin_sss(vec3 pos,vec3 N,vec3 L){\n"
"    float wrap=0.32; float d=max(dot(N,L)+wrap,0.)/(1.+wrap);\n"
"    vec3 sc=vec3(%.4f,%.4f,%.4f);\n"
"    return sc*d*%.4f;}\n"
"  /* Schumann pulse */\n"
"  float schumann(float t){return sin(t*%.4f*2.*PI)*0.5+0.5;}\n"
"  void main(){\n"
"    vec3 N=normalize(vNorm);\n"
"    vec3 L=normalize(uLightDir);\n"
"    vec3 V=normalize(vec3(0.,0.,1.)-vPos);\n"
"    vec3 H=normalize(L+V);\n"
"    float NdL=max(dot(N,L),0.),NdV=max(dot(N,V),1e-4);\n"
"    float NdH=max(dot(N,H),0.),VdH=max(dot(V,H),0.);\n"
"    vec3 alb=vec3(%.4f,%.4f,%.4f);\n"
"    vec3 F0=mix(vec3(0.04),alb,uMetallic);\n"
"    float a=uRoughness*uRoughness;\n"
"    float D=D_GGX(NdH,a);\n"
"    float G=G_Smith(NdV,NdL,a);\n"
"    vec3 F=F_Schlick(VdH,F0);\n"
"    vec3 spec=D*G*F/(4.*NdV*NdL+1e-7);\n"
"    vec3 diff=(1.-F)*(1.-uMetallic)*alb/PI;\n"
"    vec3 sss=skin_sss(vPos,N,L);\n"
"    vec3 col=(diff+spec)*uLightCol*NdL+sss;\n"
"    /* ACES tonemap */\n"
"    col*=0.6;float aa=2.51,bb=0.03,cc=2.43,dd=0.59,ee=0.14;\n"
"    col=clamp((col*(aa*col+bb))/(col*(cc*col+dd)+ee),0.,1.);\n"
"    /* φ-pulse ambient */\n"
"    col+=vec3(0.02)*schumann(uTime);\n"
"    fragColor=vec4(pow(col,vec3(1./2.2)),1.);\n"
"  }`;\n\n"
"  function compileShader(type,src){\n"
"    const s=gl.createShader(type);gl.shaderSource(s,src);gl.compileShader(s);\n"
"    if(!gl.getShaderParameter(s,gl.COMPILE_STATUS)){console.error(gl.getShaderInfoLog(s));return null;}\n"
"    return s;}\n"
"  const vs=compileShader(gl.VERTEX_SHADER,VS);\n"
"  const fs=compileShader(gl.FRAGMENT_SHADER,FS);\n"
"  const prog=gl.createProgram();\n"
"  gl.attachShader(prog,vs);gl.attachShader(prog,fs);gl.linkProgram(prog);\n"
"  if(!gl.getProgramParameter(prog,gl.LINK_STATUS)){console.error(gl.getProgramInfoLog(prog));return;}\n\n"
"  /* Icosfera φ-paramétrica procedural */\n"
"  const t=(1.+Math.sqrt(5.))/2.;\n"
"  const verts=[[-1,t,0],[1,t,0],[-1,-t,0],[1,-t,0],\n"
"    [0,-1,t],[0,1,t],[0,-1,-t],[0,1,-t],\n"
"    [t,0,-1],[t,0,1],[-t,0,-1],[-t,0,1]];\n"
"  const idx=[0,11,5,0,5,1,0,1,7,0,7,10,0,10,11,1,5,9,5,11,4,11,10,2,10,7,6,7,1,8,\n"
"    3,9,4,3,4,2,3,2,6,3,6,8,3,8,9,4,9,5,2,4,11,6,2,10,8,6,7,9,8,1];\n"
"  const pv=[],pn=[],pu=[];\n"
"  for(let i=0;i<idx.length;i+=3){\n"
"    for(let j=0;j<3;j++){\n"
"      const v=verts[idx[i+j]];\n"
"      const len=Math.hypot(...v);\n"
"      const n=v.map(x=>x/len);\n"
"      pv.push(...n); pn.push(...n);\n"
"      pu.push(Math.atan2(n[0],n[2])/(2*Math.PI)+.5,Math.acos(n[1])/Math.PI);\n"
"    }\n"
"  }\n"
"  function mkBuf(data,target){\n"
"    const b=gl.createBuffer();gl.bindBuffer(target,b);\n"
"    gl.bufferData(target,new Float32Array(data),gl.STATIC_DRAW);return b;}\n"
"  const vbo=mkBuf(pv,gl.ARRAY_BUFFER);\n"
"  const nbo=mkBuf(pn,gl.ARRAY_BUFFER);\n"
"  const ubo=mkBuf(pu,gl.ARRAY_BUFFER);\n\n"
"  function attr(buf,loc,n){\n"
"    gl.bindBuffer(gl.ARRAY_BUFFER,buf);\n"
"    gl.enableVertexAttribArray(loc);\n"
"    gl.vertexAttribPointer(loc,n,gl.FLOAT,false,0,0);}\n"
"  const aPos=gl.getAttribLocation(prog,'aPos');\n"
"  const aNorm=gl.getAttribLocation(prog,'aNorm');\n"
"  const aUV=gl.getAttribLocation(prog,'aUV');\n"
"  const uMVP=gl.getUniformLocation(prog,'uMVP');\n"
"  const uTime=gl.getUniformLocation(prog,'uTime');\n"
"  const uLD=gl.getUniformLocation(prog,'uLightDir');\n"
"  const uLC=gl.getUniformLocation(prog,'uLightCol');\n"
"  const uRough=gl.getUniformLocation(prog,'uRoughness');\n"
"  const uMetal=gl.getUniformLocation(prog,'uMetallic');\n\n"
"  let angle=0,t0=performance.now();\n"
"  const PHIf=%.8f;\n"
"  function frame(ts){\n"
"    const dt=(ts-t0)/1000;t0=ts;\n"
"    if(%s)angle+=dt*%.4f;\n"
"    gl.viewport(0,0,W,H);\n"
"    gl.clearColor(0,0,0,1);gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);\n"
"    gl.enable(gl.DEPTH_TEST);\n"
"    gl.useProgram(prog);\n"
"    attr(vbo,aPos,3);attr(nbo,aNorm,3);attr(ubo,aUV,2);\n"
"    /* MVP: perspectiva + rotación */\n"
"    const asp=W/H,fov=Math.PI/3.5,near=0.1,far=100;\n"
"    const f=1/Math.tan(fov/2);\n"
"    const proj=[f/asp,0,0,0, 0,f,0,0,\n"
"      0,0,(far+near)/(near-far),-1, 0,0,2*far*near/(near-far),0];\n"
"    const c=Math.cos(angle),s=Math.sin(angle);\n"
"    const view=[c,0,s,0, 0,1,0,0, -s,0,c,0, 0,0,-3,1];\n"
"    const mvp=new Float32Array(16);\n"
"    for(let i=0;i<4;i++)for(let j=0;j<4;j++){\n"
"      let v=0;for(let k=0;k<4;k++)v+=proj[i*4+k]*view[k*4+j];\n"
"      mvp[i*4+j]=v;}\n"
"    gl.uniformMatrix4fv(uMVP,false,mvp);\n"
"    gl.uniform1f(uTime,ts/1000);\n"
"    gl.uniform3fv(uLD,LIGHT_DIR);\n"
"    gl.uniform3fv(uLC,LIGHT_COL);\n"
"    gl.uniform1f(uRough,ROUGHNESS);\n"
"    gl.uniform1f(uMetal,METALLIC);\n"
"    gl.drawArrays(gl.TRIANGLES,0,pv.length/3);\n"
"    requestAnimationFrame(frame);}\n"
"  requestAnimationFrame(frame);\n"
"  window.addEventListener('resize',()=>{canvas.width=window.innerWidth;canvas.height=window.innerHeight;});\n"
"})();\n"
"</script>\n</body>\n</html>\n",

        title, bg,
        ctx->show_stats
            ? "<div id=\"overlay\">"
              "RIgArt v4.0 · φ=1.618 · Schumann 7.83 Hz<br>"
              "WebGL2 · PBR · SSS Skin · ACES</div>"
            : "",
        BRIDGE_PHI, BRIDGE_SCHUMANN,
        BRIDGE_PHI, BRIDGE_PHI_INV, BRIDGE_SCHUMANN,
        lx, ly, lz,
        ctx->light_color[0]>0?ctx->light_color[0]:1.0f,
        ctx->light_color[1]>0?ctx->light_color[1]:0.98f,
        ctx->light_color[2]>0?ctx->light_color[2]:0.94f,
        ctx->roughness>0?ctx->roughness:0.35f,
        ctx->metallic>=0.0f?ctx->metallic:0.0f,
        ctx->tex_scale>0?ctx->tex_scale:1.0f,

        canvas_res.js ? canvas_res.js : "const RIG_CANVAS={width:1280,height:720,layers:[]};",
        ctx->hdr_p3 ? "display-p3" : "srgb",

        BRIDGE_PHI, BRIDGE_SCHUMANN,

        BRIDGE_PHI,

        0.5882f, 0.3647f, 0.3020f,
        0.45f,
        BRIDGE_SCHUMANN,

        0.8157f, 0.6431f, 0.5725f,

        BRIDGE_PHI,
        ctx->auto_rotate ? "true" : "false",
        BRIDGE_PHI_INV
    );

    if (pos <= 0 || pos >= BRIDGE_HTML_CAP) {
        free(html);
        rigart_v4_free_result(&comp_res);
        rigart_v4_free_result(&canvas_res);
        snprintf(out->error, 255, "renderer_html_v4: buffer overflow");
        return -1;
    }

    out->html    = html;
    out->ok      = true;
    out->certeza = BRIDGE_PHI_INV;
    out->phi_ratio = BRIDGE_PHI;

    rigart_v4_free_result(&comp_res);
    rigart_v4_free_result(&canvas_res);
    return 0;
}
