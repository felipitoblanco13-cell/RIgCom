#include "rigdeps/rig_std_base.h"
#define _POSIX_C_SOURCE 200809L
/* ═══════════════════════════════════════════════════════════════════════
 * rig_voz.c — Motor de voz físico RigCom v35
 *
 * Modelo Liljencrants-Fant (1985) — doi:10.1121/1.392239
 * 4 filtros biquad IIR en cascada (F1-F4 del tracto vocal)
 * Salida: PCM int16 48kHz mono, frame 512 muestras
 * Transporte: WebSocket → base64 → Web Audio API en el editor HTML
 *
 * Estado neuroquímico interno mapeado a eventos del compilador:
 *   BUILD_OK  → dopamina+ oxitocina+   → F0↑, voz viva
 *   BUILD_ERR → cortisol+              → Rd↓, voz tensa
 *   WARNING   → serotonina−            → jitter+
 *   IDLE      → decay gradual hacia base
 *
 * Sin dependencias de RIGADIEL ni Android/OpenSL.
 * φ = 1.6180339887498948482
 * ═══════════════════════════════════════════════════════════════════════ */

#include "../include/rig_voz.h"
#include "../include/wsserver.h"

#include "rigdeps/stdlib.h"
#include "rigdeps/string.h"
#include "rigdeps/stdio.h"
#include "rigdeps/math.h"
#include "rigdeps/stdint.h"
#include "rigdeps/stdbool.h"

/* ── Constantes físicas ──────────────────────────────────────────── */
#define FS              48000.0f
#define PI_F            3.14159265358979f
#define TWO_PI_F        6.28318530717959f
#define FRAME_SIZE      RIG_VOZ_FRAME_SIZE

/* Parámetros base: voz masculina joven 20 años, español neutro */
#define BASE_F0         130.0f   /* Hz — tenor joven                 */
#define BASE_RD         1.2f     /* Rd Rosenberg (1=tenso, 2.5=suave)*/
#define BASE_TP         0.40f    /* Tp/T0 — instante máxima apertura */
#define BASE_TN         0.10f    /* Tn/T0 — duración cierre glótico  */
#define BASE_F1         500.0f   /* Hz — F1                          */
#define BASE_F2         1500.0f  /* Hz — F2                          */
#define BASE_F3         2500.0f  /* Hz — F3                          */
#define BASE_F4         3500.0f  /* Hz — F4                          */
#define BASE_BW1        100.0f
#define BASE_BW2        150.0f
#define BASE_BW3        200.0f
#define BASE_BW4        250.0f
#define BASE_JITTER     0.003f
#define BASE_SHIMMER    0.025f

/* Decay NQ por frame (φ-based: rápido pero suave) */
#define NQ_DECAY        0.0618f  /* 1/φ² ≈ 0.382 / 6 frames ≈ 0.063 */

/* ════════════════════════════════════════════════════════════════════
 * Estado neuroquímico — todos en [0, 1]
 * ════════════════════════════════════════════════════════════════════ */
typedef struct {
    float dopamina;     /* BUILD_OK +0.35  → F0↑, ritmo↑            */
    float serotonina;   /* reposo base     → Rd↑, jitter↓           */
    float cortisol;     /* BUILD_ERR +0.40 → Rd↓, F0↑ tenso         */
    float oxitocina;    /* OK streak +0.20 → F1↑, canto espontáneo  */
    float coherencia;   /* acumulada       → shimmer↓               */
} RigVozNQ;

/* ════════════════════════════════════════════════════════════════════
 * Parámetros de tracto vocal (calculados desde NQ cada frame)
 * ════════════════════════════════════════════════════════════════════ */
typedef struct {
    float f0;           /* Frecuencia fundamental Hz                 */
    float Rd;           /* Redondez glótica actual                   */
    float Tp, Tn;       /* Fracciones de período glótico             */
    float f1, f2, f3, f4;
    float jitter;
    float shimmer;
} RigVozTracto;

