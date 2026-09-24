/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "../include/rigart_v5_mobile.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_noext_io.h"
#define PHI 1.6180339887498948482
#define PI 3.14159265358979323846

static const char* TEX_NAMES[RIG_TEX_COUNT_V5] = {
"Seda Charmeuse","Seda Dupioni","Gasa de Seda","Satén de Seda",
"Cachemira Fina","Cachemira Gruesa","Terciopelo Aplastado","Terciopelo Cortado",
"Brocado Oro","Damasco","Encaje Bolillo","Lino Fino",
"Lana Merino","Cuero Aniline","Nubuck","Aceite Viscoso",
"Aceite Iridiscente","Cera de Vela","Resina Ámbar","Laca Japonesa",
"Nácar","Jade Hetián","Malaquita","Obsidiana Profunda",
"Mokumé-Gané","Pátina Verdigris"
};
static const char* TEX_DESC[RIG_TEX_COUNT_V5] = {
"Brillo anisótropo, sheen rasante de trama",

"Hilo doble, textura de urdimbre visible",
"Translúcida, flotante, dispersión de luz",
"Máximo reflejo anisótropo de tejido",
"Microfibre 12μm con SSS cálido",
"Punto grueso visible, SSS profundo",
"BRDF retroreflexivo de Ashikhmin-Shirley",
"Patrón geométrico cortado, brillo filo",
"Hilo metálico entretejido, oro puro",
"Patrón reversible, seda + hilo",
"Transparencia orgánica, patrón floral",
"Fibra natural anisótropa, IOR=1.47",
"SSS cálido difuso, microfibra fina",
"Poros visibles, SSS cuero natural",
"Microvellosidades retroreflectantes",
"Flow dinámico viscoso, 15mPa·s",
"Película delgada, interferencia espectral",
"SSS translúcido cálido 2.3mm",
"Oclusión interna, tonos ámbar",
"Profundidad óptica, 12 capas urushi",
"Difracción espectral iridiscente",
"SSS verde translúcido hetián",
"Patrón concéntrico procedural",
"Reflejo volcánico, IOR=1.5",
"Laminado metales mixtos, oro+plata",
"Cobre oxidado, pátina verde procedural"
};
const char* rigart_v5_texture_name(RigTextureV5 t) {
if (t < 0 || t >= RIG_TEX_COUNT_V5) return "unknown";
return TEX_NAMES[t];
}
const char* rigart_v5_texture_desc(RigTextureV5 t) {
if (t < 0 || t >= RIG_TEX_COUNT_V5) return "";
return TEX_DESC[t];
}

static const char* WIN_NAMES[RIG_WIN_M_COUNT] = {
"Seda Bottom Sheet","Cachemira Drawer","Terciopelo Fullscreen",
"Aceite Card","Lino Sidebar","Brocado Overlay","Pan de Oro Dialog",
"Laca Modal","Nácar Popup","Jade Content Card","Mercurio Panel",
"Iris Esmerilado","Fibra de Carbono","Satén Ribbon Nav",
"Cobre Oxidado Tab","Cortina Drape","Origami Fold","Liquid Morph"
};
const char* rigart_v5_window_mobile_name(RigWindowMobile w) {
if (w < 0 || w >= RIG_WIN_M_COUNT) return "unknown";
return WIN_NAMES[w];
}

static const char* TYPO_NAMES[RIG_TYPO_M_COUNT] = {
"Aceite Impreso","Seda Relieve","Oro Grabado","Laca Tallada",
"Jade Inlay","Terciopelo Flocado","Cromo Líquido","Fuego Marcado",
"Cristal Facetado","Mercurio Fluido","Aurora Trace","Obsidiana Cortada",
"Hilo Tejido","Caligrafía Aceite"
};
const char* rigart_v5_typo_mobile_name(RigTypoMobile t) {
if (t < 0 || t >= RIG_TYPO_M_COUNT) return "unknown";
return TYPO_NAMES[t];
}

static const char* KB_NAMES[RIG_KB_COUNT] = {
"Obsidiana + Aceite","Seda Blanca","Pan de Oro",
"Mármol Carrara","Cachemira Noche","Fibra de Carbono",
"Laca Roja","Jade Imperial","Mercurio Líquido",
"Brocado Real","Roca Volcánica","Vidrio Aurora"
};
const char* rigart_v5_keyboard_name(RigKeyboardMaterial m) {
if (m < 0 || m >= RIG_KB_COUNT) return "unknown";
return KB_NAMES[m];
}

static const char* TPL_NAMES[RIG_TPL_COUNT] = {
"Galería Luxury","Fashion Lookbook","Joyería 3D","Parfum Landing",
"Couture Atelier","Reloj Detalle","Reloj Ceremonial","Esfera Timepiece",
"Hotel Concierge","Spa Booking","Menú Gastronómico","Bodega de Vinos",
"Finance Soberano","Subasta de Arte","Banca Privada","Jet Privado",
"Control de Yate","Lector Manuscrito","Museo de Arte","Teclado Obsidiana",
"Teclado Seda","Teclado Oro","Teclado Mármol","Marcador Ceremonial",
"Chat Soberano","Configuración Premium","Medidas Sastre","Galería RigFace"
};
const char* rigart_v5_template_name(RigTemplateMobile t) {
if (t < 0 || t >= RIG_TPL_COUNT) return "unknown";
return TPL_NAMES[t];
}

void rigart_v5_init_result(RigResultV5 *res) {
if (!res) return;
memset(res, 0, sizeof(RigResultV5));
res->phi_ratio = PHI;
res->certeza = 1.0f;
res->ok
= false;
}

void rigart_v5_free_result(RigResultV5 *res) {
if (!res) return;
free(res->html); free(res->css); free(res->js);
free(res->glsl_frag); free(res->glsl_vert);
memset(res, 0, sizeof(RigResultV5));
}

