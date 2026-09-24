/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#define _POSIX_C_SOURCE 200809L

#include "../include/rigart_v3.h"
#include "../include/wsserver.h"

#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_math.h"
#include "../include/riglib_math.h"
#include "rig_noext_types.h"
#include "rig_noext_str.h"

static float j3c_float__rig_variant_761226ce(const char *j, const char *k, float def)
{
    if (!j || !k) return def;
    const char *p = strstr(j, k);
    if (!p) return def;
    p = strchr(p, ':'); if (!p) return def;
    return (float)atof(p+1);
}
static bool j3c_bool__rig_variant_cfd2d085(const char *j, const char *k, bool def)
{
    if (!j || !k) return def;
    const char *p = strstr(j, k);
    if (!p) return def;
    p = strchr(p, ':'); if (!p) return def;
    while (*p == ':' || *p == ' ') p++;
    if (*p == 't') return true;
    if (*p == 'f') return false;
    return def;
}
static void j3c_str__rig_variant_9352dca6(const char *j, const char *k, char *out, size_t n)
{
    out[0] = '\0';
    if (!j || !k) return 0;
    const char *p = rl_strstr(j, k); if (!p) return 0;
    p = rl_strchr(p, ':'); if (!p) return 0;
    p = rl_strchr(p, '"'); if (!p) return; p++;
    size_t i = 0;
    while (*p && *p != '"' && i < n-1) out[i++] = *p++;
    out[i] = '\0';
    return 0;
}
#define j3_float  j3c_float
#define j3_bool   j3c_bool
#define j3_str    j3c_str

#ifndef PHI3
#  define PHI3      1.6180339887498948482
#  define PHI3_INV  0.6180339887498948482
#  define PHI3_2    2.6180339887498948482
#  define SCHUMANN3 7.83
#  define TAU3      6.28318530717958647692
#  define PI3       3.14159265358979323846
#  define SQRT2_3   1.41421356237309504880
#  define LN_PHI3   0.48121182505960344749
#endif

typedef struct { char *buf; size_t pos; size_t cap; } Buf3c;

static Buf3c bc_new__rig_variant_3681e398(size_t cap)
{
    Buf3c b; b.buf = calloc(1, cap); b.pos = 0; b.cap = cap; return b;
}
static void bc_cat__rig_variant_99ba61aa(Buf3c *b, const char *s)
{
    if (!b->buf || !s) return 0;
    size_t n = rl_strlen(s);
    if (b->pos + n + 1 >= b->cap) return 0;
    rl_memcpy(b->buf + b->pos, s, n);
    b->pos += n;
    return 0;
}
static void bc_printf__rig_variant_f2482fac(Buf3c *b, const char *fmt, ...)
{
    if (!b->buf) return 0;
    char tmp[8192];
    va_list ap; va_start(ap, fmt);
    vrl_snprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    bc_cat(b, tmp);
    return 0;
}
static char *bc_done__rig_dup_51723c4f(Buf3c *b)
{
    if (!b->buf) return NULL;
    b->buf[b->pos] = '\0';
    return b->buf;
}
static void rset_css__rig_variant_2e374284(RIgArtResultV3 *r, Buf3c *b)
{
    r->css      = bc_done(b);
    r->css_size = b->pos;
    r->ok       = (b->buf && b->pos > 0);
    r->certeza  = PHI3_INV;
    return 0;
}
static void rset_js__rig_variant_54929c2f(RIgArtResultV3 *r, Buf3c *b)
{
    r->js      = bc_done(b);
    r->js_size = b->pos;
    r->ok      = (b->buf && b->pos > 0);
    r->certeza = PHI3_INV;
    return 0;
}
static void rset_html__rig_variant_ee82cccd(RIgArtResultV3 *r, Buf3c *b)
{
    r->html      = bc_done(b);
    r->html_size = b->pos;
    r->ok        = (b->buf && b->pos > 0);
    r->certeza   = PHI3_INV;
    return 0;
}
static void rset_glsl_frag__rig_variant_dee9c0db(RIgArtResultV3 *r, Buf3c *b)
{
    r->glsl_frag = bc_done(b);
    r->ok        = (b->buf && b->pos > 0);
    r->certeza   = PHI3_INV;
    return 0;
}
static void typo_dissolve_css__rig_variant_9e44db93(Buf3c *b, float dur)
{
    bc_printf(b,
        "/* §25 TEXTFX DISSOLVE — Disolución estocástica */\n"
        ".rg-dissolve {\n"
        "  position: relative;\n"
        "  display: inline-block;\n"
        "  animation: rg-dissolve-out %.2fs ease-in forwards;\n"
        "}\n"
        "@keyframes rg-dissolve-out {\n"
        "  0%%   { opacity: 1; filter: blur(0px); transform: scale(1); }\n"
        "  60%%  { opacity: 0.4; filter: blur(2px); transform: scale(1.02); }\n"
        "  100%% { opacity: 0; filter: blur(8px); transform: scale(1.05); }\n"
        "}\n"
        ".rg-dissolve-in {\n"
        "  animation: rg-dissolve-in-anim %.2fs %.3fs ease-out both;\n"
        "}\n"
        "@keyframes rg-dissolve-in-anim {\n"
        "  0%%   { opacity: 0; filter: blur(8px); transform: scale(0.96); }\n"
        "  50%%  { opacity: 0.6; filter: blur(2px); }\n"
        "  100%% { opacity: 1; filter: blur(0); transform: scale(1); }\n"
        "}\n",
        dur > 0.0f ? dur : 0.8f,
        dur > 0.0f ? dur : 0.8f,
        dur > 0.0f ? dur * (float)PHI3_INV : 0.5f);
    return 0;
}
static void typo_scanline_css__rig_variant_959637d3(Buf3c *b, float dur)
{
    bc_printf(b,
        "/* §25 TEXTFX SCANLINE — Líneas CRT luxury */\n"
        ".rg-scanline {\n"
        "  position: relative;\n"
        "  display: inline-block;\n"
        "  color: #%06X;\n"
        "}\n"
        ".rg-scanline::after {\n"
        "  content: '';\n"
        "  position: absolute; inset: 0;\n"
        "  background: repeating-linear-gradient(\n"
        "    0deg,\n"
        "    transparent 0px,\n"
        "    transparent 2px,\n"
        "    rgba(0,0,0,0.18) 2px,\n"
        "    rgba(0,0,0,0.18) 4px\n"
        "  );\n"
        "  pointer-events: none;\n"
        "  animation: rg-scan-roll %.2fs linear infinite;\n"
        "  border-radius: inherit;\n"
        "}\n"
        "@keyframes rg-scan-roll {\n"
        "  0%%   { background-position: 0 0; }\n"
        "  100%% { background-position: 0 4px; }\n"
        "}\n",
        0x00E5FF,
        dur > 0.0f ? dur : (float)(1.0 / SCHUMANN3));
    return 0;
}
int rigart_typo_text_fx__rig_variant_41973c59(RIgArtTextFx fx, uint32_t primary,
                         uint32_t secondary, float dur_s,
                         RIgArtResultV3 *out)
{
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);

    bc_cat__rig_variant_99ba61aa(&css, "/* RIgArt v3.0 — Text FX standalone */\n@layer rg-textfx {\n");

    switch (fx) {
        case RIGART_TEXTFX_DISSOLVE:
            typo_dissolve_css__rig_variant_9e44db93(&css, dur_s);
            break;
        case RIGART_TEXTFX_SCANLINE:
            typo_scanline_css__rig_variant_959637d3(&css, dur_s);
            break;
        case RIGART_TEXTFX_SHIMMER: {

            float d = dur_s > 0.0f ? dur_s : 3.0f;
            bc_printf__rig_variant_f2482fac(&css,
                "@keyframes rg-shimmer-fx {\n"
                "  0%%   { background-position: -200%% center; }\n"
                "  100%% { background-position: 200%% center; }\n"
                "}\n"
                ".rg-textfx-shimmer {\n"
                "  background: linear-gradient(110deg,\n"
                "    #%06X 0%%, #%06X 40%%, #fff8c0 50%%,\n"
                "    #%06X 60%%, #%06X 100%%);\n"
                "  background-size: 200%% auto;\n"
                "  -webkit-background-clip: text;\n"
                "  background-clip: text;\n"
                "  -webkit-text-fill-color: transparent;\n"
                "  animation: rg-shimmer-fx %.2fs linear infinite;\n"
                "}\n",
                primary & 0xFFFFFF, secondary & 0xFFFFFF,
                secondary & 0xFFFFFF, primary & 0xFFFFFF, d);
            break;
        }
        case RIGART_TEXTFX_EMBOSS: {
            float r = ((primary>>16)&0xFF)/255.0f;
            float g = ((primary>>8)&0xFF)/255.0f;
            float bv= (primary&0xFF)/255.0f;
            bc_printf__rig_variant_f2482fac(&css,
                ".rg-textfx-emboss {\n"
                "  color: rgba(%.3f,%.3f,%.3f,0.9);\n"
                "  text-shadow:\n"
                "    0px 1px 0px rgba(255,255,255,0.15),\n"
                "    0px -1px 0px rgba(0,0,0,0.7),\n"
                "    0px 2px 6px rgba(0,0,0,0.45);\n"
                "  filter: drop-shadow(0 0 1px rgba(0,0,0,0.5));\n"
                "}\n", r, g, bv);
            break;
        }
        case RIGART_TEXTFX_NEON_GLOW: {
            float d = dur_s > 0.0f ? dur_s : (float)(1.0/SCHUMANN3);
            bc_printf__rig_variant_f2482fac(&css,
                ".rg-textfx-neon {\n"
                "  color: #%06X;\n"
                "  text-shadow:\n"
                "    0 0 6px #%06X, 0 0 20px #%06X,\n"
                "    0 0 40px rgba(%d,%d,%d,0.4);\n"
                "  animation: rg-neon-fx %.3fs ease-in-out infinite;\n"
                "}\n"
                "@keyframes rg-neon-fx {\n"
                "  0%%,100%% { text-shadow: 0 0 6px #%06X, 0 0 20px #%06X; }\n"
                "  50%%      { text-shadow: 0 0 10px #%06X, 0 0 35px #%06X; }\n"
                "}\n",
                primary & 0xFFFFFF,
                primary & 0xFFFFFF, primary & 0xFFFFFF,
                (primary>>16)&0xFF, (primary>>8)&0xFF, primary&0xFF, d,
                primary & 0xFFFFFF, primary & 0xFFFFFF,
                primary & 0xFFFFFF, primary & 0xFFFFFF);
            break;
        }
        case RIGART_TEXTFX_HOLOGRAM: {
            float d = dur_s > 0.0f ? dur_s : 4.0f * (float)PHI3_INV;
            bc_printf__rig_variant_f2482fac(&css,
                "@keyframes rg-holo-fx { 0%% { background-position:0%% center; } 100%% { background-position:400%% center; } }\n"
                ".rg-textfx-hologram {\n"
                "  background: linear-gradient(135deg,\n"
                "    oklch(0.85 0.2 30deg), oklch(0.9 0.25 120deg),\n"
                "    oklch(0.85 0.22 240deg), oklch(0.88 0.2 300deg),\n"
                "    oklch(0.85 0.2 30deg));\n"
                "  background-size: 400%% auto;\n"
                "  -webkit-background-clip: text;\n"
                "  background-clip: text;\n"
                "  -webkit-text-fill-color: transparent;\n"
                "  animation: rg-holo-fx %.2fs linear infinite;\n"
                "}\n", d);
            break;
        }
        case RIGART_TEXTFX_MIRROR: {
            bc_cat__rig_variant_99ba61aa(&css,
                ".rg-textfx-mirror { position:relative; display:inline-block; }\n"
                ".rg-textfx-mirror::after {\n"
                "  content: attr(data-text);\n"
                "  display: block;\n"
                "  transform: scaleY(-1) translateY(0.05em);\n"
                "  background: linear-gradient(180deg,rgba(255,215,0,0.35),transparent 60%);\n"
                "  -webkit-background-clip: text;\n"
                "  background-clip: text;\n"
                "  -webkit-text-fill-color: transparent;\n"
                "  pointer-events: none;\n"
                "  mask-image: linear-gradient(180deg,rgba(0,0,0,0.4),transparent);\n"
                "  -webkit-mask-image: linear-gradient(180deg,rgba(0,0,0,0.4),transparent);\n"
                "}\n");
            break;
        }
        default: break;
    }

    bc_cat__rig_variant_99ba61aa(&css, "}\n");
    rset_css__rig_variant_2e374284(out, &css);
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

void rigart_typo_lemniscate_svg__rig_variant_735d0850(float W, float H, float a,
                                  const char *text, float font_size,
                                  char *out_svg, size_t out_n)
{

    Buf3c b = bc_new(RIGART_MAX_SVG_V3);
    float cx = W * 0.5f, cy = H * 0.5f;
    float scale = a > 0 ? a : (W < H ? W : H) * 0.36f;

    bc_printf(&b,
        "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        "width=\"%.0f\" height=\"%.0f\" viewBox=\"0 0 %.0f %.0f\">\n"
        "<defs>\n"
        "  <linearGradient id=\"rg-lem-grad\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"0\">\n"
        "    <stop offset=\"0%%\" stop-color=\"#7b5c00\"/>\n"
        "    <stop offset=\"50%%\" stop-color=\"#fff8c0\"/>\n"
        "    <stop offset=\"100%%\" stop-color=\"#7b5c00\"/>\n"
        "  </linearGradient>\n"
        "  <path id=\"rg-lem-path\" d=\"M",
        W, H, W, H);

    int steps = 360;
    bool first = true;
    for (int i = 0; i <= steps; i++) {
        double t  = TAU3 * i / steps;
        double s2 = sin(t) * sin(t);
        double denom = 1.0 + s2;
        float  x  = cx + (float)(scale * cos(t) / denom);
        float  y  = cy + (float)(scale * cos(t) * sin(t) / denom);
        if (first) { bc_printf(&b, "%.2f,%.2f ", x, y); first = false; }
        else        bc_printf(&b, "L%.2f,%.2f ", x, y);
    }

    bc_printf(&b,
        "Z\"/>\n</defs>\n"
        "<text font-family=\"'Cormorant Garamond',Georgia,serif\" "
        "font-size=\"%.1f\" fill=\"url(#rg-lem-grad)\">\n"
        "  <textPath href=\"#rg-lem-path\" startOffset=\"0%%\">%s</textPath>\n"
        "</text>\n</svg>\n",
        font_size > 0 ? font_size : 14.0f,
        text ? text : "");

    char *done = bc_done(&b);
    if (done && out_svg && out_n > 0)
        rl_snprintf(out_svg, out_n, "%s", done);
    rl_free(done);
    return 0;
}

int rigart_phi_grid_css_v3__rig_variant_95b2e362(int cols, int rows, float gap_px,
                             bool container_queries, RIgArtResultV3 *out)
{
    if (!out) return -1;
    memset(out, 0, sizeof(*out));

    static const int fib[] = {1,1,2,3,5,8,13,21,34};
    if (cols < 1 || cols > 8) cols = 3;
    if (rows < 1 || rows > 8) rows = 2;

    int cs = 0, rs = 0;
    for (int i = 0; i < cols; i++) cs += fib[i];
    for (int i = 0; i < rows; i++) rs += fib[i];

    float gap = gap_px > 0 ? gap_px : 8.0f * (float)PHI3_INV;

    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);

    bc_printf__rig_variant_f2482fac(&css,
        "/* RIgArt v3.0 — φ-Grid CSS v3 · %d×%d proporcional Fibonacci */\n"
        "@layer rg-phi-grid-v3 {\n"
        ".rg-phi-grid-v3 {\n"
        "  container-type: inline-size;\n"
        "  display: grid;\n"
        "  grid-template-columns:",
        cols, rows);

    for (int i = 0; i < cols; i++)
        bc_printf__rig_variant_f2482fac(&css, " %.6ffr", (double)fib[i] / (double)cs);

    bc_cat__rig_variant_99ba61aa(&css, ";\n  grid-template-rows:");
    for (int i = 0; i < rows; i++)
        bc_printf__rig_variant_f2482fac(&css, " %.6ffr", (double)fib[i] / (double)rs);

    bc_printf__rig_variant_f2482fac(&css,
        ";\n"
        "  gap: clamp(%.1fpx, %.4fvw, %.1fpx);\n"
        "  width: 100%%; min-height: 100%%;\n"
        "  align-items: start;\n"
        "}\n",
        gap * 0.5f,
        gap * 100.0f / 1440.0f,
        gap * (float)PHI3);

    bc_cat__rig_variant_99ba61aa(&css,
        ".rg-phi-grid-v3 > .rg-phi-span-major {\n"
        "  grid-column: 1 / span 1;\n"
        "}\n"
        ".rg-phi-grid-v3 > .rg-phi-span-minor {\n"
        "  grid-column: 2 / -1;\n"
        "}\n");

    if (container_queries) {

        float bp1 = 320.0f * (float)PHI3;
        float bp2 = 320.0f * (float)(PHI3 * PHI3);
        float bp3 = 320.0f * (float)(PHI3 * PHI3 * PHI3);

        bc_printf__rig_variant_f2482fac(&css,
            "\n/* Container queries φ — columnas adaptativas */\n"
            "@container (min-width: %.0fpx) {\n"
            "  .rg-phi-grid-v3 { column-gap: clamp(%.1fpx, 2vw, %.1fpx); }\n"
            "}\n"
            "@container (min-width: %.0fpx) {\n"
            "  .rg-phi-grid-v3 {\n"
            "    grid-template-columns:",
            bp1, gap, gap * (float)PHI3,
            bp2);

        int cols2 = cols + 1 < 8 ? cols + 1 : 8;
        int cs2 = 0;
        for (int i = 0; i < cols2; i++) cs2 += fib[i];
        for (int i = 0; i < cols2; i++)
            bc_printf__rig_variant_f2482fac(&css, " %.6ffr", (double)fib[i] / (double)cs2);
        bc_cat__rig_variant_99ba61aa(&css, ";\n  }\n}\n");

        bc_printf__rig_variant_f2482fac(&css,
            "@container (min-width: %.0fpx) {\n"
            "  .rg-phi-grid-v3 { gap: clamp(%.1fpx, 3vw, %.1fpx); }\n"
            "}\n",
            bp3, gap * (float)PHI3_INV, gap * (float)PHI3_2);
    }

    bc_cat__rig_variant_99ba61aa(&css, "}\n");
    rset_css__rig_variant_2e374284(out, &css);
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

static const char EFF_GLITCH_FRAG[] =
"#version 300 es\n"
"precision highp float;\n"
"/* Glitch Art — Digital corruption · RIgArt v3.0 · Adreno 720 */\n"
"uniform sampler2D u_tex;\n"
"uniform vec2  u_resolution;\n"
"uniform float u_time;\n"
"uniform float u_strength; /* [0,1] */\n"
"out vec4 fragColor;\n"
"\n"
"float hash(float p){return fract(sin(p*127.1)*43758.5);}\n"
"float hash2(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5);}\n"
"\n"
"void main(){\n"
"    vec2 uv=gl_FragCoord.xy/u_resolution;\n"
"    float gt=floor(u_time*8.0);\n"
"    float glitch_line=hash(gt)*0.85+0.05;\n"
"    float glitch_h=hash(gt+1.0)*0.15+0.02;\n"
"\n"
"    vec2 uv2=uv;\n"
"    if(abs(uv.y-glitch_line)<glitch_h && hash(gt+2.0)<u_strength){\n"
"        float shift=(hash(gt+3.0)-0.5)*0.06*u_strength;\n"
"        uv2.x+=shift;\n"
"    }\n"
"\n"
"    /* Aberración cromática durante glitch */\n"
"    float gc=(hash(gt+5.0)<u_strength*0.4)?1.0:0.0;\n"
"    float r=texture(u_tex,uv2+vec2(0.006*gc,0.0)).r;\n"
"    float g=texture(u_tex,uv2).g;\n"
"    float b=texture(u_tex,uv2-vec2(0.006*gc,0.0)).b;\n"
"\n"
"    /* Bloque de corrupción digital — teletexto */\n"
"    float noise=hash2(vec2(floor(uv2.y*80.0+gt)*13.7, uv2.x*1000.0));\n"
"    vec3 c=vec3(r,g,b);\n"
"    if(hash(gt+7.0)<u_strength*0.15 && abs(uv.y-glitch_line)<glitch_h*0.3)\n"
"        c=vec3(noise, noise*0.2, noise*0.4);\n"
"\n"
"    /* Scanline parasitaria */\n"
"    float scan=0.5+0.5*cos(gl_FragCoord.y*3.14159*2.0);\n"
"    c*=mix(1.0, scan*0.85+0.15, u_strength*0.2);\n"
"\n"
"    fragColor=vec4(clamp(c,0.0,1.0), texture(u_tex,uv).a);\n"
"}\n";

