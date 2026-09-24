/* ═══════════════════════════════════════════════════════════════════════════
 * rig_face_recon_bridge.c — PUENTE SOBERANO
 *
 * Cierra el hueco arquitectónico del ecosistema RigFace:
 *
 *   rig_face_sovereign_vision.c  →  RigSovReconParams (15 medidos)
 *                                        ↓  [ESTE MÓDULO]
 *   rig_face_engine.c            ←  RigFaceParams     (31 completos)
 *
 * El suite sovereign MIDE una cara real (68 landmarks, antropometría).
 * El engine GENERA una cara desde 31 parámetros (φ, bioquímica, arquetipos).
 * Hasta ahora no se hablaban. Este módulo los une.
 *
 * Tres operaciones:
 *   1. MAPEO      — los 14 medidos → sus homólogos en los 31
 *   2. INFERENCIA — los 17 restantes, derivados de antropometría y de φ
 *   3. INVERSIÓN  — melanina/hemoglobina/caroteno resueltos DESDE el color
 *                   de piel observado, invirtiendo skin_bio_albedo()
 *
 * Cero dependencias externas. C11 puro.
 * ═══════════════════════════════════════════════════════════════════════════ */
#include "rig_face_sovereign.h"
#include "rig_face_engine.h"
/* [CANON] <math.h> → "rig_math.h" */
#include "rig_math.h"
/* [CANON] <string.h> → "rig_noext_str.h" */
#include "rig_noext_str.h"
#define RB_PHI      1.6180339887498948f
#define RB_PHI_INV  0.6180339887498948f
static float rb_clamp(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
static float rb_lerp(float a, float b, float t)  { return a + (b - a) * t; }
/* ───────────────────────────────────────────────────────────────────────────
 * 1. INVERSIÓN BIOQUÍMICA
 *
 * skin_bio_albedo() (rig_face_ng_skin.c) hace, en orden:
 *     c  = base
 *     c *= (1 - (0.55, 0.38, 0.72) * mel_eu)
 *     c *= (1 - (0.05, 0.20, 0.60) * mel_ph)
 *     c += (0.22, 0.04, 0.02) * hb_oxy
 *     c += (0.02, 0.04, 0.12) * hb_deoxy
 *     c += (0.15, 0.12,-0.02) * bilirubin
 *     c += (0.18, 0.12, 0.00) * carotene
 *
 * Invertimos aprovechando la ortogonalidad espectral de los cromóforos:
 *
 *   · La EUMELANINA absorbe sobre todo en azul (0.72) y menos en rojo (0.55).
 *     ⇒ La luminancia total cae. Es el estimador dominante.
 *
 *   · La OXIHEMOGLOBINA suma casi solo en rojo (0.22 vs 0.04/0.02).
 *     ⇒ Se lee en el exceso de rojo tras descontar la melanina.
 *
 *   · El CAROTENO suma en rojo+verde y NADA en azul (0.18, 0.12, 0.00).
 *     ⇒ Se lee en el eje amarillo, ortogonal a la hemoglobina.
 * ─────────────────────────────────────────────────────────────────────────── */
/* Reflectancia de referencia de dermis sin cromóforos (rig_face_ng_skin.c) */
static const float RB_BASE[3] = { 0.87f, 0.775f, 0.70f };
int rig_face_invert_chromophores(const float skin_rgb[3],
                                 float *melanin_out,
                                 float *hemoglobin_out,
                                 float *carotene_out)
{
    float r, g, b, lum, base_lum, mel, rr, gg, bb;
    float red_excess, yellow_excess, hem, car;
    if (!skin_rgb) return RIG_SOV_EINVAL;
    r = rb_clamp(skin_rgb[0], 0.02f, 1.0f);
    g = rb_clamp(skin_rgb[1], 0.02f, 1.0f);
    b = rb_clamp(skin_rgb[2], 0.02f, 1.0f);
    /* ── EUMELANINA ────────────────────────────────────────────────────────
     * La absorción melánica es el término multiplicativo dominante.
     * Resolvemos por el canal AZUL, donde su coeficiente es máximo (0.72)
     * y la contaminación por hemoglobina es mínima (0.02).
     *
     *     b ≈ BASE_b · (1 - 0.72·mel)  ⇒  mel ≈ (1 - b/BASE_b) / 0.72
     */
    mel = (1.0f - b / RB_BASE[2]) / 0.72f;
    mel = rb_clamp(mel, 0.0f, 1.0f);
    /* Reconstruimos la piel que TENDRÍA ese sujeto solo con melanina */
    rr = RB_BASE[0] * (1.0f - 0.55f * mel);
    gg = RB_BASE[1] * (1.0f - 0.38f * mel);
    bb = RB_BASE[2] * (1.0f - 0.72f * mel);
    /* ── OXIHEMOGLOBINA ───────────────────────────────────────────────────
     * Lo que sobra de rojo respecto a la piel puramente melánica.
     * Coeficiente en rojo: 0.22 ⇒ hem = Δr / 0.22
     */
    red_excess = r - rr;
    hem = red_excess / 0.22f;
    hem = rb_clamp(hem, 0.0f, 1.0f);
    /* ── CAROTENO ─────────────────────────────────────────────────────────
     * Eje amarillo: suma en R y G, NADA en B. Ortogonal a la hemoglobina,
     * que suma casi solo en R. Descontamos primero la contribución de hem
     * al verde (0.04) y leemos el residuo en el canal verde (coef. 0.12).
     */
    yellow_excess = (g - gg) - 0.04f * hem;
    car = yellow_excess / 0.12f;
    car = rb_clamp(car, 0.0f, 0.6f);
    /* Coherencia fisiológica: piel muy oscura ⇒ hemoglobina menos visible */
    hem *= (1.0f - mel * 0.35f);
    hem = rb_clamp(hem, 0.05f, 1.0f);
    lum      = 0.2126f * r  + 0.7152f * g  + 0.0722f * b;
    base_lum = 0.2126f * rr + 0.7152f * gg + 0.0722f * bb;
    (void) lum; (void) base_lum;
    if (melanin_out)    *melanin_out    = mel;
    if (hemoglobin_out) *hemoglobin_out = hem;
    if (carotene_out)   *carotene_out   = car;
    return RIG_SOV_OK;
}
/* ───────────────────────────────────────────────────────────────────────────
 * 2. ESTIMACIÓN DE EDAD
 *
 * Dos señales independientes, fusionadas:
 *
 *   a) DENSIDAD DE ARRUGAS — energía de gradiente en las regiones donde el
 *      colágeno cede primero: frente, patas de gallo, surco nasolabial.
 *      Se normaliza contra el gradiente medio del rostro para ser invariante
 *      a la iluminación y al enfoque.
 *
 *   b) PROPORCIÓN DE TERCIOS — con la edad el tercio inferior se acorta
 *      (reabsorción ósea alveolar) y el superior se alarga (recesión
 *      capilar). El ratio lt/ut cae monótonamente.
 * ─────────────────────────────────────────────────────────────────────────── */
int rig_face_estimate_age(float wrinkle_density,
                          float third_ratio,
                          float *age_out)
{
    float a_wrinkle, a_thirds, age;
    if (!age_out) return RIG_SOV_EINVAL;
    /* Curva de arrugas: plana hasta ~25a, luego crece casi lineal */
    a_wrinkle = rb_clamp((wrinkle_density - 0.18f) / 0.55f, 0.0f, 1.0f);
    a_wrinkle = powf(a_wrinkle, RB_PHI_INV);      /* γ = 1/φ */
    /* Tercios: lt/ut ≈ 1.0 en el joven, ≈ 0.80 en el anciano */
    a_thirds = rb_clamp((1.02f - third_ratio) / 0.24f, 0.0f, 1.0f);
    /* Fusión φ-ponderada: la arruga es el testigo más fiable */
    age = a_wrinkle * RB_PHI_INV + a_thirds * (1.0f - RB_PHI_INV);
    *age_out = rb_clamp(age, 0.0f, 1.0f);
    return RIG_SOV_OK;
}
/* ───────────────────────────────────────────────────────────────────────────
 * 3. DIMORFISMO SEXUAL
 *
 * Índice mandíbulo-cigomático: jaw_width / zygomatic_width.
 *   · Masculino: mandíbula ancha y cuadrada  ⇒ ratio alto  (≈ 0.88)
 *   · Femenino:  mandíbula estrecha, cónica  ⇒ ratio bajo  (≈ 0.74)
 *
 * Se corrige con dos rasgos secundarios:
 *   · brow_protrusion — la cresta supraorbitaria es dimórfica
 *   · jaw_taper       — el afilamiento del mentón
 * ─────────────────────────────────────────────────────────────────────────── */
int rig_face_estimate_gender(float jaw_zygo_ratio,
                             float brow_protrusion,
                             float jaw_taper,
                             float *gender_out)
{
    float g_jaw, g_brow, g_taper, g;
    if (!gender_out) return RIG_SOV_EINVAL;
    g_jaw   = rb_clamp((jaw_zygo_ratio - 0.72f) / 0.20f, 0.0f, 1.0f);
    g_brow  = rb_clamp((brow_protrusion - 0.10f) / 0.16f, 0.0f, 1.0f);
    g_taper = rb_clamp((jaw_taper - 0.60f) / 0.28f, 0.0f, 1.0f);
    /* Pesos: la mandíbula domina, la ceja confirma, el afilamiento matiza */
    g = g_jaw * 0.52f + g_brow * 0.30f + g_taper * 0.18f;
    *gender_out = rb_clamp(g, 0.0f, 1.0f);
    return RIG_SOV_OK;
}
/* ───────────────────────────────────────────────────────────────────────────
 * 4. EL PUENTE
 *
 * RigSovReconParams (15 medidos por visión) → RigFaceParams (31 del engine)
 *
 * Los 15 medidos se normalizan contra face_width (unidad craneal), porque el
 * engine trabaja en unidades normalizadas, no en metros.
 *
 * Los 16 restantes se INFIEREN. No se inventan: se derivan de la antropometría
 * medida y de las razones áureas que el sistema ya usa como canon.
 * ─────────────────────────────────────────────────────────────────────────── */
int rig_face_params_from_recon(const RigSovReconParams *rec,
                               const float skin_rgb[3],
                               float wrinkle_density,
                               RigFaceParams *out)
{
    float U;                    /* unidad craneal: face_width medido */
    float fw, fh, dp;
    float ut, mt, lt, third_ratio;
    float zygo_w, jaw_zygo;
    float mel, hem, car, age, gen;
    int rc;
    if (!rec || !out) return RIG_SOV_EINVAL;
    if (!(rec->face_width > 1e-6f)) return RIG_SOV_ERANGE;
    memset(out, 0, sizeof(*out));
    /* ── Normalización a unidad craneal ───────────────────────────────── */
    U  = rec->face_width;
    fw = 1.0f;                              /* por definición */
    fh = rec->face_height / U;
    dp = rec->depth       / U;
    /* ═══ MAPEO DIRECTO — los 14 que la visión SÍ mide ═══════════════════ */
    out->cranium_width  = fw;
    out->cranium_height = rb_clamp(fh, 0.85f, 1.45f);
    out->cranium_depth  = rb_clamp(dp, 0.70f, 1.15f);
    out->interocular_dist    = rb_clamp(rec->eye_distance / U, 0.22f, 0.42f);
    out->eye_width           = rb_clamp(rec->eye_width    / U, 0.20f, 0.42f);
    out->nose_length         = rb_clamp(rec->nose_length  / U, 0.28f, 0.60f);
    out->nose_width          = rb_clamp(rec->nose_width   / U, 0.20f, 0.46f);
    out->mouth_width         = rb_clamp(rec->mouth_width  / U, 0.34f, 0.60f);
    out->lip_thickness_upper = rb_clamp(rec->upper_lip    / U, 0.10f, 0.32f);
    out->lip_thickness_lower = rb_clamp(rec->lower_lip    / U, 0.12f, 0.36f);
    out->jaw_width           = rb_clamp(rec->jaw_width    / U, 0.66f, 1.05f);
    /* brow_height mide la distancia ceja↔párpado. Es proxy inverso de la
     * protrusión supraorbitaria: ceja baja y pegada ⇒ cresta prominente. */
    out->brow_protrusion = rb_clamp(0.32f - rec->brow_height / U * 0.9f,
                                    0.04f, 0.30f);
    /* jaw_taper ∈ [0,1] (ancho a media altura / ancho superior).
     * Mandíbula cuadrada ⇒ taper alto ⇒ ángulo goníaco cerrado (≈105°).
     * Mandíbula cónica   ⇒ taper bajo ⇒ ángulo abierto (≈128°). */
    out->jaw_angle = rb_clamp(128.0f - rec->jaw_taper * 24.0f, 102.0f, 130.0f);
    out->chin = rb_clamp(rec->chin_height / U, 0.08f, 0.30f);
    /* ═══ INFERENCIA — los 17 que la visión NO mide ══════════════════════ */
    /* ── Tercios faciales (Leonardo / Vitruvio) ───────────────────────────
     * El canon los quiere iguales (1/3 cada uno). La cara real se desvía.
     * Reconstruimos: el tercio medio va de glabela a subnasale, y contiene
     * la nariz completa. El inferior va de subnasale a mentón. */
    mt = rb_clamp(rec->nose_length / (rec->face_height + 1e-6f) * 1.05f,
                  0.26f, 0.42f);
    lt = rb_clamp((rec->chin_height + rec->upper_lip + rec->lower_lip)
                  / (rec->face_height + 1e-6f) * 1.30f, 0.26f, 0.42f);
    ut = rb_clamp(1.0f - mt - lt, 0.24f, 0.44f);
    /* Renormalizar para que sumen exactamente 1 */
    { float s = ut + mt + lt; ut /= s; mt /= s; lt /= s; }
    out->upper_third = ut;
    out->mid_third   = mt;
    out->lower_third = lt;
    third_ratio = lt / (ut + 1e-6f);
    /* ── Profundidad orbital ──────────────────────────────────────────────
     * No es medible en 2D directamente. Correlaciona con la protrusión de
     * la ceja: cuanto más sobresale la cresta, más hundido queda el globo. */
    out->orbital_depth = rb_clamp(0.12f + out->brow_protrusion * 0.85f,
                                  0.10f, 0.34f);
    /* ── Nariz: proyección de la punta y ancho del puente ─────────────────
     * La proyección sagital no se ve de frente. Canon: en el perfil ideal,
     * la punta proyecta ≈ nose_length / φ² desde el plano facial. */
    out->nasal_tip_proj = rb_clamp(out->nose_length / (RB_PHI * RB_PHI),
                                   0.12f, 0.34f);
    /* El puente es más estrecho que el ala: razón áurea. */
    out->nasal_bridge_width = rb_clamp(out->nose_width * RB_PHI_INV,
                                       0.12f, 0.30f);
    /* ── Mentón: proyección sagital ───────────────────────────────────────
     * Correlaciona con la altura del mentón y con el dimorfismo. */
    out->chin_projection = rb_clamp(out->chin * 0.85f + 0.06f, 0.06f, 0.32f);
    /* ── Complejo cigomático ──────────────────────────────────────────────
     * El ancho de los pómulos es el ancho facial máximo: por construcción
     * del bbox, ≈ face_width. La altura del pómulo cae a la mitad del
     * tercio medio. */
    zygo_w = rb_clamp(fw * 1.02f, 0.88f, 1.16f);
    out->zygomatic_width  = zygo_w;
    out->zygomatic_height = rb_clamp(mt * fh * 1.30f, 0.34f, 0.56f);
    /* ── Cuello: no está en el encuadre facial. Canon antropométrico. ──── */
    out->neck_width  = rb_clamp(out->jaw_width * 0.46f, 0.30f, 0.50f);
    out->neck_length = rb_clamp(fh * 0.44f, 0.38f, 0.60f);
    /* ── BIOQUÍMICA: invertida desde el color de piel observado ─────────── */
    if (skin_rgb) {
        rc = rig_face_invert_chromophores(skin_rgb, &mel, &hem, &car);
        if (rc) return rc;
    } else {
        mel = 0.25f; hem = 0.45f; car = 0.10f;   /* neutro */
    }
    out->melanin    = mel;
    out->hemoglobin = hem;
    out->carotene   = car;
    /* ── EDAD: arrugas + tercios ──────────────────────────────────────── */
    rc = rig_face_estimate_age(wrinkle_density, third_ratio, &age);
    if (rc) return rc;
    out->age_factor = age;
    /* ── DIMORFISMO: índice mandíbulo-cigomático ──────────────────────── */
    jaw_zygo = out->jaw_width / (zygo_w + 1e-6f);
    rc = rig_face_estimate_gender(jaw_zygo, out->brow_protrusion,
                                  rec->jaw_taper, &gen);
    if (rc) return rc;
    out->gender_factor = gen;
    /* ── φ del sujeto: su razón facial real ───────────────────────────── */
    out->phi_ratio = rb_clamp(fh / fw, 1.25f, 1.75f);
    return RIG_SOV_OK;
}
/* ───────────────────────────────────────────────────────────────────────────
 * 5. ARQUETIPO MÁS CERCANO
 *
 * Dados los 31 parámetros de un sujeto real, ¿a cuál de los 33 arquetipos
 * canónicos se parece más?
 *
 * Distancia euclídea ponderada. Los pesos no son uniformes: los rasgos que
 * el ojo humano usa para reconocer una identidad (nariz, ojos, mandíbula)
 * pesan más que los que apenas registra (cuello, caroteno).
 * ─────────────────────────────────────────────────────────────────────────── */
static const float RB_W[31] = {
    /* cw    ch    cd    phi   ut    mt    lt   */
       1.2f, 1.2f, 0.8f, 1.6f, 0.9f, 0.9f, 0.9f,
    /* iod   ew    od    bp                     */
       1.8f, 1.6f, 1.0f, 1.1f,
    /* nl    nw    ntp   nbw                    */
       1.7f, 1.6f, 1.2f, 0.9f,
    /* mw    ltu   ltl   jw    ja    cp    chin */
       1.5f, 1.3f, 1.3f, 1.7f, 1.2f, 1.0f, 0.9f,
    /* zw    zh                                 */
       1.4f, 1.0f,
    /* nkw   nkl                                */
       0.3f, 0.3f,
    /* mel   hem   car                          */
       1.5f, 0.6f, 0.4f,
    /* age   gen                                */
       1.3f, 1.1f
};
int rig_face_nearest_archetype(const RigFaceParams *subject,
                               int *archetype_out,
                               float *distance_out)
{
    const float *sp;
    int n, best = -1;
    float best_d = 1e30f;
    if (!subject || !archetype_out) return RIG_SOV_EINVAL;
    sp = (const float*)subject;
    n  = rig_face_archetype_count();
    for (int i = 0; i < n; i++) {
        const RigFaceArchetype *a = rig_face_archetype_get(i);
        const float *ap;
        float d = 0.0f;
        if (!a) continue;
        ap = a->params;
        for (int k = 0; k < 31; k++) {
            float diff;
            /* jaw_angle vive en grados: normalizar antes de comparar */
            if (k == 19) diff = (sp[k] - ap[k]) / 30.0f;
            else         diff = (sp[k] - ap[k]);
            d += RB_W[k] * diff * diff;
        }
        d = sqrtf(d);
        if (d < best_d) { best_d = d; best = i; }
    }
    if (best < 0) return RIG_SOV_ESTATE;
    *archetype_out = best;
    if (distance_out) *distance_out = best_d;
    return RIG_SOV_OK;
}
/* ───────────────────────────────────────────────────────────────────────────
 * 6. PIPELINE COMPLETO — de la imagen al avatar
 *
 *   imagen → detect_landmarks → recon_params → [PUENTE] → RigFaceParams
 *          → rig_face_create → malla con bioquímica NG
 * ─────────────────────────────────────────────────────────────────────────── */
int rig_face_from_image(const RigSovImage *image,
                        unsigned subdiv_level,
                        RigFaceParams *params_out,
                        RigFaceMesh **mesh_out,
                        int *archetype_out)
{
    RigSovLandmarks lm;
    RigSovReconParams rec;
    RigSovMesh scratch;
    RigFaceParams p;
    float skin_rgb[3];
    float wrinkle;
    int rc, arch;
    if (!image || !params_out) return RIG_SOV_EINVAL;
    rig_sov_mesh_init(&scratch);
    /* 1. Visión: 68 landmarks + antropometría */
    rc = rig_sov_reconstruct_face(image, &rec, &lm, &scratch);
    rig_sov_mesh_free(&scratch);
    if (rc) return rc;
    /* 2. Color de piel: muestreo de la mejilla (bajo el pómulo, lejos de
     *    ojos, boca y sombras nasales). Landmarks 2..4 y 12..14. */
    rc = rig_face_sample_skin_color(image, &lm, skin_rgb);
    if (rc) return rc;
    /* 3. Densidad de arrugas: energía de gradiente normalizada en frente,
     *    patas de gallo y surco nasolabial. */
    rc = rig_face_measure_wrinkles(image, &lm, &wrinkle);
    if (rc) return rc;
    /* 4. EL PUENTE: 15 medidos → 31 completos */
    rc = rig_face_params_from_recon(&rec, skin_rgb, wrinkle, &p);
    if (rc) return rc;
    /* 5. Arquetipo más cercano de los 33 canónicos */
    rc = rig_face_nearest_archetype(&p, &arch, NULL);
    if (rc) arch = -1;
    *params_out = p;
    if (archetype_out) *archetype_out = arch;
    /* 6. Generar la malla con el engine soberano */
    if (mesh_out) {
        *mesh_out = rig_face_create(&p, subdiv_level);
        if (!*mesh_out) return RIG_SOV_ENOMEM;
    }
    return RIG_SOV_OK;
}