int rigart_v5_textile_glsl(const RigTextureV5Ctx *ctx, RigResultV5 *out) {
if (!ctx || !out) return -1;
rigart_v5_init_result(out);
char *fs = malloc(RIG_V5_SHADER_BUF);
if (!fs) { snprintf(out->error,256,"OOM shader"); return -2; }
switch (ctx->type) {
case RIG_TEX_SILK_CHARMEUSE:
case RIG_TEX_SILK_SATIN:
snprintf(fs, RIG_V5_SHADER_BUF,
"#version 300 es\nprecision highp float;\nin vec2 vUV; uniform float uT;\n"
"out vec4 frag;\nconst float PHI=%.10f;\n"
"void main(){\n"
" vec3 L=normalize(vec3(sin(uT*.3)*.4+.3,.85,.5));\n"
" vec3 T=vec3(cos(uT*.1),sin(uT*.1),0.);\n"
" vec3 H=normalize(L+vec3(0,0,1));\n"
" float tdh=dot(T,H),ndh=max(dot(normalize(vec3((vUV-.5)*.08,1.)),H),0.);\n"
" float D=exp(-(tdh*tdh/%.4f+max(1.-tdh*tdh,0.)/%.4f)/(ndh*ndh+.001))/(.001+4.*%.4f*%.4f*pow(ndh,4.));\n"
" float sh=pow(1.-max(dot(normalize(vec3((vUV-.5)*.08,1.)),vec3(0,0,1)),0.),PHI*1.4)*%.4f;\n"
" vec3 col=vec3(%.3f,%.3f,%.3f)*(max(dot(normalize(vec3((vUV-.5)*.08,1.)),L),0.)*.7+.22)+vec3(D*.2)+vec3(%.3f,%.3f,%.3f)*sh*.5;\n"
" frag=vec4(pow(max(col,vec3(0.)),vec3(1./2.2)),1.);\n"
"}\n",
PHI,
ctx->sheen_angle * ctx->sheen_angle,
ctx->weave_scale > 0 ? ctx->weave_scale * ctx->weave_scale : 0.36f,
ctx->sheen_angle, ctx->weave_scale > 0 ? ctx->weave_scale : 0.6f,
ctx->sheen_strength,
ctx->base_color.x, ctx->base_color.y, ctx->base_color.z,
ctx->highlight_color.x, ctx->highlight_color.y, ctx->highlight_color.z
);
break;
case RIG_TEX_OIL_IRIDESCENT:
snprintf(fs, RIG_V5_SHADER_BUF,
"#version 300 es\nprecision highp float;\nin vec2 vUV; uniform float uT;\n"
"out vec4 frag;\nconst float PI=3.14159;\n"
"void main(){\n"

" vec2 fl=vec2(sin(vUV.y*3.+uT*%.2f)*.06,cos(vUV.x*2.5+uT*%.2f)*.05);\n"
" vec2 fuv=vUV+fl;\n"
" float th=400.+200.*sin(fuv.x*9.+uT*.5)+150.*cos(fuv.y*7.-uT*.4);\n"
" float OPD=2.*1.47*th;\n"
" vec3 rgb=vec3(.5+.5*cos(PI*2.*OPD/680.),.5+.5*cos(PI*2.*OPD/530.),.5+.5*cos(PI*2.*OPD/450.));\n"
" vec3 base=vec3(%.3f,%.3f,%.3f);\n"
" float r=.04+.94*pow(1.-(.8+.2*sin(vUV.x*8.+uT*.3)),.8);\n"
" frag=vec4(pow(max(base+rgb*r,vec3(0.)),vec3(1./2.2)),1.);\n"
"}\n",
ctx->flow_speed, ctx->flow_speed * 0.85f,
ctx->base_color.x, ctx->base_color.y, ctx->base_color.z
);
break;
default:
snprintf(fs, RIG_V5_SHADER_BUF,
"#version 300 es\nprecision highp float;\nin vec2 vUV; uniform float uT;\n"
"out vec4 frag;\nvoid main(){frag=vec4(%.3f,%.3f,%.3f,1.);}\n",
ctx->base_color.x, ctx->base_color.y, ctx->base_color.z
);
break;
}
out->glsl_frag = fs;
out->ok = true;
return 0;
}

void rigart_v5_ws_handle(WsServer *srv, const char *cmd, const char *payload) {
if (!srv || !cmd || !payload) return;
if (strcmp(cmd, "v5_material_list") == 0) {
char buf[4096]; int
w=snprintf(buf,sizeof(buf),"{\"ev\":\"v5_material_list\",\"count\":%d,\"materials\":[",RIG_TEX_COUNT_V5);
for (int i = 0; i < RIG_TEX_COUNT_V5 && w < (int)sizeof(buf)-64; i++) {
w+=snprintf(buf+w,sizeof(buf)-w,"%s{\"id\":%d,\"name\":\"%s\"}",i?",":"",i,rigart_v5_texture_name((RigTextureV5)i));
}
snprintf(buf+w,sizeof(buf)-w,"]}");
ws_broadcastf(srv,"%s",buf);
} else if (strcmp(cmd, "v5_template_list") == 0) {
char buf[4096]; int
w=snprintf(buf,sizeof(buf),"{\"ev\":\"v5_template_list\",\"count\":%d,\"templates\":[",RIG_TPL_COUNT);
for (int i = 0; i < RIG_TPL_COUNT && w < (int)sizeof(buf)-64; i++) {

w+=snprintf(buf+w,sizeof(buf)-w,"%s{\"id\":%d,\"name\":\"%s\"}",i?",":"",i,rigart_v5_template_name((RigTemplateMobile)i));
}
snprintf(buf+w,sizeof(buf)-w,"]}");
ws_broadcastf(srv,"%s",buf);
} else if (strcmp(cmd, "v5_keyboard_list") == 0) {
char buf[2048]; int
w=snprintf(buf,sizeof(buf),"{\"ev\":\"v5_keyboard_list\",\"keyboards\":[");
for (int i = 0; i < RIG_KB_COUNT && w < (int)sizeof(buf)-64; i++) {
w+=snprintf(buf+w,sizeof(buf)-w,"%s{\"id\":%d,\"name\":\"%s\"}",i?",":"",i,rigart_v5_keyboard_name((RigKeyboardMaterial)i));
}
snprintf(buf+w,sizeof(buf)-w,"]}");
ws_broadcastf(srv,"%s",buf);
} else {
ws_broadcastf(srv,"{\"ev\":\"v5_ack\",\"cmd\":\"%s\",\"ok\":true}",cmd);
}
}

/* ═══════════════════════════════════════════════════════════════════════════
 * §IMPLEMENTACIONES PENDIENTES — rigart_v5_mobile.c  (antes: 16 sin cuerpo)
 * Autor: Richard Felipe Urbina
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rig_noext_str.h"
#include "rig_noext_io.h"
#include "rig_math.h"

/* ─── Nombres de enumeraciones ──────────────────────────────────────────── */

