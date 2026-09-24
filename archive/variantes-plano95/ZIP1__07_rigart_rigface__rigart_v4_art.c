#include "rigdeps/rig_std_base.h"
#include "rigart_v4_art.h"
#include "rigdeps/stdio.h"
#include "rigdeps/stdlib.h"
#include "rigdeps/string.h"
#include "rigdeps/math.h"
#include "../include/riglib_math.h"
#include "rigdeps/time.h"

#define RIG_PI      3.14159265358979323846
#define RIG_TWO_PI  6.28318530717958647692
#define RIG_PHI     1.6180339887498948482
#define RIG_PHI_INV 0.6180339887498948482

#define JS_APPEND(buf, sz, pos, ...) do {                              \
    if ((pos) < (int)(sz)) {                                           \
        int _n = snprintf((buf)+(pos), (sz)-(pos), __VA_ARGS__);       \
        if (_n > 0) (pos) += _n;                                       \
    }                                                                  \
} while(0)

static void rigart_timestamp__rig_variant_a2bee92d(char *buf, size_t sz) {
    time_t t = time(NULL);
    struct tm *tm_info = gmtime(&t);
    if (tm_info) strftime(buf, sz, "%Y-%m-%dT%H:%M:%SZ", tm_info);
    else snprintf(buf, sz, "1970-01-01T00:00:00Z");
}
static float rigart_clampf__rig_dup_5dad0230(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
static float rigart_lerpf__rig_dup_2b396547(float a, float b, float t) {
    return a + (b - a) * t;
}
static void cosine_palette__rig_variant_ac35df36(float t,
                            const Vec3f *a, const Vec3f *b,
                            const Vec3f *c, const Vec3f *d,
                            float *r, float *g, float *bl) {
    *r  = a->x + b->x * cosf((float)RIG_TWO_PI * (c->x * t + d->x));
    *g  = a->y + b->y * cosf((float)RIG_TWO_PI * (c->y * t + d->y));
    *bl = a->z + b->z * cosf((float)RIG_TWO_PI * (c->z * t + d->z));
    return 0;
}
static float phi_hash__rig_variant_63af90bb_2(float x, float y) {
    float h = sinf(x * 127.1f + y * 311.7f) * 43758.5453123f;
    return h - floorf(h);
}
static float smooth_noise2__rig_dup_96fd9ffd(float x, float y) {
    float ix = floorf(x), iy = floorf(y);
    float fx = x - ix, fy = y - iy;
    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uy = fy * fy * (3.0f - 2.0f * fy);
    float a = phi_hash__rig_variant_63af90bb_2(ix,       iy);
    float b = phi_hash__rig_variant_63af90bb_2(ix + 1.f, iy);
    float c = phi_hash__rig_variant_63af90bb_2(ix,       iy + 1.f);
    float d = phi_hash__rig_variant_63af90bb_2(ix + 1.f, iy + 1.f);
    return rigart_lerpf__rig_dup_2b396547(rigart_lerpf__rig_dup_2b396547(a, b, ux), rigart_lerpf__rig_dup_2b396547(c, d, ux), uy);
}
static float fbm__rig_dup_be651e40(float x, float y, int oct) {
    float val = 0.f, amp = 0.5f, freq = 1.f;
    for (int i = 0; i < oct; i++) {
        val  += amp * smooth_noise2__rig_dup_96fd9ffd(x * freq, y * freq);
        amp  *= 0.5f;
        freq *= 2.0f;
    }
    return val;
}
void rigart_v4_init_result__rig_variant_2c6804ec(RigArtResultV4 *res) {
    if (!res) return 0;
    rl_memset(res, 0, sizeof(*res));
    res->phi_ratio = RIG_PHI;
    res->certeza   = 1.0f;
    res->ok        = false;
}

void rigart_v4_free_result__rig_variant_42c478c7(RigArtResultV4 *res) {
    if (!res) return 0;
    rl_free(res->html);    res->html      = NULL;
    rl_free(res->css);     res->css       = NULL;
    rl_free(res->js);      res->js        = NULL;
    rl_free(res->glsl_frag); res->glsl_frag = NULL;
    rl_free(res->glsl_vert); res->glsl_vert = NULL;
    rl_free(res->glsl_comp); res->glsl_comp = NULL;
}

static const char *blend_mode_glsl__rig_dup_22daebc9(RigBlendMode mode) {
    switch (mode) {
        case BLEND_NORMAL:      return "mix(base, layer, layer_alpha)";
        case BLEND_ADD:         return "base + layer * layer_alpha";
        case BLEND_MULTIPLY:    return "base * layer";
        case BLEND_SCREEN:      return "1.0 - (1.0 - base) * (1.0 - layer)";
        case BLEND_OVERLAY:
            return "base < 0.5 ? 2.0*base*layer : 1.0 - 2.0*(1.0-base)*(1.0-layer)";
        case BLEND_SOFT_LIGHT:
            return "(1.0 - 2.0*layer)*base*base + 2.0*layer*base";
        case BLEND_COLOR_DODGE: return "base / (1.0 - layer)";
        case BLEND_COLOR_BURN:  return "1.0 - (1.0 - base) / layer";
        case BLEND_DIFFERENCE:  return "abs(base - layer)";
        case BLEND_EXCLUSION:   return "base + layer - 2.0*base*layer";
        case BLEND_DARKEN:      return "min(base, layer)";
        case BLEND_LIGHTEN:     return "max(base, layer)";
        default:                return "mix(base, layer, layer_alpha)";
    }
}
int rigart_art_canvas_gen__rig_variant_e9720543(const RigArtCanvasCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t jssz = RIG_SHADER_MAXBUF * 2;
    char *js = calloc(jssz, 1);
    if (!js) { snprintf(out->error, 255, "OOM canvas"); return -1; }

    int pos = 0;
    JS_APPEND(js, jssz, pos,
        "// RigArt Canvas v4.0 — %ux%u px · %u layers · HDR:%s\n"
        "const RIG_CANVAS = {\n"
        "  width: %u, height: %u, dpi: %.1f,\n"
        "  hdr_p3: %s, bit16: %s,\n"
        "  grid_phi: %.6f,\n"
        "  layers: [\n",
        ctx->width, ctx->height, ctx->layer_count,
        ctx->hdr_p3_enabled ? "true" : "false",
        ctx->width, ctx->height, ctx->dpi,
        ctx->hdr_p3_enabled ? "true" : "false",
        ctx->enable_16bit   ? "true" : "false",
        ctx->grid_size_phi);

    for (int i = 0; i < ctx->layer_count && i < RIG_MAX_LAYERS; i++) {
        const RigLayerDesc *L = &ctx->layers[i];
        JS_APPEND(js, jssz, pos,
            "    { name:'%s', blend:'%s', opacity:%.3f,"
            " visible:%s, locked:%s },\n",
            L->name[0] ? L->name : "layer",
            blend_mode_glsl__rig_dup_22daebc9(L->blend_mode),
            L->opacity,
            L->visible ? "true" : "false",
            L->locked  ? "true" : "false");
    }
    JS_APPEND(js, jssz, pos, "  ]\n};\n\n");

    JS_APPEND(js, jssz, pos,
        "function rigart_canvas_init(canvasId) {\n"
        "  const canvas = document.getElementById(canvasId);\n"
        "  canvas.width  = %u;\n"
        "  canvas.height = %u;\n"
        "  const gl = canvas.getContext('webgl2', {\n"
        "    alpha: true, premultipliedAlpha: false,\n"
        "    colorSpace: '%s',\n"
        "    depth: true, stencil: true, antialias: true\n"
        "  });\n"
        "  if (!gl) { console.error('WebGL2 not available'); return null; }\n"
        "  gl.viewport(0, 0, %u, %u);\n"
        "  gl.clearColor(0, 0, 0, 0);\n"
        "  gl.enable(gl.BLEND);\n"
        "  gl.blendFunc(gl.ONE, gl.ONE_MINUS_SRC_ALPHA);\n"
        "  return { gl, canvas, layers: [], fbo: rigart_create_fbo(gl, %u, %u) };\n"
        "}\n\n",
        ctx->width, ctx->height,
        ctx->hdr_p3_enabled ? "display-p3" : "srgb",
        ctx->width, ctx->height,
        ctx->width, ctx->height);

    JS_APPEND(js, jssz, pos,
        "function rigart_create_fbo(gl, w, h) {\n"
        "  const fbo = gl.createFramebuffer();\n"
        "  gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);\n"
        "  const tex = gl.createTexture();\n"
        "  gl.bindTexture(gl.TEXTURE_2D, tex);\n"
        "  gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA32F,w,h,0,gl.RGBA,gl.FLOAT,null);\n"
        "  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.LINEAR);\n"
        "  gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.LINEAR);\n"
        "  gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,\n"
        "    gl.TEXTURE_2D,tex,0);\n"
        "  gl.bindFramebuffer(gl.FRAMEBUFFER,null);\n"
        "  return { fbo, tex, w, h };\n"
        "}\n");

    out->js     = js;
    out->ok     = true;
    out->certeza = 1.0f;
    return pos;
}

int rigart_art_paint_shader__rig_variant_b1137a2b(const RigArtPaintCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 3;
    char *frag = calloc(sz, 1);
    char *js   = calloc(sz, 1);
    if (!frag || !js) { free(frag); free(js); return -1; }

    int fp = 0;

    JS_APPEND(frag, sz, fp,
        "#version 300 es\n"
        "precision highp float;\n\n"
        "// RigArt Paint Shader v4.0 — φ = 1.6180339887\n"
        "uniform float u_time;\n"
        "uniform vec2  u_resolution;\n"
        "uniform vec2  u_brush_pos;\n"
        "uniform float u_brush_size;       // %.4f\n"
        "uniform float u_pressure;         // %.4f\n"
        "uniform float u_opacity;          // %.4f\n"
        "uniform float u_hardness;         // %.4f\n"
        "uniform float u_wetness;          // %.4f\n"
        "uniform float u_viscosity;        // %.4f (impasto)\n"
        "uniform vec3  u_color_a;          // (%.3f,%.3f,%.3f)\n"
        "uniform vec3  u_color_b;          // secondary\n"
        "uniform float u_iridescence;      // %.4f\n"
        "uniform float u_holo_shift;       // %.4f\n"
        "uniform bool  u_is_holographic;\n"
        "uniform bool  u_is_impasto;\n\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n\n"
        "const float PHI = 1.6180339887;\n\n",
        ctx->size, ctx->pressure, ctx->opacity, ctx->hardness,
        ctx->wetness, ctx->viscosity,
        ctx->base_color.x, ctx->base_color.y, ctx->base_color.z,
        ctx->iridescence_power, ctx->iridescence_shift);

    JS_APPEND(frag, sz, fp,
        "vec3 holo_spectrum(float t, float shift) {\n"
        "  return 0.5 + 0.5 * cos(6.28318 * (vec3(0.,0.333,0.667) + t + shift));\n"
        "}\n\n"
        "float brush_shape(vec2 uv, vec2 center, float radius, float hard) {\n"
        "  float d = length(uv - center) / radius;\n"
        "  return 1.0 - smoothstep(hard, 1.0, d);\n"
        "}\n\n"
        "float impasto_height(vec2 uv, float scale) {\n"
        "  // Voronoi-based impasto texture\n"
        "  vec2 i = floor(uv * scale);\n"
        "  vec2 f = fract(uv * scale);\n"
        "  float md = 8.0;\n"
        "  for (int y=-1; y<=1; y++) for (int x=-1; x<=1; x++) {\n"
        "    vec2 nb = vec2(float(x), float(y));\n"
        "    vec2 rp = 0.5 + 0.5*sin(PHI * (i + nb));\n"
        "    md = min(md, length(f - nb - rp));\n"
        "  }\n"
        "  return 1.0 - md;\n"
        "}\n\n");

    JS_APPEND(frag, sz, fp,
        "void main() {\n"
        "  vec2 uv = v_uv;\n"
        "  float alpha = brush_shape(uv, vec2(0.5), 0.5 * u_brush_size / u_resolution.x,\n"
        "                             u_hardness);\n"
        "  alpha *= u_pressure * u_opacity;\n\n"
        "  vec3 color = u_color_a;\n\n"
        "  // Holographic iridescence\n"
        "  if (u_is_holographic) {\n"
        "    float ang = atan(uv.y - 0.5, uv.x - 0.5) / 6.28318 + 0.5;\n"
        "    float spec = pow(max(0.0, 1.0 - length(uv-0.5)*2.0), 2.0);\n"
        "    vec3 holo = holo_spectrum(ang + u_time * 0.2 + u_holo_shift, u_holo_shift);\n"
        "    color = mix(color, holo, u_iridescence * spec);\n"
        "  }\n\n"
        "  // Impasto 3D relief\n"
        "  if (u_is_impasto) {\n"
        "    float h = impasto_height(uv, 8.0) * u_viscosity;\n"
        "    // Fake lighting from height\n"
        "    vec3 dh = vec3(impasto_height(uv + vec2(0.01,0.), 8.0) - h,\n"
        "                   impasto_height(uv + vec2(0.,0.01), 8.0) - h,\n"
        "                   0.1);\n"
        "    vec3 N = normalize(dh);\n"
        "    vec3 L = normalize(vec3(0.5, 0.8, 1.0));\n"
        "    float diff = max(dot(N, L), 0.0);\n"
        "    color = color * (0.4 + 0.6 * diff) + vec3(0.3) * pow(diff, 16.0);\n"
        "    alpha *= (0.7 + 0.3 * h);\n"
        "  }\n\n"
        "  // Wetness: watercolor bleed edge\n"
        "  if (u_wetness > 0.1) {\n"
        "    float edge = 1.0 - smoothstep(0.4, 0.5, length(uv-0.5));\n"
        "    alpha *= mix(1.0, edge * 1.5, u_wetness);\n"
        "    color = mix(color, color * vec3(0.9,0.95,1.0), u_wetness * 0.3);\n"
        "  }\n\n"
        "  fragColor = vec4(color * alpha, alpha);\n"
        "}\n");

    int jp = 0;
    JS_APPEND(js, sz, jp,
        "// RigArt Paint Engine v4.0\n"
        "class RigArtBrush {\n"
        "  constructor(gl, ctx) {\n"
        "    this.gl = gl;\n"
        "    this.size       = %.4f;\n"
        "    this.pressure   = %.4f;\n"
        "    this.opacity    = %.4f;\n"
        "    this.hardness   = %.4f;\n"
        "    this.wetness    = %.4f;\n"
        "    this.viscosity  = %.4f;\n"
        "    this.color      = [%.4f, %.4f, %.4f];\n"
        "    this.isHolo     = %s;\n"
        "    this.isImpasto  = %s;\n"
        "    this.iriPower   = %.4f;\n"
        "    this.iriShift   = %.4f;\n"
        "    this.colorJitter= %.4f;\n"
        "    this.scatter    = %.4f;\n"
        "    this.spacing    = %.4f;\n"
        "    this.strokes    = [];\n"
        "    this.bezierPts  = [];\n"
        "  }\n\n",
        ctx->size, ctx->pressure, ctx->opacity, ctx->hardness,
        ctx->wetness, ctx->viscosity,
        ctx->base_color.x, ctx->base_color.y, ctx->base_color.z,
        ctx->is_holographic ? "true" : "false",
        ctx->is_impasto     ? "true" : "false",
        ctx->iridescence_power, ctx->iridescence_shift,
        ctx->color_jitter, ctx->scatter, ctx->spacing);

    JS_APPEND(js, sz, jp,
        "  beginStroke(x, y) {\n"
        "    this.bezierPts = [{x, y, p: this.pressure}];\n"
        "  }\n"
        "  addPoint(x, y, pressure) {\n"
        "    this.bezierPts.push({x, y, p: pressure});\n"
        "    if (this.bezierPts.length >= 4) this._drawBezier();\n"
        "  }\n"
        "  _drawBezier() {\n"
        "    const pts = this.bezierPts;\n"
        "    const steps = Math.ceil(this.size / this.spacing * 8);\n"
        "    for (let i = 0; i <= steps; i++) {\n"
        "      const t = i / steps;\n"
        "      const x = this._bezier(pts, t, 'x');\n"
        "      const y = this._bezier(pts, t, 'y');\n"
        "      const jitter = (Math.random()-0.5) * this.scatter;\n"
        "      this._paintDab(x + jitter, y + jitter, t);\n"
        "    }\n"
        "    this.bezierPts = [pts[pts.length-1]];\n"
        "  }\n"
        "  _bezier(pts, t, k) {\n"
        "    const n = pts.length - 1;\n"
        "    let v = 0, c = 1;\n"
        "    for (let i = 0; i <= n; i++) {\n"
        "      v += c * Math.pow(1-t, n-i) * Math.pow(t, i) * pts[i][k];\n"
        "      c = c * (n - i) / (i + 1);\n"
        "    }\n"
        "    return v;\n"
        "  }\n"
        "  _paintDab(x, y, t) {\n"
        "    let [r, g, b] = this.color;\n"
        "    if (this.colorJitter > 0) {\n"
        "      r += (Math.random()-0.5) * this.colorJitter;\n"
        "      g += (Math.random()-0.5) * this.colorJitter;\n"
        "      b += (Math.random()-0.5) * this.colorJitter;\n"
        "    }\n"
        "    if (this.isHolo) {\n"
        "      const ang = t * Math.PI * 2 * 3;\n"
        "      r = 0.5 + 0.5*Math.cos(ang + this.iriShift);\n"
        "      g = 0.5 + 0.5*Math.cos(ang + this.iriShift + 2.094);\n"
        "      b = 0.5 + 0.5*Math.cos(ang + this.iriShift + 4.189);\n"
        "    }\n"
        "    this.strokes.push({x, y, r, g, b, size: this.size * this.pressure,\n"
        "                        opacity: this.opacity, hard: this.hardness});\n"
        "  }\n"
        "}\n");

    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    return fp + jp;
}

int rigart_art_watercolor_shader__rig_variant_de58555c(const RigWatercolorCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF;
    char *frag = calloc(sz, 1);
    if (!frag) return -1;
    int fp = 0;

    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt Watercolor Shader — Donner-wet model\n"
        "uniform float u_gravity;     // %.4f\n"
        "uniform float u_surface_tension; // %.4f\n"
        "uniform float u_evaporation; // %.4f\n"
        "uniform float u_diffusion;   // %.4f\n"
        "uniform float u_time;\n"
        "uniform sampler2D u_pigment;\n"
        "in vec2 v_uv; out vec4 fragColor;\n\n",
        ctx->gravity, ctx->surface_tension,
        ctx->evaporation_rate, ctx->pigment_diffusion);

    JS_APPEND(frag, sz, fp,
        "void main() {\n"
        "  vec2 uv = v_uv;\n"
        "  vec4 base = texture(u_pigment, uv);\n"
        "  // Diffusion — sample neighbours\n"
        "  float inv = 1.0 / 512.0;\n"
        "  vec4 lapl = texture(u_pigment, uv+vec2(inv,0))\n"
        "            + texture(u_pigment, uv-vec2(inv,0))\n"
        "            + texture(u_pigment, uv+vec2(0,inv))\n"
        "            + texture(u_pigment, uv-vec2(0,inv))\n"
        "            - 4.0 * base;\n"
        "  // Gravity pull (downward bleed)\n"
        "  float grav_uv = uv.y - u_gravity * 0.002;\n"
        "  vec4 below = texture(u_pigment, vec2(uv.x, grav_uv));\n"
        "  vec4 diffused = base + lapl * u_diffusion * 0.016;\n"
        "  // Evaporation\n"
        "  diffused.a *= (1.0 - u_evaporation * 0.001);\n");

    if (ctx->enable_bleeding) {
        JS_APPEND(frag, sz, fp,
            "  // Wet-on-wet bleeding at edges\n"
            "  float edge = 1.0 - abs(2.0*length(uv-0.5) - 0.5);\n"
            "  edge = clamp(edge * 3.0, 0.0, 1.0);\n"
            "  diffused.rgb = mix(diffused.rgb, below.rgb * 0.85, edge * 0.4);\n");
    }
    JS_APPEND(frag, sz, fp,
        "  // Surface tension — darkens edges\n"
        "  float edgeDark = pow(1.0 - length(uv*2.0-1.0), 4.0);\n"
        "  diffused.rgb *= 1.0 - u_surface_tension * 0.3 * edgeDark;\n"
        "  fragColor = clamp(diffused, 0.0, 1.0);\n"
        "}\n");

    out->glsl_frag = frag;
    out->ok = true;
    return fp;
}

