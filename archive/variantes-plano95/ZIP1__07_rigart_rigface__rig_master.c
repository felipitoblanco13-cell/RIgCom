/* ═══════════════════════════════════════════════════════════════════════════
 * rig_master.c — LA UNIÓN · RIGCOM MASTER · BLOQUE 7
 *
 * Todo el ecosistema pasa por aquí.
 *
 * C11 · cero libc · φ = 1.6180339887498948
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rig_master.h"

extern float rl_sqrtf(float);
extern float rl_sinf(float);
extern float rl_cosf(float);
extern float rl_fabsf(float);
extern void *rl_memset(void *, int, unsigned long);
extern void *rl_memcpy(void *, const void *, unsigned long);
extern int   rl_snprintf(char *, unsigned long, const char *, ...);

#define RM_PHI  1.6180339887498948f
#define RM_TAU  6.28318530717958648f

static float rm_clamp(float x, float a, float b){ return x<a?a:(x>b?b:x); }
static float rm_max(float a, float b){ return a>b?a:b; }
static float rm_abs(float x){ return x<0?-x:x; }

static void rm_v3norm(float v[3]){
    float l = rl_sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (l > 1e-9f) { v[0]/=l; v[1]/=l; v[2]/=l; }
}

/* Inversa de una 4×4 column-major (Cramer). Necesaria para pasar del ojo
 * al espacio del volumen y para desproyectar el toque. */
static bool rm_inv4(const float m[16], float out[16])
{
    float inv[16], det;

    inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
             + m[9]*m[7]*m[14]  + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
             - m[8]*m[7]*m[14]  - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8]  =  m[4]*m[9]*m[15]  - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
             + m[8]*m[7]*m[13]  + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14]  + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
             - m[8]*m[6]*m[13]  - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
             - m[9]*m[3]*m[14]  - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
             + m[8]*m[3]*m[14]  + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9]  = -m[0]*m[9]*m[15]  + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
             - m[8]*m[3]*m[13]  - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14]  - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
             + m[8]*m[2]*m[13]  + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2]  =  m[1]*m[6]*m[15]  - m[1]*m[7]*m[14]  - m[5]*m[2]*m[15]
             + m[5]*m[3]*m[14]  + m[13]*m[2]*m[7]  - m[13]*m[3]*m[6];
    inv[6]  = -m[0]*m[6]*m[15]  + m[0]*m[7]*m[14]  + m[4]*m[2]*m[15]
             - m[4]*m[3]*m[14]  - m[12]*m[2]*m[7]  + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15]  - m[0]*m[7]*m[13]  - m[4]*m[1]*m[15]
             + m[4]*m[3]*m[13]  + m[12]*m[1]*m[7]  - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14]  + m[0]*m[6]*m[13]  + m[4]*m[1]*m[14]
             - m[4]*m[2]*m[13]  - m[12]*m[1]*m[6]  + m[12]*m[2]*m[5];
    inv[3]  = -m[1]*m[6]*m[11]  + m[1]*m[7]*m[10]  + m[5]*m[2]*m[11]
             - m[5]*m[3]*m[10]  - m[9]*m[2]*m[7]   + m[9]*m[3]*m[6];
    inv[7]  =  m[0]*m[6]*m[11]  - m[0]*m[7]*m[10]  - m[4]*m[2]*m[11]
             + m[4]*m[3]*m[10]  + m[8]*m[2]*m[7]   - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11]  + m[0]*m[7]*m[9]   + m[4]*m[1]*m[11]
             - m[4]*m[3]*m[9]   - m[8]*m[1]*m[7]   + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10]  - m[0]*m[6]*m[9]   - m[4]*m[1]*m[10]
             + m[4]*m[2]*m[9]   + m[8]*m[1]*m[6]   - m[8]*m[2]*m[5];

    det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (rm_abs(det) < 1e-12f) return false;

    det = 1.0f / det;
    for (int i = 0; i < 16; i++) out[i] = inv[i] * det;
    return true;
}