/* WIN_NAMES, TYPO_NAMES, KB_NAMES ya definidos en la primera sección del archivo */
static const char *WIN_DESCS[RIG_WIN_M_COUNT] = {
    "Hoja inferior de seda, deslizamiento suave con tensión superficial",
    "Cajón de cachemira, resistencia táctil con rebote",
    "Pantalla completa de terciopelo, retroreflexión de Ashikhmin-Shirley",
    "Tarjeta de superficie oleosa, física de fluido viscoso 15mPa·s",
    "Barra lateral de lino, fibra natural anisótropa IOR=1.47",
    "Overlay de brocado, hilo metálico oro entretejido",
    "Diálogo de hoja de oro, laminado óptico multicapa",
    "Modal de laca japonesa, 12 capas urushi con profundidad óptica",
    "Popup de nácar, difracción espectral iridiscente",
    "Tarjeta de jade, SSS verde translúcido con patrón concéntrico",
    "Panel mercurio líquido, flow dinámico con tensión superficial",
    "Panel iris esmerilado, material de vidrio difuso retroiluminado",
    "Panel fibra de carbono, tejido de 0°/45°/90° con brillo anisotrópico",
    "Nav de cinta satén, máximo reflejo anisótropo de tejido",
    "Tab de cobre oxidado, pátina verde procedural con IOR variable",
    "Cortina con gravedad y viento procedural sobre malla de tela",
    "Fold de origami, plegado geométrico con aristas crisp",
    "Morph líquido, interpolación de forma con física de fluido"
};

const char *rigart_v5_window_mobile_desc(RigWindowMobile w) {
    if (w < 0 || w >= RIG_WIN_M_COUNT) return "";
    return WIN_DESCS[w];
}

/* ─── Textile CSS ────────────────────────────────────────────────────────── */
int rigart_v5_textile_css(const RigTextureV5Ctx *ctx, RigResultV5 *out) {
    if (!ctx || !out) return -1;
    rigart_v5_init_result(out);

    char *css = malloc(RIG_V5_SHADER_BUF);
    if (!css) return -1;

    /* Calcular hsl base desde base_color */
    int r = (int)(ctx->base_color.x * 255.0f);
    int g = (int)(ctx->base_color.y * 255.0f);
    int b = (int)(ctx->base_color.z * 255.0f);

    float weave_px  = fmaxf(1.0f, ctx->weave_scale * 4.0f);
    float sheen_deg = ctx->sheen_angle * 57.2957795f;

    int len = snprintf(css, RIG_V5_SHADER_BUF,
        "/* RIGCOM rigart v5 — Textile CSS: %s */\n"
        ".rig-textile {\n"
        "  background-color: rgb(%d,%d,%d);\n"
        "  background-image: repeating-linear-gradient(\n"
        "    %.1fdeg,\n"
        "    rgba(255,255,255,%.3f) 0px,\n"
        "    rgba(255,255,255,0)   %.1fpx,\n"
        "    rgba(0,0,0,%.3f)      %.1fpx,\n"
        "    rgba(0,0,0,0)         %.1fpx\n"
        "  );\n"
        "  opacity: %.3f;\n"
        "  backdrop-filter: blur(%.1fpx);\n"
        "  -webkit-backdrop-filter: blur(%.1fpx);\n"
        "}\n",
        rigart_v5_texture_name(ctx->type),
        r, g, b,
        sheen_deg,
        ctx->sheen_strength * 0.5f, weave_px * 0.5f,
        ctx->sheen_strength * 0.25f, weave_px,
        weave_px * 2.0f,
        ctx->opacity,
        ctx->surface_tension * 2.0f,
        ctx->surface_tension * 2.0f
    );

    out->css    = css;
    out->css_sz = (size_t)len;
    out->ok     = true;
    out->phi_ratio  = 1.6180339887498948482;
    out->certeza    = 0.97f;
    return 0;
}

/* ─── Textile Full (CSS + GLSL + HTML wrapper) ───────────────────────────── */
int rigart_v5_textile_full(const RigTextureV5Ctx *ctx, RigResultV5 *out) {
    if (!ctx || !out) return -1;
    rigart_v5_init_result(out);

    RigResultV5 css_res = {0}, glsl_res = {0};
    if (rigart_v5_textile_css(ctx, &css_res)  != 0) return -1;
    if (rigart_v5_textile_glsl(ctx, &glsl_res) != 0) {
        rigart_v5_free_result(&css_res); return -1;
    }

    char *html = malloc(RIG_V5_SHADER_BUF);
    if (!html) { rigart_v5_free_result(&css_res); rigart_v5_free_result(&glsl_res); return -1; }

    int hlen = snprintf(html, RIG_V5_SHADER_BUF,
        "<!DOCTYPE html>\n<html>\n<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<title>RIGCOM Textile: %s</title>\n"
        "<style>\n%s\n</style>\n</head>\n"
        "<body class=\"rig-textile\">\n"
        "<canvas id=\"rig-gl\" width=\"512\" height=\"512\"></canvas>\n"
        "<script>\n"
        "const FRAG='%s';\n"
        "/* RIGCOM v5 WebGL bootstrap */\n"
        "const c=document.getElementById('rig-gl');\n"
        "const gl=c.getContext('webgl2');\n"
        "if(gl){/* shader setup handled by rigart_v5_ws_handle */}\n"
        "</script>\n</body>\n</html>\n",
        rigart_v5_texture_name(ctx->type),
        css_res.css ? css_res.css : "",
        glsl_res.glsl_frag ? glsl_res.glsl_frag : ""
    );

    out->html       = html;
    out->html_sz    = (size_t)hlen;
    out->css        = css_res.css;  css_res.css = NULL;
    out->css_sz     = css_res.css_sz;
    out->glsl_frag  = glsl_res.glsl_frag; glsl_res.glsl_frag = NULL;
    out->glsl_vert  = glsl_res.glsl_vert; glsl_res.glsl_vert = NULL;
    out->ok         = true;
    out->phi_ratio  = 1.6180339887498948482;
    out->certeza    = 0.96f;
    rigart_v5_free_result(&css_res);
    rigart_v5_free_result(&glsl_res);
    return 0;
}

