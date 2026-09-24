/* ============================================================================
 * rig_face_voice_synth.c
 *
 * RigCom :: Phoneme-to-Audio Formant Synthesis Module (aditivo puro)
 *
 * audio.c ya hace audio->fonema (FFT + formantes F1/F2 via PhonModel).
 * Este modulo agrega la direccion INVERSA: fonema->audio, sintesis de voz
 * real por formantes paralelos (arquitectura clasica tipo Holmes/Klatt
 * simplificada), para que el mismo motor pueda GENERAR voz sintetica a
 * partir de una secuencia de fonemas, no solo analizarla.
 *
 *   1. Tabla de formantes por fonema (vocales: datos promedio estandar de
 *      fonetica acustica adulta F1/F2/F3; consonantes: clases simplificadas
 *      fricativa sorda/sonora, nasal, oclusiva, aproximante).
 *   2. Fuente glotal: tren de pulsos con jitter/shimmer para voz sonora
 *      (integrador con fuga sobre tren de impulsos -> inclinacion
 *      espectral realista), ruido blanco para sonidos sordos.
 *   3. Banco de 3 resonadores biquad EN PARALELO (formant synthesis
 *      clasica), con interpolacion suave de formantes en fronteras entre
 *      fonemas (crossfade coseno) para evitar clics.
 *   4. Contorno de pitch (prosody) via keyframes interpolados.
 *   5. Escritura de WAV PCM16 real (formato RIFF/WAVE estandar).
 *   6. Serializacion de una secuencia fonetica (para reutilizar guiones).
 * ==========================================================================*/

#ifndef RIGCOM_PUBLIC
#define RIGCOM_PUBLIC
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define RIG_VOICE_MAX_PHONEMES_SEQ  512
#define RIG_VOICE_MAX_PITCH_KEYS    64
#define RIG_VOICE_MAGIC             0x564f4931u /* "VOI1" */
#define RIG_VOICE_VERSION           1

typedef enum {
    RIG_PHON_SIL=0, RIG_PHON_AA, RIG_PHON_EH, RIG_PHON_IY, RIG_PHON_AO, RIG_PHON_UW, RIG_PHON_AX,
    RIG_PHON_M, RIG_PHON_N, RIG_PHON_L, RIG_PHON_R,
    RIG_PHON_S, RIG_PHON_SH, RIG_PHON_F, RIG_PHON_Z, RIG_PHON_V,
    RIG_PHON_P, RIG_PHON_T, RIG_PHON_K, RIG_PHON_B, RIG_PHON_D, RIG_PHON_G,
    RIG_PHON_COUNT
} RigPhonemeId;

typedef struct {
    float f1, f2, f3;
    float bw1, bw2, bw3;
    float gain1, gain2, gain3;
    int   is_voiced;      /* 1 = fuente glotal, 0 = ruido */
    float noise_mix;      /* 0..1, mezcla de ruido incluso si is_voiced (fricativas sonoras) */
    int   is_plosive;     /* 1 = silencio breve + burst, en vez de sostenido */
    float default_duration_ms;
} RigPhonemeFormants;

/* Datos promedio de formantes de vocales (Peterson & Barney / IPA
 * estandar, voz adulta tipica) y clases de consonantes simplificadas. */