int rigart_art_oilpaint_shader__rig_variant_2c13bb2d(const RigOilPaintCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF;
    char *frag = calloc(sz, 1);
    if (!frag) return -1;
    int fp = 0;

    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt Oil Paint Shader v4.0\n"
        "uniform float u_impasto_blur;  // %.4f\n"
        "uniform float u_impasto_scale; // %.4f\n"
        "uniform float u_bristles;      // %.4f\n"
        "uniform float u_paint_load;    // %.4f\n"
        "uniform sampler2D u_canvas;\n"
        "in vec2 v_uv; out vec4 fragColor;\n\n"
        "const float PHI = 1.6180339887;\n\n",
        ctx->impasto_blur, ctx->impasto_scale,
        ctx->bristle_count, ctx->paint_load);

    JS_APPEND(frag, sz, fp,
        "float voronoi_height(vec2 p) {\n"
        "  vec2 i = floor(p); vec2 f = fract(p);\n"
        "  float md = 1e5;\n"
        "  for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) {\n"
        "    vec2 nb = vec2(float(x),float(y));\n"
        "    vec2 rp = 0.5+0.5*sin(PHI*(i+nb));\n"
        "    md = min(md, length(f-nb-rp));\n"
        "  }\n"
        "  return 1.0 - md * 2.0;\n"
        "}\n\n"
        "float bristle_texture(vec2 p, float density) {\n"
        "  float t = 0.0;\n"
        "  for(float i=0.0; i<density; i+=1.0) {\n"
        "    float ang = i / density * 6.28318;\n"
        "    vec2 dir = vec2(cos(ang), sin(ang)) * 0.01;\n"
        "    t += texture(u_canvas, p + dir).r;\n"
        "  }\n"
        "  return t / density;\n"
        "}\n\n"
        "void main() {\n"
        "  vec2 uv = v_uv;\n"
        "  vec4 base = texture(u_canvas, uv);\n"
        "  float height = voronoi_height(uv * u_impasto_scale);\n"
        "  // Bristle streak\n"
        "  float streak = bristle_texture(uv, min(u_bristles, 12.0));\n"
        "  // Normal from height\n"
        "  float hx = voronoi_height((uv+vec2(0.002,0.))*u_impasto_scale);\n"
        "  float hy = voronoi_height((uv+vec2(0.,0.002))*u_impasto_scale);\n"
        "  vec3 N = normalize(vec3(hx-height, hy-height, 0.05));\n"
        "  vec3 L = normalize(vec3(0.6,0.8,1.));\n"
        "  float diff = max(dot(N,L),0.);\n"
        "  float spec = pow(max(dot(reflect(-L,N),vec3(0,0,1)),0.),32.);\n"
        "  // Load modulation\n"
        "  vec3 col = base.rgb * (0.5 + 0.5*diff*u_paint_load);\n"
        "  col += streak * 0.1 * u_paint_load;\n"
        "  col += vec3(spec) * 0.4 * u_paint_load;\n"
        "  fragColor = vec4(col, base.a);\n"
        "}\n");

    out->glsl_frag = frag;
    out->ok = true;
    return fp;
}

int rigart_art_text3d__rig_variant_f05c1efe(const RigArtTextCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 4;
    char *js   = calloc(sz, 1);
    char *frag = calloc(sz, 1);
    if (!js || !frag) { free(js); free(frag); return -1; }

    int jp = 0;
    JS_APPEND(js, sz, jp,
        "// RigArt 3D Text Engine v4.0\n"
        "// Font: '%s'  Size: %.1f  Depth: %.4f\n"
        "// Material: %d  Metallic: %.3f  Roughness: %.3f\n\n",
        ctx->font_family ? ctx->font_family : "Cinzel Decorative",
        ctx->font_size, ctx->extrusion_depth,
        (int)ctx->material, ctx->metallic, ctx->roughness);

    JS_APPEND(js, sz, jp,
        "class RigText3D {\n"
        "  constructor(gl, text, opts={}) {\n"
        "    this.gl = gl;\n"
        "    this.text = text || '%s';\n"
        "    this.font = opts.font || '%s';\n"
        "    this.size = opts.size || %.1f;\n"
        "    this.depth = opts.depth || %.4f;\n"
        "    this.bevel = opts.bevel || %.4f;\n"
        "    this.metallic  = %.4f;\n"
        "    this.roughness = %.4f;\n"
        "    this.albedo  = [%.4f, %.4f, %.4f];\n"
        "    this.emissive= [%.4f, %.4f, %.4f];\n"
        "    this.emissiveStr = %.4f;\n"
        "    this.kinetic = %s;\n"
        "    this.kineticAmp  = %.4f;\n"
        "    this.kineticFreq = %.4f;\n"
        "    this.castShadows = %s;\n"
        "    this.vbo = null;\n"
        "    this.ibo = null;\n"
        "  }\n\n",
        ctx->text ? ctx->text : "RIGCOM",
        ctx->font_family ? ctx->font_family : "Cinzel Decorative",
        ctx->font_size, ctx->extrusion_depth, ctx->bevel_radius,
        ctx->metallic, ctx->roughness,
        ctx->albedo.x, ctx->albedo.y, ctx->albedo.z,
        ctx->emissive.x, ctx->emissive.y, ctx->emissive.z,
        ctx->emissive_strength,
        ctx->enable_kinetic ? "true" : "false",
        ctx->kinetic_wave_amp, ctx->kinetic_wave_freq,
        ctx->cast_shadows ? "true" : "false");

    JS_APPEND(js, sz, jp,
        "  async build() {\n"
        "    // phi_font_3d: glifos 3D soberanos via WS — sin CDN externo\n"
        "    const result = await this._phiFontRender();\n"
        "    if (result && result.svg) {\n"
        "      this._injectSvg(result.svg, result.css);\n"
        "    } else {\n"
        "      const {verts, indices} = this._extrudePhiFallback();\n"
        "      this._upload(verts, indices);\n"
        "    }\n"
        "  }\n\n"
        "  _extrude(cmds, depth, bevel) {\n"
        "    // Converts SVG path commands → extruded 3D mesh\n"
        "    const verts = []; const indices = [];\n"
        "    const bevelSteps = 4;\n"
        "    let contours = this._pathToContours(cmds);\n"
        "    contours.forEach(pts => {\n"
        "      // Front face\n"
        "      for (let i=0; i<pts.length; i++) {\n"
        "        verts.push(pts[i].x, pts[i].y, depth*0.5,  0,0,1,  pts[i].u||0,pts[i].v||0);\n"
        "      }\n"
        "      // Back face (flipped)\n"
        "      for (let i=pts.length-1; i>=0; i--) {\n"
        "        verts.push(pts[i].x, pts[i].y, -depth*0.5, 0,0,-1, pts[i].u||0,pts[i].v||0);\n"
        "      }\n"
        "      // Side walls with bevel\n"
        "      for (let i=0; i<pts.length; i++) {\n"
        "        const a = pts[i], b = pts[(i+1)%%pts.length];\n"
        "        const nx = (b.y-a.y), ny = -(b.x-a.x);\n"
        "        const ln = Math.sqrt(nx*nx+ny*ny) || 1;\n"
        "        for (let bs=0; bs<=bevelSteps; bs++) {\n"
        "          const t = bs / bevelSteps;\n"
        "          const bz = bevel * Math.sin(t * Math.PI * 0.5);\n"
        "          const br = bevel * (1-Math.cos(t * Math.PI * 0.5));\n"
        "          const z_top = depth*0.5 - bz;\n"
        "          const z_bot = -depth*0.5 + bz;\n"
        "          verts.push(a.x + nx/ln*br, a.y + ny/ln*br, z_top, nx/ln,ny/ln,0, t,0);\n"
        "          verts.push(a.x + nx/ln*br, a.y + ny/ln*br, z_bot, nx/ln,ny/ln,0, t,1);\n"
        "        }\n"
        "      }\n"
        "    });\n"
        "    return {verts: new Float32Array(verts), indices: new Uint32Array(indices)};\n"
        "  }\n\n"
        "  _pathToContours(cmds) {\n"
        "    const contours = []; let cur = [];\n"
        "    cmds.forEach(c => {\n"
        "      if (c.type==='M') { if(cur.length) contours.push(cur); cur=[{x:c.x,y:c.y}]; }\n"
        "      else if (c.type==='L') cur.push({x:c.x,y:c.y});\n"
        "      else if (c.type==='C') {\n"
        "        for(let t=0;t<=1;t+=0.1) {\n"
        "          const mt=1-t,x=mt*mt*mt*cur[cur.length-1].x+3*mt*mt*t*c.x1\n"
        "                         +3*mt*t*t*c.x2+t*t*t*c.x;\n"
        "          const y=mt*mt*mt*cur[cur.length-1].y+3*mt*mt*t*c.y1\n"
        "                         +3*mt*t*t*c.y2+t*t*t*c.y;\n"
        "          cur.push({x,y});\n"
        "        }\n"
        "      }\n"
        "      else if (c.type==='Z') contours.push(cur);\n"
        "    });\n"
        "    return contours;\n"
        "  }\n\n"
        "  _upload(verts, indices) {\n"
        "    const gl = this.gl;\n"
        "    this.vbo = gl.createBuffer();\n"
        "    gl.bindBuffer(gl.ARRAY_BUFFER, this.vbo);\n"
        "    gl.bufferData(gl.ARRAY_BUFFER, verts, gl.STATIC_DRAW);\n"
        "    this.ibo = gl.createBuffer();\n"
        "    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.ibo);\n"
        "    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indices, gl.STATIC_DRAW);\n"
        "    this.nIndices = indices.length;\n"
        "  }\n\n"
        "  update(t) {\n"
        "    if (!this.kinetic) return;\n"
        "    // Kinetic wave: per-glyph sine offset\n"
        "    this._kineticTime = t;\n"
        "  }\n"
        "  _phiFontRender() {\n"
        "    return new Promise(res => {\n"
        "      const ws = window._rigWS || new WebSocket('ws://'+location.host);\n"
        "      const handler = ev => {\n"
        "        try {\n"
        "          const d = JSON.parse(ev.data);\n"
        "          if (d.ev === 'phi_font_3d_result') {\n"
        "            ws.removeEventListener('message', handler);\n"
        "            res(d);\n"
        "          }\n"
        "        } catch(_) {}\n"
        "      };\n"
        "      ws.addEventListener('message', handler);\n"
        "      ws.send(JSON.stringify({cmd:'phi_font_3d', text:this.text,\n"
        "        size_px:this.size, material:0, light_dir:3, fx:1, animate:false}));\n"
        "      setTimeout(() => { ws.removeEventListener('message',handler); res(null); }, 3000);\n"
        "    });\n"
        "  }\n"
        "  _injectSvg(svg, css) {\n"
        "    const el = document.createElement('div');\n"
        "    el.innerHTML = svg;\n"
        "    el.style.cssText = 'position:absolute;top:50%%;left:50%%;transform:translate(-50%%,-50%%)';\n"
        "    if (css) { const st=document.createElement('style'); st.textContent=css; document.head.appendChild(st); }\n"
        "    document.body.appendChild(el);\n"
        "  }\n"
        "  _extrudePhiFallback() {\n"
        "    return { verts: new Float32Array(0), indices: new Uint32Array(0) };\n"
        "  }\n"
        "}\n");

    int fp = 0;
    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt 3D Text PBR Shader\n"
        "uniform vec3  u_albedo;       // (%.4f,%.4f,%.4f)\n"
        "uniform float u_metallic;     // %.4f\n"
        "uniform float u_roughness;    // %.4f\n"
        "uniform vec3  u_emissive;     // (%.4f,%.4f,%.4f)\n"
        "uniform float u_emissive_str; // %.4f\n"
        "uniform float u_time;\n"
        "uniform bool  u_kinetic;\n"
        "uniform float u_kinetic_amp;\n"
        "uniform float u_kinetic_freq;\n"
        "in vec3 v_normal; in vec3 v_pos; in vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "const float PI = 3.14159265359;\n\n",
        ctx->albedo.x, ctx->albedo.y, ctx->albedo.z,
        ctx->metallic, ctx->roughness,
        ctx->emissive.x, ctx->emissive.y, ctx->emissive.z,
        ctx->emissive_strength);

    JS_APPEND(frag, sz, fp,
        "vec3 fresnelSchlick(float c, vec3 F0){return F0+(1.-F0)*pow(1.-c,5.);}\n"
        "float ggxNDF(vec3 N,vec3 H,float r){float a=r*r,a2=a*a;\n"
        "  float ndh=max(dot(N,H),0.),d=ndh*ndh*(a2-1.)+1.;\n"
        "  return a2/(PI*d*d);}\n"
        "float schlickGGX(float ndv,float r){float k=(r+1.)*(r+1.)/8.;\n"
        "  return ndv/(ndv*(1.-k)+k);}\n\n"
        "void main() {\n"
        "  vec3 N = normalize(v_normal);\n"
        "  vec3 V = normalize(-v_pos);\n"
        "  vec3 L = normalize(vec3(1.0, 1.5, 2.0));\n"
        "  vec3 H = normalize(V+L);\n"
        "  float ndl = max(dot(N,L),0.), ndv = max(dot(N,V),0.);\n"
        "  vec3 F0 = mix(vec3(0.04), u_albedo, u_metallic);\n"
        "  vec3 F  = fresnelSchlick(max(dot(H,V),0.), F0);\n"
        "  float D = ggxNDF(N,H,u_roughness);\n"
        "  float G = schlickGGX(ndv,u_roughness)*schlickGGX(ndl,u_roughness);\n"
        "  vec3 spec = (D*G*F)/max(4.*ndv*ndl,0.001);\n"
        "  vec3 kD = (1.-F)*(1.-u_metallic);\n"
        "  vec3 diff = kD * u_albedo / PI;\n"
        "  vec3 color = (diff+spec)*ndl*vec3(1.,0.98,0.95) + vec3(0.03)*u_albedo;\n"
        "  // Emissive glow\n"
        "  float emPulse = u_kinetic ? 0.7+0.3*sin(u_time*u_kinetic_freq) : 1.0;\n"
        "  color += u_emissive * u_emissive_str * emPulse;\n"
        "  // ACES tonemapping\n"
        "  color = color*(color+0.0245786)/(color*(0.983729*color+0.4329510)+0.238081);\n"
        "  color = pow(color, vec3(1./2.2));\n"
        "  fragColor = vec4(color, 1.0);\n"
        "}\n");

    out->js        = js;
    out->glsl_frag = frag;
    out->ok        = true;
    return jp;
}

