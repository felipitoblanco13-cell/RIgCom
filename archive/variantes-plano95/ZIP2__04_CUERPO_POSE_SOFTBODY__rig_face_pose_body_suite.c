#include "rig_face_pose_body_suite.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static int suite_mkdir(const char *path)
{
    if (!path || !*path) return -1;
    if (mkdir(path, 0775) == 0 || errno == EEXIST) return 0;
    return -1;
}

static int suite_path(char *dst, size_t cap, const char *dir, const char *name)
{
    if (!dst || !cap || !dir || !name) return -1;
    size_t n = strlen(dir);
    int rc = snprintf(dst, cap, "%s%s%s", dir, n && dir[n - 1] == '/' ? "" : "/", name);
    return rc > 0 && (size_t)rc < cap ? 0 : -1;
}

static int suite_write_text(const char *path, const char *text)
{
    if (!path || !text) return -1;
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t n = strlen(text);
    int rc = fwrite(text, 1, n, f) == n ? 0 : -1;
    if (fclose(f) != 0) rc = -1;
    return rc;
}

static int suite_write_ng_result(const char *dir, const char *stem, RigArtResultNG *r)
{
    char p[1024];
    if (!r || !r->ok) return -1;
    if (r->html) { if (suite_path(p,sizeof(p),dir,stem)) return -1; if (suite_write_text(p,r->html)) return -1; }
    return 0;
}

RigSuiteBuildOptions rig_suite_default_options(void)
{
    RigSuiteBuildOptions o;
    memset(&o, 0, sizeof(o));
    o.archetype = RIG_ARCH_HYPERREALIST;
    o.face_subdivision = 6;
    o.body = rig_sov_body_default_params();
    o.material = (RigSovMaterial){{0.55f,0.32f,0.24f},0.46f,0.0f,0.0f};
    o.generate_body_glb = true;
    o.generate_body_obj = true;
    o.generate_face_obj = true;
    o.generate_studio_html = true;
    o.generate_renderer_html = true;
    o.generate_shader_pack = true;
    o.generate_microdetail_maps = true;
    return o;
}

const char *rig_suite_version(void)
{
    return "Rig Face Pose Body Suite 1.0.0";
}

static int generate_body(const char *dir, const RigSuiteBuildOptions *o,
                         size_t *vertices, size_t *triangles, size_t *joints)
{
    RigSovMesh mesh;
    RigSovSkeleton skeleton;
    char path[1024], err[256];
    rig_sov_mesh_init(&mesh);
    int rc = rig_sov_generate_complete_body(&o->body, &mesh, &skeleton);
    if (!rc) rc = rig_sov_mesh_validate(&mesh, err, sizeof(err));
    if (!rc && o->generate_body_glb) {
        if (suite_path(path,sizeof(path),dir,"complete_body.glb")) rc = -1;
        else rc = rig_sov_export_glb(&mesh, &skeleton, &o->material, path);
    }
    if (!rc && o->generate_body_obj) {
        if (suite_path(path,sizeof(path),dir,"complete_body.obj")) rc = -1;
        else rc = rig_sov_mesh_write_obj(&mesh, path);
    }
    if (!rc) {
        if (vertices) *vertices = mesh.vertex_count;
        if (triangles) *triangles = mesh.triangle_count;
        if (joints) *joints = skeleton.joint_count;
    }
    rig_sov_mesh_free(&mesh);
    return rc;
}

static int generate_face(const char *dir, const RigSuiteBuildOptions *o,
                         uint32_t *vertices, uint32_t *triangles)
{
    const RigFaceArchetype *arch = rig_archetype_get(o->archetype);
    if (!arch) return -1;
    RigFaceMeshV2 *face = rig_face_v2_create(&arch->params, o->face_subdivision);
    if (!face) return -1;
    face->archetype_id = o->archetype;
    face->has_archetype = true;
    face->iris_left = arch->iris;
    face->iris_right = arch->iris;
    face->ear_left_params = arch->ear_left;
    face->ear_right_params = arch->ear_right;
    face->lip = arch->lip;

    int rc = 0;
    if (o->generate_microdetail_maps) {
        if (rig_face_build_pore_system(face, 1.0f) ||
            rig_face_build_vascular_tree(face) ||
            rig_face_build_wrinkle_lines(face, arch->params.age_factor, 0.0f)) rc = -1;
    }
    char path[1024];
    if (!rc && o->generate_face_obj) {
        if (suite_path(path,sizeof(path),dir,"hyperreal_face.obj")) rc = -1;
        else rc = rig_face_v2_export_obj(face, path);
    }
    if (!rc && o->generate_shader_pack) {
        char *shader = (char *)calloc(1u, 128u * 1024u);
        if (!shader) rc = -1;
        else {
            int n = rig_codegen_glsl_skin_shader(face, shader, 128u * 1024u);
            if (n < 0 || suite_path(path,sizeof(path),dir,"face_skin_codegen.glsl") ||
                suite_write_text(path, shader)) rc = -1;
            free(shader);
        }
    }
    if (!rc) {
        if (vertices) *vertices = face->base.n_verts;
        if (triangles) *triangles = face->base.n_tris;
    }
    rig_face_v2_destroy(face);
    return rc;
}

