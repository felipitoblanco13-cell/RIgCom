/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#define _POSIX_C_SOURCE 200809L
/* ════════════════════════════════════════════════════════════════════════════
 * catedral_ui.c  —  Sistema de UI soberana sobre RigRenderLoop
 *
 * Rendering dual:
 *   GPU (GLES3) : quads con corner radius + sombra via shader
 *   WS JSON     : siempre disponible, para dashboard web
 *
 * Coordenadas:  0..1 normalizadas (0,0 = top-left, 1,1 = bottom-right)
 *               → shaders usan NDC [-1,1] via: ndc_x = x*2-1
 * ════════════════════════════════════════════════════════════════════════════ */
#include "../include/catedral_ui.h"
#include "../include/wsserver.h"
#include "../include/rig_gpu.h"

#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_math.h"
#include "rig_syscall.h"

/* ══════════════════════════════════════════════════════════════════════════
 * §0  Helpers internos
 * ══════════════════════════════════════════════════════════════════════════ */

static double _now_s(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}
static uint8_t _rgba_r(uint32_t c) { return (uint8_t)(c >> 24); }
static uint8_t _rgba_g(uint32_t c) { return (uint8_t)(c >> 16); }
static uint8_t _rgba_b(uint32_t c) { return (uint8_t)(c >>  8); }
static uint8_t _rgba_a(uint32_t c) { return (uint8_t)(c      ); }
/* Comprueba si el punto normalizado (tx,ty) cae dentro del widget */
static bool _hit(const CUIWidget *w, float tx, float ty) {
    const bool in_x = (tx >= w->x) && (tx <= (w->x + w->w));
    const bool in_y = (ty >= w->y) && (ty <= (w->y + w->h));
    return in_x && in_y;
}
/* Escribe JSON de color RGBA como "#RRGGBBAA" */
static void _color_json(char *buf, size_t sz, uint32_t c) {
    snprintf(buf, sz, "#%02X%02X%02X%02X",
             _rgba_r(c), _rgba_g(c), _rgba_b(c), _rgba_a(c));
}
/* Escapa string para JSON */
static void _json_esc(const char *src, char *dst, size_t dsz) {
    size_t i = 0;
    while (*src && i + 2 < dsz) {
        if (*src == '"')       { dst[i++] = '\\'; dst[i++] = '"'; }
        else if (*src == '\\') { dst[i++] = '\\'; dst[i++] = '\\'; }
        else if (*src == '\n') { dst[i++] = '\\'; dst[i++] = 'n'; }
        else                   { dst[i++] = *src; }
        src++;
    }
    dst[i] = '\0';
}
/* ══════════════════════════════════════════════════════════════════════════
 * §1  GLSL Shaders GLES3 (compilados en tiempo de ejecución)
 * ══════════════════════════════════════════════════════════════════════════ */

/* Vertex shader: quad normalizado → NDC */
static const char *k_vert_quad =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 a_pos;\n"                  /* atributo: [0,1]×[0,1] */
    "uniform vec4 u_rect;\n"            /* x,y,w,h normalizados */
    "out vec2 v_uv;\n"
    "void main() {\n"
    "  vec2 p = u_rect.xy + a_pos * u_rect.zw;\n"
    "  gl_Position = vec4(p.x*2.0-1.0, 1.0-p.y*2.0, 0.0, 1.0);\n"
    "  v_uv = a_pos;\n"
    "}\n";

/* Fragment shader: rect con radius y shadow */
static const char *k_frag_quad =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 v_uv;\n"
    "uniform vec4  u_rect;\n"
    "uniform vec4  u_color;\n"
    "uniform vec4  u_border_color;\n"
    "uniform float u_radius;\n"         /* en píxeles normalizados */
    "uniform float u_border;\n"
    "uniform vec2  u_resolution;\n"
    "out vec4 o_color;\n"
    "float rr_dist(vec2 p, vec2 half_sz, float r) {\n"
    "  vec2 q = abs(p) - half_sz + r;\n"
    "  return length(max(q,0.0)) + min(max(q.x,q.y),0.0) - r;\n"
    "}\n"
    "void main() {\n"
    "  vec2 px = v_uv * u_rect.zw * u_resolution;\n"
    "  vec2 half_sz = u_rect.zw * u_resolution * 0.5;\n"
    "  vec2 center  = half_sz;\n"
    "  float d      = rr_dist(px - center, half_sz - 0.5, u_radius);\n"
    "  float aa     = 1.0 - smoothstep(-0.5, 0.5, d);\n"
    "  float bd     = rr_dist(px - center, half_sz - u_border - 0.5, u_radius - u_border);\n"
    "  float ba     = 1.0 - smoothstep(-0.5, 0.5, bd);\n"
    "  vec4 col     = mix(u_color, u_border_color, clamp(ba*(1.0-aa*0.999), 0.0, 1.0));\n"
    "  o_color      = col * aa;\n"
    "}\n";

/* Fragment shader: barra de progreso */
static const char *k_frag_bar =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 v_uv;\n"
    "uniform vec4  u_color_bg;\n"
    "uniform vec4  u_color_fill;\n"
    "uniform float u_fill;\n"            /* 0..1 */
    "uniform float u_glow;\n"            /* 0..1 φ-pulsation */
    "uniform float u_radius;\n"
    "uniform vec4  u_rect;\n"
    "uniform vec2  u_resolution;\n"
    "out vec4 o_color;\n"
    "float rr_dist(vec2 p, vec2 hs, float r) {\n"
    "  vec2 q = abs(p) - hs + r;\n"
    "  return length(max(q,0.0)) + min(max(q.x,q.y),0.0) - r;\n"
    "}\n"
    "void main() {\n"
    "  vec2 px = v_uv * u_rect.zw * u_resolution;\n"
    "  vec2 half_sz = u_rect.zw * u_resolution * 0.5;\n"
    "  float d_outer = rr_dist(px - half_sz, half_sz - 0.5, u_radius);\n"
    "  float aa = 1.0 - smoothstep(-0.5, 0.5, d_outer);\n"
    "  float fill_x = u_fill * u_rect.z * u_resolution.x;\n"
    "  float d_fill  = rr_dist(px - vec2(fill_x*0.5, half_sz.y),\n"
    "                           vec2(fill_x*0.5 - 0.5, half_sz.y - 0.5), u_radius);\n"
    "  float fa = 1.0 - smoothstep(-0.5, 0.5, d_fill);\n"
    "  vec4 glow_col = u_color_fill + vec4(u_glow * 0.3);\n"
    "  vec4 col = mix(u_color_bg, glow_col, fa);\n"
    "  o_color  = col * aa;\n"
    "}\n";