static const RigPhonemeFormants RIG_PHONEME_TABLE[RIG_PHON_COUNT] = {
    /*                 f1    f2    f3   bw1 bw2 bw3  g1   g2   g3  voiced noise plosive dur_ms */
    /* SIL */        {  0,    0,    0,   60, 90,120, 0.0f,0.0f,0.0f, 0, 0.0f, 0, 80.0f },
    /* AA (padre) */ {730, 1090, 2440,   70,100,120, 1.0f,0.7f,0.4f, 1, 0.0f, 0, 140.0f },
    /* EH (bet)   */ {530, 1840, 2480,   60,110,130, 1.0f,0.7f,0.4f, 1, 0.0f, 0, 120.0f },
    /* IY (beet)  */ {270, 2290, 3010,   50,120,140, 1.0f,0.65f,0.35f,1, 0.0f, 0, 130.0f },
    /* AO (bought)*/ {570,  840, 2410,   70, 90,120, 1.0f,0.7f,0.4f, 1, 0.0f, 0, 140.0f },
    /* UW (boot)  */ {300,  870, 2240,   55, 90,120, 1.0f,0.6f,0.35f,1, 0.0f, 0, 130.0f },
    /* AX (schwa) */ {500, 1500, 2500,   70,100,130, 0.9f,0.6f,0.35f,1, 0.0f, 0, 90.0f },
    /* M nasal    */ {250, 1000, 2200,   60,150,180, 1.0f,0.35f,0.2f, 1, 0.05f,0, 90.0f },
    /* N nasal    */ {280, 1700, 2600,   60,150,180, 1.0f,0.35f,0.2f, 1, 0.05f,0, 90.0f },
    /* L liquida  */ {360, 1300, 2600,   60,110,140, 1.0f,0.55f,0.3f, 1, 0.0f, 0, 90.0f },
    /* R aprox.   */ {310, 1060, 1380,   60,110,140, 1.0f,0.6f,0.4f, 1, 0.0f, 0, 90.0f },
    /* S fric sorda*/{4500, 6500, 8000, 400,600,700, 0.1f,0.4f,1.0f, 0, 1.0f, 0, 110.0f },
    /* SH fric sorda*/{2500,4500, 7000, 350,500,700, 0.1f,0.6f,0.9f, 0, 1.0f, 0, 110.0f },
    /* F fric sorda*/{1400, 3500, 6000, 400,600,700, 0.2f,0.4f,0.5f, 0, 1.0f, 0, 90.0f },
    /* Z fric son.*/ {4500, 6500, 8000, 400,600,700, 0.15f,0.4f,0.8f,1, 0.75f,0, 100.0f },
    /* V fric son.*/ {1400, 3500, 6000, 400,600,700, 0.25f,0.4f,0.4f,1, 0.65f,0, 90.0f },
    /* P plosiva  */ { 400,  900, 2200,  80,140,180, 0.6f,0.5f,0.3f, 0, 0.9f, 1, 60.0f },
    /* T plosiva  */ { 400, 1700, 2600,  80,140,180, 0.6f,0.6f,0.4f, 0, 0.9f, 1, 60.0f },
    /* K plosiva  */ { 400, 2000, 3000,  80,140,180, 0.6f,0.6f,0.4f, 0, 0.9f, 1, 60.0f },
    /* B plosiva sonora*/{400,900, 2200,  80,140,180, 0.6f,0.5f,0.3f, 1, 0.5f, 1, 70.0f },
    /* D plosiva sonora*/{400,1700,2600,  80,140,180, 0.6f,0.6f,0.4f, 1, 0.5f, 1, 70.0f },
    /* G plosiva sonora*/{400,2000,3000,  80,140,180, 0.6f,0.6f,0.4f, 1, 0.5f, 1, 70.0f },
};

typedef struct {
    int   phoneme_id;
    float duration_ms;   /* 0 = usar default_duration_ms de la tabla */
} RigPhonemeSeqEntry;

typedef struct {
    float time_ms;
    float f0_hz;
} RigPitchKey;

typedef struct {
    RigPhonemeSeqEntry entries[RIG_VOICE_MAX_PHONEMES_SEQ];
    int entry_count;
    RigPitchKey pitch_keys[RIG_VOICE_MAX_PITCH_KEYS];
    int pitch_key_count;
    float jitter_percent;   /* variacion aleatoria de F0 ciclo a ciclo, tipico 0.5-1.5% */
    float shimmer_percent;  /* variacion aleatoria de amplitud ciclo a ciclo, tipico 3-6% */
} RigVoiceScript;

/* -------------------------------------------------------------------------
 * PRNG local
 * ---------------------------------------------------------------------- */
static unsigned int rig_voice_xorshift32__rig_dup_5b233d5a(unsigned int *state) {
    unsigned int x = *state ? *state : 0xBADC0FFEu;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5; *state = x; return x;
}
static float rig_voice_rand_bipolar__rig_dup_411df7f6(unsigned int *state) {
    return ((float)(rig_voice_xorshift32__rig_dup_5b233d5a(state) & 0x00FFFFFFu) / (float)0x00800000u) - 1.0f;
}