/* ════════════════════════════════════════════════════════════════════
 * Filtro biquad IIR — forma directa II transpuesta
 *   y[n] = b0·x[n] + b1·x[n-1] + b2·x[n-2] - a1·y[n-1] - a2·y[n-2]
 * ════════════════════════════════════════════════════════════════════ */
typedef struct {
    float b0, b1, b2;
    float a1, a2;
    float x1, x2;   /* estado entrada  */
    float y1, y2;   /* estado salida   */
} Bicuad;

/* Resonador de Markel & Gray (1976):
 *   r  = exp(-π·bw/FS)
 *   θ  = 2π·fc/FS
 *   b0 = 1-r,  b1 = 0,  b2 = -(1-r)
 *   a1 = -2r·cos(θ),  a2 = r²
 */
static void bicuad_init(Bicuad *f, float fc, float bw) {
    float r  = expf(-PI_F * bw / FS);
    float th = TWO_PI_F * fc / FS;
    f->b0 = 1.0f - r;
    f->b1 = 0.0f;
    f->b2 = -(1.0f - r);
    f->a1 = -2.0f * r * cosf(th);
    f->a2 = r * r;
    f->x1 = f->x2 = f->y1 = f->y2 = 0.0f;
}
static inline float bicuad_proc(Bicuad *f, float x) {
    float y = f->b0*x + f->b1*f->x1 + f->b2*f->x2
            - f->a1*f->y1 - f->a2*f->y2;
    f->x2 = f->x1; f->x1 = x;
    f->y2 = f->y1; f->y1 = y;
    return y;
}
/* ════════════════════════════════════════════════════════════════════
 * Contexto completo (opaque al exterior)
 * ════════════════════════════════════════════════════════════════════ */
struct RigVozCtx {
    RigVozNQ     nq;
    RigVozTracto tracto;
    uint32_t     prng;        /* PRNG para jitter/shimmer              */
    float        t_glot;      /* fase glótica actual [0, T0)           */
    uint32_t     ok_streak;   /* compilaciones OK consecutivas         */
    uint32_t     frame_count;
};

/* ════════════════════════════════════════════════════════════════════
 * Modelo LF — pulso glótico Liljencrants-Fant 1985
 *
 * t_norm ∈ [0, 1)  — tiempo normalizado por T0
 * tp     = Tp/T0   — instante de máxima apertura
 * tn     = Tn/T0   — duración del cierre glótico
 * rd     = Rd      — redondez (1.0 tenso … 2.7 muy suave)
 *
 * Fase apertura  [0, tp]:     E = sin(π·t/tp)
 * Fase cierre    [tp, tp+tn]: E = exp(-dt/τ)·cos(π·dt/tn)
 * Fase retorno   [tp+tn, 1):  E = 0
 * ════════════════════════════════════════════════════════════════════ */
static float lf_pulse(float t_norm, float tp, float tn, float rd) {
    if (t_norm <= tp) {
        return sinf(PI_F * t_norm / (tp + 1e-6f));
    }
    float tc = tp + tn;
    if (t_norm <= tc) {
        float dt  = t_norm - tp;
        float tau = tn / (rd * 0.7f + 0.3f + 1e-6f);
        return expf(-dt / (tau + 1e-6f)) * cosf(PI_F * dt / (tn + 1e-6f));
    }
    return 0.0f;
}
/* ════════════════════════════════════════════════════════════════════
 * NQ → parámetros de tracto vocal
 * ════════════════════════════════════════════════════════════════════ */