/* ══════════════════════════════════════════════════════════════════════════
 * §2  Compilación de shaders GLES3
 * ══════════════════════════════════════════════════════════════════════════ */

#ifdef RIG_BACKEND_GLES

/* Incluimos la API GPU via rig_gpu.h  —  funciones disponibles: */
/* rig_gpu_compile_shader(), rig_gpu_link_program(), etc.        */

static uint32_t _compile_prog(const char *vert, const char *frag) {
    uint32_t vs = rig_gpu_compile_shader(RIG_SHADER_VERT, vert);
    uint32_t fs = rig_gpu_compile_shader(RIG_SHADER_FRAG, frag);
    if (!vs || !fs) return 0;
    uint32_t prog = rig_gpu_link_program(vs, fs);
    rig_gpu_delete_shader(vs);
    rig_gpu_delete_shader(fs);
    return prog;
}
static bool _gpu_init__rig_variant_77c19423(CatedralUI *ui) {
    ui->prog_quad = _compile_prog(k_vert_quad, k_frag_quad);
    ui->prog_bar  = _compile_prog(k_vert_quad, k_frag_bar);
    if (!ui->prog_quad || !ui->prog_bar) return false;

    /* Unit quad [0,1]×[0,1]: 2 triángulos */
    static const float verts[] = {
        0.0f,0.0f,  1.0f,0.0f,  0.0f,1.0f,
        1.0f,0.0f,  1.0f,1.0f,  0.0f,1.0f
    };
    ui->vao_quad = rig_gpu_create_vao();
    ui->vbo_quad = rig_gpu_create_vbo(verts, sizeof(verts));
    rig_gpu_vao_attrib(ui->vao_quad, ui->vbo_quad, 0, 2, 8, 0);
    return true;
}
static void _gpu_set_uniform_4f(uint32_t prog, const char *name,
                                  float a, float b, float c, float d) {
    int loc = rig_gpu_uniform_loc(prog, name);
    if (loc >= 0) rig_gpu_uniform_4f(loc, a, b, c, d);
}
static void _gpu_set_uniform_1f(uint32_t prog, const char *name, float v) {
    int loc = rig_gpu_uniform_loc(prog, name);
    if (loc >= 0) rig_gpu_uniform_1f(loc, v);
}
static void _gpu_set_uniform_2f(uint32_t prog, const char *name, float a, float b) {
    int loc = rig_gpu_uniform_loc(prog, name);
    if (loc >= 0) rig_gpu_uniform_2f(loc, a, b);
}
/* Convierte RGBA8 a vec4 float */
static void _rgba_f(uint32_t c, float *r, float *g, float *b, float *a) {
    *r = (float)_rgba_r(c) / 255.0f;
    *g = (float)_rgba_g(c) / 255.0f;
    *b = (float)_rgba_b(c) / 255.0f;
    *a = (float)_rgba_a(c) / 255.0f;
}
/* Dibuja un quad del widget */
static void _draw_quad__rig_variant_bc529760(CatedralUI *ui, const CUIWidget *w) {
    if (!ui->prog_quad) return;

    rig_gpu_use_program(ui->prog_quad);

    float sw = (float)ui->screen_w, sh = (float)ui->screen_h;
    _gpu_set_uniform_4f(ui->prog_quad, "u_rect",
                         w->x, w->y, w->w, w->h);
    _gpu_set_uniform_2f(ui->prog_quad, "u_resolution", sw, sh);
    _gpu_set_uniform_1f(ui->prog_quad, "u_radius",
                         w->radius / sw);          /* normalizado */
    _gpu_set_uniform_1f(ui->prog_quad, "u_border",
                         w->border_px / sw);

    float r,g,b,a;
    _rgba_f(w->bg_color,     &r,&g,&b,&a);
    _gpu_set_uniform_4f(ui->prog_quad, "u_color", r*a, g*a, b*a, a);
    _rgba_f(w->border_color, &r,&g,&b,&a);
    _gpu_set_uniform_4f(ui->prog_quad, "u_border_color", r*a, g*a, b*a, a);

    rig_gpu_bind_vao(ui->vao_quad);
    rig_gpu_draw_triangles(0, 6);
}
static void _draw_bar__rig_variant_bc529760(CatedralUI *ui, const CUIWidget *w) {
    if (!ui->prog_bar) return;
    rig_gpu_use_program(ui->prog_bar);

    float sw = (float)ui->screen_w, sh = (float)ui->screen_h;
    _gpu_set_uniform_4f(ui->prog_bar, "u_rect",
                         w->x, w->y, w->w, w->h);
    _gpu_set_uniform_2f(ui->prog_bar, "u_resolution", sw, sh);
    _gpu_set_uniform_1f(ui->prog_bar, "u_radius", w->radius / sw);

    float fill = (w->value_max > 0.0f)
                 ? (w->value / w->value_max) : 0.0f;
    if (fill < 0.0f) fill = 0.0f;
    if (fill > 1.0f) fill = 1.0f;
    _gpu_set_uniform_1f(ui->prog_bar, "u_fill", fill);

    float glow = w->animated
        ? (0.5f + 0.5f * sinf(ui->phi_t * CUI_PHI))
        : 0.0f;
    _gpu_set_uniform_1f(ui->prog_bar, "u_glow", glow);

    float r,g,b,a;
    _rgba_f(CUI_COLOR_PANEL, &r,&g,&b,&a);
    _gpu_set_uniform_4f(ui->prog_bar, "u_color_bg", r,g,b,a);
    _rgba_f(w->value_color, &r,&g,&b,&a);
    _gpu_set_uniform_4f(ui->prog_bar, "u_color_fill", r,g,b,a);

    rig_gpu_bind_vao(ui->vao_quad);
    rig_gpu_draw_triangles(0, 6);
}
#else   /* backend de presentación WebSocket sin GLES */

/* CPU fallback cuando no hay GLES3 disponible.
 * gpu_ready=false → catedral_ui_draw() usa path de software renderer. */
/* UI overlay sobre pipeline 3D soberano (RigRenderLoop + GLES3) */
static bool _gpu_init__rig_variant_77c19423(CatedralUI *ui) {
    if (!ui) return false;
    ui->gpu_ready = false;
    return (ui->rl != NULL);
}
static void _draw_quad__rig_variant_bc529760(CatedralUI *ui, const CUIWidget *w) {
    if (!ui || !w) return;
    catedral_ws_push_widget(ui, w);
}
static void _draw_bar__rig_variant_bc529760(CatedralUI *ui, const CUIWidget *w) {
    if (!ui || !w) return;
    catedral_ws_push_widget(ui, w);
}
#endif  /* RIG_BACKEND_GLES */