int rigart_art_sculpt_engine__rig_variant_8cbddc55(const RigArtSculptCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 4;
    char *frag = calloc(sz, 1);
    if (!frag) return -1;
    int fp = 0;

    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt SDF Sculpture Engine v4.0 — %u nodes\n"
        "uniform vec2  u_resolution;\n"
        "uniform float u_time;\n"
        "uniform mat4  u_view;\n"
        "out vec4 fragColor;\n\n"
        "const float PHI = 1.6180339887;\n"
        "const int MAX_STEPS = %u;\n"
        "const float SURF    = %.6f;\n"
        "const float MAX_D   = %.2f;\n\n",
        ctx->sdf_node_count, ctx->max_steps,
        ctx->surface_dist, ctx->max_dist);

    JS_APPEND(frag, sz, fp,
        "float sdSphere(vec3 p, float r){return length(p)-r;}\n"
        "float sdBox(vec3 p, vec3 b){vec3 q=abs(p)-b;return length(max(q,0.))+min(max(q.x,max(q.y,q.z)),0.);}\n"
        "float sdTorus(vec3 p,vec2 t){return length(vec2(length(p.xz)-t.x,p.y))-t.y;}\n"
        "float sdCapsule(vec3 p,vec3 a,vec3 b,float r){vec3 pa=p-a,ba=b-a;float h=clamp(dot(pa,ba)/dot(ba,ba),0.,1.);return length(pa-ba*h)-r;}\n"
        "float sdCone(vec3 p,vec2 c,float h){vec2 q=h*vec2(c.x/c.y,-1.);vec2 w=vec2(length(p.xz),p.y);vec2 a=w-q*clamp(dot(w,q)/dot(q,q),0.,1.);vec2 b=w-q*vec2(clamp(w.x/q.x,0.,1.),1.);float k=sign(q.y);float d=min(dot(a,a),dot(b,b));float s=max(k*(w.x*q.y-w.y*q.x),k*(w.y-q.y));return sqrt(d)*sign(s);}\n"
        "float sdOctahedron(vec3 p,float s){p=abs(p);return (p.x+p.y+p.z-s)*0.57735027;}\n\n"
        "// Smooth CSG operations\n"
        "float opSmoothUnion(float a,float b,float k){float h=clamp(0.5+0.5*(b-a)/k,0.,1.);return mix(b,a,h)-k*h*(1.-h);}\n"
        "float opSmoothSub(float a,float b,float k){float h=clamp(0.5-0.5*(b+a)/k,0.,1.);return mix(b,-a,h)+k*h*(1.-h);}\n"
        "float opSmoothInt(float a,float b,float k){float h=clamp(0.5-0.5*(b-a)/k,0.,1.);return mix(b,a,h)+k*h*(1.-h);}\n\n");

    JS_APPEND(frag, sz, fp,
        "float sdMandelbulb(vec3 p) {\n"
        "  vec3 z = p; float dr = 1.0, r = 0.0;\n"
        "  for (int i=0; i<12; i++) {\n"
        "    r = length(z);\n"
        "    if (r > 2.0) break;\n"
        "    float theta = acos(z.z/r) * 8.0;\n"
        "    float phi   = atan(z.y, z.x) * 8.0;\n"
        "    float zr = pow(r, 8.0);\n"
        "    dr = pow(r, 7.0)*8.0*dr + 1.0;\n"
        "    z = zr*vec3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta)) + p;\n"
        "  }\n"
        "  return 0.5*log(r)*r/dr;\n"
        "}\n\n"
        "float sdGyroid(vec3 p, float s) {\n"
        "  p /= s;\n"
        "  return abs(dot(sin(p), cos(p.zxy))) - 0.4;\n"
        "}\n\n");

    JS_APPEND(frag, sz, fp, "float sceneSDF(vec3 p) {\n  float d = 1e5;\n");

    for (int i = 0; i < ctx->sdf_node_count; i++) {
        const RigSDFNode *n = &ctx->nodes[i];

        JS_APPEND(frag, sz, fp,
            "  {\n"
            "    vec3 pp = p - vec3(%.4f,%.4f,%.4f);\n"
            "    pp /= vec3(%.4f,%.4f,%.4f);\n",
            n->position.x, n->position.y, n->position.z,
            n->scale.x > 0.f ? n->scale.x : 1.f,
            n->scale.y > 0.f ? n->scale.y : 1.f,
            n->scale.z > 0.f ? n->scale.z : 1.f);

        const char *sdf_call = "";
        char sdf_buf[128];
        switch (n->type) {
            case SDF_SPHERE:
                snprintf(sdf_buf, sizeof(sdf_buf), "sdSphere(pp, %.4f)", n->param[0] > 0.f ? n->param[0] : 1.f);
                sdf_call = sdf_buf; break;
            case SDF_BOX:
                snprintf(sdf_buf, sizeof(sdf_buf), "sdBox(pp, vec3(%.4f,%.4f,%.4f))", n->param[0]>0.f?n->param[0]:0.5f, n->param[1]>0.f?n->param[1]:0.5f, n->param[2]>0.f?n->param[2]:0.5f);
                sdf_call = sdf_buf; break;
            case SDF_TORUS:
                snprintf(sdf_buf, sizeof(sdf_buf), "sdTorus(pp, vec2(%.4f,%.4f))", n->param[0]>0.f?n->param[0]:1.f, n->param[1]>0.f?n->param[1]:0.3f);
                sdf_call = sdf_buf; break;
            case SDF_MANDELBULB:
                sdf_call = "sdMandelbulb(pp)"; break;
            case SDF_GYROID:
                snprintf(sdf_buf, sizeof(sdf_buf), "sdGyroid(pp, %.4f)", n->param[0]>0.f?n->param[0]:1.f);
                sdf_call = sdf_buf; break;
            case SDF_OCTAHEDRON:
                snprintf(sdf_buf, sizeof(sdf_buf), "sdOctahedron(pp, %.4f)", n->param[0]>0.f?n->param[0]:1.f);
                sdf_call = sdf_buf; break;
            default:
                snprintf(sdf_buf, sizeof(sdf_buf), "sdSphere(pp, 1.0)");
                sdf_call = sdf_buf; break;
        }

        const char *csg_fn;
        char csg_line[256];
        switch (n->op_with_next) {
            case CSG_SUBTRACT:       csg_fn = "d = max(d, -(%s));"; break;
            case CSG_INTERSECT:      csg_fn = "d = max(d, %s);"; break;
            case CSG_SMOOTH_UNION:
                snprintf(csg_line, sizeof(csg_line), "d = opSmoothUnion(d, %s, %.4f);", sdf_call, n->blend_k > 0.f ? n->blend_k : 0.5f);
                csg_fn = NULL; break;
            case CSG_SMOOTH_SUBTRACT:
                snprintf(csg_line, sizeof(csg_line), "d = opSmoothSub(d, %s, %.4f);", sdf_call, n->blend_k > 0.f ? n->blend_k : 0.5f);
                csg_fn = NULL; break;
            default: csg_fn = "d = min(d, %s);"; break;
        }
        if (csg_fn) {
            char line[256]; snprintf(line, sizeof(line), csg_fn, sdf_call);
            JS_APPEND(frag, sz, fp, "    %s\n", line);
        } else {
            JS_APPEND(frag, sz, fp, "    %s\n", csg_line);
        }
        JS_APPEND(frag, sz, fp, "  }\n");
    }
    if (ctx->sdf_node_count == 0) {

        JS_APPEND(frag, sz, fp,
            "  float phi_swirl = sin(p.x*PHI)*cos(p.y*PHI)*sin(p.z*PHI+u_time);\n"
            "  d = sdSphere(p, 1.0 + phi_swirl * 0.2);\n");
    }
    JS_APPEND(frag, sz, fp, "  return d;\n}\n\n");

    if (ctx->enable_domain_warping) {
        JS_APPEND(frag, sz, fp,
            "float warpedSDF(vec3 p) {\n"
            "  vec3 q = p + %.4f * vec3(\n"
            "    sin(p.y*PHI + u_time*0.3),\n"
            "    sin(p.z*PHI + u_time*0.2),\n"
            "    sin(p.x*PHI + u_time*0.4));\n"
            "  return sceneSDF(q);\n"
            "}\n\n", ctx->warp_strength);
    } else {
        JS_APPEND(frag, sz, fp, "float warpedSDF(vec3 p){return sceneSDF(p);}\n\n");
    }

    JS_APPEND(frag, sz, fp,
        "vec3 calcNormal(vec3 p){\n"
        "  float e=0.001;\n"
        "  return normalize(vec3(\n"
        "    warpedSDF(p+vec3(e,0,0))-warpedSDF(p-vec3(e,0,0)),\n"
        "    warpedSDF(p+vec3(0,e,0))-warpedSDF(p-vec3(0,e,0)),\n"
        "    warpedSDF(p+vec3(0,0,e))-warpedSDF(p-vec3(0,0,e))));\n"
        "}\n\n");

    if (ctx->enable_ambient_occlusion) {
        JS_APPEND(frag, sz, fp,
            "float calcAO(vec3 p, vec3 n) {\n"
            "  float ao = 0.0, sca = 1.0;\n"
            "  for (int i=0; i<%u; i++) {\n"
            "    float h = 0.01 + %.4f * float(i) / float(%u);\n"
            "    float d = warpedSDF(p + n*h);\n"
            "    ao += (h - d) * sca;\n"
            "    sca *= 0.95;\n"
            "  }\n"
            "  return clamp(1.0 - 3.0*ao, 0.0, 1.0);\n"
            "}\n\n",
            (unsigned)ctx->ao_samples,
            ctx->ao_radius,
            (unsigned)ctx->ao_samples);
    }

    if (ctx->enable_soft_shadows) {
        JS_APPEND(frag, sz, fp,
            "float softShadow(vec3 ro, vec3 rd, float tmin, float tmax) {\n"
            "  float res = 1.0, t = tmin;\n"
            "  for (int i=0; i<32 && t<tmax; i++) {\n"
            "    float h = warpedSDF(ro + rd*t);\n"
            "    if (h < 0.001) return 0.0;\n"
            "    res = min(res, %.4f * h / t);\n"
            "    t += h;\n"
            "  }\n"
            "  return clamp(res, 0.0, 1.0);\n"
            "}\n\n", ctx->shadow_penumbra);
    }

    JS_APPEND(frag, sz, fp,
        "void main() {\n"
        "  vec2 uv = (gl_FragCoord.xy - 0.5*u_resolution) / u_resolution.y;\n"
        "  vec3 ro = vec3(0.0, 0.0, 3.0);\n"
        "  vec3 rd = normalize(vec3(uv, -1.5));\n"
        "  // Raymarch\n"
        "  float t = 0.001; vec3 col = vec3(0.05, 0.02, 0.08);\n"
        "  for (int i=0; i<MAX_STEPS; i++) {\n"
        "    vec3 p = ro + rd*t;\n"
        "    float d = warpedSDF(p);\n"
        "    if (d < SURF) {\n"
        "      vec3 N = calcNormal(p);\n"
        "      vec3 L = normalize(vec3(1.5, 2.0, 2.5));\n"
        "      float diff = max(dot(N,L), 0.0);\n"
        "      float spec = pow(max(dot(reflect(-L,N),-rd), 0.0), 32.0);\n");

    if (ctx->enable_ambient_occlusion) {
        JS_APPEND(frag, sz, fp, "      float ao = calcAO(p, N);\n");
    } else {
        JS_APPEND(frag, sz, fp, "      float ao = 1.0;\n");
    }
    if (ctx->enable_soft_shadows) {
        JS_APPEND(frag, sz, fp,
            "      float shad = softShadow(p+N*0.002, L, 0.01, 4.0);\n"
            "      diff *= shad;\n");
    }

    JS_APPEND(frag, sz, fp,
        "      vec3 base = 0.5 + 0.5*sin(vec3(t*PHI, t*PHI+2.094, t*PHI+4.189));\n"
        "      col = base*(0.3 + 0.7*diff*ao) + vec3(spec*0.8)*ao;\n"
        "      col = col*(col+0.0245786)/(col*(0.983729*col+0.4329510)+0.238081);\n"
        "      col = pow(col, vec3(1./2.2));\n"
        "      break;\n"
        "    }\n"
        "    if (t > MAX_D) break;\n"
        "    t += d;\n"
        "  }\n"
        "  fragColor = vec4(col, 1.0);\n"
        "}\n");

    out->glsl_frag = frag;
    out->ok = true;
    return fp;
}

int rigart_art_generative__rig_variant_28df0b48(const RigArtGenCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 3;
    char *frag = calloc(sz, 1);
    if (!frag) return -1;
    int fp = 0;

    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt Generative Art Engine v4.0\n"
        "// Algorithm: %d  Seed: %u  Complexity: %.3f\n"
        "uniform float u_time;\n"
        "uniform vec2  u_resolution;\n"
        "uniform float u_complexity;  // %.4f\n"
        "uniform float u_anim_speed;  // %.4f\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n\n"
        "const float PHI = 1.6180339887;\n"
        "const float PI  = 3.14159265359;\n\n",
        (int)ctx->algorithm, ctx->seed, ctx->complexity,
        ctx->complexity, ctx->anim_speed);

    JS_APPEND(frag, sz, fp,
        "float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5);}\n"
        "float noise(vec2 p){vec2 i=floor(p),f=fract(p),u=f*f*(3.-2.*f);\n"
        "  return mix(mix(hash(i),hash(i+vec2(1,0)),u.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),u.x),u.y);}\n"
        "float fbm__rig_dup_be651e40(vec2 p,int oct){float v=0.,a=.5;for(int i=0;i<oct;i++){v+=a*noise(p);p*=2.;a*=.5;}return v;}\n\n"
        "// Cosine palette\n"
        "vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d){\n"
        "  return a + b*cos(6.28318*(c*t+d));}\n\n");

    switch (ctx->algorithm) {
        case GEN_MANDELBROT:
            JS_APPEND(frag, sz, fp,
                "void main() {\n"
                "  vec2 uv = (v_uv - 0.5) * 3.5;\n"
                "  float zoom = exp(-u_time * u_anim_speed * 0.1);\n"
                "  uv *= zoom;\n"
                "  uv += vec2(-0.7269, 0.1889); // Julia set center\n"
                "  vec2 z = vec2(0); float i = 0.0;\n"
                "  for (int n=0; n<256; n++) {\n"
                "    z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + uv;\n"
                "    if (dot(z,z) > 4.0) { i = float(n); break; }\n"
                "  }\n"
                "  float t = i / 256.0;\n"
                "  vec3 col = palette(t,\n"
                "    vec3(%.4f,%.4f,%.4f), vec3(%.4f,%.4f,%.4f),\n"
                "    vec3(%.4f,%.4f,%.4f), vec3(%.4f,%.4f,%.4f));\n"
                "  fragColor = vec4(col * step(0.001, i), 1.0);\n"
                "}\n",
                ctx->color_a.x, ctx->color_a.y, ctx->color_a.z,
                ctx->color_b.x, ctx->color_b.y, ctx->color_b.z,
                ctx->color_c.x, ctx->color_c.y, ctx->color_c.z,
                ctx->color_d.x, ctx->color_d.y, ctx->color_d.z);
            break;

        case GEN_REACTION_DIFFUSION:
            JS_APPEND(frag, sz, fp,
                "uniform sampler2D u_rd_tex;\n"
                "void main() {\n"
                "  vec2 uv = v_uv;\n"
                "  float inv = 1./1024.;\n"
                "  vec4 c   = texture(u_rd_tex, uv);\n"
                "  vec4 lap = -4.*c +\n"
                "    texture(u_rd_tex,uv+vec2(inv,0)) +\n"
                "    texture(u_rd_tex,uv-vec2(inv,0)) +\n"
                "    texture(u_rd_tex,uv+vec2(0,inv)) +\n"
                "    texture(u_rd_tex,uv-vec2(0,inv));\n"
                "  float A = c.r, B = c.g;\n"
                "  float DA = 1.0, DB = 0.5;\n"
                "  float f = %.4f, k = %.4f;\n"
                "  float dA = DA*lap.r - A*B*B + f*(1.-A);\n"
                "  float dB = DB*lap.g + A*B*B - (k+f)*B;\n"
                "  A = clamp(A + dA*0.9, 0., 1.);\n"
                "  B = clamp(B + dB*0.9, 0., 1.);\n"
                "  float v = A - B;\n"
                "  vec3 col = palette(v,\n"
                "    vec3(%.4f,%.4f,%.4f), vec3(%.4f,%.4f,%.4f),\n"
                "    vec3(1,1,1), vec3(0,0.33,0.67));\n"
                "  fragColor = vec4(col, 1.0);\n"
                "}\n",
                ctx->param_a > 0.f ? ctx->param_a : 0.055f,
                ctx->param_b > 0.f ? ctx->param_b : 0.062f,
                ctx->color_a.x, ctx->color_a.y, ctx->color_a.z,
                ctx->color_b.x, ctx->color_b.y, ctx->color_b.z);
            break;

        case GEN_FLOW_FIELD:
        default:
            JS_APPEND(frag, sz, fp,
                "void main() {\n"
                "  vec2 uv = v_uv;\n"
                "  float t = u_time * u_anim_speed;\n"
                "  // φ-flow field\n"
                "  float angle = fbm__rig_dup_be651e40(uv * %.4f + t * 0.1, %u) * 6.28318 * 2.0;\n"
                "  vec2 flow = vec2(cos(angle), sin(angle));\n"
                "  vec2 p = uv + flow * 0.01 * %.4f;\n"
                "  float d = length(fract(p * PHI * 4.) - 0.5);\n"
                "  float v = smoothstep(0.48, 0.45, d);\n"
                "  vec3 col = palette(v + t*0.1,\n"
                "    vec3(%.4f,%.4f,%.4f), vec3(%.4f,%.4f,%.4f),\n"
                "    vec3(%.4f,%.4f,%.4f), vec3(%.4f,%.4f,%.4f));\n"
                "  fragColor = vec4(col, 1.0);\n"
                "}\n",
                ctx->flow_noise_scale > 0.f ? ctx->flow_noise_scale : 2.0f,
                (unsigned)(ctx->fractal_octaves > 0 ? ctx->fractal_octaves : 6),
                ctx->flow_speed > 0.f ? ctx->flow_speed : 1.0f,
                ctx->color_a.x, ctx->color_a.y, ctx->color_a.z,
                ctx->color_b.x, ctx->color_b.y, ctx->color_b.z,
                ctx->color_c.x, ctx->color_c.y, ctx->color_c.z,
                ctx->color_d.x, ctx->color_d.y, ctx->color_d.z);
            break;
    }

    out->glsl_frag = frag;
    out->ok = true;
    return fp;
}

