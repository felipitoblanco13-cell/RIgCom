/* ═══════════════════════════════════════════════════════════════════════════
 * rig_haptic.c — EL TACTO · RIGCOM MASTER · BLOQUE 6
 *
 * Aquí cierra la invención.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * LA LÍNEA QUE FALTABA
 *
 * En rig_master.c, `modulation_freq_hz` aparecía exactamente DOS veces:
 * cuando se inicializaba a 0, y cuando se le asignaba un valor.
 * Nunca se leía. Nunca se aplicaba.
 *
 * Y en usonic_update_drivers():
 *
 *      int32_t amp = (int32_t)(USONIC_MAX_PRESSURE_PA * envelope_level);
 *                                                        ↑ amplitud CONSTANTE
 *
 * Consecuencia: EL MOTOR NO SE SIENTE. Literalmente. Pones la mano y no
 * notas absolutamente nada.
 *
 * Por qué: LA PIEL HUMANA NO PERCIBE 40 kHz. No puede. Los corpúsculos de
 * Pacini —los mecanorreceptores de la vibración— tienen su pico de
 * sensibilidad en 200-300 Hz y su banda muere sobre los 800 Hz.
 *
 * La haptics ultrasónica funciona EXCLUSIVAMENTE por modulación de
 * amplitud: modulas la portadora de 40 kHz con una envolvente de ~200 Hz,
 * y lo que el dedo siente ES LA ENVOLVENTE. La portadora es solo el
 * vehículo que transporta la presión hasta el aire.
 *
 * Un haz de 40 kHz sin modular produce presión de radiación ESTÁTICA.
 * Imperceptible. Nada.
 *
 * Esta es la línea:
 *
 *      env = 0.5 + 0.5·sin(2π · f_mod · t)
 *      amp = MAX · envelope · env
 *
 * ─────────────────────────────────────────────────────────────────────────
 * PSICOFÍSICA DEL TACTO (lo que gobierna todos los números de aquí)
 *
 *   PACINI    40–800 Hz, pico 250 Hz  → VIBRACIÓN. La textura fina.
 *   MEISSNER   5–50 Hz, pico  30 Hz   → DESLIZAMIENTO. El inicio del contacto.
 *   MERKEL   0.4–100 Hz               → PRESIÓN ESTÁTICA. La forma, la dureza.
 *   RUFFINI                            → ESTIRAMIENTO de la piel.
 *
 * Para que una textura se sienta REAL hay que excitar Pacini (grano fino)
 * Y Meissner (patrón espacial) a la vez, con coherencia entre ambos.
 *
 * C11 · cero libc · φ
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rig_haptic.h"

extern float rl_sqrtf(float);
extern float rl_sinf(float);
extern float rl_cosf(float);
extern float rl_fabsf(float);
extern void *rl_memset(void *, int, unsigned long);

#define HP_PI   3.14159265358979324f
#define HP_TAU  6.28318530717958648f
#define HP_PHI  1.6180339887498948f

static float hp_clamp(float x, float a, float b){ return x<a?a:(x>b?b:x); }
static float hp_max(float a, float b){ return a>b?a:b; }

static UltrasonicEngine g_us = {0};


/* ═══════════════════════════════════════════════════════════════════════════
 * §1  EL PHASED ARRAY
 *
 * 8×8 transductores, separación 20 mm, apertura 160 mm.
 * Portadora 40 kHz → λ = 343 000 / 40 000 = 8.575 mm.  (correcto)
 *
 * Para enfocar en un punto, cada elemento emite con un retardo de fase igual
 * a la diferencia de camino hasta el foco. Las ondas llegan EN FASE y se
 * suman coherentemente: ahí se concentra la presión.
 * ═══════════════════════════════════════════════════════════════════════════ */
