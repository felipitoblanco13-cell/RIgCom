/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
/* ═══════════════════════════════════════════════════════════════════════════
 * rigcom_fenestrae_scene.c  —  Escena animada "RIGCOM Fenestrae"
 *
 * Renderiza la imagen de referencia usando el Software Rasterizer PBR
 * soberano de rig_master.c.  Cero dependencias externas.
 *
 * Autor: Richard Felipe Urbina
 * φ = 1.6180339887498948482
 *
 * §1   Constantes φ + geometría
 * §2   Mat4 soberana (column-major — igual que sr_mm4 / sr_mv4)
 * §3   Geometría: dodecaedro φ-exacto (12 caras × 3 tris = 36 tris)
 * §4   Geometría: espiral Fibonacci 3D (89 segmentos → billboards)
 * §5   Geometría: campo de partículas 144 (ángulo áureo)
 * §6   Geometría: octaedro para partícula unitaria
 * §7   API pública de la escena
 *      fens_init()            — crea GPU, PBR, mallas, programas
 *      fens_frame(time_s)     — renderiza un frame animado
 *      fens_destroy()         — libera todo
 *      fens_framebuffer()     — puntero al buffer RGBA final
 *      fens_fb_width/height() — dimensiones
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "../include/rig_master.h"
#include "../include/riglib.h"
#include "../include/riglib_math.h"
#include "rig_noext_types.h"
#include "rig_noext_types.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * §1  Constantes φ
 * ═══════════════════════════════════════════════════════════════════════════ */

#define FEN_PHI        1.6180339887498948482f
#define FEN_PHI_INV    0.6180339887498948482f
#define FEN_PHI_INV2   0.3819660112501051518f
#define FEN_GOLDEN_ANG 2.39996322972865332f   /* 2π / φ² rad */
#define FEN_PI         3.14159265358979323846f
#define FEN_2PI        6.28318530717958647692f
#define FEN_SQRT3      1.73205080756887729353f

/* Resolución portrait — misma proporción que la imagen */
#define FEN_W  1080u
#define FEN_H  1920u

/* Número de vértices por elemento de la escena */
#define FEN_DODECA_VERTS   60u   /* 12 caras × 5 verts, sin compartir */
#define FEN_DODECA_IDX    108u   /* 12 caras × 3 tris × 3 índices     */
#define FEN_SPIRAL_SEGS    88u   /* 89 puntos → 88 segmentos billboard */
#define FEN_SPIRAL_VERTS  352u   /* 88 × 4 verts por quad             */
#define FEN_SPIRAL_IDX    528u   /* 88 × 6 índices por quad           */
#define FEN_PART_COUNT    144u   /* número de Fibonacci — ángulo áureo */
#define FEN_OCT_VERTS       6u   /* octaedro unitario                 */
#define FEN_OCT_IDX        24u   /* 8 caras × 3 índices               */
#define FEN_PART_VERTS    864u   /* 144 × 6 verts                     */
#define FEN_PART_IDX     3456u   /* 144 × 24 índices                  */

/* SR_FLOATS_PER_VERT = 15: [x y z | nx ny nz | tx ty tz | u v | r g b a] */
#define FEN_FPV 15u

/* ═══════════════════════════════════════════════════════════════════════════
 * §2  Mat4 soberana — column-major, misma que sr_mm4 / sr_mv4
 * ═══════════════════════════════════════════════════════════════════════════ */

static void fm_id(float m[16])
{
    rl_memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

/* o = a × b  (ambas column-major) */
static void fm_mul(float o[16], const float a[16], const float b[16])
{
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float s = 0.f;
            for (int k = 0; k < 4; k++) s += a[r + k*4] * b[k + c*4];
            o[r + c*4] = s;
        }
}

/* Perspectiva estándar (ángulo en radianes) */
static void fm_perspective(float m[16], float fov_rad, float aspect,
                            float znear, float zfar)
{
    fm_id(m);
    float f = 1.f / rl_tanf(fov_rad * 0.5f);
    float nf = 1.f / (znear - zfar);
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (zfar + znear) * nf;
    m[11] = -1.f;
    m[14] = 2.f * zfar * znear * nf;
    m[15] = 0.f;
}

/* Traslación */
static void fm_translate(float m[16], float tx, float ty, float tz)
{
    fm_id(m);
    m[12] = tx; m[13] = ty; m[14] = tz;
}

/* Escala uniforme */
static void fm_scale(float m[16], float s)
{
    fm_id(m);
    m[0] = m[5] = m[10] = s;
}

/* Rotación alrededor del eje Y */
static void fm_rotate_y(float m[16], float rad)
{
    fm_id(m);
    float c = rl_cosf(rad), s = rl_sinf(rad);
    m[0]  =  c;  m[2]  = s;
    m[8]  = -s;  m[10] = c;
}

/* Rotación alrededor del eje X */
static void fm_rotate_x(float m[16], float rad)
{
    fm_id(m);
    float c = rl_cosf(rad), s = rl_sinf(rad);
    m[5]  =  c;  m[6]  = -s;
    m[9]  =  s;  m[10] =  c;
}

/* Rotación alrededor del eje Z */
static void fm_rotate_z(float m[16], float rad)
{
    fm_id(m);
    float c = rl_cosf(rad), s = rl_sinf(rad);
    m[0]  =  c;  m[1]  = -s;
    m[4]  =  s;  m[5]  =  c;
}

