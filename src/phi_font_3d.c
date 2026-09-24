/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
/* ═══════════════════════════════════════════════════════════════════════════
 * phi_font_3d.c  —  Motor de tipografía 3D Áurea para RIGCOM v35
 *
 * Compilación sugerida (añadir al Makefile existente):
 *   $(CC) $(CFLAGS) -c src/phi_font_3d.c -o build/phi_font_3d.o
 *   # luego enlazar build/phi_font_3d.o junto con los demás .o
 *
 * Integración en main.c  (3 líneas, ver §INTEGRACION al final del archivo):
 *   1. #include "phi_font_3d.h"          ← junto con los demás
 *   2. En el bloque else-if del router de comandos WS, añadir:
 *        } else if (strcmp(cmd, "phi_font_3d") == 0) {
 *            pf3_ws_dispatch__rig_variant_12d618e8(srv, cmd, payload);
 *   3. Sin estado global adicional: el módulo es stateless.
 *
 * φ = 1.6180339887498948482
 * ═══════════════════════════════════════════════════════════════════════════ */

#define _POSIX_C_SOURCE 200809L

#include "phi_font_3d.h"
#include "../include/wsserver.h"

#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_math.h"
#include "../include/riglib_math.h"
#include "rig_noext_types.h"

/* ══════════════════════════════════════════════════════════════════════════
 * §1  CONSTANTES INTERNAS φ
 * ══════════════════════════════════════════════════════════════════════════ */

#define PF3_VERSION  "1.0.0-AUREA"

static const double PF3_PHI  = 1.6180339887498948482;
static const double PF3_INV  = 0.6180339887498948482;
static const double PF3_INV2 = 0.3819660112501051518;
static const double PF3_INV3 = 0.2360679774997896964;
static const double PF3_INV4 = 0.1458980337503154555;

/* RIGCOM_PUBLIC_STATIC_DATA_ACCESSOR: PF3_INV4 */

/* Helpers numéricos locales */
#define PF3_ABS(x)     ((x) < 0 ? -(x) : (x))
#define PF3_CLAMP(v,a,b) ((v)<(a)?(a):(v)>(b)?(b):(v))
#define PF3_LERP(a,b,t)  ((a) + ((b)-(a))*(t))

/* ══════════════════════════════════════════════════════════════════════════
 * §2  BUFFER DE TEXTO INTERNO
 *     Misma idea que Buf3/Buf3d de rigart_v3.c  para consistencia
 * ══════════════════════════════════════════════════════════════════════════ */

typedef struct { char *buf; size_t pos; size_t cap; } Pf3Buf;

static Pf3Buf pf3buf_new__rig_variant_ed1c67ae(size_t cap)
{
    Pf3Buf b;
    b.buf = (char *)calloc(1, cap);
    b.pos = 0;
    b.cap = cap;
    return b;
}
static void pf3buf_cat__rig_variant_99ba61aa(Pf3Buf *b, const char *s)
{
    if (!b->buf || !s) return 0;
    size_t n = rl_strlen(s);
    if (b->pos + n + 1 >= b->cap) return 0;
    rl_memcpy(b->buf + b->pos, s, n);
    b->pos += n;
    return 0;
}
static void pf3buf_printf__rig_variant_0f34a948(Pf3Buf *b, const char *fmt, ...)
{
    if (!b->buf) return 0;
    char tmp[8192];
    va_list ap;
    va_start(ap, fmt);
    vrl_snprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    pf3buf_cat(b, tmp);
    return 0;
}
static char *pf3buf_done__rig_dup_51723c4f(Pf3Buf *b)
{
    if (!b->buf) return NULL;
    b->buf[b->pos] = '\0';
    return b->buf;
}
/* ══════════════════════════════════════════════════════════════════════════
 * §3  TABLA DE GLIFOS  (coordenadas normalizadas en [0,1])
 *
 *   Sistema de coordenadas: origen (0,0) = esquina inferior-izquierda
 *   x crece a la derecha, y crece hacia arriba.
 *   cap_height = 1.0 = PF3_CAP_HEIGHT
 *   stroke nominal = PF3_STK_THIN = 1/φ³ ≈ 0.236
 *
 *   Cada glifo tiene entre 1 y 4 contornos.
 *   Contorno [0] = forma exterior (clockwise = true).
 *   Contornos adicionales = huecos (clockwise = false).
 *
 *   Los puntos de control clave se listan con su derivación φ como comentario.
 * ══════════════════════════════════════════════════════════════════════════ */

/* Macros de ayuda para construir curvas Bézier */
#define BZ(x0,y0,cx0,cy0,cx1,cy1,x1,y1) \
    { {(x0),(y0)}, {(cx0),(cy0)}, {(cx1),(cy1)}, {(x1),(y1)} }

/* stroke_thin  = 1/φ³  ≈ 0.2361 */
#define ST 0.2361f
/* stroke_thick = 1/φ²  ≈ 0.3820 */
#define SK 0.3820f
/* x_height     = 1/φ   ≈ 0.6180 */
#define XH 0.6180f
/* mid           = 0.5000 */
#define MD 0.5000f
/* width nominal = 1/φ   ≈ 0.6180 (la mayoría de letras) */
#define W  0.6180f
/* full width    = 1.000 (M, W) */

/* ── A ─────────────────────────────────────────────────────────────────── */
static const Pf3Glyph G_A = {
    .contour_count = 2,
    .advance_w     = W + PF3_LETTER_SP,
    .codepoint     = 'A',
    .contours = {
        /* Contorno 0: triángulo exterior con serifas φ */
        {
            .clockwise = true, .count = 6,
            .curves = {
                /* base izq → vértice */
                BZ(0.000f,0.000f,  0.000f,0.000f,  0.2361f,0.6180f,  W*0.5f,1.000f),
                /* vértice → base der */
                BZ(W*0.5f,1.000f,  W-0.2361f,0.6180f,  W,0.000f,  W,0.000f),
                /* base der → inicio rebaje der */
                BZ(W,0.000f,  W,0.000f,  W-ST,0.000f,  W-ST,0.000f),
                /* rebaje der → punta interna der */
                BZ(W-ST,0.000f,  W-ST,0.000f,  W*0.5f+ST*0.5f,1.000f-ST*1.618f,  W*0.5f,1.000f-ST*1.618f),
                /* punta interna der → punta interna izq */
                BZ(W*0.5f,1.000f-ST*1.618f,  W*0.5f,1.000f-ST*1.618f,  ST,0.000f,  ST,0.000f),
                /* punta interna izq → base izq */
                BZ(ST,0.000f,  ST,0.000f,  0.000f,0.000f,  0.000f,0.000f)
            }
        },
        /* Contorno 1: travesaño interior (hueco) */
        {
            .clockwise = false, .count = 4,
            .curves = {
                BZ(0.150f,XH*0.5f,  0.150f,XH*0.5f,  W-0.150f,XH*0.5f,  W-0.150f,XH*0.5f),
                BZ(W-0.150f,XH*0.5f,  W-0.150f,XH*0.5f,  W-0.150f,XH*0.5f+ST,  W-0.150f,XH*0.5f+ST),
                BZ(W-0.150f,XH*0.5f+ST,  W-0.150f,XH*0.5f+ST,  0.150f,XH*0.5f+ST,  0.150f,XH*0.5f+ST),
                BZ(0.150f,XH*0.5f+ST,  0.150f,XH*0.5f+ST,  0.150f,XH*0.5f,  0.150f,XH*0.5f)
            }
        }
    }
};