static const char EFF_DOF_BOKEH_FRAG[] =
"#version 300 es\n"
"precision highp float;\n"
"/* Depth of Field — Bokeh hexagonal · RIgArt v3.0 · Adreno 720 */\n"
"uniform sampler2D u_tex;\n"
"uniform sampler2D u_depth;  /* mapa de profundidad [0,1] */\n"
"uniform vec2  u_resolution;\n"
"uniform float u_focal_dist; /* distancia focal [0,1] */\n"
"uniform float u_strength;   /* intensidad max blur [0,1] */\n"
"uniform float u_time;\n"
"out vec4 fragColor;\n"
"\n"
"/* Forma de apertura hexagonal */\n"
"float hexagon(vec2 p, float sz){\n"
"    p=abs(p);\n"
"    return max(dot(p,normalize(vec2(1.0,1.732))), p.x) < sz ? 1.0 : 0.0;\n"
"}\n"
"\n"
"void main(){\n"
"    vec2 uv=gl_FragCoord.xy/u_resolution;\n"
"    float depth=texture(u_depth,uv).r;\n"
"    float coc=abs(depth-u_focal_dist)*u_strength*24.0;\n"
"    coc=clamp(coc, 0.0, 12.0);\n"
"\n"
"    vec4 col=vec4(0.0);\n"
"    float total=0.0;\n"
"\n"
"    /* Kernel hexagonal — 37 samples optimizado Adreno */\n"
"    for(int i=-3;i<=3;i++){\n"
"        for(int j=-3;j<=3;j++){\n"
"            vec2 offset=vec2(float(i),float(j))*coc/u_resolution;\n"
"            float w=hexagon(vec2(float(i),float(j)), 3.2);\n"
"            if(w<0.5) continue;\n"
"            float d=texture(u_depth, uv+offset).r;\n"
"            /* Solo muestras dentro del CoC */\n"
"            float dw=(abs(d-u_focal_dist)*u_strength < 0.1)?1.0:0.5;\n"
"            col+=texture(u_tex, uv+offset)*w*dw;\n"
"            total+=w*dw;\n"
"        }\n"
"    }\n"
"    col/=total;\n"
"\n"
"    /* Bokeh highlight — nits boost en zonas brillantes */\n"
"    float lum=dot(col.rgb, vec3(0.299,0.587,0.114));\n"
"    col.rgb+=max(lum-0.85,0.0)*vec3(1.0,0.95,0.85)*coc*0.08;\n"
"    fragColor=col;\n"
"}\n";

static const char EFF_GOD_RAYS_FRAG[] =
"#version 300 es\n"
"precision highp float;\n"
"/* God Rays — Volumetric light · 64 samples · RIgArt v3.0 */\n"
"uniform sampler2D u_tex;\n"
"uniform vec2  u_resolution;\n"
"uniform float u_time;\n"
"uniform vec2  u_light_pos;  /* fuente de luz [0,1] */\n"
"uniform float u_decay;      /* decaimiento [0.9,0.999] */\n"
"uniform float u_exposure;   /* exposición [0.1,1.0] */\n"
"uniform float u_strength;\n"
"out vec4 fragColor;\n"
"\n"
"#define SAMPLES 64\n"
"\n"
"void main(){\n"
"    vec2 uv=gl_FragCoord.xy/u_resolution;\n"
"    vec2 delta=(uv-u_light_pos)/float(SAMPLES);\n"
"    vec2 tc=uv;\n"
"    float decay=1.0;\n"
"    vec3 scatter=vec3(0.0);\n"
"\n"
"    for(int i=0;i<SAMPLES;i++){\n"
"        tc-=delta;\n"
"        vec3 s=texture(u_tex,tc).rgb;\n"
"        /* Occlusion — solo muestras brillantes contribuyen */\n"
"        float lum=dot(s,vec3(0.299,0.587,0.114));\n"
"        s*=step(0.3,lum);\n"
"        scatter+=s*decay;\n"
"        decay*=u_decay;\n"
"    }\n"
"    scatter*=u_exposure*u_strength/float(SAMPLES);\n"
"\n"
"    /* Modulación Schumann — pulso sutil 7.83 Hz */\n"
"    float pulse=1.0+0.05*sin(u_time*6.28318*7.83);\n"
"    scatter*=pulse;\n"
"\n"
"    /* Tinte cálido dorado en rayos */\n"
"    scatter*=vec3(1.0, 0.92, 0.75);\n"
"\n"
"    vec4 scene=texture(u_tex,uv);\n"
"    fragColor=vec4(scene.rgb+scatter, scene.a);\n"
"}\n";

static const char EFF_CAUSTICS_FRAG[] =
"#version 300 es\n"
"precision highp float;\n"
"/* Caustics — Patrones φ-ondas · RIgArt v3.0 · Adreno 720 */\n"
"uniform sampler2D u_tex;\n"
"uniform vec2  u_resolution;\n"
"uniform float u_time;\n"
"uniform float u_strength;\n"
"out vec4 fragColor;\n"
"\n"
"#define PHI 1.6180339887\n"
"\n"
"float caustic(vec2 p, float t){\n"
"    /* Superposición de 5 ondas φ-rotadas */\n"
"    float c=0.0;\n"
"    for(int i=0;i<5;i++){\n"
"        float angle=float(i)*3.14159*2.0/PHI;\n"
"        vec2 d=vec2(cos(angle),sin(angle));\n"
"        float freq=1.0+float(i)*0.618;\n"
"        c+=sin(dot(p*freq,d)+t*(0.5+float(i)*0.382));\n"
"    }\n"
"    return c/5.0;\n"
"}\n"
"\n"
"void main(){\n"
"    vec2 uv=gl_FragCoord.xy/u_resolution;\n"
"    vec2 p=uv*6.0;\n"
"\n"
"    float c1=caustic(p, u_time*1.0);\n"
"    float c2=caustic(p*PHI, u_time*PHI_INV);\n"
"    /* Interferencia destructiva-constructiva */\n"
"    float pattern=pow(max(c1+c2+1.0, 0.0)*0.5, 2.0);\n"
"\n"
"    vec4 scene=texture(u_tex,uv);\n"
"    /* Tinte agua profunda */\n"
"    vec3 tint=mix(\n"
"        vec3(0.0,0.15,0.35),\n"
"        vec3(0.6,0.9,1.0),\n"
"        pattern);\n"
"    fragColor=vec4(scene.rgb+tint*pattern*u_strength*0.4, scene.a);\n"
"}\n";

static const char EFF_HOLOGRAM_SCAN_FRAG[] =
"#version 300 es\n"
"precision highp float;\n"
"/* Hologram Scan — Escaneo holográfico · RIgArt v3.0 */\n"
"uniform sampler2D u_tex;\n"
"uniform vec2  u_resolution;\n"
"uniform float u_time;\n"
"uniform float u_strength;\n"
"out vec4 fragColor;\n"
"\n"
"void main(){\n"
"    vec2 uv=gl_FragCoord.xy/u_resolution;\n"
"    vec4 scene=texture(u_tex,uv);\n"
"\n"
"    /* Scanline de escaneo — barre de arriba a abajo */\n"
"    float scan_y=mod(u_time*0.4, 1.2)-0.1;\n"
"    float beam=exp(-pow((uv.y-scan_y)*18.0, 2.0));\n"
"\n"
"    /* Líneas horizontales holográficas */\n"
"    float lines=0.5+0.5*cos(uv.y*u_resolution.y*0.5*3.14159);\n"
"    lines=pow(lines, 3.0);\n"
"\n"
"    /* Aberración de borde — iridiscencia */\n"
"    float edge=smoothstep(0.0,0.08,uv.x)*smoothstep(1.0,0.92,uv.x)\n"
"              *smoothstep(0.0,0.06,uv.y)*smoothstep(1.0,0.94,uv.y);\n"
"    float chromatic=(1.0-edge)*0.015*u_strength;\n"
"    float r=texture(u_tex,uv+vec2(chromatic,0.0)).r;\n"
"    float b=texture(u_tex,uv-vec2(chromatic,0.0)).b;\n"
"    vec3 c=vec3(r,scene.g,b);\n"
"\n"
"    /* Tinte holográfico cian-verde */\n"
"    vec3 holo_tint=vec3(0.1,0.9,0.75);\n"
"    c+=holo_tint*beam*u_strength*0.5;\n"
"    c*=mix(1.0, 0.75+0.25*lines, u_strength*0.4);\n"
"\n"
"    /* Flicker Schumann */\n"
"    float flicker=1.0+0.02*sin(u_time*6.28318*7.83)*u_strength;\n"
"    fragColor=vec4(clamp(c*flicker,0.0,1.5), scene.a);\n"
"}\n";

static const char EFF_DUST_MOTES_FRAG[] =
"#version 300 es\n"
"precision highp float;\n"
"/* Dust Motes — Polvo ambiental flotante · RIgArt v3.0 */\n"
"uniform sampler2D u_tex;\n"
"uniform vec2  u_resolution;\n"
"uniform float u_time;\n"
"uniform float u_strength;\n"
"out vec4 fragColor;\n"
"\n"
"float hash21(vec2 p){\n"
"    p=fract(p*vec2(443.897,441.423));\n"
"    p+=dot(p,p.yx+19.19);\n"
"    return fract((p.x+p.y)*p.x);\n"
"}\n"
"\n"
"/* Mote individual — partícula circular suave */\n"
"float mote(vec2 uv, vec2 center, float sz, float softness){\n"
"    float d=length(uv-center);\n"
"    return smoothstep(sz, sz-softness, d);\n"
"}\n"
"\n"
"void main(){\n"
"    vec2 uv=gl_FragCoord.xy/u_resolution;\n"
"    vec4 scene=texture(u_tex,uv);\n"
"    float dust=0.0;\n"
"\n"
"    /* 24 partículas de polvo — distribución pseudo-aleatoria */\n"
"    for(int i=0;i<24;i++){\n"
"        vec2 seed=vec2(float(i)*127.1, float(i)*311.7);\n"
"        vec2 base=vec2(hash21(seed), hash21(seed+vec2(1.0)));\n"
"        float speed=hash21(seed+vec2(2.0))*0.04+0.01;\n"
"        float phase=hash21(seed+vec2(3.0))*6.28318;\n"
"        float sz=hash21(seed+vec2(4.0))*0.003+0.001;\n"
"\n"
"        /* Movimiento: deriva + oscilación suave φ */\n"
"        vec2 pos=base;\n"
"        pos.y=fract(pos.y - u_time*speed);\n"
"        pos.x+=sin(u_time*hash21(seed+vec2(5.0))+phase)*0.015;\n"
"\n"
"        /* Parallax de profundidad — las partículas más grandes son más cercanas */\n"
"        float depth=sz/0.004;\n"
"        float brightness=mix(0.3,1.0,depth)*u_strength;\n"
"        dust+=mote(uv, pos, sz, sz*0.6)*brightness;\n"
"    }\n"
"\n"
"    /* Tinte dorado-cálido para partículas de polvo */\n"
"    vec3 dust_color=vec3(1.0,0.95,0.75);\n"
"    fragColor=vec4(scene.rgb+dust_color*dust, scene.a);\n"
"}\n";

static const char EFF_VIGNETTE_HDR_FRAG[] =
"#version 300 es\n"
"precision highp float;\n"
"/* Vignette HDR — OLED luxury · RIgArt v3.0 · Adreno 720 */\n"
"uniform sampler2D u_tex;\n"
"uniform vec2  u_resolution;\n"
"uniform float u_time;\n"
"uniform float u_strength;   /* [0,1] */\n"
"uniform float u_exposure;   /* [0.5,2.0] */\n"
"uniform float u_hot_threshold; /* umbral hot pixels OLED */\n"
"out vec4 fragColor;\n"
"\n"
"/* Curva ACES filmica para HDR mapping */\n"
"vec3 aces_film(vec3 x){\n"
"    return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14), 0.0, 1.0);\n"
"}\n"
"\n"
"void main(){\n"
"    vec2 uv=gl_FragCoord.xy/u_resolution;\n"
"    vec4 base=texture(u_tex,uv);\n"
"    vec3 c=base.rgb*u_exposure;\n"
"\n"
"    /* Viñeta φ-elíptica — bordes naturales */\n"
"    vec2 cv=(uv-0.5)*vec2(1.0,1.0/1.6180);\n"
"    float vdist=dot(cv,cv);\n"
"    float vig=1.0-u_strength*smoothstep(0.0,1.0,vdist*3.2);\n"
"    vig=max(vig, 0.0);\n"
"\n"
"    /* Hot pixels protection — OLED burn mitigation */\n"
"    float lum=dot(c,vec3(0.2126,0.7152,0.0722));\n"
"    float hot=smoothstep(u_hot_threshold,1.0,lum);\n"
"    c=mix(c, c*0.85, hot*0.3);\n"
"\n"
"    /* HDR bloom subtle en bordes — OLED micro-glow */\n"
"    float edge_glow=pow(1.0-vig, 3.0)*0.12*u_strength;\n"
"    c+=vec3(0.8,0.7,0.5)*edge_glow;\n"
"\n"
"    c*=vig;\n"
"    c=aces_film(c);\n"
"    fragColor=vec4(c, base.a);\n"
"}\n";

static const char EFF_BLOOM_OLED_FRAG[] =
"#version 300 es\n"
"precision highp float;\n"
"/* Bloom OLED — Dual threshold Kawase · RIgArt v3.0 · Adreno 720 */\n"
"uniform sampler2D u_tex;\n"
"uniform sampler2D u_blur; /* pre-blurred pass */\n"
"uniform vec2  u_resolution;\n"
"uniform float u_threshold;     /* bloom base [0.6,0.9] */\n"
"uniform float u_hot_threshold; /* hot pixels OLED [0.85,0.98] */\n"
"uniform float u_strength;\n"
"uniform float u_time;\n"
"out vec4 fragColor;\n"
"\n"
"/* Kawase blur tap — single pass approximation */\n"
"vec3 kawase(sampler2D t, vec2 uv, vec2 px, float off){\n"
"    vec3 c =texture(t,uv+vec2( off, off)*px).rgb;\n"
"    c+=texture(t,uv+vec2(-off, off)*px).rgb;\n"
"    c+=texture(t,uv+vec2( off,-off)*px).rgb;\n"
"    c+=texture(t,uv+vec2(-off,-off)*px).rgb;\n"
"    return c*0.25;\n"
"}\n"
"\n"
"void main(){\n"
"    vec2 uv=gl_FragCoord.xy/u_resolution;\n"
"    vec2 px=1.0/u_resolution;\n"
"    vec4 scene=texture(u_tex,uv);\n"
"    float lum=dot(scene.rgb,vec3(0.2126,0.7152,0.0722));\n"
"\n"
"    /* Umbral base — bloom general */\n"
"    float base_bloom=max(lum-u_threshold, 0.0)/(1.0-u_threshold);\n"
"\n"
"    /* Umbral hot — OLED super-brights */\n"
"    float hot_bloom=max(lum-u_hot_threshold, 0.0)/(1.0-u_hot_threshold);\n"
"\n"
"    /* Kawase multi-tap: offset φ */\n"
"    vec3 b1=kawase(u_tex, uv, px, 1.0);\n"
"    vec3 b2=kawase(u_tex, uv, px, 1.0*(float)1.6180);\n"
"    vec3 b3=kawase(u_tex, uv, px, 2.6180);\n"
"    vec3 bloom_col=b1*0.5+b2*0.33+b3*0.17;\n"
"\n"
"    /* Bloom dorado para hot pixels OLED */\n"
"    vec3 hot_tint=vec3(1.0,0.9,0.6)*hot_bloom;\n"
"\n"
"    /* Schumann pulse en bloom */\n"
"    float pulse=1.0+0.04*sin(u_time*6.28318*7.83);\n"
"\n"
"    vec3 c=scene.rgb\n"
"          +bloom_col*base_bloom*u_strength*pulse\n"
"          +hot_tint*u_strength*0.5;\n"
"\n"
"    /* Soft ACES clamp — preserva colores HDR */\n"
"    c=c/(c+vec3(0.155))*1.019;\n"
"    fragColor=vec4(clamp(c,0.0,1.8), scene.a);\n"
"}\n";

static const char *s_effect_frags[RIGART_EFFECT_COUNT] = {
     NULL,
     NULL,
     EFF_DOF_BOKEH_FRAG,
     NULL,
     NULL,
     EFF_GLITCH_FRAG,
     EFF_GOD_RAYS_FRAG,
     EFF_CAUSTICS_FRAG,
     EFF_HOLOGRAM_SCAN_FRAG,
     EFF_DUST_MOTES_FRAG,
     EFF_VIGNETTE_HDR_FRAG,
     EFF_BLOOM_OLED_FRAG,
};

static const char EFFECT_VERT[] =
"#version 300 es\n"
"/* RIgArt v3.0 — post-process quad vertex */\n"
"in vec2 a_pos;\n"
"void main(){ gl_Position=vec4(a_pos,0.0,1.0); }\n";

int rigart_effect_glsl__rig_variant_c9fc5e13(const RIgArtEffectCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));

    const char *frag = (ctx->type < RIGART_EFFECT_COUNT)
                     ? s_effect_frags[ctx->type] : NULL;

    if (!frag) {

        Buf3c b = bc_new__rig_variant_3681e398(512);
        bc_printf__rig_variant_f2482fac(&b,
            "/* RIgArt v3.0 — GLSL %s · definido en rigart_v3.c */\n",
            rigart_effect_name(ctx->type));
        rset_glsl_frag__rig_variant_dee9c0db(out, &b);
        out->glsl_vert = strdup(EFFECT_VERT);
        return 1;
    }

    out->glsl_frag = strdup(frag);
    out->glsl_vert = strdup(EFFECT_VERT);
    out->ok        = true;
    out->certeza   = PHI3_INV;
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