/* MVP = proj × view × model */
static void fm_mvp(float out[16],
                   const float proj[16],
                   const float view[16],
                   const float model[16])
{
    float vp[16], tmp[16];
    fm_mul(vp, proj, view);
    fm_mul(tmp, vp, model);
    rl_memcpy(out, tmp, 64);
}

/* asin/acos soberanas — riglib_math.h NO declara rl_asinf/rl_acosf
 * (solo sin, cos, tan, atan2, sqrt). Se reconstruyen con la identidad
 * estándar asin(x) = atan2(x, sqrt(1-x²)), exacta en [-1,1].          */
static inline float fen_asinf(float x)
{
    x = x < -1.f ? -1.f : (x > 1.f ? 1.f : x);
    return rl_atan2f(x, rl_sqrtf(1.f - x*x));
}
static inline float fen_acosf(float x)
{
    x = x < -1.f ? -1.f : (x > 1.f ? 1.f : x);
    return rl_atan2f(rl_sqrtf(1.f - x*x), x);
}

/* Normalizar vec3 inline */
static void fn_norm(float v[3])
{
    float l = rl_sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (l < 1e-7f) return;
    float inv = 1.f / l;
    v[0] *= inv; v[1] *= inv; v[2] *= inv;
}

/* Cross product */
static void fn_cross(const float a[3], const float b[3], float o[3])
{
    o[0] = a[1]*b[2] - a[2]*b[1];
    o[1] = a[2]*b[0] - a[0]*b[2];
    o[2] = a[0]*b[1] - a[1]*b[0];
}

/* ═══════════════════════════════════════════════════════════════════════════
 * §3  Dodecaedro regular — 20 vértices φ-exactos, 12 caras pentagonales
 *     Coordenadas directas de rigart_geo_extra.c (mismo origen canónico)
 * ═══════════════════════════════════════════════════════════════════════════ */

static const float FEN_DV[20][3] = {
    /* 0–7   cubo (±1, ±1, ±1)            */
    {-1.f,-1.f,-1.f}, { 1.f,-1.f,-1.f},
    { 1.f, 1.f,-1.f}, {-1.f, 1.f,-1.f},
    {-1.f,-1.f, 1.f}, { 1.f,-1.f, 1.f},
    { 1.f, 1.f, 1.f}, {-1.f, 1.f, 1.f},
    /* 8–11  rectángulo Y: (0, ±φ, ±1/φ) */
    { 0.f,-FEN_PHI,-FEN_PHI_INV}, { 0.f, FEN_PHI,-FEN_PHI_INV},
    { 0.f, FEN_PHI, FEN_PHI_INV}, { 0.f,-FEN_PHI, FEN_PHI_INV},
    /* 12–15 rectángulo Z: (±1/φ, 0, ±φ) */
    {-FEN_PHI_INV, 0.f,-FEN_PHI}, { FEN_PHI_INV, 0.f,-FEN_PHI},
    { FEN_PHI_INV, 0.f, FEN_PHI}, {-FEN_PHI_INV, 0.f, FEN_PHI},
    /* 16–19 rectángulo X: (±φ, ±1/φ, 0) */
    {-FEN_PHI,-FEN_PHI_INV, 0.f}, { FEN_PHI,-FEN_PHI_INV, 0.f},
    { FEN_PHI, FEN_PHI_INV, 0.f}, {-FEN_PHI, FEN_PHI_INV, 0.f}
};

/* Las 12 caras pentagonales — mismos índices que rigart_geo_extra.c */
static const int FEN_DF[12][5] = {
    { 0, 8,11, 4,16}, { 2,14,13,12, 3}, { 5,17,18, 6,14},
    { 7,15,14, 6,10}, { 4,11,10, 7,15}, { 0,16,19, 7, 3},
    { 1,17, 5,11, 8}, { 2,18,17, 1,13}, { 9,10, 6,18, 2},
    { 3,12, 9, 2,19}, { 0,13, 1, 8,12}, { 9,19, 7,15,10}
};

/*
 * Genera VBO/IBO del dodecaedro escalado a radio `r`.
 * vbo: FEN_DODECA_VERTS × FEN_FPV floats
 * ibo: FEN_DODECA_IDX uint32_t
 * col_r/g/b/a: color de vértice (usado como tinte sobre el material)
 */
