/* ==========================================================================
 * 18_rig_face_suite_compat.c   —   RIGFACE :: C UNIFICADO
 *
 * Copia canonica : nested/rig_face_v2_bridge/rig_face_suite_compat.c
 * Copias fundidas: 1
 * Funciones      : 4      Unidades injertadas: 0      Variantes: 0
 *
 * Copia canonica preservada verbatim. Ninguna funcion eliminada,
 * reducida ni omitida. Las divergencias de cuerpo bajo un mismo
 * nombre se conservan como <nombre>__vN con su procedencia.
 * ========================================================================== */
#include "rig_face_v2_bridge.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_printf(const char *fmt, ...)
{
    va_list ap, cp;
    va_start(ap, fmt);
    va_copy(cp, ap);
    int n = vsnprintf(NULL, 0, fmt, cp);
    va_end(cp);
    if (n < 0) {
        va_end(ap);
        return NULL;
    }
    char *s = (char *)malloc((size_t)n + 1u);
    if (s) {
        (void)vsnprintf(s, (size_t)n + 1u, fmt, ap);
    }
    va_end(ap);
    return s;
}

int ws_broadcastf(WsServer *srv, const char *fmt, ...)
{
    if (!fmt) return -1;
    va_list ap;
    va_start(ap, fmt);
    FILE *sink = (srv && srv->sink) ? srv->sink : stdout;
    int rc = vfprintf(sink, fmt, ap);
    va_end(ap);
    if (rc < 0) return -1;
    return fputc('\n', sink) == EOF ? -1 : 0;
}

int rigart_art_compositor(const RigArtCompositorCtx *c, RigArtResultV4 *out)
{
    if (!c || !out) return -1;
    rigart_v4_init_result(out);
    out->js = dup_printf(
        "const RIG_COMPOSITOR={exposure:%.6g,gamma:%.6g,contrast:%.6g,saturation:%.6g,bloom:%s,bloomStrength:%.6g,dof:%s,aces:%s,vignette:%s};\n"
        "function rigAces(x){return Math.max(0,Math.min(1,(x*(2.51*x+.03))/(x*(2.43*x+.59)+.14)));}\n",
        c->exposure, c->gamma, c->contrast, c->saturation,
        c->enable_bloom ? "true" : "false", c->bloom_strength,
        c->enable_dof ? "true" : "false",
        c->tonemapping_aces ? "true" : "false",
        c->enable_vignette ? "true" : "false");
    out->ok = out->js != NULL;
    out->phi_ratio = 1.61803398875f;
    out->certeza = .61803398875f;
    return out->ok ? 0 : -1;
}

int rigart_art_canvas_gen(const RigArtCanvasCtx *c, RigArtResultV4 *out)
{
    if (!c || !out || c->width <= 0 || c->height <= 0 ||
        c->layer_count < 0 || c->layer_count > 16) return -1;
    rigart_v4_init_result(out);
    size_t cap = 4096u + (size_t)c->layer_count * 256u;
    out->js = (char *)calloc(cap, 1u);
    if (!out->js) return -1;
    size_t p = (size_t)snprintf(out->js, cap,
        "const RIG_CANVAS={width:%d,height:%d,dpi:%.6g,hdrP3:%s,bit16:%s,phiGrid:%.9g,layers:[",
        c->width, c->height, c->dpi,
        c->hdr_p3_enabled ? "true" : "false",
        c->enable_16bit ? "true" : "false", c->grid_size_phi);
    for (int i = 0; i < c->layer_count && p < cap; ++i) {
        const RigArtCanvasLayer *l = &c->layers[i];
        int n = snprintf(out->js + p, cap - p,
            "%s{name:\"%s\",blend:%d,opacity:%.6g,visible:%s}",
            i ? "," : "", l->name, l->blend_mode, l->opacity,
            l->visible ? "true" : "false");
        if (n < 0 || (size_t)n >= cap - p) {
            rigart_v4_free_result(out);
            return -1;
        }
        p += (size_t)n;
    }
    if (snprintf(out->js + p, cap - p, "]};\n") < 0) {
        rigart_v4_free_result(out);
        return -1;
    }
    out->ok = true;
    out->phi_ratio = 1.61803398875f;
    out->certeza = .61803398875f;
    return 0;
}