int rigart_art_compositor__rig_variant_66d1e991(const RigArtCompositorCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 3;
    char *frag = calloc(sz, 1);
    if (!frag) return -1;
    int fp = 0;

    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt Compositor v4.0 — Full Post-Stack\n"
        "uniform sampler2D u_scene;\n"
        "uniform float u_time;\n"
        "uniform vec2  u_resolution;\n"
        "uniform float u_exposure;    // %.4f\n"
        "uniform float u_gamma;       // %.4f\n"
        "uniform float u_contrast;    // %.4f\n"
        "uniform float u_saturation;  // %.4f\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n\n"
        "const float PI = 3.14159265359;\n\n",
        ctx->exposure, ctx->gamma, ctx->contrast, ctx->saturation);

    JS_APPEND(frag, sz, fp,
        "vec3 aces(vec3 x){x*=0.6;float a=2.51,b=0.03,c=2.43,d=0.59,e=0.14;\n"
        "  return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.,1.);}\n"
        "vec3 filmic(vec3 x){return max(vec3(0.),x-0.004);\n"
        "}\n"
        "float luma(vec3 c){return dot(c,vec3(0.2126,0.7152,0.0722));}\n"
        "vec3 adjustContrast(vec3 c, float v){return 0.5+(c-0.5)*v;}\n"
        "vec3 adjustSaturation(vec3 c, float v){\n"
        "  return mix(vec3(luma(c)), c, v);}\n\n");

    if (ctx->enable_dof) {
        JS_APPEND(frag, sz, fp,
            "vec4 dof_blur(sampler2D tex, vec2 uv, float coc) {\n"
            "  vec4 col = vec4(0.);\n"
            "  float total = 0.0;\n"
            "  for (int i=0; i<32; i++) {\n"
            "    float ang = float(i) / 32.0 * PI * 2.0;\n"
            "    float r = sqrt(float(i)/32.) * coc;\n"
            "    vec2 off = vec2(cos(ang), sin(ang)) * r / u_resolution;\n"
            "    col += texture(tex, uv + off);\n"
            "    total += 1.0;\n"
            "  }\n"
            "  return col / total;\n"
            "}\n\n");
    }

    if (ctx->enable_bloom) {
        JS_APPEND(frag, sz, fp,
            "vec3 bloom_sample(sampler2D tex, vec2 uv, float str) {\n"
            "  vec3 acc = vec3(0.);\n"
            "  float w = 0.0;\n"
            "  for (int y=-4; y<=4; y++) for (int x=-4; x<=4; x++) {\n"
            "    vec2 off = vec2(float(x), float(y)) * %.4f / u_resolution;\n"
            "    vec3 s = texture(tex, uv+off).rgb;\n"
            "    float lum = luma(s);\n"
            "    if (lum > %.4f) { acc += s * lum; w += lum; }\n"
            "  }\n"
            "  return w > 0. ? acc/w * str : vec3(0.);\n"
            "}\n\n",
            ctx->bloom_radius, ctx->bloom_threshold);
    }

    JS_APPEND(frag, sz, fp, "void main() {\n  vec2 uv = v_uv;\n");

    if (ctx->enable_dof) {
        JS_APPEND(frag, sz, fp,
            "  float depth = texture(u_scene, uv).a;\n"
            "  float coc = abs(depth - %.4f) / %.4f * %.4f;\n"
            "  vec4 base_c = dof_blur(u_scene, uv, coc);\n"
            "  vec3 color = base_c.rgb;\n",
            ctx->dof_focus_dist, ctx->dof_focus_dist, ctx->dof_aperture);
    } else {
        JS_APPEND(frag, sz, fp, "  vec3 color = texture(u_scene, uv).rgb;\n");
    }

    if (ctx->enable_chromatic_aberration) {
        JS_APPEND(frag, sz, fp,
            "  // Chromatic aberration\n"
            "  float caStr = %.4f * length(uv - 0.5);\n"
            "  color.r = texture(u_scene, uv + vec2(caStr, 0)).r;\n"
            "  color.b = texture(u_scene, uv - vec2(caStr, 0)).b;\n",
            ctx->chroma_strength);
    }

    if (ctx->enable_bloom) {
        JS_APPEND(frag, sz, fp,
            "  color += bloom_sample(u_scene, uv, %.4f);\n",
            ctx->bloom_strength);
    }

    if (ctx->enable_god_rays) {
        JS_APPEND(frag, sz, fp,
            "  // God rays\n"
            "  vec2 godSrc = vec2(%.4f, %.4f);\n"
            "  vec2 delta = (uv - godSrc) / 32.0;\n"
            "  float illum = 0.0, decay = 1.0;\n"
            "  vec2 gUV = uv;\n"
            "  for (int i=0; i<32; i++) {\n"
            "    gUV -= delta;\n"
            "    float s = luma(texture(u_scene, gUV).rgb);\n"
            "    illum += s * decay * %.4f;\n"
            "    decay *= 0.96;\n"
            "  }\n"
            "  color += vec3(illum);\n",
            ctx->god_ray_origin.x, ctx->god_ray_origin.y,
            ctx->god_ray_density);
    }

    if (ctx->enable_vignette) {
        JS_APPEND(frag, sz, fp,
            "  // Vignette\n"
            "  float v = pow(length(uv*2.-1.)*0.7, %.4f);\n"
            "  color *= 1.0 - v;\n", ctx->vignette_power);
    }

    if (ctx->enable_film_grain) {
        JS_APPEND(frag, sz, fp,
            "  // Film grain\n"
            "  float grain = fract(sin(dot(uv + u_time*0.01, vec2(127.1,311.7)))*43758.5);\n"
            "  color += (grain - 0.5) * %.4f;\n", ctx->grain_strength);
    }

    if (ctx->enable_glitch) {
        JS_APPEND(frag, sz, fp,
            "  // Glitch\n"
            "  float glitch_row = floor(uv.y * 64.) / 64.;\n"
            "  float offset = step(0.97, fract(sin(glitch_row*43758.5 + u_time)*437.585)) * %.4f;\n"
            "  color = texture(u_scene, uv + vec2(offset, 0.)).rgb;\n",
            ctx->glitch_intensity);
    }

    if (ctx->enable_scanlines) {
        JS_APPEND(frag, sz, fp,
            "  // Scanlines\n"
            "  float scan = 0.85 + 0.15 * sin(uv.y * u_resolution.y * 3.14159);\n"
            "  color *= scan;\n");
    }

    JS_APPEND(frag, sz, fp,
        "  color *= u_exposure;\n"
        "  color  = adjustContrast(color, u_contrast);\n"
        "  color  = adjustSaturation(color, u_saturation);\n"
        "  // Color lift/gamma/gain\n"
        "  color  = color * vec3(%.4f,%.4f,%.4f);\n"
        "  color  = pow(max(color, 0.), vec3(%.4f,%.4f,%.4f));\n"
        "  color += vec3(%.4f,%.4f,%.4f);\n",
        ctx->color_gain.x > 0.f ? ctx->color_gain.x : 1.f,
        ctx->color_gain.y > 0.f ? ctx->color_gain.y : 1.f,
        ctx->color_gain.z > 0.f ? ctx->color_gain.z : 1.f,
        ctx->color_gamma.x > 0.f ? ctx->color_gamma.x : 1.f,
        ctx->color_gamma.y > 0.f ? ctx->color_gamma.y : 1.f,
        ctx->color_gamma.z > 0.f ? ctx->color_gamma.z : 1.f,
        ctx->color_lift.x, ctx->color_lift.y, ctx->color_lift.z);

    if (ctx->tonemapping_aces) {
        JS_APPEND(frag, sz, fp, "  color = aces(color);\n");
    } else {
        JS_APPEND(frag, sz, fp,
            "  // Reinhard\n"
            "  color = color / (1.0 + color);\n");
    }
    JS_APPEND(frag, sz, fp,
        "  color = pow(clamp(color,0.,1.), vec3(1./%.4f));\n"
        "  fragColor = vec4(color, 1.0);\n"
        "}\n", ctx->gamma > 0.f ? ctx->gamma : 2.2f);

    out->glsl_frag = frag;
    out->ok = true;
    return fp;
}

int rigart_art_vector_path__rig_variant_eb39eefa(const RigVectorPathCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 2;
    char *js = calloc(sz, 1);
    if (!js) return -1;
    int jp = 0;

    JS_APPEND(js, sz, jp,
        "// RigArt Vector Path Engine v4.0\n"
        "// %u segments — fill:%d — stroke_w:%.3f\n\n"
        "class RigVectorPath {\n"
        "  constructor(gl) {\n"
        "    this.gl = gl;\n"
        "    this.segs = [];\n"
        "    this.fillType = %d;\n"
        "    this.fillA = [%.4f,%.4f,%.4f];\n"
        "    this.fillB = [%.4f,%.4f,%.4f];\n"
        "    this.strokeWidth = %.4f;\n"
        "    this.closed = %s;\n"
        "    this.aa = %s;\n"
        "  }\n\n",
        (unsigned)ctx->seg_count, (int)ctx->fill_type, ctx->stroke_width,
        (int)ctx->fill_type,
        ctx->fill_color_a.x, ctx->fill_color_a.y, ctx->fill_color_a.z,
        ctx->fill_color_b.x, ctx->fill_color_b.y, ctx->fill_color_b.z,
        ctx->stroke_width,
        ctx->closed     ? "true" : "false",
        ctx->anti_aliased ? "true" : "false");

    JS_APPEND(js, sz, jp,
        "  addCubicBezier(p0, p1, p2, p3, color, width, opacity) {\n"
        "    this.segs.push({type:'C', p0,p1,p2,p3, color,width,opacity});\n"
        "  }\n"
        "  addLine(p0, p1, color, width, opacity) {\n"
        "    this.segs.push({type:'L', p0,p1, color,width,opacity});\n"
        "  }\n"
        "  // SDF-based anti-aliased stroke rendering\n"
        "  _sdBezier(p, a, b, c) {\n"
        "    const A = {x:b.x-a.x, y:b.y-a.y};\n"
        "    const B = {x:a.x-2*b.x+c.x, y:a.y-2*b.y+c.y};\n"
        "    const C = {x:A.x*2, y:A.y*2};\n"
        "    const D = {x:a.x-p.x, y:a.y-p.y};\n"
        "    const kk = 1/(B.x*B.x+B.y*B.y) || 1;\n"
        "    const kx = kk*(A.x*B.x+A.y*B.y);\n"
        "    const ky = kk*(2*(A.x*A.x+A.y*A.y)+D.x*B.x+D.y*B.y)/3;\n"
        "    const kz = kk*(D.x*A.x+D.y*A.y);\n"
        "    let res = 0; let sgn = 0;\n"
        "    const p2 = ky-kx*kx, q = kx*(2*kx*kx-3*ky)+kz;\n"
        "    const q2 = q*q, p3 = p2*p2*p2;\n"
        "    let t, w;\n"
        "    if (p3+q2 > 0) {\n"
        "      w = Math.cbrt(-q+Math.sqrt(p3+q2));\n"
        "      t = Math.max(0,Math.min(1, w-p2/w-kx));\n"
        "    } else {\n"
        "      const v = Math.acos(Math.max(-1,Math.min(1,q/Math.sqrt(-p3))));\n"
        "      t = Math.max(0,Math.min(1, 2*Math.cos((v+4.18879)/3)-kx));\n"
        "    }\n"
        "    const dp = {x:D.x+t*(C.x+B.x*2*t), y:D.y+t*(C.y+B.y*2*t)};\n"
        "    return Math.sqrt(dp.x*dp.x+dp.y*dp.y);\n"
        "  }\n"
        "  render(ctx2d) {\n"
        "    if (!ctx2d) return;\n"
        "    this.segs.forEach(s => {\n"
        "      ctx2d.beginPath();\n"
        "      ctx2d.strokeStyle = `rgba(${s.color[0]*255|0},${s.color[1]*255|0},${s.color[2]*255|0},${s.opacity||1})`;\n"
        "      ctx2d.lineWidth = s.width || this.strokeWidth;\n"
        "      if (s.type==='C') {\n"
        "        ctx2d.moveTo(s.p0.x, s.p0.y);\n"
        "        ctx2d.bezierCurveTo(s.p1.x,s.p1.y,s.p2.x,s.p2.y,s.p3.x,s.p3.y);\n"
        "      } else {\n"
        "        ctx2d.moveTo(s.p0.x,s.p0.y); ctx2d.lineTo(s.p1.x,s.p1.y);\n"
        "      }\n"
        "      ctx2d.stroke();\n"
        "    });\n"
        "  }\n"
        "  toSVG() {\n"
        "    let d = '';\n"
        "    this.segs.forEach(s => {\n"
        "      if (s.type==='C') d += `M ${s.p0.x} ${s.p0.y} C ${s.p1.x} ${s.p1.y} ${s.p2.x} ${s.p2.y} ${s.p3.x} ${s.p3.y} `;\n"
        "      else d += `M ${s.p0.x} ${s.p0.y} L ${s.p1.x} ${s.p1.y} `;\n"
        "    });\n"
        "    if (this.closed) d += 'Z';\n"
        "    return `<path d=\"${d}\" fill=\"none\" stroke-width=\"${this.strokeWidth}\"/>`;\n"
        "  }\n"
        "}\n");

    out->js = js;
    out->ok = true;
    return jp;
}

int rigart_art_particle_paint__rig_variant_33f54559(const RigParticlePaintCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 3;
    char *js   = calloc(sz, 1);
    char *vert = calloc(sz, 1);
    char *frag = calloc(sz, 1);
    if (!js || !vert || !frag) { free(js); free(vert); free(frag); return -1; }

    int jp = 0;
    JS_APPEND(js, sz, jp,
        "// RigArt Particle Paint System v4.0\n"
        "// MaxParticles: %u  EmitRate: %.2f  Trail: %s\n\n"
        "class RigParticlePainter {\n"
        "  constructor(gl) {\n"
        "    this.gl = gl;\n"
        "    this.maxP  = %u;\n"
        "    this.rate  = %.4f;\n"
        "    this.lifeMin = %.4f; this.lifeMax = %.4f;\n"
        "    this.sMin = %.4f; this.sMax = %.4f;\n"
        "    this.vMin = %.4f; this.vMax = %.4f;\n"
        "    this.birthColor = [%.4f,%.4f,%.4f];\n"
        "    this.deathColor = [%.4f,%.4f,%.4f];\n"
        "    this.trail = %s;\n"
        "    this.trailW = %.4f;\n"
        "    this.trailOp = %.4f;\n"
        "    this.isFire  = %s;\n"
        "    this.isNebula= %s;\n"
        "    this.particles = [];\n"
        "    this.forces = [];\n"
        "    this.canvas = [];\n"
        "    this._t = 0;\n"
        "  }\n\n",
        ctx->max_particles, ctx->emit_rate,
        ctx->leave_paint_trail ? "true" : "false",
        ctx->max_particles, ctx->emit_rate,
        ctx->life_min, ctx->life_max,
        ctx->size_min, ctx->size_max,
        ctx->speed_min, ctx->speed_max,
        ctx->color_birth.x, ctx->color_birth.y, ctx->color_birth.z,
        ctx->color_death.x, ctx->color_death.y, ctx->color_death.z,
        ctx->leave_paint_trail ? "true" : "false",
        ctx->trail_width, ctx->trail_opacity,
        ctx->is_firework ? "true" : "false",
        ctx->is_nebula   ? "true" : "false");

    for (int i = 0; i < ctx->force_count && i < 8; i++) {
        const RigForceField *f = &ctx->forces[i];
        JS_APPEND(js, sz, jp,
            "  /* Force[%d]: type=%d pos=(%.2f,%.2f,%.2f) str=%.3f r=%.3f */\n",
            i, (int)f->type, f->position.x, f->position.y, f->position.z,
            f->strength, f->radius);
    }

    JS_APPEND(js, sz, jp,
        "  addForce(type, pos, str, r) {\n"
        "    this.forces.push({type, pos, str, r});\n"
        "  }\n"
        "  _spawn() {\n"
        "    if (this.particles.length >= this.maxP) return;\n"
        "    const life = this.lifeMin + Math.random()*(this.lifeMax-this.lifeMin);\n"
        "    const spd  = this.vMin  + Math.random()*(this.vMax-this.vMin);\n"
        "    const ang  = Math.random() * Math.PI * 2;\n"
        "    const sz   = this.sMin  + Math.random()*(this.sMax-this.sMin);\n"
        "    this.particles.push({\n"
        "      x: Math.random(), y: Math.random(),\n"
        "      vx: Math.cos(ang)*spd, vy: Math.sin(spd)*spd,\n"
        "      life, maxLife: life, size: sz, t: 0\n"
        "    });\n"
        "  }\n"
        "  _applyForces(p, dt) {\n"
        "    this.forces.forEach(f => {\n"
        "      const dx = f.pos.x - p.x, dy = (f.pos.y||0) - p.y;\n"
        "      const d2 = dx*dx + dy*dy;\n"
        "      if (d2 < f.r*f.r) {\n"
        "        const d = Math.sqrt(d2) + 0.001;\n"
        "        const fn = f.str / d;\n"
        "        switch(f.type) {\n"
        "          case 0: p.vy += 9.8 * f.str * dt; break; // gravity\n"
        "          case 2: { // vortex\n"
        "            const tx=-dy/d, ty=dx/d;\n"
        "            p.vx+=tx*fn*dt; p.vy+=ty*fn*dt; break;\n"
        "          }\n"
        "          case 6: p.vx+=dx/d*fn*dt; p.vy+=dy/d*fn*dt; break; // attractor\n"
        "          case 7: p.vx-=dx/d*fn*dt; p.vy-=dy/d*fn*dt; break; // repulsor\n"
        "          default: p.vx+=(Math.random()-0.5)*fn*dt; p.vy+=(Math.random()-0.5)*fn*dt;\n"
        "        }\n"
        "      }\n"
        "    });\n"
        "  }\n"
        "  update(dt) {\n"
        "    this._t += dt;\n"
        "    // Spawn\n"
        "    const toSpawn = this.rate * dt;\n"
        "    for (let i=0; i<toSpawn; i++) this._spawn();\n"
        "    // Update\n"
        "    this.particles = this.particles.filter(p => p.life > 0);\n"
        "    this.particles.forEach(p => {\n"
        "      p.t += dt; p.life -= dt;\n"
        "      this._applyForces(p, dt);\n"
        "      p.x += p.vx*dt; p.y += p.vy*dt;\n"
        "      if (this.isFire) { p.vy -= 0.3*dt; p.vx *= 0.99; }\n"
        "      if (this.isNebula) {\n"
        "        const swirl = Math.sin(p.t*2)*0.01;\n"
        "        p.vx += -p.vy*swirl; p.vy += p.vx*swirl;\n"
        "      }\n"
        "    });\n"
        "  }\n"
        "  render(ctx2d) {\n"
        "    this.particles.forEach(p => {\n"
        "      const lt = p.t / p.maxLife;\n"
        "      const r = this.birthColor[0]*(1-lt) + this.deathColor[0]*lt;\n"
        "      const g = this.birthColor[1]*(1-lt) + this.deathColor[1]*lt;\n"
        "      const b = this.birthColor[2]*(1-lt) + this.deathColor[2]*lt;\n"
        "      const a = (1-lt) * (1-Math.pow(lt,4));\n"
        "      const W = ctx2d.canvas.width, H = ctx2d.canvas.height;\n"
        "      ctx2d.save();\n"
        "      if (this.leave_paint_trail) {\n"
        "        ctx2d.strokeStyle = `rgba(${r*255|0},${g*255|0},${b*255|0},${a*this.trailOp})`;\n"
        "        ctx2d.lineWidth = this.trailW;\n"
        "        ctx2d.beginPath();\n"
        "        ctx2d.moveTo((p.x-p.vx*0.05)*W, (p.y-p.vy*0.05)*H);\n"
        "        ctx2d.lineTo(p.x*W, p.y*H);\n"
        "        ctx2d.stroke();\n"
        "      }\n"
        "      ctx2d.fillStyle = `rgba(${r*255|0},${g*255|0},${b*255|0},${a})`;\n"
        "      ctx2d.beginPath();\n"
        "      ctx2d.arc(p.x*W, p.y*H, p.size*W*0.005, 0, Math.PI*2);\n"
        "      ctx2d.fill();\n"
        "      ctx2d.restore();\n"
        "    });\n"
        "  }\n"
        "}\n");

    int vp = 0;
    JS_APPEND(vert, sz, vp,
        "#version 300 es\n"
        "in vec3 a_pos;\n"
        "in float a_size;\n"
        "in float a_age;\n"
        "uniform mat4 u_mvp;\n"
        "out float v_age;\n"
        "void main() {\n"
        "  gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
        "  gl_PointSize = a_size * (1.0 - a_age);\n"
        "  v_age = a_age;\n"
        "}\n");

    int fp = 0;
    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "uniform vec3 u_birth_color; // (%.4f,%.4f,%.4f)\n"
        "uniform vec3 u_death_color; // (%.4f,%.4f,%.4f)\n"
        "uniform float u_time;\n"
        "uniform bool  u_is_nebula;\n"
        "in float v_age;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "  vec2 uv = gl_PointCoord - 0.5;\n"
        "  float d = length(uv);\n"
        "  if (d > 0.5) discard;\n"
        "  float a = smoothstep(0.5, 0.0, d) * (1.0 - v_age);\n"
        "  vec3 col = mix(u_birth_color, u_death_color, v_age);\n"
        "  if (u_is_nebula) {\n"
        "    float ang = atan(uv.y, uv.x);\n"
        "    col = 0.5 + 0.5*cos(vec3(ang + v_age*6.28318) + vec3(0,2.094,4.189));\n"
        "  }\n"
        "  fragColor = vec4(col, a);\n"
        "}\n",
        ctx->color_birth.x, ctx->color_birth.y, ctx->color_birth.z,
        ctx->color_death.x, ctx->color_death.y, ctx->color_death.z);

    out->js        = js;
    out->glsl_vert = vert;
    out->glsl_frag = frag;
    out->ok = true;
    return jp;
}

