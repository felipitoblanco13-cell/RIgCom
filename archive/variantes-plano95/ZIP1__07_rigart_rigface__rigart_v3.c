#include "rigdeps/rig_std_base.h"
#include "../include/rigart_v3.h"
#include "../include/wsserver.h"

#include "rigdeps/stdio.h"
#include "rigdeps/stdlib.h"
#include "rigdeps/string.h"
#include "rigdeps/math.h"
#include "../include/riglib_math.h"
#include "rigdeps/stdarg.h"

#ifndef PHI3
#  define PHI3      1.6180339887498948482
#  define PHI3_INV  0.6180339887498948482
#  define PHI3_2    2.6180339887498948482
#  define SCHUMANN3 7.83
#  define TAU3      6.28318530717958647692
#  define PI3       3.14159265358979323846
#endif

typedef struct { char *buf; size_t pos; size_t cap; } Buf3;

static Buf3 b3_new(size_t cap)
{
    Buf3 b; b.buf = rl_calloc(1, cap); b.pos = 0; b.cap = cap; return b;
}
static int b3_cat(Buf3 *b, const char *s)
{
    if (!b->buf || !s) return 0;
    size_t n = rl_strlen(s);
    if (b->pos + n + 1 >= b->cap) return 0;
    rl_memcpy(b->buf + b->pos, s, n);
    b->pos += n;
    return 0;
}
static int b3_printf(Buf3 *b, const char *fmt, ...)
{
    if (!b->buf) return 0;
    char tmp[8192];
    va_list ap; va_start(ap, fmt);
    vrl_snprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    b3_cat(b, tmp);
    return 0;
}
static char *b3_done(Buf3 *b)
{
    if (!b->buf) return NULL;
    b->buf[b->pos] = '\0';
    return b->buf;
}
static int r3_set_css(RIgArtResultV3 *r, Buf3 *b)
{
    r->css      = b3_done(b);
    r->css_size = b->pos;
    r->ok       = (b->buf && b->pos > 0);
    r->certeza  = PHI3_INV;
    return 0;
}
static int r3_set_html(RIgArtResultV3 *r, Buf3 *b)
{
    r->html      = b3_done(b);
    r->html_size = b->pos;
    r->ok        = (b->buf && b->pos > 0);
    r->certeza   = PHI3_INV;
    return 0;
}
static int r3_set_js(RIgArtResultV3 *r, Buf3 *b)
{
    r->js      = b3_done(b);
    r->js_size = b->pos;
    r->ok      = (b->buf && b->pos > 0);
    r->certeza = PHI3_INV;
    return 0;
}
int rigart_free_result(RIgArtResult *r)
{
    if (!r) return 0;
    rl_free(r->html);   r->html = NULL;
    rl_free(r->css);    r->css  = NULL;
    rl_free(r->svg);    r->svg  = NULL;
    rl_free(r->js);     r->js   = NULL;
    rl_free(r->glsl_vert); r->glsl_vert = NULL;
    rl_free(r->glsl_frag); r->glsl_frag = NULL;
    return 0;
}

int rigart_free_result_v3(RIgArtResultV3 *r)
{
    if (!r) return 0;
    rl_free(r->html);      r->html = NULL;
    rl_free(r->css);       r->css  = NULL;
    rl_free(r->svg);       r->svg  = NULL;
    rl_free(r->js);        r->js   = NULL;
    rl_free(r->glsl_vert); r->glsl_vert = NULL;
    rl_free(r->glsl_frag); r->glsl_frag = NULL;
    return 0;
}

const char *rigart_typo_name(RIgArtTypo t)
{
    static const char *n[] = {
        "imperial","sovereign","cipher","rune",
        "ethereal","chroma","kinetic","magnetic"
    };
    return (t < RIGART_TYPO_COUNT) ? n[t] : "unknown";
}

const char *rigart_typo_font_stack(RIgArtTypo t)
{
    static const char *stacks[] = {
         "'Cormorant Garamond','IM Fell English',Georgia,serif",
         "'Cinzel Decorative','Cinzel','Playfair Display',serif",
         "'JetBrains Mono','Fira Code','Courier New',monospace",
         "'MedievalSharp','Uncial Antiqua',fantasy,serif",
         "'Raleway','Gill Sans','Century Gothic',sans-serif",
         "'Bebas Neue','Impact','Anton',sans-serif",
         "'Oswald','Barlow Condensed','Arial Narrow',sans-serif",
         "'Cinzel','Trajan Pro','Times New Roman',serif",
    };
    return (t < RIGART_TYPO_COUNT) ? stacks[t] : "'serif'";
}