static void nq_to_tracto(const RigVozNQ *nq, RigVozTracto *v) {
    /* F0: base 130Hz. Dopamina y cortisol suben, serotonina baja. */
    float f0 = BASE_F0
              + nq->dopamina   * 30.0f
              + nq->cortisol   * 20.0f
              - nq->serotonina * 15.0f
              - nq->oxitocina  * 10.0f;
    v->f0 = f0 < 85.0f ? 85.0f : (f0 > 300.0f ? 300.0f : f0);

    /* Rd: serotonina+oxitocina relajan, cortisol tensa */
    float rd = BASE_RD
              + nq->serotonina * 0.8f
              + nq->oxitocina  * 0.5f
              - nq->cortisol   * 0.6f
              - nq->dopamina   * 0.15f;
    v->Rd = rd < 0.5f ? 0.5f : (rd > 2.7f ? 2.7f : rd);

    /* Tp, Tn: cortisol hace el cierre más brusco */
    float tp = BASE_TP - nq->cortisol * 0.08f + nq->serotonina * 0.05f;
    float tn = BASE_TN + nq->cortisol * 0.05f;
    v->Tp = tp < 0.20f ? 0.20f : (tp > 0.65f ? 0.65f : tp);
    v->Tn = tn < 0.05f ? 0.05f : (tn > 0.30f ? 0.30f : tn);

    /* Formantes */
    float f1 = BASE_F1 + nq->oxitocina  * 120.0f + nq->dopamina * 60.0f - nq->cortisol * 80.0f;
    float f2 = BASE_F2 + nq->oxitocina  * 200.0f                        - nq->cortisol * 100.0f;
    float f3 = BASE_F3 + nq->dopamina   * 80.0f  - nq->serotonina * 60.0f;
    float f4 = BASE_F4 + nq->coherencia * 100.0f - nq->cortisol  * 150.0f;

    v->f1 = f1 < 200.0f  ? 200.0f  : (f1 > 900.0f  ? 900.0f  : f1);
    v->f2 = f2 < 700.0f  ? 700.0f  : (f2 > 3000.0f ? 3000.0f : f2);
    v->f3 = f3 < 2000.0f ? 2000.0f : (f3 > 3500.0f ? 3500.0f : f3);
    v->f4 = f4 < 3000.0f ? 3000.0f : (f4 > 4500.0f ? 4500.0f : f4);

    /* Jitter y shimmer */
    float jit = BASE_JITTER   + nq->cortisol * 0.012f - nq->serotonina * 0.002f;
    float shm = BASE_SHIMMER  + nq->cortisol * 0.030f - nq->serotonina * 0.010f
                              - nq->coherencia * 0.008f;
    v->jitter  = jit < 0.0001f ? 0.0001f : (jit > 0.04f  ? 0.04f  : jit);
    v->shimmer = shm < 0.001f  ? 0.001f  : (shm > 0.08f  ? 0.08f  : shm);
}
/* ════════════════════════════════════════════════════════════════════
 * Síntesis de un frame PCM (FRAME_SIZE muestras @ 48kHz)
 * ════════════════════════════════════════════════════════════════════ */