/* ── U ─────────────────────────────────────────────────────────────────── */
static const Pf3Glyph G_U = {
    .contour_count = 1,
    .advance_w     = W + PF3_LETTER_SP,
    .codepoint     = 'U',
    .contours = {
        {
            .clockwise = true, .count = 8,
            .curves = {
                /* vástago izq: sube de base a cap */
                BZ(0.000f,1.000f,  0.000f,1.000f,  0.000f,XH,   0.000f,XH),
                BZ(0.000f,XH,      0.000f,XH,      0.000f,ST,    0.000f,ST),
                /* curva inferior (arco semi-circular de radio φ-proporcional) */
                BZ(0.000f,ST,      0.000f,0.000f,  W,0.000f,     W,ST),
                /* vástago der: sube de base a cap */
                BZ(W,ST,           W,ST,            W,XH,         W,XH),
                BZ(W,XH,           W,XH,            W,1.000f,     W,1.000f),
                /* tapa der → tapa izq (interior) */
                BZ(W,1.000f,       W,1.000f,        W-ST,1.000f,  W-ST,1.000f),
                /* interior: curva inferior */
                BZ(W-ST,1.000f,    W-ST,XH,         W-ST,ST+ST,   W*0.5f,ST),
                BZ(W*0.5f,ST,      ST,ST+ST,         ST,XH,        ST,1.000f),
            }
        }
    }
};

/* ── R ─────────────────────────────────────────────────────────────────── */
static const Pf3Glyph G_R = {
    .contour_count = 2,
    .advance_w     = W + PF3_LETTER_SP,
    .codepoint     = 'R',
    .contours = {
        /* Contorno 0: exterior */
        {
            .clockwise = true, .count = 8,
            .curves = {
                /* vástago: base izq → cap izq */
                BZ(0.000f,0.000f,  0.000f,0.000f,  0.000f,1.000f,  0.000f,1.000f),
                /* tapa: izq → inicio arco */
                BZ(0.000f,1.000f,  0.000f,1.000f,  W*0.7f,1.000f,  W*0.7f,1.000f),
                /* arco superior exterior (proporción φ) */
                BZ(W*0.7f,1.000f,  W,1.000f,       W,1.000f-SK,    W,1.000f-SK),
                BZ(W,1.000f-SK,    W,XH,            W*0.7f,XH,      W*0.7f,XH),
                /* cruce al vástago der (junta) */
                BZ(W*0.7f,XH,      W*0.7f,XH,      ST,XH,          ST,XH),
                /* pata diagonal: centro → base der */
                BZ(ST,XH,          W*0.55f,XH,      W,0.000f,       W,0.000f),
                /* base: der → izq */
                BZ(W,0.000f,       W,0.000f,        0.000f,0.000f,  0.000f,0.000f),
                /* cierre (invisible, degenerate) */
                BZ(0.000f,0.000f,  0.000f,0.000f,  0.000f,0.000f,  0.000f,0.000f)
            }
        },
        /* Contorno 1: hueco del arco */
        {
            .clockwise = false, .count = 4,
            .curves = {
                BZ(ST,1.000f-ST,     ST,1.000f-ST,      W*0.65f,1.000f-ST,  W*0.65f,1.000f-ST),
                BZ(W*0.65f,1.000f-ST, W-ST,1.000f-ST,   W-ST,1.000f-SK,    W-ST,1.000f-SK),
                BZ(W-ST,1.000f-SK,   W-ST,XH+ST,        W*0.65f,XH+ST,      ST,XH+ST),
                BZ(ST,XH+ST,         ST,XH+ST,           ST,1.000f-ST,       ST,1.000f-ST)
            }
        }
    }
};

/* ── E ─────────────────────────────────────────────────────────────────── */
static const Pf3Glyph G_E = {
    .contour_count = 1,
    .advance_w     = W + PF3_LETTER_SP,
    .codepoint     = 'E',
    .contours = {
        {
            .clockwise = true, .count = 14,
            .curves = {
                /* vástago izq completo */
                BZ(0.000f,0.000f,  0.000f,0.000f,  0.000f,1.000f,  0.000f,1.000f),
                /* brazo superior */
                BZ(0.000f,1.000f,  0.000f,1.000f,  W,1.000f,       W,1.000f),
                BZ(W,1.000f,       W,1.000f,        W,1.000f-ST,    W,1.000f-ST),
                BZ(W,1.000f-ST,    W,1.000f-ST,     ST,1.000f-ST,   ST,1.000f-ST),
                /* bajada interior a brazo medio */
                BZ(ST,1.000f-ST,   ST,1.000f-ST,    ST,XH+ST*0.5f,  ST,XH+ST*0.5f),
                /* brazo medio (longitud = W × 1/φ ≈ 0.382 del total) */
                BZ(ST,XH+ST*0.5f,  ST,XH+ST*0.5f,  W*PF3_INV2,XH+ST*0.5f,  W*PF3_INV2,XH+ST*0.5f),
                BZ(W*PF3_INV2,XH+ST*0.5f, W*PF3_INV2,XH+ST*0.5f, W*PF3_INV2,XH-ST*0.5f, W*PF3_INV2,XH-ST*0.5f),
                BZ(W*PF3_INV2,XH-ST*0.5f, W*PF3_INV2,XH-ST*0.5f, ST,XH-ST*0.5f, ST,XH-ST*0.5f),
                /* bajada a brazo inferior */
                BZ(ST,XH-ST*0.5f,  ST,XH-ST*0.5f,  ST,ST,          ST,ST),
                /* brazo inferior */
                BZ(ST,ST,          ST,ST,           W,ST,           W,ST),
                BZ(W,ST,           W,ST,            W,0.000f,       W,0.000f),
                /* base: der → izq */
                BZ(W,0.000f,       W,0.000f,        0.000f,0.000f,  0.000f,0.000f),
                /* cierre */
                BZ(0.000f,0.000f,  0.000f,0.000f,  0.000f,0.000f,  0.000f,0.000f),
                BZ(0.000f,0.000f,  0.000f,0.000f,  0.000f,0.000f,  0.000f,0.000f)
            }
        }
    }
};