int rigart_effect_css__rig_variant_5e946cde(const RIgArtEffectCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));

    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);
    const char *sel = ctx->selector[0] ? ctx->selector : ".rg-effect";
    float str = ctx->strength > 0 ? ctx->strength : 0.5f;
    float rad = ctx->radius_px > 0 ? ctx->radius_px : 20.0f;
    float spd = ctx->anim_speed > 0 ? ctx->anim_speed : 1.0f;

    bc_printf__rig_variant_f2482fac(&css, "/* RIgArt v3.0 — Effect CSS %s */\n@layer rg-effect {\n",
              rigart_effect_name(ctx->type));

    switch (ctx->type) {
        case RIGART_EFFECT_CHROMATIC:
            bc_printf__rig_variant_f2482fac(&css,
                "%s { filter:\n"
                "  drop-shadow(%.1fpx 0 %.1fpx rgba(255,0,0,%.2f))\n"
                "  drop-shadow(-%.1fpx 0 %.1fpx rgba(0,100,255,%.2f));\n"
                "}\n",
                sel, str*3.0f, rad*0.3f, str*0.8f,
                str*3.0f, rad*0.3f, str*0.8f);
            break;

        case RIGART_EFFECT_FILM_GRAIN:
            bc_printf__rig_variant_f2482fac(&css,
                "@keyframes rg-grain-css {\n"
                "  0%%,100%% { background-position: 0%% 0%%; }\n"
                "  50%%      { background-position: 100%% 100%%; }\n"
                "}\n"
                "%s::after {\n"
                "  content:''; position:absolute; inset:0;\n"
                "  background-image: url(\"data:image/svg+xml,%%3Csvg viewBox='0 0 200 200' xmlns='http://www.w3.org/2000/svg'%%3E%%3Cfilter id='n'%%3E%%3CfeTurbulence type='fractalNoise' baseFrequency='0.65' numOctaves='3' stitchTiles='stitch'/%%3E%%3C/filter%%3E%%3Crect width='100%%' height='100%%' filter='url(%%23n)' opacity='%.2f'/%%3E%%3C/svg%%3E\");\n"
                "  opacity: %.2f;\n"
                "  pointer-events:none; z-index:9999;\n"
                "  animation: rg-grain-css %.2fs steps(6) infinite;\n"
                "  mix-blend-mode: overlay;\n"
                "}\n",
                sel, str*0.5f, str*0.3f, 1.0f/spd);
            break;

        case RIGART_EFFECT_VIGNETTE_HDR:
            bc_printf__rig_variant_f2482fac(&css,
                "%s {\n"
                "  box-shadow: inset 0 0 %.1fpx %.1fpx rgba(0,0,0,%.2f);\n"
                "}\n",
                sel, rad*2.0f, rad, str*0.9f);
            break;

        case RIGART_EFFECT_SCAN_LINES:
            bc_printf__rig_variant_f2482fac(&css,
                "%s {\n"
                "  background-image: repeating-linear-gradient(\n"
                "    0deg,\n"
                "    transparent 0px, transparent 1px,\n"
                "    rgba(0,0,0,%.2f) 1px, rgba(0,0,0,%.2f) 2px\n"
                "  );\n"
                "  background-size: 100%% 4px;\n"
                "}\n",
                sel, str*0.4f, str*0.4f);
            break;

        case RIGART_EFFECT_BLOOM_OLED:
            bc_printf__rig_variant_f2482fac(&css,
                "@keyframes rg-bloom-pulse {\n"
                "  0%%,100%% { filter: brightness(1) saturate(1.2); }\n"
                "  50%%      { filter: brightness(%.2f) saturate(1.5); }\n"
                "}\n"
                "%s { animation: rg-bloom-pulse %.2fs ease-in-out infinite; }\n",
                1.0f + str * 0.3f,
                sel, 1.0f / (float)SCHUMANN3 / spd);
            break;

        case RIGART_EFFECT_GLITCH_ART:
            bc_printf__rig_variant_f2482fac(&css,
                "@keyframes rg-glitch-css {\n"
                "  0%%,90%%,100%% { transform:none; filter:none; }\n"
                "  91%%  { transform:translateX(%.1fpx); filter:hue-rotate(90deg); }\n"
                "  93%%  { transform:translateX(-%.1fpx) skewX(-2deg); }\n"
                "  95%%  { transform:none; filter:hue-rotate(-45deg); }\n"
                "}\n"
                "%s { animation: rg-glitch-css %.2fs steps(1) infinite; }\n",
                str*6.0f, str*4.0f, sel, 3.0f/spd);
            break;

        case RIGART_EFFECT_HOLOGRAM_SCAN:
            bc_printf__rig_variant_f2482fac(&css,
                "@keyframes rg-holo-scan-css {\n"
                "  0%%   { box-shadow: inset 0 0 %.1fpx rgba(0,229,255,%.2f); }\n"
                "  50%%  { box-shadow: inset 0 0 %.1fpx rgba(0,229,255,%.2f); }\n"
                "  100%% { box-shadow: inset 0 0 %.1fpx rgba(0,229,255,%.2f); }\n"
                "}\n"
                "%s {\n"
                "  animation: rg-holo-scan-css %.2fs ease-in-out infinite;\n"
                "  border: 1px solid rgba(0,229,255,%.2f);\n"
                "}\n",
                rad*0.5f, str*0.5f,
                rad,      str*0.9f,
                rad*0.5f, str*0.5f,
                sel, 1.0f/(float)SCHUMANN3/spd, str*0.4f);
            break;

        default:
            bc_printf__rig_variant_f2482fac(&css, "/* %s — sin variante CSS pura */\n",
                      rigart_effect_name(ctx->type));
            break;
    }

    bc_cat__rig_variant_99ba61aa(&css, "}\n");
    rset_css__rig_variant_2e374284(out, &css);
    return 0;
}

int rigart_effect_compose__rig_variant_305f5366(const RIgArtEffectCtx *effects, int count,
                           RIgArtResultV3 *out)
{
    if (!effects || count <= 0 || !out) return -1;
    memset(out, 0, sizeof(*out));

    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);
    bc_cat__rig_variant_99ba61aa(&css,
        "/* RIgArt v3.0 — Effect Compose stack */\n"
        "@layer rg-effect-compose {\n");

    for (int i = 0; i < count && i < RIGART_EFFECT_MAX_STACK; i++) {
        RIgArtResultV3 tmp = {0};
        rigart_effect_css__rig_variant_5e946cde(&effects[i], &tmp);
        if (tmp.css) {
            bc_cat__rig_variant_99ba61aa(&css, tmp.css);
            bc_cat__rig_variant_99ba61aa(&css, "\n");
        }
        rigart_free_result_v3(&tmp);
    }

    bc_cat__rig_variant_99ba61aa(&css, "}\n");
    rset_css__rig_variant_2e374284(out, &css);
    return 0;
}

int rigart_effect_html__rig_variant_0fd139ec(const RIgArtEffectCtx *ctx,
                        RIgArtMaterial base_mat,
                        RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));

    const char *eff_name = rigart_effect_name(ctx->type);
    const char *mat_name = rigart_material_name__rig_dup_66ec8909(base_mat);

    RIgArtResultV3 glsl_r = {0};
    rigart_effect_glsl__rig_variant_c9fc5e13(ctx, &glsl_r);
    const char *frag_src = glsl_r.glsl_frag
        ? glsl_r.glsl_frag
        : "/* shader en rigart_v3.c */";

    Buf3c h = bc_new__rig_variant_3681e398(RIGART_MAX_HTML_V3);

    bc_printf__rig_variant_f2482fac(&h,
        "<!DOCTYPE html>\n"
        "<html lang='es'>\n"
        "<head>\n"
        "<meta charset='UTF-8'>\n"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>\n"
        "<title>RIgArt v3.0 — %s · %s</title>\n"
        "<style>\n"
        "  *,*::before,*::after{box-sizing:border-box;margin:0;padding:0}\n"
        "  body{background:#04040c;display:flex;align-items:center;"
        "justify-content:center;min-height:100vh;overflow:hidden}\n"
        "  canvas{display:block;max-width:100vw;max-height:100vh}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<canvas id='rg-canvas' width='800' height='600'></canvas>\n"
        "<script>\n"
        "/* RIgArt v3.0 — %s effect · WebGL2 · Adreno 720 target */\n"
        "(function(){\n"
        "  const C=document.getElementById('rg-canvas');\n"
        "  const gl=C.getContext('webgl2',{alpha:false,antialias:false,\n"
        "    colorSpace:'display-p3',depth:false});\n"
        "  if(!gl){document.body.innerHTML='<p style=color:#f00>WebGL2 requerido</p>';return;}\n"
        "\n"
        "  /* Vertex shader */\n"
        "  const VS=`#version 300 es\n"
        "  in vec2 a_pos;\n"
        "  void main(){gl_Position=vec4(a_pos,0.0,1.0);}`;\n"
        "\n"
        "  /* Fragment shader — %s */\n"
        "  const FS=`%s`;\n"
        "\n"
        "  function compile(type,src){\n"
        "    const s=gl.createShader(type);\n"
        "    gl.shaderSource(s,src); gl.compileShader(s);\n"
        "    if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))\n"
        "      console.error(gl.getShaderInfoLog(s));\n"
        "    return s;\n"
        "  }\n"
        "  const prog=gl.createProgram();\n"
        "  gl.attachShader(prog,compile(gl.VERTEX_SHADER,VS));\n"
        "  gl.attachShader(prog,compile(gl.FRAGMENT_SHADER,FS));\n"
        "  gl.linkProgram(prog); gl.useProgram(prog);\n"
        "\n"
        "  /* Quad de pantalla completa */\n"
        "  const vbo=gl.createBuffer();\n"
        "  gl.bindBuffer(gl.ARRAY_BUFFER,vbo);\n"
        "  gl.bufferData(gl.ARRAY_BUFFER,\n"
        "    new Float32Array([-1,-1,1,-1,-1,1,-1,1,1,-1,1,1]),\n"
        "    gl.STATIC_DRAW);\n"
        "  const aPos=gl.getAttribLocation(prog,'a_pos');\n"
        "  gl.enableVertexAttribArray(aPos);\n"
        "  gl.vertexAttribPointer(aPos,2,gl.FLOAT,false,0,0);\n"
        "\n"
        "  /* Uniforms */\n"
        "  const uRes=gl.getUniformLocation(prog,'u_resolution');\n"
        "  const uTime=gl.getUniformLocation(prog,'u_time');\n"
        "  const uStr=gl.getUniformLocation(prog,'u_strength');\n"
        "  gl.uniform2f(uRes, C.width, C.height);\n"
        "  gl.uniform1f(uStr, %.3f);\n"
        "\n"
        "  let t0=performance.now();\n"
        "  function frame(){\n"
        "    const t=(performance.now()-t0)/1000;\n"
        "    gl.uniform1f(uTime,t);\n"
        "    gl.drawArrays(gl.TRIANGLES,0,6);\n"
        "    requestAnimationFrame(frame);\n"
        "  }\n"
        "  requestAnimationFrame(frame);\n"
        "})();\n"
        "</script>\n"
        "</body>\n"
        "</html>\n",
        eff_name, mat_name,
        eff_name, eff_name,
        frag_src,
        ctx->strength > 0 ? ctx->strength : 0.5f);

    rset_html__rig_variant_ee82cccd(out, &h);
    rigart_free_result_v3(&glsl_r);
    return 0;
}

const char *rigart_transition_name__rig_dup_8c45035d(RIgArtTransition t)
{
    static const char *n[] = {
        "phi-morph","liquid-warp","particle-scatter","particle-gather",
        "reveal-sweep","iris-open","shatter-glass","fold-space",
        "static-burst","liquid-metal"
    };
    return (t < RIGART_TRANS_COUNT) ? n[t] : "unknown";
}

const char *rigart_easing_str__rig_dup_e0a3dd41(RIgArtEasing e)
{
    static const char *s[] = {
        "linear",
        "cubic-bezier(0.618,0,0.382,1)",
        "linear(0,.01,.04,.09,.16,.25,.36,.49,.64,.81,1,\n"
        "        .97,.94,.91,.88,.86,.84,.83,.82,.81,.82,.84,.87,.9,.93,.96,\n"
        "        .98,1,1.01,1.01,1,1)",
        "cubic-bezier(0.9,0,1,1)",
        "cubic-bezier(0,0,0.1,1)",
        "cubic-bezier(0.34,1.56,0.64,1)",
        "cubic-bezier(0.5,-0.5,0.5,1.5)",
    };
    return (e < RIGART_EASE_COUNT) ? s[e] : "linear";
}