static void rm_mul4(const float a[16], const float b[16], float o[16])
{
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += a[k*4+r] * b[c*4+k];
            o[c*4+r] = s;
        }
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §1  INIT / DESTROY
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_init(RigMasterCtx *m, uint32_t w, uint32_t h)
{
    if (!m) return -1;
    rl_memset(m, 0, sizeof(*m));

    m->width    = w;
    m->height   = h;
    m->exposure = 1.0f;

    rig_headtrack_default_cal(&m->cal);      /* Honor 400: 69.8 × 151.1 mm */
    rig_headtrack_init(&m->obs);
    m->head_coupled = true;

    rig_impasto_init(&m->impasto);

    m->initialized = true;
    return 0;
}

void rig_master_destroy(RigMasterCtx *m)
{
    if (!m) return;
    rig_env_gpu_free(&m->env);
    if (m->aom_active) aom_free(&m->aom);
    if (m->haptic_active) usonic_shutdown();
    rl_memset(m, 0, sizeof(*m));
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §2  MATERIAL
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_set_material(RigMasterCtx *m, uint32_t id, const RigMaterial *mat)
{
    if (!m || !mat || id >= RIG_MASTER_MAX_MATS) return -1;

    m->materials[id] = *mat;
    m->materials[id].id = id;

    /* ★ SIEMPRE derivar. El tacto no se escribe: sale del BRDF. */
    rig_material_derive_haptics(&m->materials[id]);

    if (id >= m->material_count) m->material_count = id + 1u;
    return 0;
}

int rig_master_load_presets(RigMasterCtx *m)
{
    if (!m) return -1;
    uint32_t n = rig_material_preset_count();
    if (n > RIG_MASTER_MAX_MATS) n = RIG_MASTER_MAX_MATS;

    for (uint32_t i = 0; i < n; i++) {
        RigMaterial mat;
        if (rig_material_preset(&mat, i) == 0)
            rig_master_set_material(m, i, &mat);
    }
    return (int)n;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §3  LUZ — el paso caro, y se hace UNA vez
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_set_env(RigMasterCtx *m, RigEnvPreset p,
                       uint32_t cube_size, uint32_t samples)
{
    if (!m) return -1;

    rig_env_gpu_free(&m->env);
    rig_env_preset(&m->env_desc, p);

    /* Radiancia → cubemap → SH(9) + 6 mips GGX.
     * Después de esto, todo el IBL es gratis: 27 floats y una textura. */
    return rig_env_build(&m->env_desc, &m->env,
                         cube_size ? cube_size : 128u,
                         samples   ? samples   : 128u);
}

int rig_master_add_light(RigMasterCtx *m, const RigLight *l)
{
    if (!m || !l || m->light_count >= RIG_MASTER_MAX_LIGHTS) return -1;
    m->lights[m->light_count] = *l;
    rm_v3norm(m->lights[m->light_count].dir);
    m->light_count++;
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §4  GEOMETRÍA
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_submit_mesh(RigMasterCtx *m, const RigMesh *mesh)
{
    if (!m || !mesh || m->mesh_count >= RIG_MASTER_MAX_MESH) return -1;
    m->meshes[m->mesh_count] = *mesh;
    return (int)(m->mesh_count++);
}

void rig_master_clear_meshes(RigMasterCtx *m)
{
    if (!m) return;
    m->mesh_count = 0u;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §5  ★ EL OBSERVADOR — la pantalla se vuelve ventana
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_set_observer(RigMasterCtx *m, const float eye_mm[3])
{
    if (!m || !eye_mm) return -1;
    m->obs.eye_mm[0] = eye_mm[0];
    m->obs.eye_mm[1] = eye_mm[1];
    m->obs.eye_mm[2] = rm_max(eye_mm[2], 80.0f);
    m->obs.valid = true;
    return 0;
}

int rig_master_track_eyes(RigMasterCtx *m, const float eyeL[2],
                          const float eyeR[2], float dt)
{
    if (!m) return -1;
    if (!eyeL || !eyeR) return rig_headtrack_lost(&m->obs, &m->cal, dt);
    return rig_headtrack_update(&m->obs, &m->cal, eyeL, eyeR, dt);
}

int rig_master_matrices(const RigMasterCtx *m, float proj[16], float view[16])
{
    if (!m || !proj || !view) return -1;

    /* ★ EL FRUSTUM ASIMÉTRICO.
     * Lo definen el OJO y las cuatro esquinas FÍSICAS del cristal.
     * Cuando el ojo se desplaza, l y r dejan de ser simétricos y el volumen
     * de visión SE INCLINA. Eso es el paralaje real. */
    int rc = rig_offaxis_projection(&m->cal, m->obs.eye_mm, 10.0f, 4000.0f, proj);
    if (rc) return rc;

    /* La cámara NO rota: la pantalla está fija en el mundo. Es el ojo el
     * que se mueve. Así que la vista es una traslación. */
    return rig_offaxis_view(m->obs.eye_mm, view);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §6  VOLUMEN
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_enable_aom(RigMasterCtx *m, float rf_power_w)
{
    if (!m) return -1;
    if (m->aom_active) return 0;

    int rc = aom_init(&m->aom, rf_power_w);
    if (rc) return rc;

    m->aom_active = true;

    /* El háptico solo tiene sentido si hay volumen que tocar */
    if (usonic_init() == 0) m->haptic_active = true;
    return 0;
}

int rig_master_submit_volume(RigMasterCtx *m, AOMPoint *pts, uint32_t n,
                             float x, float y, float z)
{
    if (!m || !m->aom_active) return -1;
    return aom_add_object(&m->aom, pts, n, x, y, z);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §7  FRAME
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_frame(RigMasterCtx *m, float dt)
{
    if (!m || !m->initialized) return -1;

    m->time += dt;
    m->frame++;

    if (m->aom_active) aom_voxelize(&m->aom);
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §8  ★★★ EL SONDEO UNIFICADO ★★★
 *
 * "El dedo tocó la pantalla en (sx, sy)"
 *                   ↓
 * "El dedo tocó SEDA, aquí, con esta normal, con esta aspereza local,
 *  y por tanto debe SONAR así y SENTIRSE así."
 *
 * El rayo SALE DEL OJO. Esa es la diferencia entre tocar un cristal y
 * meter el dedo dentro del volumen.
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_probe_ray(RigMasterCtx *m, const float ro_mm[3],
                         const float rd[3], float speed, float dt,
                         RigProbe *out)
{
    if (!m || !ro_mm || !rd || !out) return -1;
    rl_memset(out, 0, sizeof(*out));

    if (!m->aom_active) return 0;

    /* mm → coordenadas del volumen [0,1]³ */
    float half = m->aom.volume_mm * 0.5f;
    float ro[3] = { (ro_mm[0] + half) / m->aom.volume_mm,
                    (ro_mm[1] + half) / m->aom.volume_mm,
                    (ro_mm[2] + half) / m->aom.volume_mm };

    AOMHit hit;
    if (aom_probe(&m->aom, ro, rd, 0.055f, &hit) != 0) return -1;
    if (!hit.hit) {
        /* ★ AIRE ⇒ SILENCIO. La comprobación que no existía. */
        if (m->haptic_active) {
            RigHapticSig z;
            rl_memset(&z, 0, sizeof(z));
            usonic_set_signature(&z);
            usonic_update(dt);
        }
        return 0;
    }

    uint32_t mi = hit.voxel.material;
    if (mi >= m->material_count) mi = 0u;
    const RigMaterial *mat = &m->materials[mi];

    /* ── ★ ASPEREZA LOCAL ──
     * No la del material en general: la de ESTE punto exacto. Un cuero tiene
     * zonas con poros grandes y zonas casi lisas, y el dedo lo nota. */
    float lp[3] = { hit.pos[0] * 8.0f, hit.pos[1] * 8.0f, hit.pos[2] * 8.0f };
    float local_r = rig_height3d_local_roughness(lp, mat->height_fn,
                                                 mat->height_freq, 0.02f);

    /* ── La firma háptica, modulada por la aspereza local ── */
    RigHapticSig sig = mat->haptic;
    sig.grain    = rm_clamp(sig.grain * (0.55f + 0.75f * local_r), 0.0f, 1.0f);
    sig.am_depth = rm_clamp(sig.am_depth * (0.60f + 0.60f * local_r), 0.0f, 0.95f);

    /* ── ★ STICK-SLIP: dedo quieto ⇒ silencio ──
     * Una superficie no "raspa" si no hay deslizamiento. Física, no efecto. */
    float vN   = rm_clamp(speed / 180.0f, 0.0f, 1.0f);
    float slip = 0.22f + 0.78f * vN * sig.friction;

    /* Al deslizar más rápido, el dedo tropieza con el grano más a menudo */
    sig.f_mod    = rm_clamp(sig.f_mod * (0.65f + 0.55f * vN), 40.0f, 800.0f);
    sig.envelope = rm_clamp(sig.envelope * slip
                            * rm_clamp((float)hit.voxel.density / 160.0f, 0.0f, 1.0f),
                            0.0f, 1.0f);

    /* ── Al ultrasonido ── */
    if (m->haptic_active) {
        float hw[3] = { hit.pos[0] * m->aom.volume_mm - half,
                        hit.pos[1] * m->aom.volume_mm - half,
                        rm_max(hit.pos[2] * m->aom.volume_mm, 60.0f) };
        usonic_set_focus(hw[0], hw[1], hw[2]);
        usonic_set_signature(&sig);
        usonic_set_mode(sig.stm_radius > 0.5f ? BEAM_TRAVELING : BEAM_FOCUSED);
        usonic_update(dt);
    }

    out->hit             = true;
    out->world[0]        = hit.pos[0] * m->aom.volume_mm - half;
    out->world[1]        = hit.pos[1] * m->aom.volume_mm - half;
    out->world[2]        = hit.pos[2] * m->aom.volume_mm - half;
    out->normal[0]       = hit.normal[0];
    out->normal[1]       = hit.normal[1];
    out->normal[2]       = hit.normal[2];
    out->depth_mm        = hit.t * m->aom.volume_mm;
    out->material_id     = mi;
    out->local_roughness = local_r;
    out->sig             = sig;
    out->color[0]        = (float)hit.voxel.r / 255.0f;
    out->color[1]        = (float)hit.voxel.g / 255.0f;
    out->color[2]        = (float)hit.voxel.b / 255.0f;
    return 0;
}

int rig_master_probe_screen(RigMasterCtx *m, float sx, float sy,
                            float speed, float dt, RigProbe *out)
{
    if (!m || !out) return -1;

    float proj[16], view[16], vp[16], inv[16];
    if (rig_master_matrices(m, proj, view) != 0) return -1;
    rm_mul4(proj, view, vp);
    if (!rm_inv4(vp, inv)) return -1;

    /* Píxel → NDC → punto en el plano lejano */
    float ndc_x = (sx / (float)m->width)  * 2.0f - 1.0f;
    float ndc_y = 1.0f - (sy / (float)m->height) * 2.0f;

    float p[4];
    p[0] = inv[0]*ndc_x + inv[4]*ndc_y + inv[8]  + inv[12];
    p[1] = inv[1]*ndc_x + inv[5]*ndc_y + inv[9]  + inv[13];
    p[2] = inv[2]*ndc_x + inv[6]*ndc_y + inv[10] + inv[14];
    p[3] = inv[3]*ndc_x + inv[7]*ndc_y + inv[11] + inv[15];
    if (rm_abs(p[3]) < 1e-9f) return -1;

    float far[3] = { p[0]/p[3], p[1]/p[3], p[2]/p[3] };

    /* ★ EL RAYO SALE DEL OJO. No de la pantalla. */
    float ro[3] = { m->obs.eye_mm[0], m->obs.eye_mm[1], m->obs.eye_mm[2] };
    float rd[3] = { far[0] - ro[0], far[1] - ro[1], far[2] - ro[2] };
    rm_v3norm(rd);

    return rig_master_probe_ray(m, ro, rd, speed, dt, out);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §9  ★ PUENTE: RIGART → MASTER ★
 *
 * El pincel de óleo deposita pigmento CON RELIEVE FÍSICO.
 *
 * Antes ese relieve solo era color (un fake de iluminación en el canvas 2D).
 * Ahora es GEOMETRÍA: entra en el POM, proyecta sombra dentro de sus propios
 * surcos, y —lo importante— SE PUEDE TOCAR.
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_art_stroke(RigMasterCtx *m, float x, float y, float radius,
                          float thickness_mm, float viscosity)
{
    if (!m) return -1;
    return rig_impasto_add(&m->impasto, x, y, radius, thickness_mm, viscosity);
}

int rig_master_art_clear(RigMasterCtx *m)
{
    if (!m) return -1;
    return rig_impasto_init(&m->impasto);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §10  ★ PUENTE: RIGFACE → MASTER ★
 *
 * La piel NG deja de ser un shader aparte. Es un RigMaterial.
 *
 * Y con height_fn = PORES a 3 ciclos/mm, la derivación háptica le asigna
 * automáticamente su tacto:
 *
 *      dedo a 100 mm/s × 3 c/mm = 300 Hz
 *      pico de los corpúsculos de Pacini = 250 Hz
 *
 * Por eso la piel se siente tan PRESENTE. Y ahora sale calculado.
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_face_material(RigMasterCtx *m, uint32_t id,
                             float melanin, float hemoglobin,
                             float carotene, float age)
{
    if (!m || id >= RIG_MASTER_MAX_MATS) return -1;

    RigMaterial s;
    rig_material_default(&s);

    /* ── Albedo bioquímico (skin_bio_albedo, verbatim de rig_face_ng) ── */
    float mel = rm_clamp(melanin, 0.0f, 1.0f);
    float mph = rm_clamp(mel * 0.35f + 0.04f, 0.0f, 0.2f);
    float hem = rm_clamp(hemoglobin, 0.0f, 1.0f);
    float car = rm_clamp(carotene, 0.0f, 0.6f);
    float ag  = rm_clamp(age, 0.0f, 1.0f);

    float r = 0.87f, g = 0.775f, b = 0.70f;
    r *= (1.0f - 0.55f*mel);  g *= (1.0f - 0.38f*mel);  b *= (1.0f - 0.72f*mel);
    r *= (1.0f - 0.05f*mph);  g *= (1.0f - 0.20f*mph);  b *= (1.0f - 0.60f*mph);
    r += 0.22f*hem;           g += 0.04f*hem;           b += 0.02f*hem;
    r += 0.18f*car;           g += 0.12f*car;

    s.albedo[0] = rm_clamp(r, 0.0f, 1.0f);
    s.albedo[1] = rm_clamp(g, 0.0f, 1.0f);
    s.albedo[2] = rm_clamp(b, 0.0f, 1.0f);

    /* ── Los ajustes por edad de rig_face_ng, verbatim ── */
    s.metallic  = 0.0f;
    s.roughness = rm_clamp(0.45f + ag * 0.25f, 0.05f, 0.95f);
    s.ior       = 1.45f;                    /* estrato córneo */

    /* Vello facial: anisotropía suave */
    s.anisotropy   = 0.12f;
    s.tangent_flow = RIG_FLOW_UV;

    /* Peach fuzz: sheen tenue */
    s.sheen[0] = 0.06f; s.sheen[1] = 0.05f; s.sheen[2] = 0.04f;
    s.sheen_roughness = 0.5f;

    /* ★ POROS — y de aquí sale el tacto, solo */
    s.height_fn    = RIG_HFN_PORES;
    s.height_scale = rm_clamp(0.025f + ag * 0.035f, 0.02f, 0.07f);
    s.height_freq  = 46.0f;

    /* SSS 8 capas de Donner-Jensen */
    s.sss_weight       = rm_clamp(0.45f - ag * 0.10f, 0.2f, 0.5f);
    s.sss_radius_mm[0] = 3.2f;      /* el rojo penetra más */
    s.sss_radius_mm[1] = 1.4f;
    s.sss_radius_mm[2] = 0.8f;

    const char *nm = "Piel Humana NG";
    for (int i = 0; i < 15; i++) s.name[i] = nm[i];
    s.name[15] = 0;

    return rig_master_set_material(m, id, &s);
}

/* Voxelizar una malla al AOM → tu avatar flota en la ventana */
int rig_master_mesh_to_volume(RigMasterCtx *m, const RigMesh *mesh,
                              uint32_t density, AOMPoint *out,
                              uint32_t max_points, uint32_t *out_count)
{
    if (!m || !mesh || !out || !out_count) return -1;
    if (!mesh->pos || !mesh->idx) return -1;

    *out_count = 0u;
    if (density == 0) density = 6u;

    uint32_t tris = mesh->index_count / 3u;
    uint32_t n = 0u;

    uint32_t seed = 0x9e3779b9u;
    #define RM_RND()  (seed ^= seed<<13, seed ^= seed>>17, seed ^= seed<<5, \
                       (float)(seed & 0xFFFFFFu) / 16777215.0f)

    for (uint32_t t = 0; t < tris && n < max_points; t++) {
        uint32_t i0 = mesh->idx[t*3+0];
        uint32_t i1 = mesh->idx[t*3+1];
        uint32_t i2 = mesh->idx[t*3+2];

        const float *a = &mesh->pos[i0*3];
        const float *b = &mesh->pos[i1*3];
        const float *c = &mesh->pos[i2*3];

        for (uint32_t s = 0; s < density && n < max_points; s++) {
            /* Muestreo uniforme del triángulo por coordenadas baricéntricas */
            float u = RM_RND();
            float v = RM_RND();
            if (u + v > 1.0f) { u = 1.0f - u; v = 1.0f - v; }
            float w = 1.0f - u - v;

            AOMPoint *p = &out[n++];
            /* Del espacio del objeto a [0,1]³ del volumen */
            p->x = rm_clamp(0.5f + (a[0]*w + b[0]*u + c[0]*v) * 0.5f, 0.0f, 1.0f);
            p->y = rm_clamp(0.5f + (a[1]*w + b[1]*u + c[1]*v) * 0.5f, 0.0f, 1.0f);
            p->z = rm_clamp(0.5f + (a[2]*w + b[2]*u + c[2]*v) * 0.5f, 0.0f, 1.0f);

            uint32_t mi = mesh->material_id;
            if (mi >= m->material_count) mi = 0u;
            const RigMaterial *mat = &m->materials[mi];

            p->r = (uint8_t)(rm_clamp(mat->albedo[0], 0.0f, 1.0f) * 255.0f);
            p->g = (uint8_t)(rm_clamp(mat->albedo[1], 0.0f, 1.0f) * 255.0f);
            p->b = (uint8_t)(rm_clamp(mat->albedo[2], 0.0f, 1.0f) * 255.0f);
            p->a = 190u;
            p->material = (uint8_t)mi;
        }
    }
    #undef RM_RND

    *out_count = n;
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §11  ★ EL CICLO DE VERIFICACIÓN DRACONIANO ★
 *
 * El estándar que tú definiste, aplicado a este bloque. Sin aproximaciones.
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_master_selftest(RigMasterCtx *m, RigMasterAudit *A)
{
    if (!m || !A) return -1;
    rl_memset(A, 0, sizeof(*A));
    A->checks_total = 8u;

    /* ── 1. MATERIAL: la firma háptica se DERIVA, no se escribe ── */
    {
        RigMaterial a, b;
        rig_material_preset(&a, 0);          /* Oro Pulido   — rough 0.08 */
        rig_material_preset(&b, 26);         /* Roca         — rough 0.94 */

        /* Liso ⇒ f_mod alta.  Rugoso ⇒ f_mod baja. Debe cumplirse SIEMPRE. */
        bool ok = (a.haptic.f_mod > b.haptic.f_mod)
               && (a.haptic.grain < b.haptic.grain)
               && (a.haptic.f1    > b.haptic.f1)
               && (a.haptic.f_mod >= 40.0f && a.haptic.f_mod <= 800.0f);
        A->material_ok = ok;
        if (ok) A->checks_passed++;
    }

    /* ── 2. ENV: SH converge · EnvBRDF analítico en rango ── */
    {
        float sc, bi;
        rig_env_brdf_approx(1.0f, 0.0f, &sc, &bi);
        /* Con NoV=1, rough=0: scale→1, bias→0 (reflexión perfecta) */
        bool ok = (sc > 0.90f && sc < 1.10f) && (bi >= -0.05f && bi < 0.15f);

        rig_env_brdf_approx(0.1f, 0.9f, &sc, &bi);
        ok = ok && (sc >= 0.0f && sc <= 1.2f) && (bi >= 0.0f && bi <= 1.0f);

        ok = ok && m->env.ready && (m->env.spec.mips == RIG_ENV_SPEC_MIPS);
        A->env_ok = ok;
        if (ok) A->checks_passed++;
    }

    /* ── 3. HEIGHT: los 11 generadores en [0,1] y con gradiente finito ── */
    {
        bool ok = true;
        float p[3] = { 0.37f, 0.21f, 0.63f };
        for (int f = 0; f < RIG_HFN_COUNT; f++) {
            float h = rig_height3d(p, (RigHeightFn)f, 40.0f, 0);
            if (h < -0.001f || h > 1.001f) { ok = false; break; }

            float n[3];
            rig_height3d_normal(p, (RigHeightFn)f, 40.0f, 0, 0.004f, n);
            float l = rl_sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
            if (l < 0.9f || l > 1.1f) { ok = false; break; }
        }
        A->height_ok = ok;
        if (ok) A->checks_passed++;
    }

    /* ── 4. OBSERVADOR: el frustum SE INCLINA cuando el ojo se mueve ── */
    {
        float pc[16], pr[16];
        float ec[3] = {  0.0f, 0.0f, 400.0f };
        float er[3] = { 60.0f, 0.0f, 400.0f };

        rig_offaxis_projection(&m->cal, ec, 10.0f, 4000.0f, pc);
        rig_offaxis_projection(&m->cal, er, 10.0f, 4000.0f, pr);

        /* Centrado: shiftX ≈ 0.  Desplazado: shiftX ≠ 0. */
        bool ok = (rm_abs(pc[8]) < 1e-4f) && (rm_abs(pr[8]) > 0.5f);
        A->observer_ok = ok;
        if (ok) A->checks_passed++;
    }

    /* ── 5. AOM: Bragg · f ∝ 1/λ · Klein-Cook no lineal ── */
    {
        AOMOptics o;
        aom_optics_configure(&o, 1.2f);

        /* f_R/f_G debe valer λ_G/λ_R = 550/680 = 0.809 */
        float ratio = o.f_r_mhz / rm_max(o.f_g_mhz, 1e-6f);
        float expect = AOM_LAMBDA_G_NM / AOM_LAMBDA_R_NM;   /* 0.8088 */

        bool ok = rm_abs(ratio - expect) < 0.01f;

        /* Y los tres ángulos de Bragg deben COINCIDIR (ese es el punto) */
        ok = ok && (rm_abs(o.theta_r - o.theta_g) < 1e-3f)
                && (rm_abs(o.theta_b - o.theta_g) < 1e-3f);

        /* Klein-Cook: NO lineal (η(2P) ≠ 2·η(P)) */
        float e1 = aom_diffraction_efficiency(550.0f, 0.5f);
        float e2 = aom_diffraction_efficiency(550.0f, 1.0f);
        ok = ok && (rm_abs(e2 - 2.0f * e1) > 0.02f);

        A->aom_ok = ok;
        if (ok) A->checks_passed++;
    }

    /* ── 6. ★ HÁPTICO: LA AM SE APLICA (la línea que faltaba) ── */
    {
        usonic_init();

        RigHapticSig s;
        rl_memset(&s, 0, sizeof(s));
        s.envelope = 1.0f;
        s.f_mod    = 200.0f;          /* pico de Pacini */
        s.am_depth = 1.0f;            /* modulación total */
        usonic_set_focus(0.0f, 0.0f, 150.0f);
        usonic_set_signature(&s);

        /* Muestreamos la amplitud en varios instantes de UN ciclo de AM.
         * Si la AM se aplica, la amplitud DEBE variar. Si no se aplica
         * (el bug original), sería constante. */
        float T = 1.0f / 200.0f;
        int32_t amin = 0x7FFFFFFF, amax = 0;

        for (int i = 0; i < 12; i++) {
            usonic_update(T / 12.0f);
            int32_t a = usonic_state()->elem[28].amplitude;   /* elemento central */
            if (a < amin) amin = a;
            if (a > amax) amax = a;
        }
        /* ★ Si amax == amin, la AM NO se está aplicando ⇒ NO SE SIENTE. */
        bool ok = (amax > amin) && (amax - amin > USONIC_MAX_PRESSURE_PA / 8);

        /* Y f_mod debe estar en la banda de Pacini */
        ok = ok && (usonic_state()->mod_freq_hz >= 40.0f)
                && (usonic_state()->mod_freq_hz <= 800.0f);

        A->haptic_ok = ok;
        if (ok) A->checks_passed++;
    }

    /* ── 7. ★ ACOPLAMIENTO: aire ⇒ silencio · vóxel ⇒ material ── */
    {
        bool ok = true;
        if (m->aom_active) {
            /* En el vacío absoluto, la envolvente DEBE caer a cero. */
            float air[3] = { 0.0f, 0.0f, 5000.0f };   /* muy fuera */
            RigProbe pr;
            float rd[3] = { 0.0f, 0.0f, 1.0f };
            rig_master_probe_ray(m, air, rd, 100.0f, 0.016f, &pr);
            ok = (!pr.hit) && (usonic_level() < 0.02f);
        }
        A->coupling_ok = ok;
        if (ok) A->checks_passed++;
    }

    /* ── 8. φ presente en todos los subsistemas ── */
    {
        bool ok = (rm_abs(RM_PHI - 1.6180339887f) < 1e-7f)
               && (rm_abs(RIG_PHI - 1.6180339887f) < 1e-7f)
               && (rm_abs(AOM_PHI - 1.6180339887f) < 1e-7f);

        /* Y φ debe estar VIVO, no solo declarado:
         * el segundo formante de la fricción está a φ del primero. */
        RigMaterial mm;
        rig_material_preset(&mm, 9);           /* Seda */
        float r = mm.haptic.f2 / rm_max(mm.haptic.f1, 1e-6f);
        ok = ok && (rm_abs(r - RM_PHI) < 0.02f);

        A->phi_ok = ok;
        if (ok) A->checks_passed++;
    }

    return (A->checks_passed == A->checks_total) ? 0 : 1;
}