int rigart_art_material_shader__rig_variant_e22b7d4a(const RigMaterialCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 3;
    char *frag = calloc(sz, 1);
    if (!frag) return -1;
    int fp = 0;

    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt Material: %d\n"
        "// Albedo:(%.3f,%.3f,%.3f) M:%.3f R:%.3f\n"
        "uniform vec3  u_albedo;\n"
        "uniform float u_metallic;\n"
        "uniform float u_roughness;\n"
        "uniform float u_anisotropy;\n"
        "uniform float u_ior;\n"
        "uniform bool  u_enable_sss;\n"
        "uniform vec3  u_scatter_color;\n"
        "uniform float u_scatter_radius;\n"
        "uniform bool  u_enable_iridescence;\n"
        "uniform float u_iri_strength;\n"
        "uniform float u_iri_ior;\n"
        "uniform float u_iri_thickness;\n"
        "uniform bool  u_enable_clear_coat;\n"
        "uniform float u_coat_weight;\n"
        "uniform float u_coat_roughness;\n"
        "uniform vec3  u_emissive;\n"
        "uniform float u_emissive_strength;\n"
        "uniform float u_time;\n"
        "in vec3 v_normal; in vec3 v_pos; in vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "const float PI = 3.14159265359;\n"
        "const float PHI = 1.6180339887;\n\n",
        (int)ctx->preset,
        ctx->albedo.x, ctx->albedo.y, ctx->albedo.z,
        ctx->metallic, ctx->roughness);

    JS_APPEND(frag, sz, fp,
        "vec3 fresnelSchlick(float c,vec3 F0){return F0+(1.-F0)*pow(1.-c,5.);}\n"
        "float ggxNDF(vec3 N,vec3 H,float r){float a=r*r,a2=a*a,ndh=max(dot(N,H),0.),d=ndh*ndh*(a2-1.)+1.;return a2/(PI*d*d);}\n"
        "float schlickGGX(float ndv,float r){float k=(r+1.)*(r+1.)/8.;return ndv/(ndv*(1.-k)+k);}\n\n"
        "// Iridescence (thin film) — Belcour & Barla 2017\n"
        "vec3 iridescence(float ndv, float ior, float thickness) {\n"
        "  float OPD = 2.0 * ior * thickness * ndv;\n"
        "  vec3 phi = vec3(0.0, 0.333, 0.667);\n"
        "  vec3 R = 0.5 + 0.5*cos(2.*PI*(OPD/750.*vec3(1,1.5,2.) + phi));\n"
        "  R = mix(vec3(dot(R,vec3(0.333))), R, u_iri_strength);\n"
        "  return R;\n"
        "}\n\n");

    switch (ctx->preset) {
        case MAT_GOLD_POLISHED:
        case MAT_GOLD_BRUSHED:
            JS_APPEND(frag, sz, fp,
                "vec3 goldTint(){return vec3(1.0,0.766,0.336);}\n\n");
            break;
        case MAT_SILVER:
        case MAT_CHROME:
            JS_APPEND(frag, sz, fp,
                "vec3 silverTint(){return vec3(0.972,0.960,0.915);}\n\n");
            break;
        case MAT_PLATINUM:
            JS_APPEND(frag, sz, fp,
                "vec3 platinumTint(){return vec3(0.804,0.808,0.831);}\n\n");
            break;
        case MAT_TITANIUM_MATTE:
            JS_APPEND(frag, sz, fp,
                "vec3 titaniumTint(){return vec3(0.542,0.497,0.449);}\n\n");
            break;
        case MAT_COBALT_DEEP:
            JS_APPEND(frag, sz, fp,
                "vec3 cobaltTint(){return vec3(0.072,0.118,0.412);}\n\n");
            break;
        case MAT_OBSIDIAN:
            JS_APPEND(frag, sz, fp,
                "vec3 obsidianTint(){return vec3(0.022,0.018,0.032);}\n\n");
            break;
        case MAT_CERAMIC_WHITE:
            JS_APPEND(frag, sz, fp,
                "/* Porcelana — albedo calido con micro-variación */\n"
                "vec3 ceramicAlbedo(vec2 uv) {\n"
                "  float n=fract(sin(dot(uv,vec2(127.1,311.7)))*43758.5)*0.012;\n"
                "  return vec3(0.960,0.945,0.930)+n;\n"
                "}\n"
                "/* SSS cerámica — dispersión cálida bajo-superficie */\n"
                "vec3 ceramicSSS(float ndl){\n"
                "  float wrap=ndl*0.5+0.5;\n"
                "  return vec3(1.0,0.92,0.82)*wrap*0.28;\n"
                "}\n\n");
            break;
        case MAT_PEARL_LUNAR:
            JS_APPEND(frag, sz, fp,
                "/* Perla lunar — iridiscencia φ multiespectral */\n"
                "vec3 pearlIri(float ndv, float t){\n"
                "  float angle=ndv*PHI+t*0.15;\n"
                "  return 0.5+0.5*cos(6.28318*(vec3(0.0,0.333,0.667)+angle));\n"
                "}\n"
                "vec3 pearlSSS(float ndl){\n"
                "  return vec3(0.98,0.95,0.99)*(ndl*0.4+0.3)*0.22;\n"
                "}\n\n");
            break;
        case MAT_AMBER_VOLCANIC:
            JS_APPEND(frag, sz, fp,
                "/* Ámbar — SSS naranja translúcido cálido */\n"
                "vec3 amberSSS(float ndl){\n"
                "  float t=ndl*0.5+0.5;\n"
                "  return vec3(1.0,0.55,0.10)*t*t*0.45;\n"
                "}\n\n");
            break;
        case MAT_MARBLE_WHITE:
            JS_APPEND(frag, sz, fp,
                "/* Mármol Calacatta — venas procedurales + SSS */\n"
                "float marbleVein(vec2 uv){\n"
                "  float v=sin(uv.x*8.+sin(uv.y*3.+sin(uv.x*5.)));\n"
                "  v+=sin(uv.y*12.-sin(uv.x*4.)*2.)*0.5;\n"
                "  return smoothstep(0.5,0.52,abs(v));\n"
                "}\n"
                "vec3 marbleAlbedo(vec2 uv){\n"
                "  float v=marbleVein(uv*0.3);\n"
                "  return mix(vec3(0.96,0.94,0.91),vec3(0.55,0.50,0.48),v*0.35);\n"
                "}\n\n");
            break;
        case MAT_ICE:
            JS_APPEND(frag, sz, fp,
                "/* Hielo polar — SSS azul pálido + clearcoat */\n"
                "vec3 iceSSS(float ndl){\n"
                "  float t=ndl*0.4+0.45;\n"
                "  return vec3(0.72,0.88,0.98)*t*0.35;\n"
                "}\n\n");
            break;
        case MAT_DIAMOND:
            JS_APPEND(frag, sz, fp,
                "/* Diamante — dispersión cromática IOR 2.42 */\n"
                "vec3 diamondDisperse(vec3 N, vec3 V, float t){\n"
                "  vec3 r=reflect(-V,N);\n"
                "  float disp=0.05;\n"
                "  float cr=dot(r,vec3(1.,0.,0.))*disp;\n"
                "  float cg=dot(r,vec3(0.,1.,0.))*disp;\n"
                "  float cb=dot(r,vec3(0.,0.,1.))*disp;\n"
                "  return vec3(0.5+cr,0.5+cg,0.5+cb);\n"
                "}\n\n");
            break;
        case MAT_LAVA:
            JS_APPEND(frag, sz, fp,
                "/* Lava volcánica — emissive animada roja-naranja */\n"
                "vec3 lavaGlow(vec2 uv, float t){\n"
                "  float v=sin(uv.x*PHI*6.+t)+cos(uv.y*PHI*4.-t*0.7)\n"
                "        +sin(length(uv-0.5)*PHI*8.+t*1.3);\n"
                "  v=v*0.5+0.5;\n"
                "  return mix(vec3(0.12,0.02,0.0),vec3(1.0,0.38,0.02),v*v);\n"
                "}\n\n");
            break;
        case MAT_HOLOGRAPHIC:
        case MAT_IRIDESCENT:
            JS_APPEND(frag, sz, fp,
                "vec3 holoColor(vec2 uv, float t) {\n"
                "  return 0.5+0.5*cos(6.28318*(vec3(0,0.333,0.667)+length(uv-0.5)*3.+t));\n"
                "}\n\n");
            break;
        case MAT_PLASMA_FIELD:
        case MAT_AURORA:
        case MAT_NEBULA:
            JS_APPEND(frag, sz, fp,
                "vec3 plasmaColor(vec2 uv, float t) {\n"
                "  float v = sin(uv.x*PHI*10.+t) + cos(uv.y*PHI*8.-t*1.3)\n"
                "          + sin(length(uv-0.5)*PHI*12.+t*0.7);\n"
                "  return 0.5+0.5*cos(vec3(v, v+2.094, v+4.189));\n"
                "}\n\n");
            break;
        case MAT_FABRIC_SILK:
            JS_APPEND(frag, sz, fp,
                "/* Seda — sheen anisótropo + brillo direccional */\n"
                "vec3 silkSheen(vec3 N, vec3 V, vec3 L){\n"
                "  vec3 T=normalize(cross(N,vec3(0.,1.,0.)));\n"
                "  float sinTH=sqrt(1.-dot(T,normalize(V+L))*dot(T,normalize(V+L)));\n"
                "  return vec3(0.98,0.96,0.94)*pow(sinTH,8.)*0.6;\n"
                "}\n\n");
            break;
        default: break;
    }

    JS_APPEND(frag, sz, fp,
        "void main() {\n"
        "  vec3 N = normalize(v_normal);\n"
        "  vec3 V = normalize(-v_pos);\n"
        "  vec3 L = normalize(vec3(1.5,2.,2.5));\n"
        "  vec3 H = normalize(V+L);\n"
        "  float ndl=max(dot(N,L),0.), ndv=max(dot(N,V),0.);\n"
        "  vec3 albedo = u_albedo;\n");

    switch (ctx->preset) {
        case MAT_GOLD_POLISHED:
        case MAT_GOLD_BRUSHED:
            JS_APPEND(frag, sz, fp, "  albedo = goldTint();\n");
            break;
        case MAT_SILVER:
        case MAT_CHROME:
            JS_APPEND(frag, sz, fp, "  albedo = silverTint();\n");
            break;
        case MAT_PLATINUM:
            JS_APPEND(frag, sz, fp, "  albedo = platinumTint();\n");
            break;
        case MAT_TITANIUM_MATTE:
            JS_APPEND(frag, sz, fp, "  albedo = titaniumTint();\n");
            break;
        case MAT_COBALT_DEEP:
            JS_APPEND(frag, sz, fp, "  albedo = cobaltTint();\n");
            break;
        case MAT_OBSIDIAN:
            JS_APPEND(frag, sz, fp, "  albedo = obsidianTint();\n");
            break;
        case MAT_CERAMIC_WHITE:
            JS_APPEND(frag, sz, fp, "  albedo = ceramicAlbedo(v_uv);\n");
            break;
        case MAT_MARBLE_WHITE:
            JS_APPEND(frag, sz, fp, "  albedo = marbleAlbedo(v_uv);\n");
            break;
        case MAT_LAVA:
            JS_APPEND(frag, sz, fp, "  albedo = lavaGlow(v_uv, u_time);\n");
            break;
        case MAT_HOLOGRAPHIC:
        case MAT_IRIDESCENT:
            JS_APPEND(frag, sz, fp, "  albedo = holoColor(v_uv, u_time);\n");
            break;
        case MAT_PLASMA_FIELD:
        case MAT_AURORA:
        case MAT_NEBULA:
            JS_APPEND(frag, sz, fp, "  albedo = plasmaColor(v_uv, u_time);\n");
            break;
        default: break;
    }

    JS_APPEND(frag, sz, fp,
        "  vec3 F0 = mix(vec3(0.04), albedo, u_metallic);\n"
        "  vec3 F  = fresnelSchlick(max(dot(H,V),0.), F0);\n"
        "  float D = ggxNDF(N,H,u_roughness);\n"
        "  float G = schlickGGX(ndv,u_roughness)*schlickGGX(ndl,u_roughness);\n"
        "  vec3 spec = (D*G*F)/max(4.*ndv*ndl,0.001);\n"
        "  vec3 kD = (1.-F)*(1.-u_metallic);\n"
        "  vec3 color = (kD*albedo/PI + spec)*ndl + vec3(0.03)*albedo;\n"
        "  // Iridescence\n"
        "  if (u_enable_iridescence) {\n"
        "    vec3 iri = iridescence(ndv, u_iri_ior, u_iri_thickness);\n"
        "    color = mix(color, color*iri*2., u_iri_strength*ndv);\n"
        "  }\n"
        "  // Clear coat\n"
        "  if (u_enable_clear_coat) {\n"
        "    float Dc = ggxNDF(N,H,u_coat_roughness);\n"
        "    float Gc = schlickGGX(ndv,u_coat_roughness)*schlickGGX(ndl,u_coat_roughness);\n"
        "    vec3 Fc = fresnelSchlick(max(dot(H,V),0.), vec3(0.04));\n"
        "    vec3 coat_spec = (Dc*Gc*Fc)/max(4.*ndv*ndl,0.001);\n"
        "    color = mix(color, coat_spec, u_coat_weight);\n"
        "  }\n"
        "  // SSS global\n"
        "  if (u_enable_sss) {\n"
        "    vec3 sss = u_scatter_color * exp(-u_scatter_radius*(1.-ndl));\n"
        "    color += sss * 0.3;\n"
        "  }\n");

    switch (ctx->preset) {
        case MAT_CERAMIC_WHITE:
            JS_APPEND(frag, sz, fp,
                "  // Cerámica — SSS cálido + clearcoat de barniz\n"
                "  color += ceramicSSS(ndl);\n"
                "  float cR2=0.04,Dc2=ggxNDF(N,H,cR2),Gc2=schlickGGX(ndv,cR2)*schlickGGX(ndl,cR2);\n"
                "  color+=((Dc2*Gc2*fresnelSchlick(max(dot(H,V),0.),vec3(0.04)))/max(4.*ndv*ndl,0.001))*0.55;\n");
            break;
        case MAT_PEARL_LUNAR:
            JS_APPEND(frag, sz, fp,
                "  // Perla lunar — SSS nacarado + iridiscencia φ\n"
                "  color += pearlSSS(ndl);\n"
                "  color = mix(color, color*pearlIri(ndv,u_time)*1.4, 0.35*ndv);\n");
            break;
        case MAT_AMBER_VOLCANIC:
            JS_APPEND(frag, sz, fp,
                "  // Ámbar — SSS naranja translúcido\n"
                "  color += amberSSS(ndl);\n");
            break;
        case MAT_MARBLE_WHITE:
            JS_APPEND(frag, sz, fp,
                "  // Mármol — SSS suave + clearcoat de pulido\n"
                "  vec3 mSSS=vec3(0.98,0.96,0.93)*(ndl*0.3+0.2)*0.18;\n"
                "  color += mSSS;\n");
            break;
        case MAT_ICE:
            JS_APPEND(frag, sz, fp,
                "  // Hielo — SSS azul polar\n"
                "  color += iceSSS(ndl);\n"
                "  float iR=0.02,Di=ggxNDF(N,H,iR),Gi=schlickGGX(ndv,iR)*schlickGGX(ndl,iR);\n"
                "  color+=((Di*Gi*fresnelSchlick(max(dot(H,V),0.),vec3(0.04)))/max(4.*ndv*ndl,0.001))*0.7;\n");
            break;
        case MAT_DIAMOND:
            JS_APPEND(frag, sz, fp,
                "  // Diamante — dispersión cromática\n"
                "  color = mix(color, diamondDisperse(N,V,u_time)*2., 0.4*ndv);\n");
            break;
        case MAT_FABRIC_SILK:
            JS_APPEND(frag, sz, fp,
                "  // Seda — sheen anisotrópico\n"
                "  color += silkSheen(N,V,L);\n");
            break;
        case MAT_LAVA:
            JS_APPEND(frag, sz, fp,
                "  // Lava — glow emissivo pulsante\n"
                "  float lavaP=0.6+0.4*sin(u_time*PHI*2.3);\n"
                "  color += lavaGlow(v_uv,u_time)*lavaP*0.8;\n");
            break;
        case MAT_NEON_RED:
            JS_APPEND(frag, sz, fp,
                "  color += vec3(1.0,0.1,0.05)*(0.6+0.4*sin(u_time*PHI*3.))*0.9;\n");
            break;
        case MAT_NEON_BLUE:
            JS_APPEND(frag, sz, fp,
                "  color += vec3(0.05,0.3,1.0)*(0.6+0.4*sin(u_time*PHI*2.8))*0.9;\n");
            break;
        case MAT_NEON_GREEN:
            JS_APPEND(frag, sz, fp,
                "  color += vec3(0.05,1.0,0.2)*(0.6+0.4*sin(u_time*PHI*3.2))*0.9;\n");
            break;
        case MAT_NEON_PURPLE:
            JS_APPEND(frag, sz, fp,
                "  color += vec3(0.7,0.1,1.0)*(0.6+0.4*sin(u_time*PHI*2.6))*0.9;\n");
            break;
        default: break;
    }

    JS_APPEND(frag, sz, fp,
        "  // Emissive global\n"
        "  color += u_emissive * u_emissive_strength;\n"
        "  // φ-rim light soberano\n"
        "  float rim=pow(1.-ndv,2.6)*PHI*0.14;\n"
        "  color += albedo*0.5*rim + vec3(0.85,0.70,0.25)*rim*0.5;\n"
        "  // Tonemap ACES\n"
        "  color = color*(color+0.0245786)/(color*(0.983729*color+0.4329510)+0.238081);\n"
        "  color = pow(clamp(color,0.,1.), vec3(1./2.2));\n"
        "  fragColor = vec4(color, 1.0);\n"
        "}\n");
    out->ok = true;
    return fp;
}