/* ─── Window Mobile ─────────────────────────────────────────────────────── */
int rigart_v5_window_mobile(const RigWindowMobileCtx *ctx, RigResultV5 *out) {
    if (!ctx || !out) return -1;
    rigart_v5_init_result(out);

    char *css = malloc(RIG_V5_SHADER_BUF);
    if (!css) return -1;

    int r = (int)(ctx->material_color.x * 255.0f);
    int g = (int)(ctx->material_color.y * 255.0f);
    int b = (int)(ctx->material_color.z * 255.0f);
    int br = (int)(ctx->border_color.x * 255.0f);
    int bg = (int)(ctx->border_color.y * 255.0f);
    int bb = (int)(ctx->border_color.z * 255.0f);

    int len = snprintf(css, RIG_V5_SHADER_BUF,
        "/* RIGCOM rigart v5 — Window Mobile: %s */\n"
        ".rig-window {\n"
        "  width: %.1f%%;\n"
        "  height: %.1f%%;\n"
        "  background: rgba(%d,%d,%d,%.3f);\n"
        "  border-radius: %.1fpx;\n"
        "  border: %.1fpx solid rgba(%d,%d,%d,1);\n"
        "%s"
        "  backdrop-filter: blur(%.1fpx);\n"
        "  -webkit-backdrop-filter: blur(%.1fpx);\n"
        "  box-shadow: 0 %.1fpx %.1fpx rgba(0,0,0,0.35);\n"
        "  transition: transform %.3fs cubic-bezier(0.34,1.56,0.64,1);\n"
        "  touch-action: pan-y;\n"
        "  user-select: none;\n"
        "}\n",
        WIN_NAMES[ctx->type < RIG_WIN_M_COUNT ? ctx->type : 0],
        ctx->width_pct  * 100.0f,
        ctx->height_pct * 100.0f,
        r, g, b,
        ctx->glass_opacity,
        ctx->corner_radius_px,
        ctx->border_width_px, br, bg, bb,
        ctx->border_glow
            ? "  filter: drop-shadow(0 0 8px rgba(255,215,0,0.6));\n" : "",
        ctx->backdrop_blur_px,
        ctx->backdrop_blur_px,
        ctx->elevation_dp * 0.5f,
        ctx->elevation_dp * 2.0f,
        ctx->transition_dur_s
    );

    out->css    = css;
    out->css_sz = (size_t)len;
    out->ok     = true;
    out->phi_ratio = 1.6180339887498948482;
    out->certeza   = 0.95f;
    return 0;
}

/* ─── Typo Mobile ───────────────────────────────────────────────────────── */

int rigart_v5_typo_mobile(const RigTypoMobileCtx *ctx, RigResultV5 *out) {
    if (!ctx || !out) return -1;
    rigart_v5_init_result(out);

    char *css = malloc(RIG_V5_SHADER_BUF);
    if (!css) return -1;

    int r = (int)(ctx->base_color.x * 255.0f);
    int g = (int)(ctx->base_color.y * 255.0f);
    int b = (int)(ctx->base_color.z * 255.0f);

    /* Sombra de texto según material */
    const char *text_shadow = "";
    switch (ctx->style) {
        case RIG_TYPO_M_GOLD_ENGRAVED:
            text_shadow = "  text-shadow: 0 1px 2px rgba(255,215,0,0.8);\n"; break;
        case RIG_TYPO_M_CHROME_LIQUID:
            text_shadow = "  text-shadow: 0 0 8px rgba(200,220,255,0.9);\n"; break;
        case RIG_TYPO_M_FIRE_BRAND:
            text_shadow = "  text-shadow: 0 0 12px rgba(255,80,0,0.8);\n"; break;
        case RIG_TYPO_M_AURORA_TRACE:
            text_shadow = "  text-shadow: 0 0 16px rgba(80,255,180,0.7);\n"; break;
        default: break;
    }

    int len = snprintf(css, RIG_V5_SHADER_BUF,
        "/* RIGCOM rigart v5 — Typo: %s */\n"
        ".rig-typo {\n"
        "  font-family: '%s', serif;\n"
        "  font-size: %.1fpx;\n"
        "  font-weight: %.0f;\n"
        "  letter-spacing: %.3fem;\n"
        "  line-height: %.2f;\n"
        "  color: rgb(%d,%d,%d);\n"
        "%s"
        "%s"
        "  -webkit-font-smoothing: antialiased;\n"
        "  text-rendering: geometricPrecision;\n"
        "}\n",
        TYPO_NAMES[ctx->style < RIG_TYPO_M_COUNT ? ctx->style : 0],
        ctx->font_family[0] ? ctx->font_family : "serif",
        ctx->size_px,
        ctx->weight * 100.0f,
        ctx->letter_spacing,
        ctx->line_height,
        r, g, b,
        text_shadow,
        ctx->uppercase ? "  text-transform: uppercase;\n" : ""
    );

    out->css    = css;
    out->css_sz = (size_t)len;
    out->ok     = true;
    out->phi_ratio = 1.6180339887498948482;
    out->certeza   = 0.94f;
    return 0;
}