static void trans_phi_morph__rig_variant_01b19ca8(Buf3c *b, const RIgArtTransitionCtx *c)
{
    const char *cls = c->reverse ? "rg-phi-morph-out" : "rg-phi-morph-in";
    bc_printf(b,
        "/* §28 PHI MORPH — Transformación φ-eased */\n"
        "@keyframes %s {\n"
        "  from { opacity:0; transform:scale(%.3f) translateY(%.1fpx); }\n"
        "  to   { opacity:1; transform:scale(1) translateY(0); }\n"
        "}\n"
        ".%s {\n"
        "  animation: %s %.3fs %s %.3fs both;\n"
        "}\n",
        cls,
        c->scale_start > 0 ? c->scale_start : 0.9f,
        c->reverse ? -12.0f : 12.0f,
        cls, cls,
        c->duration_s > 0 ? c->duration_s : (float)PHI3_INV,
        rigart_easing_str(c->easing),
        c->delay_s > 0 ? c->delay_s : 0.0f);
    return 0;
}
static void trans_liquid_warp__rig_variant_24707fa8(Buf3c *b, const RIgArtTransitionCtx *c)
{
    float dur = c->duration_s > 0 ? c->duration_s : 0.7f;
    bc_printf(b,
        "/* §28 LIQUID WARP — Deformación clip-path líquida */\n"
        "@keyframes rg-liquid-warp-in {\n"
        "  0%%   { clip-path:circle(0%% at 50%% 50%%); opacity:0; }\n"
        "  40%%  { clip-path:circle(55%% at 50%% 50%%); opacity:0.8; }\n"
        "  70%%  { clip-path:ellipse(60%% 55%% at 50%% 50%%); }\n"
        "  85%%  { clip-path:ellipse(55%% 58%% at 48%% 52%%); }\n"
        "  100%% { clip-path:inset(0 0 0 0 round 0px); opacity:1; }\n"
        "}\n"
        "@keyframes rg-liquid-warp-out {\n"
        "  0%%   { clip-path:inset(0 0 0 0 round 0px); opacity:1; }\n"
        "  30%%  { clip-path:ellipse(55%% 58%% at 52%% 48%%); }\n"
        "  70%%  { clip-path:circle(55%% at 50%% 50%%); opacity:0.7; }\n"
        "  100%% { clip-path:circle(0%% at 50%% 50%%); opacity:0; }\n"
        "}\n"
        ".rg-liquid-warp-in  { animation:rg-liquid-warp-in  %.3fs %s %.3fs both; }\n"
        ".rg-liquid-warp-out { animation:rg-liquid-warp-out %.3fs %s %.3fs both; }\n",
        dur, rigart_easing_str(c->easing), c->delay_s,
        dur, rigart_easing_str(c->easing), c->delay_s);
    return 0;
}
static void trans_particle_scatter__rig_variant_eca81102(Buf3c *b, const RIgArtTransitionCtx *c)
{
    float dur = c->duration_s > 0 ? c->duration_s : 0.6f;
    uint32_t ac = c->accent_color ? c->accent_color : 0xFFD700;
    bc_printf(b,
        "/* §28 PARTICLE SCATTER — Dispersión en partículas JS + CSS */\n"
        "@keyframes rg-scatter-piece {\n"
        "  0%%   { opacity:1; transform:translate(0,0) scale(1) rotate(0deg); }\n"
        "  100%% { opacity:0; transform:\n"
        "            translate(var(--rg-px,0px),var(--rg-py,0px))\n"
        "            scale(var(--rg-ps,0.2))\n"
        "            rotate(var(--rg-pr,360deg)); }\n"
        "}\n"
        ".rg-scatter-piece {\n"
        "  animation: rg-scatter-piece %.3fs %s calc(var(--rg-i,0)*%.3fs + %.3fs) both;\n"
        "}\n"
        "/* JS helper — fragmenta el elemento en piezas */\n"
        "/* Color de acento: #%06X */\n",
        dur, rigart_easing_str(c->easing),
        c->stagger_delay > 0 ? c->stagger_delay : 0.015f,
        c->delay_s > 0 ? c->delay_s : 0.0f,
        ac & 0xFFFFFF);
    return 0;
}
static void trans_particle_gather__rig_variant_dd621e23(Buf3c *b, const RIgArtTransitionCtx *c)
{
    float dur = c->duration_s > 0 ? c->duration_s : 0.65f;
    bc_printf(b,
        "/* §28 PARTICLE GATHER — Coalescencia desde partículas */\n"
        "@keyframes rg-gather-piece {\n"
        "  0%%   { opacity:0; transform:\n"
        "            translate(var(--rg-px,0px),var(--rg-py,0px))\n"
        "            scale(0.1) rotate(var(--rg-pr,360deg)); }\n"
        "  100%% { opacity:1; transform:translate(0,0) scale(1) rotate(0deg); }\n"
        "}\n"
        ".rg-gather-piece {\n"
        "  animation: rg-gather-piece %.3fs %s calc(var(--rg-i,0)*%.3fs + %.3fs) both;\n"
        "}\n",
        dur, rigart_easing_str(c->easing),
        c->stagger_delay > 0 ? c->stagger_delay : 0.012f,
        c->delay_s > 0 ? c->delay_s : 0.0f);
    return 0;
}
static void trans_reveal_sweep__rig_variant_aa62689f(Buf3c *b, const RIgArtTransitionCtx *c)
{
    float dur = c->duration_s > 0 ? c->duration_s : 0.55f;
    uint32_t ac = c->accent_color ? c->accent_color : 0xFFD700;
    bc_printf(b,
        "/* §28 REVEAL SWEEP — Barrido de luz dorada */\n"
        "@property --rg-sweep-x {\n"
        "  syntax:'<percentage>'; inherits:false; initial-value:-10%%;\n"
        "}\n"
        "@keyframes rg-reveal-sweep {\n"
        "  0%%   {\n"
        "    --rg-sweep-x: -10%%;\n"
        "    clip-path: inset(0 100%% 0 0);\n"
        "    opacity:0;\n"
        "  }\n"
        "  30%%  { opacity:1; }\n"
        "  100%% {\n"
        "    --rg-sweep-x: 110%%;\n"
        "    clip-path: inset(0 0%% 0 0);\n"
        "  }\n"
        "}\n"
        ".rg-reveal-sweep {\n"
        "  position: relative;\n"
        "  animation: rg-reveal-sweep %.3fs %s %.3fs both;\n"
        "}\n"
        ".rg-reveal-sweep::after {\n"
        "  content:'';\n"
        "  position:absolute; inset:0;\n"
        "  background: linear-gradient(90deg,\n"
        "    transparent 0%%,\n"
        "    rgba(%d,%d,%d,0.7) 50%%,\n"
        "    transparent 100%%);\n"
        "  width:40%%;\n"
        "  left: var(--rg-sweep-x,-10%%);\n"
        "  pointer-events:none;\n"
        "  transition: left %.3fs;\n"
        "}\n",
        dur, rigart_easing_str(c->easing), c->delay_s,
        (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF,
        dur);
    return 0;
}
static void trans_iris_open__rig_variant_76fda554(Buf3c *b, const RIgArtTransitionCtx *c)
{
    float dur = c->duration_s > 0 ? c->duration_s : (float)PHI3_INV;
    bc_printf(b,
        "/* §28 IRIS OPEN — Iris circular φ-proporcional */\n"
        "@keyframes rg-iris-open {\n"
        "  0%%   { clip-path: circle(0%% at 50%% 50%%); opacity:0; }\n"
        "  10%%  { opacity:1; }\n"
        "  100%% { clip-path: circle(%.1f%% at 50%% 50%%); }\n"
        "}\n"
        "@keyframes rg-iris-close {\n"
        "  0%%   { clip-path: circle(%.1f%% at 50%% 50%%); opacity:1; }\n"
        "  90%%  { opacity:1; }\n"
        "  100%% { clip-path: circle(0%% at 50%% 50%%); opacity:0; }\n"
        "}\n"
        ".rg-iris-open  { animation: rg-iris-open  %.3fs %s %.3fs both; }\n"
        ".rg-iris-close { animation: rg-iris-close %.3fs %s %.3fs both; }\n",

        85.4f, 85.4f,
        dur, rigart_easing_str(c->easing), c->delay_s,
        dur, rigart_easing_str(c->easing), c->delay_s);
    return 0;
}
static void trans_shatter_glass__rig_variant_9e88f222(Buf3c *b, const RIgArtTransitionCtx *c)
{
    float dur = c->duration_s > 0 ? c->duration_s : 0.5f;
    bc_printf(b,
        "/* §28 SHATTER GLASS — Cristal que se rompe */\n"
        "@keyframes rg-shatter-piece {\n"
        "  0%%   { opacity:1; transform:translate(0,0) rotate(0deg) scale(1); filter:none; }\n"
        "  20%%  { filter: brightness(1.5); }\n"
        "  100%% {\n"
        "    opacity:0;\n"
        "    transform:\n"
        "      translate(var(--rg-sx,0px), var(--rg-sy,30px))\n"
        "      rotate(var(--rg-sr,45deg))\n"
        "      scale(var(--rg-ss,0.3));\n"
        "    filter: blur(3px) brightness(2);\n"
        "  }\n"
        "}\n"
        ".rg-shatter-piece {\n"
        "  animation: rg-shatter-piece %.3fs %s calc(var(--rg-i,0)*0.03s + %.3fs) forwards;\n"
        "}\n"
        ".rg-shatter-flash {\n"
        "  animation: rg-shatter-flash %.3fs ease-out;\n"
        "}\n"
        "@keyframes rg-shatter-flash {\n"
        "  0%%  { background:rgba(255,255,255,0.8); }\n"
        "  100%% { background:transparent; }\n"
        "}\n",
        dur, rigart_easing_str(c->easing), c->delay_s, dur * 0.15f);
    return 0;
}
static void trans_fold_space__rig_variant_5ee6b590(Buf3c *b, const RIgArtTransitionCtx *c)
{
    float dur = c->duration_s > 0 ? c->duration_s : 0.65f;
    bc_printf(b,
        "/* §28 FOLD SPACE — Pliegue 3D del espacio */\n"
        ".rg-fold-scene {\n"
        "  perspective: 900px;\n"
        "  perspective-origin: 50%% 40%%;\n"
        "}\n"
        "@keyframes rg-fold-space-in {\n"
        "  0%%   { transform: rotateX(-90deg) translateZ(-60px); opacity:0; }\n"
        "  60%%  { transform: rotateX(8deg) translateZ(5px); opacity:1; }\n"
        "  80%%  { transform: rotateX(-3deg) translateZ(-2px); }\n"
        "  100%% { transform: rotateX(0) translateZ(0); }\n"
        "}\n"
        "@keyframes rg-fold-space-out {\n"
        "  0%%   { transform: rotateX(0) translateZ(0); opacity:1; }\n"
        "  40%%  { transform: rotateX(8deg) translateZ(5px); }\n"
        "  100%% { transform: rotateX(90deg) translateZ(-60px); opacity:0; }\n"
        "}\n"
        ".rg-fold-in  { transform-style:preserve-3d; animation:rg-fold-space-in  %.3fs %s %.3fs both; }\n"
        ".rg-fold-out { transform-style:preserve-3d; animation:rg-fold-space-out %.3fs %s %.3fs both; }\n",
        dur, rigart_easing_str(c->easing), c->delay_s,
        dur, rigart_easing_str(c->easing), c->delay_s);
    return 0;
}
static void trans_static_burst__rig_variant_86e8c395(Buf3c *b, const RIgArtTransitionCtx *c)
{
    float dur = c->duration_s > 0 ? c->duration_s : 0.4f;
    bc_printf(b,
        "/* §28 STATIC BURST — Explosión estática glitch */\n"
        "@keyframes rg-static-in {\n"
        "  0%%   { opacity:0; filter:brightness(3) contrast(8) saturate(0); transform:scale(1.05); }\n"
        "  8%%   { opacity:0.7; filter:brightness(2) contrast(4) hue-rotate(90deg); }\n"
        "  15%%  { opacity:0.4; filter:brightness(1.5) contrast(2) hue-rotate(-30deg); }\n"
        "  25%%  { opacity:0.9; filter:none; transform:scale(1.02); }\n"
        "  40%%  { opacity:1; transform:scale(1); filter:none; }\n"
        "  100%% { opacity:1; transform:scale(1); filter:none; }\n"
        "}\n"
        "@keyframes rg-static-out {\n"
        "  0%%   { opacity:1; filter:none; transform:scale(1); }\n"
        "  60%%  { opacity:0.9; }\n"
        "  75%%  { opacity:0.5; filter:brightness(2) saturate(0) contrast(4); }\n"
        "  85%%  { opacity:0.3; filter:brightness(4) hue-rotate(180deg); }\n"
        "  100%% { opacity:0; filter:brightness(8); transform:scale(1.08); }\n"
        "}\n"
        ".rg-static-in  { animation: rg-static-in  %.3fs steps(1,end) %.3fs both; }\n"
        ".rg-static-out { animation: rg-static-out %.3fs steps(1,end) %.3fs both; }\n",
        dur, c->delay_s, dur, c->delay_s);
    return 0;
}
static void trans_liquid_metal__rig_variant_abd8b6e2(Buf3c *b, const RIgArtTransitionCtx *c)
{
    float dur = c->duration_s > 0 ? c->duration_s : 0.75f;
    uint32_t ac = c->accent_color ? c->accent_color : 0xC0C0C0;
    bc_printf(b,
        "/* §28 LIQUID METAL — Metamorfosis de mercurio */\n"
        "@property --rg-metal-r {\n"
        "  syntax:'<percentage>'; inherits:false; initial-value:0%%;\n"
        "}\n"
        "@keyframes rg-liquid-metal-in {\n"
        "  0%%   { --rg-metal-r:0%%; clip-path:polygon(50%% 0%%,50%% 0%%,50%% 100%%,50%% 100%%); opacity:0; }\n"
        "  20%%  { opacity:1; filter:drop-shadow(0 0 12px rgba(%d,%d,%d,0.8)); }\n"
        "  50%%  { clip-path:polygon(0%% 0%%,100%% 0%%,100%% 100%%,0%% 100%%); --rg-metal-r:20%%; }\n"
        "  80%%  { filter:drop-shadow(0 0 4px rgba(%d,%d,%d,0.3)); }\n"
        "  100%% { --rg-metal-r:0%%; clip-path:polygon(0%% 0%%,100%% 0%%,100%% 100%%,0%% 100%%); filter:none; }\n"
        "}\n"
        ".rg-liquid-metal-in {\n"
        "  border-radius: var(--rg-metal-r,0%%);\n"
        "  animation: rg-liquid-metal-in %.3fs %s %.3fs both;\n"
        "}\n",
        (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF,
        (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF,
        dur, rigart_easing_str(c->easing), c->delay_s);
    return 0;
}
int rigart_transition_css__rig_variant_36a774bb(const RIgArtTransitionCtx *ctx,
                           RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));
    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);

    bc_cat__rig_variant_99ba61aa(&css, "/* RIgArt v3.0 — Transition Engine · φ=1.618 */\n"
                 "@layer rg-transitions {\n");

    switch (ctx->type) {
        case RIGART_TRANS_PHI_MORPH:     trans_phi_morph__rig_variant_01b19ca8(&css, ctx);     break;
        case RIGART_TRANS_LIQUID_WARP:   trans_liquid_warp__rig_variant_24707fa8(&css, ctx);   break;
        case RIGART_TRANS_PARTICLE_SCAT: trans_particle_scatter__rig_variant_eca81102(&css, ctx); break;
        case RIGART_TRANS_PARTICLE_GAT:  trans_particle_gather__rig_variant_dd621e23(&css, ctx);  break;
        case RIGART_TRANS_REVEAL_SWEEP:  trans_reveal_sweep__rig_variant_aa62689f(&css, ctx);  break;
        case RIGART_TRANS_IRIS_OPEN:     trans_iris_open__rig_variant_76fda554(&css, ctx);     break;
        case RIGART_TRANS_SHATTER_GLASS: trans_shatter_glass__rig_variant_9e88f222(&css, ctx); break;
        case RIGART_TRANS_FOLD_SPACE:    trans_fold_space__rig_variant_5ee6b590(&css, ctx);    break;
        case RIGART_TRANS_STATIC_BURST:  trans_static_burst__rig_variant_86e8c395(&css, ctx);  break;
        case RIGART_TRANS_LIQUID_METAL:  trans_liquid_metal__rig_variant_abd8b6e2(&css, ctx);  break;
        default: break;
    }

    bc_cat__rig_variant_99ba61aa(&css, "}\n");
    rset_css__rig_variant_2e374284(out, &css);
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

int rigart_page_transition__rig_variant_f13f5be0(const RIgArtTransitionCtx *enter,
                            const RIgArtTransitionCtx *leave,
                            RIgArtResultV3 *out)
{
    if (!enter || !leave || !out) return -1;
    memset(out, 0, sizeof(*out));

    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);
    bc_cat__rig_variant_99ba61aa(&css, "/* RIgArt v3.0 — Page Transition · @view-transition */\n");

    bc_printf__rig_variant_f2482fac(&css,
        "@view-transition { navigation: auto; }\n"
        "\n"
        "/* Salida — página actual */\n"
        "::view-transition-old(root) {\n"
        "  animation-name: rg-page-out;\n"
        "  animation-duration: %.3fs;\n"
        "  animation-timing-function: %s;\n"
        "  animation-delay: %.3fs;\n"
        "  animation-fill-mode: both;\n"
        "}\n"
        "/* Entrada — página nueva */\n"
        "::view-transition-new(root) {\n"
        "  animation-name: rg-page-in;\n"
        "  animation-duration: %.3fs;\n"
        "  animation-timing-function: %s;\n"
        "  animation-delay: %.3fs;\n"
        "  animation-fill-mode: both;\n"
        "}\n",
        leave->duration_s > 0 ? leave->duration_s : 0.4f,
        rigart_easing_str__rig_dup_e0a3dd41(leave->easing),
        leave->delay_s > 0 ? leave->delay_s : 0.0f,
        enter->duration_s > 0 ? enter->duration_s : 0.5f,
        rigart_easing_str__rig_dup_e0a3dd41(enter->easing),
        enter->delay_s > 0 ? enter->delay_s : 0.0f);

    RIgArtTransitionCtx leave_mod = *leave;
    leave_mod.reverse = true;
    RIgArtResultV3 tmp_out = {0}, tmp_in = {0};
    rigart_transition_css__rig_variant_36a774bb(&leave_mod, &tmp_out);
    rigart_transition_css__rig_variant_36a774bb(enter, &tmp_in);

    bc_printf__rig_variant_f2482fac(&css,
        "\n@keyframes rg-page-out {\n"
        "  0%%   { opacity:1; transform:translateX(0); }\n"
        "  100%% { opacity:0; transform:translateX(-%.0fpx); }\n"
        "}\n"
        "@keyframes rg-page-in {\n"
        "  0%%   { opacity:0; transform:translateX(%.0fpx); }\n"
        "  100%% { opacity:1; transform:translateX(0); }\n"
        "}\n",
        20.0f * (float)PHI3,
        20.0f * (float)PHI3);

    if (tmp_out.css) bc_cat__rig_variant_99ba61aa(&css, tmp_out.css);
    if (tmp_in.css)  bc_cat__rig_variant_99ba61aa(&css, tmp_in.css);
    rigart_free_result_v3(&tmp_out);
    rigart_free_result_v3(&tmp_in);

    rset_css__rig_variant_2e374284(out, &css);
    return 0;
}

int rigart_element_enter__rig_dup_eab66077(const RIgArtTransitionCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    RIgArtTransitionCtx c = *ctx;
    c.reverse = false;
    return rigart_transition_css__rig_variant_36a774bb(&c, out);
}

int rigart_element_exit__rig_dup_914d46eb(const RIgArtTransitionCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    RIgArtTransitionCtx c = *ctx;
    c.reverse = true;
    return rigart_transition_css__rig_variant_36a774bb(&c, out);
}

const char *rigart_behavior_name__rig_dup_29af96b5(RIgArtBehavior b)
{
    static const char *n[] = {
        "magnetic","gyro-3d","inertial","pressure","spring",
        "parallax","gravity","orbital","ripple","fluid-trail"
    };
    return (b < RIGART_BEHAV_COUNT) ? n[b] : "unknown";
}