int usonic_init(void)
{
    if (g_us.initialized) return 0;
    rl_memset(&g_us, 0, sizeof(g_us));

    /* Posición física de cada elemento, centrada en el origen */
    for (uint32_t r = 0; r < USONIC_ROWS; r++) {
        for (uint32_t c = 0; c < USONIC_COLS; c++) {
            uint32_t i = r * USONIC_COLS + c;
            g_us.elem[i].x = ((float)c - (float)(USONIC_COLS - 1u) * 0.5f)
                           * USONIC_SPACING_MM;
            g_us.elem[i].y = ((float)r - (float)(USONIC_ROWS - 1u) * 0.5f)
                           * USONIC_SPACING_MM;
            g_us.elem[i].z = 0.0f;
            g_us.elem[i].phase     = 0.0f;
            g_us.elem[i].amplitude = 0;
        }
    }

    g_us.mode           = BEAM_FOCUSED;
    g_us.envelope       = 0.0f;
    g_us.mod_freq_hz    = 200.0f;      /* pico de Pacini */
    g_us.am_depth       = 0.7f;
    g_us.stm_radius_mm  = 0.0f;
    g_us.stm_freq_hz    = 0.0f;
    g_us.time_s         = 0.0f;
    g_us.initialized    = true;
    return 0;
}

void usonic_shutdown(void)
{
    if (!g_us.initialized) return;
    for (uint32_t i = 0; i < USONIC_ELEMENTS; i++) g_us.elem[i].amplitude = 0;
    rl_memset(&g_us, 0, sizeof(g_us));
}

int usonic_set_focus(float x_mm, float y_mm, float z_mm)
{
    if (!g_us.initialized) return -1;      /* ★ devuelve valor. Antes: return; */
    g_us.focus[0] = x_mm;
    g_us.focus[1] = y_mm;
    g_us.focus[2] = hp_max(z_mm, 20.0f);
    return 0;
}

int usonic_set_signature(const RigHapticSig *s)
{
    if (!g_us.initialized || !s) return -1;
    g_us.envelope      = hp_clamp(s->envelope,  0.0f, 1.0f);
    g_us.mod_freq_hz   = hp_clamp(s->f_mod,    40.0f, 800.0f);   /* Pacini */
    g_us.am_depth      = hp_clamp(s->am_depth,  0.0f, 1.0f);
    g_us.stm_radius_mm = hp_clamp(s->stm_radius, 0.0f, 12.0f);
    g_us.stm_freq_hz   = hp_clamp(s->stm_freq,   0.0f, 300.0f);
    return 0;
}