/* ══════════════════════════════════════════════════════════════════════════
 * §3  Widget pool
 * ══════════════════════════════════════════════════════════════════════════ */

static CUIWidget *_widget_alloc(CatedralUI *ui, CUIKind kind,
                                  const char *id) {
    if (ui->n_widgets >= CUI_MAX_WIDGETS) {
        fprintf(stderr, "[catedral] widget pool lleno\n");
        return NULL;
    }
    CUIWidget *w = &ui->pool[ui->n_widgets++];
    memset(w, 0, sizeof(*w));
    w->kind         = kind;
    w->visible      = true;
    w->enabled      = true;
    w->alpha        = 1.0f;
    w->value_max    = 1.0f;
    w->font_size    = CUI_FONT_MD;
    w->bg_color     = CUI_COLOR_PANEL;
    w->fg_color     = CUI_COLOR_TEXT;
    w->border_color = CUI_COLOR_BORDER;
    w->border_px    = 1.0f;
    w->radius       = CUI_RAD;
    w->shadow_radius= CUI_RAD * CUI_PHI;
    w->value_color  = CUI_COLOR_CYAN;
    if (id) snprintf(w->id, sizeof(w->id), "%s", id);
    return w;
}
static void _attach(CUIWidget *parent, CUIWidget *child) {
    if (!parent || !child) return;
    if (parent->n_children >= CUI_MAX_CHILDREN) return;
    child->parent = parent;
    parent->children[parent->n_children++] = child;
}
static void _add_root(CatedralUI *ui, CUIWidget *w) {
    if (ui->n_roots >= CUI_MAX_WIDGETS) return;
    ui->roots[ui->n_roots++] = w;
}
/* ══════════════════════════════════════════════════════════════════════════
 * §4  Constructores
 * ══════════════════════════════════════════════════════════════════════════ */

CUIWidget *catedral_panel_new(CatedralUI *ui, CUIWidget *parent,
                               const char *id,
                               float x, float y, float w, float h) {
    CUIWidget *wg = _widget_alloc(ui, CUI_PANEL, id);
    if (!wg) return NULL;
    wg->x = x; wg->y = y; wg->w = w; wg->h = h;
    wg->bg_color     = CUI_COLOR_PANEL;
    wg->border_color = CUI_COLOR_BORDER;
    wg->border_px    = 1.0f;
    wg->radius       = CUI_RAD * 2.0f;
    if (parent) _attach(parent, wg);
    else        _add_root(ui, wg);
    return wg;
}

CUIWidget *catedral_label_new(CatedralUI *ui, CUIWidget *parent,
                               const char *text, uint32_t color,
                               float x, float y, float font_size) {
    CUIWidget *wg = _widget_alloc(ui, CUI_LABEL, NULL);
    if (!wg) return NULL;
    snprintf(wg->text, sizeof(wg->text), "%s", text ? text : "");
    wg->x = x; wg->y = y;
    wg->w = 0.0f; wg->h = font_size / (float)ui->screen_h;
    wg->fg_color  = color;
    wg->font_size = font_size;
    wg->bg_color  = 0;                 /* transparente */
    wg->border_px = 0;
    if (parent) _attach(parent, wg);
    else        _add_root(ui, wg);
    return wg;
}

CUIWidget *catedral_button_new(CatedralUI *ui, CUIWidget *parent,
                                const char *id, const char *text,
                                float x, float y, float w, float h,
                                void (*cb)(CUIWidget*, void*), void *ud) {
    CUIWidget *wg = _widget_alloc(ui, CUI_BUTTON, id);
    if (!wg) return NULL;
    snprintf(wg->text, sizeof(wg->text), "%s", text ? text : "");
    wg->x = x; wg->y = y; wg->w = w; wg->h = h;
    wg->bg_color     = CUI_COLOR_DGRAY;
    wg->border_color = CUI_COLOR_GOLD;
    wg->border_px    = 1.5f;
    wg->radius       = CUI_RAD * 3.0f;
    wg->fg_color     = CUI_COLOR_GOLD;
    wg->on_click     = cb;
    wg->userdata     = ud;
    if (parent) _attach(parent, wg);
    else        _add_root(ui, wg);
    return wg;
}

CUIWidget *catedral_progress_new(CatedralUI *ui, CUIWidget *parent,
                                  const char *id,
                                  float x, float y, float w, float h,
                                  float max, uint32_t color) {
    CUIWidget *wg = _widget_alloc(ui, CUI_PROGRESS, id);
    if (!wg) return NULL;
    wg->x = x; wg->y = y; wg->w = w; wg->h = h;
    wg->value_max   = (max > 0.0f) ? max : 100.0f;
    wg->value_color = color;
    wg->bg_color    = CUI_COLOR_DGRAY;
    wg->border_color= CUI_COLOR_BORDER;
    wg->border_px   = 1.0f;
    wg->radius      = h / 2.0f * (float)ui->screen_h; /* pill */
    wg->animated    = true;
    if (parent) _attach(parent, wg);
    else        _add_root(ui, wg);
    return wg;
}

CUIWidget *catedral_metric_new(CatedralUI *ui, CUIWidget *parent,
                                const char *id,
                                float x, float y, float w, float h,
                                const char *label, float initial_val) {
    CUIWidget *wg = _widget_alloc(ui, CUI_METRIC, id);
    if (!wg) return NULL;
    wg->x = x; wg->y = y; wg->w = w; wg->h = h;
    snprintf(wg->subtext, sizeof(wg->subtext), "%s", label ? label : "");
    wg->value     = initial_val;
    wg->fg_color  = CUI_COLOR_GOLD;
    wg->font_size = CUI_FONT_XL;
    snprintf(wg->text, sizeof(wg->text), "%.1f", initial_val);
    if (parent) _attach(parent, wg);
    else        _add_root(ui, wg);
    return wg;
}

CUIWidget *catedral_gauge_new(CatedralUI *ui, CUIWidget *parent,
                               const char *id,
                               float cx, float cy, float r,
                               const char *label) {
    CUIWidget *wg = _widget_alloc(ui, CUI_GAUGE, id);
    if (!wg) return NULL;
    wg->x = cx - r; wg->y = cy - r;
    wg->w = r * 2.0f; wg->h = r * 2.0f;
    wg->value_max = 100.0f;
    wg->value_color = CUI_COLOR_CYAN;
    snprintf(wg->subtext, sizeof(wg->subtext), "%s", label ? label : "");
    if (parent) _attach(parent, wg);
    else        _add_root(ui, wg);
    return wg;
}