static void behav_magnetic__rig_variant_6523f400(Buf3c *b, const RIgArtBehaviorCtx *c)
{
    const char *sel = c->selector[0] ? c->selector : ".rg-magnetic";
    float range = c->range_px > 0 ? c->range_px : 80.0f;
    float str   = c->strength > 0 ? c->strength : 0.35f;
    float damp  = c->damping  > 0 ? c->damping  : 0.15f;
    float maxd  = c->max_disp_px > 0 ? c->max_disp_px : 24.0f;

    bc_printf(b,
        "(function rigMagnetic(){\n"
        "  const els=document.querySelectorAll('%s');\n"
        "  els.forEach(el=>{\n"
        "    let ox=0,oy=0,vx=0,vy=0,raf=0;\n"
        "    function spring(){\n"
        "      vx+=(ox-parseFloat(el.dataset.tx||0))*%.3f;\n"
        "      vy+=(oy-parseFloat(el.dataset.ty||0))*%.3f;\n"
        "      vx*=(1-%.3f); vy*=(1-%.3f);\n"
        "      ox+=vx; oy+=vy;\n"
        "      ox=Math.max(-%.1f,Math.min(%.1f,ox));\n"
        "      oy=Math.max(-%.1f,Math.min(%.1f,oy));\n"
        "      el.style.transform=`translate(${ox.toFixed(2)}px,${oy.toFixed(2)}px)`;\n"
        "      raf=requestAnimationFrame(spring);\n"
        "    }\n"
        "    el.addEventListener('mousemove',e=>{\n"
        "      const r=el.getBoundingClientRect();\n"
        "      const cx=r.left+r.width/2, cy=r.top+r.height/2;\n"
        "      const dx=e.clientX-cx, dy=e.clientY-cy;\n"
        "      const d=Math.hypot(dx,dy);\n"
        "      if(d<%.1f){\n"
        "        el.dataset.tx=dx*%.3f;\n"
        "        el.dataset.ty=dy*%.3f;\n"
        "      }\n"
        "    },{passive:true});\n"
        "    el.addEventListener('mouseleave',()=>{\n"
        "      el.dataset.tx=0; el.dataset.ty=0;\n"
        "    });\n"
        "    spring();\n"
        "  });\n"
        "})();\n",
        sel,
        str * 0.5f, str * 0.5f,
        damp, damp,
        maxd, maxd, maxd, maxd,
        range, str, str);
    return 0;
}
static void behav_gyro_3d__rig_variant_a44ebaca(Buf3c *b, const RIgArtBehaviorCtx *c)
{
    const char *sel = c->selector[0] ? c->selector : ".rg-gyro-3d";
    float maxr  = c->max_rot_deg  > 0 ? c->max_rot_deg  : 12.0f;
    float fov   = c->tilt_fov_px  > 0 ? c->tilt_fov_px  : 800.0f;
    float damp  = c->damping      > 0 ? c->damping       : 0.08f;

    bc_printf(b,
        "(function rigGyro3D(){\n"
        "  const els=document.querySelectorAll('%s');\n"
        "  if(!els.length) return;\n"
        "  let tx=0,ty=0,cx=0,cy=0;\n"
        "  const MAX=%.1f, DAMP=%.3f;\n"
        "\n"
        "  /* Gyroscopio en dispositivos móviles */\n"
        "  if(window.DeviceOrientationEvent){\n"
        "    window.addEventListener('deviceorientation',e=>{\n"
        "      if(e.beta==null) return;\n"
        "      ty=Math.max(-MAX,Math.min(MAX,(e.gamma||0)/3));\n"
        "      tx=Math.max(-MAX,Math.min(MAX,((e.beta||0)-45)/3));\n"
        "    },{passive:true});\n"
        "  } else {\n"
        "    /* Táctil nativo — Honor 400 120Hz */\n"
        "    window.addEventListener('mousemove',e=>{\n"
        "      const ox=(e.clientX/window.innerWidth-0.5)*2;\n"
        "      const oy=(e.clientY/window.innerHeight-0.5)*2;\n"
        "      ty=ox*MAX; tx=-oy*MAX;\n"
        "    },{passive:true});\n"
        "  }\n"
        "\n"
        "  (function loop(){\n"
        "    cx+=(tx-cx)*DAMP; cy+=(ty-cy)*DAMP;\n"
        "    els.forEach(el=>{\n"
        "      el.style.transform=\n"
        "        `perspective(%.0fpx) rotateX(${cy.toFixed(2)}deg) rotateY(${cx.toFixed(2)}deg)`;\n"
        "    });\n"
        "    requestAnimationFrame(loop);\n"
        "  })();\n"
        "})();\n",
        sel, maxr, damp, fov);
    return 0;
}
static void behav_inertial__rig_variant_026fac0b(Buf3c *b, const RIgArtBehaviorCtx *c)
{
    const char *sel = c->selector[0] ? c->selector : ".rg-inertial";
    float damp = c->damping > 0 ? c->damping : 0.12f;

    bc_printf(b,
        "(function rigInertial(){\n"
        "  const el=document.querySelector('%s');\n"
        "  if(!el) return;\n"
        "  let sy=el.scrollTop,vy=0,dragging=false,startY=0,startSY=0;\n"
        "  const DAMP=1-%.3f;\n"
        "\n"
        "  el.addEventListener('touchstart',e=>{\n"
        "    dragging=true; vy=0;\n"
        "    startY=e.touches[0].clientY;\n"
        "    startSY=el.scrollTop;\n"
        "  },{passive:true});\n"
        "  el.addEventListener('touchmove',e=>{\n"
        "    if(!dragging) return;\n"
        "    const dy=startY-e.touches[0].clientY;\n"
        "    el.scrollTop=startSY+dy;\n"
        "    vy=dy;\n"
        "  },{passive:true});\n"
        "  el.addEventListener('touchend',()=>{ dragging=false; });\n"
        "\n"
        "  (function inertiaLoop(){\n"
        "    if(!dragging && Math.abs(vy)>0.1){\n"
        "      el.scrollTop+=vy*0.016;\n"
        "      vy*=DAMP;\n"
        "    }\n"
        "    requestAnimationFrame(inertiaLoop);\n"
        "  })();\n"
        "})();\n",
        sel, damp);
    return 0;
}
static void behav_pressure__rig_variant_a498f0d8(Buf3c *b, const RIgArtBehaviorCtx *c)
{
    const char *sel = c->selector[0] ? c->selector : ".rg-pressure";
    bc_printf(b,
        "(function rigPressure(){\n"
        "  const els=document.querySelectorAll('%s');\n"
        "  els.forEach(el=>{\n"
        "    el.addEventListener('pointerdown',e=>{\n"
        "      /* Force disponible en stylus/Force Touch */\n"
        "      const force=e.pressure||0.5;\n"
        "      const sc=1-force*0.12;\n"
        "      el.style.transition='transform 0.05s ease';\n"
        "      el.style.transform=`scale(${sc})`;\n"
        "      el.style.filter=`brightness(${0.8+force*0.4})`;\n"
        "    });\n"
        "    el.addEventListener('pointerup',()=>{\n"
        "      el.style.transform='scale(1)';\n"
        "      el.style.filter='';\n"
        "      el.style.transition='transform 0.3s cubic-bezier(0.34,1.56,0.64,1)';\n"
        "    });\n"
        "    el.addEventListener('pointercancel',()=>{\n"
        "      el.style.transform='scale(1)'; el.style.filter='';\n"
        "    });\n"
        "  });\n"
        "})();\n",
        sel);
    return 0;
}
static void behav_spring__rig_variant_a5de3c1b(Buf3c *b, const RIgArtBehaviorCtx *c)
{
    const char *sel = c->selector[0] ? c->selector : ".rg-spring";
    float k     = c->spring_k    > 0 ? c->spring_k    : 0.18f;
    float mass  = c->spring_mass > 0 ? c->spring_mass : 1.0f;
    float damp  = c->damping     > 0 ? c->damping     : 0.22f;
    float maxd  = c->max_disp_px > 0 ? c->max_disp_px : 30.0f;

    bc_printf(b,
        "(function rigSpring(){\n"
        "  const els=document.querySelectorAll('%s');\n"
        "  const K=%.4f, M=%.2f, D=%.4f, MAX=%.1f;\n"
        "  els.forEach(el=>{\n"
        "    let px=0,py=0,vx=0,vy=0,tx=0,ty=0;\n"
        "    el.addEventListener('mousemove',e=>{\n"
        "      const r=el.getBoundingClientRect();\n"
        "      tx=(e.clientX-(r.left+r.width/2))*0.3;\n"
        "      ty=(e.clientY-(r.top+r.height/2))*0.3;\n"
        "      tx=Math.max(-MAX,Math.min(MAX,tx));\n"
        "      ty=Math.max(-MAX,Math.min(MAX,ty));\n"
        "    },{passive:true});\n"
        "    el.addEventListener('mouseleave',()=>{ tx=0; ty=0; });\n"
        "    (function loop(){\n"
        "      /* F = -k*(x-target) - damping*v */\n"
        "      const fx=(-K*(px-tx)-D*vx)/M;\n"
        "      const fy=(-K*(py-ty)-D*vy)/M;\n"
        "      vx+=fx; vy+=fy;\n"
        "      px+=vx; py+=vy;\n"
        "      el.style.transform=`translate(${px.toFixed(2)}px,${py.toFixed(2)}px)`;\n"
        "      requestAnimationFrame(loop);\n"
        "    })();\n"
        "  });\n"
        "})();\n",
        sel, k, mass, damp, maxd);
    return 0;
}
static void behav_parallax__rig_variant_96c936ee(Buf3c *b, const RIgArtBehaviorCtx *c)
{
    const char *sel = c->selector[0] ? c->selector : ".rg-parallax";;
    float str = c->strength > 0 ? c->strength : 0.3f;

    bc_printf(b,
        "(function rigParallax(){\n"
        "  const layers=[...document.querySelectorAll('%s [data-depth]')];\n"
        "  let mx=0,my=0,cx=0,cy=0;\n"
        "  window.addEventListener('mousemove',e=>{\n"
        "    mx=(e.clientX/window.innerWidth-0.5)*2;\n"
        "    my=(e.clientY/window.innerHeight-0.5)*2;\n"
        "  },{passive:true});\n"
        "  if(window.DeviceOrientationEvent){\n"
        "    window.addEventListener('deviceorientation',e=>{\n"
        "      mx=(e.gamma||0)/30; my=(e.beta||0)/30;\n"
        "    },{passive:true});\n"
        "  }\n"
        "  (function loop(){\n"
        "    cx+=(mx-cx)*0.08; cy+=(my-cy)*0.08;\n"
        "    layers.forEach(l=>{\n"
        "      const d=parseFloat(l.dataset.depth||0.5);\n"
        "      const dx=cx*d*%.1f*16;\n"
        "      const dy=cy*d*%.1f*12;\n"
        "      l.style.transform=`translate(${dx.toFixed(2)}px,${dy.toFixed(2)}px)`;\n"
        "    });\n"
        "    requestAnimationFrame(loop);\n"
        "  })();\n"
        "})();\n",
        sel, str * 100.0f, str * 100.0f);
    return 0;
}
static void behav_gravity__rig_variant_750f103f(Buf3c *b, const RIgArtBehaviorCtx *c)
{
    const char *sel = c->selector[0] ? c->selector : ".rg-gravity";
    float G   = c->gravity_g  > 0 ? c->gravity_g  : 0.4f;
    float damp = c->damping   > 0 ? c->damping     : 0.18f;
    float maxd = c->max_disp_px > 0 ? c->max_disp_px : 40.0f;

    bc_printf(b,
        "(function rigGravity(){\n"
        "  const el=document.querySelector('%s');\n"
        "  if(!el) return;\n"
        "  const G=%.3f, DAMP=%.3f, MAX=%.1f;\n"
        "  let x=0,y=0,vx=0,vy=0,gx=0,gy=G;\n"
        "\n"
        "  if(window.DeviceMotionEvent){\n"
        "    window.addEventListener('devicemotion',e=>{\n"
        "      const a=e.accelerationIncludingGravity||{};\n"
        "      gx=-(a.x||0)*G*0.1;\n"
        "      gy=(a.y||0)*G*0.1;\n"
        "    },{passive:true});\n"
        "  }\n"
        "\n"
        "  (function loop(){\n"
        "    vx+=gx; vy+=gy;\n"
        "    vx*=(1-DAMP); vy*=(1-DAMP);\n"
        "    x+=vx; y+=vy;\n"
        "    x=Math.max(-MAX,Math.min(MAX,x));\n"
        "    y=Math.max(-MAX,Math.min(MAX,y));\n"
        "    /* Rebote elástico en límites */\n"
        "    if(Math.abs(x)>=MAX){ vx*=-0.5; }\n"
        "    if(Math.abs(y)>=MAX){ vy*=-0.5; }\n"
        "    el.style.transform=`translate(${x.toFixed(2)}px,${y.toFixed(2)}px)`;\n"
        "    requestAnimationFrame(loop);\n"
        "  })();\n"
        "})();\n",
        sel, G, damp, maxd);
    return 0;
}
static void behav_orbital__rig_variant_ff6a78c5(Buf3c *b, const RIgArtBehaviorCtx *c)
{
    const char *sel = c->selector[0] ? c->selector : ".rg-orbital";
    float str = c->strength > 0 ? c->strength : 0.5f;
    float freq = c->frequency_hz > 0 ? c->frequency_hz : (float)PHI3_INV;

    bc_printf(b,
        "(function rigOrbital(){\n"
        "  const els=document.querySelectorAll('%s');\n"
        "  const FREQ=%.4f, STR=%.2f;\n"
        "  els.forEach((el,idx)=>{\n"
        "    let cx=0,cy=0,active=false;\n"
        "    const N=els.length;\n"
        "    const phase=idx*(Math.PI*2/N);\n"
        "    el.addEventListener('pointerenter',e=>{\n"
        "      const r=el.getBoundingClientRect();\n"
        "      cx=r.left+r.width/2; cy=r.top+r.height/2;\n"
        "      active=true;\n"
        "    });\n"
        "    el.addEventListener('pointerleave',()=>{ active=false; el.style.transform=''; });\n"
        "    let t=0;\n"
        "    (function loop(){\n"
        "      if(active){\n"
        "        t+=FREQ*0.016;\n"
        "        const r=6*STR;\n"
        "        const ox=Math.cos(t+phase)*r;\n"
        "        const oy=Math.sin(t+phase)*r*0.618;\n"
        "        el.style.transform=`translate(${ox.toFixed(2)}px,${oy.toFixed(2)}px)`;\n"
        "      }\n"
        "      requestAnimationFrame(loop);\n"
        "    })();\n"
        "  });\n"
        "})();\n",
        sel, freq, str);
    return 0;
}
static void behav_ripple__rig_variant_9b1dae0c(Buf3c *b, const RIgArtBehaviorCtx *c)
{
    const char *sel = c->selector[0] ? c->selector : ".rg-ripple";
    uint32_t tc = c->trail_color ? c->trail_color : 0xFFD700;
    float str = c->strength > 0 ? c->strength : 0.7f;

    bc_printf(b,
        "(function rigRipple(){\n"
        "  const els=document.querySelectorAll('%s');\n"
        "  els.forEach(el=>{\n"
        "    el.style.position='relative';\n"
        "    el.style.overflow='hidden';\n"
        "    el.addEventListener('pointerdown',e=>{\n"
        "      const r=el.getBoundingClientRect();\n"
        "      const x=e.clientX-r.left, y=e.clientY-r.top;\n"
        "      const rip=document.createElement('span');\n"
        "      const size=Math.max(r.width,r.height)*2;\n"
        "      rip.style.cssText=[\n"
        "        `position:absolute`,\n"
        "        `width:${size}px`,`height:${size}px`,\n"
        "        `left:${x-size/2}px`,`top:${y-size/2}px`,\n"
        "        `border-radius:50%%`,\n"
        "        `background:rgba(%d,%d,%d,%.2f)`,\n"
        "        `transform:scale(0)`,\n"
        "        `animation:rg-ripple-expand 0.6s cubic-bezier(0,0,0.2,1) forwards`,\n"
        "        `pointer-events:none`,`z-index:99`\n"
        "      ].join(';');\n"
        "      el.appendChild(rip);\n"
        "      rip.addEventListener('animationend',()=>rip.remove());\n"
        "    });\n"
        "  });\n"
        "  if(!document.getElementById('rg-ripple-style')){\n"
        "    const st=document.createElement('style');\n"
        "    st.id='rg-ripple-style';\n"
        "    st.textContent='@keyframes rg-ripple-expand{to{transform:scale(1);opacity:0}}';\n"
        "    document.head.appendChild(st);\n"
        "  }\n"
        "})();\n",
        sel,
        (tc>>16)&0xFF, (tc>>8)&0xFF, tc&0xFF, str * 0.35f);
    return 0;
}
static void behav_fluid_trail__rig_variant_4dcdee13(Buf3c *b, const RIgArtBehaviorCtx *c)
{
    uint32_t tc = c->trail_color ? c->trail_color : 0xFFD700;
    float tw    = c->trail_width > 0 ? c->trail_width : 3.0f;
    float str   = c->strength   > 0 ? c->strength    : 0.6f;

    bc_printf(b,
        "(function rigFluidTrail(){\n"
        "  const CVS=document.createElement('canvas');\n"
        "  CVS.style.cssText='position:fixed;inset:0;pointer-events:none;z-index:9998;opacity:%.2f';\n"
        "  document.body.appendChild(CVS);\n"
        "  const ctx=CVS.getContext('2d');\n"
        "  CVS.width=window.innerWidth; CVS.height=window.innerHeight;\n"
        "  window.addEventListener('resize',()=>{\n"
        "    CVS.width=window.innerWidth; CVS.height=window.innerHeight;\n"
        "  });\n"
        "\n"
        "  const pts=[];\n"
        "  const COLOR='rgba(%d,%d,%d,%.2f)';\n"
        "  const WIDTH=%.1f;\n"
        "\n"
        "  window.addEventListener('pointermove',e=>{\n"
        "    pts.push({x:e.clientX,y:e.clientY,t:Date.now()});\n"
        "    if(pts.length>64) pts.shift();\n"
        "  },{passive:true});\n"
        "\n"
        "  (function loop(){\n"
        "    ctx.clearRect(0,0,CVS.width,CVS.height);\n"
        "    const now=Date.now();\n"
        "    /* Eliminar puntos viejos (>%.0fms) */\n"
        "    while(pts.length>0 && now-pts[0].t>%.0f) pts.shift();\n"
        "    if(pts.length<2){ requestAnimationFrame(loop); return; }\n"
        "\n"
        "    ctx.beginPath();\n"
        "    ctx.moveTo(pts[0].x,pts[0].y);\n"
        "    for(let i=1;i<pts.length-1;i++){\n"
        "      /* Spline de Catmull-Rom → suavidad fluida */\n"
        "      const mx=(pts[i].x+pts[i+1].x)/2;\n"
        "      const my=(pts[i].y+pts[i+1].y)/2;\n"
        "      const age=(now-pts[i].t)/%.0f;\n"
        "      ctx.globalAlpha=Math.max(0,1-age);\n"
        "      ctx.quadraticCurveTo(pts[i].x,pts[i].y,mx,my);\n"
        "    }\n"
        "    ctx.strokeStyle=COLOR;\n"
        "    ctx.lineWidth=WIDTH;\n"
        "    ctx.lineCap='round';\n"
        "    ctx.lineJoin='round';\n"
        "    ctx.stroke();\n"
        "    ctx.globalAlpha=1;\n"
        "    requestAnimationFrame(loop);\n"
        "  })();\n"
        "})();\n",
        str,
        (tc>>16)&0xFF, (tc>>8)&0xFF, tc&0xFF, 0.85f,
        tw,
        600.0f, 600.0f, 600.0f);
    return 0;
}
int rigart_behavior_js__rig_variant_2f8cdaae(const RIgArtBehaviorCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));
    Buf3c js = bc_new__rig_variant_3681e398(RIGART_MAX_JS);

    bc_cat__rig_variant_99ba61aa(&js, "/* RIgArt v3.0 — Behavior Engine · φ=1.618 */\n"
                "(function rigartBehavior(){\n'use strict';\n");

    switch (ctx->type) {
        case RIGART_BEHAV_MAGNETIC:    behav_magnetic__rig_variant_6523f400(&js, ctx);    break;
        case RIGART_BEHAV_GYRO_3D:     behav_gyro_3d__rig_variant_a44ebaca(&js, ctx);     break;
        case RIGART_BEHAV_INERTIAL:    behav_inertial__rig_variant_026fac0b(&js, ctx);    break;
        case RIGART_BEHAV_PRESSURE:    behav_pressure__rig_variant_a498f0d8(&js, ctx);    break;
        case RIGART_BEHAV_SPRING:      behav_spring__rig_variant_a5de3c1b(&js, ctx);      break;
        case RIGART_BEHAV_PARALLAX:    behav_parallax__rig_variant_96c936ee(&js, ctx);    break;
        case RIGART_BEHAV_GRAVITY:     behav_gravity__rig_variant_750f103f(&js, ctx);     break;
        case RIGART_BEHAV_ORBITAL:     behav_orbital__rig_variant_ff6a78c5(&js, ctx);     break;
        case RIGART_BEHAV_RIPPLE:      behav_ripple__rig_variant_9b1dae0c(&js, ctx);      break;
        case RIGART_BEHAV_FLUID_TRAIL: behav_fluid_trail__rig_variant_4dcdee13(&js, ctx); break;
        default: break;
    }

    bc_cat__rig_variant_99ba61aa(&js, "})();\n");
    rset_js__rig_variant_54929c2f(out, &js);
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

int rigart_behavior_compose__rig_variant_04a94e9f(const RIgArtBehaviorCtx *behaviors,
                             int count, RIgArtResultV3 *out)
{
    if (!behaviors || count <= 0 || !out) return -1;
    memset(out, 0, sizeof(*out));
    Buf3c js = bc_new__rig_variant_3681e398(RIGART_MAX_JS);

    bc_cat__rig_variant_99ba61aa(&js, "/* RIgArt v3.0 — Behavior Compose · IIFE soberano */\n"
                "document.addEventListener('DOMContentLoaded',function(){\n"
                "'use strict';\n");

    for (int i = 0; i < count && i < RIGART_BEHAV_MAX_COMP; i++) {
        RIgArtResultV3 tmp = {0};
        rigart_behavior_js__rig_variant_2f8cdaae(&behaviors[i], &tmp);
        if (tmp.js) {
            bc_cat__rig_variant_99ba61aa(&js, "/* Behavior: ");
            bc_cat__rig_variant_99ba61aa(&js, rigart_behavior_name__rig_dup_29af96b5(behaviors[i].type));
            bc_cat__rig_variant_99ba61aa(&js, " */\n");
            bc_cat__rig_variant_99ba61aa(&js, tmp.js);
            bc_cat__rig_variant_99ba61aa(&js, "\n");
        }
        rigart_free_result_v3(&tmp);
    }

    bc_cat__rig_variant_99ba61aa(&js, "});\n");
    rset_js__rig_variant_54929c2f(out, &js);
    return 0;
}

int rigart_behavior_html__rig_dup_304ad628(const RIgArtBehaviorCtx *ctx,
                          RIgArtMaterial base_mat,
                          RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;

    RIgArtResultV3 js_r = {0};
    rigart_behavior_js__rig_variant_2f8cdaae(ctx, &js_r);

    Buf3c h = bc_new__rig_variant_3681e398(RIGART_MAX_HTML_V3);
    bc_printf__rig_variant_f2482fac(&h,
        "<!DOCTYPE html>\n"
        "<html lang='es'>\n"
        "<head>\n"
        "<meta charset='UTF-8'>\n"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>\n"
        "<title>RIgArt v3.0 — %s · %s</title>\n"
        "<style>\n"
        "  *,*::before,*::after{box-sizing:border-box;margin:0;padding:0}\n"
        "  body{background:#06050f;min-height:100vh;\n"
        "       display:flex;align-items:center;justify-content:center;\n"
        "       font-family:'Raleway','Helvetica Neue','Arial',sans-serif;color:#ffd700}\n"
        "  %s {\n"
        "    padding:48px 64px;\n"
        "    background:rgba(8,8,18,0.85);\n"
        "    border:1px solid rgba(255,215,0,0.3);\n"
        "    border-radius:16px;\n"
        "    cursor:pointer;\n"
        "    user-select:none;\n"
        "    will-change:transform;\n"
        "  }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<div class='%s'>RIgArt %s — %s</div>\n"
        "<script>\n%s\n</script>\n"
        "</body>\n</html>\n",
        rigart_behavior_name__rig_dup_29af96b5(ctx->type),
        rigart_material_name__rig_dup_66ec8909(base_mat),
        ctx->selector[0] ? ctx->selector : ".rg-demo",
        ctx->selector[0] ? ctx->selector + 1 : "rg-demo",
        rigart_behavior_name__rig_dup_29af96b5(ctx->type),
        rigart_material_name__rig_dup_66ec8909(base_mat),
        js_r.js ? js_r.js : "");

    rset_html__rig_variant_ee82cccd(out, &h);
    rigart_free_result_v3(&js_r);
    return 0;
}

static void oklch_to_srgb__rig_variant_395c3cac(float L, float C, float h,
                            float *ro, float *go, float *bo)
{

    float hr = h * (float)(PI3 / 180.0);
    float a  = C * cosf(hr);
    float b  = C * sinf(hr);

    float l_ = L + 0.3963377774f * a + 0.2158037573f * b;
    float m_ = L - 0.1055613458f * a - 0.0638541728f * b;
    float s_ = L - 0.0894841775f * a - 1.2914855480f * b;
    float lc = l_*l_*l_;
    float mc = m_*m_*m_;
    float sc = s_*s_*s_;

    float rl =  4.0767416621f*lc - 3.3077115913f*mc + 0.2309699292f*sc;
    float gl = -1.2684380046f*lc + 2.6097574011f*mc - 0.3413193965f*sc;
    float bl = -0.0041960863f*lc - 0.7034186147f*mc + 1.7076147010f*sc;

    #define SRGB_GAMMA(x) ((x)<=0.0031308f ? 12.92f*(x) : 1.055f*powf((x),1.0f/2.4f)-0.055f)
    *ro = SRGB_GAMMA(rl);
    *go = SRGB_GAMMA(gl);
    *bo = SRGB_GAMMA(bl);
    #undef SRGB_GAMMA
    return 0;
}
void rigart_color_oklch_str__rig_variant_a8774524(float L, float C, float h, float alpha,
                             char *out, size_t n)
{
    if (!out || !n) return 0;
    if (alpha >= 1.0f)
        rl_snprintf(out, n, "oklch(%.4f %.4f %.2f)", L, C, h);
    else
        rl_snprintf(out, n, "oklch(%.4f %.4f %.2f / %.3f)", L, C, h, alpha);
    return 0;
}