void rig_voz_synth(RigVozCtx *ctx, int16_t *out_pcm) {
    if (!ctx || !out_pcm) return;

    nq_to_tracto(&ctx->nq, &ctx->tracto);
    const RigVozTracto *v = &ctx->tracto;

    /* 4 resonadores del tracto vocal */
    Bicuad b1, b2, b3, b4;
    bicuad_init(&b1, v->f1, BASE_BW1);
    bicuad_init(&b2, v->f2, BASE_BW2);
    bicuad_init(&b3, v->f3, BASE_BW3);
    bicuad_init(&b4, v->f4, BASE_BW4);

    float T0  = FS / (v->f0 + 1e-6f);
    float amp = 0.82f;
    uint32_t prng = ctx->prng;

    for (int i = 0; i < FRAME_SIZE; i++) {
        /* Jitter: variación de T0 muestra a muestra */
        prng = prng * 1664525u + 1013904223u;
        float jit_f = 1.0f + v->jitter
                    * ((float)(prng & 0xFFFF) / 65535.0f - 0.5f) * 2.0f;
        float T0_j = T0 * jit_f;
        if (T0_j < 160.0f) T0_j = 160.0f;  /* tope 300 Hz */

        /* Pulso LF en la fase actual */
        float t_norm = ctx->t_glot / T0_j;
        if (t_norm > 1.0f) t_norm = 1.0f;
        float glot = lf_pulse(t_norm, v->Tp, v->Tn, v->Rd);

        /* Shimmer: variación de amplitud */
        prng = prng * 1664525u + 1013904223u;
        float shm_f = 1.0f + v->shimmer
                    * ((float)(prng & 0xFF) / 255.0f - 0.5f);
        glot *= amp * shm_f;

        /* Tracto vocal: 4 biquads en cascada */
        float sig = bicuad_proc(&b1, glot);
        sig = bicuad_proc(&b2, sig);
        sig = bicuad_proc(&b3, sig);
        sig = bicuad_proc(&b4, sig);

        /* Cuantizar a int16 con headroom */
        float s = sig * 26000.0f;
        if (s >  32767.0f) s =  32767.0f;
        if (s < -32768.0f) s = -32768.0f;
        out_pcm[i] = (int16_t)s;

        /* Avanzar fase glótica */
        ctx->t_glot += 1.0f;
        if (ctx->t_glot >= T0_j) ctx->t_glot -= T0_j;
    }

    ctx->prng = prng;
    ctx->frame_count++;
}

/* ════════════════════════════════════════════════════════════════════
 * Decay NQ hacia el equilibrio base cada frame
 * ════════════════════════════════════════════════════════════════════ */
static void nq_decay(RigVozNQ *nq) {
#define CLAMP01(x) ((x) < 0.0f ? 0.0f : ((x) > 1.0f ? 1.0f : (x)))
    /* serotonina base 0.45 — reposo */
    nq->dopamina   = CLAMP01(nq->dopamina   - NQ_DECAY * 0.8f);
    nq->cortisol   = CLAMP01(nq->cortisol   - NQ_DECAY * 0.6f);
    nq->serotonina = CLAMP01(nq->serotonina + (0.45f - nq->serotonina) * NQ_DECAY);
    nq->oxitocina  = CLAMP01(nq->oxitocina  - NQ_DECAY * 0.3f);
    nq->coherencia = CLAMP01(nq->coherencia + (0.30f - nq->coherencia) * NQ_DECAY * 0.4f);
#undef CLAMP01
}
/* ════════════════════════════════════════════════════════════════════
 * Base64 encoder — sin newlines, RFC 4648
 * ════════════════════════════════════════════════════════════════════ */
static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static size_t b64_encode__rig_variant_d3e56a9b(const uint8_t *src, size_t len, char *dst) {
    size_t i = 0, o = 0;
    while (i < len) {
        uint32_t b = (uint32_t)src[i++] << 16;
        if (i < len) b |= (uint32_t)src[i++] << 8;
        if (i < len) b |= (uint32_t)src[i++];
        dst[o++] = B64[(b >> 18) & 0x3F];
        dst[o++] = B64[(b >> 12) & 0x3F];
        dst[o++] = (i - 1 < len || (i >= len && i - 1 >= len - 0))
                   ? B64[(b >> 6) & 0x3F] : '=';
        dst[o++] = (i - 0 <= len) ? B64[b & 0x3F] : '=';
    }
    dst[o] = '\0';
    return o;
}
/* ════════════════════════════════════════════════════════════════════
 * Broadcast PCM por WebSocket como base64
 *
 * Evento: {"ev":"voz_frame","rate":48000,"ch":1,"bits":16,"pcm_b64":"..."}
 *
 * El editor HTML recibe el evento y reproduce con:
 *   const buf = atob(msg.pcm_b64)
 *   AudioContext.decodeAudioData(...)
 * ════════════════════════════════════════════════════════════════════ */