/* ─── Typo Sculpt GLSL ───────────────────────────────────────────────────── */
int rigart_v5_typo_sculpt_glsl(const RigTypoMobileCtx *ctx, RigResultV5 *out) {
    if (!ctx || !out) return -1;
    rigart_v5_init_result(out);

    char *frag = malloc(RIG_V5_SHADER_BUF);
    if (!frag) return -1;

    float depth = ctx->depth_px / 100.0f;
    float bevel = ctx->bevel_smooth;
    float mr = ctx->material_color.x, mg = ctx->material_color.y, mb = ctx->material_color.z;

    int len = snprintf(frag, RIG_V5_SHADER_BUF,
        "#version 300 es\n"
        "precision highp float;\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "uniform sampler2D u_glyph_sdf;  /* SDF del glifo */\n"
        "uniform sampler2D u_normal_map;\n"
        "uniform vec3  u_light_dir;\n"
        "uniform float u_time;\n"
        "uniform float u_depth;    /* = %.4f */\n"
        "uniform float u_bevel;    /* = %.4f */\n"
        "uniform vec3  u_material; /* = vec3(%.3f,%.3f,%.3f) */\n"
        "\n"
        "float sdf_to_alpha(float d) {\n"
        "  return clamp((d + 0.5) / (fwidth(d) * 1.4142), 0.0, 1.0);\n"
        "}\n"
        "\n"
        "void main() {\n"
        "  float sdf = texture(u_glyph_sdf, v_uv).r;\n"
        "  float alpha = sdf_to_alpha(sdf);\n"
        "  vec3 n = normalize(texture(u_normal_map, v_uv).rgb * 2.0 - 1.0);\n"
        "  /* Bisel en el filo */\n"
        "  float edge = smoothstep(0.5 - u_bevel, 0.5, sdf);\n"
        "  n.z = mix(n.z, 1.0, edge);\n"
        "  /* Iluminación Blinn-Phong con profundidad escultórica */\n"
        "  vec3  L   = normalize(u_light_dir);\n"
        "  float diff = max(dot(n, L), 0.0);\n"
        "  vec3  H   = normalize(L + vec3(0.0, 0.0, 1.0));\n"
        "  float spec = pow(max(dot(n, H), 0.0), 64.0);\n"
        "  vec3 color = u_material * (0.3 + 0.6 * diff) + vec3(spec * 0.5);\n"
        "  /* Extrusión de profundidad por sombra propia */\n"
        "  float shadow = smoothstep(0.5, 0.5 + u_depth, sdf);\n"
        "  color *= mix(0.6, 1.0, shadow);\n"
        "  fragColor = vec4(color, alpha);\n"
        "}\n",
        depth, bevel, mr, mg, mb
    );

    out->glsl_frag = frag;
    out->ok        = true;
    out->phi_ratio = 1.6180339887498948482;
    out->certeza   = 0.93f;
    (void)len;
    return 0;
}

/* ─── Keyboard ───────────────────────────────────────────────────────────── */

int rigart_v5_keyboard(const RigKeyboardCtx *ctx, RigResultV5 *out) {
    if (!ctx || !out) return -1;
    rigart_v5_init_result(out);

    char *html = malloc(RIG_V5_SHADER_BUF);
    if (!html) return -1;

    int kr = (int)(ctx->key_color.x * 255.0f);
    int kg = (int)(ctx->key_color.y * 255.0f);
    int kb = (int)(ctx->key_color.z * 255.0f);
    int tr = (int)(ctx->text_color.x * 255.0f);
    int tg = (int)(ctx->text_color.y * 255.0f);
    int tb = (int)(ctx->text_color.z * 255.0f);

    static const char *ROWS[3] = {
        "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"
    };

    char keys_html[4096] = "";
    size_t kpos = 0;
    for (int row = 0; row < 3; row++) {
        kpos += (size_t)snprintf(keys_html + kpos, sizeof(keys_html) - kpos,
            "<div class='rig-kb-row'>");
        for (const char *c = ROWS[row]; *c; c++) {
            kpos += (size_t)snprintf(keys_html + kpos, sizeof(keys_html) - kpos,
                "<button class='rig-key'>%c</button>", *c);
        }
        kpos += (size_t)snprintf(keys_html + kpos, sizeof(keys_html) - kpos,
            "</div>\n");
    }

    int len = snprintf(html, RIG_V5_SHADER_BUF,
        "<!-- RIGCOM rigart v5 — Keyboard: %s -->\n"
        "<style>\n"
        ".rig-keyboard { display:flex; flex-direction:column; gap:%.1fpx;"
        "  padding:8px; user-select:none; }\n"
        ".rig-kb-row { display:flex; justify-content:center; gap:%.1fpx; }\n"
        ".rig-key {\n"
        "  width:%.1fpx; height:%.1fpx;\n"
        "  background:rgb(%d,%d,%d);\n"
        "  color:rgb(%d,%d,%d);\n"
        "  border-radius:%.1fpx;\n"
        "  border:none; cursor:pointer;\n"
        "  transform-style:preserve-3d;\n"
        "  box-shadow: 0 %.1fpx 0 rgba(0,0,0,0.4);\n"
        "  transition: transform 80ms, box-shadow 80ms;\n"
        "}\n"
        ".rig-key:active {\n"
        "  transform: translateY(%.1fpx);\n"
        "  box-shadow: 0 0 0 rgba(0,0,0,0.4);\n"
        "}\n"
        "</style>\n"
        "<div class='rig-keyboard'>\n%s\n"
        "<div class='rig-kb-row'>"
        "<button class='rig-key' style='width:%.1fpx'>ESPACIO</button>"
        "</div>\n</div>\n",
        KB_NAMES[ctx->surface < RIG_KB_COUNT ? ctx->surface : 0],
        ctx->key_gap_px,
        ctx->key_gap_px,
        ctx->key_width_px, ctx->key_height_px,
        kr, kg, kb, tr, tg, tb,
        ctx->corner_radius,
        ctx->key_depth_px,
        ctx->key_depth_px * 0.7f,
        keys_html,
        ctx->key_width_px * 4.0f
    );

    out->html    = html;
    out->html_sz = (size_t)len;
    out->ok      = true;
    out->phi_ratio = 1.6180339887498948482;
    out->certeza   = 0.95f;
    return 0;
}

int rigart_v5_keyboard_glsl(const RigKeyboardCtx *ctx, RigResultV5 *out) {
    if (!ctx || !out) return -1;
    rigart_v5_init_result(out);

    char *frag = malloc(RIG_V5_SHADER_BUF);
    if (!frag) return -1;

    float kr = ctx->key_color.x, kg = ctx->key_color.y, kb2 = ctx->key_color.z;
    float rip = ctx->oil_ripple ? ctx->ripple_speed : 0.0f;

    int len = snprintf(frag, RIG_V5_SHADER_BUF,
        "#version 300 es\n"
        "precision highp float;\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "uniform float u_time;\n"
        "uniform vec2  u_touch;       /* posición toque [0,1] */\n"
        "uniform float u_press;       /* 0..1 */\n"
        "uniform vec3  u_key_color;   /* = vec3(%.3f,%.3f,%.3f) */\n"
        "uniform float u_ripple_spd;  /* = %.3f */\n"
        "\n"
        "void main() {\n"
        "  vec2 uv = v_uv;\n"
        "  vec3 col = u_key_color;\n"
        "  /* Specular highlight en centro de tecla */\n"
        "  float d = length(uv - 0.5);\n"
        "  float spec = smoothstep(0.45, 0.1, d);\n"
        "  col = mix(col, col + 0.35, spec);\n"
        "  /* Depresión al pulsar */\n"
        "  float press_shadow = mix(1.0, 0.78, u_press * smoothstep(0.5, 0.0, d));\n"
        "  col *= press_shadow;\n"
        "  /* Ripple de aceite (si activo) */\n"
        "  if (u_ripple_spd > 0.001) {\n"
        "    float dist = length(uv - u_touch);\n"
        "    float wave = sin(dist * 40.0 - u_time * u_ripple_spd) * 0.03;\n"
        "    wave *= smoothstep(0.6, 0.0, dist) * u_press;\n"
        "    col += vec3(wave);\n"
        "  }\n"
        "  /* Bisel de profundidad en bordes */\n"
        "  vec2 bevel = smoothstep(0.0, 0.08, uv) * smoothstep(1.0, 0.92, uv);\n"
        "  float rim = 1.0 - bevel.x * bevel.y;\n"
        "  col = mix(col, col * 0.5, rim * 0.6);\n"
        "  fragColor = vec4(col, 1.0);\n"
        "}\n",
        kr, kg, kb2, rip
    );
    (void)len;

    out->glsl_frag = frag;
    out->ok        = true;
    out->phi_ratio = 1.6180339887498948482;
    out->certeza   = 0.94f;
    return 0;
}