int rigart_art_pattern__rig_variant_f439aa39(const RigPatternCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 2;
    char *frag = calloc(sz, 1);
    if (!frag) return -1;
    int fp = 0;

    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt Pattern Engine v4.0 — type:%d\n"
        "uniform float u_time;\n"
        "uniform float u_scale;    // %.4f\n"
        "uniform float u_rotation; // %.4f\n"
        "uniform vec3  u_color_a;  // (%.3f,%.3f,%.3f)\n"
        "uniform vec3  u_color_b;  // (%.3f,%.3f,%.3f)\n"
        "uniform float u_contrast; // %.4f\n"
        "uniform float u_distortion;// %.4f\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "const float PHI = 1.6180339887;\n"
        "const float PI  = 3.14159265359;\n\n",
        (int)ctx->type,
        ctx->scale, ctx->rotation,
        ctx->color_a.x, ctx->color_a.y, ctx->color_a.z,
        ctx->color_b.x, ctx->color_b.y, ctx->color_b.z,
        ctx->contrast, ctx->distortion);

    JS_APPEND(frag, sz, fp,
        "float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5);}\n"
        "vec2 hash2(vec2 p){return fract(sin(mat2(127.1,311.7,269.5,183.3)*p)*43758.5);}\n\n"
        "float voronoi(vec2 p) {\n"
        "  vec2 i=floor(p), f=fract(p);\n"
        "  float md=8., ms=0.;\n"
        "  for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) {\n"
        "    vec2 nb=vec2(x,y), rp=hash2(i+nb);\n"
        "    if (u_time > 0.) rp = 0.5+0.5*sin(u_time*PHI + 6.28318*rp);\n"
        "    vec2 r=nb+rp-f;\n"
        "    float d=dot(r,r);\n"
        "    if(d<md){ms=md; md=d;}\n"
        "  }\n"
        "  return sqrt(md);\n"
        "}\n\n"
        "float truchet_circles(vec2 p) {\n"
        "  vec2 i=floor(p), f=fract(p)-0.5;\n"
        "  float h=hash(i);\n"
        "  vec2 q = h>.5 ? f : vec2(f.x, -f.y);\n"
        "  return abs(length(q) - 0.5);\n"
        "}\n\n"
        "float islamic_8fold(vec2 p) {\n"
        "  p = fract(p) - 0.5;\n"
        "  float ang = PI/8.;\n"
        "  float r = length(p);\n"
        "  float a = mod(atan(p.y,p.x)+ang, 2.*ang) - ang;\n"
        "  vec2 q = r*vec2(cos(a),sin(a));\n"
        "  return abs(q.x - 0.5/cos(ang));\n"
        "}\n\n"
        "float penrose(vec2 p) {\n"
        "  // P3 tiling approximation via golden ratio modulation\n"
        "  float s=0.;\n"
        "  for (int i=0;i<5;i++) {\n"
        "    float ang = float(i)*PI*2./5.;\n"
        "    vec2 d = vec2(cos(ang),sin(ang));\n"
        "    s += abs(fract(dot(p,d)*PHI) - 0.5);\n"
        "  }\n"
        "  return s/5.;\n"
        "}\n\n"
        "float dna_helix(vec2 p) {\n"
        "  float a = sin(p.y * PI * 4.) * 0.3;\n"
        "  float b = sin(p.y * PI * 4. + PI) * 0.3;\n"
        "  float d1 = abs(p.x - a) - 0.04;\n"
        "  float d2 = abs(p.x - b) - 0.04;\n"
        "  // Rungs\n"
        "  float rung = abs(fract(p.y*8.)-0.5) - 0.05;\n"
        "  rung *= step(abs(p.x), 0.3);\n"
        "  return min(min(d1,d2), rung);\n"
        "}\n\n"
        "float circuit_board(vec2 p) {\n"
        "  vec2 g = fract(p*4.) - 0.5;\n"
        "  float r = min(abs(g.x), abs(g.y));\n"
        "  float via = length(fract(p*8.)-0.5) - 0.1;\n"
        "  return min(r - 0.05, via);\n"
        "}\n\n");

    JS_APPEND(frag, sz, fp,
        "void main() {\n"
        "  vec2 uv = v_uv;\n"
        "  // Apply rotation\n"
        "  float ca = cos(u_rotation), sa = sin(u_rotation);\n"
        "  uv = mat2(ca,-sa,sa,ca) * (uv - 0.5) + 0.5;\n"
        "  uv *= u_scale;\n"
        "  // Distortion\n"
        "  if (u_distortion > 0.0) {\n"
        "    uv += u_distortion * vec2(sin(uv.y*PHI*4.+u_time), cos(uv.x*PHI*4.-u_time)) * 0.1;\n"
        "  }\n"
        "  float d = 0.;\n");

    switch (ctx->type) {
        case PAT_VORONOI_FLAT:
        case PAT_VORONOI_RIDGED:
        case PAT_VORONOI_CELLS:
            JS_APPEND(frag, sz, fp, "  d = voronoi(uv);\n");
            if (ctx->type == PAT_VORONOI_RIDGED)
                JS_APPEND(frag, sz, fp, "  d = abs(2.0*d-1.);\n");
            break;
        case PAT_TRUCHET_CIRCLES:
        case PAT_TRUCHET_LINES:
            JS_APPEND(frag, sz, fp, "  d = truchet_circles(uv);\n");
            break;
        case PAT_ISLAMIC_8FOLD:
        case PAT_ISLAMIC_12FOLD:
            JS_APPEND(frag, sz, fp, "  d = islamic_8fold(uv);\n");
            break;
        case PAT_PENROSE_P2:
        case PAT_PENROSE_P3:
            JS_APPEND(frag, sz, fp, "  d = penrose(uv);\n");
            break;
        case PAT_DNA_HELIX:
            JS_APPEND(frag, sz, fp, "  d = dna_helix(uv);\n");
            break;
        case PAT_CIRCUIT_BOARD:
            JS_APPEND(frag, sz, fp, "  d = circuit_board(uv);\n");
            break;
        case PAT_PHI_GRID:
            JS_APPEND(frag, sz, fp,
                "  d = min(abs(fract(uv.x*PHI)-0.5), abs(fract(uv.y*PHI)-0.5));\n");
            break;
        default:
            JS_APPEND(frag, sz, fp, "  d = voronoi(uv);\n");
            break;
    }

    JS_APPEND(frag, sz, fp,
        "  float v = smoothstep(0., u_contrast*0.5, d);\n"
        "  vec3 col = mix(u_color_a, u_color_b, v);\n"
        "  fragColor = vec4(col, 1.0);\n"
        "}\n");

    out->glsl_frag = frag;
    out->ok = true;
    return fp;
}

int rigart_art_holographic__rig_variant_c0d91e97(const RigHoloCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 3;
    char *frag = calloc(sz, 1);
    if (!frag) return -1;
    int fp = 0;

    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt Holographic Effects v4.0\n"
        "uniform float u_time;\n"
        "uniform vec2  u_resolution;\n"
        "uniform sampler2D u_scene;\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "const float PI = 3.14159265359;\n"
        "const float PHI = 1.6180339887;\n\n"
        "float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5);}\n"
        "float noise(vec2 p){vec2 i=floor(p),f=fract(p),u=f*f*(3.-2.*f);\n"
        "  return mix(mix(hash(i),hash(i+vec2(1,0)),u.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),u.x),u.y);}\n\n");

    if (ctx->enable_diffraction) {
        JS_APPEND(frag, sz, fp,
            "vec3 diffraction(vec2 uv, float scale, float intensity) {\n"
            "  float ang = atan(uv.y-0.5, uv.x-0.5);\n"
            "  float r = length(uv - 0.5);\n"
            "  float diff = sin(r * scale * 200. - u_time) * intensity;\n"
            "  return 0.5 + 0.5*cos(vec3(diff, diff+2.094, diff+4.189));\n"
            "}\n\n");
    }

    if (ctx->enable_aurora) {
        JS_APPEND(frag, sz, fp,
            "vec3 aurora(vec2 uv, float speed, float complexity) {\n"
            "  float t = u_time * speed;\n"
            "  float y = uv.y + 0.5;\n"
            "  float band = noise(vec2(uv.x * complexity + t, t * 0.3)) * 0.5 + 0.5;\n"
            "  float mask = smoothstep(0.3, 0.7, y) * (1. - smoothstep(0.7, 1., y));\n"
            "  float wave = sin(uv.x * PHI * 8. + t * 2.) * 0.5 + 0.5;\n"
            "  vec3 ca = vec3(%.4f,%.4f,%.4f);\n"
            "  vec3 cb = vec3(%.4f,%.4f,%.4f);\n"
            "  return mix(ca, cb, wave) * band * mask * 2.;\n"
            "}\n\n",
            ctx->aurora_color_a.x, ctx->aurora_color_a.y, ctx->aurora_color_a.z,
            ctx->aurora_color_b.x, ctx->aurora_color_b.y, ctx->aurora_color_b.z);
    }

    if (ctx->enable_plasma) {
        JS_APPEND(frag, sz, fp,
            "vec3 plasma(vec2 uv, float scale, float speed) {\n"
            "  float t = u_time * speed;\n"
            "  float v = sin(uv.x*scale*PHI + t)\n"
            "          + cos(uv.y*scale*PHI - t*1.3)\n"
            "          + sin(length(uv-0.5)*scale*PHI*2. + t*0.7)\n"
            "          + sin((uv.x+uv.y)*scale*0.5 + t);\n"
            "  return 0.5 + 0.5*cos(vec3(v, v+2.094, v+4.189));\n"
            "}\n\n");
    }

    JS_APPEND(frag, sz, fp, "void main() {\n  vec2 uv = v_uv;\n");
    JS_APPEND(frag, sz, fp, "  vec3 color = texture(u_scene, uv).rgb;\n\n");

    if (ctx->enable_iridescence) {
        JS_APPEND(frag, sz, fp,
            "  // Iridescence\n"
            "  float ang = atan(uv.y-0.5, uv.x-0.5) / PI;\n"
            "  vec3 iri = 0.5+0.5*cos(6.28318*(vec3(0,.333,.667)+ang*%.4f+u_time*%.4f));\n"
            "  color = mix(color, iri, %.4f * length(uv-0.5)*2.);\n",
            ctx->iri_range, ctx->iri_speed, ctx->iri_base_hue > 0.f ? ctx->iri_base_hue : 0.8f);
    }

    if (ctx->enable_diffraction) {
        JS_APPEND(frag, sz, fp,
            "  color += diffraction(uv, %.4f, %.4f);\n",
            ctx->diffraction_scale, ctx->diffraction_intensity);
    }

    if (ctx->enable_aurora) {
        JS_APPEND(frag, sz, fp,
            "  color += aurora(uv, %.4f, %.4f);\n",
            ctx->aurora_speed, ctx->aurora_complexity);
    }

    if (ctx->enable_plasma) {
        JS_APPEND(frag, sz, fp,
            "  color = mix(color, plasma(uv, %.4f, %.4f), 0.5);\n",
            ctx->plasma_scale, ctx->plasma_speed);
    }

    if (ctx->enable_holo_scanlines) {
        JS_APPEND(frag, sz, fp,
            "  // Holographic scan lines\n"
            "  float scan = 0.5 + 0.5*sin(uv.y / %.4f * PI);\n"
            "  color = mix(color, color*scan, %.4f);\n",
            ctx->scanline_pitch, ctx->scanline_opacity);
    }

    if (ctx->enable_prism) {
        JS_APPEND(frag, sz, fp,
            "  // Prism dispersion\n"
            "  float d = length(uv-0.5);\n"
            "  float disp = %.4f * d;\n"
            "  color.r = texture(u_scene, uv + vec2(disp,0)).r;\n"
            "  color.b = texture(u_scene, uv - vec2(disp,0)).b;\n",
            ctx->prism_dispersion);
    }

    if (ctx->enable_holo_glitch) {
        JS_APPEND(frag, sz, fp,
            "  // Holographic glitch\n"
            "  float block = floor(uv.y / %.4f) * %.4f;\n"
            "  float glitch_t = floor(u_time / %.4f) * %.4f;\n"
            "  float jitter = step(0.95, hash(vec2(block, glitch_t))) * %.4f;\n"
            "  color = texture(u_scene, uv + vec2(jitter,0)).rgb;\n",
            ctx->glitch_block_size, ctx->glitch_block_size,
            ctx->glitch_frequency, ctx->glitch_frequency,
            ctx->glitch_block_size);
    }

    JS_APPEND(frag, sz, fp,
        "  fragColor = vec4(color, 1.0);\n}\n");

    out->glsl_frag = frag;
    out->ok = true;
    return fp;
}