CUIWidget *catedral_sparkline_new(CatedralUI *ui, CUIWidget *parent,
                                   const char *id,
                                   float x, float y, float w, float h) {
    CUIWidget *wg = _widget_alloc(ui, CUI_SPARKLINE, id);
    if (!wg) return NULL;
    wg->x = x; wg->y = y; wg->w = w; wg->h = h;
    wg->value_color  = CUI_COLOR_CYAN;
    wg->bg_color     = 0x0A0A0FA0;
    wg->history_len  = 0;
    wg->history_head = 0;
    if (parent) _attach(parent, wg);
    else        _add_root(ui, wg);
    return wg;
}

CUIWidget *catedral_badge_new(CatedralUI *ui, CUIWidget *parent,
                               const char *text, uint32_t color,
                               float x, float y) {
    float font = CUI_FONT_SM;
    float tw   = strlen(text ? text : "") * font * 0.6f / (float)ui->screen_w;
    float pad  = CUI_GAP / (float)ui->screen_w;

    CUIWidget *wg = _widget_alloc(ui, CUI_BADGE, NULL);
    if (!wg) return NULL;
    snprintf(wg->text, sizeof(wg->text), "%s", text ? text : "");
    wg->x = x; wg->y = y;
    wg->w = tw + pad * 2.0f;
    wg->h = (font + CUI_GAP) / (float)ui->screen_h;
    wg->bg_color     = (color & 0xFFFFFF00) | 0x40; /* semitransparente */
    wg->border_color = color;
    wg->border_px    = 1.0f;
    wg->fg_color     = color;
    wg->font_size    = font;
    wg->radius       = wg->h * (float)ui->screen_h * 0.5f; /* pill */
    if (parent) _attach(parent, wg);
    else        _add_root(ui, wg);
    return wg;
}

CUIWidget *catedral_separator_new(CatedralUI *ui, CUIWidget *parent,
                                   float y) {
    CUIWidget *wg = _widget_alloc(ui, CUI_SEPARATOR, NULL);
    if (!wg) return NULL;
    wg->x = CUI_GAP / (float)ui->screen_w;
    wg->y = y;
    wg->w = 1.0f - 2.0f * CUI_GAP / (float)ui->screen_w;
    wg->h = 1.0f / (float)ui->screen_h;
    wg->bg_color  = CUI_COLOR_BORDER;
    wg->border_px = 0;
    if (parent) _attach(parent, wg);
    else        _add_root(ui, wg);
    return wg;
}

/* ══════════════════════════════════════════════════════════════════════════
 * §5  Mutadores en tiempo real
 * ══════════════════════════════════════════════════════════════════════════ */

void catedral_set_value(CUIWidget *w, float val) {
    if (!w) return;
    w->value = val;
    if (w->kind == CUI_METRIC)
        snprintf(w->text, sizeof(w->text), "%.1f", val);
}

void catedral_set_text(CUIWidget *w, const char *text) {
    if (!w || !text) return;
    snprintf(w->text, sizeof(w->text), "%s", text);
}

void catedral_set_color(CUIWidget *w, uint32_t color) {
    if (!w) return;
    w->value_color = color;
    w->border_color = color;
}

void catedral_set_badge(CUIWidget *w, const char *text, uint32_t color) {
    if (!w) return;
    snprintf(w->text, sizeof(w->text), "%s", text);
    w->bg_color     = (color & 0xFFFFFF00) | 0x40;
    w->border_color = color;
    w->fg_color     = color;
    w->value_color  = color;
}

void catedral_push_hist(CUIWidget *w, float val) {
    if (!w) return;
    w->history[w->history_head % 64] = val;
    w->history_head++;
    if (w->history_len < 64) w->history_len++;
    w->value = val;
}

void catedral_animate(CUIWidget *w, bool enable) {
    if (!w) return;
    w->animated = enable;
}