/* -------------------------------------------------------------------------
 * Script API
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_voice_script_init__rig_dup_82ffe777(RigVoiceScript *script) {
    if (!script) return -1;
    memset(script, 0, sizeof(*script));
    script->jitter_percent = 0.8f;
    script->shimmer_percent = 4.0f;
    return 0;
}

RIGCOM_PUBLIC int rig_voice_script_add_phoneme__rig_dup_769c52e8(RigVoiceScript *script, RigPhonemeId id, float duration_ms_or_zero) {
    if (!script) return -1;
    if (script->entry_count >= RIG_VOICE_MAX_PHONEMES_SEQ) return -2;
    if (id < 0 || id >= RIG_PHON_COUNT) return -3;
    script->entries[script->entry_count].phoneme_id = id;
    script->entries[script->entry_count].duration_ms = duration_ms_or_zero;
    script->entry_count++;
    return script->entry_count - 1;
}

RIGCOM_PUBLIC int rig_voice_script_add_pitch_key__rig_dup_dd5f22a3(RigVoiceScript *script, float time_ms, float f0_hz) {
    if (!script) return -1;
    if (script->pitch_key_count >= RIG_VOICE_MAX_PITCH_KEYS) return -2;
    script->pitch_keys[script->pitch_key_count].time_ms = time_ms;
    script->pitch_keys[script->pitch_key_count].f0_hz = f0_hz;
    script->pitch_key_count++;
    return script->pitch_key_count - 1;
}

static float rig_voice_f0_at__rig_dup_eb8a74cd(const RigVoiceScript *script, float time_ms) {
    if (script->pitch_key_count == 0) return 120.0f;
    if (script->pitch_key_count == 1) return script->pitch_keys[0].f0_hz;
    if (time_ms <= script->pitch_keys[0].time_ms) return script->pitch_keys[0].f0_hz;
    for (int i = 0; i < script->pitch_key_count - 1; i++) {
        const RigPitchKey *a = &script->pitch_keys[i];
        const RigPitchKey *b = &script->pitch_keys[i+1];
        if (time_ms >= a->time_ms && time_ms <= b->time_ms) {
            float span = b->time_ms - a->time_ms;
            float t = span > 1e-6f ? (time_ms - a->time_ms) / span : 0.0f;
            return a->f0_hz + (b->f0_hz - a->f0_hz) * t;
        }
    }
    return script->pitch_keys[script->pitch_key_count - 1].f0_hz;
}

/* -------------------------------------------------------------------------
 * Resonador biquad (formante), forma directa II transpuesta, clasico
 * filtro de 2 polos por formante (r = radio del polo, theta = angulo).
 * ---------------------------------------------------------------------- */
typedef struct { float z1, z2; } RigFormantResonatorState;

static float rig_voice_formant_process__rig_dup_4d24019a(RigFormantResonatorState *st, float x,
                                        float freq_hz, float bw_hz, float sample_rate) {
    float r = expf(-3.14159265f * bw_hz / sample_rate);
    float theta = 2.0f * 3.14159265f * freq_hz / sample_rate;
    float a1 = 2.0f * r * cosf(theta);
    float a2 = -r * r;
    /* normalizacion aproximada de ganancia para que la resonancia no
     * dependa fuertemente del ancho de banda elegido */
    float norm = (1.0f - r * r) * sinf(theta > 1e-4f ? theta : 1e-4f);
    if (norm < 1e-6f) norm = 1e-6f;

    float y = x * norm + a1 * st->z1 + a2 * st->z2;
    st->z2 = st->z1;
    st->z1 = y;
    return y;
}

/* -------------------------------------------------------------------------
 * Sintesis principal: recorre el script fonema por fonema, genera fuente
 * (pulso glotal o ruido segun corresponda), la pasa por 3 resonadores en
 * paralelo con interpolacion de formantes en las fronteras, y escribe en
 * out_pcm (mono, float en rango [-1,1]).
 * ---------------------------------------------------------------------- */