int usonic_set_mode(UltrasonicBeamMode m)
{
    if (!g_us.initialized) return -1;
    g_us.mode = m;
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §2  ★★★  LA ACTUALIZACIÓN — AQUÍ ESTÁ TODO  ★★★
 * ═══════════════════════════════════════════════════════════════════════════ */
int usonic_update(float dt)
{
    if (!g_us.initialized) return -1;

    g_us.time_s += dt;
    const float t = g_us.time_s;

    /* ═══════════════════════════════════════════════════════════════════
     * ★ 1. SPATIOTEMPORAL MODULATION (STM)
     *
     * El estado del arte NO usa AM sola. Hace ORBITAR el punto focal en un
     * círculo pequeño (2-8 mm) a alta velocidad (~100-200 Hz).
     *
     * Por qué se siente MUCHO más fuerte: en vez de presionar un punto
     * fijo, el foco BARRE la piel. El barrido excita a los Meissner
     * (deslizamiento) además de a los Pacini (vibración). Dos poblaciones
     * de receptores en vez de una.
     *
     * Y el radio de la órbita codifica el GRANO: fino para la seda, ancho
     * para la piedra.
     * ═══════════════════════════════════════════════════════════════════ */
    float fx = g_us.focus[0];
    float fy = g_us.focus[1];
    float fz = g_us.focus[2];

    if (g_us.stm_radius_mm > 0.05f && g_us.stm_freq_hz > 1.0f) {
        float orbit = HP_TAU * g_us.stm_freq_hz * t;
        fx += rl_cosf(orbit) * g_us.stm_radius_mm;
        fy += rl_sinf(orbit) * g_us.stm_radius_mm;
    }

    /* ═══════════════════════════════════════════════════════════════════
     * ★ 2. LA MODULACIÓN AM  —  LA LÍNEA QUE FALTABA
     *
     * Sin esto, el motor entero no se siente. Cero. Nada.
     *
     * La piel NO percibe la portadora de 40 kHz. Percibe la ENVOLVENTE.
     * ═══════════════════════════════════════════════════════════════════ */
    float am = 1.0f;
    if (g_us.mod_freq_hz > 1.0f) {
        float carrier_env = 0.5f + 0.5f * rl_sinf(HP_TAU * g_us.mod_freq_hz * t);
        /* am_depth = 0 → sin modular (no se siente)
         * am_depth = 1 → modulación total (máxima percepción) */
        am = (1.0f - g_us.am_depth) + g_us.am_depth * carrier_env;
    }

    float level = g_us.envelope * am;
    level = hp_clamp(level, 0.0f, 1.0f);

    /* ═══════════════════════════════════════════════════════════════════
     * ★ 3. BEAMFORMING — retardo de fase por diferencia de camino
     *
     * Para que las 64 ondas lleguen EN FASE al foco, cada elemento debe
     * emitir adelantado según lo LEJOS que esté:
     *
     *      φ_i = −2π · d_i / λ        (módulo 2π)
     *
     * Los elementos lejanos emiten antes; los cercanos, después. Todas las
     * ondas coinciden en el foco. Ahí, y solo ahí, la presión se dispara.
     * ═══════════════════════════════════════════════════════════════════ */
    const float lambda = USONIC_WAVELENGTH_MM;      /* 8.575 mm */

    for (uint32_t i = 0; i < USONIC_ELEMENTS; i++) {
        UltrasonicElement *e = &g_us.elem[i];

        float dx = fx - e->x;
        float dy = fy - e->y;
        float dz = fz - e->z;
        float d  = rl_sqrtf(dx*dx + dy*dy + dz*dz);

        float phase = 0.0f;

        switch (g_us.mode) {
        case BEAM_FLAT:
            phase = 0.0f;                          /* onda plana */
            break;

        case BEAM_FOCUSED:
            /* ★ El foco: fase = −2π·d/λ */
            phase = -HP_TAU * (d / lambda);
            break;

        case BEAM_TRAVELING:
            /* Onda viajera: el foco enfocado + una rampa de fase que
             * desplaza el punto de presión lateralmente. */
            phase = -HP_TAU * (d / lambda)
                  + HP_TAU * 0.15f * ((float)i / (float)USONIC_ELEMENTS)
                  + HP_TAU * 2.0f * t;
            break;

        default:
            phase = -HP_TAU * (d / lambda);
            break;
        }

        /* Envolver a [0, 2π) */
        while (phase < 0.0f)     phase += HP_TAU;
        while (phase >= HP_TAU)  phase -= HP_TAU;
        e->phase = phase;

        /* ★ LA AMPLITUD, CON SU MODULACIÓN.
         * Antes era: amp = MAX · envelope    ← constante, imperceptible.
         * Ahora es:  amp = MAX · envelope · AM(t)                          */
        float amp = (float)USONIC_MAX_PRESSURE_PA * level;

        /* Apodización: los elementos del borde emiten algo menos.
         * Reduce los lóbulos laterales, que son lo que emborrona el foco
         * y hace que la textura se sienta "sucia". */
        float rx = e->x / (USONIC_APERTURE_MM * 0.5f);
        float ry = e->y / (USONIC_APERTURE_MM * 0.5f);
        float rr = rl_sqrtf(rx*rx + ry*ry);
        float apod = 0.54f + 0.46f * rl_cosf(HP_PI * hp_clamp(rr, 0.0f, 1.0f));
        amp *= apod;

        e->amplitude = (int32_t)hp_clamp(amp, 0.0f,
                                         (float)USONIC_MAX_PRESSURE_PA);
    }

    g_us.current_level = level;
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §3  PRESIÓN EN UN PUNTO — la superposición COHERENTE de verdad
 *
 * Antes era una gaussiana alrededor del foco. Eso ignora los lóbulos
 * laterales, que son justamente lo que ensucia la nitidez táctil.
 *
 * Ahora: suma compleja real de las 64 contribuciones.
 *
 *      p(r) = Σ  (A_i / d_i) · exp( i·(k·d_i + φ_i) )
 *
 * Y la FUERZA sobre la piel es la presión de radiación acústica, que va con
 * el CUADRADO de la presión, no con la presión:
 *
 *      F = 2·α·I/c ,   I ∝ p²
 * ═══════════════════════════════════════════════════════════════════════════ */
float usonic_pressure_at(float x, float y, float z)
{
    if (!g_us.initialized) return 0.0f;

    const float k = HP_TAU / USONIC_WAVELENGTH_MM;   /* número de onda */
    float re = 0.0f, im = 0.0f;

    for (uint32_t i = 0; i < USONIC_ELEMENTS; i++) {
        const UltrasonicElement *e = &g_us.elem[i];
        if (e->amplitude <= 0) continue;

        float dx = x - e->x, dy = y - e->y, dz = z - e->z;
        float d  = rl_sqrtf(dx*dx + dy*dy + dz*dz);
        if (d < 1.0f) d = 1.0f;                       /* campo cercano */

        /* Divergencia esférica: la amplitud cae con 1/d */
        float a  = (float)e->amplitude / d * 20.0f;
        float ph = k * d + e->phase;

        re += a * rl_cosf(ph);
        im += a * rl_sinf(ph);
    }
    return rl_sqrtf(re*re + im*im);
}

/* Fuerza de radiación acústica sobre la piel.
 * F ∝ p² — el cuadrado, no la presión. Por eso duplicar la presión
 * cuadruplica la fuerza percibida. */
float usonic_force_at(float x, float y, float z)
{
    float p = usonic_pressure_at(x, y, z);
    /* F = 2·α·I/c,  con I = p²/(2·ρ·c).
     * Constante agrupada para dar mN con presiones en Pa. */
    return (p * p) * 1.16e-8f;
}

float usonic_level(void){ return g_us.initialized ? g_us.current_level : 0.0f; }
const UltrasonicEngine* usonic_state(void){ return &g_us; }


/* ═══════════════════════════════════════════════════════════════════════════
 * §4  ★★★  EL ACOPLAMIENTO AOM ↔ HÁPTICO  ★★★
 *
 * ESTE es el puente. Son veinte líneas y es todo el proyecto.
 *
 * Antes:
 *      if (hand_present) {
 *          usonic_set_focal_point(hand_x, hand_y, hand_z);
 *          usonic_set_envelope(1.f);        ← SIEMPRE 1.0
 *      }
 *
 * La envolvente valía 1.0 siempre que hubiera una mano, estuviera donde
 * estuviera: atravesando el holograma o en el vacío a treinta centímetros.
 * El háptico NO CONSULTABA la rejilla de vóxeles. No sabía si tocabas algo.
 *
 * Ahora:
 *      1. Se consulta la rejilla en la posición de la mano.
 *      2. Si es aire (densidad ≈ 0) → envolvente 0. NO SIENTES NADA.
 *      3. Si hay vóxel → se lee su MATERIAL.
 *      4. Del material sale la FIRMA HÁPTICA (f_mod, AM, STM, envolvente).
 *      5. Y el ultrasonido reproduce ESA firma.
 *
 * Resultado: metes la mano en el holograma y sientes SEDA donde hay seda,
 * y PIEDRA donde hay piedra. Y en el aire, nada.
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_haptic_couple(const AOMEngine *aom,
                      const RigMaterial *mats, uint32_t mat_count,
                      const float hand_mm[3],
                      float finger_speed_mm_s,
                      float dt,
                      RigHapticOut *out)
{
    if (!aom || !hand_mm || !out) return -1;
    rl_memset(out, 0, sizeof(*out));

    if (!g_us.initialized) usonic_init();

    /* ── 1. De milímetros a coordenadas del volumen [0,1]³ ── */
    float half = aom->volume_mm * 0.5f;
    float p[3] = {
        (hand_mm[0] + half) / aom->volume_mm,
        (hand_mm[1] + half) / aom->volume_mm,
        (hand_mm[2])        / aom->volume_mm
    };

    /* ── 2. ★ CONSULTAR LA REJILLA ── */
    RigVoxel v;
    aom_sample(aom, p, &v);

    float density = (float)v.density / 255.0f;

    /* ── 3. ¿AIRE? → silencio absoluto ──
     * Esta es la comprobación que no existía. */
    if (density < 0.05f) {
        RigHapticSig zero;
        rl_memset(&zero, 0, sizeof(zero));
        zero.envelope = 0.0f;
        usonic_set_signature(&zero);
        usonic_update(dt);
        out->in_contact = false;
        out->pressure_pa = 0.0f;
        return 0;
    }

    /* ── 4. ★ EL MATERIAL DEL VÓXEL ── */
    uint32_t mi = v.material;
    if (!mats || mi >= mat_count) mi = 0;

    RigHapticSig sig = mats ? mats[mi].haptic : (RigHapticSig){0};

    /* ── 5. Modular por la DENSIDAD (borde blando, núcleo firme) ──
     * En el borde del objeto la densidad baja: la envolvente cae. Eso hace
     * que el contacto tenga una "entrada" suave en vez de un golpe seco.
     * Los Meissner detectan justamente eso. */
    float edge = hp_clamp(density * 1.6f, 0.0f, 1.0f);
    sig.envelope *= edge;

    /* ── 6. ★ STICK-SLIP: la ganancia sigue la VELOCIDAD del dedo ──
     * Dedo quieto → silencio. Al arrastrar, la textura aparece.
     * Es exactamente lo que pasa en el mundo real: una superficie no
     * "suena" ni "raspa" si no hay deslizamiento. */
    float v_norm = hp_clamp(finger_speed_mm_s / 180.0f, 0.0f, 1.0f);
    float slip   = 0.22f + 0.78f * v_norm * sig.friction;

    /* La modulación AM crece con la velocidad: al deslizar más rápido,
     * el dedo tropieza con el grano más a menudo. Física pura. */
    sig.f_mod    = hp_clamp(sig.f_mod * (0.65f + 0.55f * v_norm),
                            40.0f, 800.0f);
    sig.am_depth = hp_clamp(sig.am_depth * (0.55f + 0.60f * v_norm),
                            0.0f, 1.0f);
    sig.envelope = hp_clamp(sig.envelope * slip, 0.0f, 1.0f);

    /* ── 7. Al ultrasonido ── */
    usonic_set_focus(hand_mm[0], hand_mm[1], hp_max(hand_mm[2], 60.0f));
    usonic_set_signature(&sig);
    usonic_set_mode(sig.stm_radius > 0.5f ? BEAM_TRAVELING : BEAM_FOCUSED);
    usonic_update(dt);

    /* ── 8. Devolver también lo que necesitan el audio y el LRA ── */
    out->in_contact  = true;
    out->material_id = mi;
    out->density     = density;
    out->sig         = sig;
    out->pressure_pa = usonic_pressure_at(hand_mm[0], hand_mm[1], hand_mm[2]);
    out->force_mn    = usonic_force_at   (hand_mm[0], hand_mm[1], hand_mm[2]);
    out->color[0]    = (float)v.r / 255.0f;
    out->color[1]    = (float)v.g / 255.0f;
    out->color[2]    = (float)v.b / 255.0f;

    return 0;
}