static void fen_gen_dodeca(float *vbo, uint32_t *ibo, float r,
                            float col_r, float col_g,
                            float col_b, float col_a)
{
    /* radio de circunscripción del dodecaedro base = sqrt(3) φ */
    float scale = r / (FEN_SQRT3 * FEN_PHI);
    uint32_t vi = 0, ii = 0;

    for (int f = 0; f < 12; f++) {
        /* Calcular normal de la cara */
        const float *va = FEN_DV[FEN_DF[f][0]];
        const float *vb = FEN_DV[FEN_DF[f][1]];
        const float *vc = FEN_DV[FEN_DF[f][2]];
        float e1[3] = {vb[0]-va[0], vb[1]-va[1], vb[2]-va[2]};
        float e2[3] = {vc[0]-va[0], vc[1]-va[1], vc[2]-va[2]};
        float n[3];
        fn_cross(e1, e2, n);
        fn_norm(n);
        /* Tangente = e1 normalizado */
        float t[3] = {e1[0], e1[1], e1[2]};
        fn_norm(t);

        uint32_t base_vi = vi;

        /* 5 vértices de la cara */
        for (int k = 0; k < 5; k++) {
            const float *p = FEN_DV[FEN_DF[f][k]];
            float *w = vbo + vi * FEN_FPV;
            /* posición (model space) */
            w[0] = p[0] * scale;
            w[1] = p[1] * scale;
            w[2] = p[2] * scale;
            /* normal */
            w[3] = n[0]; w[4] = n[1]; w[5] = n[2];
            /* tangente */
            w[6] = t[0]; w[7] = t[1]; w[8] = t[2];
            /* UV esférico */
            float pn[3] = {p[0], p[1], p[2]};
            fn_norm(pn);
            w[9]  = 0.5f + rl_atan2f(pn[2], pn[0]) / FEN_2PI;
            w[10] = 0.5f - fen_asinf(pn[1]) / FEN_PI;
            /* color */
            w[11] = col_r; w[12] = col_g;
            w[13] = col_b; w[14] = col_a;
            vi++;
        }

        /* 3 triángulos en abanico: (0,1,2), (0,2,3), (0,3,4) */
        for (int k = 1; k <= 3; k++) {
            ibo[ii++] = base_vi;
            ibo[ii++] = base_vi + (uint32_t)k;
            ibo[ii++] = base_vi + (uint32_t)k + 1u;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * §4  Espiral Fibonacci 3D — 89 puntos, ángulo áureo entre brazos
 *     Cada segmento → quad billboard (4 verts, 2 tris)
 * ═══════════════════════════════════════════════════════════════════════════ */

#define FEN_SPIRAL_PTS 89

static void fen_spiral_pt(int i, float r_max, float out[3])
{
    /* Radio crece con φ^(i/φ), altura oscila, ángulo áureo */
    float t      = (float)i / (float)(FEN_SPIRAL_PTS - 1);
    float radius = r_max * rl_powf(t, FEN_PHI_INV);
    float angle  = (float)i * FEN_GOLDEN_ANG;
    float height = r_max * 0.6f * (2.f * t - 1.f);   /* de -0.6 a +0.6 */
    out[0] = radius * rl_cosf(angle);
    out[1] = height;
    out[2] = radius * rl_sinf(angle);
}

/*
 * Genera quads orientados a cámara (billboards) para cada segmento.
 * Ancho del quad = FEN_PHI_INV2 × radio del segmento (dinámico).
 * vbo: FEN_SPIRAL_VERTS × FEN_FPV floats
 * ibo: FEN_SPIRAL_IDX uint32_t
 */
static void fen_gen_spiral(float *vbo, uint32_t *ibo, float r_max)
{
    uint32_t vi = 0, ii = 0;
    static const float cam_dir[3] = {0.f, 0.f, 1.f};  /* vista +Z */

    for (int seg = 0; seg < (int)FEN_SPIRAL_SEGS; seg++) {
        float p0[3], p1[3];
        fen_spiral_pt(seg,   r_max, p0);
        fen_spiral_pt(seg+1, r_max, p1);

        /* Dirección del segmento */
        float dir[3] = {p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2]};
        fn_norm(dir);

        /* Perpendicular en el plano de la cámara → da ancho al quad */
        float perp[3];
        fn_cross(dir, cam_dir, perp);
        if (perp[0]*perp[0]+perp[1]*perp[1]+perp[2]*perp[2] < 1e-6f) {
            perp[0] = 1.f; perp[1] = 0.f; perp[2] = 0.f;
        }
        fn_norm(perp);

        /* Radio local del segmento (más grueso en el centro) */
        float t   = (float)seg / (float)(FEN_SPIRAL_SEGS - 1);
        float w   = r_max * FEN_PHI_INV2 * 0.08f
                  * (1.f - rl_fabsf(2.f * t - 1.f) * 0.6f);

        /* Brillo φ-dorado: máximo en el centro */
        float lum = 0.4f + 0.6f * rl_expf(-rl_fabsf(2.f*t - 1.f) * FEN_PHI);

        /* Normal = apuntando hacia la cámara */
        float nrm[3] = {0.f, 0.f, 1.f};
        float tan[3] = {perp[0], perp[1], perp[2]};

        uint32_t base_vi = vi;

        /* 4 vértices del quad: p0-w, p0+w, p1+w, p1-w */
        float corners[4][3] = {
            {p0[0]-perp[0]*w, p0[1]-perp[1]*w, p0[2]-perp[2]*w},
            {p0[0]+perp[0]*w, p0[1]+perp[1]*w, p0[2]+perp[2]*w},
            {p1[0]+perp[0]*w, p1[1]+perp[1]*w, p1[2]+perp[2]*w},
            {p1[0]-perp[0]*w, p1[1]-perp[1]*w, p1[2]-perp[2]*w}
        };
        float uvs[4][2] = {{0.f,0.f},{1.f,0.f},{1.f,1.f},{0.f,1.f}};

        for (int k = 0; k < 4; k++) {
            float *w_vb = vbo + vi * FEN_FPV;
            w_vb[0] = corners[k][0];
            w_vb[1] = corners[k][1];
            w_vb[2] = corners[k][2];
            w_vb[3] = nrm[0]; w_vb[4] = nrm[1]; w_vb[5] = nrm[2];
            w_vb[6] = tan[0]; w_vb[7] = tan[1]; w_vb[8] = tan[2];
            w_vb[9]  = uvs[k][0];
            w_vb[10] = uvs[k][1];
            /* Color dorado-púrpura φ */
            w_vb[11] = 0.62f * lum;   /* R */
            w_vb[12] = 0.40f * lum;   /* G */
            w_vb[13] = 0.90f * lum;   /* B */
            w_vb[14] = 1.f;
            vi++;
        }

        /* 2 triángulos: (0,1,2), (0,2,3) */
        ibo[ii++] = base_vi;     ibo[ii++] = base_vi+1; ibo[ii++] = base_vi+2;
        ibo[ii++] = base_vi;     ibo[ii++] = base_vi+2; ibo[ii++] = base_vi+3;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * §5  Campo de partículas — 144 instancias, distribución ángulo áureo
 *     Cada partícula = octaedro pequeño (§6 lo define)
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Vértices del octaedro unitario */
static const float FEN_OCT_V[6][3] = {
    { 0.f, 1.f, 0.f},  /* top    */
    { 1.f, 0.f, 0.f},  /* +X     */
    { 0.f, 0.f, 1.f},  /* +Z     */
    {-1.f, 0.f, 0.f},  /* -X     */
    { 0.f, 0.f,-1.f},  /* -Z     */
    { 0.f,-1.f, 0.f}   /* bottom */
};

/* 8 caras del octaedro */
static const uint32_t FEN_OCT_F[8][3] = {
    {0,1,2},{0,2,3},{0,3,4},{0,4,1},
    {5,2,1},{5,3,2},{5,4,3},{5,1,4}
};

/*
 * Genera VBO/IBO con 144 octaedros distribuidos en esfera con ángulo áureo.
 * radio_inner: radio mínimo (interior del dodecaedro)
 * radio_outer: radio máximo (exterior + espacio de partículas)
 */
static void fen_gen_particles(float *vbo, uint32_t *ibo,
                               float radio_inner, float radio_outer)
{
    uint32_t vi = 0, ii = 0;

    for (uint32_t p = 0; p < FEN_PART_COUNT; p++) {
        /* Distribución Fibonacci en esfera */
        float frac  = (float)p / (float)(FEN_PART_COUNT - 1);
        float theta = fen_acosf(1.f - 2.f * frac);  /* polar [0,π] */
        float phi   = (float)p * FEN_GOLDEN_ANG;    /* azimutal */

        /* Radio: mezcla interior/exterior con variación φ */
        float r = radio_inner + (radio_outer - radio_inner)
                * rl_powf(frac, FEN_PHI_INV);

        /* Centro de esta partícula */
        float cx = r * rl_sinf(theta) * rl_cosf(phi);
        float cy = r * rl_cosf(theta);
        float cz = r * rl_sinf(theta) * rl_sinf(phi);

        /* Tamaño de la partícula — más pequeña en el exterior */
        float sz = 0.018f * (1.f - 0.5f * frac);

        /* Color: áurico en el interior, púrpura en el exterior */
        float gold  = 1.f - frac;
        float cr = 0.95f * gold + 0.28f * (1.f - gold);
        float cg = 0.78f * gold + 0.08f * (1.f - gold);
        float cb = 0.20f * gold + 0.72f * (1.f - gold);

        /* Base de índices para los 6 verts del octaedro */
        uint32_t base_vi = vi;

        /* 6 vértices del octaedro escalados y trasladados */
        for (int k = 0; k < 6; k++) {
            float *w = vbo + vi * FEN_FPV;
            w[0] = cx + FEN_OCT_V[k][0] * sz;
            w[1] = cy + FEN_OCT_V[k][1] * sz;
            w[2] = cz + FEN_OCT_V[k][2] * sz;
            /* Normal apunta hacia afuera del octaedro */
            float nrm[3] = {FEN_OCT_V[k][0], FEN_OCT_V[k][1], FEN_OCT_V[k][2]};
            fn_norm(nrm);
            w[3] = nrm[0]; w[4] = nrm[1]; w[5] = nrm[2];
            /* Tangente */
            w[6] = 1.f; w[7] = 0.f; w[8] = 0.f;
            /* UV esférico */
            w[9]  = 0.5f + rl_atan2f(nrm[2], nrm[0]) / FEN_2PI;
            w[10] = 0.5f - fen_asinf(nrm[1]) / FEN_PI;
            /* Color */
            w[11] = cr; w[12] = cg; w[13] = cb; w[14] = 1.f;
            vi++;
        }

        /* 8 caras del octaedro */
        for (int f = 0; f < 8; f++) {
            ibo[ii++] = base_vi + FEN_OCT_F[f][0];
            ibo[ii++] = base_vi + FEN_OCT_F[f][1];
            ibo[ii++] = base_vi + FEN_OCT_F[f][2];
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * §7  Estado de la escena
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    RigGPUCtx      gpu;
    RigPBRPipeline pbr;
    bool           ready;

    /* Programas (el rasterizador selecciona shader por nombre del prog) */
    uint32_t prog_crystal;   /* Swarovski Crystal — cristal dodecaedro */
    uint32_t prog_gold;      /* Pan de Oro Real   — marco dorado       */
    uint32_t prog_spiral;    /* Impulso Neural PHI — espiral φ emissive */
    uint32_t prog_particle;  /* Energia Oscura + Pan de Oro            */

    /* VAOs */
    RigVAO *vao_dodeca;      /* dodecaedro cristal exterior */
    RigVAO *vao_gold;        /* dodecaedro dorado interior  */
    RigVAO *vao_spiral;      /* espiral Fibonacci            */
    RigVAO *vao_particles;   /* campo de partículas          */

    /* Matrices fijas */
    float proj[16];
    float view[16];

    /* Camera en world space */
    float cam_pos[3];
} FenScene;

static FenScene g_fen;

/* ── Helper: crea un programa (vertex+frag placeholders soberanos) ── */
static uint32_t fen_prog(RigGPUCtx *gpu)
{
    uint32_t vs = rig_shader_compile(gpu, RIG_SHADER_VERT, "", "fen_vs");
    uint32_t fs = rig_shader_compile(gpu, RIG_SHADER_FRAG, "", "fen_fs");
    return rig_program_link(gpu, vs, fs);
}

/* ── Helper: crea VBO + IBO y devuelve un VAO ─────────────────────── */
static RigVAO* fen_make_vao(RigGPUCtx *gpu,
                              const float    *vbo_data, uint32_t n_verts,
                              const uint32_t *ibo_data, uint32_t n_idx)
{
    uint32_t vbo = rig_buffer_create(gpu, RIG_BUF_VERTEX,
                                     vbo_data,
                                     (size_t)n_verts * FEN_FPV * sizeof(float),
                                     RIG_BUF_STATIC);
    uint32_t ibo = rig_buffer_create(gpu, RIG_BUF_INDEX,
                                     ibo_data,
                                     (size_t)n_idx * sizeof(uint32_t),
                                     RIG_BUF_STATIC);
    /*
     * rig_draw_indexed lee el VBO como bloques de SR_FLOATS_PER_VERT floats:
     *   attrib 0 (pos):     offset 0,  stride FEN_FPV
     *   attrib 1 (normal):  offset 3
     *   attrib 2 (tangent): offset 6
     *   attrib 3 (uv):      offset 9
     *   attrib 4 (color):   offset 11
     * Los RigVertexAttrib se pasan solo para documentación;
     * rig_draw_indexed los usa directamente por índice fijo.
     */
    RigVertexAttrib att[5] = {
        {0, 3, 0x1406/*GL_FLOAT*/, false, FEN_FPV * 4u,  0u              },
        {1, 3, 0x1406,             false, FEN_FPV * 4u,  3u * 4u         },
        {2, 3, 0x1406,             false, FEN_FPV * 4u,  6u * 4u         },
        {3, 2, 0x1406,             false, FEN_FPV * 4u,  9u * 4u         },
        {4, 4, 0x1406,             false, FEN_FPV * 4u, 11u * 4u         }
    };
    return rig_vao_create(gpu, vbo, ibo, att, 5, n_verts, n_idx);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * fens_init — inicializa todo
 * ═══════════════════════════════════════════════════════════════════════════ */

int fens_init(void)
{
    rl_memset(&g_fen, 0, sizeof(g_fen));

    /* ── GPU + PBR ─────────────────────────────────────────────────────── */
    /* Cuarto argumento: offscreen = true (render fuera de pantalla). */
    if (rig_gpu_init(&g_fen.gpu, NULL, NULL, true) != 0)
        return -1;

    if (rig_pbr_pipeline_init(&g_fen.gpu, &g_fen.pbr, FEN_W, FEN_H) != 0)
        return -2;

    RigGPUCtx *gpu = &g_fen.gpu;

    /* ── Programas ─────────────────────────────────────────────────────── */
    g_fen.prog_crystal  = fen_prog(gpu);
    g_fen.prog_gold     = fen_prog(gpu);
    g_fen.prog_spiral   = fen_prog(gpu);
    g_fen.prog_particle = fen_prog(gpu);

    /* ── Asignar materiales PBR a cada programa ─────────────────────────
     *   rig_mat_set() popula uAlbedo, uMetallic, uRoughness, uSSS,
     *   uEmissive, uEmissionStr, uIridescence, uRimColor, uClearcoat.
     *   El rasterizador los lee en sr_shade_pbr / sr_shade_*.       */
    rig_mat_set(gpu, g_fen.prog_crystal,  "Swarovski Crystal");
    rig_mat_set(gpu, g_fen.prog_gold,     "Pan de Oro Real");
    rig_mat_set(gpu, g_fen.prog_spiral,   "Impulso Neural PHI");
    rig_mat_set(gpu, g_fen.prog_particle, "Energia Oscura");

    /* Ajuste fino: clearcoat adicional al cristal */
    rig_uniform_1f(gpu, g_fen.prog_crystal, "uClearcoat",  0.98f);
    rig_uniform_1f(gpu, g_fen.prog_crystal, "uIridescence", 0.80f);
    /* Marco dorado: brillo anisótropo máximo */
    rig_uniform_1f(gpu, g_fen.prog_gold, "uClearcoat", 0.85f);
    /* Espiral: emisión fuerte dorada-violeta */
    rig_uniform_3f(gpu, g_fen.prog_spiral, "uEmissive",
                   0.72f, 0.32f, 0.98f);
    rig_uniform_1f(gpu, g_fen.prog_spiral, "uEmissionStr", 2.5f);
    /* Partículas: mezcla emisiva */
    rig_uniform_3f(gpu, g_fen.prog_particle, "uEmissive",
                   0.95f, 0.78f, 0.25f);
    rig_uniform_1f(gpu, g_fen.prog_particle, "uEmissionStr", 1.8f);

    /* ── Cámara ─────────────────────────────────────────────────────────
     *   Colocada en +Z para ver el dodecaedro de frente, ligeramente
     *   elevada con ángulo X de 15° (mismo que la imagen)            */
    g_fen.cam_pos[0] = 0.f;
    g_fen.cam_pos[1] = 0.5f;   /* ligeramente elevada */
    g_fen.cam_pos[2] = 5.5f;

    /* Proyección: FOV 38° (un poco más cerrado que 45° para mejor proporción) */
    fm_perspective(g_fen.proj,
                   38.f * FEN_PI / 180.f,
                   (float)FEN_W / (float)FEN_H,
                   0.1f, 50.f);

    /* Vista: look-at hacia el origen */
    {
        float eye[3] = {g_fen.cam_pos[0], g_fen.cam_pos[1], g_fen.cam_pos[2]};
        float at[3]  = {0.f, 0.f, 0.f};
        float up[3]  = {0.f, 1.f, 0.f};
        /* forward */
        float fwd[3] = {at[0]-eye[0], at[1]-eye[1], at[2]-eye[2]};
        fn_norm(fwd);
        /* right */
        float rgt[3]; fn_cross(fwd, up, rgt); fn_norm(rgt);
        /* true up */
        float tup[3]; fn_cross(rgt, fwd, tup);
        /* Build view matrix (column-major) */
        float *v = g_fen.view;
        fm_id(v);
        v[0] = rgt[0]; v[4] = rgt[1];  v[8]  =  rgt[2];
        v[1] = tup[0]; v[5] = tup[1];  v[9]  =  tup[2];
        v[2] =-fwd[0]; v[6] =-fwd[1];  v[10] = -fwd[2];
        v[12]= -(rgt[0]*eye[0]+rgt[1]*eye[1]+rgt[2]*eye[2]);
        v[13]= -(tup[0]*eye[0]+tup[1]*eye[1]+tup[2]*eye[2]);
        v[14]=  (fwd[0]*eye[0]+fwd[1]*eye[1]+fwd[2]*eye[2]);
    }

    /* ── Luz principal: warm directional top-front (mismo que init) ─────
     *   Ajustamos para el look de la imagen: suave desde arriba-frente  */
    rig_uniform_3f(gpu, g_fen.prog_crystal,  "uLPos",
                   0.3f, 1.2f, 1.5f);
    rig_uniform_3f(gpu, g_fen.prog_crystal,  "uLCol",
                   1.0f, 0.95f, 0.85f);
    rig_uniform_3f(gpu, g_fen.prog_gold,     "uLPos",
                   0.3f, 1.2f, 1.5f);
    rig_uniform_3f(gpu, g_fen.prog_gold,     "uLCol",
                   1.0f, 0.90f, 0.70f);   /* más cálido en el oro */

    /* ── Geometría: dodecaedro cristal exterior ─────────────────────── */
    {
        float *vbo = (float*)rl_malloc(FEN_DODECA_VERTS * FEN_FPV * sizeof(float));
        uint32_t *ibo = (uint32_t*)rl_malloc(FEN_DODECA_IDX * sizeof(uint32_t));
        if (vbo && ibo) {
            /* radio = 1.62 ≈ φ — llena bien el encuadre portrait */
            fen_gen_dodeca(vbo, ibo, FEN_PHI,
                           0.94f, 0.95f, 0.98f, 1.f); /* blanco-azulado cristal */
            g_fen.vao_dodeca = fen_make_vao(gpu,
                                             vbo, FEN_DODECA_VERTS,
                                             ibo, FEN_DODECA_IDX);
        }
        rl_free(vbo); rl_free(ibo);
    }

    /* ── Geometría: dodecaedro dorado interior (φ⁻² = 0.382 del ext) ── */
    {
        float *vbo = (float*)rl_malloc(FEN_DODECA_VERTS * FEN_FPV * sizeof(float));
        uint32_t *ibo = (uint32_t*)rl_malloc(FEN_DODECA_IDX * sizeof(uint32_t));
        if (vbo && ibo) {
            fen_gen_dodeca(vbo, ibo, FEN_PHI * FEN_PHI_INV2,
                           1.0f, 0.84f, 0.28f, 1.f); /* tinte dorado */
            g_fen.vao_gold = fen_make_vao(gpu,
                                           vbo, FEN_DODECA_VERTS,
                                           ibo, FEN_DODECA_IDX);
        }
        rl_free(vbo); rl_free(ibo);
    }

    /* ── Geometría: espiral Fibonacci interior ───────────────────────── */
    {
        float *vbo = (float*)rl_malloc(FEN_SPIRAL_VERTS * FEN_FPV * sizeof(float));
        uint32_t *ibo = (uint32_t*)rl_malloc(FEN_SPIRAL_IDX * sizeof(uint32_t));
        if (vbo && ibo) {
            fen_gen_spiral(vbo, ibo, FEN_PHI * 0.55f);  /* cabe dentro del dodecaedro */
            g_fen.vao_spiral = fen_make_vao(gpu,
                                             vbo, FEN_SPIRAL_VERTS,
                                             ibo, FEN_SPIRAL_IDX);
        }
        rl_free(vbo); rl_free(ibo);
    }

    /* ── Geometría: campo de partículas ─────────────────────────────── */
    {
        float *vbo = (float*)rl_malloc(FEN_PART_VERTS * FEN_FPV * sizeof(float));
        uint32_t *ibo = (uint32_t*)rl_malloc(FEN_PART_IDX * sizeof(uint32_t));
        if (vbo && ibo) {
            /* partículas: entre el dodecaedro (φ) y 2.8× fuera */
            fen_gen_particles(vbo, ibo, FEN_PHI * 1.05f, FEN_PHI * 1.8f);
            g_fen.vao_particles = fen_make_vao(gpu,
                                                vbo, FEN_PART_VERTS,
                                                ibo, FEN_PART_IDX);
        }
        rl_free(vbo); rl_free(ibo);
    }

    g_fen.ready = true;
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * fens_frame — render de un frame animado
 *   time_s: tiempo acumulado en segundos
 * ═══════════════════════════════════════════════════════════════════════════ */

void fens_frame(float time_s)
{
    if (!g_fen.ready) return;
    RigGPUCtx      *gpu = &g_fen.gpu;
    RigPBRPipeline *pbr = &g_fen.pbr;

    rig_gpu_frame_begin(gpu);

    /* ── PBR: inicio de geometría → render al FBO ───────────────────── */
    rig_pbr_begin_geometry(gpu, pbr);

    /* Clear: void black (#000000) fondo cósmico */
    rig_state_clear(gpu,
                    0.000f, 0.000f, 0.008f, 1.f,  /* casi negro con toque violeta */
                    1.0f);

    /* CamPos en todos los programas */
    for (int pi = 0; pi < 4; pi++) {
        uint32_t prg = (pi==0) ? g_fen.prog_crystal
                     : (pi==1) ? g_fen.prog_gold
                     : (pi==2) ? g_fen.prog_spiral
                     :           g_fen.prog_particle;
        rig_uniform_3f(gpu, prg, "uCamPos",
                       g_fen.cam_pos[0],
                       g_fen.cam_pos[1],
                       g_fen.cam_pos[2]);
    }

    /* ── Animación: el dodecaedro gira en Y con periodo 2π/φ ────────── */
    float rot_y  =  time_s * FEN_PHI_INV * 0.3f;   /* externo: lento */
    float rot_y2 = -time_s * FEN_PHI_INV2 * 0.5f;  /* interno: contra-giro */
    float rot_x  =  FEN_PI * 0.12f;                 /* inclinación 22°  */

    /* Pulso de escala: s = 1 + 0.025 × sin(t × φ²) */
    float pulse = 1.f + 0.025f * rl_sinf(time_s * SR_PHI2);

    /* ── Render: dodecaedro cristal exterior ───────────────────────── */
    {
        float mR[16], mRx[16], mS[16], mT[16], model[16], tmp[16];
        fm_rotate_y(mR,  rot_y);
        fm_rotate_x(mRx, rot_x);
        fm_scale(mS, pulse);
        fm_translate(mT, 0.f, 0.1f, 0.f);   /* ligeramente arriba del centro */

        fm_mul(tmp,   mRx, mR);   /* combina rotaciones */
        fm_mul(model, mS, tmp);
        fm_mul(tmp, mT, model);
        rl_memcpy(model, tmp, 64);

        float mvp[16];
        fm_mvp(mvp, g_fen.proj, g_fen.view, model);

        rig_program_bind(gpu, g_fen.prog_crystal);
        rig_uniform_mat4(gpu, g_fen.prog_crystal, "u_mvp",   mvp);
        rig_uniform_mat4(gpu, g_fen.prog_crystal, "u_model", model);

        /* Backface culling OFF para cristal (queremos ver las caras interiores) */
        rig_state_cull(gpu, false, 0);
        rig_vao_bind(gpu, g_fen.vao_dodeca);
        rig_draw_indexed(gpu, g_fen.vao_dodeca, 0, FEN_DODECA_IDX);
        rig_state_cull(gpu, true, 0);
    }

    /* ── Render: dodecaedro dorado interior ────────────────────────── */
    {
        float mR[16], mRx[16], mS[16], mT[16], model[16], tmp[16];
        fm_rotate_y(mR,  rot_y2);            /* contra-gira */
        fm_rotate_x(mRx, rot_x);
        fm_scale(mS, pulse);
        fm_translate(mT, 0.f, 0.1f, 0.f);

        fm_mul(tmp,   mRx, mR);
        fm_mul(model, mS, tmp);
        fm_mul(tmp, mT, model);
        rl_memcpy(model, tmp, 64);

        float mvp[16];
        fm_mvp(mvp, g_fen.proj, g_fen.view, model);

        rig_program_bind(gpu, g_fen.prog_gold);
        rig_uniform_mat4(gpu, g_fen.prog_gold, "u_mvp",   mvp);
        rig_uniform_mat4(gpu, g_fen.prog_gold, "u_model", model);

        rig_vao_bind(gpu, g_fen.vao_gold);
        rig_draw_indexed(gpu, g_fen.vao_gold, 0, FEN_DODECA_IDX);
    }

    /* ── Render: espiral Fibonacci (emissive, no necesita luces) ─────── */
    {
        /* La espiral rota con φ³ más rápido que el dodecaedro */
        float rot_sp = time_s * SR_PHI3 * 0.15f;
        float mR[16], mRz[16], model[16], tmp[16];
        fm_rotate_y(mR,  rot_sp);
        fm_rotate_z(mRz, rot_sp * FEN_PHI_INV);
        fm_mul(model, mRz, mR);
        /* Desplaza la espiral hacia el centro visual */
        float mT[16]; fm_translate(mT, 0.f, 0.08f, 0.f);
        fm_mul(tmp, mT, model);
        rl_memcpy(model, tmp, 64);

        float mvp[16];
        fm_mvp(mvp, g_fen.proj, g_fen.view, model);

        /* Modulación de emisión con el tiempo → brillo pulsante */
        float emit_pulse = 2.2f + 0.8f * rl_sinf(time_s * FEN_PHI * 1.5f);
        rig_uniform_1f(gpu, g_fen.prog_spiral, "uEmissionStr", emit_pulse);

        rig_program_bind(gpu, g_fen.prog_spiral);
        rig_uniform_mat4(gpu, g_fen.prog_spiral, "u_mvp",   mvp);
        rig_uniform_mat4(gpu, g_fen.prog_spiral, "u_model", model);

        rig_state_blend(gpu, true, 0x0302/*SRC_ALPHA*/, 0x0303/*ONE_MINUS_SRC_ALPHA*/);
        rig_vao_bind(gpu, g_fen.vao_spiral);
        rig_draw_indexed(gpu, g_fen.vao_spiral, 0, FEN_SPIRAL_IDX);
        rig_state_blend(gpu, false, 0, 0);
    }

    /* ── Render: campo de partículas ────────────────────────────────── */
    {
        /* Las partículas orbitan con ángulo áureo por frame */
        float rot_part = time_s * FEN_GOLDEN_ANG * 0.08f;
        float mR[16], mRx[16], model[16];
        fm_rotate_y(mR,  rot_part);
        fm_rotate_x(mRx, rot_part * FEN_PHI_INV);
        fm_mul(model, mRx, mR);

        float mvp[16];
        fm_mvp(mvp, g_fen.proj, g_fen.view, model);

        /* Partículas áureas pulsan en contrafase */
        float part_emit = 1.6f + 0.6f * rl_sinf(time_s * SR_PHI2 + FEN_PI);
        rig_uniform_1f(gpu, g_fen.prog_particle, "uEmissionStr", part_emit);

        rig_program_bind(gpu, g_fen.prog_particle);
        rig_uniform_mat4(gpu, g_fen.prog_particle, "u_mvp",   mvp);
        rig_uniform_mat4(gpu, g_fen.prog_particle, "u_model", model);

        rig_state_blend(gpu, true, 0x0302, 0x0303);
        rig_vao_bind(gpu, g_fen.vao_particles);
        rig_draw_indexed(gpu, g_fen.vao_particles, 0, FEN_PART_IDX);
        rig_state_blend(gpu, false, 0, 0);
    }

    /* ── PBR: fin de geometría → post-processing ────────────────────── */
    rig_pbr_end_geometry(gpu, pbr);

    /* SSAO: oclusión ambiental — da profundidad al cristal */
    rig_pbr_ssao_pass(gpu, pbr);

    /* BLOOM: 6 pasadas φ-escaladas — efecto corona del cristal
     *   threshold 0.72 → solo los highlights brillan
     *   strength  2.2  → bloom dramático para el glow áureo */
    rig_pbr_bloom_pass(gpu, pbr, 0.72f, 2.2f);

    /* TONEMAP ACES + sRGB: exposure 1.25 para brillos dramáticos */
    rig_pbr_tonemap_pass(gpu, pbr, 1.25f);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * fens_destroy
 * ═══════════════════════════════════════════════════════════════════════════ */

void fens_destroy(void)
{
    if (!g_fen.ready) return;
    RigGPUCtx *gpu = &g_fen.gpu;

    rig_vao_free(gpu, g_fen.vao_dodeca);
    rig_vao_free(gpu, g_fen.vao_gold);
    rig_vao_free(gpu, g_fen.vao_spiral);
    rig_vao_free(gpu, g_fen.vao_particles);

    rig_pbr_pipeline_destroy(gpu, &g_fen.pbr);
    rig_gpu_destroy(gpu);
    rl_memset(&g_fen, 0, sizeof(g_fen));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Acceso al framebuffer resultante (ARGB32 / RGBA8 packed)
 * ═══════════════════════════════════════════════════════════════════════════ */

const uint32_t* fens_framebuffer(void)
{
    /* El tonemap ya copió el resultado en primary_fb.
     * Como primary_fb es privado del SRBackend, lo obtenemos
     * accediendo al campo opaco egl_display — misma técnica
     * que sr_get() dentro del propio módulo.                */
    if (!g_fen.ready || !g_fen.gpu.initialized) return NULL;
    /* Casting documentado: egl_display es SRBackend* en el SR */
    typedef struct { uint32_t *primary_fb; } SRBackendOpaque;
    SRBackendOpaque *sr = (SRBackendOpaque*)g_fen.gpu.egl_display;
    return sr ? sr->primary_fb : NULL;
}

uint32_t fens_fb_width(void)  { return FEN_W; }
uint32_t fens_fb_height(void) { return FEN_H; }

/* Exportaciones trazables para toda función interna de esta unidad. */