/* ─── Template Mobile ────────────────────────────────────────────────────── */
static const char *TPL_DESCS[RIG_TPL_COUNT] = {
    "Galería de lujo con grid áureo y fondo de mármol",
    "Lookbook de moda, scroll horizontal con parallax textil",
    "Catálogo de joyería, fondo oscuro con partículas doradas",
    "Landing de perfumería, gradiente iridiscente y tipografía esculpida",
    "Atelier de costura, panel de muestras de telas interactivas",
    "Detalle de reloj, esfera analógica con materiales de lujo",
    "Reloj ceremonial con complicaciones y texturas de ébano",
    "Esfera de relojería con agujas de oro y cristal zafiro",
    "Conserjería de hotel, interfaz minimalista en mármol blanco",
    "Reserva de spa, paleta cálida con animaciones de agua",
    "Menú gastronómico con tipografía serif y fotografía hero",
    "Bodega de vinos, fondo oscuro con etiquetas animadas",
    "Finanzas soberanas, dashboard de datos con grafos animados",
    "Subasta de arte, galería con zoom fluido y metadatos",
    "Banca privada, interfaz de alta seguridad con biometría facial",
    "Jet privado, mapa de ruta 3D con datos de vuelo en tiempo real",
    "Control de yate, dashboard náutico con sensores en tiempo real",
    "Lector de manuscritos, visor de página con tipografía histórica",
    "Museo de arte, tour virtual con renderizado de obras en 3D",
    "Teclado obsidiana con teclas de aceite y retroiluminación",
    "Teclado de seda blanca con teclas de satén y destellos suaves",
    "Teclado de hoja de oro con teclas grabadas y relieves",
    "Teclado de mármol carrara con textura procedural fotorrealista",
    "Marcador ceremonial con esfera de cristal y animación de guardar",
    "Chat soberano con burbujas de material y cifrado visible",
    "Ajustes soberanos con paneles de control en madera y metal",
    "Medición de sastrería con sensores AR y visualización 3D",
    "Galería de caras RigFace con comparativa de arquetipos"
};

int rigart_v5_template(const RigTemplateMobileCtx *ctx, RigResultV5 *out) {
    if (!ctx || !out) return -1;
    rigart_v5_init_result(out);

    RigWindowMobileCtx win_ctx;
    memset(&win_ctx, 0, sizeof(win_ctx));
    win_ctx.type             = ctx->primary_window;
    win_ctx.width_pct        = 1.0f;
    win_ctx.height_pct       = 1.0f;
    win_ctx.surface_material = ctx->primary_material;
    win_ctx.material_color   = ctx->bg_color_top;
    win_ctx.corner_radius_px = ctx->corner_radius_global;
    win_ctx.glass_opacity    = ctx->dark_mode ? 0.85f : 0.95f;
    win_ctx.backdrop_blur_px = 16.0f;
    win_ctx.elevation_dp     = 4.0f;

    RigResultV5 win_res = {0};
    if (rigart_v5_window_mobile(&win_ctx, &win_res) != 0) return -1;

    int br = (int)(ctx->bg_color_top.x    * 255.0f);
    int bg = (int)(ctx->bg_color_top.y    * 255.0f);
    int bb = (int)(ctx->bg_color_top.z    * 255.0f);
    int br2= (int)(ctx->bg_color_bottom.x * 255.0f);
    int bg2= (int)(ctx->bg_color_bottom.y * 255.0f);
    int bb2= (int)(ctx->bg_color_bottom.z * 255.0f);
    int ar = (int)(ctx->accent_color.x    * 255.0f);
    int ag = (int)(ctx->accent_color.y    * 255.0f);
    int ab = (int)(ctx->accent_color.z    * 255.0f);
    int tr = (int)(ctx->text_color.x      * 255.0f);
    int tg = (int)(ctx->text_color.y      * 255.0f);
    int tb = (int)(ctx->text_color.z      * 255.0f);

    char *html = malloc(RIG_V5_SHADER_BUF);
    if (!html) { rigart_v5_free_result(&win_res); return -1; }

    int hlen = snprintf(html, RIG_V5_SHADER_BUF,
        "<!DOCTYPE html>\n<html lang='es'>\n<head>\n"
        "<meta charset='UTF-8'>\n"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>\n"
        "<title>%s</title>\n"
        "<style>\n"
        "* { box-sizing:border-box; margin:0; padding:0; }\n"
        "html,body { width:100%%; height:100%%; }\n"
        "body {\n"
        "  background: linear-gradient(180deg, rgb(%d,%d,%d) 0%%, rgb(%d,%d,%d) 100%%);\n"
        "  color: rgb(%d,%d,%d);\n"
        "  font-family: '%s', serif;\n"
        "%s\n"  /* window CSS */
        "}\n"
        ".rig-accent { color: rgb(%d,%d,%d); }\n"
        ".rig-safe-top    { padding-top: %.1fpx; }\n"
        ".rig-safe-bottom { padding-bottom: %.1fpx; }\n"
        "</style>\n</head>\n"
        "<body class='rig-window rig-safe-top'>\n"
        "<main>\n"
        "  <!-- Template: %s -->\n"
        "  <!-- %s -->\n"
        "</main>\n"
        "</body>\n</html>\n",
        rigart_v5_template_name(ctx->template_id),
        br, bg, bb, br2, bg2, bb2,
        tr, tg, tb,
        ctx->font_display[0] ? ctx->font_display : "serif",
        win_res.css ? win_res.css : "",
        ar, ag, ab,
        ctx->safe_area_top_px,
        ctx->safe_area_bottom_px,
        rigart_v5_template_name(ctx->template_id),
        rigart_v5_template_desc(ctx->template_id)
    );

    out->html    = html;
    out->html_sz = (size_t)hlen;
    out->ok      = true;
    out->phi_ratio = 1.6180339887498948482;
    out->certeza   = 0.95f;
    rigart_v5_free_result(&win_res);
    return 0;
}