RIGCOM_PUBLIC int rig_voice_synthesize__rig_dup_73b14844(const RigVoiceScript *script, float sample_rate,
                                        float *out_pcm, int max_samples, int *out_sample_count) {
    if (!script || !out_pcm || !out_sample_count || sample_rate <= 0.0f) return -1;

    unsigned int rng = 0xC0FFEE01u;
    float glottal_integrator = 0.0f;
    float phase = 0.0f; /* fase del oscilador de pulso glotal, 0..1 */
    RigFormantResonatorState res1 = {0}, res2 = {0}, res3 = {0};

    int total_samples = 0;
    float t_ms = 0.0f;
    const float crossfade_ms = 18.0f;

    for (int e = 0; e < script->entry_count; e++) {
        const RigPhonemeFormants *cur = &RIG_PHONEME_TABLE[script->entries[e].phoneme_id];
        const RigPhonemeFormants *next = (e + 1 < script->entry_count)
            ? &RIG_PHONEME_TABLE[script->entries[e+1].phoneme_id] : cur;

        float dur_ms = script->entries[e].duration_ms > 0.0f ? script->entries[e].duration_ms : cur->default_duration_ms;
        int n_samples = (int)(dur_ms * 0.001f * sample_rate);

        /* --- silencio/oclusion de plosiva: primeros 40% de la duracion en silencio, luego burst --- */
        int closure_samples = cur->is_plosive ? (int)(n_samples * 0.4f) : 0;

        for (int i = 0; i < n_samples; i++) {
            if (total_samples >= max_samples) { *out_sample_count = total_samples; return -2; }

            float local_t = (float)i / (float)(n_samples > 1 ? n_samples - 1 : 1);
            float fade_samples_frac = (crossfade_ms * 0.001f * sample_rate) / (float)(n_samples > 0 ? n_samples : 1);
            float blend = local_t > (1.0f - fade_samples_frac) && fade_samples_frac > 0.0f && fade_samples_frac < 1.0f
                          ? (local_t - (1.0f - fade_samples_frac)) / fade_samples_frac : 0.0f;
            blend = 0.5f - 0.5f * cosf(blend * 3.14159265f); /* crossfade coseno suave */

            float f1 = cur->f1 + (next->f1 - cur->f1) * blend;
            float f2 = cur->f2 + (next->f2 - cur->f2) * blend;
            float f3 = cur->f3 + (next->f3 - cur->f3) * blend;
            float bw1 = cur->bw1 + (next->bw1 - cur->bw1) * blend;
            float bw2 = cur->bw2 + (next->bw2 - cur->bw2) * blend;
            float bw3 = cur->bw3 + (next->bw3 - cur->bw3) * blend;
            float g1 = cur->gain1, g2 = cur->gain2, g3 = cur->gain3;

            float amp_envelope = 1.0f;
            if (i < closure_samples) {
                amp_envelope = 0.0f; /* silencio de oclusion */
            } else if (cur->is_plosive) {
                float since_burst = (float)(i - closure_samples) / (float)(n_samples - closure_samples > 0 ? n_samples - closure_samples : 1);
                amp_envelope = expf(-since_burst * 6.0f); /* burst con decaimiento rapido */
            } else {
                /* ataque/caida suave para evitar clics en fonemas sostenidos */
                float attack = local_t < 0.05f ? (local_t / 0.05f) : 1.0f;
                float release = local_t > 0.9f ? (1.0f - local_t) / 0.1f : 1.0f;
                amp_envelope = attack * release;
            }

            float f0 = rig_voice_f0_at__rig_dup_eb8a74cd(script, t_ms);
            float jitter = 1.0f + (script->jitter_percent * 0.01f) * rig_voice_rand_bipolar__rig_dup_411df7f6(&rng);
            float period_samples = sample_rate / (f0 * jitter > 1.0f ? f0 * jitter : 1.0f);

            float source = 0.0f;
            if (cur->is_voiced || cur->noise_mix < 1.0f) {
                phase += 1.0f / (period_samples > 1.0f ? period_samples : 1.0f);
                float pulse = 0.0f;
                if (phase >= 1.0f) {
                    phase -= 1.0f;
                    float shimmer = 1.0f + (script->shimmer_percent * 0.01f) * rig_voice_rand_bipolar__rig_dup_411df7f6(&rng);
                    pulse = shimmer;
                }
                /* integrador con fuga: convierte el tren de impulsos en una
                 * fuente con inclinacion espectral realista (~-12dB/oct) */
                glottal_integrator = glottal_integrator * 0.975f + pulse;
                source += glottal_integrator * (1.0f - cur->noise_mix);
            }
            if (cur->noise_mix > 0.0f || !cur->is_voiced) {
                float noise = rig_voice_rand_bipolar__rig_dup_411df7f6(&rng);
                float noise_amt = cur->is_voiced ? cur->noise_mix : 1.0f;
                source += noise * noise_amt * 0.35f;
            }

            float o1 = rig_voice_formant_process__rig_dup_4d24019a(&res1, source, f1, bw1, sample_rate) * g1;
            float o2 = rig_voice_formant_process__rig_dup_4d24019a(&res2, source, f2, bw2, sample_rate) * g2;
            float o3 = rig_voice_formant_process__rig_dup_4d24019a(&res3, source, f3, bw3, sample_rate) * g3;

            float sample = (o1 + o2 + o3) * amp_envelope * 0.15f; /* escala de salida global */
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;

            out_pcm[total_samples++] = sample;
            t_ms += 1000.0f / sample_rate;
        }
    }

    *out_sample_count = total_samples;
    return 0;
}