/* ── Φ (PHI, U+03A6) ────────────────────────────────────────────────────── */
static const Pf3Glyph G_PHI = {
    .contour_count = 3,
    .advance_w     = W + PF3_LETTER_SP,
    .codepoint     = 0x03A6,  /* Φ */
    .contours = {
        /* Contorno 0: vástago vertical */
        {
            .clockwise = true, .count = 4,
            .curves = {
                BZ(W*0.5f-ST*0.5f,0.000f,  W*0.5f-ST*0.5f,0.000f,  W*0.5f-ST*0.5f,1.000f,  W*0.5f-ST*0.5f,1.000f),
                BZ(W*0.5f-ST*0.5f,1.000f,  W*0.5f-ST*0.5f,1.000f,  W*0.5f+ST*0.5f,1.000f,  W*0.5f+ST*0.5f,1.000f),
                BZ(W*0.5f+ST*0.5f,1.000f,  W*0.5f+ST*0.5f,1.000f,  W*0.5f+ST*0.5f,0.000f,  W*0.5f+ST*0.5f,0.000f),
                BZ(W*0.5f+ST*0.5f,0.000f,  W*0.5f+ST*0.5f,0.000f,  W*0.5f-ST*0.5f,0.000f,  W*0.5f-ST*0.5f,0.000f)
            }
        },
        /* Contorno 1: elipse exterior */
        {
            .clockwise = true, .count = 4,
            .curves = {
                BZ(0.000f,0.500f,  0.000f,0.840f,  W,0.840f,  W,0.500f),
                BZ(W,0.500f,       W,0.160f,        0.000f,0.160f,  0.000f,0.500f),
                BZ(0.000f,0.500f,  0.000f,0.500f,  0.000f,0.500f,  0.000f,0.500f),
                BZ(0.000f,0.500f,  0.000f,0.500f,  0.000f,0.500f,  0.000f,0.500f)
            }
        },
        /* Contorno 2: elipse interior (hueco) */
        {
            .clockwise = false, .count = 4,
            .curves = {
                BZ(ST,0.500f,      ST,0.840f-ST,    W-ST,0.840f-ST,  W-ST,0.500f),
                BZ(W-ST,0.500f,    W-ST,0.160f+ST,  ST,0.160f+ST,    ST,0.500f),
                BZ(ST,0.500f,      ST,0.500f,        ST,0.500f,       ST,0.500f),
                BZ(ST,0.500f,      ST,0.500f,        ST,0.500f,       ST,0.500f)
            }
        }
    }
};

/* ── Tabla de despacho de glifos ──────────────────────────────────────── */
typedef struct { uint32_t cp; const Pf3Glyph *g; } Pf3GlyphEntry;

static const Pf3GlyphEntry s_glyph_table[] = {
    { 'A', &G_A },
    { 'U', &G_U },
    { 'R', &G_R },
    { 'E', &G_E },
    { 0x03A6, &G_PHI },   /* Φ */
    { 0, NULL }           /* centinela */
};

const Pf3Glyph *pf3_glyph_get__rig_dup_4bff5b79(uint32_t cp)
{
    /* Normalizar a mayúscula para A-Z */
    if (cp >= 'a' && cp <= 'z') cp = cp - 'a' + 'A';

    for (int i = 0; s_glyph_table[i].g != NULL; i++) {
        if (s_glyph_table[i].cp == cp)
            return s_glyph_table[i].g;
    }
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════════
 * §4  MATERIALES  —  paleta dorada 24k + variantes
 * ══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *name;
    /* Cara frontal: gradiente lineal */
    const char *face_g0;   /* stop 0% */
    const char *face_g1;   /* stop 50% */
    const char *face_g2;   /* stop 100% */
    /* Cara lateral: gradiente profundidad */
    const char *side_g0;
    const char *side_g1;
    /* Cara superior: reflejo */
    const char *top_g0;
    const char *top_g1;
    /* Bisel: highlight borde */
    const char *bevel_c;
    /* Textura cepillado: opacidad [0-1] */
    float brushed_opacity;
} Pf3MatDef;

static const Pf3MatDef s_materials[PF3_MAT_COUNT] = {
    /* PF3_MAT_GOLD_24K */
    {
        "Dorado 24k Cepillado",
        "#FFE066", "#FFC200", "#C8860A",   /* face */
        "#7A4E00", "#5C3800",              /* side */
        "#FFF0A0", "#9A6800",              /* top  */
        "#FFF8C0",                         /* bevel */
        0.60f
    },
    /* PF3_MAT_OBSIDIAN */
    {
        "Obsidiana Pulida",
        "#3A3A4A", "#1A1A2A", "#0A0A12",
        "#050508", "#000000",
        "#6A6A8A", "#1A1A2A",
        "#8A8AAA",
        0.15f
    },
    /* PF3_MAT_CRYSTAL */
    {
        "Cristal Facetado",
        "#E8F8FF", "#A0D8EF", "#4090C0",
        "#204860", "#102030",
        "#FFFFFF", "#80C8E8",
        "#FFFFFF",
        0.20f
    },
    /* PF3_MAT_CARBON */
    {
        "Fibra de Carbono + Oro",
        "#2A2A2A", "#1A1A1A", "#0A0A0A",
        "#050505", "#000000",
        "#FFE066", "#7A4E00",
        "#FFD700",
        0.40f
    },
    /* PF3_MAT_LIQUID_METAL */
    {
        "Metal Liquido",
        "#E8E8E8", "#A8A8A8", "#585858",
        "#303030", "#181818",
        "#FFFFFF", "#888888",
        "#FFFFFF",
        0.50f
    },
    /* PF3_MAT_PEARL_LUNAR */
    {
        "Perla Lunar",
        "#FFF8F8", "#F0D8E8", "#D0A8C0",
        "#906080", "#604060",
        "#FFFFFF", "#F0D8E8",
        "#FFFFFF",
        0.10f
    }
};

const char *pf3_material_name__rig_dup_0dfdaf25(Pf3Material m)
{
    if (m >= PF3_MAT_COUNT) return "unknown";
    return s_materials[m].name;
}