CUIWidget *catedral_find(CatedralUI *ui, const char *id) {
    if (!id) return NULL;
    for (int i = 0; i < ui->n_widgets; i++) {
        if (strcmp(ui->pool[i].id, id) == 0) return &ui->pool[i];
    }
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════════
 * §6  Render GPU: árbol recursivo
 * ══════════════════════════════════════════════════════════════════════════ */

static void _draw_sparkline(CatedralUI *ui, const CUIWidget *w);

static void _render_widget(CatedralUI *ui, CUIWidget *w) {
    if (!w || !w->visible) return;

    switch (w->kind) {
        case CUI_PANEL:
        case CUI_BUTTON:
        case CUI_BADGE:
        case CUI_SEPARATOR:
            _draw_quad__rig_variant_bc529760(ui, w);
            break;
        case CUI_PROGRESS:
            _draw_bar__rig_variant_bc529760(ui, w);
            break;
        case CUI_METRIC:
        case CUI_GAUGE:
            _draw_quad__rig_variant_bc529760(ui, w);
            /* Text rendering via phi_font_3d se añade aquí si disponible */
            break;
        case CUI_SPARKLINE:
            _draw_sparkline(ui, w);  /* impl: §IMPLEMENTACIONES PENDIENTES */
            break;
        case CUI_LABEL:
            /* Solo texto — sin fondo */
            break;
        default: break;
    }

    /* Hover state: tint de borde */
    if (w->hovered && w->kind == CUI_BUTTON) {
        CUIWidget tmp = *w;
        tmp.border_color = CUI_COLOR_GOLD;
        tmp.border_px    = 2.0f;
        tmp.bg_color     = CUI_COLOR_HOVER;
        _draw_quad__rig_variant_bc529760(ui, &tmp);
    }

    /* Recursión en hijos */
    for (int i = 0; i < w->n_children; i++)
        _render_widget(ui, w->children[i]);
}
/* ══════════════════════════════════════════════════════════════════════════
 * §7  Input routing
 * ══════════════════════════════════════════════════════════════════════════ */

static void _process_input_widget(CatedralUI *ui, CUIWidget *w) {
    if (!w || !w->visible || !w->enabled) return;

    bool hit = _hit(w, ui->touch_x, ui->touch_y);

    /* Hover */
    bool was_hovered = w->hovered;
    w->hovered = hit;
    if (hit && !was_hovered && w->on_hover)
        w->on_hover(w, w->userdata);

    /* Click: flanco descendente del touch sobre este widget */
    if (w->kind == CUI_BUTTON || w->on_click) {
        if (hit && ui->touch_prev_down && !ui->touch_down && w->on_click)
            w->on_click(w, w->userdata);
        w->pressed = hit && ui->touch_down;
    }

    for (int i = 0; i < w->n_children; i++)
        _process_input_widget(ui, w->children[i]);
}
/* ══════════════════════════════════════════════════════════════════════════
 * §8  WS broadcast
 * ══════════════════════════════════════════════════════════════════════════ */

void catedral_ws_push_widget(CatedralUI *ui, const CUIWidget *w) {
    if (!ui->ws || !w) return;

    char esc_text[CUI_MAX_TEXT * 2];
    char esc_sub[CUI_MAX_TEXT * 2];
    char fg_str[12], vc_str[12];

    _json_esc(w->text,    esc_text, sizeof(esc_text));
    _json_esc(w->subtext, esc_sub,  sizeof(esc_sub));
    _color_json(fg_str, sizeof(fg_str), w->fg_color);
    _color_json(vc_str, sizeof(vc_str), w->value_color);

    char buf[1024];
    snprintf(buf, sizeof(buf),
        "{\"t\":\"ui_widget\","
        "\"id\":\"%s\","
        "\"kind\":%d,"
        "\"x\":%.4f,\"y\":%.4f,\"w\":%.4f,\"h\":%.4f,"
        "\"text\":\"%s\","
        "\"sub\":\"%s\","
        "\"val\":%.4f,\"val_max\":%.4f,"
        "\"fg\":\"%s\",\"vc\":\"%s\","
        "\"vis\":%s,\"en\":%s}",
        w->id, (int)w->kind,
        w->x, w->y, w->w, w->h,
        esc_text, esc_sub,
        w->value, w->value_max,
        fg_str, vc_str,
        w->visible ? "true" : "false",
        w->enabled ? "true" : "false");

    ws_broadcastf(ui->ws, "%s", buf);
}

void catedral_ws_push(CatedralUI *ui) {
    if (!ui->ws) return;
    /* Batch: emite un array de todos los widgets */
    ws_broadcastf(ui->ws, "{\"t\":\"ui_frame\",\"fc\":%llu}",
                  (unsigned long long)ui->frame_count);
    for (int i = 0; i < ui->n_widgets; i++) {
        CUIWidget *w = &ui->pool[i];
        if (w->visible) catedral_ws_push_widget(ui, w);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * §9  Dashboard RIGCOM predefinido
 * ══════════════════════════════════════════════════════════════════════════ */

void catedral_build_rigcom_dashboard(CatedralUI *ui) {
    float pw = (float)ui->screen_w, ph = (float)ui->screen_h;
    float gap  = CUI_GAP  / pw;
    float gap_h= CUI_GAP  / ph;

    /* ── Panel cabecera ──────────────────────────────────────────────── */
    CUIWidget *hdr = catedral_panel_new(ui, NULL, "hdr",
        gap, gap_h, 1.0f - 2*gap, 0.08f);
    hdr->bg_color     = 0x0D0D1AE8;
    hdr->border_color = CUI_COLOR_GOLD;
    hdr->border_px    = 2.0f;

    catedral_label_new(ui, hdr, "⬡ RIGCOM v36 — SOVEREIGN", CUI_COLOR_GOLD,
                       gap * 2, gap_h * 2, CUI_FONT_LG);
    catedral_label_new(ui, hdr, "φ=1.618 · AES-256-IGE · MTProto 2.0 · ARM64",
                       CUI_COLOR_MUTED,
                       gap * 2, gap_h * 2 + CUI_LINE_H/ph, CUI_FONT_SM);

    catedral_badge_new(ui, hdr, "RUNNING", CUI_COLOR_GREEN,
                       1.0f - gap*2 - 0.1f, gap_h * 2);

    /* ── Panel métricas (fila) ────────────────────────────────────────── */
    float mw = (1.0f - 5*gap) / 4.0f;
    float my = gap_h + 0.08f + gap_h;

    CUIWidget *m_build = catedral_metric_new(ui, NULL, "m_build",
        gap, my, mw, 0.14f, "BUILD ms", 0.0f);
    m_build->fg_color = CUI_COLOR_GOLD;

    CUIWidget *m_files = catedral_metric_new(ui, NULL, "m_files",
        gap + mw + gap, my, mw, 0.14f, "SOURCES", 0.0f);
    m_files->fg_color = CUI_COLOR_CYAN;

    CUIWidget *m_warns = catedral_metric_new(ui, NULL, "m_warns",
        gap + 2*(mw+gap), my, mw, 0.14f, "WARNINGS", 0.0f);
    m_warns->fg_color = CUI_COLOR_ORANGE;

    CUIWidget *m_errs = catedral_metric_new(ui, NULL, "m_errs",
        gap + 3*(mw+gap), my, mw, 0.14f, "ERRORS", 0.0f);
    m_errs->fg_color = CUI_COLOR_RED;

    /* ── Panel compilación con barra de progreso ──────────────────────── */
    float cy = my + 0.14f + gap_h;
    CUIWidget *cpan = catedral_panel_new(ui, NULL, "compile_panel",
        gap, cy, 1.0f - 2*gap, 0.06f);

    catedral_label_new(ui, cpan, "Compilación", CUI_COLOR_MUTED,
                       gap*2, gap_h*2, CUI_FONT_SM);
    catedral_progress_new(ui, cpan, "compile_bar",
                          gap*2, gap_h*2 + CUI_LINE_H/ph,
                          1.0f - 6*gap, 0.025f,
                          100.0f, CUI_COLOR_GOLD);

    /* ── Sparklines: CPU, RAM, NET ────────────────────────────────────── */
    float sy = cy + 0.06f + gap_h;
    float sw3 = (1.0f - 4*gap) / 3.0f;

    CUIWidget *sp_cpu = catedral_sparkline_new(ui, NULL, "sp_cpu",
        gap, sy, sw3, 0.10f);
    catedral_label_new(ui, sp_cpu, "CPU", CUI_COLOR_CYAN,
                       gap*2, gap_h*2, CUI_FONT_SM);

    CUIWidget *sp_ram = catedral_sparkline_new(ui, NULL, "sp_ram",
        gap + sw3 + gap, sy, sw3, 0.10f);
    catedral_label_new(ui, sp_ram, "RAM", CUI_COLOR_GREEN,
                       gap*2, gap_h*2, CUI_FONT_SM);

    CUIWidget *sp_net = catedral_sparkline_new(ui, NULL, "sp_net",
        gap + 2*(sw3+gap), sy, sw3, 0.10f);
    catedral_label_new(ui, sp_net, "NET", CUI_COLOR_GOLD,
                       gap*2, gap_h*2, CUI_FONT_SM);
    sp_cpu->value_color = CUI_COLOR_CYAN;
    sp_ram->value_color = CUI_COLOR_GREEN;
    sp_net->value_color = CUI_COLOR_GOLD;

    /* ── Botones de acción ────────────────────────────────────────────── */
    float by = sy + 0.10f + gap_h;
    float bw = (1.0f - 5*gap) / 4.0f;
    float bh = 0.055f;

    catedral_button_new(ui, NULL, "btn_build", "⚙ BUILD",
        gap, by, bw, bh, NULL, NULL);
    catedral_button_new(ui, NULL, "btn_selfbuild", "⬡ SELFBUILD",
        gap + bw + gap, by, bw, bh, NULL, NULL);
    catedral_button_new(ui, NULL, "btn_lsp", "◉ LSP",
        gap + 2*(bw+gap), by, bw, bh, NULL, NULL);
    catedral_button_new(ui, NULL, "btn_wasm", "◈ WASM",
        gap + 3*(bw+gap), by, bw, bh, NULL, NULL);

    /* ── Panel MTProto / cripto status ────────────────────────────────── */
    float ky = by + bh + gap_h;
    CUIWidget *kpan = catedral_panel_new(ui, NULL, "crypto_panel",
        gap, ky, 1.0f - 2*gap, 0.07f);
    kpan->border_color = CUI_COLOR_GOLD2;

    catedral_label_new(ui, kpan, "MTProto 2.0", CUI_COLOR_GOLD2,
                       gap*2, gap_h*2, CUI_FONT_SM);
    catedral_badge_new(ui, kpan, "AES-256-IGE", CUI_COLOR_CYAN,
                       gap*2 + 0.1f, gap_h*2);
    catedral_badge_new(ui, kpan, "SHA-256", CUI_COLOR_GREEN,
                       gap*2 + 0.22f, gap_h*2);
    catedral_badge_new(ui, kpan, "φ-KDF", CUI_COLOR_GOLD,
                       gap*2 + 0.30f, gap_h*2);
    catedral_badge_new(ui, kpan, "MTP_SLOTS=89", CUI_COLOR_MUTED,
                       gap*2 + 0.38f, gap_h*2);
}

/* ══════════════════════════════════════════════════════════════════════════
 * §10  Tick y hook de render loop
 * ══════════════════════════════════════════════════════════════════════════ */

void catedral_ui_tick(CatedralUI *ui) {
    double now = _now_s();
    ui->delta_time = now - ui->frame_time;
    ui->frame_time = now;
    ui->frame_count++;

    /* φ-animation (φ rad/s) */
    ui->phi_t += (float)(ui->delta_time * CUI_PHI);

    /* Input: detectar flanco de touch */
    for (int i = 0; i < ui->n_roots; i++)
        _process_input_widget(ui, ui->roots[i]);
    ui->touch_prev_down = ui->touch_down;

    /* GPU render */
    for (int i = 0; i < ui->n_roots; i++)
        _render_widget(ui, ui->roots[i]);

    /* WS broadcast (cada 1/φ³ ≈ 0.236 s) */
    if (now - ui->last_ws_push >= ui->ws_push_interval) {
        catedral_ws_push(ui);
        ui->last_ws_push = now;
    }
}

/* Hook que se inyecta en RigRenderLoop.on_frame_end */
static void _frame_end_hook(RigRenderLoop *rl, void *ud) {
    CatedralUI *ui = (CatedralUI *)ud;
    (void)rl;
    catedral_ui_tick(ui);
}
/* ══════════════════════════════════════════════════════════════════════════
 * §11  Init / Destroy / Run / Resize / Input
 * ══════════════════════════════════════════════════════════════════════════ */

int catedral_ui_init(CatedralUI *ui, RigRenderLoop *rl, WsServer *ws,
                      uint32_t screen_w, uint32_t screen_h) {
    if (!ui) return -1;
    memset(ui, 0, sizeof(*ui));
    ui->rl        = rl;
    ui->ws        = ws;
    ui->screen_w  = screen_w  ? screen_w  : 1080;
    ui->screen_h  = screen_h  ? screen_h  : 1920;
    ui->frame_time= _now_s();
    /* 1/φ³ = φ⁻³ ≈ 0.2360 s entre pushes WS */
    ui->ws_push_interval = 1.0 / (CUI_PHI * CUI_PHI * CUI_PHI);

    ui->gpu_ready = _gpu_init__rig_variant_77c19423(ui);
    if (!ui->gpu_ready)
        fprintf(stderr, "[catedral_ui] backend WebSocket activo\n");

    fprintf(stderr, "[catedral_ui] init OK  %ux%u  gpu=%d  ws=%s\n",
            ui->screen_w, ui->screen_h, ui->gpu_ready,
            ws ? "SI" : "NO");
    return 0;
}

void catedral_ui_destroy(CatedralUI *ui) {
    if (!ui) return;
    if (ui->rl)
        ui->rl->on_frame_end = NULL;
    memset(ui, 0, sizeof(*ui));
}

void catedral_ui_run(CatedralUI *ui) {
    if (!ui || !ui->rl) return;
    /* Inyectar hook en el render loop */
    ui->rl->on_frame_end = _frame_end_hook;
    ui->rl->userdata     = ui;
    fprintf(stderr, "[catedral_ui] hook en on_frame_end instalado\n");
}

void catedral_ui_resize(CatedralUI *ui, uint32_t w, uint32_t h) {
    if (!ui) return;
    ui->screen_w = w;
    ui->screen_h = h;
}

void catedral_ui_touch(CatedralUI *ui, float nx, float ny, bool down) {
    if (!ui) return;
    ui->touch_x    = nx;
    ui->touch_y    = ny;
    ui->touch_down = down;
}

/* catedral_ui_key — implementado en §IMPLEMENTACIONES PENDIENTES */

/* ═══════════════════════════════════════════════════════════════════════════
 * §IMPLEMENTACIONES PENDIENTES — catedral_ui.c
 * Autor: Richard Felipe Urbina
 *
 * Fix #6a: Sparkline con shader de curva real (Bezier suavizado)
 * Fix #6b: catedral_ui_key() con routing completo a CUI_INPUT
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ─────────────────────────────────────────────────────────────────────────
 * SPARKLINE SHADER — dibuja la curva histórica como polilínea suavizada
 * Técnica: el fragment shader recibe los 64 samples vía uniform array y
 * evalúa la distancia al segmento más cercano en espacio de pantalla.
 * ─────────────────────────────────────────────────────────────────────────*/
static const char *k_frag_sparkline =
    "#version 300 es\n"
    "precision highp float;\n"
    "in  vec2 v_uv;\n"
    "out vec4 o_color;\n"
    "\n"
    "uniform vec4  u_rect;           /* x,y,w,h normalizados */\n"
    "uniform vec2  u_resolution;\n"
    "uniform vec4  u_color_bg;\n"
    "uniform vec4  u_color_line;     /* color de la curva */\n"
    "uniform vec4  u_color_fill;     /* área bajo la curva */\n"
    "uniform float u_radius;\n"
    "uniform float u_samples[64];   /* valores históricos normalizados 0..1 */\n"
    "uniform int   u_n_samples;     /* cuántos son válidos */\n"
    "uniform float u_line_w;        /* grosor de línea en px */\n"
    "uniform float u_glow;          /* intensidad del glow φ-pulsante */\n"
    "\n"
    "/* Distancia punto-segmento en espacio UV */\n"
    "float seg_dist(vec2 p, vec2 a, vec2 b) {\n"
    "  vec2 ab = b - a, ap = p - a;\n"
    "  float t  = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);\n"
    "  return length(ap - ab * t);\n"
    "}\n"
    "\n"
    "/* RR dist para recorte del fondo */\n"
    "float rr_dist(vec2 p, vec2 hs, float r) {\n"
    "  vec2 q = abs(p) - hs + r;\n"
    "  return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "  vec2 px = v_uv * u_rect.zw * u_resolution;\n"
    "  vec2 hs = u_rect.zw * u_resolution * 0.5;\n"
    "\n"
    "  /* Recorte RR del fondo del widget */\n"
    "  float d_bg = rr_dist(px - hs, hs - 0.5, u_radius);\n"
    "  float aa_bg = 1.0 - smoothstep(-0.5, 0.5, d_bg);\n"
    "  if (aa_bg < 0.01) discard;\n"
    "\n"
    "  vec2 sz = u_rect.zw * u_resolution;    /* tamaño en píxeles */\n"
    "  float lw  = max(u_line_w, 1.0);\n"
    "  float n   = float(max(u_n_samples - 1, 1));\n"
    "\n"
    "  /* Distancia mínima a cualquier segmento de la curva */\n"
    "  float min_d = 1e9;\n"
    "  float fill_y = sz.y;                   /* y del segmento en px relativo al uv */\n"
    "\n"
    "  for (int i = 0; i < 63; i++) {\n"
    "    if (i >= u_n_samples - 1) break;\n"
    "    float t0 = float(i)     / n;\n"
    "    float t1 = float(i + 1) / n;\n"
    "    vec2 a = vec2(t0 * sz.x, (1.0 - u_samples[i    ]) * sz.y);\n"
    "    vec2 b = vec2(t1 * sz.x, (1.0 - u_samples[i + 1]) * sz.y);\n"
    "    min_d = min(min_d, seg_dist(px, a, b));\n"
    "    /* Seguimiento del fill en la x actual */\n"
    "    float fx = px.x / sz.x;\n"
    "    if (fx >= t0 && fx < t1) {\n"
    "      float lerp_t = (fx - t0) / max(t1 - t0, 0.0001);\n"
    "      fill_y = mix(a.y, b.y, lerp_t);\n"
    "    }\n"
    "  }\n"
    "\n"
    "  /* Línea AA */\n"
    "  float line_aa  = 1.0 - smoothstep(lw * 0.5, lw * 0.5 + 1.0, min_d);\n"
    "  /* Glow φ-pulsante */\n"
    "  float glow_aa  = (1.0 - smoothstep(lw * 0.5, lw * 3.0, min_d)) * u_glow;\n"
    "  /* Área rellena bajo la curva */\n"
    "  float in_fill  = step(fill_y, px.y) * 0.32;\n"
    "\n"
    "  vec4 col = u_color_bg * aa_bg;\n"
    "  col = mix(col, u_color_fill, in_fill * aa_bg);\n"
    "  col = mix(col, u_color_line + vec4(u_glow * 0.2), (line_aa + glow_aa) * aa_bg);\n"
    "  o_color = col;\n"
    "}\n";

/* Compila el programa sparkline y lo registra en la UI */
static void _sparkline_gpu_init(CatedralUI *ui) {
    if (!ui) {
        return;
    }

    /* Estado conocido: sin programa hasta que la GPU lo compile con éxito.
     * Deja el campo determinista también en builds sin backend GLES, donde
     * _draw_sparkline() cae al fallback de quad. */
    ui->prog_sparkline = 0u;

#ifdef RIG_BACKEND_GLES
    ui->prog_sparkline = _compile_prog(k_vert_quad, k_frag_sparkline);
#endif
}
/* Dibuja el sparkline con la curva real */
static void _draw_sparkline(CatedralUI *ui, const CUIWidget *w) {
#ifdef RIG_BACKEND_GLES
    if (!ui->prog_sparkline) {
        /* Fallback: quad de fondo simple si el shader aún no está listo */
        _draw_quad__rig_variant_bc529760(ui, w);
        return;
    }

    rig_gpu_use_program(ui->prog_sparkline);

    float sw = (float)ui->screen_w, sh = (float)ui->screen_h;
    _gpu_set_uniform_4f(ui->prog_sparkline, "u_rect",  w->x, w->y, w->w, w->h);
    _gpu_set_uniform_2f(ui->prog_sparkline, "u_resolution", sw, sh);
    _gpu_set_uniform_1f(ui->prog_sparkline, "u_radius", w->radius / sw);
    _gpu_set_uniform_1f(ui->prog_sparkline, "u_line_w", 2.0f);

    float glow = w->animated
        ? (0.5f + 0.5f * sinf(ui->phi_t * CUI_PHI))
        : 0.0f;
    _gpu_set_uniform_1f(ui->prog_sparkline, "u_glow", glow);

    /* Fondo */
    float r, g, b, a;
    _rgba_f(w->bg_color, &r, &g, &b, &a);
    _gpu_set_uniform_4f(ui->prog_sparkline, "u_color_bg", r*a, g*a, b*a, a);

    /* Línea: fg_color o verde φ por defecto */
    uint32_t lc = w->fg_color ? w->fg_color : 0x00FF80FFu;
    _rgba_f(lc, &r, &g, &b, &a);
    _gpu_set_uniform_4f(ui->prog_sparkline, "u_color_line", r, g, b, a);

    /* Fill: versión oscurecida de la línea */
    _gpu_set_uniform_4f(ui->prog_sparkline, "u_color_fill",
                         r * 0.3f, g * 0.3f, b * 0.3f, 0.4f);

    /* Normalizar samples al rango [0,1] */
    int   n   = w->history_len < 64 ? w->history_len : 64;
    float vmin = 1e30f, vmax = -1e30f;
    for (int i = 0; i < n; i++) {
        int idx = (w->history_head - n + i + 64) & 63;
        float v = w->history[idx];
        if (v < vmin) vmin = v;
        if (v > vmax) vmax = v;
    }
    float range = (vmax - vmin) > 1e-6f ? (vmax - vmin) : 1.0f;

    float norm[64];
    for (int i = 0; i < n; i++) {
        int idx = (w->history_head - n + i + 64) & 63;
        norm[i] = (w->history[idx] - vmin) / range;
    }

    int loc_samples = rig_gpu_uniform_loc(ui->prog_sparkline, "u_samples");
    if (loc_samples >= 0)
        rig_gpu_uniform_1fv(loc_samples, n, norm);

    int loc_n = rig_gpu_uniform_loc(ui->prog_sparkline, "u_n_samples");
    if (loc_n >= 0)
        rig_gpu_uniform_1i(loc_n, n);

    rig_gpu_bind_vao(ui->vao_quad);
    rig_gpu_draw_triangles(0, 6);
#else
    /* Sin GPU: render de texto de consola en WS */
    _draw_quad__rig_variant_bc529760(ui, w);
#endif
}
/* Inyectar el init del sparkline en _gpu_init — se llama via parche de init */
/* NOTA: catedral_ui_init() debe llamar _sparkline_gpu_init() después del    */
/* init principal. Se añade al final de catedral_ui_init mediante wrapper.    */

/* ─────────────────────────────────────────────────────────────────────────
 * Segunda fase del renderer de sparkline.
 *
 * El switch original tiene:
 *   case CUI_SPARKLINE:
 *       _draw_quad__rig_variant_bc529760(ui, w);
 *       _draw_sparkline(ui, w)
 *       break;
 *
 * Usamos una función de dispatch que el draw original debería llamar.
 * Como catedral_ui_draw() ya existe, añadimos un wrapper de segunda fase.
 * ─────────────────────────────────────────────────────────────────────────*/

/* Dibuja UN widget enrutando al renderer correcto */
void catedral_ui_draw_widget(CatedralUI *ui, const CUIWidget *w) {
    if (!w || !w->visible) return;
    switch (w->kind) {
        case CUI_SPARKLINE:
            _draw_sparkline(ui, w);
            break;
        default:
            /* Para los demás tipos, delegar al draw original */
            break;
    }
}

/* ─────────────────────────────────────────────────────────────────────────
 * Fix #6b: catedral_ui_key — routing completo al widget CUI_INPUT con foco
 *
 * Implementación:
 *  - ENTER/CLICK → da foco al widget bajo el puntero (ver catedral_ui_touch)
 *  - Caracteres imprimibles → añaden al text del widget enfocado
 *  - BACKSPACE (key=8) → borra último carácter
 *  - ESCAPE (key=27) → quita el foco
 *  - El cambio dispara on_change si está definido
 * ─────────────────────────────────────────────────────────────────────────*/

/* Códigos de tecla especiales (compatibles con GLFW y XKB) */
#define CUI_KEY_BACKSPACE  8
#define CUI_KEY_ENTER     13
#define CUI_KEY_ESCAPE    27
#define CUI_KEY_DELETE   127
#define CUI_KEY_LEFT     260
#define CUI_KEY_RIGHT    261
#define CUI_KEY_HOME     262
#define CUI_KEY_END      263

/* Helpers de acceso al buffer de texto del input */
static size_t _input_len(const CUIWidget *w) {
    return strnlen(w->text, CUI_MAX_TEXT - 1);
}
static void _input_append_char(CUIWidget *w, char ch) {
    size_t len = _input_len(w);
    if (len + 1 < CUI_MAX_TEXT - 1) {
        w->text[len]     = ch;
        w->text[len + 1] = '\0';
    }
}
static void _input_backspace(CUIWidget *w) {
    size_t len = _input_len(w);
    if (len > 0) w->text[len - 1] = '\0';
}
/* ── catedral_ui_focus: dar foco a un widget INPUT ────────────────────── */
void catedral_ui_focus(CatedralUI *ui, CUIWidget *w) {
    if (!ui) return;
    if (ui->focused_widget) {
        ui->focused_widget->hovered = false;
    }
    ui->focused_widget = (w && w->kind == CUI_INPUT) ? w : NULL;
    if (ui->focused_widget) {
        ui->focused_widget->hovered = true; /* usa hovered como "focused" visual */
    }
}

/* ── catedral_ui_key: routing real al input con foco ─────────────────── */
void catedral_ui_key(CatedralUI *ui, uint32_t key, bool pressed) {
    if (!ui || !pressed) return;    /* sólo acción en key-down */

    /* Sin widget enfocado: intentar enfocar el primero de tipo INPUT */
    if (!ui->focused_widget) {
        for (int i = 0; i < ui->n_widgets; i++) {
            if (ui->pool[i].kind == CUI_INPUT && ui->pool[i].visible) {
                catedral_ui_focus(ui, &ui->pool[i]);
                break;
            }
        }
        if (!ui->focused_widget) return;
    }

    CUIWidget *w = ui->focused_widget;

    switch (key) {
        case CUI_KEY_ESCAPE:
            catedral_ui_focus(ui, NULL);
            return;

        case CUI_KEY_BACKSPACE:
        case CUI_KEY_DELETE:
            _input_backspace(w);
            break;

        case CUI_KEY_ENTER:
            /* ENTER confirma el valor numérico escrito, si existe. */
            if (w->on_change) {
                char *end = NULL;
                float value = strtof(w->text, &end);
                if (end == w->text) value = (float)_input_len(w);
                w->on_change(w, value, w->userdata);
            }
            catedral_ui_focus(ui, NULL);
            return;

        default:
            if (key >= 32 && key < 127) {
                /* Carácter ASCII imprimible */
                _input_append_char(w, (char)key);
            } else if (key >= 0xA0 && key <= 0xFF) {
                /* Latin-1 extendido — incluir si cabe en UTF-8 básico */
                _input_append_char(w, (char)key);
            }
            break;
    }

    /* Notificar cambio de valor */
    if (w->on_change) {
        w->on_change(w, (float)_input_len(w), w->userdata);
    }
}

/* ── Inicialización de sparkline en catedral_ui_init ─────────────────── */
/* Llama a este función AL FINAL de catedral_ui_init() para completar    */
/* el setup del shader de sparkline. El wrapper es necesario porque      */
/* catedral_ui_init() ya está definida más arriba en este fichero.       */
void catedral_ui_init_sparkline(CatedralUI *ui) {
    if (!ui) return;
    _sparkline_gpu_init(ui);
}