/* -------------------------------------------------------------------------
 * Escritura de WAV PCM16 mono estandar (RIFF/WAVE)
 * ---------------------------------------------------------------------- */
static void rig_voice_write_u32le__rig_dup_6614e729(FILE *fp, unsigned int v) {
    unsigned char b[4] = { (unsigned char)(v), (unsigned char)(v>>8), (unsigned char)(v>>16), (unsigned char)(v>>24) };
    fwrite(b, 1, 4, fp);
}
static void rig_voice_write_u16le__rig_dup_b26fc3fd(FILE *fp, unsigned short v) {
    unsigned char b[2] = { (unsigned char)(v), (unsigned char)(v>>8) };
    fwrite(b, 1, 2, fp);
}

RIGCOM_PUBLIC int rig_voice_write_wav__rig_dup_6dc95b33(const float *pcm, int sample_count, float sample_rate, FILE *fp) {
    if (!pcm || !fp || sample_count <= 0) return -1;
    unsigned int byte_rate = (unsigned int)(sample_rate * 2.0f);
    unsigned int data_bytes = (unsigned int)sample_count * 2u;

    fwrite("RIFF", 1, 4, fp);
    rig_voice_write_u32le__rig_dup_6614e729(fp, 36 + data_bytes);
    fwrite("WAVE", 1, 4, fp);
    fwrite("fmt ", 1, 4, fp);
    rig_voice_write_u32le__rig_dup_6614e729(fp, 16);
    rig_voice_write_u16le__rig_dup_b26fc3fd(fp, 1);       /* PCM */
    rig_voice_write_u16le__rig_dup_b26fc3fd(fp, 1);       /* mono */
    rig_voice_write_u32le__rig_dup_6614e729(fp, (unsigned int)sample_rate);
    rig_voice_write_u32le__rig_dup_6614e729(fp, byte_rate);
    rig_voice_write_u16le__rig_dup_b26fc3fd(fp, 2);       /* block align */
    rig_voice_write_u16le__rig_dup_b26fc3fd(fp, 16);      /* bits per sample */
    fwrite("data", 1, 4, fp);
    rig_voice_write_u32le__rig_dup_6614e729(fp, data_bytes);

    for (int i = 0; i < sample_count; i++) {
        float s = pcm[i];
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        short v = (short)(s * 32767.0f);
        rig_voice_write_u16le__rig_dup_b26fc3fd(fp, (unsigned short)v);
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Serializacion del guion fonetico (secuencia + contorno de pitch)
 * ---------------------------------------------------------------------- */
typedef struct {
    unsigned int magic;
    unsigned int version;
    unsigned int payload_size;
} RigVoiceFileHeader;

RIGCOM_PUBLIC int rig_voice_script_save__rig_dup_379a1547(const RigVoiceScript *script, FILE *fp) {
    if (!script || !fp) return -1;
    RigVoiceFileHeader hdr = { RIG_VOICE_MAGIC, RIG_VOICE_VERSION, (unsigned int)sizeof(RigVoiceScript) };
    if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (fwrite(script, sizeof(RigVoiceScript), 1, fp) != 1) return -3;
    return 0;
}

RIGCOM_PUBLIC int rig_voice_script_load__rig_dup_1967b32b(RigVoiceScript *script, FILE *fp) {
    if (!script || !fp) return -1;
    RigVoiceFileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) return -2;
    if (hdr.magic != RIG_VOICE_MAGIC) return -3;
    if (hdr.version != RIG_VOICE_VERSION) return -4;
    if (hdr.payload_size != (unsigned int)sizeof(RigVoiceScript)) return -5;
    if (fread(script, sizeof(RigVoiceScript), 1, fp) != 1) return -6;
    return 0;
}