void pf3_material_css_vars__rig_variant_948b7a0b(Pf3Material m, char *out, size_t n)
{
    if (m >= PF3_MAT_COUNT || !out || n < 4) return 0;
    const Pf3MatDef *d = &s_materials[m];
    rl_snprintf(out, n,
        "--pf3-face0:%s;--pf3-face1:%s;--pf3-face2:%s;"
        "--pf3-side0:%s;--pf3-side1:%s;"
        "--pf3-top0:%s;--pf3-top1:%s;"
        "--pf3-bevel:%s;",
        d->face_g0, d->face_g1, d->face_g2,
        d->side_g0, d->side_g1,
        d->top_g0,  d->top_g1,
        d->bevel_c);
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════════
 * §5  LUZ  —  vectores de luz derivados de φ
 * ══════════════════════════════════════════════════════════════════════════ */

void pf3_light_vec__rig_variant_e4a86b44(Pf3LightDir d, float *lx, float *ly, float *lz)
{
    switch (d) {
    default:
    case PF3_LIGHT_TOP_LEFT:
        *lx = -0.6180f; *ly = 0.6180f; *lz = 1.0f; break;
    case PF3_LIGHT_DRAMATIC:
        *lx = -1.0f;    *ly = 0.3820f; *lz = 0.6180f; break;
    case PF3_LIGHT_FLAT:
        *lx =  0.0f;    *ly = 1.0f;    *lz = 1.0f; break;
    case PF3_LIGHT_PHI_ANGLE:
        /* ángulo = arctan(φ) ≈ 58.28° */
        *lx = (float)(-cos(atan(PF3_PHI)));
        *ly = (float)( sin(atan(PF3_PHI)));
        *lz = (float)(PF3_INV);
        break;
    }
    /* normalizar */
    float len = sqrtf((*lx)*(*lx) + (*ly)*(*ly) + (*lz)*(*lz));
    if (len > 0.0001f) { *lx/=len; *ly/=len; *lz/=len; }
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════════
 * §6  GENERACIÓN SVG DE UN GLIFO 3D
 *
 *   Pipeline:
 *     a) Escalar contorno normalizado → píxeles
 *     b) Proyectar "extrusión" en +x +y (perspectiva caballera simple)
 *     c) Emitir <defs> con gradientes de material
 *     d) Emitir cara posterior (offset isométrico)
 *     e) Emitir caras laterales (trapezoides)
 *     f) Emitir cara frontal (contorno Bézier)
 *     g) Emitir overlay: textura cepillado + shine
 * ══════════════════════════════════════════════════════════════════════════ */