static void voz_broadcast_frame(RigVozCtx *ctx, WsServer *srv) {
    int16_t pcm[FRAME_SIZE];
    rig_voz_synth(ctx, pcm);

    /* Base64 del PCM crudo (little-endian int16) */
    size_t pcm_bytes = FRAME_SIZE * sizeof(int16_t);
    size_t b64_len   = ((pcm_bytes + 2) / 3) * 4 + 1;
    char  *b64       = malloc(b64_len);
    if (!b64) return;

    b64_encode__rig_variant_d3e56a9b((const uint8_t *)pcm, pcm_bytes, b64);

    ws_broadcastf(srv,
        "{\"ev\":\"voz_frame\","
        "\"rate\":48000,\"ch\":1,\"bits\":16,"
        "\"pcm_b64\":\"%s\"}",
        b64);

    free(b64);
}
/* ════════════════════════════════════════════════════════════════════
 * API pública
 * ════════════════════════════════════════════════════════════════════ */
RigVozCtx *rig_voz_new(void) {
    RigVozCtx *ctx = calloc(1, sizeof(RigVozCtx));
    if (!ctx) return NULL;

    /* NQ inicial: reposo neutro */
    ctx->nq.serotonina = 0.45f;
    ctx->nq.coherencia = 0.30f;
    ctx->prng          = 0xC0FFEE42u;
    ctx->t_glot        = 0.0f;

    return ctx;
}

void rig_voz_free(RigVozCtx *ctx) {
    free(ctx);
}

void rig_voz_evento(RigVozCtx *ctx, VozEvento ev, WsServer *srv) {
    if (!ctx) return;

    RigVozNQ *nq = &ctx->nq;

    switch (ev) {
    case VOZ_EV_BUILD_OK:
        /* Compilación limpia: dopamina sube, cortisol cae */
        ctx->ok_streak++;
        nq->dopamina   += 0.35f; if (nq->dopamina   > 1.0f) nq->dopamina   = 1.0f;
        nq->cortisol   -= 0.20f; if (nq->cortisol   < 0.0f) nq->cortisol   = 0.0f;
        nq->serotonina += 0.10f; if (nq->serotonina > 1.0f) nq->serotonina = 1.0f;
        /* Racha de OKs consecutivos → oxitocina (modo canto latente) */
        if (ctx->ok_streak >= 3) {
            nq->oxitocina += 0.20f; if (nq->oxitocina > 1.0f) nq->oxitocina = 1.0f;
        }
        nq->coherencia += 0.08f; if (nq->coherencia > 1.0f) nq->coherencia = 1.0f;
        break;

    case VOZ_EV_BUILD_ERR:
        /* Errores: cortisol sube, dopamina cae, racha se rompe */
        ctx->ok_streak = 0;
        nq->cortisol   += 0.40f; if (nq->cortisol   > 1.0f) nq->cortisol   = 1.0f;
        nq->dopamina   -= 0.15f; if (nq->dopamina   < 0.0f) nq->dopamina   = 0.0f;
        nq->serotonina -= 0.08f; if (nq->serotonina < 0.0f) nq->serotonina = 0.0f;
        nq->oxitocina  -= 0.10f; if (nq->oxitocina  < 0.0f) nq->oxitocina  = 0.0f;
        nq->coherencia -= 0.12f; if (nq->coherencia < 0.0f) nq->coherencia = 0.0f;
        break;

    case VOZ_EV_WARNING:
        /* Warnings: leve deterioro */
        nq->serotonina -= 0.05f; if (nq->serotonina < 0.0f) nq->serotonina = 0.0f;
        nq->cortisol   += 0.10f; if (nq->cortisol   > 1.0f) nq->cortisol   = 1.0f;
        break;

    case VOZ_EV_IDLE:
    default:
        /* Sin actividad: decay natural */
        break;
    }

    /* Decay NQ hacia equilibrio */
    nq_decay(nq);

    /* Sintetizar y emitir frame PCM */
    if (srv) voz_broadcast_frame(ctx, srv);
}