int rigart_v5_template_preview_glsl(RigTemplateMobile id, RigResultV5 *out) {
    if (!out) return -1;
    rigart_v5_init_result(out);

    char *frag = malloc(RIG_V5_SHADER_BUF);
    if (!frag) return -1;

    /* Codificación del ID como constante GLSL para preview shader */
    int len = snprintf(frag, RIG_V5_SHADER_BUF,
        "#version 300 es\n"
        "precision highp float;\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "uniform float u_time;\n"
        "uniform int   u_template_id; /* = %d : %s */\n"
        "\n"
        "/* Paletas por familia de template */\n"
        "vec3 palette(float t) {\n"
        "  /* Áurea: gold + ivory */\n"
        "  if (u_template_id < 8)\n"
        "    return mix(vec3(0.8,0.65,0.1), vec3(0.98,0.95,0.88), t);\n"
        "  /* Hostelería: warm + cream */\n"
        "  if (u_template_id < 12)\n"
        "    return mix(vec3(0.55,0.35,0.2), vec3(0.96,0.90,0.80), t);\n"
        "  /* Finanzas: dark + cyan */\n"
        "  if (u_template_id < 16)\n"
        "    return mix(vec3(0.05,0.08,0.12), vec3(0.2,0.9,0.8), t);\n"
        "  /* Teclados: material puro */\n"
        "  return mix(vec3(0.08,0.08,0.10), vec3(0.95,0.95,0.97), t);\n"
        "}\n"
        "\n"
        "void main() {\n"
        "  vec2 uv = v_uv;\n"
        "  /* Gradiente base con phi ratio */\n"
        "  float phi = 1.6180339887498948;\n"
        "  float t = uv.y + sin(uv.x * phi + u_time * 0.3) * 0.05;\n"
        "  vec3 col = palette(t);\n"
        "  /* Viñeta */\n"
        "  float vig = 1.0 - smoothstep(0.4, 0.9, length(uv - 0.5));\n"
        "  col *= vig;\n"
        "  fragColor = vec4(col, 1.0);\n"
        "}\n",
        (int)id,
        rigart_v5_template_name(id)
    );
    (void)len;

    out->glsl_frag = frag;
    out->ok        = true;
    out->phi_ratio = 1.6180339887498948482;
    out->certeza   = 0.92f;
    return 0;
}

const char *rigart_v5_template_desc(RigTemplateMobile t) {
    if (t < 0 || t >= RIG_TPL_COUNT) return "";
    return TPL_DESCS[t];
}

/* ─── Transition Mobile ──────────────────────────────────────────────────── */
static const char *TRANS_NAMES[RIG_TRANS_M_COUNT] = {
    "Silk Reveal","Oil Ripple","Fabric Unfold","Mercury Pour",
    "Gold Melt","Origami Open","Curtain Part","Swipe Momentum",
    "Pinch Expand","Crystal Shatter"
};

int rigart_v5_transition_mobile(const RigTransitionMobileCtx *ctx,
                                  RigResultV5 *out) {
    if (!ctx || !out) return -1;
    rigart_v5_init_result(out);

    char *css = malloc(RIG_V5_SHADER_BUF);
    if (!css) return -1;

    /* Curva de animación según tipo de transición */
    const char *easing;
    const char *transform;
    switch (ctx->type) {
        case RIG_TRANS_M_SILK_REVEAL:
            easing    = "cubic-bezier(0.25,0.46,0.45,0.94)";
            transform = "translateY(100%)"; break;
        case RIG_TRANS_M_OIL_RIPPLE:
            easing    = "cubic-bezier(0.0,0.0,0.2,1.0)";
            transform = "scale(0.95)"; break;
        case RIG_TRANS_M_MERCURY_POUR:
            easing    = "cubic-bezier(0.34,1.56,0.64,1)";
            transform = "translateX(-30%) scaleX(0.85)"; break;
        case RIG_TRANS_M_ORIGAMI_OPEN:
            easing    = "cubic-bezier(0.4,0.0,0.2,1)";
            transform = "rotateY(-90deg)"; break;
        case RIG_TRANS_M_SWIPE_MOMENTUM:
            easing    = "cubic-bezier(0.15,0.0,0.0,1.0)";
            transform = "translateX(100%)"; break;
        case RIG_TRANS_M_PINCH_EXPAND:
            easing    = "cubic-bezier(0.34,1.56,0.64,1)";
            transform = "scale(0.1)"; break;
        default:
            easing    = "ease-in-out";
            transform = "translateY(8px) opacity(0)"; break;
    }

    int len = snprintf(css, RIG_V5_SHADER_BUF,
        "/* RIGCOM rigart v5 — Transition: %s */\n"
        "%s { transform: %s; opacity: 0; transition: none; }\n"
        "%s.rig-enter-active,\n"
        "%s.rig-leave-active {\n"
        "  transition: transform %.3fs %s, opacity %.3fs %s;\n"
        "  will-change: transform, opacity;\n"
        "}\n"
        "%s.rig-enter-to {\n"
        "  transform: none;\n"
        "  opacity: 1;\n"
        "  spring-tension: %.2f;\n"   /* extensión propietaria */
        "  spring-friction: %.2f;\n"
        "}\n"
        "%s.rig-leave-to {\n"
        "  transform: %s;\n"
        "  opacity: 0;\n"
        "}\n",
        TRANS_NAMES[ctx->type < RIG_TRANS_M_COUNT ? ctx->type : 0],
        ctx->in_selector[0]  ? ctx->in_selector  : ".rig-page",
        transform,
        ctx->in_selector[0]  ? ctx->in_selector  : ".rig-page",
        ctx->out_selector[0] ? ctx->out_selector : ".rig-page",
        ctx->duration_s, easing,
        ctx->duration_s, easing,
        ctx->in_selector[0]  ? ctx->in_selector  : ".rig-page",
        ctx->spring_tension,
        ctx->spring_friction,
        ctx->out_selector[0] ? ctx->out_selector : ".rig-page",
        transform
    );

    out->css    = css;
    out->css_sz = (size_t)len;
    out->ok     = true;
    out->phi_ratio = 1.6180339887498948482;
    out->certeza   = 0.94f;
    return 0;
}