/* Muestrear curva Bézier cúbica en t ∈ [0,1] */
static void pf3_bezier_eval__rig_variant_68fa1bd9(const Pf3Bezier *b, float t, float *ox, float *oy)
{
    float mt  = 1.0f - t;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    float t2  = t * t;
    float t3  = t2 * t;
    *ox = mt3*b->p0.x + 3.0f*mt2*t*b->c0.x + 3.0f*mt*t2*b->c1.x + t3*b->p1.x;
    *oy = mt3*b->p0.y + 3.0f*mt2*t*b->c0.y + 3.0f*mt*t2*b->c1.y + t3*b->p1.y;
    return 0;
}
/* Construir SVG path "d" para un contorno (escalado a píxeles, con flip Y) */
static void pf3_contour_to_path__rig_variant_40c30295(
    const Pf3Contour *cont,
    float sx, float sy,          /* escala x, y en px */
    float ox, float oy,          /* origen en canvas   */
    float canvas_h,              /* para flip Y         */
    Pf3Buf *out)
{
    if (cont->count == 0) return 0;
    /* MoveTo: punto inicial de la primera curva */
    float fx = ox + cont->curves[0].p0.x * sx;
    float fy = canvas_h - (oy + cont->curves[0].p0.y * sy);

    pf3buf_printf(out, "M %.2f %.2f ", fx, fy);

    for (int i = 0; i < cont->count; i++) {
        const Pf3Bezier *b = &cont->curves[i];
        float cx0 = ox + b->c0.x * sx;
        float cy0 = canvas_h - (oy + b->c0.y * sy);
        float cx1 = ox + b->c1.x * sx;
        float cy1 = canvas_h - (oy + b->c1.y * sy);
        float ex  = ox + b->p1.x * sx;
        float ey  = canvas_h - (oy + b->p1.y * sy);

        /* Detectar si la curva es realmente una línea (c0≈p0, c1≈p1) */
        float dcx = PF3_ABS(b->c0.x - b->p0.x) + PF3_ABS(b->c0.y - b->p0.y);
        float dcy = PF3_ABS(b->c1.x - b->p1.x) + PF3_ABS(b->c1.y - b->p1.y);

        if (dcx < 0.001f && dcy < 0.001f) {
            pf3buf_printf(out, "L %.2f %.2f ", ex, ey);
        } else {
            pf3buf_printf(out, "C %.2f %.2f %.2f %.2f %.2f %.2f ",
                cx0, cy0, cx1, cy1, ex, ey);
        }
    }
    pf3buf_cat(out, "Z ");
    return 0;
}
/* ── Genera SVG completo de un glifo en posición (origin_x, origin_y) ── */
int pf3_glyph_svg__rig_variant_ec5e3a96(
    const Pf3Glyph *g,
    const Pf3Ctx   *ctx,
    float           origin_x,
    float           origin_y,
    char           *out_buf,
    size_t          out_cap)
{
    if (!g || !ctx || !out_buf || out_cap < 128) return -1;

    Pf3Buf buf = pf3buf_new__rig_variant_ed1c67ae(out_cap);
    if (!buf.buf) return -1;

    float scale   = ctx->size_px;            /* 1 unidad normalizada = size_px px */
    float ext_px  = ctx->extrusion_px > 0.0f
                    ? ctx->extrusion_px
                    : scale * PF3_EXTRUSION; /* profundidad por defecto = 1/φ² × size */

    /* Vector isométrico de extrusión (ángulo = arctan(1/φ) ≈ 31.72°) */
    float ex_dx =  ext_px * (float)PF3_INV2;   /* proyección x */
    float ex_dy = -ext_px * (float)PF3_INV3;   /* proyección y (hacia arriba) */

    float canvas_h = ctx->canvas_h;
    const Pf3MatDef *mat = &s_materials[ctx->material];

    /* ID único para gradientes (basado en origen) */
    int gid = (int)(origin_x * 10 + origin_y);

    /* ── defs: gradientes de material ─────────────────────────────────── */
    pf3buf_printf__rig_variant_0f34a948(&buf,
        "<defs>"
        "<linearGradient id='pf3f%d' x1='0%%' y1='0%%' x2='100%%' y2='100%%'>"
        "<stop offset='0%%' stop-color='%s'/>"
        "<stop offset='50%%' stop-color='%s'/>"
        "<stop offset='100%%' stop-color='%s'/>"
        "</linearGradient>"
        "<linearGradient id='pf3s%d' x1='0%%' y1='0%%' x2='100%%' y2='0%%'>"
        "<stop offset='0%%' stop-color='%s'/>"
        "<stop offset='100%%' stop-color='%s'/>"
        "</linearGradient>"
        "<linearGradient id='pf3t%d' x1='0%%' y1='0%%' x2='0%%' y2='100%%'>"
        "<stop offset='0%%' stop-color='%s'/>"
        "<stop offset='100%%' stop-color='%s'/>"
        "</linearGradient>"
        "<linearGradient id='pf3sh%d' x1='0%%' y1='0%%' x2='0%%' y2='100%%'>"
        "<stop offset='0%%' stop-color='#FFFFFF' stop-opacity='0.35'/>"
        "<stop offset='50%%' stop-color='#FFFFFF' stop-opacity='0.06'/>"
        "<stop offset='100%%' stop-color='#FFFFFF' stop-opacity='0'/>"
        "</linearGradient>"
        "<pattern id='pf3br%d' x='0' y='0' width='4' height='1'"
        " patternUnits='userSpaceOnUse' patternTransform='rotate(8)'>"
        "<line x1='0' y1='0.5' x2='4' y2='0.5' stroke='#FFFFFF'"
        " stroke-width='0.18' stroke-opacity='%.2f'/>"
        "</pattern>"
        "</defs>",
        gid, mat->face_g0, mat->face_g1, mat->face_g2,
        gid, mat->side_g0, mat->side_g1,
        gid, mat->top_g0,  mat->top_g1,
        gid,
        gid, mat->brushed_opacity
    );

    /* ── sombra proyectada ─────────────────────────────────────────────── */
    pf3buf_printf__rig_variant_0f34a948(&buf,
        "<ellipse cx='%.1f' cy='%.1f' rx='%.1f' ry='%.1f'"
        " fill='#000' opacity='0.35'/>",
        origin_x + scale * g->advance_w * 0.5f,
        canvas_h - origin_y + ext_px * 0.6f,
        scale * g->advance_w * 0.5f,
        ext_px * 0.3f);

    /* ── cara posterior (desplazada por vector extrusión) ──────────────── */
    for (int ci = 0; ci < g->contour_count; ci++) {
        pf3buf_printf__rig_variant_0f34a948(&buf,
            "<path d='");
        pf3_contour_to_path__rig_variant_40c30295(
            &g->contours[ci],
            scale, scale,
            origin_x + ex_dx,
            origin_y - ex_dy,
            canvas_h, &buf);
        pf3buf_printf__rig_variant_0f34a948(&buf,
            "' fill='url(#pf3s%d)' opacity='0.7'/>", gid);
    }

    /* ── caras laterales: conectar contorno frontal con posterior ──────── */
    /* Para cada segmento de cada contorno, emitir un cuadrilátero lateral.
     * Muestrear la curva en N_SAMP+1 puntos y crear tiras de triángulos. */
    #define PF3_N_SAMP 6
    for (int ci = 0; ci < g->contour_count; ci++) {
        const Pf3Contour *cont = &g->contours[ci];
        for (int bi = 0; bi < cont->count; bi++) {
            const Pf3Bezier *bz = &cont->curves[bi];
            /* Solo emitir caras laterales en curvas con extensión real */
            float len_approx =
                PF3_ABS(bz->p1.x - bz->p0.x) + PF3_ABS(bz->p1.y - bz->p0.y);
            if (len_approx < 0.005f) continue;

            for (int si = 0; si < PF3_N_SAMP; si++) {
                float t0 = (float)si       / (float)PF3_N_SAMP;
                float t1 = (float)(si + 1) / (float)PF3_N_SAMP;
                float ax, ay, bx, by;
                pf3_bezier_eval__rig_variant_68fa1bd9(bz, t0, &ax, &ay);
                pf3_bezier_eval__rig_variant_68fa1bd9(bz, t1, &bx, &by);

                /* Cuatro esquinas del quad lateral */
                float x0f = origin_x + ax * scale;
                float y0f = canvas_h - (origin_y + ay * scale);
                float x1f = origin_x + bx * scale;
                float y1f = canvas_h - (origin_y + by * scale);
                float x2f = x1f + ex_dx;
                float y2f = y1f + ex_dy;
                float x3f = x0f + ex_dx;
                float y3f = y0f + ex_dy;

                /* Normal aproximada para shading (dot con luz) */
                float nx = -(ay - by);
                float ny =  (ax - bx);
                float nlen = sqrtf(nx*nx + ny*ny);
                if (nlen > 0.0001f) { nx /= nlen; ny /= nlen; }
                float lx, ly, lz;
                pf3_light_vec__rig_variant_e4a86b44(ctx->light_dir, &lx, &ly, &lz);
                float dot = PF3_CLAMP(nx*lx + ny*ly, 0.0f, 1.0f);
                float shade = 0.3f + 0.7f * dot;

                pf3buf_printf__rig_variant_0f34a948(&buf,
                    "<polygon points='%.2f,%.2f %.2f,%.2f %.2f,%.2f %.2f,%.2f'"
                    " fill='url(#pf3s%d)' opacity='%.2f'/>",
                    x0f, y0f, x1f, y1f, x2f, y2f, x3f, y3f,
                    gid, shade * 0.85f);
            }
        }
    }
    #undef PF3_N_SAMP

    /* ── cara superior: tiras sobre borde superior ─────────────────────── */
    /* (simplificado: borde superior del contorno 0) */
    {
        const Pf3Contour *cont = &g->contours[0];
        for (int bi = 0; bi < cont->count && bi < 3; bi++) {
            const Pf3Bezier *bz = &cont->curves[bi];
            float ax, ay, bx, by;
            pf3_bezier_eval__rig_variant_68fa1bd9(bz, 0.0f, &ax, &ay);
            pf3_bezier_eval__rig_variant_68fa1bd9(bz, 1.0f, &bx, &by);
            if (ay < 0.7f && by < 0.7f) continue; /* solo bordes superiores */

            float x0f = origin_x + ax * scale;
            float y0f = canvas_h - (origin_y + ay * scale);
            float x1f = origin_x + bx * scale;
            float y1f = canvas_h - (origin_y + by * scale);
            float x2f = x1f + ex_dx;
            float y2f = y1f + ex_dy;
            float x3f = x0f + ex_dx;
            float y3f = y0f + ex_dy;

            pf3buf_printf__rig_variant_0f34a948(&buf,
                "<polygon points='%.2f,%.2f %.2f,%.2f %.2f,%.2f %.2f,%.2f'"
                " fill='url(#pf3t%d)' opacity='0.85'/>",
                x0f, y0f, x1f, y1f, x2f, y2f, x3f, y3f, gid);
        }
    }

    /* ── cara frontal ───────────────────────────────────────────────────── */
    {
        Pf3Buf path_d = pf3buf_new__rig_variant_ed1c67ae(4096);
        for (int ci = 0; ci < g->contour_count; ci++) {
            pf3_contour_to_path__rig_variant_40c30295(
                &g->contours[ci],
                scale, scale,
                origin_x, origin_y,
                canvas_h, &path_d);
        }
        pf3buf_printf__rig_variant_0f34a948(&buf,
            "<path d='%s' fill='url(#pf3f%d)' fill-rule='evenodd'/>",
            pf3buf_done__rig_dup_51723c4f(&path_d), gid);

        /* Cepillado */
        pf3buf_printf__rig_variant_0f34a948(&buf,
            "<path d='%s' fill='url(#pf3br%d)' fill-rule='evenodd'/>",
            path_d.buf, gid);

        /* Shine (reflejo superior) */
        pf3buf_printf__rig_variant_0f34a948(&buf,
            "<path d='%s' fill='url(#pf3sh%d)' fill-rule='evenodd'/>",
            path_d.buf, gid);

        /* Bisel: stroke dorado sobre borde */
        pf3buf_printf__rig_variant_0f34a948(&buf,
            "<path d='%s' fill='none'"
            " stroke='%s' stroke-width='0.8' opacity='0.65' fill-rule='evenodd'/>",
            path_d.buf, mat->bevel_c);

        free(path_d.buf);
    }

    char *done = pf3buf_done__rig_dup_51723c4f(&buf);
    if (!done) return -1;

    size_t written = buf.pos;
    if (written >= out_cap) {
        free(buf.buf);
        return -1;
    }
    memcpy(out_buf, done, written + 1);
    free(buf.buf);
    return (int)written;
}