int rigart_art_object3d__rig_variant_90aefa3d(const RigObject3DCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 3;
    char *js = calloc(sz, 1);
    if (!js) return -1;
    int jp = 0;

    JS_APPEND(js, sz, jp,
        "// RigArt 3D Object Library v4.0 — type:%d\n"
        "// pos:(%.3f,%.3f,%.3f) rot:(%.3f,%.3f,%.3f) scale:(%.3f,%.3f,%.3f)\n\n",
        (int)ctx->type,
        ctx->position.x, ctx->position.y, ctx->position.z,
        ctx->rotation.x, ctx->rotation.y, ctx->rotation.z,
        ctx->scale.x > 0.f ? ctx->scale.x : 1.f,
        ctx->scale.y > 0.f ? ctx->scale.y : 1.f,
        ctx->scale.z > 0.f ? ctx->scale.z : 1.f);

    JS_APPEND(js, sz, jp,
        "class RigObject3D {\n"
        "  static icosphere(subdivisions) {\n"
        "    const phi = (1+Math.sqrt(5))/2;\n"
        "    let v = [\n"
        "      [-1,phi,0],[1,phi,0],[-1,-phi,0],[1,-phi,0],\n"
        "      [0,-1,phi],[0,1,phi],[0,-1,-phi],[0,1,-phi],\n"
        "      [phi,0,-1],[phi,0,1],[-phi,0,-1],[-phi,0,1]\n"
        "    ].map(p=>{ const n=Math.sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);\n"
        "               return p.map(x=>x/n); });\n"
        "    let t = [\n"
        "      [0,11,5],[0,5,1],[0,1,7],[0,7,10],[0,10,11],\n"
        "      [1,5,9],[5,11,4],[11,10,2],[10,7,6],[7,1,8],\n"
        "      [3,9,4],[3,4,2],[3,2,6],[3,6,8],[3,8,9],\n"
        "      [4,9,5],[2,4,11],[6,2,10],[8,6,7],[9,8,1]\n"
        "    ];\n"
        "    const mid = {};\n"
        "    function getMid(a,b) {\n"
        "      const k = a<b?`${a}_${b}`:`${b}_${a}`;\n"
        "      if (mid[k]!==undefined) return mid[k];\n"
        "      const ma = v[a], mb = v[b];\n"
        "      const np = [ma[0]+mb[0],ma[1]+mb[1],ma[2]+mb[2]];\n"
        "      const n = Math.sqrt(np[0]*np[0]+np[1]*np[1]+np[2]*np[2]);\n"
        "      v.push(np.map(x=>x/n));\n"
        "      return mid[k] = v.length-1;\n"
        "    }\n"
        "    for (let s=0;s<(subdivisions||3);s++) {\n"
        "      const nt=[];\n"
        "      t.forEach(([a,b,c])=>{\n"
        "        const ab=getMid(a,b),bc=getMid(b,c),ca=getMid(c,a);\n"
        "        nt.push([a,ab,ca],[b,bc,ab],[c,ca,bc],[ab,bc,ca]);\n"
        "      });\n"
        "      t=nt;\n"
        "    }\n"
        "    const verts=new Float32Array(v.length*8);\n"
        "    v.forEach((p,i)=>{\n"
        "      verts[i*8]=p[0]; verts[i*8+1]=p[1]; verts[i*8+2]=p[2];\n"
        "      verts[i*8+3]=p[0]; verts[i*8+4]=p[1]; verts[i*8+5]=p[2]; // normal=pos for sphere\n"
        "      verts[i*8+6]=Math.atan2(p[2],p[0])/Math.PI*0.5+0.5;\n"
        "      verts[i*8+7]=Math.asin(p[1])/Math.PI+0.5;\n"
        "    });\n"
        "    const idx=new Uint32Array(t.length*3);\n"
        "    t.forEach((f,i)=>{idx[i*3]=f[0];idx[i*3+1]=f[1];idx[i*3+2]=f[2];});\n"
        "    return {verts, idx, nv:v.length, nt:t.length};\n"
        "  }\n\n");

    JS_APPEND(js, sz, jp,
        "  static torus(R, r, segMaj, segMin) {\n"
        "    R=R||1; r=r||0.3; segMaj=segMaj||32; segMin=segMin||16;\n"
        "    const verts=[],idx=[];\n"
        "    for(let i=0;i<=segMaj;i++) for(let j=0;j<=segMin;j++) {\n"
        "      const u=i/segMaj*Math.PI*2, v=j/segMin*Math.PI*2;\n"
        "      const x=(R+r*Math.cos(v))*Math.cos(u);\n"
        "      const y=(R+r*Math.cos(v))*Math.sin(u);\n"
        "      const z=r*Math.sin(v);\n"
        "      const nx=Math.cos(v)*Math.cos(u),ny=Math.cos(v)*Math.sin(u),nz=Math.sin(v);\n"
        "      verts.push(x,y,z,nx,ny,nz,i/segMaj,j/segMin);\n"
        "    }\n"
        "    for(let i=0;i<segMaj;i++) for(let j=0;j<segMin;j++) {\n"
        "      const a=i*(segMin+1)+j, b=a+segMin+1;\n"
        "      idx.push(a,b,a+1,b,b+1,a+1);\n"
        "    }\n"
        "    return {verts:new Float32Array(verts),idx:new Uint32Array(idx)};\n"
        "  }\n\n"
        "  static mobius(segs) {\n"
        "    segs=segs||128;\n"
        "    const verts=[], idx=[];\n"
        "    for (let i=0;i<=segs;i++) for (let j=0;j<=1;j++) {\n"
        "      const u=i/segs*Math.PI*2, v=(j-0.5)*0.5;\n"
        "      const x=(1+v*Math.cos(u/2))*Math.cos(u);\n"
        "      const y=(1+v*Math.cos(u/2))*Math.sin(u);\n"
        "      const z=v*Math.sin(u/2);\n"
        "      verts.push(x,y,z,0,0,1,i/segs,j);\n"
        "    }\n"
        "    for (let i=0;i<segs;i++) {\n"
        "      const a=i*2, b=a+2;\n"
        "      idx.push(a,b,a+1,b,b+1,a+1);\n"
        "    }\n"
        "    return {verts:new Float32Array(verts),idx:new Uint32Array(idx)};\n"
        "  }\n\n"
        "  static phiDodecahedron() {\n"
        "    const phi = (1+Math.sqrt(5))/2;\n"
        "    // 20 vertices of a dodecahedron\n"
        "    const v = [\n"
        "      [1,1,1],[1,1,-1],[1,-1,1],[1,-1,-1],\n"
        "      [-1,1,1],[-1,1,-1],[-1,-1,1],[-1,-1,-1],\n"
        "      [0,phi,1/phi],[0,phi,-1/phi],[0,-phi,1/phi],[0,-phi,-1/phi],\n"
        "      [1/phi,0,phi],[-1/phi,0,phi],[1/phi,0,-phi],[-1/phi,0,-phi],\n"
        "      [phi,1/phi,0],[phi,-1/phi,0],[-phi,1/phi,0],[-phi,-1/phi,0]\n"
        "    ].map(p=>{const n=Math.sqrt(p.reduce((s,x)=>s+x*x,0));return p.map(x=>x/n);});\n"
        "    const verts = new Float32Array(v.length*8);\n"
        "    v.forEach((p,i)=>{\n"
        "      verts[i*8]=p[0];verts[i*8+1]=p[1];verts[i*8+2]=p[2];\n"
        "      verts[i*8+3]=p[0];verts[i*8+4]=p[1];verts[i*8+5]=p[2];\n"
        "      verts[i*8+6]=p[0]*0.5+0.5;verts[i*8+7]=p[1]*0.5+0.5;\n"
        "    });\n"
        "    return {verts, nv:v.length};\n"
        "  }\n"
        "}\n\n"
        "// Factory function\n"
        "function rigart_build_object(type, subdivisions, params) {\n"
        "  switch(type) {\n"
        "    case 0: return RigObject3D.icosphere(subdivisions||%u);\n"
        "    case 4: return RigObject3D.torus(params[0]||1., params[1]||.3);\n"
        "    case 6: return RigObject3D.mobius(128);\n"
        "    case 9: return RigObject3D.phiDodecahedron();\n"
        "    default: return RigObject3D.icosphere(subdivisions||3);\n"
        "  }\n"
        "}\n",
        ctx->subdivisions > 0 ? ctx->subdivisions : 3);

    out->js = js;
    out->ok = true;
    return jp;
}

int rigart_art_timeline_gen__rig_variant_00958248(const RigTimelineCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 2;
    char *js = calloc(sz, 1);
    if (!js) return -1;
    int jp = 0;

    JS_APPEND(js, sz, jp,
        "// RigArt Animation Timeline v4.0\n"
        "// Duration: %.3fs  FPS: %.1f  Tracks: %u\n\n"
        "class RigTimeline {\n"
        "  constructor() {\n"
        "    this.duration = %.4f;\n"
        "    this.fps = %.1f;\n"
        "    this.time = %.4f;\n"
        "    this.playing = %s;\n"
        "    this.looping = false;\n"
        "    this.tracks = [];\n"
        "    this._startTime = null;\n"
        "  }\n\n",
        ctx->duration, ctx->fps, ctx->track_count,
        ctx->duration, ctx->fps, ctx->current_time,
        ctx->playing ? "true" : "false");

    JS_APPEND(js, sz, jp,
        "  addTrack(property, keyframes, looping=false) {\n"
        "    this.tracks.push({property, keyframes, looping});\n"
        "  }\n\n"
        "  _easeLinear(t)    { return t; }\n"
        "  _easeInCubic(t)   { return t*t*t; }\n"
        "  _easeOutCubic(t)  { return 1 - Math.pow(1-t,3); }\n"
        "  _easeInOutCubic(t){ return t<0.5 ? 4*t*t*t : 1-Math.pow(-2*t+2,3)/2; }\n"
        "  _easeElastic(t)   {\n"
        "    const c4=(2*Math.PI)/3;\n"
        "    return t===0?0:t===1?1:-Math.pow(2,10*t-10)*Math.sin((t*10-10.75)*c4);\n"
        "  }\n"
        "  _easeBounce(t) {\n"
        "    if (t<1/2.75) return 7.5625*t*t;\n"
        "    if (t<2/2.75) { t-=1.5/2.75; return 7.5625*t*t+0.75; }\n"
        "    if (t<2.5/2.75) { t-=2.25/2.75; return 7.5625*t*t+0.9375; }\n"
        "    t-=2.625/2.75; return 7.5625*t*t+0.984375;\n"
        "  }\n"
        "  _easeSpring(t)   { return 1 - Math.cos(t*Math.PI*4.5)*Math.exp(-t*6); }\n"
        "  _easePhiWave(t)  {\n"
        "    const phi = 1.6180339887;\n"
        "    return t - Math.sin(t*Math.PI*2*phi)*0.15;\n"
        "  }\n"
        "  _applyEase(t, type) {\n"
        "    switch(type) {\n"
        "      case 0: return this._easeLinear(t);\n"
        "      case 1: return this._easeInCubic(t);\n"
        "      case 2: return this._easeOutCubic(t);\n"
        "      case 3: return this._easeInOutCubic(t);\n"
        "      case 4: return this._easeElastic(t);\n"
        "      case 5: return this._easeBounce(t);\n"
        "      case 6: return this._easeSpring(t);\n"
        "      case 7: return this._easePhiWave(t);\n"
        "      default: return t;\n"
        "    }\n"
        "  }\n\n"
        "  evaluate(time) {\n"
        "    const result = {};\n"
        "    this.tracks.forEach(track => {\n"
        "      const keys = track.keyframes;\n"
        "      if (!keys || keys.length === 0) return;\n"
        "      if (time <= keys[0].time) { result[track.property] = keys[0].value; return; }\n"
        "      if (time >= keys[keys.length-1].time) { result[track.property] = keys[keys.length-1].value; return; }\n"
        "      for (let i=0; i<keys.length-1; i++) {\n"
        "        if (time >= keys[i].time && time <= keys[i+1].time) {\n"
        "          const span = keys[i+1].time - keys[i].time;\n"
        "          const t = (time - keys[i].time) / span;\n"
        "          const et = this._applyEase(t, keys[i].easing||0);\n"
        "          result[track.property] = keys[i].value * (1-et) + keys[i+1].value * et;\n"
        "          return;\n"
        "        }\n"
        "      }\n"
        "    });\n"
        "    return result;\n"
        "  }\n\n"
        "  play() {\n"
        "    this.playing = true;\n"
        "    this._startTime = performance.now() / 1000 - this.time;\n"
        "  }\n"
        "  pause() { this.playing = false; }\n"
        "  seek(t) { this.time = Math.max(0, Math.min(this.duration, t)); }\n"
        "  tick() {\n"
        "    if (!this.playing) return this.evaluate(this.time);\n"
        "    this.time = performance.now()/1000 - this._startTime;\n"
        "    if (this.time >= this.duration) {\n"
        "      if (this.looping) { this._startTime = performance.now()/1000; this.time=0; }\n"
        "      else { this.playing=false; this.time=this.duration; }\n"
        "    }\n"
        "    return this.evaluate(this.time);\n"
        "  }\n"
        "}\n\n"
        "// Built-in timeline from context\n"
        "const rigTimeline = new RigTimeline();\n");

    for (int i = 0; i < ctx->track_count; i++) {
        const RigAnimTrack *tr = &ctx->tracks[i];
        JS_APPEND(js, sz, jp,
            "rigTimeline.addTrack('%s', [\n", tr->property);
        for (uint32_t k = 0; k < tr->key_count && k < RIG_MAX_KEYFRAMES; k++) {
            JS_APPEND(js, sz, jp,
                "  {time:%.4f, value:%.6f, easing:%d},\n",
                tr->keys[k].time, tr->keys[k].value, (int)tr->keys[k].easing);
        }
        JS_APPEND(js, sz, jp, "], %s);\n",
            tr->looping ? "true" : "false");
    }

    out->js = js;
    out->ok = true;
    return jp;
}

int rigart_art_export__rig_variant_60e68855(const RigExportCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF;
    char *js = calloc(sz, 1);
    if (!js) return -1;
    int jp = 0;

    JS_APPEND(js, sz, jp,
        "// RigArt Export Engine v4.0\n"
        "// Format:%d  Size:%ux%u  DPI:%.1f\n\n"
        "const RigExport = {\n"
        "  toPNG(canvas, filename, scale) {\n"
        "    scale = scale || 1;\n"
        "    const out = document.createElement('canvas');\n"
        "    out.width  = canvas.width  * scale;\n"
        "    out.height = canvas.height * scale;\n"
        "    const ctx = out.getContext('2d');\n"
        "    ctx.drawImage(canvas, 0,0, out.width, out.height);\n"
        "    const link = document.createElement('a');\n"
        "    link.download = filename || 'rigart_export.png';\n"
        "    link.href = out.toDataURL('image/png');\n"
        "    link.click();\n"
        "  },\n\n"
        "  to4K(canvas, filename) {\n"
        "    this.toPNG(canvas, filename || 'rigart_4k.png',\n"
        "      Math.max(4096/canvas.width, 2160/canvas.height));\n"
        "  },\n\n"
        "  toSVG(paths, width, height, filename) {\n"
        "    let svg = `<svg xmlns='http://www.w3.org/2000/svg' `\n"
        "            + `width='${width}' height='${height}' `\n"
        "            + `viewBox='0 0 ${width} ${height}'>`;\n"
        "    paths.forEach(p => { svg += p.toSVG ? p.toSVG() : ''; });\n"
        "    svg += '</svg>';\n"
        "    const blob = new Blob([svg], {type:'image/svg+xml'});\n"
        "    const url  = URL.createObjectURL(blob);\n"
        "    const link = document.createElement('a');\n"
        "    link.download = filename || 'rigart.svg';\n"
        "    link.href = url;\n"
        "    link.click();\n"
        "    setTimeout(() => URL.revokeObjectURL(url), 1000);\n"
        "  },\n\n"
        "  toGLTF(meshes, filename) {\n"
        "    // GLTF 2.0 binary export (minimal)\n"
        "    const scenes = [{nodes:[...meshes.map((_,i)=>i)]}];\n"
        "    const json = {\n"
        "      asset:{version:'2.0',generator:'RigArt v4.0'},\n"
        "      scene:0, scenes,\n"
        "      nodes: meshes.map((m,i)=>({mesh:i,name:`mesh_${i}`})),\n"
        "      meshes: meshes.map((m,i)=>({name:`mesh_${i}`,\n"
        "        primitives:[{attributes:{POSITION:i*2, NORMAL:i*2+1}}]})),\n"
        "    };\n"
        "    const jsonStr = JSON.stringify(json);\n"
        "    const blob = new Blob([jsonStr], {type:'model/gltf+json'});\n"
        "    const url  = URL.createObjectURL(blob);\n"
        "    const link = document.createElement('a');\n"
        "    link.download = filename || 'rigart.gltf';\n"
        "    link.href = url;\n"
        "    link.click();\n"
        "    setTimeout(() => URL.revokeObjectURL(url), 1000);\n"
        "  },\n\n"
        "  toSpriteSheet(frames, cols, filename) {\n"
        "    if (!frames.length) return;\n"
        "    const fw = frames[0].width, fh = frames[0].height;\n"
        "    const rows = Math.ceil(frames.length / cols);\n"
        "    const sheet = document.createElement('canvas');\n"
        "    sheet.width = fw*cols; sheet.height = fh*rows;\n"
        "    const ctx = sheet.getContext('2d');\n"
        "    frames.forEach((f,i)=>{\n"
        "      ctx.drawImage(f, (i%%cols)*fw, Math.floor(i/cols)*fh);\n"
        "    });\n"
        "    this.toPNG(sheet, filename||'sprite_sheet.png');\n"
        "  }\n"
        "};\n",
        (int)ctx->format, ctx->width, ctx->height, ctx->dpi);

    out->js = js;
    out->ok = true;
    return jp;
}