void rigart_color_p3_str__rig_variant_cdbfb0a7(float L, float C, float h, float alpha,
                          char *out, size_t n)
{
    if (!out || !n) return 0;
    float r, g, bv;
    oklch_to_srgb(L, C, h, &r, &g, &bv);

    float pr =  r * 0.8225f + g * 0.1774f + bv * 0.0f;
    float pg =  r * 0.0332f + g * 0.9669f + bv * 0.0f;
    float pb =  r * 0.0171f + g * 0.0724f + bv * 0.9108f;
    pr = fmaxf(0.0f, fminf(1.0f, pr));
    pg = fmaxf(0.0f, fminf(1.0f, pg));
    pb = fmaxf(0.0f, fminf(1.0f, pb));

    if (alpha >= 1.0f)
        rl_snprintf(out, n, "color(display-p3 %.4f %.4f %.4f)", pr, pg, pb);
    else
        rl_snprintf(out, n, "color(display-p3 %.4f %.4f %.4f / %.3f)", pr, pg, pb, alpha);
    return 0;
}

void rigart_color_blackbody__rig_variant_d00dd965(float temp_k,
                             float *ro, float *go, float *bo)
{
    float t = fmaxf(1000.0f, fminf(20000.0f, temp_k)) / 100.0f;
    float r, g, b;

    if (t <= 66.0f) {
        r = 1.0f;
        g = (0.39008157876901960784f * logf(t) - 0.63184144378862745098f);
        b = (t <= 19.0f) ? 0.0f :
            (0.54320678911019607843f * logf(t - 10.0f) - 1.19625408914f);
    } else {
        r = 1.29293618606274509804f * powf(t - 60.0f, -0.1332047592f);
        g = 1.12989086089529411765f * powf(t - 60.0f, -0.0755148492f);
        b = 1.0f;
    }
    *ro = fmaxf(0.0f, fminf(1.0f, r));
    *go = fmaxf(0.0f, fminf(1.0f, g));
    *bo = fmaxf(0.0f, fminf(1.0f, b));
    return 0;
}

int rigart_gradient_perceptual__rig_variant_bd711f7f(const RIgArtOKLCh *stops, int count,
                                float angle_deg, bool radial,
                                RIgArtResultV3 *out)
{
    if (!stops || count < 2 || !out) return -1;
    memset(out, 0, sizeof(*out));

    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);

    int interp = (count - 1) * 4 + 1;
    char oklch_s[64];

    if (radial)
        bc_cat__rig_variant_99ba61aa(&css, ".rg-grad-perceptual { background: radial-gradient(ellipse at center,\n");
    else
        bc_printf__rig_variant_f2482fac(&css, ".rg-grad-perceptual { background: linear-gradient(%.1fdeg,\n", angle_deg);

    for (int i = 0; i < interp; i++) {
        float t  = (float)i / (float)(interp - 1);
        int  seg = (int)(t * (count - 1));
        if (seg >= count - 1) seg = count - 2;
        float lt = t * (count - 1) - seg;

        float hr0 = stops[seg].h   * (float)(PI3 / 180.0);
        float hr1 = stops[seg+1].h * (float)(PI3 / 180.0);
        float a0  = stops[seg].C   * cosf(hr0);
        float b0  = stops[seg].C   * sinf(hr0);
        float a1  = stops[seg+1].C * cosf(hr1);
        float b1  = stops[seg+1].C * sinf(hr1);

        float L = stops[seg].L + lt * (stops[seg+1].L - stops[seg].L);
        float a = a0 + lt * (a1 - a0);
        float bv = b0 + lt * (b1 - b0);
        float C = sqrtf(a*a + bv*bv);
        float h = atan2f(bv, a) * (float)(180.0 / PI3);
        if (h < 0) h += 360.0f;
        float alpha = stops[seg].alpha + lt * (stops[seg+1].alpha - stops[seg].alpha);

        rigart_color_oklch_str__rig_variant_a8774524(L, C, h, alpha, oklch_s, sizeof(oklch_s));
        bc_printf__rig_variant_f2482fac(&css, "  %s %.1f%%", oklch_s, t * 100.0f);
        if (i < interp - 1) bc_cat__rig_variant_99ba61aa(&css, ",\n");
    }
    bc_cat__rig_variant_99ba61aa(&css, "\n); }\n");

    rset_css__rig_variant_2e374284(out, &css);
    return 0;
}

void rigart_palette_v3_forge__rig_variant_92c24e4a(RIgArtPaletteV3 *out,
                              float base_hue, float base_L,
                              float base_C, int count)
{
    if (!out) return 0;
    if (count < 2) count = 2;
    if (count > 8) count = 8;

    out->count      = count;
    out->phi_harmony = (float)PHI3_INV;
    out->temperature_k = 6500.0f;

    static const char *names[] = {
        "primary","secondary","tertiary","quaternary",
        "quinary","senary","septenary","octonary"
    };

    float phi_interval = 360.0f * (float)PHI3_INV;
    for (int i = 0; i < count; i++) {
        float hue = fmodf(base_hue + phi_interval * i, 360.0f);

        float L = base_L + ((i % 2 == 0) ? 0.0f : 0.1f * (float)PHI3_INV);
        float C = base_C * (1.0f - (float)i * 0.05f);
        L = fmaxf(0.0f, fminf(1.0f, L));
        C = fmaxf(0.0f, fminf(0.4f, C));

        out->colors[i].L     = L;
        out->colors[i].C     = C;
        out->colors[i].h     = hue;
        out->colors[i].alpha = 1.0f;
        rl_snprintf(out->names[i], sizeof(out->names[i]), "%s", names[i]);
    }

    char css[2048] = ":root {\n";
    for (int i = 0; i < count; i++) {
        char tmp[128];
        rigart_color_oklch_str(out->colors[i].L, out->colors[i].C,
                                out->colors[i].h, 1.0f, tmp, sizeof(tmp));
        char line[256];
        rl_snprintf(line, sizeof(line), "  --rg-color-%s: %s;\n",
                 out->names[i], tmp);
        rl_strncat(css, line, sizeof(css) - rl_strlen(css) - 1);
    }
    rl_strncat(css, "}\n", sizeof(css) - rl_strlen(css) - 1);
    rl_snprintf(out->css_vars, sizeof(out->css_vars), "%s", css);
    return 0;
}

void rigart_palette_v3_lerp__rig_variant_70c0e660(const RIgArtPaletteV3 *a,
                             const RIgArtPaletteV3 *b,
                             float t, RIgArtPaletteV3 *out)
{
    if (!a || !b || !out) return 0;
    int count = a->count < b->count ? a->count : b->count;
    out->count       = count;
    out->phi_harmony = a->phi_harmony + t * (b->phi_harmony - a->phi_harmony);
    out->temperature_k = a->temperature_k + t * (b->temperature_k - a->temperature_k);

    for (int i = 0; i < count; i++) {
        out->colors[i].L     = a->colors[i].L     + t * (b->colors[i].L     - a->colors[i].L);
        out->colors[i].C     = a->colors[i].C     + t * (b->colors[i].C     - a->colors[i].C);

        float dh = b->colors[i].h - a->colors[i].h;
        if (dh >  180.0f) dh -= 360.0f;
        if (dh < -180.0f) dh += 360.0f;
        out->colors[i].h     = fmodf(a->colors[i].h + t * dh + 360.0f, 360.0f);
        out->colors[i].alpha = a->colors[i].alpha + t * (b->colors[i].alpha - a->colors[i].alpha);
        rl_snprintf(out->names[i], sizeof(out->names[i]), "%s", a->names[i]);
    }
    rigart_palette_v3_forge(out, out->colors[0].h, out->colors[0].L,
                             out->colors[0].C, count);
    return 0;
}

int rigart_hdr_metadata_css__rig_variant_869796b9(float peak_nits, float avg_nits,
                              RIgArtResultV3 *out)
{
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);

    bc_printf__rig_variant_f2482fac(&css,
        "/* RIgArt v3.0 — HDR10 Metadata · peak:%.0f nits · avg:%.0f nits */\n"
        "@layer rg-hdr {\n"
        "\n"
        "  /* Soporte Display P3 wide gamut */\n"
        "  @media (color-gamut: p3) {\n"
        "    :root {\n"
        "      --rg-gamut: p3;\n"
        "      --rg-gold-primary: color(display-p3 0.90 0.76 0.12);\n"
        "      --rg-gold-shine:   color(display-p3 1.00 0.98 0.80);\n"
        "      --rg-cyan-accent:  color(display-p3 0.00 0.92 1.00);\n"
        "      --rg-deep-black:   color(display-p3 0.01 0.01 0.03);\n"
        "    }\n"
        "  }\n"
        "\n"
        "  /* Soporte BT.2020 */\n"
        "  @media (color-gamut: rec2020) {\n"
        "    :root {\n"
        "      --rg-gamut: rec2020;\n"
        "      --rg-gold-primary: color(rec2020 0.82 0.68 0.09);\n"
        "      --rg-gold-shine:   color(rec2020 0.97 0.95 0.75);\n"
        "      --rg-peak-nits: %.0f;\n"
        "      --rg-avg-nits:  %.0f;\n"
        "    }\n"
        "  }\n"
        "\n"
        "  /* HDR Media Query — Chrome 125+ */\n"
        "  @media (dynamic-range: high) {\n"
        "    :root {\n"
        "      --rg-hdr-active: 1;\n"
        "      --rg-bloom-intensity: %.3f;\n"
        "      --rg-oled-boost: %.2f;\n"
        "    }\n"
        "    /* Textos en HDR — luminosidad extendida */\n"
        "    .rg-hdr-text {\n"
        "      color: color(display-p3 1.0 0.97 0.85);\n"
        "      text-shadow:\n"
        "        0 0 8px color(display-p3 1.0 0.90 0.5 / 0.7),\n"
        "        0 0 24px color(display-p3 1.0 0.85 0.3 / 0.3);\n"
        "    }\n"
        "  }\n"
        "\n"
        "  /* Fallback sRGB coherente */\n"
        "  :root {\n"
        "    --rg-gamut: srgb;\n"
        "    --rg-gold-primary: #daa520;\n"
        "    --rg-gold-shine:   #fff8c0;\n"
        "    --rg-cyan-accent:  #00e5ff;\n"
        "    --rg-deep-black:   #04040c;\n"
        "    --rg-hdr-active: 0;\n"
        "    --rg-bloom-intensity: 0;\n"
        "    --rg-oled-boost: 1;\n"
        "    --rg-peak-nits: %.0f;\n"
        "    --rg-avg-nits:  %.0f;\n"
        "  }\n"
        "}\n",
        peak_nits, avg_nits,
        peak_nits, avg_nits,
        fminf(1.0f, peak_nits / 1000.0f),
        1.0f + peak_nits / 10000.0f,
        peak_nits, avg_nits);

    rset_css__rig_variant_2e374284(out, &css);
    return 0;
}

int rigart_palette_v3_css__rig_variant_1b99fba7(const RIgArtPaletteV3 *p, RIgArtResultV3 *out)
{
    if (!p || !out) return -1;
    memset(out, 0, sizeof(*out));
    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);

    bc_printf__rig_variant_f2482fac(&css,
        "/* RIgArt v3.0 — Palette CSS · φ-harmony=%.4f */\n"
        "@layer rg-palette {\n"
        ":root {\n",
        p->phi_harmony);

    for (int i = 0; i < p->count; i++) {
        char oklch_s[64], p3_s[64];
        rigart_color_oklch_str__rig_variant_a8774524(p->colors[i].L, p->colors[i].C,
                                p->colors[i].h, p->colors[i].alpha,
                                oklch_s, sizeof(oklch_s));
        rigart_color_p3_str__rig_variant_cdbfb0a7(p->colors[i].L, p->colors[i].C,
                             p->colors[i].h, p->colors[i].alpha,
                             p3_s, sizeof(p3_s));

        bc_printf__rig_variant_f2482fac(&css,
            "  /* %s · L=%.3f C=%.3f h=%.1f° */\n"
            "  --rg-%s:      %s;\n"
            "  --rg-%s-p3:   %s;\n",
            p->names[i],
            p->colors[i].L, p->colors[i].C, p->colors[i].h,
            p->names[i], oklch_s,
            p->names[i], p3_s);
    }

    bc_cat__rig_variant_99ba61aa(&css, "}\n}\n");
    rset_css__rig_variant_2e374284(out, &css);
    return 0;
}

const char *rigart_texture_name__rig_dup_64c145a7(RIgArtTexture t)
{
    static const char *n[] = {
        "brushed-gold","hammered-gold","polished-obsidian","faceted-crystal",
        "silk-weave","carbon-fiber","snakeskin","marble-calacatta",
        "linen-fine","patina-bronze","volcanic-rock","liquid-metal"
    };
    return (t < RIGART_TEX_COUNT) ? n[t] : "unknown";
}

const char *rigart_texture_desc__rig_dup_4ea485b3(RIgArtTexture t)
{
    static const char *d[] = {
        "Oro cepillado industrial anisótropo",
        "Oro martillado artesanal facetado",
        "Obsidiana pulida espejo PBR",
        "Cristal facetado prisma refractivo",
        "Seda tejida microscópica multifilamento",
        "Fibra de carbono trama diagonal 45°",
        "Piel exótica reptil escamas procedural",
        "Mármol Calacatta venas grises Perlin",
        "Lino fino hilado cruzado trama suave",
        "Bronce patinado Verdigris procedural",
        "Basalto volcánico poroso Voronoi",
        "Mercurio fluido curl-noise 3D",
    };
    return (t < RIGART_TEX_COUNT) ? d[t] : "";
}

static const char PBR_VERT[] =
"#version 300 es\n"
"precision highp float;\n"
"/* RIgArt v3.0 — PBR Vertex · φ=1.618 */\n"
"in vec3 a_pos;\n"
"in vec3 a_normal;\n"
"in vec2 a_uv;\n"
"in vec4 a_tangent;\n"
"uniform mat4 u_mvp;\n"
"uniform mat4 u_model;\n"
"uniform mat3 u_normal_mat;\n"
"out vec3 v_pos_ws;\n"
"out vec3 v_normal;\n"
"out vec2 v_uv;\n"
"out mat3 v_tbn;\n"
"void main(){\n"
"  vec4 ws=u_model*vec4(a_pos,1.0);\n"
"  v_pos_ws=ws.xyz;\n"
"  v_normal=normalize(u_normal_mat*a_normal);\n"
"  v_uv=a_uv;\n"
"  vec3 T=normalize(u_normal_mat*a_tangent.xyz);\n"
"  vec3 N=v_normal;\n"
"  vec3 B=cross(N,T)*a_tangent.w;\n"
"  v_tbn=mat3(T,B,N);\n"
"  gl_Position=u_mvp*vec4(a_pos,1.0);\n"
"}\n";

#define PBR_HEADER \
"#version 300 es\n" \
"precision highp float;\n" \
"uniform vec3  u_cam_pos;\n" \
"uniform vec3  u_light_dir;\n" \
"uniform vec3  u_light_col;\n" \
"uniform float u_time;\n" \
"uniform float u_roughness;\n" \
"uniform float u_metallic;\n" \
"uniform float u_scale;\n" \
"uniform float u_ao_strength;\n" \
"in vec3 v_pos_ws;\n" \
"in vec3 v_normal;\n" \
"in vec2 v_uv;\n" \
"in mat3 v_tbn;\n" \
"out vec4 fragColor;\n" \
"\n" \
"#define PHI 1.6180339887\n" \
"#define SCH 7.83\n" \
"\n" \
"/* GGX NDF */\n" \
"float GGX_D(float NdH, float a){\n" \
"  float a2=a*a; float d=NdH*NdH*(a2-1.0)+1.0;\n" \
"  return a2/(3.14159*d*d);\n" \
"}\n" \
"/* Schlick-GGX visibility */\n" \
"float GGX_V(float NdV, float NdL, float k){\n" \
"  float sv=NdV/(NdV*(1.0-k)+k);\n" \
"  float sl=NdL/(NdL*(1.0-k)+k);\n" \
"  return sv*sl;\n" \
"}\n" \
"/* Fresnel Schlick */\n" \
"vec3 Fresnel(float VdH, vec3 F0){\n" \
"  return F0+(1.0-F0)*pow(1.0-VdH,5.0);\n" \
"}\n" \
"/* PBR shading — Cook-Torrance */\n" \
"vec3 pbr_shade(vec3 N,vec3 V,vec3 L,vec3 albedo,float rough,float metal,vec3 light_col){\n" \
"  vec3 H=normalize(V+L);\n" \
"  float NdL=max(dot(N,L),0.0);\n" \
"  float NdV=max(dot(N,V),0.001);\n" \
"  float NdH=max(dot(N,H),0.0);\n" \
"  float VdH=max(dot(V,H),0.0);\n" \
"  float a=rough*rough;\n" \
"  float k=(rough+1.0)*(rough+1.0)/8.0;\n" \
"  vec3 F0=mix(vec3(0.04),albedo,metal);\n" \
"  float D=GGX_D(NdH,a);\n" \
"  float Gv=GGX_V(NdV,NdL,k);\n" \
"  vec3  Fv=Fresnel(VdH,F0);\n" \
"  vec3 specular=D*Gv*Fv/(4.0*NdV*NdL+0.001);\n" \
"  vec3 kD=(1.0-Fv)*(1.0-metal);\n" \
"  return (kD*albedo/3.14159+specular)*light_col*NdL;\n" \
"}\n"

#define HASH_FUNCS \
"float hash(vec2 p){p=fract(p*vec2(443.897,441.423));p+=dot(p,p.yx+19.19);return fract((p.x+p.y)*p.x);}\n" \
"vec2 hash2(vec2 p){return vec2(hash(p),hash(p+vec2(7.391,3.127)));}\n" \
"float hash3(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5);}\n"

static const char TEX_BRUSHED_GOLD_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 BRUSHED GOLD — Oro cepillado anisótropo · RIgArt v3.0 */\n"
"vec3 brushed_normal(vec2 uv, float sc){\n"
"  /* Rayaduras anisotrópicas horizontales */\n"
"  float n1=hash(vec2(floor(uv.y*sc*120.0),u_time*0.01))*0.5;\n"
"  float n2=hash(vec2(floor(uv.y*sc*300.0)+1.0,0.0))*0.25;\n"
"  float ny=n1+n2-0.375;\n"
"  return normalize(vec3(0.0, ny*0.4, 1.0));\n"
"}\n"
"void main(){\n"
"  vec2 uv=v_uv*u_scale;\n"
"  vec3 N=normalize(v_tbn*brushed_normal(uv,u_scale));\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"\n"
"  /* Albedo: oro 24K — tono cálido saturado */\n"
"  vec3 gold_base=vec3(0.83,0.68,0.22);\n"
"  vec3 gold_dark=vec3(0.48,0.36,0.08);\n"
"  /* Variación de tono por scratch */\n"
"  float scratch=hash(vec2(floor(uv.y*200.0),0.5));\n"
"  vec3 albedo=mix(gold_dark,gold_base, scratch);\n"
"\n"
"  /* Anisotropía — specular elongado en X */\n"
"  vec3 T=normalize(v_tbn[0]);\n"
"  float TdV=dot(T,V), TdL=dot(T,L);\n"
"  float aniso=sqrt(max(0.0, 1.0-TdV*TdV)*max(0.0,1.0-TdL*TdL));\n"
"  float rough_t=u_roughness*0.15;   /* tangencial suave */\n"
"  float rough_b=u_roughness*0.85;   /* binormal rugoso */\n"
"\n"
"  vec3 col=pbr_shade(N,V,L,albedo,rough_t,u_metallic,u_light_col);\n"
"\n"
"  /* Schumann shimmer */\n"
"  float pulse=1.0+0.04*sin(u_time*6.28318*SCH);\n"
"  col*=pulse;\n"
"\n"
"  /* ACES tonemapping */\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
"}\n";