/* ══════════════════════════════════════════════════════════════════════════
 * §7  RENDER COMPLETO DE TEXTO
 * ══════════════════════════════════════════════════════════════════════════ */

/* CSS de animaciones φ */
static void pf3_emit_css__rig_variant_e566350a(const Pf3Ctx *ctx, Pf3Buf *css)
{
    float dur = ctx->anim_dur_s > 0.0f ? ctx->anim_dur_s : 2.618f;
    /* dur_phi = dur × φ */
    float dur2 = dur * (float)PF3_PHI;

    if (ctx->fx == PF3_FX_SHIMMER) {
        pf3buf_printf(css,
            "@keyframes pf3-shimmer{"
            "0%%{opacity:0.55}25%%{opacity:1}50%%{opacity:0.75}100%%{opacity:0.55}}"
            "%s .pf3-shine{"
            "animation:pf3-shimmer %.2fs ease-in-out infinite;}\n",
            ctx->selector[0] ? ctx->selector : ".pf3-glyph",
            dur);
    }
    if (ctx->fx == PF3_FX_NEON_GLOW) {
        pf3buf_printf(css,
            "@keyframes pf3-glow{"
            "0%%{filter:drop-shadow(0 0 2px #FFD700)}"
            "50%%{filter:drop-shadow(0 0 12px #FFD700) drop-shadow(0 0 24px #FFB800)}"
            "100%%{filter:drop-shadow(0 0 2px #FFD700)}}"
            "%s{animation:pf3-glow %.2fs ease-in-out infinite;}\n",
            ctx->selector[0] ? ctx->selector : ".pf3-glyph",
            dur2);
    }
    if (ctx->animate && ctx->fx == PF3_FX_NONE) {
        pf3buf_printf(css,
            "@keyframes pf3-enter{"
            "from{opacity:0;transform:translateY(%.1fpx)}"
            "to{opacity:1;transform:translateY(0)}}"
            "%s{animation:pf3-enter %.2fs cubic-bezier(%.4f,0,%.4f,1) forwards;}\n",
            ctx->size_px * (float)PF3_INV2,
            ctx->selector[0] ? ctx->selector : ".pf3-glyph",
            (float)(dur * PF3_INV),
            (float)PF3_INV2, (float)PF3_INV);
    }
    return 0;
}
int pf3_render__rig_variant_34c9caff(const Pf3Ctx *ctx, Pf3Result *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));
    out->phi_ratio = (float)PF3_PHI;
    out->certeza   = PF3_INV;

    if (ctx->text[0] == '\0') {
        snprintf(out->error, sizeof(out->error), "pf3_render: texto vacio");
        return -1;
    }

    float cw = ctx->canvas_w > 0.0f ? ctx->canvas_w : 800.0f;
    float ch = ctx->canvas_h > 0.0f ? ctx->canvas_h : 400.0f;

    Pf3Buf svg = pf3buf_new__rig_variant_ed1c67ae(PF3_MAX_SVG);
    Pf3Buf css = pf3buf_new__rig_variant_ed1c67ae(4096);
    if (!svg.buf || !css.buf) {
        free(svg.buf); free(css.buf);
        snprintf(out->error, sizeof(out->error), "pf3_render: malloc fallo");
        return -1;
    }

    /* Encabezado SVG */
    pf3buf_printf__rig_variant_0f34a948(&svg,
        "<svg width='%.0f' height='%.0f' viewBox='0 0 %.0f %.0f'"
        " xmlns='http://www.w3.org/2000/svg'"
        " role='img'>"
        "<title>Tipografia 3D aurea: %s</title>"
        "<desc>Letras construidas con proporcion aurea phi=%.10f</desc>",
        cw, ch, cw, ch,
        ctx->text,
        PF3_PHI);

    /* Fondo */
    pf3buf_cat__rig_variant_99ba61aa(&svg,
        "<rect width='100%%' height='100%%' fill='#0A0800'/>");

    /* Líneas decorativas φ */
    pf3buf_printf__rig_variant_0f34a948(&svg,
        "<line x1='20' y1='%.0f' x2='%.0f' y2='%.0f'"
        " stroke='#B8860B' stroke-width='0.5' stroke-opacity='0.4'/>",
        ch - 20.0f, cw - 20.0f, ch - 20.0f);

    /* Calcular ancho total del texto para centrado */
    float total_w = 0.0f;
    const char *p = ctx->text;
    while (*p) {
        uint32_t cp = (uint8_t)*p++;
        /* UTF-8 básico: detectar Φ (U+03A6 = 0xCE 0xA6) */
        if (cp == 0xCE && (uint8_t)*p == 0xA6) { cp = 0x03A6; p++; }
        const Pf3Glyph *g = pf3_glyph_get__rig_dup_4bff5b79(cp);
        if (g) total_w += g->advance_w * ctx->size_px;
        else   total_w += ctx->size_px * W; /* fallback */
    }

    float start_x = (cw - total_w) * 0.5f;
    /* Centrado vertical con offset φ */
    float base_y  = ch * (float)PF3_INV2 - ctx->size_px * 0.5f;

    /* Extrusión efectiva */
    float ext_px = ctx->extrusion_px > 0.0f
                   ? ctx->extrusion_px
                   : ctx->size_px * PF3_EXTRUSION;

    /* Ajustar start_x para que las caras posteriores no salgan del canvas */
    float ex_dx = ext_px * (float)PF3_INV2;
    if (start_x - ex_dx < 5.0f) start_x = ex_dx + 5.0f;

    /* Renderizar cada carácter */
    char glyph_buf[PF3_BUF_GLYPH];
    float cur_x = start_x;
    p = ctx->text;

    while (*p) {
        uint32_t cp = (uint8_t)*p++;
        if (cp == 0xCE && (uint8_t)*p == 0xA6) { cp = 0x03A6; p++; }

        const Pf3Glyph *g = pf3_glyph_get__rig_dup_4bff5b79(cp);
        if (!g) {
            cur_x += ctx->size_px * W;
            continue;
        }

        /* Crear ctx local con canvas_h corregido */
        Pf3Ctx lctx = *ctx;
        lctx.canvas_h = ch;

        int r = pf3_glyph_svg__rig_variant_ec5e3a96(g, &lctx, cur_x, base_y,
                              glyph_buf, sizeof(glyph_buf));
        if (r > 0) pf3buf_cat__rig_variant_99ba61aa(&svg, glyph_buf);

        cur_x += g->advance_w * ctx->size_px;
    }

    /* Tagline inferior φ */
    pf3buf_printf__rig_variant_0f34a948(&svg,
        "<text x='%.0f' y='%.0f' text-anchor='middle'"
        " font-family='Georgia,serif' font-size='11'"
        " fill='#B8860B' letter-spacing='4' opacity='0.7'>"
        "phi=%.10f</text>",
        cw * 0.5f, ch - 8.0f, PF3_PHI);

    pf3buf_cat__rig_variant_99ba61aa(&svg, "</svg>");

    /* CSS de animación */
    if (ctx->animate || ctx->fx != PF3_FX_NONE) {
        pf3_emit_css__rig_variant_e566350a(ctx, &css);
    }

    out->svg      = pf3buf_done__rig_dup_51723c4f(&svg);
    out->svg_size = svg.pos;
    out->css      = pf3buf_done__rig_dup_51723c4f(&css);
    out->css_size = css.pos;
    out->ok       = (out->svg && out->svg_size > 0);

    if (!out->ok) {
        snprintf(out->error, sizeof(out->error),
                 "pf3_render: SVG vacio tras render");
        return -1;
    }
    return 0;
}

