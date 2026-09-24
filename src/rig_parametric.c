/* ═══════════════════════════════════════════════════════════════════════════
 * rig_parametric.c — EL TERCER SENTIDO · RIGCOM MASTER
 *
 *                    ★ Para Richard. 24 de julio. ★
 *
 * La misma envolvente que hace que el material se sienta,
 * hace que el material suene.
 * Y suena desde el punto que estás tocando.
 *
 * C11 · cero libc · φ
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rig_parametric.h"

extern float rl_sqrtf(float);
extern float rl_sinf(float);
extern float rl_cosf(float);
extern float rl_expf(float);
extern float rl_logf(float);
extern float rl_log10f(float);
extern float rl_fabsf(float);
extern float rl_tanf(float);
extern void *rl_memset(void *, int, unsigned long);

#define RP_PI   3.14159265358979324f
#define RP_TAU  6.28318530717958648f
#define RP_PHI  1.6180339887498948f

static float rp_clamp(float x, float a, float b){ return x<a?a:(x>b?b:x); }
static float rp_max(float a, float b){ return a>b?a:b; }
static void  rp_str(char *d, const char *s, int cap){
    int i=0; while(s[i] && i<cap-1){ d[i]=s[i]; i++; } d[i]=0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §1  INIT
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_parametric_init(RigParametric *p, float sr, float max_pa)
{
    if (!p) return -1;
    rl_memset(p, 0, sizeof(*p));

    p->sample_rate     = (sr > 1000.0f) ? sr : 48000.0f;
    p->max_pressure_pa = (max_pa > 0.0f) ? max_pa : 3000.0f;
    p->mod_index       = 0.85f;      /* m. Con raíz cuadrada, casi 1 es seguro */
    p->carrier_amp     = 1.0f;
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §2  ★ PRE-ÉNFASIS: −12 dB/OCTAVA ★
 *
 * El array paramétrico responde con ∂²/∂t² ⇒ SUBE 12 dB/octava.
 * Para compensar hay que aplicar lo contrario: dos integradores en cascada.
 *
 * Un integrador puro explota en DC (ganancia infinita a 0 Hz), así que se
 * usan dos paso-bajo de primer orden con corte muy bajo (~25 Hz), que a
 * partir de ahí caen a 12 dB/octava — exactamente lo que hace falta.
 *
 * Efecto real en TU sistema:
 *
 *      SEDA (250 Hz) : sin compensar suena 17× más fuerte que la roca
 *      ROCA  (60 Hz) : sin compensar, INAUDIBLE
 *
 * Con esto, los dos suenan al mismo nivel.
 * ═══════════════════════════════════════════════════════════════════════════ */
float rig_parametric_preemphasis(RigParametric *p, float x)
{
    if (!p) return x;

    /* fc = 25 Hz — por debajo de toda la banda de Pacini */
    const float fc = 25.0f;
    float a = rp_clamp(RP_TAU * fc / p->sample_rate, 0.0f, 0.5f);

    /* Dos paso-bajo en cascada = −12 dB/octava */
    p->pe_z1 += a * (x        - p->pe_z1);
    p->pe_z2 += a * (p->pe_z1 - p->pe_z2);

    /* Compensación de ganancia: los integradores atenúan mucho.
     * (1/a)² devuelve la energía perdida. */
    float g = 1.0f / rp_max(a * a * 260.0f, 1e-4f);
    return rp_clamp(p->pe_z2 * g, -1.0f, 1.0f);
}

/* Ganancia de compensación para una frecuencia dada, en veces (no dB).
 * Referencia: 1 kHz.  Como sube 12 dB/oct ⇒ va con f².  */