static int generate_ng(const char *dir, const RigSuiteBuildOptions *o)
{
    (void)o;
    RigFaceNGParams p;
    RigFaceNGRenderCtx render;
    RigArtResultNG result;
    memset(&result,0,sizeof(result));
    if (rig_face_ng_params_default(&p)) return -1;
    snprintf(p.name,sizeof(p.name),"Rig Face Pose Body — Hyperreal Studio");
    p.lod = RIG_LOD_CINEMA;
    p.quality = 1.0f;
    memset(&render,0,sizeof(render));
    render.cam_fov_deg = 35.0f;
    render.cam_dist = 2.35f;
    render.exposure = 1.0f;
    render.gamma = 2.2f;
    render.bloom_enabled = true;
    render.dof_enabled = true;
    render.vignette = true;
    render.light_count = 3;
    if (rig_face_ng_assemble(&p,&render,&result) <= 0) return -1;
    int rc = 0;
    if (o->generate_studio_html) rc = suite_write_ng_result(dir,"rig_suite_studio.html",&result);
    rig_face_ng_result_free(&result);
    if (rc || !o->generate_shader_pack) return rc;

    RigArtResultNG skin={0}, eye={0}, hair={0}, anim={0};
    RigEyeNGCtx ec={3.5f,5.9f,0.32f,0.20f,0.08f,0.65f,0.42f,0.52f,0.86f,0.20f,0.18f,0.0f,0.1f,3.0f,2.6f};
    RigHairNGCtx hc={0};
    hc.melanin_eu=.55f; hc.melanin_ph=.10f; hc.melanin_sigma=.08f; hc.tint_r=.18f; hc.tint_g=.08f; hc.tint_b=.035f;
    hc.curve_type=HAIR_CURVE_WAVY; hc.curl_radius_mm=8.0f; hc.curl_freq=1.4f; hc.strand_width_root=0.00008f; hc.strand_width_tip=0.000025f;
    hc.length_avg_cm=18.0f; hc.length_sigma=3.0f; hc.strand_count=90000; hc.cross_section_ratio=.82f; hc.stiffness=.72f; hc.damping=.06f;
    hc.gravity_scale=1.0f; hc.pbd_iterations=8; hc.pbd_segments=16; hc.roughness_long=.22f; hc.roughness_azimuth=.38f;
    hc.cuticle_tilt=.055f; hc.specular_lobe_r=1.0f; hc.specular_lobe_tt=.55f; hc.specular_lobe_trt=.35f; hc.ior_cortex=1.55f; hc.ior_medulla=1.30f;
    hc.clump_strength=.25f; hc.clump_radius_mm=5.0f; hc.wisp_strength=.08f; hc.parting_pos=.5f; hc.parting_sharpness=.7f;
    if (rig_face_ng_skin_shader(&p.skin,&skin) < 0 || rig_face_ng_eye_shader(&ec,&eye) < 0 ||
        rig_face_ng_hair_shader(&hc,&hair) < 0 || rig_face_ng_anim_generate(&p.anim,&anim) < 0) rc=-1;
    char path[1024];
#define SAVE_FIELD(obj,field,name) do { if(!rc && (obj).field){ if(suite_path(path,sizeof(path),dir,name)||suite_write_text(path,(obj).field)) rc=-1; } } while(0)
    SAVE_FIELD(skin,glsl_vert,"skin_ng.vert.glsl"); SAVE_FIELD(skin,glsl_frag,"skin_ng.frag.glsl");
    SAVE_FIELD(eye,glsl_vert,"eyes_ng.vert.glsl"); SAVE_FIELD(eye,glsl_frag,"eyes_ng.frag.glsl");
    SAVE_FIELD(hair,glsl_vert,"hair_ng.vert.glsl"); SAVE_FIELD(hair,glsl_frag,"hair_ng.frag.glsl");
    SAVE_FIELD(anim,js,"animation_ng.js");
#undef SAVE_FIELD
    rig_face_ng_result_free(&skin); rig_face_ng_result_free(&eye); rig_face_ng_result_free(&hair); rig_face_ng_result_free(&anim);
    return rc;
}