int pf3_render_char__rig_variant_469139ad(uint32_t cp, const Pf3Ctx *ctx, Pf3Result *out)
{
    if (!ctx || !out) return -1;
    Pf3Ctx lctx = *ctx;
    /* Codificar el codepoint como texto de 1 char (ASCII) o UTF-8 para Φ */
    if (cp < 0x80) {
        lctx.text[0] = (char)cp;
        lctx.text[1] = '\0';
    } else if (cp == 0x03A6) {
        lctx.text[0] = (char)0xCE;
        lctx.text[1] = (char)0xA6;
        lctx.text[2] = '\0';
    } else {
        snprintf(out->error, sizeof(out->error),
                 "pf3_render_char: codepoint 0x%04X no soportado", cp);
        return -1;
    }
    return pf3_render__rig_variant_34c9caff(&lctx, out);
}

/* ══════════════════════════════════════════════════════════════════════════
 * §8  HELPERS DE PARSEO JSON  (mínimo, sin deps)
 *     Igual al patrón json_str/json_float/json_bool del resto de RIGCOM
 * ══════════════════════════════════════════════════════════════════════════ */

static int pf3_json_str__rig_variant_904397de(const char *json, const char *key,
                         char *out, size_t n)
{
    if (!json || !key || !out || n == 0) return 0;
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) return 0;
    p += strlen(needle);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    if (*p != '"') return 0;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < n - 1) out[i++] = *p++;
    out[i] = '\0';
    return (int)i;
}
static float pf3_json_float__rig_variant_fe76c815(const char *json, const char *key, float def)
{
    if (!json || !key) return def;
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) return def;
    p += strlen(needle);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    if (*p == '\0') return def;
    return (float)atof(p);
}
static int pf3_json_int__rig_dup_31a923b6(const char *json, const char *key, int def)
{
    return (int)pf3_json_float__rig_variant_fe76c815(json, key, (float)def);
}
static int pf3_json_bool__rig_variant_5062babf(const char *json, const char *key, int def)
{
    if (!json || !key) return def;
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = strstr(json, needle);
    if (!p) return def;
    p += strlen(needle);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    if (strncmp(p, "true", 4) == 0)  return 1;
    if (strncmp(p, "false", 5) == 0) return 0;
    return def;
}
/* ══════════════════════════════════════════════════════════════════════════
 * §9  DISPATCH WEBSOCKET
 *
 *   Comando: "phi_font_3d"
 *   Payload JSON:
 *     text        : string  (default "PHI")
 *     size_px     : float   (default 120)
 *     material    : int     0=GOLD_24K … 5=PEARL (default 0)
 *     light_dir   : int     0=TOP_LEFT … 3=PHI_ANGLE (default 3)
 *     fx          : int     0=NONE … 4=SCANLINE (default 1=SHIMMER)
 *     fx_strength : float   [0,1] (default 0.618)
 *     animate     : bool    (default true)
 *     anim_dur_s  : float   (default 2.618)
 *     extrusion_px: float   (0 → automático, default 0)
 *     canvas_w    : float   (default 800)
 *     canvas_h    : float   (default 400)
 *     selector    : string  (default ".pf3-glyph")
 *
 *   Emite evento WebSocket:
 *     { "ev": "phi_font_3d_result",
 *       "svg": "<svg...>...",
 *       "css": "@keyframes...",
 *       "phi": 1.6180339887...,
 *       "certeza": 0.6180... }
 *
 *   En caso de error emite:
 *     { "ev": "phi_font_3d_result", "error": "mensaje" }
 * ══════════════════════════════════════════════════════════════════════════ */