static const char TEX_HAMMERED_GOLD_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 HAMMERED GOLD — Oro martillado facetado · RIgArt v3.0 */\n"
"/* Voronoi de facetas de martillo */\n"
"vec2 voronoi(vec2 p){\n"
"  vec2 i=floor(p), f=fract(p);\n"
"  float md=8.0; vec2 mc=vec2(0.0);\n"
"  for(int x=-1;x<=1;x++) for(int y=-1;y<=1;y++){\n"
"    vec2 r=vec2(x,y);\n"
"    vec2 rp=hash2(i+r)+r-f;\n"
"    float d=dot(rp,rp);\n"
"    if(d<md){md=d; mc=rp;}\n"
"  }\n"
"  return vec2(sqrt(md), length(mc));\n"
"}\n"
"void main(){\n"
"  vec2 uv=v_uv*u_scale*4.0;\n"
"  vec2 vc=voronoi(uv);\n"
"  /* Normal de faceta — gradiente de Voronoi */\n"
"  float eps=0.02;\n"
"  float dx=voronoi(uv+vec2(eps,0.0)).x-voronoi(uv-vec2(eps,0.0)).x;\n"
"  float dy=voronoi(uv+vec2(0.0,eps)).x-voronoi(uv-vec2(0.0,eps)).x;\n"
"  vec3 Nmap=normalize(vec3(-dx,dy,0.25)*2.0);\n"
"  vec3 N=normalize(v_tbn*Nmap);\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"\n"
"  /* Albedo: variación por faceta */\n"
"  float facet=smoothstep(0.0,0.15,vc.x);\n"
"  vec3 albedo=mix(vec3(0.9,0.78,0.28),vec3(0.62,0.48,0.10),facet);\n"
"\n"
"  float rough=u_roughness*0.4+vc.x*0.3;\n"
"  vec3 col=pbr_shade(N,V,L,albedo,rough,u_metallic,u_light_col);\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
"}\n";

static const char TEX_POLISHED_OBS_FRAG[] =
PBR_HEADER
"/* §31 POLISHED OBSIDIAN — Espejo negro PBR · RIgArt v3.0 */\n"
"void main(){\n"
"  vec3 N=normalize(v_normal);\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"  /* Obsidiana: casi negro, muy metálico, reflejo especular limpio */\n"
"  vec3 albedo=vec3(0.02,0.02,0.025);\n"
"  float rough=u_roughness*0.05; /* ultra-pulido */\n"
"  vec3 col=pbr_shade(N,V,L,albedo,rough,0.98,u_light_col);\n"
"  /* Reflejo ambiente — esfera IBL simulada */\n"
"  vec3 R=reflect(-V,N);\n"
"  vec3 sky=mix(vec3(0.02,0.02,0.06), vec3(0.06,0.04,0.12), R.y*0.5+0.5);\n"
"  col+=sky*0.35;\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
"}\n";

static const char TEX_CRYSTAL_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 FACETED CRYSTAL — Prisma refractivo · RIgArt v3.0 */\n"
"void main(){\n"
"  vec2 uv=v_uv*u_scale*3.0;\n"
"  /* Perturbación de normal por facetas */\n"
"  float nx=hash(uv+vec2(0.0,u_time*0.02))-0.5;\n"
"  float ny=hash(uv+vec2(0.5,u_time*0.02))-0.5;\n"
"  vec3 Nmap=normalize(vec3(nx,ny,1.0)*vec3(0.6,0.6,1.0));\n"
"  vec3 N=normalize(v_tbn*Nmap);\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"\n"
"  /* Dispersión prismática — tinte diferente por canal */\n"
"  float ior=1.52;\n"
"  vec3 R=refract(-V,N,1.0/ior);\n"
"  float disp=length(R)*0.02;\n"
"  vec3 albedo=vec3(\n"
"    0.92+disp,\n"
"    0.9+hash(uv)*0.08,\n"
"    0.95+disp*1.3);\n"
"\n"
"  float rough=u_roughness*0.08;\n"
"  vec3 col=pbr_shade(N,V,L,albedo,rough,0.0,u_light_col);\n"
"  /* Caustic de prisma */\n"
"  float caus=pow(max(dot(R,L),0.0),32.0);\n"
"  col+=vec3(0.8,0.9,1.0)*caus*0.5;\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.6),0.85);\n"
"}\n";

static const char TEX_SILK_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 SILK WEAVE — Seda microscópica · RIgArt v3.0 */\n"
"void main(){\n"
"  vec2 uv=v_uv*u_scale*20.0;\n"
"  /* Trama de tejido — modulación sinusoidal cruzada */\n"
"  float wu=sin(uv.x*3.14159*2.0);\n"
"  float wv=sin(uv.y*3.14159*2.0);\n"
"  float weave=abs(wu*wv);\n"
"\n"
"  /* Normal de hilo */\n"
"  float nx=cos(uv.x*3.14159*2.0)*0.3;\n"
"  float ny=cos(uv.y*3.14159*2.0)*0.3;\n"
"  vec3 N=normalize(v_tbn*normalize(vec3(nx,ny,1.0)));\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"\n"
"  /* Albedo dorado-champagne */\n"
"  vec3 silk_base=vec3(0.92,0.82,0.62);\n"
"  vec3 silk_dark=vec3(0.55,0.45,0.30);\n"
"  vec3 albedo=mix(silk_dark,silk_base,weave);\n"
"\n"
"  float rough=mix(0.55,0.15,weave);\n"
"  vec3 col=pbr_shade(N,V,L,albedo,rough,0.05,u_light_col);\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
"}\n";

static const char TEX_CARBON_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 CARBON FIBER — Fibra de carbono diagonal 45° · RIgArt v3.0 */\n"
"void main(){\n"
"  vec2 uv45=v_uv*u_scale*12.0;\n"
"  vec2 rot45=vec2((uv45.x+uv45.y)*0.70711,(uv45.y-uv45.x)*0.70711);\n"
"  float fu=fract(rot45.x), fv=fract(rot45.y);\n"
"  float fiber=max(\n"
"    smoothstep(0.4,0.5,fu)-smoothstep(0.5,0.6,fu),\n"
"    smoothstep(0.4,0.5,fv)-smoothstep(0.5,0.6,fv));\n"
"  vec3 N=normalize(v_normal);\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"  vec3 albedo=mix(vec3(0.04,0.04,0.05),vec3(0.12,0.12,0.14),fiber);\n"
"  float rough=mix(0.08,0.45,fiber);\n"
"  vec3 col=pbr_shade(N,V,L,albedo,rough,0.95,u_light_col);\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
"}\n";

static const char TEX_SNAKESKIN_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 SNAKESKIN — Piel reptil escamas procedural · RIgArt v3.0 */\n"
"void main(){\n"
"  vec2 uv=v_uv*u_scale*8.0;\n"
"  /* Hexagonal tiling para escamas */\n"
"  vec2 p=uv; p.x+=0.5*floor(mod(p.y,2.0));\n"
"  vec2 f=fract(p)-0.5; vec2 i=floor(p);\n"
"  float d=length(f)-0.38;\n"
"  float scale_edge=smoothstep(0.0,0.04,d);\n"
"  float scale_dark=hash(i)*0.3+0.1;\n"
"  vec3 albedo=mix(vec3(0.05,0.12,0.08),vec3(0.25,0.38,0.18)*scale_dark,scale_edge);\n"
"  float nx=(hash(i+vec2(0.1,0.0))-0.5)*0.4;\n"
"  float ny=(hash(i+vec2(0.0,0.1))-0.5)*0.4;\n"
"  vec3 N=normalize(v_tbn*normalize(vec3(nx*scale_edge,ny*scale_edge,1.0)));\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"  float rough=mix(0.12,0.65,scale_edge);\n"
"  vec3 col=pbr_shade(N,V,L,albedo,rough,0.1,u_light_col);\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
"}\n";

static const char TEX_MARBLE_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 MARBLE CALACATTA — Venas Perlin grises · RIgArt v3.0 */\n"
"float fbm(vec2 p){\n"
"  float v=0.0,a=0.5;\n"
"  for(int i=0;i<5;i++){ v+=a*hash(p); p*=2.0; a*=0.5; }\n"
"  return v;\n"
"}\n"
"void main(){\n"
"  vec2 uv=v_uv*u_scale*2.0;\n"
"  float vein=abs(sin(uv.x*3.14159+fbm(uv*3.0)*2.5));\n"
"  vein=pow(vein,3.0);\n"
"  vec3 white=vec3(0.96,0.95,0.93);\n"
"  vec3 grey=vec3(0.55,0.52,0.50);\n"
"  vec3 albedo=mix(white,grey,vein*0.7+fbm(uv*8.0)*0.08);\n"
"  vec3 N=normalize(v_normal);\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"  vec3 col=pbr_shade(N,V,L,albedo,0.08,0.0,u_light_col);\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
"}\n";

static const char TEX_LINEN_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 LINEN FINE — Lino fino hilado · RIgArt v3.0 */\n"
"void main(){\n"
"  vec2 uv=v_uv*u_scale*30.0;\n"
"  float h=sin(uv.x*3.14159*2.0)*0.5+0.5;\n"
"  float v2=sin(uv.y*3.14159*2.0)*0.5+0.5;\n"
"  float weave=mix(h,v2,0.5);\n"
"  vec3 albedo=mix(vec3(0.82,0.76,0.62),vec3(0.95,0.90,0.78),weave);\n"
"  albedo+=hash(uv)*0.04-0.02;\n"
"  vec3 N=normalize(v_normal);\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"  vec3 col=pbr_shade(N,V,L,albedo,0.75,0.0,u_light_col);\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
"}\n";

static const char TEX_PATINA_BRONZE_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 PATINA BRONZE — Bronce patinado Verdigris · RIgArt v3.0 */\n"
"float fbm3(vec2 p){float v=0.0,a=0.5;for(int i=0;i<4;i++){v+=a*hash(p);p*=2.0;a*=0.5;}return v;}\n"
"void main(){\n"
"  vec2 uv=v_uv*u_scale*3.0;\n"
"  float patina=fbm3(uv);\n"
"  vec3 bronze=vec3(0.72,0.45,0.20);\n"
"  vec3 verdigris=vec3(0.18,0.58,0.42);\n"
"  vec3 albedo=mix(bronze,verdigris,smoothstep(0.4,0.7,patina));\n"
"  float rough=mix(0.35,0.80,patina);\n"
"  vec3 N=normalize(v_normal);\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"  vec3 col=pbr_shade(N,V,L,albedo,rough,0.6,u_light_col);\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
"}\n";

static const char TEX_VOLCANIC_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 VOLCANIC ROCK — Basalto Voronoi poroso · RIgArt v3.0 */\n"
"vec2 voronoi_v(vec2 p){vec2 i=floor(p),f=fract(p);float md=8.0;vec2 mc=vec2(0.0);for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++){vec2 r=vec2(x,y);vec2 rp=hash2(i+r)+r-f;float d=dot(rp,rp);if(d<md){md=d;mc=rp;}}return vec2(sqrt(md),length(mc));}\n"
"void main(){\n"
"  vec2 uv=v_uv*u_scale*5.0;\n"
"  vec2 vc=voronoi_v(uv);\n"
"  float pore=smoothstep(0.0,0.08,vc.x);\n"
"  vec3 albedo=mix(vec3(0.04,0.04,0.05),vec3(0.18,0.16,0.14),pore);\n"
"  float rough=mix(0.9,0.6,pore);\n"
"  float nx=voronoi_v(uv+vec2(0.02,0.0)).x-voronoi_v(uv-vec2(0.02,0.0)).x;\n"
"  float ny=voronoi_v(uv+vec2(0.0,0.02)).x-voronoi_v(uv-vec2(0.0,0.02)).x;\n"
"  vec3 N=normalize(v_tbn*normalize(vec3(-nx,-ny,0.3)*2.0));\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"  vec3 col=pbr_shade(N,V,L,albedo,rough,0.05,u_light_col);\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.0),1.0);\n"
"}\n";

static const char TEX_LIQUID_METAL_FRAG[] =
PBR_HEADER
HASH_FUNCS
"/* §31 LIQUID METAL — Mercurio curl-noise 3D · RIgArt v3.0 */\n"
"vec2 curl(vec2 p, float t){\n"
"  float eps=0.01;\n"
"  float h1=hash(p+vec2(0.0,eps)+t*0.1);\n"
"  float h2=hash(p-vec2(0.0,eps)+t*0.1);\n"
"  float h3=hash(p+vec2(eps,0.0)+t*0.1);\n"
"  float h4=hash(p-vec2(eps,0.0)+t*0.1);\n"
"  return vec2((h1-h2),(h4-h3))/(2.0*eps);\n"
"}\n"
"void main(){\n"
"  vec2 uv=v_uv*u_scale*4.0;\n"
"  vec2 flow=curl(uv,u_time)*0.3;\n"
"  vec2 uv2=uv+flow;\n"
"  float metal=hash(uv2);\n"
"  vec3 albedo=mix(vec3(0.68,0.72,0.78),vec3(0.92,0.94,0.96),metal);\n"
"  float nx=(hash(uv2+vec2(0.02,0.0))-hash(uv2-vec2(0.02,0.0)))*3.0;\n"
"  float ny=(hash(uv2+vec2(0.0,0.02))-hash(uv2-vec2(0.0,0.02)))*3.0;\n"
"  vec3 N=normalize(v_tbn*normalize(vec3(nx,ny,1.0)));\n"
"  vec3 V=normalize(u_cam_pos-v_pos_ws);\n"
"  vec3 L=normalize(u_light_dir);\n"
"  float pulse=1.0+0.04*sin(u_time*6.28318*7.83);\n"
"  vec3 col=pbr_shade(N,V,L,albedo,0.06*pulse,0.98,u_light_col);\n"
"  col=col*(2.51*col+0.03)/(col*(2.43*col+0.59)+0.14);\n"
"  fragColor=vec4(clamp(col,0.0,1.5),1.0);\n"
"}\n";

static const char *s_tex_frags[RIGART_TEX_COUNT] = {
    TEX_BRUSHED_GOLD_FRAG,
    TEX_HAMMERED_GOLD_FRAG,
    TEX_POLISHED_OBS_FRAG,
    TEX_CRYSTAL_FRAG,
    TEX_SILK_FRAG,
    TEX_CARBON_FRAG,
    TEX_SNAKESKIN_FRAG,
    TEX_MARBLE_FRAG,
    TEX_LINEN_FRAG,
    TEX_PATINA_BRONZE_FRAG,
    TEX_VOLCANIC_FRAG,
    TEX_LIQUID_METAL_FRAG,
};

int rigart_texture_glsl__rig_variant_a52acd92(const RIgArtTextureCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));
    if (ctx->type >= RIGART_TEX_COUNT) return -1;

    out->glsl_frag = strdup(s_tex_frags[ctx->type]);
    out->glsl_vert = strdup(PBR_VERT);
    out->ok        = true;
    out->certeza   = PHI3_INV;
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

int rigart_texture_css__rig_variant_8fd51b45(const RIgArtTextureCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));

    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);
    const char *name = rigart_texture_name__rig_dup_64c145a7(ctx->type);

    bc_printf__rig_variant_f2482fac(&css,
        "/* RIgArt v3.0 — Texture CSS · %s */\n"
        "@layer rg-texture {\n"
        ".rg-tex-%s {\n"
        "  background-image: url(\"data:image/svg+xml,"
        "%%3Csvg xmlns='http://www.w3.org/2000/svg' width='200' height='200'%%3E"
        "%%3Cfilter id='n'%%3E%%3CfeTurbulence type='fractalNoise' "
        "baseFrequency='%.2f' numOctaves='4' stitchTiles='stitch'/%%3E"
        "%%3CfeColorMatrix type='saturate' values='0'/%%3E%%3C/filter%%3E"
        "%%3Crect width='100%%' height='100%%' filter='url(%%23n)' opacity='%.2f'/%%3E"
        "%%3C/svg%%3E\");\n"
        "  background-repeat: repeat;\n"
        "  background-size: %.0fpx %.0fpx;\n"
        "}\n"
        "}\n",
        name, name,
        ctx->scale > 0 ? 0.65f / ctx->scale : 0.65f,
        ctx->tint_strength > 0 ? ctx->tint_strength * 0.4f : 0.2f,
        100.0f / (ctx->scale > 0 ? ctx->scale : 1.0f),
        100.0f / (ctx->scale > 0 ? ctx->scale : 1.0f));

    rset_css__rig_variant_2e374284(out, &css);
    return 0;
}

int rigart_pbr_material_html__rig_variant_6a151049(const RIgArtTextureCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));

    RIgArtResultV3 glsl_r = {0};
    rigart_texture_glsl__rig_variant_a52acd92(ctx, &glsl_r);

    Buf3c h = bc_new__rig_variant_3681e398(RIGART_MAX_HTML_V3);
    bc_printf__rig_variant_f2482fac(&h,
        "<!DOCTYPE html>\n"
        "<html lang='es'>\n"
        "<head>\n"
        "<meta charset='UTF-8'>\n"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>\n"
        "<title>RIgArt v3.0 — PBR %s</title>\n"
        "<style>*{margin:0;padding:0;box-sizing:border-box}"
        "body{background:#04040c;display:flex;align-items:center;"
        "justify-content:center;min-height:100vh}"
        "canvas{display:block}</style>\n"
        "</head>\n"
        "<body>\n"
        "<canvas id='rg-canvas' width='600' height='600'></canvas>\n"
        "<script>\n"
        "/* RIgArt v3.0 — PBR %s · WebGL2 · Adreno 720 */\n"
        "(function(){\n"
        "  const C=document.getElementById('rg-canvas');\n"
        "  const gl=C.getContext('webgl2',{colorSpace:'display-p3'});\n"
        "  if(!gl){C.outerHTML='<p style=color:#f00>WebGL2 req</p>';return;}\n"
        "  function sh(t,s){const x=gl.createShader(t);"
        "gl.shaderSource(x,s);gl.compileShader(x);"
        "if(!gl.getShaderParameter(x,gl.COMPILE_STATUS))"
        "console.error(gl.getShaderInfoLog(x));return x;}\n"
        "  const VS=`%s`;\n"
        "  const FS=`%s`;\n"
        "  const p=gl.createProgram();\n"
        "  gl.attachShader(p,sh(gl.VERTEX_SHADER,VS));\n"
        "  gl.attachShader(p,sh(gl.FRAGMENT_SHADER,FS));\n"
        "  gl.linkProgram(p); gl.useProgram(p);\n"
        "  const vbo=gl.createBuffer();\n"
        "  gl.bindBuffer(gl.ARRAY_BUFFER,vbo);\n"
        "  gl.bufferData(gl.ARRAY_BUFFER,"
        "new Float32Array([-1,-1,1,-1,-1,1,-1,1,1,-1,1,1]),gl.STATIC_DRAW);\n"
        "  const aPos=gl.getAttribLocation(p,'a_pos');\n"
        "  gl.enableVertexAttribArray(aPos);\n"
        "  gl.vertexAttribPointer(aPos,2,gl.FLOAT,false,0,0);\n"
        "  const uT=gl.getUniformLocation(p,'u_time');\n"
        "  const uR=gl.getUniformLocation(p,'u_roughness');\n"
        "  const uM=gl.getUniformLocation(p,'u_metallic');\n"
        "  const uS=gl.getUniformLocation(p,'u_scale');\n"
        "  gl.uniform1f(uR,%.2f); gl.uniform1f(uM,%.2f);\n"
        "  gl.uniform1f(uS,%.2f);\n"
        "  let t0=performance.now();\n"
        "  (function loop(){\n"
        "    gl.uniform1f(uT,(performance.now()-t0)/1000);\n"
        "    gl.drawArrays(gl.TRIANGLES,0,6);\n"
        "    requestAnimationFrame(loop);\n"
        "  })();\n"
        "})();\n"
        "</script>\n"
        "</body>\n</html>\n",
        rigart_texture_name__rig_dup_64c145a7(ctx->type),
        rigart_texture_name__rig_dup_64c145a7(ctx->type),
        glsl_r.glsl_vert ? glsl_r.glsl_vert : PBR_VERT,
        glsl_r.glsl_frag ? glsl_r.glsl_frag : "void main(){fragColor=vec4(1.0);}",
        ctx->roughness > 0 ? ctx->roughness : 0.5f,
        ctx->metallic > 0 ? ctx->metallic : 0.8f,
        ctx->scale > 0 ? ctx->scale : 1.0f);

    rset_html__rig_variant_ee82cccd(out, &h);
    rigart_free_result_v3(&glsl_r);
    return 0;
}