int rigart_art_style_apply__rig_variant_f4ca8bdb(const RigStyleCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 3;
    char *css = calloc(sz, 1);
    char *js  = calloc(sz, 1);
    if (!css || !js) { free(css); free(js); return -1; }

    typedef struct { const char *bg1, *bg2, *acc1, *acc2, *text, *font; } StyleDef;
    static const StyleDef presets[] = {
         {"#0a0510","#1a0a2e","#c9a84c","#8b6914","#e8d5b0","Cinzel Decorative"},
         {"#0d0221","#110833","#00fff5","#ff0080","#e0e0ff","Orbitron"},
         {"#2a1a00","#3d2600","#d4af37","#b8860b","#faf0dc","IM Fell English"},
         {"#0a0a0a","#111111","#888888","#555555","#cccccc","Space Mono"},
         {"#020d1a","#051a30","#40e0d0","#7b68ee","#ddeeff","Quicksand"},
         {"#e8f4f8","#d0e8ef","#7eb8d4","#4a9ab5","#1a3a4a","Josefin Sans"},
        {"#0c0c10","#16161e","#8eaac3","#5a7a96","#ccd9e3","Exo 2"},
         {"#050010","#0a0020","#8b00ff","#4400cc","#cc88ff","Audiowide"},
        {"#f5e6c8","#e8d5a0","#8b4513","#6b3410","#2a1000","Uncial Antiqua"},
         {"#001a00","#002600","#00ff41","#008020","#88ff99","Share Tech Mono"},
         {"#f8f4e8","#ede8d5","#c9a84c","#9a7830","#2a1a00","Philosopher"},
         {"#0a1a0a","#051005","#44ff44","#228822","#ccffcc","Rajdhani"},
    };
    int pi = (int)ctx->preset;
    if (pi < 0 || pi >= 12) pi = 0;
    const StyleDef *sd = &presets[pi];

    int cp = 0;
    JS_APPEND(css, sz, cp,
        "/* RigArt Style Preset v4.0 — %d */\n"
        ":root {\n"
        "  --rig-bg1: %s; --rig-bg2: %s;\n"
        "  --rig-acc1: %s; --rig-acc2: %s;\n"
        "  --rig-text: %s;\n"
        "  --rig-font: '%s';\n"
        "  --rig-radius: %.4fpx;\n"
        "  --phi: 1.6180339887;\n"
        "}\n\n"
        "body {\n"
        "  background: linear-gradient(135deg, var(--rig-bg1), var(--rig-bg2));\n"
        "  color: var(--rig-text);\n"
        "  font-family: var(--rig-font), sans-serif;\n"
        "  margin: 0; min-height: 100vh;\n"
        "}\n\n"
        ".rig-panel {\n"
        "  background: rgba(0,0,0,0.4);\n"
        "  border: 1px solid var(--rig-acc1);\n"
        "  border-radius: var(--rig-radius);\n"
        "  backdrop-filter: blur(12px);\n"
        "  padding: 1.618rem;\n"
        "}\n\n"
        ".rig-btn {\n"
        "  background: linear-gradient(135deg, var(--rig-acc1), var(--rig-acc2));\n"
        "  color: var(--rig-bg1);\n"
        "  border: none; border-radius: calc(var(--rig-radius)*0.618);\n"
        "  padding: 0.618rem 1.618rem;\n"
        "  font-family: var(--rig-font);\n"
        "  font-weight: bold; cursor: pointer;\n"
        "  transition: all 0.3s;\n"
        "}\n"
        ".rig-btn:hover { filter: brightness(1.3); transform: scale(1.05); }\n\n"
        ".rig-title {\n"
        "  font-size: clamp(1.5rem, 3vw, 3rem);\n"
        "  background: linear-gradient(135deg, var(--rig-acc1), var(--rig-acc2));\n"
        "  -webkit-background-clip: text;\n"
        "  -webkit-text-fill-color: transparent;\n"
        "  background-clip: text;\n"
        "  letter-spacing: 0.1em;\n"
        "}\n",
        pi,
        sd->bg1, sd->bg2, sd->acc1, sd->acc2, sd->text, sd->font,
        ctx->border_radius_phi > 0.f ? ctx->border_radius_phi * (float)RIG_PHI : 8.f);

    int jp = 0;
    if (ctx->enable_particles) {
        JS_APPEND(js, sz, jp,
            "// RigArt style particles background\n"
            "function rigStyleParticles(canvas) {\n"
            "  const ctx = canvas.getContext('2d');\n"
            "  const phi = 1.6180339887;\n"
            "  const pts = Array.from({length:80},(_,i)=>({x:Math.random(),y:Math.random(),vx:(Math.random()-.5)*.001,vy:(Math.random()-.5)*.001,r:Math.random()*2+1}));\n"
            "  function draw() {\n"
            "    ctx.clearRect(0,0,canvas.width,canvas.height);\n"
            "    pts.forEach(p=>{\n"
            "      p.x+=p.vx; p.y+=p.vy;\n"
            "      if(p.x<0||p.x>1) p.vx*=-1;\n"
            "      if(p.y<0||p.y>1) p.vy*=-1;\n"
            "      ctx.beginPath();\n"
            "      ctx.arc(p.x*canvas.width,p.y*canvas.height,p.r,0,Math.PI*2);\n"
            "      ctx.fillStyle='%s44';\n"
            "      ctx.fill();\n"
            "    });\n"
            "    // Draw connections\n"
            "    for(let i=0;i<pts.length;i++) for(let j=i+1;j<pts.length;j++) {\n"
            "      const d=Math.hypot((pts[i].x-pts[j].x)*canvas.width,(pts[i].y-pts[j].y)*canvas.height);\n"
            "      if(d<120){\n"
            "        ctx.beginPath();\n"
            "        ctx.moveTo(pts[i].x*canvas.width,pts[i].y*canvas.height);\n"
            "        ctx.lineTo(pts[j].x*canvas.width,pts[j].y*canvas.height);\n"
            "        ctx.strokeStyle='%s'+(1-d/120).toString(16).slice(2,4).padStart(2,'0');\n"
            "        ctx.lineWidth=0.5;\n"
            "        ctx.stroke();\n"
            "      }\n"
            "    }\n"
            "    requestAnimationFrame(draw);\n"
            "  }\n"
            "  draw();\n"
            "}\n",
            sd->acc1, sd->acc1);
    }

    out->css = css;
    out->js  = js;
    out->ok  = true;
    return cp + jp;
}

int rigart_art_symmetry_shader__rig_variant_5ec4a796(const RigSymmetryCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 2;
    char *frag = calloc(sz, 1);
    if (!frag) return -1;
    int fp = 0;

    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt Symmetry Engine v4.0 — mode:%d\n"
        "uniform sampler2D u_source;\n"
        "uniform float u_rot_offset;  // %.4f\n"
        "uniform float u_frac_depth;  // %.4f\n"
        "in  vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "const float PI  = 3.14159265359;\n"
        "const float PHI = 1.6180339887;\n\n",
        (int)ctx->mode, ctx->rotation_offset, ctx->fractal_depth);

    JS_APPEND(frag, sz, fp,
        "vec2 fold_bilateral_x(vec2 uv) { return vec2(abs(uv.x-0.5)+0.5-0.5, uv.y); }\n"
        "vec2 fold_bilateral_y(vec2 uv) { return vec2(uv.x, abs(uv.y-0.5)+0.5-0.5); }\n"
        "vec2 fold_radial(vec2 uv, float n) {\n"
        "  vec2 p = uv - 0.5;\n"
        "  float a = atan(p.y, p.x) + u_rot_offset;\n"
        "  float r = length(p);\n"
        "  float seg = PI * 2. / n;\n"
        "  a = mod(a, seg);\n"
        "  if (a > seg * 0.5) a = seg - a;\n"
        "  return vec2(cos(a), sin(a)) * r + 0.5;\n"
        "}\n"
        "vec2 fold_kaleidoscope(vec2 uv) {\n"
        "  vec2 p = uv - 0.5;\n"
        "  float a = atan(p.y, p.x);\n"
        "  float r = length(p);\n"
        "  a = mod(a + u_rot_offset, PI / 4.);\n"
        "  if (a > PI/8.) a = PI/4. - a;\n"
        "  p = vec2(cos(a), sin(a)) * r;\n"
        "  // Radial mirror\n"
        "  r = mod(r, 0.3);\n"
        "  if (r > 0.15) r = 0.3 - r;\n"
        "  return vec2(cos(a), sin(a)) * r + 0.5;\n"
        "}\n"
        "vec2 fold_phi(vec2 uv) {\n"
        "  vec2 p = uv - 0.5;\n"
        "  float a = atan(p.y, p.x) + u_rot_offset;\n"
        "  float r = length(p);\n"
        "  float seg = PI * 2. / PHI;\n"
        "  a = mod(a, seg);\n"
        "  if (a > seg*0.5) a = seg-a;\n"
        "  return vec2(cos(a), sin(a)) * r + 0.5;\n"
        "}\n\n"
        "void main() {\n"
        "  vec2 uv = v_uv;\n"
        "  vec2 suv = uv;\n");

    switch (ctx->mode) {
        case SYM_BILATERAL_X:   JS_APPEND(frag, sz, fp, "  suv = fold_bilateral_x(uv);\n"); break;
        case SYM_BILATERAL_Y:   JS_APPEND(frag, sz, fp, "  suv = fold_bilateral_y(uv);\n"); break;
        case SYM_BILATERAL_XY:  JS_APPEND(frag, sz, fp, "  suv = fold_bilateral_y(fold_bilateral_x(uv));\n"); break;
        case SYM_RADIAL_3:      JS_APPEND(frag, sz, fp, "  suv = fold_radial(uv, 3.);\n"); break;
        case SYM_RADIAL_4:      JS_APPEND(frag, sz, fp, "  suv = fold_radial(uv, 4.);\n"); break;
        case SYM_RADIAL_5:      JS_APPEND(frag, sz, fp, "  suv = fold_radial(uv, 5.);\n"); break;
        case SYM_RADIAL_6:      JS_APPEND(frag, sz, fp, "  suv = fold_radial(uv, 6.);\n"); break;
        case SYM_RADIAL_8:      JS_APPEND(frag, sz, fp, "  suv = fold_radial(uv, 8.);\n"); break;
        case SYM_RADIAL_12:     JS_APPEND(frag, sz, fp, "  suv = fold_radial(uv, 12.);\n"); break;
        case SYM_KALEIDOSCOPE:  JS_APPEND(frag, sz, fp, "  suv = fold_kaleidoscope(uv);\n"); break;
        case SYM_PHI_ROTATIONAL:JS_APPEND(frag, sz, fp, "  suv = fold_phi(uv);\n"); break;
        case SYM_FRACTAL:
            JS_APPEND(frag, sz, fp,
                "  // Fractal fold (IFS)\n"
                "  vec2 p = uv - 0.5;\n"
                "  for (int i=0; i<int(u_frac_depth); i++) {\n"
                "    p = abs(p) - 0.25;\n"
                "    float l = length(p);\n"
                "    if (l < 0.01) break;\n"
                "    p /= l * l;\n"
                "  }\n"
                "  suv = p * 0.5 + 0.5;\n");
            break;
        default: break;
    }

    JS_APPEND(frag, sz, fp,
        "  vec4 col = texture(u_source, clamp(suv, 0.001, 0.999));\n"
        "  fragColor = col;\n"
        "}\n");

    out->glsl_frag = frag;
    out->ok = true;
    return fp;
}

int rigart_art_lighting__rig_variant_175a802b(const RigLightingCtx *ctx, RigArtResultV4 *out) {
    if (!ctx || !out) return -1;
    rigart_v4_init_result__rig_variant_2c6804ec(out);

    size_t sz = RIG_SHADER_MAXBUF * 3;
    char *frag = calloc(sz, 1);
    char *js   = calloc(sz, 1);
    if (!frag || !js) { free(frag); free(js); return -1; }

    int fp = 0;
    JS_APPEND(frag, sz, fp,
        "#version 300 es\nprecision highp float;\n"
        "// RigArt Lighting Studio v4.0 — %u lights\n"
        "uniform vec3  u_ambient;     // (%.3f,%.3f,%.3f) × %.3f\n"
        "uniform bool  u_enable_gi;   // %s\n"
        "uniform float u_gi_strength; // %.4f\n"
        "uniform float u_gi_radius;   // %.4f\n"
        "in vec3 v_pos; in vec3 v_normal; in vec2 v_uv;\n"
        "out vec4 fragColor;\n"
        "const float PI = 3.14159265359;\n"
        "const float PHI = 1.6180339887;\n\n",
        (unsigned)ctx->light_count,
        ctx->ambient_color.x, ctx->ambient_color.y, ctx->ambient_color.z,
        ctx->ambient_strength,
        ctx->enable_global_illumination ? "true" : "false",
        ctx->gi_strength, ctx->gi_radius);

    for (int i = 0; i < ctx->light_count && i < RIG_MAX_LIGHTS; i++) {
        const RigLightDesc *L = &ctx->lights[i];
        JS_APPEND(frag, sz, fp,
            "uniform vec3  u_light%d_pos;   // (%.3f,%.3f,%.3f)\n"
            "uniform vec3  u_light%d_color; // (%.3f,%.3f,%.3f)\n"
            "uniform float u_light%d_intensity; // %.3f\n"
            "uniform float u_light%d_range;     // %.3f\n"
            "uniform float u_light%d_inner;     // %.3f\n"
            "uniform float u_light%d_outer;     // %.3f\n",
            i, L->position.x, L->position.y, L->position.z,
            i, L->color.x, L->color.y, L->color.z,
            i, L->intensity,
            i, L->range,
            i, L->inner_angle,
            i, L->outer_angle);
    }

    JS_APPEND(frag, sz, fp,
        "\nvec3 fresnelSchlick(float c,vec3 F0){return F0+(1.-F0)*pow(1.-c,5.);}\n"
        "float ggxNDF(vec3 N,vec3 H,float r){float a=r*r,a2=a*a,ndh=max(dot(N,H),0.),d=ndh*ndh*(a2-1.)+1.;return a2/(PI*d*d);}\n"
        "float schlickGGX(float ndv,float r){float k=(r+1.)*(r+1.)/8.;return ndv/(ndv*(1.-k)+k);}\n\n"
        "uniform vec3  u_albedo;\n"
        "uniform float u_metallic, u_roughness;\n\n"
        "void main() {\n"
        "  vec3 N = normalize(v_normal);\n"
        "  vec3 V = normalize(-v_pos);\n"
        "  float ndv = max(dot(N,V),0.);\n"
        "  vec3 F0 = mix(vec3(0.04), u_albedo, u_metallic);\n"
        "  vec3 Lo = vec3(0.);\n\n");

    for (int i = 0; i < ctx->light_count && i < RIG_MAX_LIGHTS; i++) {
        const RigLightDesc *L = &ctx->lights[i];
        JS_APPEND(frag, sz, fp, "  {\n");
        switch (L->type) {
            case LIGHT_POINT:
                JS_APPEND(frag, sz, fp,
                    "    vec3 Lv = u_light%d_pos - v_pos;\n"
                    "    float dist = length(Lv); Lv = normalize(Lv);\n"
                    "    float att = clamp(1. - dist/u_light%d_range, 0., 1.);\n"
                    "    att *= att;\n", i, i);
                break;
            case LIGHT_SPOT:
                JS_APPEND(frag, sz, fp,
                    "    vec3 Lv = normalize(u_light%d_pos - v_pos);\n"
                    "    float dist = length(u_light%d_pos - v_pos);\n"
                    "    float att = clamp(1.-dist/u_light%d_range,0.,1.); att*=att;\n"
                    "    vec3 spotDir = normalize(-u_light%d_pos);\n"
                    "    float theta = dot(Lv, spotDir);\n"
                    "    float eps = u_light%d_inner - u_light%d_outer;\n"
                    "    att *= clamp((theta-u_light%d_outer)/eps,0.,1.);\n",
                    i, i, i, i, i, i, i);
                break;
            case LIGHT_DIRECTIONAL:
            default:
                JS_APPEND(frag, sz, fp,
                    "    vec3 Lv = normalize(u_light%d_pos);\n"
                    "    float att = 1.0;\n", i);
                break;
        }
        JS_APPEND(frag, sz, fp,
            "    vec3 H = normalize(V + Lv);\n"
            "    float ndl = max(dot(N,Lv),0.);\n"
            "    vec3 F = fresnelSchlick(max(dot(H,V),0.),F0);\n"
            "    float D = ggxNDF(N,H,u_roughness);\n"
            "    float G = schlickGGX(ndv,u_roughness)*schlickGGX(ndl,u_roughness);\n"
            "    vec3 spec = (D*G*F)/max(4.*ndv*ndl,0.001);\n"
            "    vec3 kD = (1.-F)*(1.-u_metallic);\n"
            "    Lo += (kD*u_albedo/PI+spec)*ndl*u_light%d_color*u_light%d_intensity*att;\n"
            "  }\n", i, i);
    }

    if (ctx->enable_global_illumination) {
        JS_APPEND(frag, sz, fp,
            "  // GI approximation — bent normals + SH\n"
            "  float gi = max(0., dot(N, vec3(0.,1.,0.)) * 0.5 + 0.5);\n"
            "  Lo += u_albedo * gi * u_gi_strength * vec3(0.3,0.4,0.6);\n");
    }

    JS_APPEND(frag, sz, fp,
        "  vec3 ambient = u_ambient_color * u_ambient_strength * u_albedo;\n"
        "  vec3 color = ambient + Lo;\n"
        "  // ACES tonemap\n"
        "  color = color*(color+0.0245786)/(color*(0.983729*color+0.4329510)+0.238081);\n"
        "  color = pow(clamp(color,0.,1.), vec3(1./2.2));\n"
        "  fragColor = vec4(color,1.0);\n"
        "}\n");

    int jp = 0;
    JS_APPEND(js, sz, jp,
        "// RigArt Lighting Studio — setUniforms helper\n"
        "function rigSetLightUniforms(gl, prog, lightingCtx) {\n"
        "  lightingCtx.lights.forEach((L,i) => {\n"
        "    gl.uniform3f(gl.getUniformLocation(prog,`u_light${i}_pos`),   L.x,L.y,L.z);\n"
        "    gl.uniform3f(gl.getUniformLocation(prog,`u_light${i}_color`), L.cr,L.cg,L.cb);\n"
        "    gl.uniform1f(gl.getUniformLocation(prog,`u_light${i}_intensity`), L.intensity);\n"
        "    gl.uniform1f(gl.getUniformLocation(prog,`u_light${i}_range`), L.range);\n"
        "    gl.uniform1f(gl.getUniformLocation(prog,`u_light${i}_inner`), L.inner);\n"
        "    gl.uniform1f(gl.getUniformLocation(prog,`u_light${i}_outer`), L.outer);\n"
        "  });\n"
        "  gl.uniform3f(gl.getUniformLocation(prog,'u_ambient'),\n"
        "    %.4f, %.4f, %.4f);\n"
        "}\n",
        ctx->ambient_color.x * ctx->ambient_strength,
        ctx->ambient_color.y * ctx->ambient_strength,
        ctx->ambient_color.z * ctx->ambient_strength);

    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    return fp + jp;
}