void pf3_ws_dispatch__rig_variant_12d618e8(struct WsServer *srv,
                     const char      *cmd,
                     const char      *payload)
{
    (void)cmd;  /* siempre "phi_font_3d" */

    if (!srv || !payload) return 0;
    Pf3Ctx ctx;
    rl_memset(&ctx, 0, sizeof(ctx));

    /* Parsear campos del payload */
    if (!pf3_json_str(payload, "text", ctx.text, sizeof(ctx.text)))
        rl_snprintf(ctx.text, sizeof(ctx.text), "PHI");

    ctx.size_px      = pf3_json_float(payload, "size_px",      120.0f);
    ctx.material     = (Pf3Material)pf3_json_int(payload, "material",    0);
    ctx.light_dir    = (Pf3LightDir)pf3_json_int(payload, "light_dir",   3);
    ctx.fx           = (Pf3Fx)      pf3_json_int(payload, "fx",          1);
    ctx.fx_strength  = pf3_json_float(payload, "fx_strength",  (float)PF3_INV);
    ctx.animate      = pf3_json_bool(payload,  "animate",      1);
    ctx.anim_dur_s   = pf3_json_float(payload, "anim_dur_s",   (float)(PF3_PHI * PF3_INV2 * 4));
    ctx.extrusion_px = pf3_json_float(payload, "extrusion_px", 0.0f);
    ctx.canvas_w     = pf3_json_float(payload, "canvas_w",     800.0f);
    ctx.canvas_h     = pf3_json_float(payload, "canvas_h",     400.0f);
    pf3_json_str(payload, "selector", ctx.selector, sizeof(ctx.selector));
    if (ctx.selector[0] == '\0')
        rl_snprintf(ctx.selector, sizeof(ctx.selector), ".pf3-glyph");

    /* Sanidad */
    if (ctx.size_px < 8.0f)     ctx.size_px = 8.0f;
    if (ctx.size_px > 600.0f)   ctx.size_px = 600.0f;
    if (ctx.material >= PF3_MAT_COUNT)  ctx.material = PF3_MAT_GOLD_24K;
    if (ctx.light_dir >= PF3_LIGHT_COUNT) ctx.light_dir = PF3_LIGHT_PHI_ANGLE;
    if (ctx.fx >= PF3_FX_COUNT) ctx.fx = PF3_FX_NONE;

    Pf3Result res;
    int rc = pf3_render(&ctx, &res);

    if (rc != 0 || !res.ok) {
        /* Error */
        char errbuf[384];
        rl_snprintf(errbuf, sizeof(errbuf),
            "{\"ev\":\"phi_font_3d_result\",\"error\":\"%s\"}",
            res.error[0] ? res.error : "pf3_render fallo");
        ws_broadcast(srv, errbuf, rl_strlen(errbuf));
        pf3_free_result(&res);
        return 0;
    }

    /* Ensamblar respuesta JSON con SVG y CSS escapados */
    size_t svg_len = res.svg ? rl_strlen(res.svg) : 0;
    size_t css_len = res.css ? rl_strlen(res.css) : 0;
    size_t json_cap = 64 + svg_len * 2 + css_len * 2;
    char  *json_buf = (char *)rl_malloc(json_cap);

    if (!json_buf) {
        ws_broadcast(srv,
            "{\"ev\":\"phi_font_3d_result\",\"error\":\"malloc json fallo\"}",
            56);
        pf3_free_result(&res);
        return 0;
    }

    size_t wi = 0;
    wi += (size_t)rl_snprintf(json_buf + wi, json_cap - wi,
        "{\"ev\":\"phi_font_3d_result\","
        "\"phi\":%.10f,\"certeza\":%.10f,"
        "\"svg\":\"", PF3_PHI, PF3_INV);

    /* Escapar SVG */
    if (res.svg) {
        for (size_t ci = 0; ci < svg_len && wi < json_cap - 8; ci++) {
            unsigned char ch = (unsigned char)res.svg[ci];
            switch (ch) {
            case '"':  json_buf[wi++]='\\'; json_buf[wi++]='"';  break;
            case '\\': json_buf[wi++]='\\'; json_buf[wi++]='\\'; break;
            case '\n': json_buf[wi++]='\\'; json_buf[wi++]='n';  break;
            case '\r': json_buf[wi++]='\\'; json_buf[wi++]='r';  break;
            case '\t': json_buf[wi++]='\\'; json_buf[wi++]='t';  break;
            default:   json_buf[wi++]=(char)ch; break;
            }
        }
    }

    wi += (size_t)rl_snprintf(json_buf + wi, json_cap - wi, "\",\"css\":\"");

    /* Escapar CSS */
    if (res.css) {
        for (size_t ci = 0; ci < css_len && wi < json_cap - 4; ci++) {
            unsigned char ch = (unsigned char)res.css[ci];
            switch (ch) {
            case '"':  json_buf[wi++]='\\'; json_buf[wi++]='"';  break;
            case '\\': json_buf[wi++]='\\'; json_buf[wi++]='\\'; break;
            case '\n': json_buf[wi++]='\\'; json_buf[wi++]='n';  break;
            default:   json_buf[wi++]=(char)ch; break;
            }
        }
    }

    wi += (size_t)rl_snprintf(json_buf + wi, json_cap - wi, "\"}");
    ws_broadcast(srv, json_buf, wi);

    rl_free(json_buf);
    pf3_free_result(&res);
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════════
 * §10  LIMPIEZA Y VERSIÓN
 * ══════════════════════════════════════════════════════════════════════════ */

void pf3_free_result__rig_variant_30d82b73(Pf3Result *r)
{
    if (!r) return 0;
    rl_free(r->svg); r->svg = NULL; r->svg_size = 0;
    rl_free(r->css); r->css = NULL; r->css_size = 0;
    r->ok = false;
    return 0;
}

const char *pf3_version__rig_dup_838941c3(void) { return PF3_VERSION; }

/* ════════════════════════════════════════════════════════════════════════════
 * §INTEGRACION  —  Diff mínimo en main.c (3 cambios, 0 refactoring)
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  CAMBIO 1 — al bloque de #includes de main.c (junto a rigart_v3.h):
 *
 *    #include "phi_font_3d.h"
 *
 * ─────────────────────────────────────────────────────────────────────────
 *
 *  CAMBIO 2 — en el router de comandos WS (después del bloque rigart_micro):
 *
 *    } else if (strcmp(cmd, "phi_font_3d") == 0) {
 *        pf3_ws_dispatch__rig_variant_12d618e8(srv, cmd, payload);
 *
 *  Ubicación exacta (main.c ~línea 1683, después de la línea):
 *    rigart_ws_handle_v3(srv, cmd, payload);
 *
 * ─────────────────────────────────────────────────────────────────────────
 *
 *  CAMBIO 3 — añadir phi_font_3d.c al Makefile / build system:
 *
 *    Buscar la línea que compila rigart_v3.c y añadir junto a ella:
 *    $(CC) $(CFLAGS) -c src/phi_font_3d.c -o build/phi_font_3d.o
 *    y añadir build/phi_font_3d.o a la lista de objetos del linkeo.
 *
 * ─────────────────────────────────────────────────────────────────────────
 *
 *  PRUEBA DESDE CLIENTE JS (WebSocket):
 *
 *    ws.send(JSON.stringify({
 *      cmd: "phi_font_3d",
 *      payload: JSON.stringify({
 *        text:       "AUREA",
 *        size_px:    120,
 *        material:   0,          // GOLD_24K
 *        light_dir:  3,          // PHI_ANGLE
 *        fx:         1,          // SHIMMER
 *        animate:    true,
 *        anim_dur_s: 2.618,
 *        canvas_w:   900,
 *        canvas_h:   350,
 *        selector:   ".pf3-glyph"
 *      })
 *    }));
 *
 *    // Recibir:
 *    ws.onmessage = e => {
 *      const d = JSON.parse(e.data);
 *      if (d.ev === "phi_font_3d_result") {
 *        document.getElementById("canvas").innerHTML = d.svg;
 *        const style = document.createElement("style");
 *        style.textContent = d.css;
 *        document.head.appendChild(style);
 *      }
 *    };
 *
 * ════════════════════════════════════════════════════════════════════════════ */