static int generate_v2_pack(const char *dir)
{
    RigFaceAssemblyCtx c;
    RigArtResultV4 r;
    memset(&c, 0, sizeof(c));
    c.lod = 3;
    c.age.age = 28.0f;
    c.age.ethnicity = 1;
    c.age.skin_sag = 0.03f;
    c.age.nasolabial_depth = 0.08f;
    c.age.crow_feet = 0.03f;
    c.age.forehead_wrinkles = 0.02f;
    c.age.jowl_amount = 0.01f;
    c.age.orbital_fat_pad = 0.55f;
    c.skin.melanin = 0.35f;
    c.skin.hemoglobin = 0.42f;
    c.skin.carotene = 0.08f;
    c.skin.bilirubin = 0.02f;
    c.skin.lipid_roughness = 0.38f;
    c.skin.specular_ior = 1.42f;
    c.skin.pore_density = 0.78f;
    c.skin.pore_depth = 0.045f;
    c.skin.pore_scale = 1.0f;
    c.skin.scatter_radius_oil = (Vec3f){0.08f,0.04f,0.02f};
    c.skin.scatter_radius_epidermis = (Vec3f){0.35f,0.16f,0.08f};
    c.skin.scatter_radius_dermis = (Vec3f){1.0f,0.45f,0.22f};
    c.skin.scatter_radius_subcut = (Vec3f){1.8f,0.75f,0.35f};
    c.skin.transmittance = 0.38f;
    c.skin.vein_color = (Vec3f){0.16f,0.28f,0.34f};
    c.skin.vein_depth = 0.35f;
    c.skin.vein_visibility = 0.22f;
    c.skin.wrinkle_density = 0.12f;
    c.skin.wrinkle_depth = 0.035f;
    c.skin.skin_oiliness = 0.30f;
    c.animator.blink_rate = 16.0f;
    c.animator.blink_duration = 0.12f;
    c.animator.breath_rate = 14.0f;
    c.animator.breath_depth = 0.02f;
    c.animator.emotion_blend_speed = 7.0f;
    c.animator.eye_lead_ratio = 0.62f;
    c.animator.head_follow_speed = 4.0f;
    c.animator.saccade_amplitude = 0.035f;
    c.animator.saccade_probability = 0.32f;
    c.animator.saccade_speed = 18.0f;
    c.animator.enable_micro_twitches = true;
    c.animator.twitch_amplitude = 0.004f;
    c.animator.twitch_frequency = 0.8f;
    c.moisture.enable_tear_meniscus = true;
    c.moisture.tear_level = 0.35f;
    c.moisture.tear_streak_speed = 0.08f;
    c.moisture.tear_color = (Vec3f){0.90f,0.96f,1.0f};
    c.moisture.lip_gloss_amount = 0.42f;
    c.moisture.lip_gloss_specularity = 0.75f;
    c.moisture.sweat_density = 0.02f;
    c.moisture.sweat_bead_size = 0.015f;
    c.moisture.sweat_specularity = 0.85f;
    rigart_v4_init_result(&r);
    int rc = rig_face_v2_assembly(&c, &r) < 0 ? -1 : 0;
    char path[1024];
#define SAVE_V2(field,name) do { if(!rc && r.field){ if(suite_path(path,sizeof(path),dir,name)||suite_write_text(path,r.field)) rc=-1; } } while(0)
    SAVE_V2(html,"rig_suite_v2.html");
    SAVE_V2(css,"rig_suite_v2.css");
    SAVE_V2(js,"rig_suite_v2.js");
    SAVE_V2(glsl_vert,"rig_suite_v2.vert.glsl");
    SAVE_V2(glsl_frag,"rig_suite_v2.frag.glsl");
#undef SAVE_V2
    rigart_v4_free_result(&r);
    return rc;
}