const char *rigart_micro_name__rig_dup_b0a22288(RIgArtMicro m)
{
    static const char *n[] = {
        "press","ripple","success","error","hover","focus","drag","long-press"
    };
    return (m < RIGART_MICRO_COUNT) ? n[m] : "unknown";
}

int rigart_micro_css__rig_variant_64234132(const RIgArtMicroCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    memset(out, 0, sizeof(*out));

    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);
    const char *sel = ctx->selector[0] ? ctx->selector : ".rg-micro";
    float dur = ctx->duration_s > 0 ? ctx->duration_s : 0.2f;
    uint32_t rc = ctx->ripple_color ? ctx->ripple_color : 0xFFD700;
    uint32_t gc = ctx->glow_color   ? ctx->glow_color   : 0xFFD700;

    bc_printf__rig_variant_f2482fac(&css, "/* RIgArt v3.0 — Micro %s */\n@layer rg-micro {\n",
              rigart_micro_name__rig_dup_b0a22288(ctx->type));

    switch (ctx->type) {
        case RIGART_MICRO_PRESS:
            bc_printf__rig_variant_f2482fac(&css,
                "@keyframes rg-press {\n"
                "  0%%,100%% { transform:scale(1); }\n"
                "  50%%      { transform:scale(%.3f); }\n"
                "}\n"
                "%s:active { animation:rg-press %.3fs ease; }\n",
                ctx->scale_factor > 0 ? ctx->scale_factor : 0.94f,
                sel, dur);
            break;

        case RIGART_MICRO_SUCCESS:
            bc_printf__rig_variant_f2482fac(&css,
                "@keyframes rg-success {\n"
                "  0%%   { box-shadow:0 0 0 0 rgba(%d,%d,%d,0.7); }\n"
                "  70%%  { box-shadow:0 0 0 %.0fpx rgba(%d,%d,%d,0); }\n"
                "  100%% { box-shadow:0 0 0 0 rgba(%d,%d,%d,0); }\n"
                "}\n"
                ".rg-success { animation:rg-success %.3fs ease-out forwards; }\n",
                (gc>>16)&0xFF,(gc>>8)&0xFF,gc&0xFF,
                ctx->glow_radius_px > 0 ? ctx->glow_radius_px : 20.0f,
                (gc>>16)&0xFF,(gc>>8)&0xFF,gc&0xFF,
                (gc>>16)&0xFF,(gc>>8)&0xFF,gc&0xFF, dur * 2.0f);
            break;

        case RIGART_MICRO_ERROR:
            bc_printf__rig_variant_f2482fac(&css,
                "@keyframes rg-shake {\n"
                "  0%%,100%% { transform:translateX(0); }\n"
                "  20%%,60%% { transform:translateX(-5px); }\n"
                "  40%%,80%% { transform:translateX(5px); }\n"
                "}\n"
                ".rg-error { animation:rg-shake %.3fs ease-in-out; }\n", dur);
            break;

        case RIGART_MICRO_HOVER:
            bc_printf__rig_variant_f2482fac(&css,
                "%s {\n"
                "  transition:transform %.3fs cubic-bezier(0.34,1.56,0.64,1),\n"
                "             box-shadow %.3fs ease;\n"
                "}\n"
                "%s:hover {\n"
                "  transform:translateY(-%.1fpx);\n"
                "  box-shadow:0 %.0fpx %.0fpx rgba(%d,%d,%d,0.35);\n"
                "}\n",
                sel, dur, dur, sel,
                ctx->translate_y_px > 0 ? ctx->translate_y_px : 3.0f,
                ctx->glow_radius_px > 0 ? ctx->glow_radius_px * 0.5f : 10.0f,
                ctx->glow_radius_px > 0 ? ctx->glow_radius_px : 20.0f,
                (gc>>16)&0xFF,(gc>>8)&0xFF,gc&0xFF);
            break;

        case RIGART_MICRO_FOCUS:
            bc_printf__rig_variant_f2482fac(&css,
                "%s:focus-visible {\n"
                "  outline:none;\n"
                "  box-shadow:0 0 0 3px rgba(%d,%d,%d,0.6),\n"
                "             0 0 0 6px rgba(%d,%d,%d,0.2);\n"
                "  transition:box-shadow %.3fs ease;\n"
                "}\n",
                sel,
                (gc>>16)&0xFF,(gc>>8)&0xFF,gc&0xFF,
                (gc>>16)&0xFF,(gc>>8)&0xFF,gc&0xFF, dur);
            break;

        case RIGART_MICRO_DRAG:
            bc_printf__rig_variant_f2482fac(&css,
                "%s.rg-dragging {\n"
                "  opacity:0.85;\n"
                "  transform:scale(1.04) rotate(1.5deg);\n"
                "  box-shadow:0 20px 40px rgba(0,0,0,0.4),\n"
                "             0 0 20px rgba(%d,%d,%d,0.2);\n"
                "  cursor:grabbing;\n"
                "  transition:transform 0.1s ease,box-shadow 0.1s ease;\n"
                "}\n",
                sel, (gc>>16)&0xFF,(gc>>8)&0xFF,gc&0xFF);
            break;

        case RIGART_MICRO_LONG:

            bc_printf__rig_variant_f2482fac(&css,
                ".rg-long-ring {\n"
                "  position:absolute;\n"
                "  inset:-4px;\n"
                "  border-radius:50%%;\n"
                "  border:2px solid rgba(%d,%d,%d,0.6);\n"
                "  clip-path:polygon(50%% 0%%,50%% 50%%,100%% 0%%,100%% 100%%,0%% 100%%,0%% 0%%);\n"
                "  animation:rg-long-fill %.3fs linear forwards;\n"
                "}\n"
                "@keyframes rg-long-fill {\n"
                "  from { stroke-dashoffset:100; }\n"
                "  to   { stroke-dashoffset:0; }\n"
                "}\n",
                (rc>>16)&0xFF,(rc>>8)&0xFF,rc&0xFF, dur);
            break;

        case RIGART_MICRO_RIPPLE:
        default:

            bc_printf__rig_variant_f2482fac(&css,
                "@keyframes rg-micro-ripple { to { transform:scale(2.5);opacity:0; } }\n"
                "%s { position:relative; overflow:hidden; }\n"
                "%s .rg-ripple-spot {\n"
                "  position:absolute;\n"
                "  border-radius:50%%;\n"
                "  background:rgba(%d,%d,%d,0.35);\n"
                "  transform:scale(0);\n"
                "  animation:rg-micro-ripple %.3fs ease-out forwards;\n"
                "  pointer-events:none;\n"
                "}\n",
                sel, sel,
                (rc>>16)&0xFF,(rc>>8)&0xFF,rc&0xFF, dur * 3.0f);
            break;
    }

    bc_cat__rig_variant_99ba61aa(&css, "}\n");
    rset_css__rig_variant_2e374284(out, &css);
    return 0;
}

int rigart_micro_scrollbar__rig_dup_80671fc7(RIgArtMaterial mat, float width_px,
                             RIgArtResultV3 *out)
{
    (void)width_px;
    return rigart_scrollbar_luxury(mat, out);
}

int rigart_micro_system__rig_variant_7ce24c84(RIgArtMaterial mat, bool dark_mode, RIgArtResultV3 *out)
{
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    Buf3c css = bc_new__rig_variant_3681e398(RIGART_MAX_CSS_V3);

    const char *bg   = dark_mode ? "#04040c" : "#f5f5f0";
    const char *text = dark_mode ? "rgba(245,232,192,0.9)" : "rgba(20,16,8,0.9)";

    bc_printf__rig_variant_f2482fac(&css,
        "/* RIgArt v3.0 — Micro System · %s · %s */\n"
        "@layer rg-micro-system {\n"
        ":root {\n"
        "  --rg-bg: %s;\n"
        "  --rg-text: %s;\n"
        "  --rg-mat: %s;\n"
        "  color-scheme: %s;\n"
        "}\n"
        "/* Press universal */\n"
        "*:active { transform:scale(0.97); }\n"
        "/* Focus universal */\n"
        "*:focus-visible { outline:2px solid rgba(255,215,0,0.7); outline-offset:3px; }\n"
        "/* Hover universal */\n"
        "button,a,[role=button] { transition:transform 0.15s ease,box-shadow 0.15s ease; }\n"
        "button:hover,a:hover,[role=button]:hover { transform:translateY(-2px); }\n"
        "}\n",
        rigart_material_name__rig_dup_66ec8909(mat),
        dark_mode ? "dark" : "light",
        bg, text,
        rigart_material_name__rig_dup_66ec8909(mat),
        dark_mode ? "dark" : "light");

    rset_css__rig_variant_2e374284(out, &css);
    return 0;
}

int rigart_micro_long_press_js__rig_variant_4fe02caf(const char *selector,
                                float duration_s, uint32_t color,
                                RIgArtResultV3 *out)
{
    if (!selector || !out) return -1;
    memset(out, 0, sizeof(*out));
    Buf3c js = bc_new__rig_variant_3681e398(RIGART_MAX_JS);

    bc_printf__rig_variant_f2482fac(&js,
        "(function rigLongPress(){\n"
        "  const els=document.querySelectorAll('%s');\n"
        "  const DUR=%.0f;\n"
        "  els.forEach(el=>{\n"
        "    let timer=null,start=0;\n"
        "    el.style.position='relative';\n"
        "    el.addEventListener('pointerdown',e=>{\n"
        "      start=Date.now();\n"
        "      const ring=document.createElement('div');\n"
        "      ring.className='rg-long-ring';\n"
        "      ring.style.cssText=[\n"
        "        'position:absolute','inset:-4px','border-radius:50%%',\n"
        "        `border:2px solid rgba(%d,%d,%d,0.7)`,\n"
        "        `animation:rg-long-fill ${DUR}ms linear forwards`\n"
        "      ].join(';');\n"
        "      el.appendChild(ring);\n"
        "      timer=setTimeout(()=>{\n"
        "        el.dispatchEvent(new CustomEvent('longpress'));\n"
        "        ring.remove();\n"
        "      }, DUR);\n"
        "    });\n"
        "    const cancel=()=>{ if(timer){clearTimeout(timer);timer=null;}"
        "el.querySelector('.rg-long-ring')?.remove(); };\n"
        "    el.addEventListener('pointerup',cancel);\n"
        "    el.addEventListener('pointerleave',cancel);\n"
        "    el.addEventListener('pointercancel',cancel);\n"
        "  });\n"
        "  if(!document.getElementById('rg-long-style')){\n"
        "    const st=document.createElement('style');\n"
        "    st.id='rg-long-style';\n"
        "    st.textContent='@keyframes rg-long-fill{from{opacity:0}to{opacity:1}}';\n"
        "    document.head.appendChild(st);\n"
        "  }\n"
        "})();\n",
        selector,
        duration_s > 0 ? duration_s * 1000.0f : 800.0f,
        (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF);

    rset_js__rig_variant_54929c2f(out, &js);
    return 0;
}

void rigart_ws_handle_v3__rig_variant_61a30c4a(WsServer *srv, const char *cmd, const char *payload)
{
    if (!srv || !cmd) return 0;
    RIgArtResultV3 out = {0};
    int ret = -1;

    if (rl_strcmp(cmd, "rigart_typo") == 0) {
        RIgArtTypoCtx ctx = {0};
        ctx.style    = (RIgArtTypo)j3_float(payload, "\"style\"", 0);
        ctx.size_px  = j3_float(payload, "\"size_px\"", 32.0f);
        ctx.animate  = j3_bool(payload, "\"animate\"", true);
        ctx.hdr_glow = j3_bool(payload, "\"hdr_glow\"", false);
        j3_str(payload, "\"text\"", ctx.text, sizeof(ctx.text));
        j3_str(payload, "\"selector\"", ctx.selector, sizeof(ctx.selector));
        ret = rigart_typo_forge(&ctx, &out);
    }
    else if (rl_strcmp(cmd, "rigart_typo_fx") == 0) {
        RIgArtTextFx fx = (RIgArtTextFx)j3_float(payload, "\"fx\"", 0);
        uint32_t pri = (uint32_t)j3_float(payload, "\"primary\"", 0xFFD700);
        uint32_t sec = (uint32_t)j3_float(payload, "\"secondary\"", 0xB8860B);
        float dur    = j3_float(payload, "\"dur\"", 3.0f);
        ret = rigart_typo_text_fx(fx, pri, sec, dur, &out);
    }
    else if (rl_strcmp(cmd, "rigart_typo_path") == 0) {
        RIgArtTypoCtx ctx = {0};
        ctx.path_type = (RIgArtTextPath)j3_float(payload, "\"path\"", 0);
        ctx.animate   = true;
        j3_str(payload, "\"text\"", ctx.text, sizeof(ctx.text));
        ctx.size_px   = j3_float(payload, "\"size\"", 14.0f);
        float cw = j3_float(payload, "\"w\"", 400.0f);
        float ch = j3_float(payload, "\"h\"", 400.0f);
        ret = rigart_typo_text_path(&ctx, cw, ch, &out);
    }

    else if (rl_strcmp(cmd, "rigart_window") == 0) {
        RIgArtWindowCtx ctx = {0};
        ctx.type           = (RIgArtWindow)j3_float(payload, "\"type\"", 0);
        ctx.blur_px        = j3_float(payload, "\"blur\"", 40.0f);
        ctx.opacity        = j3_float(payload, "\"opacity\"", 0.85f);
        ctx.has_particles  = j3_bool(payload, "\"particles\"", false);
        j3_str(payload, "\"selector\"", ctx.selector, sizeof(ctx.selector));
        ret = rigart_window_forge(&ctx, &out);
    }
    else if (rl_strcmp(cmd, "rigart_scrollbar") == 0) {
        RIgArtMaterial mat = (RIgArtMaterial)j3_float(payload, "\"mat\"", 0);
        ret = rigart_scrollbar_luxury(mat, &out);
    }

    else if (rl_strcmp(cmd, "rigart_effect") == 0) {
        RIgArtEffectCtx ctx = {0};
        ctx.type     = (RIgArtEffect)j3_float(payload, "\"type\"", 0);
        ctx.strength = j3_float(payload, "\"strength\"", 0.5f);
        ctx.animate  = j3_bool(payload, "\"animate\"", true);
        j3_str(payload, "\"selector\"", ctx.selector, sizeof(ctx.selector));
        ret = rigart_effect_css(&ctx, &out);
    }

    else if (rl_strcmp(cmd, "rigart_transition") == 0) {
        RIgArtTransitionCtx ctx = {0};
        ctx.type       = (RIgArtTransition)j3_float(payload, "\"type\"", 0);
        ctx.duration_s = j3_float(payload, "\"dur\"", 0.6f);
        ctx.easing     = (RIgArtEasing)j3_float(payload, "\"ease\"", 1);
        ret = rigart_transition_css(&ctx, &out);
    }

    else if (rl_strcmp(cmd, "rigart_behavior") == 0) {
        RIgArtBehaviorCtx ctx = {0};
        ctx.type     = (RIgArtBehavior)j3_float(payload, "\"type\"", 0);
        ctx.strength = j3_float(payload, "\"strength\"", 0.5f);
        ctx.damping  = j3_float(payload, "\"damping\"", 0.15f);
        j3_str(payload, "\"selector\"", ctx.selector, sizeof(ctx.selector));
        ret = rigart_behavior_js(&ctx, &out);
    }

    else if (rl_strcmp(cmd, "rigart_color_p3") == 0) {
        float L = j3_float(payload, "\"L\"", 0.7f);
        float C = j3_float(payload, "\"C\"", 0.2f);
        float h = j3_float(payload, "\"h\"", 45.0f);
        char buf[64];
        rigart_color_p3_str(L, C, h, 1.0f, buf, sizeof(buf));

        char resp[128];
        rl_snprintf(resp, sizeof(resp), "{\"ok\":true,\"color\":\"%s\"}", buf);
        ws_broadcastf(srv, "%s", resp);
        rigart_free_result_v3(&out);
        return 0;
    }
    else if (rl_strcmp(cmd, "rigart_gradient_perc") == 0) {

        RIgArtOKLCh stops[2] = {
            {j3_float(payload,"\"L0\"",0.5f), j3_float(payload,"\"C0\"",0.2f),
             j3_float(payload,"\"h0\"",30.0f), 1.0f},
            {j3_float(payload,"\"L1\"",0.8f), j3_float(payload,"\"C1\"",0.15f),
             j3_float(payload,"\"h1\"",210.0f), 1.0f}
        };
        float angle = j3_float(payload, "\"angle\"", 135.0f);
        ret = rigart_gradient_perceptual(stops, 2, angle, false, &out);
    }

    else if (rl_strcmp(cmd, "rigart_texture") == 0) {
        RIgArtTextureCtx ctx = {0};
        ctx.type      = (RIgArtTexture)j3_float(payload, "\"type\"", 0);
        ctx.scale     = j3_float(payload, "\"scale\"", 1.0f);
        ctx.roughness = j3_float(payload, "\"roughness\"", 0.5f);
        ctx.metallic  = j3_float(payload, "\"metallic\"", 0.8f);
        ret = rigart_texture_css(&ctx, &out);
    }

    else if (rl_strcmp(cmd, "rigart_micro") == 0) {
        RIgArtMicroCtx ctx = {0};
        ctx.type       = (RIgArtMicro)j3_float(payload, "\"type\"", 0);
        ctx.duration_s = j3_float(payload, "\"dur\"", 0.2f);
        ctx.ripple_color = (uint32_t)j3_float(payload, "\"color\"", 0xFFD700);
        j3_str(payload, "\"selector\"", ctx.selector, sizeof(ctx.selector));
        ret = rigart_micro_css(&ctx, &out);
    }

    if (ret == 0 && out.ok) {
        char header[256];
        rl_snprintf(header, sizeof(header),
            "{\"ok\":true,\"cmd\":\"%s\",\"certeza\":%.4f,\"phi\":%.6f,\"data\":\"",
            cmd, out.certeza, out.phi_ratio);
        ws_broadcastf(srv, "%s", header);
        if (out.css)  ws_broadcastf(srv, "%s", out.css);
        if (out.js)   ws_broadcastf(srv, "%s", out.js);
        if (out.html) ws_broadcastf(srv, "%s", out.html);
        ws_broadcastf(srv, "%s", "\"}");
    } else {
        char err[256];
        rl_snprintf(err, sizeof(err),
            "{\"ok\":false,\"cmd\":\"%s\",\"error\":\"%s\"}",
            cmd, out.error[0] ? out.error : "error interno");
        ws_broadcastf(srv, "%s", err);
    }
    rigart_free_result_v3(&out);
    return 0;
}

const char *rigart_material_name__rig_dup_66ec8909(RIgArtMaterial m)
{

    static const char *n[] = {
        "obsidian",
        "gold-24k",
        "crystal",
        "carbon",
        "marble",
        "liquid-metal",
        "silk",
        "hologram",
        "ceramic-white",
        "pearl-lunar",
        "cobalt-deep",
        "amber-volcanic"
    };
    return (m < RIGART_MAT_COUNT) ? n[m] : "unknown";
}

const char *rigart_micro_scrollbar_name__rig_dup_f49a2e55(RIgArtMaterial mat)
{
    return rigart_material_name__rig_dup_66ec8909(mat);
}
