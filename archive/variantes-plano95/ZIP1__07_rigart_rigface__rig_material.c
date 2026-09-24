/* ═══════════════════════════════════════════════════════════════════════════
 * rig_material.c — DERIVACIÓN HÁPTICA + PRESETS · RIGCOM MASTER
 *
 * Aquí vive el puente que no existía:
 *
 *      BRDF  →  TACTO
 *
 * La rugosidad microfacética no solo dice cómo se dispersa la luz.
 * Dice también con qué frecuencia tropieza el dedo al deslizar, qué
 * formantes tiene el sonido de fricción, y cuánto agarra la piel.
 *
 * Todo sale de la misma física de la superficie. Un solo número —roughness—
 * gobierna tres sentidos. Eso es lo que hace que el material sea REAL.
 *
 * C11 · cero libc · φ
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rig_material.h"

/* ── RigLibC soberano ── */
extern float rl_sqrtf(float);
extern float rl_powf(float, float);
extern float rl_expf(float);
extern float rl_logf(float);
extern float rl_sinf(float);
extern float rl_cosf(float);
extern void *rl_memset(void *, int, unsigned long);
extern void *rl_memcpy(void *, const void *, unsigned long);

static float rm_clamp(float x, float a, float b){ return x<a?a:(x>b?b:x); }
static float rm_max(float a, float b){ return a>b?a:b; }
static float rm_lum(const float c[3]){
    return 0.2126f*c[0] + 0.7152f*c[1] + 0.0722f*c[2];
}
static void rm_str(char *dst, const char *src, int cap){
    int i = 0;
    while (src[i] && i < cap-1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §1  PROPIEDADES TÁCTILES DE CADA HEIGHT FIELD
 *
 * Cada función de relieve tiene un "grano" intrínseco que el dedo siente,
 * INDEPENDIENTE de la rugosidad óptica.
 *
 * Un espejo moleteado es ópticamente liso (rough 0.05) y táctilmente áspero.
 * El BRDF solo no basta: hay que preguntarle al relieve.
 * ═══════════════════════════════════════════════════════════════════════════ */

float rig_hfn_tactile_noise(RigHeightFn fn)
{
    switch (fn) {
    case RIG_HFN_NONE:      return 0.00f;   /* cristal pulido               */
    case RIG_HFN_PORES:     return 0.35f;   /* piel, cuero — grano fino     */
    case RIG_HFN_WEAVE:     return 0.28f;   /* trama regular → poco ruido   */
    case RIG_HFN_FBM:       return 0.85f;   /* roca — caos total            */
    case RIG_HFN_SCALES:    return 0.55f;   /* escamas — bordes duros       */
    case RIG_HFN_BRUSHED:   return 0.22f;   /* surcos paralelos → direccional */
    case RIG_HFN_HAMMERED:  return 0.42f;   /* ondulado suave               */
    case RIG_HFN_IMPASTO:   return 0.70f;   /* pincelada — irregular        */
    case RIG_HFN_KNURL:     return 0.62f;   /* moleteado — muy tangible     */
    case RIG_HFN_CRACKLE:   return 0.50f;   /* grietas — filos              */
    case RIG_HFN_FIBER:     return 0.18f;   /* terciopelo — suave, denso    */
    default:                return 0.30f;
    }
}

/* Frecuencia espacial característica: ciclos por milímetro.
 * Al deslizar el dedo a velocidad v (mm/s), la frecuencia temporal que
 * excita a los Pacini es:   f_táctil = v · spatial_freq
 *
 * Un dedo va a ~100 mm/s. Poros a 3 ciclos/mm ⇒ 300 Hz. Justo en el pico
 * de Pacini (250 Hz). Por eso la piel se siente tan "presente". */
float rig_hfn_spatial_freq(RigHeightFn fn)
{
    switch (fn) {
    case RIG_HFN_NONE:      return 0.0f;
    case RIG_HFN_PORES:     return 3.0f;    /* poros ~0.3 mm                */
    case RIG_HFN_WEAVE:     return 5.5f;    /* hilos ~0.18 mm — muy fino    */
    case RIG_HFN_FBM:       return 0.7f;    /* roca — grano grueso          */
    case RIG_HFN_SCALES:    return 1.2f;
    case RIG_HFN_BRUSHED:   return 8.0f;    /* surcos micrométricos         */
    case RIG_HFN_HAMMERED:  return 0.35f;   /* hoyos de ~3 mm               */
    case RIG_HFN_IMPASTO:   return 1.0f;
    case RIG_HFN_KNURL:     return 2.2f;
    case RIG_HFN_CRACKLE:   return 1.6f;
    case RIG_HFN_FIBER:     return 12.0f;   /* fibras ~0.08 mm — susurro    */
    default:                return 2.0f;
    }
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §2  ★ LA DERIVACIÓN ★  —  BRDF → TACTO
 *
 * Esta función es el corazón de la innovación. Convierte un material óptico
 * en una experiencia táctil y sonora, sin catálogo aparte, sin datos a mano.
 * ═══════════════════════════════════════════════════════════════════════════ */

int rig_material_derive_haptics(RigMaterial *m)
{
    if (!m) return -1;

    RigHapticSig *h = &m->haptic;

    const float r      = rm_clamp(m->roughness, 0.0f, 1.0f);
    const float metal  = rm_clamp(m->metallic,  0.0f, 1.0f);
    const float trans  = rm_clamp(m->transmission, 0.0f, 1.0f);
    const float sheenL = rm_clamp(rm_lum(m->sheen), 0.0f, 1.0f);
    const float coat   = rm_clamp(m->clearcoat, 0.0f, 1.0f);

    const float grain_noise = rig_hfn_tactile_noise(m->height_fn);
    const float grain_freq  = rig_hfn_spatial_freq(m->height_fn);
    const float relief      = rm_clamp(m->height_scale * 4.0f, 0.0f, 1.0f);

    /* ── GRANO: la aspereza espacial real que siente el dedo ───────────────
     * Es la fusión de la rugosidad óptica y del relieve geométrico.
     * Pesada hacia el relieve, porque el dedo siente la GEOMETRÍA,
     * no la microfaceta. */
    h->grain = rm_clamp(grain_noise * relief * RIG_PHI_INV
                        + r * (1.0f - RIG_PHI_INV) * 0.55f,
                        0.0f, 1.0f);

    /* ── f_mod: LA FRECUENCIA DE MODULACIÓN AM ────────────────────────────
     * Sin esto, el ultrasonido de 40 kHz es INVISIBLE para la piel.
     * Lo que el dedo siente es la envolvente.
     *
     * Superficie lisa  → deslizamiento continuo y rápido → alta frecuencia
     * Superficie rugosa→ tropiezos lentos e irregulares  → baja frecuencia
     *
     * Rango: [50, 320] Hz. Centrado en la banda de Pacini (pico 250 Hz).
     *
     * Y se corrige con la frecuencia espacial del relieve: una trama muy
     * fina (seda, 5.5 ciclos/mm) hace vibrar más rápido que un poro grueso. */
    float f_base = 320.0f - r * 250.0f;                 /* 320 → 70 Hz      */
    float f_grain = (grain_freq > 0.01f)
                  ? rm_clamp(grain_freq * 42.0f, 30.0f, 420.0f)  /* v≈100mm/s */
                  : f_base;
    h->f_mod = rm_clamp(f_base * 0.55f + f_grain * 0.45f, 40.0f, 420.0f);

    /* ── AM depth: cuánto se modula ───────────────────────────────────────
     * Liso = superficie continua = poca variación = AM superficial.
     * Rugoso = tropiezos = AM profunda, casi al 100%. */
    h->am_depth = rm_clamp(0.10f + h->grain * 0.82f, 0.05f, 0.95f);

    /* ── DUREZA (Merkel: presión estática) ────────────────────────────────
     * El metal es duro. El terciopelo cede. El vidrio es durísimo.
     * El sheen (fibras que se doblan) resta dureza. */
    h->hardness = rm_clamp(
          metal  * 0.42f                    /* conductor = rígido           */
        + coat   * 0.18f                    /* laca dura sobre la base      */
        + (1.0f - r) * 0.22f                /* pulido ⇒ sustrato compacto   */
        + (1.0f - trans) * 0.10f
        + 0.18f                             /* base                          */
        - sheenL * 0.55f                    /* ★ fibra ⇒ BLANDO             */
        , 0.02f, 1.0f);

    /* ── ENVOLVENTE: presión base del foco ultrasónico ────────────────────
     * Duro ⇒ presión alta y sostenida. Blando ⇒ caricia. */
    h->envelope = rm_clamp(0.42f + h->hardness * 0.50f, 0.15f, 0.98f);

    /* ── STM (Spatiotemporal Modulation) ──────────────────────────────────
     * El estado del arte no usa AM sola: hace ORBITAR el punto focal en un
     * círculo pequeño a alta velocidad. Se siente varias veces más fuerte,
     * porque barre la piel en vez de presionar un punto.
     *
     * Radio: fino para superficies finas, ancho para superficies gruesas.
     * Órbita: en la banda de Meissner-Pacini. */
    h->stm_radius = rm_clamp(2.0f + h->grain * 6.0f, 1.5f, 9.0f);  /* mm   */
    h->stm_freq   = rm_clamp(180.0f - h->grain * 90.0f, 70.0f, 220.0f);

    /* ── SONIDO DE FRICCIÓN: los formantes ────────────────────────────────
     * Rozar seda suena AGUDO (susurro de alta frecuencia).
     * Rozar roca suena GRAVE (rugido de baja frecuencia).
     *
     * El "brillo" del sonido cae con la rugosidad. El segundo formante
     * está a φ del primero — la razón que el oído percibe como natural. */
    float brightness = 1.0f - h->grain;
    h->f1 = rm_clamp(700.0f + brightness * 6200.0f, 500.0f, 7200.0f);
    h->f2 = rm_clamp(h->f1 * RIG_PHI, 700.0f, 11000.0f);

    /* ── Q: la pureza del tono ────────────────────────────────────────────
     * Liso ⇒ resonancia limpia y estrecha (un tono).
     * Rugoso ⇒ banda ancha (un rugido). */
    h->q1 = rm_clamp(1.5f + brightness * 13.0f, 1.2f, 15.0f);
    h->q2 = h->q1 * RIG_PHI_INV;

    /* ── Mezcla de ruido: tono vs siseo ── */
    h->noise_mix = rm_clamp(0.12f + h->grain * 0.80f, 0.08f, 0.95f);

    /* ── FRICCIÓN: la ganancia por velocidad (stick-slip) ─────────────────
     * Si el dedo está quieto → silencio absoluto.
     * Al arrastrar, la amplitud sube con la velocidad × fricción.
     *
     * El metal pulido resbala. El caucho agarra. */
    h->friction = rm_clamp(0.06f + r * 0.62f + h->grain * 0.28f
                           - metal * 0.12f,        /* metal ⇒ resbala       */
                           0.03f, 0.98f);

    /* ── STICK-SLIP: el "chirrido" del agarre-deslizamiento ───────────────
     * Alto en superficies blandas y rugosas (goma, cuero seco).
     * Cero en superficies duras y lisas (hielo, cristal, mercurio). */
    h->stick_slip = rm_clamp(h->friction * (1.0f - h->hardness) * 1.6f
                             + sheenL * 0.30f,
                             0.0f, 1.0f);

    /* ── LRA: PWM del motor de vibración del móvil ────────────────────────
     * La Web Vibration API NO da amplitud. Pero sí da duraciones al ms.
     * Con PWM se modula la intensidad PERCIBIDA:
     *
     *     intensidad ≈ on / (on + off)
     *
     * Y el período (on+off) fija la frecuencia percibida.
     * El LRA del Honor resuena en ~170-230 Hz: justo en el pico de Pacini. */
    {
        float period_ms = 1000.0f / rm_clamp(h->f_mod, 40.0f, 250.0f);
        float duty      = rm_clamp(0.25f + h->am_depth * 0.55f, 0.15f, 0.85f);
        float on        = period_ms * duty;
        float off       = period_ms - on;
        h->lra_on_ms  = (uint16_t)rm_clamp(on  + 0.5f, 1.0f, 60000.0f);
        h->lra_off_ms = (uint16_t)rm_clamp(off + 0.5f, 1.0f, 60000.0f);
    }

    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §3  DEFAULT
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_material_default(RigMaterial *m)
{
    if (!m) return -1;
    rl_memset(m, 0, sizeof(*m));

    m->albedo[0] = m->albedo[1] = m->albedo[2] = 0.8f;
    m->metallic        = 0.0f;
    m->roughness       = 0.5f;
    m->ior             = 1.45f;
    m->anisotropy      = 0.0f;
    m->tangent_flow    = RIG_FLOW_UV;
    m->sheen_roughness = 0.3f;
    m->clearcoat_ior   = 1.5f;
    m->clearcoat_roughness = 0.05f;
    m->iri_ior         = 1.3f;
    m->height_fn       = RIG_HFN_NONE;
    m->height_scale    = 0.0f;
    m->height_freq     = 1.0f;
    m->thickness_mm    = 1.0f;

    rm_str(m->name, "default", 48);
    rig_material_derive_haptics(m);
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §4  PRESETS — el catálogo RigArt, ahora COMPLETO
 *
 * Estos ya no son "albedo + rugosidad". Ahora cada uno declara su
 * anisotropía, su sheen, su transmisión, su clearcoat y su relieve.
 * Y de ahí sale su tacto y su sonido, automáticamente.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *name;
    float alb[3];
    float metallic, roughness, ior;
    float aniso; RigTangentFlow flow;
    float sheen[3], sheen_r;
    float trans, absorb[3], thick, disp;
    float coat, coat_r;
    float iri, iri_nm;
    RigHeightFn hfn; float hscale, hfreq;
    float sss;
} RigPreset;

static const RigPreset RIG_PRESETS[] = {

/* ═══ METALES — sin IBL eran bolas negras. Ahora reflejan el mundo. ═══ */
{ "Oro Pulido",        {1.000f,0.766f,0.336f}, 1,0.08f,0.47f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_NONE,0.00f,1, 0 },

{ "Oro Cepillado",     {1.000f,0.766f,0.336f}, 1,0.32f,0.47f,
  0.80f,RIG_FLOW_FIXED,{0,0,0},0.3f, 0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_BRUSHED,0.02f,60, 0 },          /* ★ anisotropía + surcos       */

{ "Oro Martillado",    {0.98f,0.75f,0.35f},    1,0.28f,0.47f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_HAMMERED,0.35f,3, 0 },          /* ★ relieve macroscópico       */

{ "Plata",             {0.972f,0.960f,0.915f}, 1,0.06f,0.45f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_NONE,0,1, 0 },

{ "Cromo",             {0.550f,0.556f,0.554f}, 1,0.03f,0.35f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_NONE,0,1, 0 },

{ "Titanio Mate",      {0.542f,0.497f,0.449f}, 1,0.62f,0.45f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_FBM,0.04f,40, 0 },              /* multiscatter lo salva         */

{ "Fibra de Carbono",  {0.055f,0.058f,0.065f}, 0.6f,0.38f,1.55f,
  0.72f,RIG_FLOW_UV,  {0,0,0},0.3f,  0,{0,0,0},1,0,  0.6f,0.08f, 0,0,
  RIG_HFN_WEAVE,0.05f,24, 0 },            /* ★ aniso + trama + clearcoat  */

{ "Mokumé-Gané",       {0.72f,0.66f,0.50f},    1,0.24f,0.46f,
  0.55f,RIG_FLOW_CURL,{0,0,0},0.3f,  0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_BRUSHED,0.015f,20, 0 },         /* ★ tangentes curl-noise       */

{ "Mercurio",          {0.78f,0.78f,0.80f},    1,0.02f,0.42f,
  0.0f,RIG_FLOW_CURL, {0,0,0},0.3f,  0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_NONE,0,1, 0 },

/* ═══ TEXTILES — sin sheen no existían. Ahora sí. ═══ */
{ "Seda Charmeuse",    {0.72f,0.66f,0.56f},    0,0.22f,1.47f,
  0.88f,RIG_FLOW_UV,  {0.30f,0.28f,0.24f},0.22f, 0,{0,0,0},1,0, 0,0.05f, 0,0,
  RIG_HFN_WEAVE,0.008f,90, 0.10f },       /* ★ EL caso: aniso + sheen     */

{ "Satén de Seda",     {0.80f,0.74f,0.64f},    0,0.14f,1.47f,
  0.94f,RIG_FLOW_UV,  {0.22f,0.20f,0.17f},0.18f, 0,{0,0,0},1,0, 0,0.05f, 0,0,
  RIG_HFN_WEAVE,0.005f,120, 0.08f },      /* aniso casi total              */

{ "Terciopelo",        {0.28f,0.06f,0.14f},    0,0.90f,1.46f,
  0.0f,RIG_FLOW_UV,   {0.72f,0.55f,0.60f},0.42f, 0,{0,0,0},1,0, 0,0.05f, 0,0,
  RIG_HFN_FIBER,0.06f,140, 0.18f },       /* ★ SHEEN dominante            */

{ "Cachemira",         {0.78f,0.70f,0.62f},    0,0.85f,1.46f,
  0.0f,RIG_FLOW_UV,   {0.62f,0.58f,0.52f},0.55f, 0,{0,0,0},1,0, 0,0.05f, 0,0,
  RIG_HFN_FIBER,0.05f,90, 0.34f },        /* sheen + SSS cálido            */

{ "Lino Fino",         {0.82f,0.76f,0.64f},    0,0.72f,1.47f,
  0.42f,RIG_FLOW_UV,  {0.38f,0.35f,0.30f},0.45f, 0,{0,0,0},1,0, 0,0.05f, 0,0,
  RIG_HFN_WEAVE,0.030f,32, 0.12f },

{ "Brocado de Oro",    {0.62f,0.50f,0.24f},    0.75f,0.42f,1.5f,
  0.60f,RIG_FLOW_UV,  {0.25f,0.22f,0.12f},0.30f, 0,{0,0,0},1,0, 0,0.05f, 0,0,
  RIG_HFN_WEAVE,0.045f,18, 0.05f },

{ "Nubuck",            {0.52f,0.34f,0.22f},    0,0.88f,1.5f,
  0.0f,RIG_FLOW_UV,   {0.48f,0.40f,0.34f},0.60f, 0,{0,0,0},1,0, 0,0.05f, 0,0,
  RIG_HFN_FIBER,0.04f,70, 0.20f },

{ "Cuero Anilina",     {0.42f,0.22f,0.12f},    0,0.58f,1.5f,
  0.0f,RIG_FLOW_UV,   {0.10f,0.08f,0.06f},0.40f, 0,{0,0,0},1,0, 0.15f,0.35f, 0,0,
  RIG_HFN_PORES,0.055f,26, 0.22f },       /* ★ poros ⇒ tacto de cuero     */

/* ═══ TRANSMISIVOS — eran plástico opaco. Ahora transmiten. ═══ */
{ "Diamante",          {1.0f,1.0f,1.0f},       0,0.02f,2.417f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  1.0f,{0.002f,0.002f,0.002f},4,0.90f,
  0,0.05f, 0,0,  RIG_HFN_NONE,0,1, 0 },   /* ★ dispersión = el fuego      */

{ "Hielo",             {0.92f,0.96f,1.0f},     0,0.10f,1.309f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0.92f,{0.03f,0.012f,0.008f},12,0.10f,
  0,0.05f, 0,0,  RIG_HFN_CRACKLE,0.03f,6, 0.30f },

{ "Jade Hetián",       {0.42f,0.72f,0.55f},    0,0.24f,1.66f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0.72f,{0.28f,0.06f,0.20f},8,0,
  0,0.05f, 0,0,  RIG_HFN_NONE,0,1, 0.85f }, /* ★ Beer-Lambert + SSS       */

{ "Ámbar",             {0.86f,0.48f,0.10f},    0,0.18f,1.55f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0.80f,{0.05f,0.22f,0.62f},10,0.05f,
  0,0.05f, 0,0,  RIG_HFN_NONE,0,1, 0.55f },

{ "Cera de Vela",      {0.92f,0.86f,0.72f},    0,0.42f,1.44f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0.35f,{0.10f,0.14f,0.22f},3,0,
  0,0.05f, 0,0,  RIG_HFN_NONE,0.01f,4, 0.95f },

/* ═══ CLEARCOAT — la laca era plástico brillante. Ahora es laca. ═══ */
{ "Laca Japonesa",     {0.10f,0.02f,0.02f},    0,0.32f,1.5f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  1.0f,0.02f, 0,0,
  RIG_HFN_NONE,0,1, 0.10f },              /* ★ 12 capas urushi = coat 1.0 */

{ "Nácar",             {0.88f,0.86f,0.84f},    0.15f,0.16f,1.53f,
  0.35f,RIG_FLOW_SPIRAL,{0,0,0},0.3f, 0.12f,{0,0,0},1,0, 0.85f,0.04f,
  1.0f,420.0f,  RIG_HFN_SCALES,0.012f,30, 0.42f },  /* ★ iri + coat + spiral */

/* ═══ PIEDRA / MINERAL ═══ */
{ "Mármol Calacatta",  {0.92f,0.90f,0.86f},    0,0.28f,1.49f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0.10f,{0.4f,0.4f,0.35f},6,0,
  0.25f,0.10f, 0,0,  RIG_HFN_NONE,0.004f,5, 0.62f },

{ "Obsidiana",         {0.035f,0.033f,0.040f}, 0,0.05f,1.48f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_NONE,0,1, 0 },

{ "Roca Volcánica",    {0.14f,0.13f,0.12f},    0,0.94f,1.5f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_FBM,0.22f,7, 0 },               /* ★ el más áspero al tacto     */

{ "Pátina Verdigris",  {0.30f,0.55f,0.48f},    0.55f,0.72f,1.5f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_CRACKLE,0.06f,12, 0 },

/* ═══ PIEL (el puente con RigFace) ═══ */
{ "Piel Humana",       {0.87f,0.775f,0.70f},   0,0.52f,1.45f,
  0.12f,RIG_FLOW_UV,  {0.06f,0.05f,0.04f},0.5f, 0,{0,0,0},1,0, 0,0.05f, 0,0,
  RIG_HFN_PORES,0.035f,46, 0.72f },       /* ★ poros + vello + SSS        */

/* ═══ VIDRIO / CERÁMICA ═══ */
{ "Cerámica Blanca",   {0.94f,0.94f,0.93f},    0,0.14f,1.5f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  0.7f,0.03f, 0,0,
  RIG_HFN_NONE,0,1, 0.15f },

{ "Cristal Facetado",  {0.98f,0.99f,1.0f},     0,0.04f,1.52f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0.95f,{0.01f,0.01f,0.02f},6,0.30f,
  0,0.05f, 0,0,  RIG_HFN_NONE,0,1, 0 },

/* ═══ IRIDISCENTES ═══ */
{ "Iridiscente",       {0.60f,0.60f,0.66f},    0.6f,0.20f,1.5f,
  0.0f,RIG_FLOW_UV,   {0,0,0},0.3f,  0,{0,0,0},1,0,  0.5f,0.06f,
  1.0f,480.0f,  RIG_HFN_NONE,0,1, 0 },

{ "Aceite Iridiscente",{0.16f,0.14f,0.12f},    0.4f,0.08f,1.47f,
  0.0f,RIG_FLOW_CURL, {0,0,0},0.3f,  0.25f,{0.3f,0.3f,0.3f},0.4f,0,
  1.0f,0.02f,  1.0f,650.0f,  RIG_HFN_NONE,0,1, 0 },

{ "Moleteado Táctil",  {0.62f,0.63f,0.65f},    1,0.34f,0.45f,
  0.30f,RIG_FLOW_FIXED,{0,0,0},0.3f, 0,{0,0,0},1,0,  0,0.05f, 0,0,
  RIG_HFN_KNURL,0.18f,9, 0 },             /* liso al ojo, áspero al dedo   */
};

#define RIG_PRESET_N ((uint32_t)(sizeof(RIG_PRESETS)/sizeof(RIG_PRESETS[0])))

uint32_t rig_material_preset_count(void){ return RIG_PRESET_N; }

int rig_material_preset(RigMaterial *m, uint32_t index)
{
    if (!m || index >= RIG_PRESET_N) return -1;
    const RigPreset *p = &RIG_PRESETS[index];

    rig_material_default(m);
    m->id = index;
    rm_str(m->name, p->name, 48);

    m->albedo[0]=p->alb[0]; m->albedo[1]=p->alb[1]; m->albedo[2]=p->alb[2];
    m->metallic  = p->metallic;
    m->roughness = p->roughness;
    m->ior       = p->ior;

    m->anisotropy   = p->aniso;
    m->tangent_flow = p->flow;

    m->sheen[0]=p->sheen[0]; m->sheen[1]=p->sheen[1]; m->sheen[2]=p->sheen[2];
    m->sheen_roughness = p->sheen_r;

    m->transmission = p->trans;
    m->absorption[0]=p->absorb[0];
    m->absorption[1]=p->absorb[1];
    m->absorption[2]=p->absorb[2];
    m->thickness_mm = p->thick;
    m->dispersion   = p->disp;

    m->clearcoat           = p->coat;
    m->clearcoat_roughness = p->coat_r;

    m->iri_strength     = p->iri;
    m->iri_thickness_nm = p->iri_nm;

    m->height_fn    = p->hfn;
    m->height_scale = p->hscale;
    m->height_freq  = p->hfreq;

    m->sss_weight = p->sss;
    m->sss_radius_mm[0] = 3.2f * p->sss;    /* rojo penetra más            */
    m->sss_radius_mm[1] = 1.4f * p->sss;
    m->sss_radius_mm[2] = 0.8f * p->sss;

    /* ★ Y de aquí sale el tacto, solo. */
    rig_material_derive_haptics(m);
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §5  EMPAQUETADO GPU — 32 floats → uniforms del fragment
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_material_pack_gpu(const RigMaterial *m, float out[32])
{
    if (!m || !out) return -1;
    int i = 0;
    out[i++] = m->albedo[0];  out[i++] = m->albedo[1];  out[i++] = m->albedo[2];
    out[i++] = m->metallic;
    out[i++] = m->roughness;
    out[i++] = m->ior;
    out[i++] = m->anisotropy;
    out[i++] = m->tangent_rotation;
    out[i++] = (float)m->tangent_flow;
    out[i++] = m->sheen[0];   out[i++] = m->sheen[1];   out[i++] = m->sheen[2];
    out[i++] = m->sheen_roughness;
    out[i++] = m->transmission;
    out[i++] = m->absorption[0]; out[i++] = m->absorption[1]; out[i++] = m->absorption[2];
    out[i++] = m->thickness_mm;
    out[i++] = m->dispersion;
    out[i++] = m->clearcoat;
    out[i++] = m->clearcoat_roughness;
    out[i++] = m->clearcoat_ior;
    out[i++] = m->iri_strength;
    out[i++] = m->iri_thickness_nm;
    out[i++] = m->emissive[0]; out[i++] = m->emissive[1]; out[i++] = m->emissive[2];
    out[i++] = m->emissive_strength;
    out[i++] = (float)m->height_fn;
    out[i++] = m->height_scale;
    out[i++] = m->height_freq;
    out[i++] = m->sss_weight;
    return 0;
}