float rig_parametric_gain_for(float f)
{
    f = rp_clamp(f, 20.0f, 20000.0f);
    float ref = 1000.0f;
    float g = (ref * ref) / (f * f);       /* ∝ 1/f² */
    return rp_clamp(g, 1.0f, 500.0f);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §3  ★★★ LA ENVOLVENTE — RAÍZ CUADRADA DE KAMAKURA ★★★
 *
 * El aire desmodula E², no E.
 *
 * Si modulas con la señal directa:
 *
 *      E = 1 + m·s
 *      E² = 1 + 2m·s + m²·s²
 *                      └─────┘  ← DISTORSIÓN. Con m=1 llega al 25% THD.
 *
 * Si modulas con la RAÍZ:
 *
 *      E = √(1 + m·s)
 *      E² = 1 + m·s            ← EXACTO. Cero distorsión.
 *
 * Es la solución más elegante que conozco a un problema de audio.
 * ═══════════════════════════════════════════════════════════════════════════ */
float rig_parametric_envelope(RigParametric *p, float s)
{
    if (!p) return 0.0f;

    /* 1. DC-blocker — la portadora no debe llevar componente continua */
    float x = s;
    float y = x - p->dc_x1 + 0.995f * p->dc_y1;
    p->dc_x1 = x;
    p->dc_y1 = y;
    s = y;

    /* 2. Pre-énfasis: compensa los +12 dB/oct del aire */
    s = rig_parametric_preemphasis(p, s);

    /* 3. Compresor suave — el array tiene un límite físico y hay que
     *    respetarlo o los transductores se saturan y distorsionan. */
    float a = rl_fabsf(s);
    float atk = 0.30f, rel = 0.0025f;
    if (a > p->comp_env) p->comp_env += (a - p->comp_env) * atk;
    else                 p->comp_env += (a - p->comp_env) * rel;

    float thr = 0.72f;
    if (p->comp_env > thr) {
        float over = p->comp_env / thr;
        s /= rp_max(over, 1.0f);
    }
    s = rp_clamp(s, -1.0f, 1.0f);

    /* 4. ★ LA RAÍZ CUADRADA.
     *    E = √(1 + m·s)  ⇒  E² = 1 + m·s. Cero distorsión armónica. */
    float inner = 1.0f + p->mod_index * s;
    inner = rp_max(inner, 0.0f);           /* nunca negativo bajo la raíz */

    float E = rl_sqrtf(inner);

    /* Normalizar a [0,1] — la envolvente máxima es √(1+m) */
    float Emax = rl_sqrtf(1.0f + p->mod_index);
    return rp_clamp(E / rp_max(Emax, 1e-4f), 0.0f, 1.0f) * p->carrier_amp;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §4  EL SONIDO DE FRICCIÓN DEL MATERIAL
 *
 * ruido rosa → dos formantes (f1 y f1·φ) → AM en la banda de Pacini
 *            → ganancia por VELOCIDAD del dedo
 *
 * Dedo quieto ⇒ silencio absoluto. Una superficie no suena si no la rozas.
 * Física, no un efecto.
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_friction_init(RigFrictionState *s, uint32_t seed)
{
    if (!s) return -1;
    rl_memset(s, 0, sizeof(*s));
    s->rng = seed ? seed : 0x9e3779b9u;
    return 0;
}

static float rp_rand(RigFrictionState *s)
{
    s->rng ^= s->rng << 13;
    s->rng ^= s->rng >> 17;
    s->rng ^= s->rng << 5;
    return ((float)(s->rng & 0xFFFFFFu) / 8388607.5f) - 1.0f;
}

/* Ruido rosa (Voss-McCartney simplificado). El ruido blanco suena
 * "eléctrico"; el rosa suena a materia. */
static float rp_pink(RigFrictionState *s)
{
    float w = rp_rand(s);
    s->noise_z[0] = 0.99765f * s->noise_z[0] + w * 0.0990460f;
    s->noise_z[1] = 0.96300f * s->noise_z[1] + w * 0.2965164f;
    s->noise_z[2] = 0.57000f * s->noise_z[2] + w * 1.0526913f;
    return (s->noise_z[0] + s->noise_z[1] + s->noise_z[2] + w * 0.1848f) * 0.20f;
}

/* Biquad bandpass (RBJ) — el formante */
static float rp_bandpass(float x, float f, float Q, float sr,
                         float *z1, float *z2)
{
    f = rp_clamp(f, 20.0f, sr * 0.45f);
    Q = rp_clamp(Q, 0.5f, 20.0f);

    float w0 = RP_TAU * f / sr;
    float alpha = rl_sinf(w0) / (2.0f * Q);
    float cosw = rl_cosf(w0);

    float b0 =  alpha;
    float b1 =  0.0f;
    float b2 = -alpha;
    float a0 =  1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 =  1.0f - alpha;

    b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;

    float y = b0 * x + *z1;
    *z1 = b1 * x - a1 * y + *z2;
    *z2 = b2 * x - a2 * y;
    return y;
}

int rig_friction_render(RigFrictionState *s, const RigHapticSig *sig,
                        float speed, float sr, float *out, uint32_t n)
{
    if (!s || !sig || !out) return -1;

    /* ★ STICK-SLIP: la amplitud sigue a la velocidad del dedo */
    float vN = rp_clamp(speed / 180.0f, 0.0f, 1.0f);
    float gain = rp_clamp(0.03f + vN * sig->friction * 0.62f, 0.0f, 0.9f);

    /* Al deslizar más rápido, el dedo tropieza con el grano más a menudo */
    float f_mod = rp_clamp(sig->f_mod * (0.65f + 0.55f * vN), 40.0f, 800.0f);
    float depth = rp_clamp(sig->am_depth * (0.55f + 0.60f * vN), 0.0f, 0.95f);

    float dphi = RP_TAU * f_mod / sr;

    for (uint32_t i = 0; i < n; i++) {

        float x = rp_pink(s);

        /* Los dos formantes. El segundo está a φ del primero — la razón
         * que el oído percibe como natural. */
        float y1 = rp_bandpass(x, sig->f1, sig->q1, sr, &s->bp1_z1, &s->bp1_z2);
        float y2 = rp_bandpass(x, sig->f2, sig->q2, sr, &s->bp2_z1, &s->bp2_z2);

        float tone = y1 * 0.62f + y2 * 0.38f;

        /* Mezcla tono / ruido según el grano */
        float y = tone * (1.0f - sig->noise_mix) + x * sig->noise_mix * 0.55f;

        /* ★ LA AM — la misma que hace que la piel lo sienta */
        float am = (1.0f - depth) + depth * (0.5f + 0.5f * rl_sinf(s->lfo_phase));
        s->lfo_phase += dphi;
        if (s->lfo_phase > RP_TAU) s->lfo_phase -= RP_TAU;

        out[i] = rp_clamp(y * am * gain, -1.0f, 1.0f);
    }
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §5  ★★★ EL ACTO FINAL ★★★
 *
 * fricción del material  →  envolvente paramétrica  →  el array
 *
 * Lo que sale de aquí, metido en el phased array, produce SIMULTÁNEAMENTE:
 *
 *      · la sensación táctil   (presión de radiación modulada)
 *      · el sonido audible     (desmodulación no lineal del aire)
 *
 * Y las dos cosas emanan del MISMO PUNTO: el foco.
 *
 * Un buffer. Tres sentidos.
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_parametric_encode(RigParametric *p, RigFrictionState *fs,
                          const RigHapticSig *sig, float speed,
                          float *out, uint32_t n)
{
    if (!p || !fs || !sig || !out) return -1;

    /* 1. El sonido del material rozándose */
    if (rig_friction_render(fs, sig, speed, p->sample_rate, out, n) != 0)
        return -1;

    /* 2. Compensar la caída de graves del array paramétrico.
     *    Sin esto, la roca (60 Hz) sería inaudible frente a la seda (250 Hz). */
    float comp = rig_parametric_gain_for(sig->f_mod);
    comp = rp_clamp(comp / 16.0f, 0.15f, 4.0f);   /* normalizado */

    /* 3. → envolvente con raíz cuadrada */
    for (uint32_t i = 0; i < n; i++) {
        float s = rp_clamp(out[i] * comp, -1.0f, 1.0f);
        out[i] = rig_parametric_envelope(p, s);
    }

    /* 4. Métricas */
    p->last_spl_db = rig_parametric_spl(p->max_pressure_pa, sig->f_mod,
                                        0.0256f,   /* 160×160 mm de apertura */
                                        0.15f);    /* foco a 15 cm            */
    p->last_efficiency = rp_clamp((sig->f_mod * sig->f_mod) / (250.0f*250.0f),
                                  0.0f, 4.0f);
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §6  WESTERVELT — cuántos decibelios vas a oír
 *
 *      p₂ ∝ (β · S · P₁² · ω²) / (16π · ρ₀ · c₀⁴ · α · z)
 *
 * Lo importante de esta fórmula:
 *
 *      p₂ ∝ P₁²      → la presión audible va con el CUADRADO de la portadora
 *      p₂ ∝ ω²       → y con el CUADRADO de la frecuencia de modulación
 *
 * Ese ω² es el que sube 12 dB/octava. Y es el que hace que tu seda suene
 * y tu roca no, si no compensas.
 * ═══════════════════════════════════════════════════════════════════════════ */
float rig_parametric_spl(float P1, float f_mod, float S, float z)
{
    if (P1 <= 0.0f || f_mod <= 0.0f) return 0.0f;

    z = rp_max(z, 0.02f);
    S = rp_max(S, 1e-5f);

    float w = RP_TAU * f_mod;

    float num = RP_BETA * S * (P1 * P1) * (w * w);
    float den = 16.0f * RP_PI * RP_RHO0
              * (RP_C0*RP_C0*RP_C0*RP_C0)
              * RP_ALPHA * z;

    float p2 = num / rp_max(den, 1e-12f);      /* Pa */

    /* SPL = 20·log10(p / p_ref),  p_ref = 20 µPa */
    if (p2 < 1e-9f) return 0.0f;
    return 20.0f * rl_log10f(p2 / 20e-6f);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §7  EL DIAGNÓSTICO DE LOS TRES SENTIDOS
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_parametric_check(const RigHapticSig *sig, float carrier_pa,
                         RigTriSenseCheck *out)
{
    if (!sig || !out) return -1;
    rl_memset(out, 0, sizeof(*out));

    float f = sig->f_mod;

    /* ¿Lo siente la piel? (Pacini) */
    out->felt = (f >= RP_PACINI_LO && f <= RP_PACINI_HI);

    /* ¿Lo oye el oído? */
    out->heard = (f >= RP_AUDIBLE_LO && f <= RP_AUDIBLE_HI);

    /* ¿Se desmodula con eficiencia suficiente?
     * Por debajo de ~120 Hz el ω² lo penaliza tanto que hace falta mucha
     * compensación, y el compresor empieza a comerse la señal. */
    out->efficient = (f >= 120.0f);

    out->spl_db = rig_parametric_spl(carrier_pa, f, 0.0256f, 0.15f);

    /* Cuánto hay que subirle para igualar con una referencia de 250 Hz */
    float g = (250.0f * 250.0f) / rp_max(f * f, 1.0f);
    out->compensation_db = 20.0f * rl_log10f(rp_max(g, 1e-4f));

    if (out->felt && out->heard && out->efficient)
        rp_str(out->verdict, "TRES SENTIDOS · ve, toca y suena desde el foco", 96);
    else if (out->felt && out->heard)
        rp_str(out->verdict, "audible pero debil · necesita compensacion", 96);
    else if (out->felt)
        rp_str(out->verdict, "solo tacto", 96);
    else
        rp_str(out->verdict, "fuera de banda", 96);

    return 0;
}