static int generate_renderer(const char *dir, const RigSuiteBuildOptions *o)
{
    if (!o->generate_renderer_html) return 0;
    RIgArtRendererCtxV4 c;
    RigArtResultV4 r;
    memset(&c,0,sizeof(c));
    snprintf(c.title,sizeof(c.title),"Rig Face Pose Body Suite");
    snprintf(c.bg_color,sizeof(c.bg_color),"#030208");
    c.exposure=1.0f; c.enable_bloom=true; c.bloom_strength=.618f; c.enable_dof=true;
    c.show_stats=true; c.hdr_p3=true; c.auto_rotate=true; c.roughness=.35f; c.metallic=.0f; c.tex_scale=1.0f;
    c.light_dir[0]=4.0f;c.light_dir[1]=8.0f;c.light_dir[2]=5.0f;
    c.light_color[0]=1.0f;c.light_color[1]=.98f;c.light_color[2]=.94f;
    rigart_v4_init_result(&r);
    int rc = rigart_renderer_html_v4(&c,&r);
    char path[1024];
    if (!rc && r.html) {
        if (suite_path(path,sizeof(path),dir,"rig_suite_renderer.html") || suite_write_text(path,r.html)) rc=-1;
    }
    rigart_v4_free_result(&r);
    return rc;
}

int rig_suite_generate(const char *dir, const RigSuiteBuildOptions *input,
                       char *report, size_t report_size)
{
    if (!dir || !*dir) return -1;
    RigSuiteBuildOptions o = input ? *input : rig_suite_default_options();
    if ((int)o.archetype < 0 || o.archetype >= RIG_ARCH_COUNT) o.archetype = RIG_ARCH_HYPERREALIST;
    if (o.face_subdivision < 2) o.face_subdivision=2;
    if (o.face_subdivision > 6) o.face_subdivision=6;
    if (suite_mkdir(dir)) return -1;
    size_t bv=0,bt=0,bj=0; uint32_t fv=0,ft=0;
    int rc = generate_body(dir,&o,&bv,&bt,&bj);
    if (!rc) rc = generate_face(dir,&o,&fv,&ft);
    if (!rc) rc = generate_ng(dir,&o);
    if (!rc) rc = generate_v2_pack(dir);
    if (!rc) rc = generate_renderer(dir,&o);
    char path[1024], manifest[4096];
    const RigFaceArchetype *a=rig_archetype_get(o.archetype);
    int n=snprintf(manifest,sizeof(manifest),
        "{\n  \"suite\": \"%s\",\n  \"archetype_id\": %d,\n  \"archetype\": \"%s\",\n"
        "  \"face_subdivision\": %u,\n  \"body\": {\"vertices\": %zu, \"triangles\": %zu, \"joints\": %zu},\n"
        "  \"face\": {\"vertices\": %u, \"triangles\": %u},\n  \"status\": \"%s\"\n}\n",
        rig_suite_version(),(int)o.archetype,a?a->name:"unknown",o.face_subdivision,bv,bt,bj,fv,ft,rc?"error":"complete");
    if (n>0 && (size_t)n<sizeof(manifest) && !suite_path(path,sizeof(path),dir,"suite_manifest.json")) {
        if (suite_write_text(path,manifest) && !rc) rc=-1;
    }
    if (report && report_size) {
        snprintf(report,report_size,rc?"FAIL: unified suite generation":"OK: unified suite — body %zu/%zu, %zu joints; face %u/%u; archetype %s; studio NG/V2, renderer and shader pack generated",
            bv,bt,bj,fv,ft,a?a->name:"unknown");
    }
    return rc;
}

int rig_suite_selftest(const char *dir, char *report, size_t report_size)
{
    char sov[512]={0}, integrated[512]={0}, sub[1024];
    if (!dir || suite_mkdir(dir)) return -1;
    if (suite_path(sub,sizeof(sub),dir,"sovereign_validation")) return -1;
    int a=rig_sov_selftest(sub,sov,sizeof(sov));
    RigSuiteBuildOptions o=rig_suite_default_options();
    if (suite_path(sub,sizeof(sub),dir,"integrated_validation")) return -1;
    int b=rig_suite_generate(sub,&o,integrated,sizeof(integrated));
    if (report && report_size) snprintf(report,report_size,"sovereign=[%s] integrated=[%s]",sov,integrated);
    return a? a : b;
}