int rigart_typo_forge(const RIgArtTypoCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    rl_memset(out, 0, sizeof(*out));

    Buf3 css = b3_new(RIGART_MAX_CSS_V3);
    const char *sel  = ctx->selector[0] ? ctx->selector : ".rg-typo";
    const char *font = rigart_typo_font_stack(ctx->style);
    float sz   = ctx->size_px    > 0 ? ctx->size_px    : 32.0f;
    float szmi = ctx->size_min_px > 0 ? ctx->size_min_px : sz * 0.5f;
    float szma = ctx->size_max_px > 0 ? ctx->size_max_px : sz * PHI3;
    float wght = ctx->weight     > 0 ? ctx->weight     : 700.0f;
    float lh   = ctx->line_height > 0 ? ctx->line_height : (float)PHI3_INV + 1.0f;
    float dur  = ctx->anim_dur_s  > 0 ? ctx->anim_dur_s  : 3.0f;

    uint32_t cp = ctx->color_primary   ? ctx->color_primary   : 0xF2D680;
    uint32_t cs = ctx->color_secondary ? ctx->color_secondary : 0xB8861A;
    uint32_t cg = ctx->color_glow      ? ctx->color_glow      : 0xE8B820;

    b3_printf(&css,
        "/* RIgArt v3.0 — Typography · %s · φ=1.618 */\n"
        "/* phi_font_3d: tipografía soberana φ — sin Google Fonts */\n"
        "/* WS cmd: phi_font_3d → pf3_ws_dispatch() → SVG Bézier nativo */\n"
        "@layer rg-typo {\n\n",
        rigart_typo_name(ctx->style));

    if (ctx->responsive) {
        b3_printf(&css,
            "%s {\n"
            "  font-family: %s;\n"
            "  font-size: clamp(%.1fpx, %.4fvw, %.1fpx);\n",
            sel, font, szmi, sz * 100.0f / 1440.0f, szma);
    } else {
        b3_printf(&css,
            "%s {\n"
            "  font-family: %s;\n"
            "  font-size: %.1fpx;\n",
            sel, font, sz);
    }

    b3_printf(&css,
        "  font-weight: %.0f;\n"
        "  line-height: %.4f;\n"
        "  letter-spacing: %.4fem;\n"
        "  %s\n",
        wght, lh,
        ctx->tracking > 0 ? ctx->tracking : 0.04f,
        ctx->uppercase ? "text-transform: uppercase;\n" : "");

    b3_printf(&css,
        "  background: linear-gradient(160deg, #%06X 0%%, #%06X 50%%, #%06X 100%%);\n"
        "  -webkit-background-clip: text;\n"
        "  background-clip: text;\n"
        "  -webkit-text-fill-color: transparent;\n",
        cp & 0xFFFFFF, cs & 0xFFFFFF, (cp >> 1) & 0xFFFFFF);

    if (ctx->hdr_glow) {
        float gr = ctx->glow_radius_px > 0 ? ctx->glow_radius_px : 20.0f;
        b3_printf(&css,
            "  filter: drop-shadow(0 0 %.0fpx rgba(%d,%d,%d,0.8))\n"
            "          drop-shadow(0 0 %.0fpx rgba(%d,%d,%d,0.3));\n",
            gr * 0.4f, (cg>>16)&0xFF, (cg>>8)&0xFF, cg&0xFF,
            gr,        (cg>>16)&0xFF, (cg>>8)&0xFF, cg&0xFF);
    }

    if (ctx->variable_wght > 0 || ctx->variable_wdth > 0) {
        b3_printf(&css,
            "  font-variation-settings: 'wght' %.0f, 'wdth' %.0f, 'opsz' %.0f;\n",
            ctx->variable_wght > 0 ? ctx->variable_wght : wght,
            ctx->variable_wdth > 0 ? ctx->variable_wdth : 100.0f,
            ctx->variable_opsz > 0 ? ctx->variable_opsz : sz);
    }

    b3_cat(&css, "}\n");

    if (ctx->animate) {
        b3_printf(&css,
            "@keyframes rg-typo-shimmer-%s {\n"
            "  0%%   { background-position: -200%% center; }\n"
            "  100%% { background-position:  200%% center; }\n"
            "}\n"
            "%s {\n"
            "  background-size: 200%% auto;\n"
            "  animation: rg-typo-shimmer-%s %.2fs linear infinite;\n"
            "}\n",
            sel + 1, sel, sel + 1, dur);
    }

    b3_cat(&css, "}\n");
    r3_set_css(out, &css);
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

int rigart_typo_kinetic_js(const char *selector, float stagger_s,
                             const char *anim_name, RIgArtResultV3 *out)
{
    if (!selector || !out) return -1;
    rl_memset(out, 0, sizeof(*out));

    float stg = stagger_s > 0 ? stagger_s : 0.04f;
    const char *anim = anim_name && anim_name[0] ? anim_name : "rg-kinetic-char";

    Buf3 js = b3_new(RIGART_MAX_JS);
    b3_printf(&js,
        "/* RIgArt v3.0 — Kinetic Typography · %s */\n"
        "(function rigKinetic(){\n"
        "  const els=document.querySelectorAll('%s');\n"
        "  els.forEach(el=>{\n"
        "    if(el.dataset.rgKinetic) return;\n"
        "    el.dataset.rgKinetic='1';\n"
        "    const text=el.textContent;\n"
        "    el.innerHTML='';\n"
        "    [...text].forEach((ch,i)=>{\n"
        "      const span=document.createElement('span');\n"
        "      span.textContent=ch==' '?'\\u00A0':ch;\n"
        "      span.style.display='inline-block';\n"
        "      span.style.animationName='%s';\n"
        "      span.style.animationDuration='0.6s';\n"
        "      span.style.animationTimingFunction='cubic-bezier(0.618,0,0.382,1)';\n"
        "      span.style.animationFillMode='both';\n"
        "      span.style.animationDelay=(i*%.4f)+'s';\n"
        "      el.appendChild(span);\n"
        "    });\n"
        "  });\n"
        "})();\n",
        selector, selector, anim, stg);

    r3_set_js(out, &js);
    return 0;
}

int rigart_typo_text_path(const RIgArtTypoCtx *ctx,
                           float canvas_w, float canvas_h,
                           RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    rl_memset(out, 0, sizeof(*out));

    float W  = canvas_w  > 0 ? canvas_w  : 400.0f;
    float H  = canvas_h  > 0 ? canvas_h  : 400.0f;
    float cx = W * 0.5f,  cy = H * 0.5f;
    float R  = ctx->path_radius > 0 ? ctx->path_radius : (W < H ? W : H) * 0.36f;
    float fs = ctx->size_px > 0 ? ctx->size_px : 14.0f;
    const char *text = ctx->text[0] ? ctx->text : "RIgArt v3.0";
    const char *font = rigart_typo_font_stack(ctx->style);
    float dur = ctx->anim_dur_s > 0 ? ctx->anim_dur_s : 8.0f;

    uint32_t cp = ctx->color_primary ? ctx->color_primary : 0xF2D680;
    uint32_t cs = ctx->color_secondary ? ctx->color_secondary : 0xB8861A;

    Buf3 svg = b3_new(RIGART_MAX_SVG_V3);

    b3_printf(&svg,
        "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        "width=\"%.0f\" height=\"%.0f\" viewBox=\"0 0 %.0f %.0f\">\n"
        "<defs>\n"
        "  <linearGradient id=\"rg-path-grad\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"0\">\n"
        "    <stop offset=\"0%%\" stop-color=\"#%06X\"/>\n"
        "    <stop offset=\"50%%\" stop-color=\"#fff8c0\"/>\n"
        "    <stop offset=\"100%%\" stop-color=\"#%06X\"/>\n"
        "  </linearGradient>\n",
        W, H, W, H,
        cp & 0xFFFFFF, cs & 0xFFFFFF);

    switch (ctx->path_type) {
        case RIGART_PATH_CIRCLE:
            b3_printf(&svg,
                "  <circle id=\"rg-txt-path\" cx=\"%.1f\" cy=\"%.1f\" r=\"%.1f\" fill=\"none\"/>\n",
                cx, cy, R);
            break;
        case RIGART_PATH_WAVE:
            b3_printf(&svg,
                "  <path id=\"rg-txt-path\" fill=\"none\" d=\"M %.1f,%.1f "
                "Q %.1f,%.1f %.1f,%.1f T %.1f,%.1f\"/>\n",
                cx - R, cy, cx - R*0.5f, cy - R*0.5f,
                cx, cy, cx + R, cy);
            break;
        case RIGART_PATH_ARCH:
            b3_printf(&svg,
                "  <path id=\"rg-txt-path\" fill=\"none\" "
                "d=\"M %.1f,%.1f A %.1f,%.1f 0 0,1 %.1f,%.1f\"/>\n",
                cx - R, cy, R, R, cx + R, cy);
            break;
        case RIGART_PATH_LEMNISCATE: {

            b3_printf(&svg, "  <path id=\"rg-txt-path\" fill=\"none\" d=\"M");
            int steps = 120;
            for (int i = 0; i <= steps; i++) {
                double t    = TAU3 * i / steps;
                double s2   = sin(t) * sin(t);
                double denom = 1.0 + s2;
                float  x    = cx + (float)(R * cos(t) / denom);
                float  y    = cy + (float)(R * cos(t) * sin(t) / denom);
                b3_printf(&svg, i ? " L%.2f,%.2f" : "%.2f,%.2f", x, y);
            }
            b3_cat(&svg, " Z\"/>\n");
            break;
        }
        default:
            b3_printf(&svg, "  <path id=\"rg-txt-path\" fill=\"none\" d=\"M");
            for (int i = 0; i <= 360; i++) {
                double angle = i * PI3 / 180.0;
                double r2    = R * exp(0.15 * angle / TAU3);
                float  x     = cx + (float)(r2 * cos(angle));
                float  y     = cy + (float)(r2 * sin(angle));
                b3_printf(&svg, i ? " L%.2f,%.2f" : "%.2f,%.2f", x, y);
            }
            b3_cat(&svg, "\"/>\n");
            break;
    }

    b3_printf(&svg,
        "</defs>\n"
        "<text font-family=\"%s\" font-size=\"%.1f\" fill=\"url(#rg-path-grad)\">\n"
        "  <textPath href=\"#rg-txt-path\" startOffset=\"0%%\">%s\n"
        "    <animate attributeName=\"startOffset\"\n"
        "      from=\"0%%\" to=\"100%%\"\n"
        "      dur=\"%.1fs\" repeatCount=\"indefinite\"/>\n"
        "  </textPath>\n"
        "</text>\n"
        "</svg>\n",
        font, fs, text, dur);

    out->svg      = b3_done(&svg);
    out->svg_size = svg.pos;
    out->ok       = true;
    out->certeza  = PHI3_INV;
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

const char *rigart_window_name(RIgArtWindow w)
{
    static const char *n[] = {
        "glass-vault","obsidian-card","hologram-frame","sovereign-modal",
        "aurora-panel","crystal-drawer","forge-container","phi-grid-layout"
    };
    return (w < RIGART_WIN_COUNT) ? n[w] : "unknown";
}

int rigart_scrollbar_luxury(RIgArtMaterial mat, RIgArtResultV3 *out)
{
    if (!out) return -1;
    rl_memset(out, 0, sizeof(*out));

    const char *mat_name = rigart_material_name(mat);
    Buf3 css = b3_new(RIGART_MAX_CSS_V3);

    const char *track_color, *thumb_color, *thumb_hover, *thumb_glow;
    switch (mat) {
        case RIGART_MAT_GOLD_24K:
            track_color = "rgba(26,18,0,0.6)";
            thumb_color = "linear-gradient(180deg,#c8a45a,#7a5510)";
            thumb_hover = "linear-gradient(180deg,#f8d050,#c8900c)";
            thumb_glow  = "rgba(232,184,32,0.4)";
            break;
        case RIGART_MAT_OBSIDIAN:
            track_color = "rgba(4,4,8,0.8)";
            thumb_color = "linear-gradient(180deg,#3a3a4a,#1a1a28)";
            thumb_hover = "linear-gradient(180deg,#6a6a8a,#3a3a5a)";
            thumb_glow  = "rgba(100,100,180,0.3)";
            break;
        case RIGART_MAT_CRYSTAL:
            track_color = "rgba(200,220,255,0.1)";
            thumb_color = "linear-gradient(180deg,rgba(200,220,255,0.6),rgba(150,180,255,0.4))";
            thumb_hover = "linear-gradient(180deg,rgba(220,235,255,0.9),rgba(180,200,255,0.7))";
            thumb_glow  = "rgba(150,200,255,0.4)";
            break;
        case RIGART_MAT_HOLOGRAM:
            track_color = "rgba(10,0,20,0.7)";
            thumb_color = "linear-gradient(180deg,#8800ff,#4400aa)";
            thumb_hover = "linear-gradient(180deg,#bb44ff,#7722dd)";
            thumb_glow  = "rgba(136,0,255,0.5)";
            break;
        default:
            track_color = "rgba(10,8,2,0.6)";
            thumb_color = "linear-gradient(180deg,#c8a45a,#7a5510)";
            thumb_hover = "linear-gradient(180deg,#f8d050,#c8900c)";
            thumb_glow  = "rgba(200,164,90,0.35)";
            break;
    }

    b3_printf(&css,
        "/* RIgArt v3.0 — Scrollbar Luxury · %s · φ=1.618 */\n"
        "@layer rg-scrollbar {\n"
        "/* Chrome / Edge / Safari */\n"
        "::-webkit-scrollbar {\n"
        "  width: 6px;\n"
        "  height: 6px;\n"
        "}\n"
        "::-webkit-scrollbar-track {\n"
        "  background: %s;\n"
        "  border-radius: 3px;\n"
        "}\n"
        "::-webkit-scrollbar-thumb {\n"
        "  background: %s;\n"
        "  border-radius: 3px;\n"
        "  transition: background 0.3s ease;\n"
        "}\n"
        "::-webkit-scrollbar-thumb:hover {\n"
        "  background: %s;\n"
        "  box-shadow: 0 0 8px %s;\n"
        "}\n"
        "::-webkit-scrollbar-corner {\n"
        "  background: transparent;\n"
        "}\n"
        "/* Firefox */\n"
        "* {\n"
        "  scrollbar-width: thin;\n"
        "  scrollbar-color: rgba(200,164,90,0.4) transparent;\n"
        "}\n"
        "}\n",
        mat_name,
        track_color, thumb_color, thumb_hover, thumb_glow);

    r3_set_css(out, &css);
    return 0;
}

int rigart_window_forge(const RIgArtWindowCtx *ctx, RIgArtResultV3 *out)
{
    if (!ctx || !out) return -1;
    rl_memset(out, 0, sizeof(*out));

    const char *sel = ctx->selector[0] ? ctx->selector : ".rg-window";
    float blur  = ctx->blur_px    > 0   ? ctx->blur_px    : 40.0f;
    float sat   = ctx->saturation > 0   ? ctx->saturation : 1.8f;
    float op    = ctx->opacity    > 0   ? ctx->opacity    : 0.85f;
    float br    = ctx->border_width_px > 0 ? ctx->border_width_px : 1.0f;
    float rad   = ctx->corner_radius_px > 0 ? ctx->corner_radius_px
                                              : 16.0f * (float)PHI3_INV;
    float elev  = ctx->elevation  > 0   ? ctx->elevation  : 2.0f;
    float anim  = ctx->border_anim_dur > 0 ? ctx->border_anim_dur : 3.0f;
    uint32_t bc = ctx->border_color ? ctx->border_color : 0xC8A45A;
    uint32_t ac = ctx->accent_color ? ctx->accent_color : 0xFFD700;
    uint32_t pc = ctx->particle_color ? ctx->particle_color : 0xFFD700;

    Buf3 css = b3_new(RIGART_MAX_CSS_V3);

    b3_printf(&css,
        "/* RIgArt v3.0 — Window Engine · %s · φ=1.618 */\n"
        "@layer rg-window {\n",
        rigart_window_name(ctx->type));

    switch (ctx->type) {
        case RIGART_WIN_GLASS_VAULT:
            b3_printf(&css,
                "%s {\n"
                "  background: rgba(8,6,2,%.2f);\n"
                "  backdrop-filter: blur(%.0fpx) saturate(%.1f) brightness(1.1);\n"
                "  -webkit-backdrop-filter: blur(%.0fpx) saturate(%.1f) brightness(1.1);\n"
                "  border: %.1fpx solid rgba(%d,%d,%d,0.35);\n"
                "  border-radius: %.1fpx;\n"
                "  box-shadow:\n"
                "    0 %.0fpx %.0fpx rgba(0,0,0,0.6),\n"
                "    inset 0 1px 0 rgba(%d,%d,%d,0.15);\n"
                "}\n",
                sel, op,
                blur, sat, blur, sat,
                br, (bc>>16)&0xFF, (bc>>8)&0xFF, bc&0xFF,
                rad,
                elev*8.0f, elev*32.0f,
                (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF);
            break;

        case RIGART_WIN_OBSIDIAN_CARD:
            b3_printf(&css,
                "%s {\n"
                "  background: linear-gradient(145deg,\n"
                "    rgba(18,14,6,0.95),\n"
                "    rgba(8,6,2,0.98));\n"
                "  border: %.1fpx solid rgba(%d,%d,%d,0.25);\n"
                "  border-radius: %.1fpx;\n"
                "  box-shadow:\n"
                "    %.0fpx %.0fpx %.0fpx rgba(0,0,0,0.7),\n"
                "    -%.0fpx -%.0fpx %.0fpx rgba(%d,%d,%d,0.06),\n"
                "    inset 0 1px 0 rgba(%d,%d,%d,0.1);\n"
                "}\n",
                sel,
                br, (bc>>16)&0xFF, (bc>>8)&0xFF, bc&0xFF,
                rad,
                elev*5.0f, elev*5.0f, elev*18.0f,
                elev*3.0f, elev*3.0f, elev*12.0f,
                (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF,
                (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF);
            break;

        case RIGART_WIN_HOLOGRAM_FRAME:
            b3_printf(&css,
                "@keyframes rg-holo-border {\n"
                "  0%%   { border-color: rgba(%d,%d,%d,0.6); }\n"
                "  33%%  { border-color: rgba(0,229,255,0.6); }\n"
                "  66%%  { border-color: rgba(180,0,255,0.6); }\n"
                "  100%% { border-color: rgba(%d,%d,%d,0.6); }\n"
                "}\n"
                "%s {\n"
                "  background: rgba(0,8,16,0.7);\n"
                "  backdrop-filter: blur(%.0fpx);\n"
                "  -webkit-backdrop-filter: blur(%.0fpx);\n"
                "  border: 1px solid rgba(%d,%d,%d,0.6);\n"
                "  border-radius: %.1fpx;\n"
                "  box-shadow: 0 0 24px rgba(0,229,255,0.15),\n"
                "              inset 0 0 40px rgba(0,229,255,0.04);\n"
                "  animation: rg-holo-border %.2fs linear infinite;\n"
                "}\n",
                (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF,
                (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF,
                sel,
                blur * 0.5f, blur * 0.5f,
                (bc>>16)&0xFF, (bc>>8)&0xFF, bc&0xFF,
                rad, anim);
            break;

        case RIGART_WIN_SOVEREIGN_MODAL:
            b3_printf(&css,
                "%s {\n"
                "  position: relative;\n"
                "  background: linear-gradient(160deg,rgba(20,14,4,0.96),rgba(8,5,0,0.99));\n"
                "  border: 1px solid rgba(%d,%d,%d,0.4);\n"
                "  border-radius: %.1fpx;\n"
                "  box-shadow:\n"
                "    0 0 0 1px rgba(%d,%d,%d,0.08),\n"
                "    0 %.0fpx %.0fpx rgba(0,0,0,0.8),\n"
                "    0 0 80px rgba(%d,%d,%d,0.1);\n"
                "  overflow: hidden;\n"
                "}\n",
                sel,
                (bc>>16)&0xFF, (bc>>8)&0xFF, bc&0xFF,
                rad,
                (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF,
                elev*12.0f, elev*48.0f,
                (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF);
            break;

        case RIGART_WIN_AURORA_PANEL:
            b3_printf(&css,
                "@keyframes rg-aurora-bg {\n"
                "  0%%   { background-position: 0%% 50%%; }\n"
                "  50%%  { background-position: 100%% 50%%; }\n"
                "  100%% { background-position: 0%% 50%%; }\n"
                "}\n"
                "%s {\n"
                "  background: linear-gradient(-45deg,\n"
                "    rgba(0,48,32,0.9), rgba(0,24,48,0.9),\n"
                "    rgba(32,0,64,0.9), rgba(0,32,64,0.9));\n"
                "  background-size: 400%% 400%%;\n"
                "  animation: rg-aurora-bg %.1fs ease infinite;\n"
                "  border: 1px solid rgba(0,229,150,0.25);\n"
                "  border-radius: %.1fpx;\n"
                "  backdrop-filter: blur(%.0fpx);\n"
                "  -webkit-backdrop-filter: blur(%.0fpx);\n"
                "}\n",
                sel, anim * 3.0f, rad, blur * 0.3f, blur * 0.3f);
            break;

        case RIGART_WIN_CRYSTAL_DRAWER:
            b3_printf(&css,
                "%s {\n"
                "  background: rgba(180,210,255,0.08);\n"
                "  backdrop-filter: blur(%.0fpx) saturate(%.1f);\n"
                "  -webkit-backdrop-filter: blur(%.0fpx) saturate(%.1f);\n"
                "  border: 1px solid rgba(200,220,255,0.25);\n"
                "  border-radius: %.1fpx;\n"
                "  box-shadow:\n"
                "    inset 0 1px 0 rgba(255,255,255,0.15),\n"
                "    0 %.0fpx %.0fpx rgba(0,0,0,0.5);\n"
                "}\n",
                sel, blur, sat * 1.2f, blur, sat * 1.2f,
                rad, elev*6.0f, elev*24.0f);
            break;

        default:
            b3_printf(&css,
                "%s {\n"
                "  background: rgba(8,6,2,%.2f);\n"
                "  border: %.1fpx solid rgba(%d,%d,%d,0.3);\n"
                "  border-radius: %.1fpx;\n"
                "  box-shadow: 0 0 %.0fpx rgba(%d,%d,%d,0.15);\n"
                "}\n",
                sel, op, br,
                (bc>>16)&0xFF, (bc>>8)&0xFF, bc&0xFF,
                rad, elev*16.0f,
                (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF);
            break;
    }

    if (ctx->title[0]) {
        b3_printf(&css,
            "%s .rg-win-title {\n"
            "  font-family: 'Cinzel Decorative','Palatino Linotype','Palatino',Georgia,serif;\n"
            "  font-size: clamp(10px, 2.5vw, 14px);\n"
            "  letter-spacing: 0.35em;\n"
            "  color: rgba(%d,%d,%d,0.85);\n"
            "  padding: 16px 20px 12px;\n"
            "  border-bottom: 1px solid rgba(%d,%d,%d,0.15);\n"
            "}\n",
            sel,
            (ac>>16)&0xFF, (ac>>8)&0xFF, ac&0xFF,
            (bc>>16)&0xFF, (bc>>8)&0xFF, bc&0xFF);
    }

    if (ctx->sensor_tilt) {
        float max_tilt = ctx->max_tilt_deg > 0 ? ctx->max_tilt_deg : 12.0f;
        b3_printf(&css,
            "%s {\n"
            "  transform-style: preserve-3d;\n"
            "  will-change: transform;\n"
            "  transition: transform 0.1s ease;\n"
            "}\n"
            "/* JS: tilt max %.1f deg via gyroscope */\n",
            sel, max_tilt);
    }

    if (ctx->has_particles) {
        b3_printf(&css,
            "%s .rg-win-particles {\n"
            "  position: absolute;\n"
            "  inset: 0;\n"
            "  pointer-events: none;\n"
            "  overflow: hidden;\n"
            "  border-radius: inherit;\n"
            "}\n"
            "/* Particle color: rgba(%d,%d,%d,0.4) */\n",
            sel,
            (pc>>16)&0xFF, (pc>>8)&0xFF, pc&0xFF);
    }

    b3_cat(&css, "}\n");
    r3_set_css(out, &css);
    out->phi_ratio = (float)PHI3_INV;
    return 0;
}

#ifndef WS_BROADCASTF_DEFINED
#define WS_BROADCASTF_DEFINED
__attribute__((weak)) void ws_broadcastf__rig_variant_36a95ffd(WsServer *srv, const char *fmt, ...)
{
    (void)srv;
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
#endif

const char *rigart_effect_name(RIgArtEffect e)
{
    static const char *n[] = {
        "chromatic-aberration",
        "lens-flare",
        "dof-bokeh",
        "film-grain",
        "scan-lines",
        "glitch-art",
        "god-rays",
        "caustics",
        "hologram-scan",
        "dust-motes",
        "vignette-hdr",
        "bloom-oled",
    };
    return (e < RIGART_EFFECT_COUNT) ? n[e] : "unknown";
}