const char *rigart_v5_transition_mobile_name(RigTransitionMobile t) {
    if (t < 0 || t >= RIG_TRANS_M_COUNT) return "unknown";
    return TRANS_NAMES[t];
}

/* ─── Effect Mobile ──────────────────────────────────────────────────────── */
static const char *FX_NAMES[RIG_FX_M_COUNT] = {
    "Oil Surface","Silk Ripple","Fabric Wind","Mercury Pool",
    "Gold Pour","Cashmere Stroke","Wax Drip","Ink Bloom",
    "Velvet Press","Crystal Prism","Brocade Weave","Lace Trace"
};

int rigart_v5_effect_mobile(const RigEffectMobileCtx *ctx, RigResultV5 *out) {
    if (!ctx || !out) return -1;
    rigart_v5_init_result(out);

    char *frag = malloc(RIG_V5_SHADER_BUF);
    if (!frag) return -1;

    float pr = ctx->color_primary.x, pg = ctx->color_primary.y, pb = ctx->color_primary.z;
    float sr = ctx->color_secondary.x, sg = ctx->color_secondary.y, sb = ctx->color_secondary.z;

    int len = snprintf(frag, RIG_V5_SHADER_BUF,
        "#version 300 es\n"
        "precision highp float;\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "uniform float u_time;\n"
        "uniform float u_intensity;   /* = %.3f */\n"
        "uniform float u_scale;       /* = %.3f */\n"
        "uniform float u_speed;       /* = %.3f */\n"
        "uniform float u_viscosity;   /* = %.3f */\n"
        "uniform float u_surface_ten; /* = %.3f */\n"
        "uniform vec2  u_touch;\n"
        "uniform float u_touch_r;     /* = %.1f */\n"
        "uniform vec3  u_color_a;     /* = vec3(%.3f,%.3f,%.3f) */\n"
        "uniform vec3  u_color_b;     /* = vec3(%.3f,%.3f,%.3f) */\n"
        "\n"
        "float hash(vec2 p) {\n"
        "  return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.545);\n"
        "}\n"
        "\n"
        "vec2 fluid_vel(vec2 p, float t) {\n"
        "  float v = u_viscosity * 0.1;\n"
        "  return vec2(\n"
        "    sin(p.y * 3.14 * u_scale + t * u_speed) * v,\n"
        "    cos(p.x * 2.71 * u_scale - t * u_speed * 0.7) * v\n"
        "  );\n"
        "}\n"
        "\n"
        "void main() {\n"
        "  vec2 uv = v_uv;\n"
        "  float t = u_time;\n"
        "  vec2 vel = fluid_vel(uv, t);\n"
        "  vec2 uv2 = uv + vel;\n"
        "  float st = u_surface_ten;\n"
        "  float n1 = hash(floor(uv2 * 8.0)) * st;\n"
        "  float n2 = hash(floor(uv2 * 16.0 + t)) * st * 0.5;\n"
        "  float pattern = n1 + n2;\n"
        "  float td = length(uv - u_touch);\n"
        "  float touch_fx = smoothstep(u_touch_r / 512.0, 0.0, td);\n"
        "  pattern += touch_fx * u_intensity;\n"
        "  vec3 col = mix(u_color_a, u_color_b, clamp(pattern, 0.0, 1.0));\n"
        "  col *= u_intensity;\n"
        "  fragColor = vec4(col, u_intensity);\n"
        "}\n",
        ctx->intensity, ctx->scale, ctx->speed, ctx->viscosity,
        ctx->surface_tension, ctx->touch_radius_px,
        pr, pg, pb, sr, sg, sb
    );
    (void)len;

    out->glsl_frag = frag;
    out->ok        = true;
    out->phi_ratio = 1.6180339887498948482;
    out->certeza   = 0.93f;
    return 0;
}

int rigart_v5_effect_mobile_compose(const RigEffectMobileCtx *fx,
                                      int count, RigResultV5 *out) {
    if (!fx || count <= 0 || !out) return -1;
    rigart_v5_init_result(out);

    /* Composición: aplica efectos en secuencia como capas de render */
    char *frag = malloc(RIG_V5_SHADER_BUF);
    if (!frag) return -1;

    char layers[4096] = "";
    size_t lpos = 0;
    for (int i = 0; i < count && i < 8; i++) {
        lpos += (size_t)snprintf(layers + lpos, sizeof(layers) - lpos,
            "  /* Capa %d: %s  intensidad=%.2f */\n"
            "  col = mix(col, col + vec3(%.3f,%.3f,%.3f) * %.3f,"
            " %.3f * (1.0 - float(%d) * 0.1));\n",
            i, FX_NAMES[fx[i].type < RIG_FX_M_COUNT ? fx[i].type : 0],
            fx[i].intensity,
            fx[i].color_primary.x, fx[i].color_primary.y, fx[i].color_primary.z,
            fx[i].intensity,
            fx[i].intensity, i
        );
    }

    int len = snprintf(frag, RIG_V5_SHADER_BUF,
        "#version 300 es\n"
        "precision highp float;\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "uniform float u_time;\n"
        "/* Composición de %d efectos */\n"
        "void main() {\n"
        "  vec3 col = vec3(0.0);\n"
        "%s"
        "  fragColor = vec4(col, 1.0);\n"
        "}\n",
        count, layers
    );
    (void)len;

    out->glsl_frag = frag;
    out->ok        = true;
    out->phi_ratio = 1.6180339887498948482;
    out->certeza   = (float)(1.0 - count * 0.02);
    return 0;
}

const char *rigart_v5_effect_name(RigEffectMobile e) {
    if (e < 0 || e >= RIG_FX_M_COUNT) return "unknown";
    return FX_NAMES[e];
}
