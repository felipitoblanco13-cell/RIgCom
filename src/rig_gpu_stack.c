/* ═══════════════════════════════════════════════════════════════════════════
 * rig_gpu_stack.c — MOTOR GPU DE ALTO NIVEL · RIGCOM
 *
 * Extraído desde CONSTRUIDO/rig_master.c consolidado.
 * Contiene el stack completo GPU (GLES3): shaders, programas, buffers,
 * VAOs, texturas, FBOs, estados de render, primitivas de dibujo 2D.
 *
 * Vive junto a rig_gpu_old.c (carga dinámica de punteros GL) y provee
 * las funciones de alto nivel que otros módulos consumen.
 *
 * C11 · cero libc · φ
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rig_gpu.h"
#include "rig_master.h"

extern float rl_sqrtf(float);
extern float rl_sinf(float);
extern float rl_cosf(float);
extern float rl_fabsf(float);
extern void *rl_memset(void *, int, unsigned long);
extern void *rl_memcpy(void *, const void *, unsigned long);
extern int   rl_snprintf(char *, unsigned long, const char *, ...);

static inline SRBackend* sr_get(RigGPUCtx *g){ return (SRBackend*)g->egl_display; }

static inline float sr_clamp(float v, float lo, float hi){
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline float sr_maxf(float a, float b){ return a > b ? a : b; }

static inline float sr_minf(float a, float b){ return a < b ? a : b; }

static inline float sr_lerpf(float a, float b, float t){ return a + (b - a) * t; }

static inline float sr_dot3(const float a[3], const float b[3]){
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

static inline void sr_cross3(const float a[3], const float b[3], float o[3]){
    o[0] = a[1]*b[2] - a[2]*b[1];
    o[1] = a[2]*b[0] - a[0]*b[2];
    o[2] = a[0]*b[1] - a[1]*b[0];
}

static inline void sr_norm3(float v[3]){
    float l = rl_sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if(l < 1e-7f) return;
    float inv = 1.f / l;
    v[0] *= inv; v[1] *= inv; v[2] *= inv;
}

static void sr_mv4(const float m[16], const float v[4], float o[4]){
    for(int i = 0; i < 4; i++)
        o[i] = m[i]*v[0] + m[i+4]*v[1] + m[i+8]*v[2] + m[i+12]*v[3];
}

static void sr_mm4(const float a[16], const float b[16], float o[16]){
    for(int c = 0; c < 4; c++)
        for(int r = 0; r < 4; r++){
            float s = 0.f;
            for(int k = 0; k < 4; k++) s += a[r + k*4] * b[k + c*4];
            o[r + c*4] = s;
        }
}

static void sr_m4id(float m[16]){
    rl_memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

static inline uint32_t sr_ihash2(uint32_t x, uint32_t y){
    uint32_t h = x*73856093u ^ y*19349663u;
    h ^= h >> 16; h *= 0x45d9f3bu; h ^= h >> 16; h *= 0x45d9f3bu; h ^= h >> 16;
    return h;
}

static inline float sr_h2f(uint32_t h){
    float f;
    uint32_t b = (h >> 9) | 0x3F800000u;
    __builtin_memcpy(&f, &b, 4);
    return f - 1.f;
}

static inline float sr_phi_hash(float u, float v){
    return sr_h2f(sr_ihash2((uint32_t)(u * 73856093.f), (uint32_t)(v * 19349663.f)));
}

static float sr_fbm(float u, float v, int oct){
    float val = 0.f, amp = 1.f, freq = 1.f;
    for(int i = 0; i < oct; i++){
        float ix = rl_floorf(u * freq), iy = rl_floorf(v * freq);
        float fx = rl_fmodf(u * freq - ix, 1.f), fy = rl_fmodf(v * freq - iy, 1.f);
        float v00 = sr_h2f(sr_ihash2((uint32_t)ix,   (uint32_t)iy));
        float v10 = sr_h2f(sr_ihash2((uint32_t)ix+1, (uint32_t)iy));
        float v01 = sr_h2f(sr_ihash2((uint32_t)ix,   (uint32_t)iy+1));
        float v11 = sr_h2f(sr_ihash2((uint32_t)ix+1, (uint32_t)iy+1));
        float sx = fx*fx*(3.f - 2.f*fx), sy = fy*fy*(3.f - 2.f*fy);
        val += amp * (sr_lerpf(sr_lerpf(v00, v10, sx), sr_lerpf(v01, v11, sx), sy));
        amp  *= RL_PHI_INV;
        freq *= RL_PHI;
    }
    return val;
}

static void sr_voronoi(float u, float v, float scale, float *d1, float *d2){
    float pu = u * scale, pv = v * scale;
    float pu0 = rl_floorf(pu), pv0 = rl_floorf(pv);
    *d1 = 1e9f; *d2 = 1e9f;
    for(int dy = -2; dy <= 2; dy++) for(int dx = -2; dx <= 2; dx++){
        float cx = pu0 + (float)dx, cy = pv0 + (float)dy;
        float sx = sr_h2f(sr_ihash2((uint32_t)((int)cx & 0xFFFF), (uint32_t)((int)cy & 0xFFFF)));
        float sy = sr_h2f(sr_ihash2((uint32_t)((int)cx & 0xFFFF)+37, (uint32_t)((int)cy & 0xFFFF)+19));
        float px2 = cx + sx*RL_PHI_INV, py2 = cy + sy*RL_PHI_INV;
        float ex = pu - px2, ey = pv - py2;
        float d  = rl_sqrtf(ex*ex + ey*ey);
        if(d < *d1){ *d2 = *d1; *d1 = d; } else if(d < *d2){ *d2 = d; }
    }
}

static void sr_texsample(SRBackend *sr, uint32_t tid, float u, float v, float o[4]){
    o[0] = o[1] = o[2] = 0.5f; o[3] = 1.f;
    if(tid >= RIG_GPU_MAX_TEXTURES || !sr->tex_data[tid] || !g_gpu_ctx) return;
    RigTexture *t = NULL;
    for(uint32_t i = 0; i < g_gpu_ctx->n_textures; i++)
        if(g_gpu_ctx->textures[i].id == tid){ t = &g_gpu_ctx->textures[i]; break; }
    if(!t || !t->width || !t->height) return;
    uint32_t W = t->width, H = t->height;
    u = rl_fmodf(u, 1.f); if(u < 0.f) u += 1.f;
    v = rl_fmodf(v, 1.f); if(v < 0.f) v += 1.f;
    float fx = u*(float)(W-1), fy = v*(float)(H-1);
    uint32_t x0 = (uint32_t)fx, y0 = (uint32_t)fy;
    uint32_t x1 = x0+1 < W ? x0+1 : x0, y1 = y0+1 < H ? y0+1 : y0;
    float tx = fx - (float)x0, ty = fy - (float)y0;
    if(t->format == RIG_TEX_RGBA8){
        const uint8_t *d   = (const uint8_t*)sr->tex_data[tid];
        const uint8_t *p00 = d + (y0*W + x0)*4, *p10 = d + (y0*W + x1)*4;
        const uint8_t *p01 = d + (y1*W + x0)*4, *p11 = d + (y1*W + x1)*4;
        for(int c = 0; c < 4; c++)
            o[c] = sr_lerpf(sr_lerpf(p00[c], p10[c], tx), sr_lerpf(p01[c], p11[c], tx), ty) / 255.f;
    } else if(t->format == RIG_TEX_RGBA32F){
        const float *d   = (const float*)sr->tex_data[tid];
        const float *p00 = d + (y0*W + x0)*4, *p10 = d + (y0*W + x1)*4;
        const float *p01 = d + (y1*W + x0)*4, *p11 = d + (y1*W + x1)*4;
        for(int c = 0; c < 4; c++)
            o[c] = sr_lerpf(sr_lerpf(p00[c], p10[c], tx), sr_lerpf(p01[c], p11[c], tx), ty);
    } else if(t->format == RIG_TEX_R8){
        const uint8_t *d = (const uint8_t*)sr->tex_data[tid];
        float top = sr_lerpf(d[y0*W + x0], d[y0*W + x1], tx) / 255.f;
        float bot = sr_lerpf(d[y1*W + x0], d[y1*W + x1], tx) / 255.f;
        o[0] = o[1] = o[2] = o[3] = sr_lerpf(top, bot, ty);
    }
}

static float sr_depth_sample(SRBackend *sr, uint32_t fbo_id, float u, float v){
    if(fbo_id >= RIG_GPU_MAX_FBOS || !sr->fbo_depth[fbo_id]) return 1.f;
    uint32_t W = sr->fbo_w[fbo_id], H = sr->fbo_h[fbo_id];
    if(!W || !H) return 1.f;
    int x = (int)(sr_clamp(u, 0.f, 1.f) * (float)(W-1));
    int y = (int)(sr_clamp(v, 0.f, 1.f) * (float)(H-1));
    return sr->fbo_depth[fbo_id][(uint32_t)y * W + (uint32_t)x];
}

static float sr_shadow_pcf(SRBackend *sr, uint32_t sfbo, float su, float sv, float sdepth){
    if(sfbo >= RIG_GPU_MAX_FBOS || !sr->fbo_depth[sfbo]) return 1.f;
    uint32_t W = sr->fbo_w[sfbo], H = sr->fbo_h[sfbo];
    float tu = 1.f / (float)W, tv = 1.f / (float)H;
    float shadow = 0.f, bias = 0.001f;
    for(int s = 0; s < 9; s++){
        float smp = sr_depth_sample(sr, sfbo, su + SR_PCF_KX[s]*tu*2.f, sv + SR_PCF_KY[s]*tv*2.f);
        shadow += (sdepth - bias > smp) ? 0.f : 1.f;
    }
    return shadow / 9.f;
}

static SRUniform* sr_uget(SRBackend *sr, uint32_t p, const char *n){
    if(p >= RIG_GPU_MAX_PROGRAMS) return NULL;
    for(uint32_t i = 0; i < sr->n_uniforms[p]; i++)
        if(rl_strncmp(sr->uniforms[p][i].name, n, RIG_GPU_MAX_UNIFORM_NAME-1) == 0)
            return &sr->uniforms[p][i];
    return NULL;
}

static SRUniform* sr_uset(SRBackend *sr, uint32_t p, const char *n, SRUnifType t){
    if(p >= RIG_GPU_MAX_PROGRAMS) return NULL;
    SRUniform *u = sr_uget(sr, p, n);
    if(!u){
        if(sr->n_uniforms[p] >= SR_MAX_UNIFORMS) return NULL;
        u = &sr->uniforms[p][sr->n_uniforms[p]++];
        rl_strncpy(u->name, n, RIG_GPU_MAX_UNIFORM_NAME-1);
    }
    u->type = t; u->set = true;
    return u;
}

static void sr_fast_cache(SRBackend *sr, uint32_t p, const char *n){
    SRUniform *u = sr_uget(sr, p, n);
    if(!u || !u->set) return;
    if(!rl_strcmp(n,"u_mvp")   || !rl_strcmp(n,"uViewProj")) rl_memcpy(sr->mvp,   u->val.m16, 64);
    else if(!rl_strcmp(n,"u_model") || !rl_strcmp(n,"uModel"))   rl_memcpy(sr->model, u->val.m16, 64);
    else if(!rl_strcmp(n,"uView"))                                 rl_memcpy(sr->view,  u->val.m16, 64);
    else if(!rl_strcmp(n,"uProj"))                                 rl_memcpy(sr->proj,  u->val.m16, 64);
    else if(!rl_strcmp(n,"uLightVP"))                              rl_memcpy(sr->light_vp, u->val.m16, 64);
    else if(!rl_strcmp(n,"uCamPos") || !rl_strcmp(n,"u_cam_pos")) rl_memcpy(sr->cam_pos, u->val.v3, 12);
    else if(!rl_strcmp(n,"u_light_dir") || !rl_strcmp(n,"uLDir")) rl_memcpy(sr->light_dir, u->val.v3, 12);
}

static inline float sr_D_GGX(float NdotH, float a2){
    float d = NdotH*NdotH*(a2 - 1.f) + 1.f;
    return a2 / (SR_PI * d*d + 1e-7f);
}

static inline float sr_G_Smith(float NdotV, float NdotL, float rough){
    float k = (rough + 1.f) * (rough + 1.f) * 0.125f;
    return (NdotV / (NdotV*(1.f-k) + k)) * (NdotL / (NdotL*(1.f-k) + k));
}

static inline void sr_F_Schlick(float HdotV, const float F0[3], float F[3]){
    float t = rl_powf(sr_clamp(1.f - HdotV, 0.f, 1.f), 5.f);
    for(int i = 0; i < 3; i++) F[i] = F0[i] + (1.f - F0[i]) * t;
}

static inline float sr_atten(float dist){
    float d2 = dist*dist, phi2 = RL_PHI_INV * RL_PHI_INV;
    return 1.f / (1.f + phi2*dist + phi2*phi2*d2);
}

static inline float sr_aces(float x){
    x = x*(2.51f*x + 0.03f) / (x*(2.43f*x + 0.59f) + 0.14f);
    return x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
}

static inline float sr_srgb(float x){
    return x < 0.0031308f ? 12.92f*x : 1.055f*rl_powf(x, 1.f/2.4f) - 0.055f;
}

static inline uint32_t sr_pack(float r, float g, float b){
    return ((uint32_t)(sr_clamp(r,0.f,1.f)*255.f) << 16) |;
           ((uint32_t)(sr_clamp(g,0.f,1.f)*255.f) <<  8) |
            (uint32_t)(sr_clamp(b,0.f,1.f)*255.f);
}

static void sr_ui_pixel(SRBackend *sr, int x, int y, uint32_t color)
{
    if(!sr || !sr->active_color || x<0 || y<0 ||
       x>=(int)sr->active_w || y>=(int)sr->active_h) return;
    if(sr_ui_scissor_enabled &&
       (x<sr_ui_sx || y<sr_ui_sy || x>=sr_ui_sx+sr_ui_sw || y>=sr_ui_sy+sr_ui_sh)) return;
    sr->active_color[(uint32_t)y*sr->active_pitch32+(uint32_t)x]=color;
}

void gpu_begin_2d(int vp_w,int vp_h){
    SRBackend *sr=sr_get(g_gpu_ctx); if(!sr) return;
    sr->state.viewport_x=0; sr->state.viewport_y=0;
    sr->state.viewport_w=(uint32_t)(vp_w>0?vp_w:1);
    sr->state.viewport_h=(uint32_t)(vp_h>0?vp_h:1);
}

void gpu_end_2d(void){ sr_ui_scissor_enabled=0; }

void gpu_set_scissor(int x,int y,int w,int h){
    sr_ui_sx=x; sr_ui_sy=y; sr_ui_sw=w>0?w:0; sr_ui_sh=h>0?h:0;
    sr_ui_scissor_enabled=(w>0&&h>0);
}

void gpu_clear_scissor(void){ sr_ui_scissor_enabled=0; }

void gpu_draw_rect(float x,float y,float w,float h,float r,float g,float b,float a,float border_r){
    SRBackend *sr=sr_get(g_gpu_ctx); if(!sr || w<=0.f || h<=0.f || a<=0.f) return;
    uint32_t color=sr_pack(r*a,g*a,b*a);
    int x0=(int)x,y0=(int)y,x1=(int)(x+w),y1=(int)(y+h);
    float radius=border_r>0.f?border_r:0.f, rr=radius*radius;
    for(int py=y0;py<y1;py++) for(int px=x0;px<x1;px++){
        if(radius>0.f){
            float cx=px<x0+(int)radius?x0+radius:(px>x1-(int)radius?x1-radius:(float)px);
            float cy=py<y0+(int)radius?y0+radius:(py>y1-(int)radius?y1-radius:(float)py);
            float dx=(float)px-cx,dy=(float)py-cy; if(dx*dx+dy*dy>rr) continue;
        }
        sr_ui_pixel(sr,px,py,color);
    }
}

void gpu_draw_line(float x0,float y0,float x1,float y1,float r,float g,float b,float a,float thickness){
    SRBackend *sr=sr_get(g_gpu_ctx); if(!sr || a<=0.f) return;
    float dx=x1-x0,dy=y1-y0; int steps=(int)rl_fabsf(dx); if((int)rl_fabsf(dy)>steps) steps=(int)rl_fabsf(dy);
    if(steps<1) steps=1;
    int half=(int)(thickness>1.f?thickness*.5f:0.f);
    uint32_t color=sr_pack(r*a,g*a,b*a);
    for(int i=0;i<=steps;i++){
        float t=(float)i/(float)steps; int px=(int)(x0+dx*t),py=(int)(y0+dy*t);
        for(int oy=-half;oy<=half;oy++) for(int ox=-half;ox<=half;ox++) sr_ui_pixel(sr,px+ox,py+oy,color);
    }
}

void gpu_draw_text(const char *text,float x,float y,float size,float r,float g,float b,float a,float rotation){
    if(!text || size<=0.f) return;
    float advance=size*.62f, stroke=size*.09f; if(stroke<1.f) stroke=1.f;
    for(size_t i=0;text[i];i++){
        if(text[i]!=' '){
            float gx=x+(float)i*advance;
            gpu_draw_line(gx,y,gx+advance*.65f,y,r,g,b,a,stroke);
            gpu_draw_line(gx,y,gx,y+size,r,g,b,a,stroke);
            gpu_draw_line(gx,y+size,gx+advance*.65f,y+size,r,g,b,a,stroke);
        }
    }
    (void)rotation;
}

static void sr_sh_irr(float sh[3][9], const float N[3], float out[3]){
    float b[9] = {
        0.282095f,
        0.488603f*N[1], 0.488603f*N[2], 0.488603f*N[0],
        1.092548f*N[0]*N[1], 1.092548f*N[1]*N[2],
        0.315392f*(3.f*N[2]*N[2] - 1.f),
        1.092548f*N[0]*N[2], 0.546274f*(N[0]*N[0] - N[1]*N[1])
    };
    for(int c = 0; c < 3; c++){
        out[c] = 0.f;
        for(int k = 0; k < 9; k++) out[c] += sh[c][k] * b[k];
        if(out[c] < 0.f) out[c] = 0.f;
    }
}

static void sr_shade_pbr(SRBackend *sr, uint32_t prog,
    const float *vpos, const float *vnorm, const float *vtang,
    const float *vuv,  const float *vcol,  uint32_t *out)
{
    float albedo[3] = {0.8f, 0.8f, 0.8f}, rough = 0.5f, metal = 0.f, ao = 1.f;
    float emissive[3] = {0.f, 0.f, 0.f};
    SRUniform *u;
#define SRG(N,F)       do{ if((u=sr_uget(sr,prog,N))&&u->set) F=u->val.f; }while(0)
#define SRG3(N,A,B,C)  do{ if((u=sr_uget(sr,prog,N))&&u->set){ A=u->val.v3[0]; B=u->val.v3[1]; C=u->val.v3[2]; } }while(0)
    SRG3("uAlbedo",  albedo[0], albedo[1], albedo[2]);
    SRG3("u_albedo", albedo[0], albedo[1], albedo[2]);
    SRG("uRoughness", rough);  SRG("uRoughVal",  rough);
    SRG("uMetallic",  metal);  SRG("uMetalVal",  metal);
    SRG("uAO", ao);
#undef SRG
#undef SRG3
    albedo[0] *= vcol[0]; albedo[1] *= vcol[1]; albedo[2] *= vcol[2];
    uint32_t tex_alb = RIG_GPU_INVALID_ID;
    if((u = sr_uget(sr, prog, "uTexAlbedo")) && u->set) tex_alb = (uint32_t)u->val.i;
    if(tex_alb != RIG_GPU_INVALID_ID){
        float tc[4]; sr_texsample(sr, tex_alb, vuv[0], vuv[1], tc);
        albedo[0] *= tc[0]; albedo[1] *= tc[1]; albedo[2] *= tc[2];
    }
    float N[3] = {vnorm[0], vnorm[1], vnorm[2]}; sr_norm3(N);
    float V[3] = {sr->cam_pos[0]-vpos[0], sr->cam_pos[1]-vpos[1], sr->cam_pos[2]-vpos[2]};
    sr_norm3(V);
    float NdotV = sr_maxf(sr_dot3(N, V), 0.f);
    float F0[3] = {
        sr_lerpf(0.04f, albedo[0], metal),
        sr_lerpf(0.04f, albedo[1], metal),
        sr_lerpf(0.04f, albedo[2], metal)
    };
    float Lo[3] = {0.f, 0.f, 0.f};
    for(uint32_t li = 0; li < sr->n_lights; li++){
        SRLight *lgt = &sr->lights[li];
        float L[3], attn = 1.f;
        if(lgt->type == 1){
            L[0] = -lgt->pos_x; L[1] = -lgt->pos_y; L[2] = -lgt->pos_z;
        } else {
            L[0] = lgt->pos_x - vpos[0];
            L[1] = lgt->pos_y - vpos[1];
            L[2] = lgt->pos_z - vpos[2];
            float dist = rl_sqrtf(sr_dot3(L, L));
            if(dist > 1e-5f){ float inv = 1.f/dist; L[0]*=inv; L[1]*=inv; L[2]*=inv; }
            attn = sr_atten(dist);
        }
        float NdotL = sr_maxf(sr_dot3(N, L), 0.f);
        if(NdotL < 1e-5f) continue;
        float H[3] = {V[0]+L[0], V[1]+L[1], V[2]+L[2]}; sr_norm3(H);
        float NdotH = sr_maxf(sr_dot3(N, H), 0.f), HdotV = sr_maxf(sr_dot3(H, V), 0.f);
        float a2 = rough*rough*rough*rough;
        float D = sr_D_GGX(NdotH, a2), G = sr_G_Smith(NdotV, NdotL, rough), F[3];
        sr_F_Schlick(HdotV, F0, F);
        float denom = 4.f*NdotV*NdotL + 1e-4f;
        float li2 = lgt->intensity * attn * NdotL;
        for(int c = 0; c < 3; c++){
            float kd = (1.f - F[c]) * (1.f - metal);
            float lc = (c==0) ? lgt->color_r : (c==1 ? lgt->color_g : lgt->color_b);
            Lo[c] += (kd*albedo[c]*SR_INV_PI + D*G*F[c]/denom) * lc * li2;
        }
    }
    float shadow = 1.f;
    if(sr->shadow_fbo_id < RIG_GPU_MAX_FBOS){
        float lp[4] = {vpos[0], vpos[1], vpos[2], 1.f}, lc[4];
        sr_mv4(sr->light_vp, lp, lc);
        if(rl_fabsf(lc[3]) > 1e-5f){
            float su = (lc[0]/lc[3])*.5f + .5f;
            float sv = (lc[1]/lc[3])*.5f + .5f;
            float sd =  lc[2]/lc[3]*.5f  + .5f;
            shadow = sr_shadow_pcf(sr, sr->shadow_fbo_id, su, sv, sd);
        }
    }
    float amb[3]; sr_sh_irr(sr->ibl_sh, N, amb);
    float r = sr_aces((Lo[0]*shadow + amb[0]*albedo[0]*ao + emissive[0]) * sr->ibl_intensity);
    float g = sr_aces((Lo[1]*shadow + amb[1]*albedo[1]*ao + emissive[1]) * sr->ibl_intensity);
    float b = sr_aces((Lo[2]*shadow + amb[2]*albedo[2]*ao + emissive[2]) * sr->ibl_intensity);
    *out = sr_pack(sr_srgb(r), sr_srgb(g), sr_srgb(b));
    (void)vtang;
}

static void sr_shade_skin(SRBackend *sr, uint32_t prog,
    const float *vpos, const float *vnorm, const float *vtang,
    const float *vuv,  const float *vcol,  uint32_t *out)
{
    float melanin = 0.3f, pheomelanin = 0.15f, hemoglobin = 0.2f,
          carotene = 0.1f, roughness = 0.35f, ior = 1.40f;
    SRUniform *u;
    if((u = sr_uget(sr,prog,"u_melanin"))    && u->set) melanin     = u->val.f;
    if((u = sr_uget(sr,prog,"uMelanin"))     && u->set) melanin     = u->val.f;
    if((u = sr_uget(sr,prog,"uPheomelanin")) && u->set) pheomelanin = u->val.f;
    if((u = sr_uget(sr,prog,"uHemoglobin"))  && u->set) hemoglobin  = u->val.f;
    if((u = sr_uget(sr,prog,"uCarotene"))    && u->set) carotene    = u->val.f;
    if((u = sr_uget(sr,prog,"uRoughness"))   && u->set) roughness   = u->val.f;
    float base[3] = {
        sr_clamp(0.82f*(1.f-melanin*.55f-pheomelanin*.15f)+hemoglobin*.22f+carotene*.18f,0,1)*vcol[0],
        sr_clamp(0.64f*(1.f-melanin*.38f-pheomelanin*.05f)+hemoglobin*.04f+carotene*.12f,0,1)*vcol[1],
        sr_clamp(0.49f*(1.f-melanin*.72f-pheomelanin*.02f)+hemoglobin*.02f,0,1)*vcol[2]
    };
    float d1, d2; sr_voronoi(vuv[0], vuv[1], 120.f, &d1, &d2);
    float pore = sr_clamp(1.f - d1*7.f, 0.f, 1.f), rim = sr_clamp(d2 - d1, 0.f, 1.f);
    base[0] *= (1.f - pore*.18f) - rim*.08f;
    base[1] *= (1.f - pore*.14f) - rim*.06f;
    base[2] *= (1.f - pore*.10f) - rim*.04f;
    float vasc = rl_powf(sr_maxf(sr_fbm(vuv[0]*4.f, vuv[1]*4.f, 3) - .55f, 0.f)*3.f, RL_PHI);
    base[0] += vasc*.12f; base[1] += vasc*.02f;
    static const float sigma_s[4][3] = {
        {.02f,.02f,.02f},{.62f,.57f,.40f},{2.62f,1.64f,.73f},{1.70f,1.10f,.52f}
    };
    static const float sigma_a[4][3] = {
        {.001f,.001f,.001f},{.012f,.015f,.025f},{.011f,.013f,.019f},{.003f,.004f,.005f}
    };
    static const float wt[4]  = {0.0557f, 0.0902f, 0.1459f, 0.2360f};
    static const float mfp[4] = {0.5f, 0.809f, 1.309f, 2.118f};
    float N[3] = {vnorm[0], vnorm[1], vnorm[2]}; sr_norm3(N);
    float V[3] = {sr->cam_pos[0]-vpos[0], sr->cam_pos[1]-vpos[1], sr->cam_pos[2]-vpos[2]};
    sr_norm3(V);
    float L[3] = {0.f, 1.f, 0.f};
    if(sr->n_lights > 0){
        if(sr->lights[0].type == 1){
            L[0] = -sr->lights[0].pos_x; L[1] = -sr->lights[0].pos_y; L[2] = -sr->lights[0].pos_z;
        } else {
            L[0] = sr->lights[0].pos_x - vpos[0];
            L[1] = sr->lights[0].pos_y - vpos[1];
            L[2] = sr->lights[0].pos_z - vpos[2];
        }
        sr_norm3(L);
    }
    float sss[3] = {0.f, 0.f, 0.f};
    for(int layer = 0; layer < 4; layer++){
        float path = mfp[layer];
        for(int c = 0; c < 3; c++){
            float ext = sigma_s[layer][c] + sigma_a[layer][c];
            sss[c] += wt[layer]*base[c]*rl_expf(-ext*path)*sigma_s[layer][c]/(ext + 1e-7f);
        }
    }
    float NdotL = sr_dot3(N, L), wrap = sr_maxf(NdotL*.6f + .4f, 0.f);
    float H[3] = {V[0]+L[0], V[1]+L[1], V[2]+L[2]}; sr_norm3(H);
    float NdotH = sr_maxf(sr_dot3(N,H),0.f), NdotV = sr_maxf(sr_dot3(N,V),0.f);
    float HdotV = sr_maxf(sr_dot3(H,V),0.f);
    float F0s = (ior-1.f)*(ior-1.f)/((ior+1.f)*(ior+1.f));
    float F0[3] = {F0s, F0s, F0s}, F[3]; sr_F_Schlick(HdotV, F0, F);
    float a2 = roughness*roughness*roughness*roughness;
    float D = sr_D_GGX(NdotH,a2), G = sr_G_Smith(NdotV,sr_maxf(NdotL,0.f),roughness);
    float spec = D*G*F[0] / (4.f*NdotV*sr_maxf(NdotL,0.f) + 1e-4f);
    float lc[3] = {1.f,.96f,.88f}, lint = 1.f;
    if(sr->n_lights > 0){
        lc[0]=sr->lights[0].color_r; lc[1]=sr->lights[0].color_g;
        lc[2]=sr->lights[0].color_b; lint =sr->lights[0].intensity;
    }
    float shadow = 1.f;
    if(sr->shadow_fbo_id < RIG_GPU_MAX_FBOS){
        float lp[4]={vpos[0],vpos[1],vpos[2],1.f}, lcp[4]; sr_mv4(sr->light_vp,lp,lcp);
        if(rl_fabsf(lcp[3]) > 1e-5f){
            float su=(lcp[0]/lcp[3])*.5f+.5f, sv=(lcp[1]/lcp[3])*.5f+.5f, sd=lcp[2]/lcp[3]*.5f+.5f;
            shadow = sr_shadow_pcf(sr, sr->shadow_fbo_id, su, sv, sd);
        }
    }
    float amb[3]; sr_sh_irr(sr->ibl_sh, N, amb);
    float r=(sss[0]*wrap+spec)*lc[0]*lint*shadow+amb[0]*base[0]*.25f;
    float g=(sss[1]*wrap+spec)*lc[1]*lint*shadow+amb[1]*base[1]*.25f;
    float b=(sss[2]*wrap+spec)*lc[2]*lint*shadow+amb[2]*base[2]*.25f;
    if(NdotL < 0.f){ float bk=sr_maxf(-NdotL,0.f)*.3f; r+=sss[0]*bk*lc[0]; g+=sss[1]*bk*lc[1]; b+=sss[2]*bk*lc[2]; }
    *out = sr_pack(sr_srgb(sr_aces(r)), sr_srgb(sr_aces(g)), sr_srgb(sr_aces(b)));
    (void)vtang;
}

static void sr_shade_hair(SRBackend *sr, uint32_t prog,
    const float *vpos, const float *vnorm, const float *vtang,
    const float *vuv,  const float *vcol,  uint32_t *out)
{
    float melanin_eu=.4f, melanin_ph=.2f, rough1=.3f, rough2=.5f, shift=.08f, thick=.7f;
    SRUniform *u;
    if((u=sr_uget(sr,prog,"u_melanin_eu"))   && u->set) melanin_eu = u->val.f;
    if((u=sr_uget(sr,prog,"uMelaninEu"))     && u->set) melanin_eu = u->val.f;
    if((u=sr_uget(sr,prog,"u_melanin_ph"))   && u->set) melanin_ph = u->val.f;
    if((u=sr_uget(sr,prog,"u_kajiya_shift")) && u->set) shift      = u->val.f;
    if((u=sr_uget(sr,prog,"uRoughness1"))    && u->set) rough1     = u->val.f;
    if((u=sr_uget(sr,prog,"uRoughness2"))    && u->set) rough2     = u->val.f;
    if((u=sr_uget(sr,prog,"uHairThick"))     && u->set) thick      = u->val.f;
    float hair[3] = {
        rl_expf(-melanin_eu*.26f)*(1.f+melanin_ph*.20f),
        rl_expf(-melanin_eu*.48f)*(1.f+melanin_ph*.05f),
        rl_expf(-melanin_eu*.90f)
    };
    float trans[3] = {
        rl_expf(-thick*melanin_eu),
        rl_expf(-thick*melanin_eu)*.72f,
        rl_expf(-thick*melanin_eu)*.46f
    };
    float T[3]={vtang[0],vtang[1],vtang[2]}; sr_norm3(T);
    float N[3]={vnorm[0],vnorm[1],vnorm[2]}; sr_norm3(N);
    float V[3]={sr->cam_pos[0]-vpos[0],sr->cam_pos[1]-vpos[1],sr->cam_pos[2]-vpos[2]};
    sr_norm3(V);
    float L[3]={0.f,1.f,0.f}, lc[3]={1.f,1.f,1.f}, lint=1.f;
    if(sr->n_lights > 0){
        SRLight *lgt = &sr->lights[0];
        if(lgt->type==1){ L[0]=-lgt->pos_x; L[1]=-lgt->pos_y; L[2]=-lgt->pos_z; }
        else { L[0]=lgt->pos_x-vpos[0]; L[1]=lgt->pos_y-vpos[1]; L[2]=lgt->pos_z-vpos[2]; }
        sr_norm3(L);
        lc[0]=lgt->color_r; lc[1]=lgt->color_g; lc[2]=lgt->color_b; lint=lgt->intensity;
    }
    float TdotL  = sr_dot3(T, L);
    float diffuse = rl_sqrtf(sr_maxf(0.f, 1.f - TdotL*TdotL));
    float H[3] = {V[0]+L[0],V[1]+L[1],V[2]+L[2]}; sr_norm3(H);
    float T1[3] = {T[0]+shift*N[0], T[1]+shift*N[1], T[2]+shift*N[2]}; sr_norm3(T1);
    float shift2 = shift * RL_PHI;
    float T2[3] = {T[0]-shift2*.5f*N[0], T[1]-shift2*.5f*N[1], T[2]-shift2*.5f*N[2]}; sr_norm3(T2);
    float TH1 = sr_dot3(T1,H), sinTH1 = rl_sqrtf(sr_maxf(0.f,1.f-TH1*TH1));
    float spec1 = rl_powf(sinTH1, 1.f/(rough1*rough1 + 1e-4f));
    float TH2 = sr_dot3(T2,H), sinTH2 = rl_sqrtf(sr_maxf(0.f,1.f-TH2*TH2));
    float spec2 = rl_powf(sinTH2, 1.f/(rough2*rough2 + 1e-4f));
    float TdotV = sr_dot3(T,V), sinTV = rl_sqrtf(sr_maxf(0.f,1.f-TdotV*TdotV));
    float transmission = sinTV * sr_maxf(-TdotL, 0.f);
    float NdotL = sr_maxf(sr_dot3(N,L), .05f);
    float r = sr_srgb(sr_aces((hair[0]*diffuse+spec1*.45f)*lc[0]*lint*NdotL+(spec2*hair[0]*.20f)*lc[0]*lint+trans[0]*transmission*lc[0]*lint*.3f));
    float g = sr_srgb(sr_aces((hair[1]*diffuse+spec1*.45f)*lc[1]*lint*NdotL+(spec2*hair[1]*.20f)*lc[1]*lint+trans[1]*transmission*lc[1]*lint*.3f));
    float b = sr_srgb(sr_aces((hair[2]*diffuse+spec1*.45f)*lc[2]*lint*NdotL+(spec2*hair[2]*.20f)*lc[2]*lint+trans[2]*transmission*lc[2]*lint*.3f));
    *out = sr_pack(r, g, b);
    (void)vuv; (void)vcol;
}

static void sr_shade_holo(SRBackend *sr, uint32_t prog,
    const float *vpos, const float *vnorm, const float *vtang,
    const float *vuv,  const float *vcol,  uint32_t *out)
{
    float scanfreq=100.f, ca_str=.003f, holo_intensity=1.f;
    SRUniform *u;
    if((u=sr_uget(sr,prog,"u_scanline_freq")) && u->set) scanfreq       = u->val.f;
    if((u=sr_uget(sr,prog,"uHoloIntensity"))  && u->set) holo_intensity = u->val.f;
    if((u=sr_uget(sr,prog,"uChrAberration"))  && u->set) ca_str         = u->val.f;
    float N[3]={vnorm[0],vnorm[1],vnorm[2]}; sr_norm3(N);
    float V[3]={sr->cam_pos[0]-vpos[0],sr->cam_pos[1]-vpos[1],sr->cam_pos[2]-vpos[2]};
    sr_norm3(V);
    float NdotV = sr_maxf(sr_dot3(N, V), 0.f);
    float rim = rl_powf(1.f - NdotV, RL_PHI) * holo_intensity;
    float fringes=0.f, famp=1.f;
    for(int i=0; i<6; i++){
        fringes += famp * rl_sinf(vpos[1]*scanfreq*rl_powf(RL_PHI,(float)i) + sr->time_s*2.f*(float)(i+1));
        famp *= RL_PHI_INV;
    }
    float scanline_mask = ((fringes*.5f + .5f) > .75f) ? .25f : 1.f;
    float flicker = sr_phi_hash(rl_floorf(sr->time_s*8.f), 42.f);
    if(flicker > .93f) scanline_mask *= .1f;
    float sp_r = sr_fbm(vuv[0]*6.f + sr->time_s*.08f + ca_str, vuv[1]*6.f, 6);
    float sp_g = sr_fbm(vuv[0]*6.f + sr->time_s*.08f,          vuv[1]*6.f, 6);
    float sp_b = sr_fbm(vuv[0]*6.f + sr->time_s*.08f - ca_str, vuv[1]*6.f, 6);
    float theta = rl_atan2f(NdotV, 1.f - NdotV), lambda = rl_sinf(theta)*RL_PHI_INV;
    float inter = 0.f;
    for(int s=0; s<16; s++){
        float ang = (float)s * SR_GOLDEN_ANGLE;
        float px2 = rl_cosf(ang)*rl_sqrtf((float)s+1.f)*.1f;
        float py2 = rl_sinf(ang)*rl_sqrtf((float)s+1.f)*.1f;
        float dx = vuv[0]-px2, dy = vuv[1]-py2;
        inter += rl_sinf(rl_sqrtf(dx*dx+dy*dy)*SR_PI*scanfreq*RL_PHI_INV + sr->time_s);
    }
    inter = inter/16.f*.5f + .5f;
    float r = sr_clamp((sr_clamp(.5f+lambda*1.4f,0,1)*(sp_r*.5f+.5f)+rim+inter*.3f)*scanline_mask,0,1);
    float g = sr_clamp((sr_clamp(.8f-lambda*.3f, 0,1)*(sp_g*.5f+.5f)+rim*.5f+inter*.2f)*scanline_mask,0,1);
    float b = sr_clamp((sr_clamp(.5f-lambda*1.f, 0,1)*(sp_b*.5f+.5f)+rim+inter*.3f)*scanline_mask,0,1);
    *out = sr_pack(r, g, b);
    (void)vtang; (void)vcol;
}

static void sr_shade_silk(SRBackend *sr, uint32_t prog,
    const float *vpos, const float *vnorm, const float *vtang,
    const float *vuv,  const float *vcol,  uint32_t *out)
{
    float color[3]={.9f,.85f,.78f}, freq=80.f; SRUniform *u;
    if((u=sr_uget(sr,prog,"uSilkColor")) && u->set){ color[0]=u->val.v3[0]; color[1]=u->val.v3[1]; color[2]=u->val.v3[2]; }
    if((u=sr_uget(sr,prog,"uWeaveFreq")) && u->set) freq=u->val.f;
    color[0]*=vcol[0]; color[1]*=vcol[1]; color[2]*=vcol[2];
    float warp = rl_sinf(vuv[0]*freq*SR_PI)*.5f+.5f, weft = rl_sinf(vuv[1]*freq*SR_PI*RL_PHI)*.5f+.5f;
    float weave = warp*weft;
    float N[3]={vnorm[0],vnorm[1],vnorm[2]}; sr_norm3(N);
    float T[3]={vtang[0],vtang[1],vtang[2]}; sr_norm3(T);
    float B[3]; sr_cross3(N, T, B); sr_norm3(B);
    float V[3]={sr->cam_pos[0]-vpos[0],sr->cam_pos[1]-vpos[1],sr->cam_pos[2]-vpos[2]}; sr_norm3(V);
    float L[3]={0.f,1.f,0.f};
    if(sr->n_lights>0){
        if(sr->lights[0].type==1){ L[0]=-sr->lights[0].pos_x; L[1]=-sr->lights[0].pos_y; L[2]=-sr->lights[0].pos_z; }
        else{ L[0]=sr->lights[0].pos_x-vpos[0]; L[1]=sr->lights[0].pos_y-vpos[1]; L[2]=sr->lights[0].pos_z-vpos[2]; }
        sr_norm3(L);
    }
    float H[3]={V[0]+L[0],V[1]+L[1],V[2]+L[2]}; sr_norm3(H);
    float ax=sr_lerpf(.05f,.3f,1.f-weave), ay=sr_lerpf(.3f,.05f,1.f-weave)*RL_PHI;
    float HdotT=sr_dot3(H,T), HdotB=sr_dot3(H,B), NdotH=sr_maxf(sr_dot3(N,H),0.f);
    float NdotL=sr_maxf(sr_dot3(N,L),0.f), NdotV=sr_maxf(sr_dot3(N,V),1e-3f);
    float ward_exp = -(HdotT*HdotT/(ax*ax)+HdotB*HdotB/(ay*ay))/(1.f-NdotH*NdotH+1e-7f);
    float ward = rl_expf(ward_exp)/(4.f*ax*ay*rl_sqrtf(NdotL*NdotV)+1e-4f);
    float diffuse = NdotL*(1.f-weave*.6f);
    float lc[3]={1,1,1}, lint=1.f;
    if(sr->n_lights>0){ lc[0]=sr->lights[0].color_r; lc[1]=sr->lights[0].color_g; lc[2]=sr->lights[0].color_b; lint=sr->lights[0].intensity; }
    *out = sr_pack(sr_srgb(sr_aces((color[0]*diffuse*SR_INV_PI+ward*.5f)*lc[0]*lint)),
                   sr_srgb(sr_aces((color[1]*diffuse*SR_INV_PI+ward*.5f)*lc[1]*lint)),
                   sr_srgb(sr_aces((color[2]*diffuse*SR_INV_PI+ward*.5f)*lc[2]*lint)));
}

static void sr_shade_velvet(SRBackend *sr, uint32_t prog,
    const float *vpos, const float *vnorm, const float *vtang,
    const float *vuv,  const float *vcol,  uint32_t *out)
{
    float color[3]={.3f,.05f,.4f}; SRUniform *u;
    if((u=sr_uget(sr,prog,"uVelvetColor")) && u->set){ color[0]=u->val.v3[0]; color[1]=u->val.v3[1]; color[2]=u->val.v3[2]; }
    color[0]*=vcol[0]; color[1]*=vcol[1]; color[2]*=vcol[2];
    float N[3]={vnorm[0],vnorm[1],vnorm[2]}; sr_norm3(N);
    float V[3]={sr->cam_pos[0]-vpos[0],sr->cam_pos[1]-vpos[1],sr->cam_pos[2]-vpos[2]}; sr_norm3(V);
    float L[3]={0.f,1.f,0.f};
    if(sr->n_lights>0){
        if(sr->lights[0].type==1){ L[0]=-sr->lights[0].pos_x; L[1]=-sr->lights[0].pos_y; L[2]=-sr->lights[0].pos_z; }
        else{ L[0]=sr->lights[0].pos_x-vpos[0]; L[1]=sr->lights[0].pos_y-vpos[1]; L[2]=sr->lights[0].pos_z-vpos[2]; }
        sr_norm3(L);
    }
    float NdotL=sr_maxf(sr_dot3(N,L),0.f), NdotV=sr_maxf(sr_dot3(N,V),1e-3f);
    float sig2=.64f, A=1.f-sig2/(2.f*(sig2+.33f)), B2=.45f*sig2/(sig2+.09f);
    float tL=rl_atan2f(rl_sqrtf(1.f-NdotL*NdotL),NdotL), tV=rl_atan2f(rl_sqrtf(1.f-NdotV*NdotV),NdotV);
    float lp[3]={L[0]-N[0]*NdotL,L[1]-N[1]*NdotL,L[2]-N[2]*NdotL};
    float vp2[3]={V[0]-N[0]*NdotV,V[1]-N[1]*NdotV,V[2]-N[2]*NdotV};
    float cosfi = sr_maxf(sr_dot3(lp,vp2),0.f);
    float ON = NdotL*(A + B2*cosfi*rl_sinf(sr_maxf(tL,tV))*rl_tanf(sr_minf(tL,tV)));
    float sheen = rl_powf(1.f-NdotV,RL_PHI)*sr_maxf(NdotL,0.f);
    float lc[3]={1,1,1}, lint=1.f;
    if(sr->n_lights>0){ lc[0]=sr->lights[0].color_r; lc[1]=sr->lights[0].color_g; lc[2]=sr->lights[0].color_b; lint=sr->lights[0].intensity; }
    *out = sr_pack(sr_srgb(sr_aces((color[0]*ON+sheen*color[0]*.8f)*lc[0]*lint)),
                   sr_srgb(sr_aces((color[1]*ON+sheen*color[1]*.8f)*lc[1]*lint)),
                   sr_srgb(sr_aces((color[2]*ON+sheen*color[2]*.8f)*lc[2]*lint)));
    (void)vtang; (void)vuv; (void)prog;
}

static void sr_shade_carbon(SRBackend *sr, uint32_t prog,
    const float *vpos, const float *vnorm, const float *vtang,
    const float *vuv,  const float *vcol,  uint32_t *out)
{
    float N_tiles=16.f; SRUniform *u;
    if((u=sr_uget(sr,prog,"uCarbonTiles")) && u->set) N_tiles=u->val.f;
    float gu=rl_floorf(vuv[0]*N_tiles), gv=rl_floorf(vuv[1]*N_tiles);
    int cell = (int)(gu+gv) % 2;
    float fu=vuv[0]*N_tiles-gu, fv=vuv[1]*N_tiles-gv;
    float fiber_t = cell ? fu : fv, cross_t = cell ? fv : fu;
    float fiber_w = rl_expf(-cross_t*cross_t*30.f);
    float N[3]={vnorm[0],vnorm[1],vnorm[2]}; sr_norm3(N);
    float T[3];
    if(cell){ T[0]=vtang[0]; T[1]=vtang[1]; T[2]=vtang[2]; }
    else    { T[0]=vnorm[1]; T[1]=-vnorm[0]; T[2]=0.f; }
    sr_norm3(T);
    float NF[3]={
        N[0]+rl_sinf(fiber_t*SR_PI)*T[0]*.5f,
        N[1]+rl_sinf(fiber_t*SR_PI)*T[1]*.5f,
        N[2]+rl_sinf(fiber_t*SR_PI)*T[2]*.5f
    }; sr_norm3(NF);
    float V[3]={sr->cam_pos[0]-vpos[0],sr->cam_pos[1]-vpos[1],sr->cam_pos[2]-vpos[2]}; sr_norm3(V);
    float L[3]={0.f,1.f,0.f};
    if(sr->n_lights>0){
        if(sr->lights[0].type==1){ L[0]=-sr->lights[0].pos_x; L[1]=-sr->lights[0].pos_y; L[2]=-sr->lights[0].pos_z; }
        else{ L[0]=sr->lights[0].pos_x-vpos[0]; L[1]=sr->lights[0].pos_y-vpos[1]; L[2]=sr->lights[0].pos_z-vpos[2]; }
        sr_norm3(L);
    }
    float H[3]={V[0]+L[0],V[1]+L[1],V[2]+L[2]}; sr_norm3(H);
    float NdotH=sr_maxf(sr_dot3(NF,H),0.f), NdotV=sr_maxf(sr_dot3(NF,V),0.f);
    float NdotL=sr_maxf(sr_dot3(NF,L),0.f), HdotV=sr_maxf(sr_dot3(H,V),0.f);
    float F0[3]={.04f,.04f,.04f}, F[3]; sr_F_Schlick(HdotV,F0,F);
    float rough=sr_lerpf(.08f,.45f,1.f-fiber_w), a2=rough*rough*rough*rough;
    float D=sr_D_GGX(NdotH,a2), G=sr_G_Smith(NdotV,NdotL,rough);
    float spec=D*G*F[0]/(4.f*NdotV*NdotL+1e-4f);
    float col=sr_lerpf(.02f,.08f,fiber_w);
    float lc[3]={1,1,1}, lint=1.f;
    if(sr->n_lights>0){ lc[0]=sr->lights[0].color_r; lc[1]=sr->lights[0].color_g; lc[2]=sr->lights[0].color_b; lint=sr->lights[0].intensity; }
    *out = sr_pack(sr_srgb(sr_aces((col*NdotL*SR_INV_PI+spec)*lc[0]*lint)),
                   sr_srgb(sr_aces((col*NdotL*SR_INV_PI+spec)*lc[1]*lint)),
                   sr_srgb(sr_aces((col*NdotL*SR_INV_PI+spec)*lc[2]*lint)));
    (void)vcol; (void)prog;
}

static void sr_shade_denim(SRBackend *sr, uint32_t prog,
    const float *vpos, const float *vnorm, const float *vtang,
    const float *vuv,  const float *vcol,  uint32_t *out)
{
    float N_tiles=24.f, wear=.4f; SRUniform *u;
    if((u=sr_uget(sr,prog,"uDenimWear"))  && u->set) wear    = u->val.f;
    if((u=sr_uget(sr,prog,"uDenimTiles")) && u->set) N_tiles = u->val.f;
    int gu2=(int)rl_floorf(vuv[0]*N_tiles), gv2=(int)rl_floorf(vuv[1]*N_tiles);
    float is_indigo = ((gu2+gv2)%3 != 1) ? 1.f : 0.f;
    float fade = sr_fbm(vuv[0]*.5f, vuv[1]*.5f, 4)*wear;
    float col[3]={
        sr_lerpf(.85f,.08f+fade*.3f, is_indigo)*vcol[0],
        sr_lerpf(.82f,.12f+fade*.25f,is_indigo)*vcol[1],
        sr_lerpf(.68f,.38f+fade*.4f, is_indigo)*vcol[2]
    };
    float N[3]={vnorm[0],vnorm[1],vnorm[2]}; sr_norm3(N);
    float V[3]={sr->cam_pos[0]-vpos[0],sr->cam_pos[1]-vpos[1],sr->cam_pos[2]-vpos[2]}; sr_norm3(V);
    float L[3]={0.f,1.f,0.f};
    if(sr->n_lights>0){
        if(sr->lights[0].type==1){ L[0]=-sr->lights[0].pos_x; L[1]=-sr->lights[0].pos_y; L[2]=-sr->lights[0].pos_z; }
        else{ L[0]=sr->lights[0].pos_x-vpos[0]; L[1]=sr->lights[0].pos_y-vpos[1]; L[2]=sr->lights[0].pos_z-vpos[2]; }
        sr_norm3(L);
    }
    float NdotL=sr_maxf(sr_dot3(N,L),0.f), NdotV=sr_maxf(sr_dot3(N,V),1e-3f);
    float sig2=.25f, A2=1.f-sig2/(2.f*(sig2+.33f)), B3=.45f*sig2/(sig2+.09f);
    float tL=rl_atan2f(rl_sqrtf(1.f-NdotL*NdotL),NdotL), tV=rl_atan2f(rl_sqrtf(1.f-NdotV*NdotV),NdotV);
    float ON = NdotL*(A2 + B3*.5f*rl_sinf(sr_maxf(tL,tV))*rl_tanf(sr_minf(tL,tV)));
    float lc[3]={1,1,1}, lint=1.f;
    if(sr->n_lights>0){ lc[0]=sr->lights[0].color_r; lc[1]=sr->lights[0].color_g; lc[2]=sr->lights[0].color_b; lint=sr->lights[0].intensity; }
    *out = sr_pack(sr_srgb(sr_aces(col[0]*ON*lc[0]*lint)),
                   sr_srgb(sr_aces(col[1]*ON*lc[1]*lint)),
                   sr_srgb(sr_aces(col[2]*ON*lc[2]*lint)));
    (void)vtang; (void)vpos; (void)prog;
}

static void sr_shade_marble(SRBackend *sr, uint32_t prog,
    const float *vpos, const float *vnorm, const float *vtang,
    const float *vuv,  const float *vcol,  uint32_t *out)
{
    float fbm_val = sr_fbm(vuv[0]*2.f, vuv[1]*2.f, 6);
    float vein  = rl_powf(rl_fabsf(rl_sinf((vuv[0]*2.5f+fbm_val*2.8f)*SR_PI)), RL_PHI);
    float vein2 = rl_powf(rl_fabsf(rl_sinf((vuv[1]*4.f*RL_PHI+fbm_val*1.5f)*SR_PI)), SR_PHI2)*.4f;
    float t = sr_clamp(vein+vein2, 0.f, 1.f);
    float gold_t = sr_fbm(vuv[0]*8.f, vuv[1]*8.f, 3)*.25f;
    float col[3]={
        sr_lerpf(sr_lerpf(.96f,.52f,t),.75f,gold_t),
        sr_lerpf(sr_lerpf(.95f,.49f,t),.65f,gold_t),
        sr_lerpf(sr_lerpf(.93f,.44f,t),.30f,gold_t)
    };
    float N[3]={vnorm[0],vnorm[1],vnorm[2]}; sr_norm3(N);
    float V[3]={sr->cam_pos[0]-vpos[0],sr->cam_pos[1]-vpos[1],sr->cam_pos[2]-vpos[2]}; sr_norm3(V);
    float L[3]={0.f,1.f,0.f};
    if(sr->n_lights>0){
        if(sr->lights[0].type==1){ L[0]=-sr->lights[0].pos_x; L[1]=-sr->lights[0].pos_y; L[2]=-sr->lights[0].pos_z; }
        else{ L[0]=sr->lights[0].pos_x-vpos[0]; L[1]=sr->lights[0].pos_y-vpos[1]; L[2]=sr->lights[0].pos_z-vpos[2]; }
        sr_norm3(L);
    }
    float H[3]={V[0]+L[0],V[1]+L[1],V[2]+L[2]}; sr_norm3(H);
    float NdotL=sr_maxf(sr_dot3(N,L),0.f), NdotH=sr_maxf(sr_dot3(N,H),0.f);
    float NdotV=sr_maxf(sr_dot3(N,V),1e-3f), HdotV=sr_maxf(sr_dot3(H,V),0.f);
    float rough=sr_lerpf(.02f,.15f,t), a2=rough*rough*rough*rough;
    float F0[3]={.04f,.04f,.04f}, F[3]; sr_F_Schlick(HdotV,F0,F);
    float spec=sr_D_GGX(NdotH,a2)*sr_G_Smith(NdotV,NdotL,rough)*F[0]/(4.f*NdotV*NdotL+1e-4f);
    float lc[3]={1,1,1}, lint=1.f;
    if(sr->n_lights>0){ lc[0]=sr->lights[0].color_r; lc[1]=sr->lights[0].color_g; lc[2]=sr->lights[0].color_b; lint=sr->lights[0].intensity; }
    *out = sr_pack(sr_srgb(sr_aces((col[0]*NdotL*SR_INV_PI+spec*1.5f)*lc[0]*lint)),
                   sr_srgb(sr_aces((col[1]*NdotL*SR_INV_PI+spec*1.5f)*lc[1]*lint)),
                   sr_srgb(sr_aces((col[2]*NdotL*SR_INV_PI+spec*1.5f)*lc[2]*lint)));
    (void)vtang; (void)vcol; (void)prog;
}

static void sr_shade_skybox(SRBackend *sr, uint32_t prog,
    const float *vpos, const float *vnorm, const float *vtang,
    const float *vuv,  const float *vcol,  uint32_t *out)
{
    float D[3]={vnorm[0],vnorm[1],vnorm[2]}; sr_norm3(D);
    float sun[3]={sr->light_dir[0],sr->light_dir[1],sr->light_dir[2]};
    if(sun[0]==0.f && sun[1]==0.f && sun[2]==0.f) sun[1]=1.f;
    sr_norm3(sun);
    float theta = rl_atan2f(rl_sqrtf(D[0]*D[0]+D[2]*D[2]), D[1]);
    float dsx=D[0]-sun[0], dsy=D[1]-sun[1], dsz=D[2]-sun[2];
    float gamma_val = rl_atan2f(rl_sqrtf(dsx*dsx+dsy*dsy+dsz*dsz), 1.f);
    float turb = 2.f; SRUniform *us = sr_uget(sr, prog, "uTurbidity");
    if(us && us->set) turb = us->val.f;
    float AY=-.0193f*turb-.2592f, BY=-.0665f*turb+.0008f, CY=-.0004f*turb+.2125f;
    float DY=-.0641f*turb-.8989f, EY= .0022f*turb+.0452f;
    float cos_theta=rl_cosf(theta), cos_gamma=rl_cosf(gamma_val);
    float Y = (1.f+AY*rl_expf(BY/sr_maxf(cos_theta,.001f))) *
              (1.f+CY*rl_expf(DY*gamma_val)+EY*cos_gamma*cos_gamma);
    Y = sr_clamp(Y*.3f, 0.f, 1.f);
    float mie   = rl_powf(sr_maxf(cos_gamma, 0.f), 512.f)*2.f;
    float horiz = rl_powf(sr_clamp(1.f-cos_theta, 0.f, 1.f), RL_PHI);
    *out = sr_pack(sr_srgb(sr_aces(Y*.08f+mie+horiz*.6f)),
                   sr_srgb(sr_aces(Y*.18f+mie+horiz*.4f)),
                   sr_srgb(sr_aces(Y*.65f+mie+horiz*.2f)));
    (void)vpos; (void)vtang; (void)vuv; (void)vcol;
}

static SRShaderFn sr_pick_shader(SRBackend *sr, uint32_t prog){
    if(sr_uget(sr,prog,"u_melanin")    || sr_uget(sr,prog,"uMelanin"))    return sr_shade_skin;
    if(sr_uget(sr,prog,"u_kajiya_shift"))                                   return sr_shade_hair;
    if(sr_uget(sr,prog,"u_scanline_freq"))                                  return sr_shade_holo;
    if(sr_uget(sr,prog,"uWeaveFreq")   || sr_uget(sr,prog,"uSilkColor"))   return sr_shade_silk;
    if(sr_uget(sr,prog,"uVelvetColor"))                                     return sr_shade_velvet;
    if(sr_uget(sr,prog,"uCarbonTiles"))                                     return sr_shade_carbon;
    if(sr_uget(sr,prog,"uDenimWear")   || sr_uget(sr,prog,"uDenimTiles"))  return sr_shade_denim;
    if(sr_uget(sr,prog,"uTurbidity"))                                        return sr_shade_skybox;
    SRUniform *mt = sr_uget(sr, prog, "u_mat_type");
    if(mt && mt->set){
        switch(mt->val.i){
            case 1: return sr_shade_silk;
            case 2: return sr_shade_velvet;
            case 3: return sr_shade_denim;
            case 4: return sr_shade_carbon;
            case 9: return sr_shade_skybox;
            case 10: return sr_shade_marble;
            default: break;
        }
    }
    return sr_shade_pbr;
}

static inline float sr_edge(float ax, float ay, float bx, float by, float px, float py){
    return (px-ax)*(by-ay) - (py-ay)*(bx-ax);
}

static void sr_raster_triangle(SRBackend *sr, uint32_t prog,
    const SRVertex *v0, const SRVertex *v1, const SRVertex *v2,
    uint32_t y0, uint32_t y1)
{
    uint32_t W = sr->active_w, H = sr->active_h, pitch = sr->active_pitch32;
    if(!sr->active_color || !sr->active_depth) return;
    float iw0=1.f/v0->pos[3], iw1=1.f/v1->pos[3], iw2=1.f/v2->pos[3];
    float sx0=(v0->pos[0]*iw0*.5f+.5f)*(float)W, sy0=(1.f-v0->pos[1]*iw0*.5f-.5f)*(float)H, sz0=v0->pos[2]*iw0;
    float sx1=(v1->pos[0]*iw1*.5f+.5f)*(float)W, sy1=(1.f-v1->pos[1]*iw1*.5f-.5f)*(float)H, sz1=v1->pos[2]*iw1;
    float sx2=(v2->pos[0]*iw2*.5f+.5f)*(float)W, sy2=(1.f-v2->pos[1]*iw2*.5f-.5f)*(float)H, sz2=v2->pos[2]*iw2;
    int minx=(int)sr_maxf(sr_minf(sr_minf(sx0,sx1),sx2),0.f);
    int maxx=(int)sr_minf(sr_maxf(sr_maxf(sx0,sx1),sx2)+1.f,(float)(W-1));
    int miny=(int)sr_maxf(sr_maxf(sr_minf(sr_minf(sy0,sy1),sy2),0.f),(float)y0);
    int maxy=(int)sr_minf(sr_minf(sr_maxf(sr_maxf(sy0,sy1),sy2)+1.f,(float)(H-1)),(float)(y1-1));
    if(minx>maxx || miny>maxy) return;
    float area = sr_edge(sx0,sy0,sx1,sy1,sx2,sy2);
    if(rl_fabsf(area) < 0.5f) return;
    float inv_area = 1.f / area;
    SRShaderFn shade = sr_pick_shader(sr, prog);
    for(int py = miny; py <= maxy; py++){
        for(int px = minx; px <= maxx; px++){
            float pfx=(float)px+.5f, pfy=(float)py+.5f;
            float w0=sr_edge(sx1,sy1,sx2,sy2,pfx,pfy)*inv_area;
            float w1=sr_edge(sx2,sy2,sx0,sy0,pfx,pfy)*inv_area;
            float w2=1.f-w0-w1;
            if(w0<0.f || w1<0.f || w2<0.f) continue;
            float iw=w0*iw0+w1*iw1+w2*iw2; if(iw<1e-7f) continue;
            float inv_iw=1.f/iw;
            float l0=w0*iw0*inv_iw, l1=w1*iw1*inv_iw, l2=w2*iw2*inv_iw;
            float z=sz0*l0+sz1*l1+sz2*l2;
            uint32_t idx=(uint32_t)py*pitch+(uint32_t)px;
            if(sr->state.depth_test && z>=sr->active_depth[idx]) continue;
            if(sr->state.depth_write) sr->active_depth[idx]=z;
            float wp[3],wn[3],wt[3],wuv[2],wc[4];
            for(int c=0;c<3;c++){
                wp[c]=v0->world[c]*l0+v1->world[c]*l1+v2->world[c]*l2;
                wn[c]=v0->normal[c]*l0+v1->normal[c]*l1+v2->normal[c]*l2;
                wt[c]=v0->tangent[c]*l0+v1->tangent[c]*l1+v2->tangent[c]*l2;
            }
            for(int c=0;c<2;c++) wuv[c]=v0->uv[c]*l0+v1->uv[c]*l1+v2->uv[c]*l2;
            for(int c=0;c<4;c++) wc[c]=v0->color[c]*l0+v1->color[c]*l1+v2->color[c]*l2;
            sr_norm3(wn);
            uint32_t pixel=0;
            shade(sr,prog,wp,wn,wt,wuv,wc,&pixel);
            sr->active_color[idx]=pixel;
        }
    }
}

static void* sr_worker(void *arg){
    SRBackend *sr = (SRBackend*)arg;
    for(;;){
        rl_mutex_lock(&sr->job_mutex);
        while(sr->job_head == sr->job_tail && !sr->workers_exit)
            rl_cond_wait(&sr->job_cond, &sr->job_mutex);
        if(sr->workers_exit){ rl_mutex_unlock(&sr->job_mutex); break; }
        SRTileJob job = sr->job_queue[sr->job_head % SR_MAX_JOBS];
        sr->job_head++;
        rl_mutex_unlock(&sr->job_mutex);
        sr_raster_triangle(sr, job.prog_id, &job.v0, &job.v1, &job.v2, job.y0, job.y1);
        rl_mutex_lock(&sr->job_mutex);
        if(sr->jobs_pending > 0) sr->jobs_pending--;
        if(sr->jobs_pending == 0) rl_cond_signal(&sr->done_cond);
        rl_mutex_unlock(&sr->job_mutex);
    }
    return NULL;
}

static void sr_workers_init(SRBackend *sr){
    rl_mutex_init(&sr->job_mutex);
    rl_cond_init(&sr->job_cond);
    rl_cond_init(&sr->done_cond);
    sr->job_head = sr->job_tail = sr->jobs_pending = 0;
    sr->workers_exit = false;
#ifndef RIG_THREAD_NONE
    /* Crear los SR_WORKERS hilos reales solo cuando hay backend de SO */
    for(int i = 0; i < SR_WORKERS; i++)
        rl_thread_create(&sr->workers[i], sr_worker, sr);
#endif
}

static void sr_workers_destroy(SRBackend *sr){
#ifndef RIG_THREAD_NONE
    rl_mutex_lock(&sr->job_mutex);
    sr->workers_exit = true;
    rl_cond_broadcast(&sr->job_cond);
    rl_mutex_unlock(&sr->job_mutex);
    for(int i = 0; i < SR_WORKERS; i++) rl_thread_join(&sr->workers[i]);
#endif
    rl_mutex_destroy(&sr->job_mutex);
    rl_cond_destroy(&sr->job_cond);
    rl_cond_destroy(&sr->done_cond);
}

static void sr_workers_flush(SRBackend *sr){
#ifndef RIG_THREAD_NONE
    rl_mutex_lock(&sr->job_mutex);
    while(sr->jobs_pending > 0) rl_cond_wait(&sr->done_cond, &sr->job_mutex);
    rl_mutex_unlock(&sr->job_mutex);
#else
    (void)sr; /* modo monohilo: flush inmediato, nada pendiente */
#endif
}

static void sr_submit_triangle(SRBackend *sr,
    const SRVertex *v0, const SRVertex *v1, const SRVertex *v2)
{
#ifdef RIG_THREAD_NONE
    /* Modo monohilo: rasterización síncrona directa, sin cola */
    sr_raster_triangle(sr, sr->bound_program, v0, v1, v2, 0, sr->active_h);
#else
    uint32_t H=sr->active_h, tile_h=(H+SR_WORKERS-1)/SR_WORKERS;
    rl_mutex_lock(&sr->job_mutex);
    for(int t=0; t<SR_WORKERS; t++){
        uint32_t ty0=(uint32_t)(t*tile_h), ty1=ty0+tile_h;
        if(ty1 > H) ty1 = H;
        if(ty0 >= H) break;
        if((sr->job_tail - sr->job_head) >= SR_MAX_JOBS) break;
        SRTileJob *job = &sr->job_queue[sr->job_tail % SR_MAX_JOBS];
        job->y0=ty0; job->y1=ty1;
        job->v0=*v0; job->v1=*v1; job->v2=*v2;
        job->prog_id = sr->bound_program;
        sr->job_tail++; sr->jobs_pending++;
    }
    rl_cond_broadcast(&sr->job_cond);
    rl_mutex_unlock(&sr->job_mutex);
#endif
}

int rig_gpu_init(RigGPUCtx *gpu, void *native_display, void *native_window, bool offscreen){
    rl_memset(gpu, 0, sizeof(*gpu)); gpu->verbose = true;
    SRBackend *sr = (SRBackend*)rl_calloc(1, sizeof(SRBackend)); if(!sr) return -1;
    sr->fb_w=1920; sr->fb_h=1080; sr->fb_pitch32=1920;
    if(native_display){
        RigDRMCtx *drm = (RigDRMCtx*)native_display;
        if(drm->bufs[0].map){
            sr->primary_fb    = (uint32_t*)drm->bufs[drm->cur_buf].map;
            sr->fb_pitch32    = drm->bufs[drm->cur_buf].pitch / 4;
        }
    }
    if(!sr->primary_fb){
        sr->primary_fb = (uint32_t*)rl_malloc(sr->fb_w * sr->fb_h * 4);
        if(!sr->primary_fb){ rl_free(sr); return -1; }
    }
    sr->primary_zbuf = (float*)rl_malloc(sr->fb_w * sr->fb_h * sizeof(float));
    if(!sr->primary_zbuf){ rl_free(sr); return -1; }
    for(uint32_t i=0; i < sr->fb_w*sr->fb_h; i++) sr->primary_zbuf[i] = 1.f;
    sr->active_color  = sr->primary_fb;
    sr->active_depth  = sr->primary_zbuf;
    sr->active_w      = sr->fb_w;
    sr->active_h      = sr->fb_h;
    sr->active_pitch32 = sr->fb_pitch32;
    sr->active_fbo_id = 0xFFFFFFFFu;
    sr->shadow_fbo_id = 0xFFFFFFFFu;
    sr->ibl_intensity = 0.4f;
    rl_memset(sr->ibl_sh, 0, sizeof(sr->ibl_sh));
    sr->ibl_sh[0][0]=0.3f; sr->ibl_sh[1][0]=0.3f; sr->ibl_sh[2][0]=0.35f;
    sr->state.depth_test=true; sr->state.depth_write=true; sr->state.cull=true;
    sr->state.blend=false; sr->state.depth_func=0x0201;
    sr->state.viewport_w=sr->fb_w; sr->state.viewport_h=sr->fb_h;  /* FIX-1 */
    sr->lights[0].pos_x=0.f; sr->lights[0].pos_y=1.f; sr->lights[0].pos_z=0.5f;
    sr->lights[0].color_r=1.f; sr->lights[0].color_g=0.96f; sr->lights[0].color_b=0.88f;
    sr->lights[0].intensity=1.f; sr->lights[0].type=1; sr->n_lights=1;
    sr_m4id(sr->mvp); sr_m4id(sr->model); sr_m4id(sr->view); sr_m4id(sr->proj);
    sr->cam_pos[0]=0.f; sr->cam_pos[1]=0.f; sr->cam_pos[2]=3.f;
    sr_workers_init(sr);
    gpu->egl_display = (void*)sr; gpu->initialized = true;
    gpu->caps.max_texture_size   = 8192;
    gpu->caps.max_fbo_attachments = 4;
    rl_strncpy(gpu->caps.renderer_string, "RIGCOM SR φ=1.618 SOBERANO", sizeof(gpu->caps.renderer_string)-1);
    rl_strncpy(gpu->caps.version_string, "MASTER v1.0", sizeof(gpu->caps.version_string)-1);
    g_gpu_ctx = gpu;
    (void)native_window; (void)offscreen;
    return 0;
}

void rig_gpu_destroy(RigGPUCtx *gpu){
    if(!gpu->initialized) return;
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    sr_workers_destroy(sr);
    rl_free(sr->primary_zbuf);
    for(int i=0; i<RIG_GPU_MAX_VBOS; i++)     if(sr->buf_data[i]) rl_free(sr->buf_data[i]);
    for(int i=0; i<RIG_GPU_MAX_TEXTURES; i++) if(sr->tex_data[i]) rl_free(sr->tex_data[i]);
    for(int i=0; i<RIG_GPU_MAX_FBOS; i++){
        if(sr->fbo_color_owned[i] && sr->fbo_color[i]) rl_free(sr->fbo_color[i]);  /* FIX-5 */
        if(sr->fbo_depth[i]) rl_free(sr->fbo_depth[i]);
    }
    rl_free(sr);
    gpu->egl_display = NULL; gpu->initialized = false;
    if(g_gpu_ctx == gpu) g_gpu_ctx = NULL;
}

void rig_gpu_present(RigGPUCtx *gpu) {
    if(!gpu || !gpu->initialized) return;
    gpu->swap_count++;
    gpu->last_swap_ns = gpu->frame_count * UINT64_C(16666667);
}

bool rig_gpu_make_current(RigGPUCtx *gpu)         { g_gpu_ctx = gpu; return true; }

void rig_gpu_release_thread(RigGPUCtx *gpu)       { if(g_gpu_ctx==gpu) g_gpu_ctx=NULL; }

void rig_gpu_detect_caps(RigGPUCtx *gpu) {
    if(!gpu) return;
    gpu->caps.compute_shaders=false;
    gpu->caps.ssbo=false;
    gpu->caps.indirect_draw=false;
    gpu->caps.half_float_vertex=true;
    gpu->caps.anisotropic_filter=false;
    gpu->caps.max_anisotropy=1.f;
    gpu->caps.max_texture_size=8192;
    gpu->caps.max_fbo_attachments=4;
}

void rig_gpu_print_caps(const RigGPUCtx *gpu)     { rl_dprintf(RL_STDOUT,"[MASTER] %s\n", gpu->caps.renderer_string); }

void rig_gpu_frame_begin(RigGPUCtx *gpu){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    sr->time_s += 0.016667f;
    gpu->stats.draw_calls = gpu->stats.triangles = gpu->stats.shader_switches = 0;
    gpu->stats.texture_binds = gpu->stats.state_changes = 0;
    gpu->frame_count++;
}

uint32_t rig_shader_compile(RigGPUCtx *gpu, RigShaderStage stage, const char *src, const char *debug_name){
    if(gpu->n_shaders >= RIG_GPU_MAX_SHADERS) return RIG_GPU_INVALID_ID;
    uint32_t id = gpu->n_shaders++; RigShader *s = &gpu->shaders[id];
    s->id=id; s->gl_id=id; s->stage=stage; s->compiled=true;
    if(debug_name) rl_strncpy(s->src_hash, debug_name, sizeof(s->src_hash)-1);
    (void)src; return id;
}

uint32_t rig_program_link(RigGPUCtx *gpu, uint32_t vert_id, uint32_t frag_id){
    if(gpu->n_programs >= RIG_GPU_MAX_PROGRAMS) return RIG_GPU_INVALID_ID;
    uint32_t id = gpu->n_programs++; RigProgram *p = &gpu->programs[id];
    p->id=id; p->gl_id=id; p->linked=true;
    if(vert_id < gpu->n_shaders) p->vert = &gpu->shaders[vert_id];
    if(frag_id < gpu->n_shaders) p->frag = &gpu->shaders[frag_id];
    return id;
}

uint32_t rig_program_link_compute(RigGPUCtx *gpu, uint32_t comp_id){ return rig_program_link(gpu, comp_id, comp_id); }

void rig_program_bind(RigGPUCtx *gpu, uint32_t prog_id){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    sr->bound_program = prog_id; gpu->bound_program = prog_id; gpu->stats.shader_switches++;
}

void rig_shader_free(RigGPUCtx *gpu, uint32_t id){ if(id < gpu->n_shaders) gpu->shaders[id].compiled = false; }

void rig_program_free(RigGPUCtx *gpu, uint32_t id){ if(id < gpu->n_programs) gpu->programs[id].linked = false; }

void rig_uniform_1i(RigGPUCtx *gpu, uint32_t prog, const char *name, int v){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    SRUniform *u = sr_uset(sr, prog, name, SR_UNIF_INT); if(u) u->val.i = v;
    sr_fast_cache(sr, prog, name);
}

void rig_uniform_1f(RigGPUCtx *gpu, uint32_t prog, const char *name, float v){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    SRUniform *u = sr_uset(sr, prog, name, SR_UNIF_FLOAT); if(u) u->val.f = v;
    sr_fast_cache(sr, prog, name);
}

void rig_uniform_2f(RigGPUCtx *gpu, uint32_t prog, const char *name, float x, float y){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    SRUniform *u = sr_uset(sr, prog, name, SR_UNIF_VEC2);
    if(u){ u->val.v2[0]=x; u->val.v2[1]=y; }
}

void rig_uniform_3f(RigGPUCtx *gpu, uint32_t prog, const char *name, float x, float y, float z){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    SRUniform *u = sr_uset(sr, prog, name, SR_UNIF_VEC3);
    if(u){ u->val.v3[0]=x; u->val.v3[1]=y; u->val.v3[2]=z; }
    if(rl_strcmp(name,"uCamPos")==0 || rl_strcmp(name,"u_cam_pos")==0){
        sr->cam_pos[0]=x; sr->cam_pos[1]=y; sr->cam_pos[2]=z;
    }
    if(rl_strncmp(name,"uLPos",5)==0 && sr->n_lights < SR_MAX_LIGHTS){
        /* FIX-2: i siempre era 0; actualizamos luz 0 correctamente */
        sr->lights[0].pos_x=x; sr->lights[0].pos_y=y; sr->lights[0].pos_z=z;
    }
    if(rl_strncmp(name,"uLCol",5)==0 && sr->n_lights > 0){
        sr->lights[0].color_r=x; sr->lights[0].color_g=y; sr->lights[0].color_b=z;
    }
    sr_fast_cache(sr, prog, name);
}

void rig_uniform_4f(RigGPUCtx *gpu, uint32_t prog, const char *name, float x, float y, float z, float w){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    SRUniform *u = sr_uset(sr, prog, name, SR_UNIF_VEC4);
    if(u){ u->val.v4[0]=x; u->val.v4[1]=y; u->val.v4[2]=z; u->val.v4[3]=w; }
}

void rig_uniform_mat4(RigGPUCtx *gpu, uint32_t prog, const char *name, const float *m16){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    SRUniform *u = sr_uset(sr, prog, name, SR_UNIF_MAT4);
    if(u) rl_memcpy(u->val.m16, m16, 64);
    sr_fast_cache(sr, prog, name);
}

uint32_t rig_buffer_create(RigGPUCtx *gpu, RigBufType type, const void *data, size_t size, RigBufUsage usage){
    if(gpu->n_buffers >= RIG_GPU_MAX_VBOS) return RIG_GPU_INVALID_ID;
    SRBackend *sr = sr_get(gpu); if(!sr) return RIG_GPU_INVALID_ID;
    uint32_t id = gpu->n_buffers++; RigBuffer *b = &gpu->buffers[id];
    b->id=id; b->gl_id=id; b->type=type; b->size_bytes=size; b->usage=usage;
    sr->buf_data[id] = rl_malloc(size); sr->buf_size[id] = size;
    if(data && sr->buf_data[id]) rl_memcpy(sr->buf_data[id], data, size);
    else if(sr->buf_data[id])    rl_memset(sr->buf_data[id], 0, size);
    return id;
}

void rig_buffer_update(RigGPUCtx *gpu, uint32_t buf_id, const void *data, size_t size, size_t offset){
    SRBackend *sr = sr_get(gpu);
    if(!sr || buf_id>=RIG_GPU_MAX_VBOS || !sr->buf_data[buf_id]) return;
    if(offset + size > sr->buf_size[buf_id]) return;
    rl_memcpy((uint8_t*)sr->buf_data[buf_id] + offset, data, size);
}

void rig_buffer_bind(RigGPUCtx *gpu, uint32_t buf_id, uint32_t slot){
    SRBackend *sr=sr_get(gpu);
    if(!sr || slot>=16 || buf_id>=gpu->n_buffers) return;
    sr->bound_buffers[slot]=buf_id;
}

void rig_buffer_free(RigGPUCtx *gpu, uint32_t buf_id){
    SRBackend *sr = sr_get(gpu);
    if(!sr || buf_id >= RIG_GPU_MAX_VBOS) return;
    rl_free(sr->buf_data[buf_id]); sr->buf_data[buf_id]=NULL; sr->buf_size[buf_id]=0;
}

RigVAO* rig_vao_create(RigGPUCtx *gpu, uint32_t vbo_id, uint32_t ibo_id,
    const RigVertexAttrib *attribs, uint8_t n_attribs,
    uint32_t n_vertices, uint32_t n_indices)
{
    RigVAO *vao = (RigVAO*)rl_calloc(1, sizeof(RigVAO)); if(!vao) return NULL;
    if(vbo_id < gpu->n_buffers) vao->vbo = &gpu->buffers[vbo_id];
    if(ibo_id < gpu->n_buffers) vao->ibo = &gpu->buffers[ibo_id];
    vao->n_vertices=n_vertices; vao->n_indices=n_indices; vao->gl_vao_id=vbo_id;
    if(attribs && n_attribs <= 8){
        rl_memcpy(vao->attribs, attribs, n_attribs*sizeof(RigVertexAttrib));
        vao->n_attribs = n_attribs;
    }
    return vao;
}

void rig_vao_bind(RigGPUCtx *gpu, RigVAO *vao){
    if(!vao) return;
    gpu->bound_vao = vao->gl_vao_id;  /* FIX-3: eliminada auto-asignación */
}

void rig_vao_free(RigGPUCtx *gpu, RigVAO *vao){ (void)gpu; rl_free(vao); }

static size_t sr_tex_bpp(RigTexFormat fmt){
    switch(fmt){
        case RIG_TEX_RGBA8:         return 4;
        case RIG_TEX_RGBA16F:       return 8;
        case RIG_TEX_RGBA32F:       return 16;
        case RIG_TEX_RG16F:         return 4;
        case RIG_TEX_R8:            return 1;
        case RIG_TEX_R16F:          return 2;
        case RIG_TEX_DEPTH24:       return 4;
        case RIG_TEX_DEPTH32F:      return 4;
        case RIG_TEX_CUBEMAP_RGBA16F: return 8;
        default:                    return 4;
    }
}

uint32_t rig_texture_create(RigGPUCtx *gpu, RigTexFormat fmt, uint32_t w, uint32_t h, const void *data, bool gen_mips){
    if(gpu->n_textures >= RIG_GPU_MAX_TEXTURES) return RIG_GPU_INVALID_ID;
    SRBackend *sr = sr_get(gpu); if(!sr) return RIG_GPU_INVALID_ID;
    uint32_t id = gpu->n_textures++; RigTexture *t = &gpu->textures[id];
    t->id=id; t->gl_id=id; t->format=fmt; t->width=w; t->height=h; t->mipmaps=gen_mips;
    size_t sz = (size_t)w * h * sr_tex_bpp(fmt);
    sr->tex_data[id] = rl_malloc(sz);
    if(data && sr->tex_data[id])  rl_memcpy(sr->tex_data[id], data, sz);
    else if(sr->tex_data[id])     rl_memset(sr->tex_data[id], 0, sz);
    return id;
}

uint32_t rig_texture_create_cubemap(RigGPUCtx *gpu, RigTexFormat fmt, uint32_t size, const void *faces[6]){
    size_t face_sz = (size_t)size * size * sr_tex_bpp(fmt);
    if(gpu->n_textures >= RIG_GPU_MAX_TEXTURES) return RIG_GPU_INVALID_ID;
    SRBackend *sr = sr_get(gpu); if(!sr) return RIG_GPU_INVALID_ID;
    uint32_t id = gpu->n_textures++; RigTexture *t = &gpu->textures[id];
    t->id=id; t->gl_id=id; t->format=fmt; t->width=size; t->height=size; t->is_cubemap=true;
    sr->tex_data[id] = rl_malloc(face_sz * 6);
    if(sr->tex_data[id] && faces)
        for(int i=0; i<6; i++) if(faces[i]) rl_memcpy((uint8_t*)sr->tex_data[id]+i*face_sz, faces[i], face_sz);
    return id;
}

void rig_texture_bind(RigGPUCtx *gpu, uint32_t tex_id, uint32_t slot){ gpu->stats.texture_binds++; (void)tex_id; (void)slot; }

void rig_texture_free(RigGPUCtx *gpu, uint32_t tex_id){
    SRBackend *sr = sr_get(gpu);
    if(!sr || tex_id >= RIG_GPU_MAX_TEXTURES) return;
    rl_free(sr->tex_data[tex_id]); sr->tex_data[tex_id] = NULL;
}

uint32_t rig_fbo_create(RigGPUCtx *gpu, uint32_t w, uint32_t h, RigTexFormat color_fmt, bool with_depth){
    if(gpu->n_fbos >= RIG_GPU_MAX_FBOS) return RIG_GPU_INVALID_ID;
    SRBackend *sr = sr_get(gpu); if(!sr) return RIG_GPU_INVALID_ID;
    uint32_t id = gpu->n_fbos++; RigFBO *fbo = &gpu->fbos[id];
    fbo->id=id; fbo->gl_id=id; fbo->width=w; fbo->height=h;
    size_t color_sz = (size_t)w * h * sr_tex_bpp(color_fmt);
    sr->fbo_color[id] = (uint32_t*)rl_malloc(color_sz);
    sr->fbo_w[id]=w; sr->fbo_h[id]=h;
    sr->fbo_color_owned[id] = true;    /* marca: este buffer nos pertenece */
    if(sr->fbo_color[id]) rl_memset(sr->fbo_color[id], 0, color_sz);
    if(with_depth){
        sr->fbo_depth[id] = (float*)rl_malloc(w * h * sizeof(float));
        if(sr->fbo_depth[id])
            for(uint32_t i=0; i < w*h; i++) sr->fbo_depth[id][i] = 1.f;
    }
    if(gpu->n_textures < RIG_GPU_MAX_TEXTURES){
        uint32_t tex_id = gpu->n_textures++; RigTexture *t = &gpu->textures[tex_id];
        t->id=tex_id; t->gl_id=tex_id; t->format=color_fmt; t->width=w; t->height=h;
        sr->tex_data[tex_id] = sr->fbo_color[id]; /* alias — no liberar doble */
        fbo->color[0]=t; fbo->n_color=1;
    }
    return id;
}

uint32_t rig_fbo_create_msaa(RigGPUCtx *gpu, uint32_t w, uint32_t h, uint8_t samples){
    (void)samples; return rig_fbo_create(gpu, w, h, RIG_TEX_RGBA8, true);
}

void rig_fbo_bind(RigGPUCtx *gpu, uint32_t fbo_id){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    if(fbo_id >= RIG_GPU_MAX_FBOS || !sr->fbo_color[fbo_id]) return;
    sr->active_color   = sr->fbo_color[fbo_id];
    sr->active_depth   = sr->fbo_depth[fbo_id];
    sr->active_w       = sr->fbo_w[fbo_id];
    sr->active_h       = sr->fbo_h[fbo_id];
    sr->active_pitch32 = sr->fbo_w[fbo_id];
    sr->active_fbo_id  = fbo_id;
    gpu->bound_fbo     = fbo_id;
}

void rig_fbo_unbind(RigGPUCtx *gpu){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    sr->active_color   = sr->primary_fb;
    sr->active_depth   = sr->primary_zbuf;
    sr->active_w       = sr->fb_w;
    sr->active_h       = sr->fb_h;
    sr->active_pitch32 = sr->fb_pitch32;
    sr->active_fbo_id  = 0xFFFFFFFFu;
    gpu->bound_fbo     = 0;
}

void rig_fbo_blit(RigGPUCtx *gpu, uint32_t src, uint32_t dst, uint32_t w, uint32_t h, bool color, bool depth){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    uint32_t *sc = (src==0xFFFFFFFFu)?sr->primary_fb   :(src<RIG_GPU_MAX_FBOS?sr->fbo_color[src]:NULL);
    uint32_t *dc = (dst==0xFFFFFFFFu)?sr->primary_fb   :(dst<RIG_GPU_MAX_FBOS?sr->fbo_color[dst]:NULL);
    float    *sd = (src==0xFFFFFFFFu)?sr->primary_zbuf :(src<RIG_GPU_MAX_FBOS?sr->fbo_depth[src]:NULL);
    float    *dd = (dst==0xFFFFFFFFu)?sr->primary_zbuf :(dst<RIG_GPU_MAX_FBOS?sr->fbo_depth[dst]:NULL);
    if(color && sc && dc) rl_memcpy(dc, sc, (size_t)w*h*4);
    if(depth && sd && dd) rl_memcpy(dd, sd, (size_t)w*h*sizeof(float));
}

void rig_fbo_free(RigGPUCtx *gpu, uint32_t fbo_id){
    SRBackend *sr = sr_get(gpu);
    if(!sr || fbo_id >= RIG_GPU_MAX_FBOS) return;
    if(sr->fbo_color_owned[fbo_id] && sr->fbo_color[fbo_id])  /* FIX-5 */
        rl_free(sr->fbo_color[fbo_id]);
    rl_free(sr->fbo_depth[fbo_id]);
    sr->fbo_color[fbo_id]=NULL; sr->fbo_depth[fbo_id]=NULL;
    sr->fbo_color_owned[fbo_id]=false;
    sr->fbo_w[fbo_id]=sr->fbo_h[fbo_id]=0;
}

void rig_state_default(RigGPUCtx *gpu){
    SRBackend *sr=sr_get(gpu); if(!sr) return;
    sr->state.depth_test=true; sr->state.depth_write=true;
    sr->state.blend=false; sr->state.cull=true; gpu->stats.state_changes++;
}

void rig_state_depth(RigGPUCtx *gpu, bool test, bool write, uint32_t func){
    SRBackend *sr=sr_get(gpu); if(!sr) return;
    sr->state.depth_test=test; sr->state.depth_write=write;
    sr->state.depth_func=func; gpu->stats.state_changes++;
}

void rig_state_blend(RigGPUCtx *gpu, bool enable, uint32_t src, uint32_t dst){
    SRBackend *sr=sr_get(gpu); if(!sr) return;
    sr->state.blend=enable; gpu->stats.state_changes++;
    (void)src; (void)dst;
}

void rig_state_cull(RigGPUCtx *gpu, bool enable, uint32_t mode){
    SRBackend *sr=sr_get(gpu); if(!sr) return;
    sr->state.cull=enable; gpu->stats.state_changes++;
    (void)mode;
}

void rig_state_viewport(RigGPUCtx *gpu, uint32_t x, uint32_t y, uint32_t w, uint32_t h){
    SRBackend *sr=sr_get(gpu); if(!sr) return;
    sr->state.viewport_x=x; sr->state.viewport_y=y;  /* FIX-1 */
    sr->state.viewport_w=w; sr->state.viewport_h=h;
}

void rig_state_clear(RigGPUCtx *gpu, float r, float g, float b, float a, float depth){
    SRBackend *sr=sr_get(gpu);
    if(!sr || !sr->active_color) return;
    uint32_t pixel = sr_pack(sr_srgb(r), sr_srgb(g), sr_srgb(b));
    uint32_t npix  = sr->active_w * sr->active_h;
    for(uint32_t i=0; i<npix; i++) sr->active_color[i] = pixel;
    if(sr->active_depth) for(uint32_t i=0; i<npix; i++) sr->active_depth[i] = depth;
    (void)a;
}

void rig_draw_indexed(RigGPUCtx *gpu, RigVAO *vao, uint32_t offset, uint32_t count){
    SRBackend *sr = sr_get(gpu);
    if(!sr || !vao || !vao->vbo || !vao->ibo) return;
    uint32_t vbo_id = vao->vbo->gl_id, ibo_id = vao->ibo->gl_id;
    if(vbo_id >= RIG_GPU_MAX_VBOS || !sr->buf_data[vbo_id]) return;
    if(ibo_id >= RIG_GPU_MAX_VBOS || !sr->buf_data[ibo_id]) return;
    const float    *verts   = (const float*)sr->buf_data[vbo_id];
    const uint32_t *indices = (const uint32_t*)sr->buf_data[ibo_id];
    uint32_t prog = sr->bound_program;
    float mvp_local[16];
    SRUniform *u = sr_uget(sr, prog, "u_mvp");
    if(u && u->set) rl_memcpy(mvp_local, u->val.m16, 64);
    else { float vp[16]; sr_mm4(sr->proj, sr->view, vp); sr_mm4(vp, sr->model, mvp_local); }
    uint32_t n_tris = count / 3;
    for(uint32_t tri = 0; tri < n_tris; tri++){
        uint32_t i0=indices[offset+tri*3], i1=indices[offset+tri*3+1], i2=indices[offset+tri*3+2];
        const float *d0=verts+i0*SR_FLOATS_PER_VERT;
        const float *d1=verts+i1*SR_FLOATS_PER_VERT;
        const float *d2=verts+i2*SR_FLOATS_PER_VERT;
        SRVertex sv0, sv1, sv2;
        float p0[4]={d0[0],d0[1],d0[2],1.f}, p1[4]={d1[0],d1[1],d1[2],1.f}, p2[4]={d2[0],d2[1],d2[2],1.f};
        sr_mv4(mvp_local,p0,sv0.pos); sr_mv4(mvp_local,p1,sv1.pos); sr_mv4(mvp_local,p2,sv2.pos);
        if(sr->state.cull){
            float ax=sv1.pos[0]/sv1.pos[3]-sv0.pos[0]/sv0.pos[3];
            float ay=sv1.pos[1]/sv1.pos[3]-sv0.pos[1]/sv0.pos[3];
            float bx=sv2.pos[0]/sv2.pos[3]-sv0.pos[0]/sv0.pos[3];
            float by=sv2.pos[1]/sv2.pos[3]-sv0.pos[1]/sv0.pos[3];
            if(ax*by - ay*bx <= 0.f) continue;
        }
        float wp0[4],wp1[4],wp2[4];
        sr_mv4(sr->model,p0,wp0); sr_mv4(sr->model,p1,wp1); sr_mv4(sr->model,p2,wp2);
        for(int c=0;c<3;c++){ sv0.world[c]=wp0[c]; sv1.world[c]=wp1[c]; sv2.world[c]=wp2[c]; }
        float n0[4]={d0[3],d0[4],d0[5],0.f}, n1[4]={d1[3],d1[4],d1[5],0.f}, n2[4]={d2[3],d2[4],d2[5],0.f};
        float wn0[4],wn1[4],wn2[4];
        sr_mv4(sr->model,n0,wn0); sr_mv4(sr->model,n1,wn1); sr_mv4(sr->model,n2,wn2);
        for(int c=0;c<3;c++){ sv0.normal[c]=wn0[c]; sv1.normal[c]=wn1[c]; sv2.normal[c]=wn2[c]; }
        sv0.tangent[0]=d0[6]; sv0.tangent[1]=d0[7]; sv0.tangent[2]=d0[8];
        sv1.tangent[0]=d1[6]; sv1.tangent[1]=d1[7]; sv1.tangent[2]=d1[8];
        sv2.tangent[0]=d2[6]; sv2.tangent[1]=d2[7]; sv2.tangent[2]=d2[8];
        sv0.uv[0]=d0[9];  sv0.uv[1]=d0[10]; sv1.uv[0]=d1[9];  sv1.uv[1]=d1[10];
        sv2.uv[0]=d2[9];  sv2.uv[1]=d2[10];
        sv0.color[0]=d0[11]; sv0.color[1]=d0[12]; sv0.color[2]=d0[13]; sv0.color[3]=d0[14];
        sv1.color[0]=d1[11]; sv1.color[1]=d1[12]; sv1.color[2]=d1[13]; sv1.color[3]=d1[14];
        sv2.color[0]=d2[11]; sv2.color[1]=d2[12]; sv2.color[2]=d2[13]; sv2.color[3]=d2[14];
        sr_submit_triangle(sr, &sv0, &sv1, &sv2); gpu->stats.triangles++;
    }
    sr_workers_flush(sr); gpu->stats.draw_calls++;
}

void rig_draw_arrays(RigGPUCtx *gpu, uint32_t mode, uint32_t first, uint32_t count){
    SRBackend *sr=sr_get(gpu);
    if(!sr || count<3 || sr->bound_vao_id>=SR_MAX_VAOS) return;
    SRVAOState *state=&sr->vao_states[sr->bound_vao_id];
    if(!state->active || state->vbo_id>=RIG_GPU_MAX_VBOS ||
       !sr->buf_data[state->vbo_id]) return;
    uint32_t stride=state->attrib_stride[0];
    if(!stride) stride=state->attrib_nc[0]*sizeof(float);
    if(stride<2u*sizeof(float)) return;
    const uint8_t *base=(const uint8_t*)sr->buf_data[state->vbo_id];
    uint32_t triangles=(mode==0x0005u) ? count-2u : count/3u;
    if(mode!=0x0004u && mode!=0x0005u) return;
    float mvp[16];
    SRUniform *u=sr_uget(sr,sr->bound_program,"u_mvp");
    if(u&&u->set) rl_memcpy(mvp,u->val.m16,sizeof(mvp));
    else { float vp[16]; sr_mm4(sr->proj,sr->view,vp); sr_mm4(vp,sr->model,mvp); }
    for(uint32_t tri=0;tri<triangles;tri++){
        uint32_t ids[3];
        if(mode==0x0005u){ ids[0]=first+tri; ids[1]=first+tri+1u; ids[2]=first+tri+2u; }
        else { ids[0]=first+tri*3u; ids[1]=ids[0]+1u; ids[2]=ids[0]+2u; }
        if(mode==0x0005u && (tri&1u)){ uint32_t swap=ids[0]; ids[0]=ids[1]; ids[1]=swap; }
        SRVertex v[3]; rl_memset(v,0,sizeof(v));
        for(int k=0;k<3;k++){
            const float *src=(const float*)(base+(size_t)ids[k]*stride+state->attrib_offset[0]);
            float p[4]={src[0],src[1],state->attrib_nc[0]>2?src[2]:0.f,1.f};
            sr_mv4(mvp,p,v[k].pos);
            v[k].world[0]=p[0]; v[k].world[1]=p[1]; v[k].world[2]=p[2];
            v[k].normal[2]=1.f; v[k].tangent[0]=1.f;
            v[k].color[0]=v[k].color[1]=v[k].color[2]=v[k].color[3]=1.f;
        }
        sr_submit_triangle(sr,&v[0],&v[1],&v[2]);
    }
    sr_workers_flush(sr); gpu->stats.draw_calls++; gpu->stats.triangles+=triangles;
}

void rig_draw_screen_quad(RigGPUCtx *gpu, RigPBRPipeline *pbr){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    if(pbr->fbo_gbuffer < RIG_GPU_MAX_FBOS
       && sr->fbo_color[pbr->fbo_gbuffer] && sr->primary_fb){
        uint32_t npix = sr->fb_w * sr->fb_h;
        for(uint32_t i=0; i<npix; i++) sr->primary_fb[i] = sr->fbo_color[pbr->fbo_gbuffer][i];
    }
}

int rig_pbr_pipeline_init(RigGPUCtx *gpu, RigPBRPipeline *pbr, uint32_t w, uint32_t h){
    SRBackend *sr = sr_get(gpu); if(!sr) return -1;
    rl_memset(pbr, 0, sizeof(*pbr)); pbr->width=w; pbr->height=h;
    sr->fb_w=w; sr->fb_h=h; sr->active_w=w; sr->active_h=h;
    rl_free(sr->primary_zbuf);
    sr->primary_zbuf = (float*)rl_malloc(w*h*sizeof(float)); if(!sr->primary_zbuf) return -1;
    for(uint32_t i=0; i<w*h; i++) sr->primary_zbuf[i]=1.f;
    if(!sr->primary_fb){
        sr->primary_fb=(uint32_t*)rl_malloc(w*h*4); if(!sr->primary_fb) return -1;
        sr->fb_pitch32=w;
    }
    sr->active_color=sr->primary_fb; sr->active_depth=sr->primary_zbuf; sr->active_pitch32=sr->fb_pitch32;
    pbr->fbo_gbuffer = rig_fbo_create(gpu, w, h, RIG_TEX_RGBA8, true);
    pbr->fbo_hdr     = rig_fbo_create(gpu, w, h, RIG_TEX_RGBA32F, true);
    pbr->fbo_shadow  = rig_fbo_create(gpu, 2048, 2048, RIG_TEX_R8, true);
    sr->shadow_fbo_id = pbr->fbo_shadow;
    pbr->fbo_bloom   = rig_fbo_create(gpu, w/2, h/2, RIG_TEX_RGBA8, false);
    pbr->tex_brdf_lut = rig_texture_create(gpu, RIG_TEX_RG16F, 512, 512, NULL, false);
    pbr->prog_geometry    = rig_program_link(gpu,0,0);
    pbr->prog_lighting    = rig_program_link(gpu,0,0);
    pbr->prog_pbr_forward = rig_program_link(gpu,0,0);
    pbr->prog_shadow      = rig_program_link(gpu,0,0);
    pbr->prog_bloom_down  = rig_program_link(gpu,0,0);
    pbr->prog_bloom_up    = rig_program_link(gpu,0,0);
    pbr->prog_tonemap     = rig_program_link(gpu,0,0);
    pbr->prog_sss         = rig_program_link(gpu,0,0);
    pbr->prog_ssao        = rig_program_link(gpu,0,0);
    pbr->prog_skybox      = rig_program_link(gpu,0,0);
    pbr->initialized = true;
    return 0;
}

void rig_pbr_pipeline_destroy(RigGPUCtx *gpu, RigPBRPipeline *pbr){
    if(!pbr->initialized) return;
    rig_fbo_free(gpu, pbr->fbo_gbuffer); rig_fbo_free(gpu, pbr->fbo_hdr);
    rig_fbo_free(gpu, pbr->fbo_shadow);  rig_fbo_free(gpu, pbr->fbo_bloom);
    rig_texture_free(gpu, pbr->tex_brdf_lut);
    pbr->initialized = false;
}

void rig_pbr_pipeline_resize(RigGPUCtx *gpu, RigPBRPipeline *pbr, uint32_t w, uint32_t h){
    rig_pbr_pipeline_destroy(gpu, pbr); rig_pbr_pipeline_init(gpu, pbr, w, h);
}

void rig_pbr_begin_geometry(RigGPUCtx *gpu, RigPBRPipeline *pbr){ rig_fbo_bind(gpu, pbr->fbo_gbuffer); }

void rig_pbr_end_geometry(RigGPUCtx *gpu, RigPBRPipeline *pbr){
    rig_fbo_blit(gpu, pbr->fbo_gbuffer, pbr->fbo_hdr, pbr->width, pbr->height, true, true);
    rig_fbo_unbind(gpu);
}

void rig_pbr_lighting_pass(RigGPUCtx *gpu, RigPBRPipeline *pbr){
    if(!gpu || !pbr || !pbr->initialized) return;
    rig_fbo_blit(gpu,pbr->fbo_gbuffer,pbr->fbo_hdr,pbr->width,pbr->height,true,false);
}

void rig_pbr_sss_pass(RigGPUCtx *gpu, RigPBRPipeline *pbr) {
    SRBackend *sr=sr_get(gpu);
    if(!sr || !pbr || pbr->fbo_hdr>=RIG_GPU_MAX_FBOS) return;
    uint32_t *color=sr->fbo_color[pbr->fbo_hdr];
    uint32_t w=sr->fbo_w[pbr->fbo_hdr], h=sr->fbo_h[pbr->fbo_hdr];
    if(!color || w<3 || h<3) return;
    for(uint32_t y=1;y+1<h;y++) for(uint32_t x=1;x+1<w;x++){
        uint32_t a=color[y*w+x-1], b=color[y*w+x], c=color[y*w+x+1];
        uint32_t r=(((a>>16)&255u)+2u*((b>>16)&255u)+((c>>16)&255u))/4u;
        uint32_t g=(((a>>8)&255u)+2u*((b>>8)&255u)+((c>>8)&255u))/4u;
        uint32_t bl=((a&255u)+2u*(b&255u)+(c&255u))/4u;
        color[y*w+x]=(r<<16)|(g<<8)|bl;
    }
}

void rig_pbr_ssao_pass(RigGPUCtx *gpu, RigPBRPipeline *pbr){
    SRBackend *sr = sr_get(gpu);
    if(!sr || pbr->fbo_gbuffer >= RIG_GPU_MAX_FBOS) return;
    uint32_t W=pbr->width, H=pbr->height;
    float    *depth = sr->fbo_depth[pbr->fbo_gbuffer];
    uint32_t *color = sr->fbo_color[pbr->fbo_gbuffer];
    if(!depth || !color) return;
    for(uint32_t py=0; py<H; py++) for(uint32_t px=0; px<W; px++){
        float z0 = depth[py*W+px]; if(z0 >= 1.f) continue;
        float occ = 0.f;
        for(int s=0; s<16; s++){
            int sx=(int)px+(int)(SR_SSAO_KX[s]*(float)W*.02f);
            int sy=(int)py+(int)(SR_SSAO_KY[s]*(float)H*.02f);
            if(sx<0) { sx=0; }
            if(sx>=(int)W) { sx=(int)W-1; }
            if(sy<0) { sy=0; }
            if(sy>=(int)H) { sy=(int)H-1; }
            float sz = depth[sy*W+sx];
            float range = sr_maxf(0.f, 1.f - rl_fabsf(z0-sz)*10.f);
            occ += (sz < z0 - 0.001f ? 1.f : 0.f) * range;
        }
        occ /= 16.f; float ao = 1.f - occ*.7f;
        uint32_t p = color[py*W+px];
        color[py*W+px] = ((uint32_t)((float)((p>>16)&0xFF)*ao)<<16) |
                          ((uint32_t)((float)((p>> 8)&0xFF)*ao)<< 8) |
                           (uint32_t)((float)( p     &0xFF)*ao);
    }
}

void rig_pbr_bloom_pass(RigGPUCtx *gpu, RigPBRPipeline *pbr, float threshold, float strength){
    SRBackend *sr = sr_get(gpu);
    if(!sr || pbr->fbo_gbuffer >= RIG_GPU_MAX_FBOS) return;
    uint32_t W=pbr->width, H=pbr->height;
    uint32_t *src = sr->fbo_color[pbr->fbo_gbuffer]; if(!src) return;
    float str = strength>0.f?strength:0.3f, thr = threshold>0.f?threshold:0.8f;
    float radii[6] = {1.f, RL_PHI, SR_PHI2, SR_PHI3, SR_PHI4, SR_PHI5};
    uint32_t *tmp = (uint32_t*)rl_malloc(W*H*4); if(!tmp) return;
    rl_memcpy(tmp, src, W*H*4);
    for(int pass=0; pass<6; pass++){
        int rad = (int)(radii[pass]+.5f);
        for(uint32_t py=0; py<H; py++) for(uint32_t px=0; px<W; px++){
            uint32_t p = tmp[py*W+px];
            float r2=((p>>16)&0xFF)/255.f, g2=((p>>8)&0xFF)/255.f, b2=(p&0xFF)/255.f;
            float luma=.2126f*r2+.7152f*g2+.0722f*b2; if(luma<thr) continue;
            for(int dy=-rad; dy<=rad; dy+=rad) for(int dx=-rad; dx<=rad; dx+=rad){
                if(!dx && !dy) continue;
                int nx=(int)px+dx, ny=(int)py+dy;
                if(nx<0||ny<0||nx>=(int)W||ny>=(int)H) continue;
                uint32_t *tp = &src[ny*W+nx];
                float tr=(((*tp)>>16)&0xFF)/255.f, tg=(((*tp)>>8)&0xFF)/255.f, tb=(*tp&0xFF)/255.f;
                tr=sr_clamp(tr+r2*str*.06f,0,1); tg=sr_clamp(tg+g2*str*.06f,0,1); tb=sr_clamp(tb+b2*str*.06f,0,1);
                *tp=((uint32_t)(tr*255)<<16)|((uint32_t)(tg*255)<<8)|(uint32_t)(tb*255);
            }
        }
    }
    rl_free(tmp);
}

void rig_pbr_tonemap_pass(RigGPUCtx *gpu, RigPBRPipeline *pbr, float exposure){
    SRBackend *sr = sr_get(gpu); if(!sr) return;
    uint32_t W=pbr->width, H=pbr->height;
    uint32_t *src=sr->fbo_color[pbr->fbo_gbuffer], *dst=sr->primary_fb;
    if(!src || !dst) return;
    float e = exposure>0.f ? exposure : 1.f;
    for(uint32_t i=0; i<W*H; i++){
        uint32_t p = src[i];
        float r=((p>>16)&0xFF)/255.f*e, g=((p>>8)&0xFF)/255.f*e, b=(p&0xFF)/255.f*e;
        dst[i] = sr_pack(sr_srgb(sr_aces(r)), sr_srgb(sr_aces(g)), sr_srgb(sr_aces(b)));
    }
}

const char* rig_glsl_src(const char *name){ (void)name; return "/* MASTER: sin GLSL */"; }

bool rb_gpu_compile_holo_shaders(RBGpuContext *gpu, const char *vert_src, const char *frag_src){
    (void)vert_src; (void)frag_src;
    gpu->shader_holo_ui = rig_shader_compile(gpu, RIG_SHADER_FRAG, NULL, "holo");
    return true;
}

bool rb_gpu_init_fbo_pipeline(RBGpuContext *gpu, int w, int h){
    SRBackend *sr = sr_get(gpu); if(!sr) return false;
    gpu->fbo_primary   = rig_fbo_create(gpu,(uint32_t)w,(uint32_t)h,RIG_TEX_RGBA8,true);
    gpu->fbo_refraction= rig_fbo_create(gpu,(uint32_t)w,(uint32_t)h,RIG_TEX_RGBA8,false);
    gpu->viewport_width=w; gpu->viewport_height=h;
    return true;
}

void rb_gpu_apply_catenary_transform(RBGpuContext *gpu, void *boxes, uint32_t count, float scroll_y){
    (void)gpu; if(!boxes || !count) return;
    float *b = (float*)boxes;
    for(uint32_t i=0; i<count; i++){
        float t = (float)i / (float)(count>1?count-1:1);
        b[i*4+1] += rl_sinf(t*SR_PI + scroll_y*RL_PHI) * 12.f;
    }
}

void rb_gpu_swap_buffers(RBGpuContext *gpu){ rig_gpu_present(gpu); }

uint32_t rig_gpu_compile_shader(RigShaderStage stage, const char *src){
    if(!g_gpu_ctx) return RIG_GPU_INVALID_ID;
    return rig_shader_compile(g_gpu_ctx, stage, src, "catedral_ui");
}

uint32_t rig_gpu_link_program(uint32_t vs, uint32_t fs){
    if(!g_gpu_ctx) return RIG_GPU_INVALID_ID;
    return rig_program_link(g_gpu_ctx, vs, fs);
}

void     rig_gpu_delete_shader(uint32_t id){ if(g_gpu_ctx) rig_shader_free(g_gpu_ctx, id); }

uint32_t rig_gpu_create_vao(void){
    if(g_leg_n_vao >= SR_LEG_MAX) return RIG_GPU_INVALID_ID;
    uint32_t id = g_leg_n_vao++; g_leg_vao[id] = id; return id;
}

uint32_t rig_gpu_create_vbo(const void *data, size_t size){
    if(!g_gpu_ctx) return RIG_GPU_INVALID_ID;
    return rig_buffer_create(g_gpu_ctx, RIG_BUF_VERTEX, data, size, RIG_BUF_STATIC);
}

void rig_gpu_vao_attrib(uint32_t vao_id, uint32_t vbo_id, uint32_t idx,
                        uint32_t nc, uint32_t stride, uint32_t offset){
    if(!g_gpu_ctx || vao_id>=SR_MAX_VAOS || idx>=8 || vbo_id>=g_gpu_ctx->n_buffers) return;
    SRBackend *sr=sr_get(g_gpu_ctx); if(!sr) return;
    SRVAOState *state=&sr->vao_states[vao_id];
    state->vbo_id=vbo_id; state->attrib_nc[idx]=nc;
    state->attrib_stride[idx]=stride; state->attrib_offset[idx]=offset;
    if(state->n_attribs<=idx) state->n_attribs=(uint8_t)(idx+1u);
    state->active=true;
    if(sr->n_vao_states<=vao_id) sr->n_vao_states=vao_id+1u;
}

int rig_gpu_uniform_loc(uint32_t prog, const char *name){
    if(!g_gpu_ctx) return -1;
    SRBackend *sr = sr_get(g_gpu_ctx);
    if(!sr || prog >= RIG_GPU_MAX_PROGRAMS) return -1;
    for(uint32_t i=0; i<sr->n_uniforms[prog]; i++)
        if(rl_strncmp(sr->uniforms[prog][i].name, name, RIG_GPU_MAX_UNIFORM_NAME-1)==0) return (int)i;
    if(sr->n_uniforms[prog] >= SR_MAX_UNIFORMS) return -1;
    uint32_t n = sr->n_uniforms[prog];
    rl_strncpy(sr->uniforms[prog][n].name, name, RIG_GPU_MAX_UNIFORM_NAME-1);
    sr->n_uniforms[prog]++;
    return (int)n;
}

void rig_gpu_uniform_4f(int loc, float a, float b, float c, float d){
    if(!g_gpu_ctx || loc<0) return;
    SRBackend *sr = sr_get(g_gpu_ctx); if(!sr) return;
    uint32_t prog = sr->bound_program;
    if(prog>=RIG_GPU_MAX_PROGRAMS || (uint32_t)loc>=sr->n_uniforms[prog]) return;
    SRUniform *u = &sr->uniforms[prog][loc];
    u->type=SR_UNIF_VEC4; u->val.v4[0]=a; u->val.v4[1]=b; u->val.v4[2]=c; u->val.v4[3]=d; u->set=true;
}

void rig_gpu_uniform_1f(int loc, float v){
    if(!g_gpu_ctx || loc<0) return;
    SRBackend *sr = sr_get(g_gpu_ctx); if(!sr) return;
    uint32_t prog = sr->bound_program;
    if(prog>=RIG_GPU_MAX_PROGRAMS || (uint32_t)loc>=sr->n_uniforms[prog]) return;
    SRUniform *u = &sr->uniforms[prog][loc]; u->type=SR_UNIF_FLOAT; u->val.f=v; u->set=true;
}

void rig_gpu_uniform_2f(int loc, float a, float b){
    if(!g_gpu_ctx || loc<0) return;
    SRBackend *sr = sr_get(g_gpu_ctx); if(!sr) return;
    uint32_t prog = sr->bound_program;
    if(prog>=RIG_GPU_MAX_PROGRAMS || (uint32_t)loc>=sr->n_uniforms[prog]) return;
    SRUniform *u = &sr->uniforms[prog][loc]; u->type=SR_UNIF_VEC2; u->val.v2[0]=a; u->val.v2[1]=b; u->set=true;
}

void rig_gpu_use_program(uint32_t prog){ if(g_gpu_ctx) rig_program_bind(g_gpu_ctx, prog); }

void rig_gpu_bind_vao(uint32_t vao_id)  {
    if(!g_gpu_ctx || vao_id>=SR_MAX_VAOS) return;
    SRBackend *sr=sr_get(g_gpu_ctx); if(!sr || !sr->vao_states[vao_id].active) return;
    sr->bound_vao_id=vao_id; g_gpu_ctx->bound_vao=vao_id;
}

void rig_gpu_draw_triangles(uint32_t first, uint32_t count){
    if(g_gpu_ctx) rig_draw_arrays(g_gpu_ctx, 0x0004, first, count);
}

int usonic_init_MST(void){
    if(g_usonic.initialized) return 0;
    g_usonic.carrier_freq_hz    = USONIC_FREQ_KHZ * 1000u;
    g_usonic.modulation_freq_hz = 0u;
    g_usonic.max_pressure_pa    = USONIC_MAX_PRESSURE_PA;
    g_usonic.envelope_level     = 0.f;
    g_usonic.mode               = BEAM_MODE_FLAT;
    g_usonic.thermal_protection = true;
    for(uint32_t r=0; r<USONIC_ARRAY_ROWS; r++)
        for(uint32_t c=0; c<USONIC_ARRAY_COLS; c++){
            g_usonic.drivers[r][c].phase_deg = 0u;
            g_usonic.drivers[r][c].amplitude = 0;
            g_usonic.drivers[r][c].enabled   = true;
        }
    g_usonic.focus_x = 0.5f;
    g_usonic.focus_y = 0.5f;
    g_usonic.focus_z = 100.f;
    g_usonic.initialized = true;
    return 0;
}

void usonic_set_beam_mode(UltrasonicBeamMode mode){
    if(!g_usonic.initialized) return;
    g_usonic.mode = mode;
}

void usonic_set_focal_point(float x_norm, float y_norm, float z_mm){
    if(!g_usonic.initialized) return;
    g_usonic.focus_x = x_norm;
    g_usonic.focus_y = y_norm;
    g_usonic.focus_z = z_mm;
}

void usonic_set_envelope(float level){
    if(!g_usonic.initialized) return;
    g_usonic.envelope_level = sr_clamp(level, 0.f, 1.f);
}

void usonic_set_modulation_freq(uint32_t freq_hz){
    if(!g_usonic.initialized) return;
    g_usonic.modulation_freq_hz = freq_hz;
}

int16_t usonic_pressure_at_MST(float x_mm, float y_mm, float z_mm){
    if(!g_usonic.initialized) return 0;
    float dx = x_mm - g_usonic.focus_x * USONIC_APERTURE_MM;
    float dy = y_mm - g_usonic.focus_y * USONIC_APERTURE_MM;
    float dz = z_mm - g_usonic.focus_z;
    float dist_sq = dx*dx + dy*dy + dz*dz;
    float sigma_sq = 100.f;          /* Dispersión Gaussiana [mm²] (≈ φ × 62 mm²) */
    float pressure  = (float)USONIC_MAX_PRESSURE_PA
                    * rl_expf(-dist_sq / (2.f * sigma_sq))  /* FIX-7: rl_expf */
                    * g_usonic.envelope_level;
    if(pressure > (float)USONIC_MAX_PRESSURE_PA) pressure = (float)USONIC_MAX_PRESSURE_PA;
    return (int16_t)pressure;
}

void usonic_update_drivers(void){
    if(!g_usonic.initialized) return;
    float spacing_x = USONIC_APERTURE_MM / (float)(USONIC_ARRAY_COLS - 1u);
    float spacing_y = USONIC_APERTURE_MM / (float)(USONIC_ARRAY_ROWS - 1u);
    for(uint32_t r=0; r<USONIC_ARRAY_ROWS; r++){
        for(uint32_t c=0; c<USONIC_ARRAY_COLS; c++){
            if(!g_usonic.drivers[r][c].enabled) continue;
            float elem_x = (float)c * spacing_x;
            float elem_y = (float)r * spacing_y;
            switch(g_usonic.mode){
                case BEAM_MODE_FLAT:
                    g_usonic.drivers[r][c].phase_deg = 0u;
                    break;
                case BEAM_MODE_FOCUSED: {
                    /* Beamforming: retardo de fase por diferencia de camino óptico-acústico */
                    float fx = g_usonic.focus_x * USONIC_APERTURE_MM;
                    float fy = g_usonic.focus_y * USONIC_APERTURE_MM;
                    float fz = g_usonic.focus_z;
                    float ddx = fx - elem_x, ddy = fy - elem_y;
                    /* FIX-9: usar constante USONIC_WAVELENGTH_MM = 8.575 mm */
                    float dist_elem = rl_sqrtf(ddx*ddx + ddy*ddy + fz*fz); /* FIX-7 */
                    float path_diff = dist_elem - fz;
                    float phase_rad = (path_diff / USONIC_WAVELENGTH_MM) * SR_2PI; /* FIX-9 */
                    float phase_deg = phase_rad * (180.f / SR_PI);
                    phase_deg = rl_fmodf(phase_deg, 360.f); /* FIX-7 */
                    if(phase_deg < 0.f) phase_deg += 360.f;
                    g_usonic.drivers[r][c].phase_deg = (uint16_t)phase_deg;
                    break;
                }
                case BEAM_MODE_TRAVELING: {
                    /* Patrón rotante — efecto táctil circular */
                    uint32_t rot_speed = 1000u;
                    float rotation_phase = (float)(g_usonic.timestamp_us % 1000000u)
                                         * (float)rot_speed / 1000000.f;
                    float pd = rl_fmodf(rotation_phase, 360.f);  /* FIX-7 */
                    g_usonic.drivers[r][c].phase_deg = (uint16_t)pd;
                    break;
                }
            }
            int32_t amp = (int32_t)((float)USONIC_MAX_PRESSURE_PA * g_usonic.envelope_level);
            g_usonic.drivers[r][c].amplitude =
                (amp > USONIC_MAX_PRESSURE_PA) ? (int16_t)USONIC_MAX_PRESSURE_PA : (int16_t)amp;
        }
    }
    g_usonic.timestamp_us++;
}

void usonic_shutdown_MST(void){
    if(!g_usonic.initialized) return;
    rl_memset(&g_usonic, 0, sizeof(g_usonic));  /* FIX-8 */
}

static void aom_euler_to_matrix(float rx, float ry, float rz, float m[9]){
    float cx=rl_cosf(rx), sx=rl_sinf(rx);  /* FIX-10 */
    float cy=rl_cosf(ry), sy=rl_sinf(ry);
    float cz=rl_cosf(rz), sz=rl_sinf(rz);
    m[0]= cy*cz;            m[1]=-cy*sz;            m[2]= sy;
    m[3]= sx*sy*cz+cx*sz;   m[4]=-sx*sy*sz+cx*cz;   m[5]=-sx*cy;
    m[6]=-cx*sy*cz+sx*sz;   m[7]= cx*sy*sz+sx*cz;   m[8]= cx*cy;
}

static inline void aom_mat3_mulvec3(const float m[9], float x, float y, float z,
                                     float *ox, float *oy, float *oz){
    *ox = m[0]*x + m[1]*y + m[2]*z;
    *oy = m[3]*x + m[4]*y + m[5]*z;
    *oz = m[6]*x + m[7]*y + m[8]*z;
}

static inline uint8_t aom_modulate_channel(uint8_t base, float freq_offset){
    float eff = (freq_offset > 0.001f || freq_offset < -0.001f) ? 0.95f : 1.f;
    return (uint8_t)((float)base * eff);
}

int aom_volumetric_init(uint32_t width, uint32_t height){
    if(g_aom.initialized) return 0;
    g_aom.width  = width;
    g_aom.height = height;
    g_aom.voxel_grid_depth = VOXEL_HEIGHT_MM;
    g_aom.max_objects  = AOM_MAX_OBJECTS;
    g_aom.object_count = 0u;

    g_aom.voxel_brightness = (uint8_t*)rl_malloc(VOXEL_GRID_SIZE);
    if(!g_aom.voxel_brightness) return -1;
    rl_memset(g_aom.voxel_brightness, 0, VOXEL_GRID_SIZE);  /* FIX-8 */

    g_aom.objects = (HologramObject*)rl_malloc(sizeof(HologramObject) * g_aom.max_objects);
    if(!g_aom.objects){
        rl_free(g_aom.voxel_brightness); g_aom.voxel_brightness = NULL;
        return -1;
    }
    rl_memset(g_aom.objects, 0, sizeof(HologramObject) * g_aom.max_objects);

    if(usonic_init_MST() != 0){
        rl_free(g_aom.objects);          g_aom.objects = NULL;
        rl_free(g_aom.voxel_brightness); g_aom.voxel_brightness = NULL;
        return -1;
    }

    g_aom.initialized = true;
    g_aom.frame_count = 0u;
    return 0;
}

int aom_add_object_MST(VoxelPoint *voxels, uint32_t count, float x, float y, float z){
    if(!g_aom.initialized || g_aom.object_count >= g_aom.max_objects) return -1;
    HologramObject *obj = &g_aom.objects[g_aom.object_count];
    obj->id          = g_aom.object_count;
    obj->voxels      = voxels;
    obj->voxel_count = count;
    obj->pos_x = x; obj->pos_y = y; obj->pos_z = z;
    obj->scale = 1.f;
    obj->rotation_euler[0] = obj->rotation_euler[1] = obj->rotation_euler[2] = 0.f;
    obj->visible  = true;
    obj->mod_mode = AOM_MODE_RGB;
    obj->frame_updated = g_aom.frame_count;
    return (int)(g_aom.object_count++);
}

void aom_set_object_transform(int obj_id, float x, float y, float z,
                               float rx, float ry, float rz, float scale){
    if(obj_id < 0 || obj_id >= (int)g_aom.object_count) return;
    HologramObject *obj = &g_aom.objects[obj_id];
    obj->pos_x = x; obj->pos_y = y; obj->pos_z = z;
    obj->rotation_euler[0] = rx;
    obj->rotation_euler[1] = ry;
    obj->rotation_euler[2] = rz;
    obj->scale = scale;
    obj->frame_updated = g_aom.frame_count;
}

void aom_set_modulation_mode(int obj_id, AOMModulationMode mode){
    if(obj_id < 0 || obj_id >= (int)g_aom.object_count) return;
    g_aom.objects[obj_id].mod_mode = mode;
}

void aom_haptic_update(float hand_x, float hand_y, float hand_z_mm, bool hand_present){
    if(!g_aom.initialized) return;
    if(hand_present){
        usonic_set_focal_point(hand_x, hand_y, hand_z_mm);
        usonic_set_envelope(1.f);
    } else {
        usonic_set_envelope(0.f);
    }
    usonic_update_drivers();
}

void aom_volumetric_render(uint32_t *framebuffer, uint32_t pitch){
    if(!g_aom.initialized || !framebuffer) return;
    rl_memset(g_aom.voxel_brightness, 0, VOXEL_GRID_SIZE);

    for(uint32_t oi = 0; oi < g_aom.object_count; oi++){
        HologramObject *obj = &g_aom.objects[oi];
        if(!obj->visible || !obj->voxels) continue;

        float rm[9];
        aom_euler_to_matrix(obj->rotation_euler[0], obj->rotation_euler[1],
                            obj->rotation_euler[2], rm);

        for(uint32_t vi = 0; vi < obj->voxel_count; vi++){
            VoxelPoint *vp = &obj->voxels[vi];
            float vx = vp->x * obj->scale, vy = vp->y * obj->scale, vz = vp->z * obj->scale;
            float wx, wy, wz;
            aom_mat3_mulvec3(rm, vx, vy, vz, &wx, &wy, &wz);
            wx += obj->pos_x; wy += obj->pos_y; wz += obj->pos_z;
            if(wx < 0.f || wx >= 1.f || wy < 0.f || wy >= 1.f || wz < 0.f || wz > 1.f) continue;

            uint32_t gx = (uint32_t)(wx * (float)(VOXEL_GRID_RES - 1u));
            uint32_t gy = (uint32_t)(wy * (float)(VOXEL_GRID_RES - 1u));
            uint32_t gz = (uint32_t)(wz * (float)(VOXEL_GRID_RES - 1u));
            uint32_t idx = gz*VOXEL_GRID_RES*VOXEL_GRID_RES + gy*VOXEL_GRID_RES + gx;

            uint8_t intensity = (uint8_t)(((uint32_t)vp->r + vp->g + vp->b) / 3u);
            if(obj->mod_mode == AOM_MODE_RGB){
                uint8_t r = aom_modulate_channel(vp->r, 0.f);
                uint8_t g = aom_modulate_channel(vp->g, 0.0001f);
                uint8_t b = aom_modulate_channel(vp->b, -0.0001f);
                intensity = (uint8_t)(((uint32_t)r + g + b) / 3u);
            } else if(obj->mod_mode == AOM_MODE_DEPTH){
                float depth_factor = 1.f - wz * 0.3f;
                intensity = (uint8_t)((float)intensity * depth_factor);
            }
            uint32_t accum = (uint32_t)g_aom.voxel_brightness[idx] + intensity;
            g_aom.voxel_brightness[idx] = (accum > 255u) ? 255u : (uint8_t)accum;
        }
    }

    uint32_t fb_w = g_aom.width, fb_h = g_aom.height;
    for(uint32_t gy = 0; gy < VOXEL_GRID_RES; gy++){
        for(uint32_t gx = 0; gx < VOXEL_GRID_RES; gx++){
            uint8_t maxb = 0u;
            for(uint32_t gz = 0; gz < VOXEL_GRID_RES; gz++){
                uint32_t idx = gz*VOXEL_GRID_RES*VOXEL_GRID_RES + gy*VOXEL_GRID_RES + gx;
                uint8_t b = g_aom.voxel_brightness[idx];
                if(b > maxb) maxb = b;
            }
            uint32_t fbx = (gx * fb_w) / VOXEL_GRID_RES;
            uint32_t fby = (gy * fb_h) / VOXEL_GRID_RES;
            if(fbx < fb_w && fby < fb_h){
                uint32_t fb_idx = fby * (pitch / 4u) + fbx;
                framebuffer[fb_idx] = 0xFF000000u |
                    ((uint32_t)maxb << 16) | ((uint32_t)maxb << 8) | (uint32_t)maxb;
            }
        }
    }
    g_aom.frame_count++;
}

void aom_volumetric_free(void){
    if(!g_aom.initialized) return;
    usonic_shutdown_MST();
    rl_free(g_aom.voxel_brightness); g_aom.voxel_brightness = NULL;
    rl_free(g_aom.objects);          g_aom.objects = NULL;
    rl_memset(&g_aom, 0, sizeof(g_aom));  /* FIX-8 */
}

const RigMatEntry* rig_mat_find(const char *name){
    if(!name) return NULL;
    for(int i = 0; i < RIG_MAT_TOTAL; i++)
        if(rl_strncmp(g_mat_db[i].name, name, 64) == 0)
            return &g_mat_db[i];
    return NULL;
}

void rig_mat_apply(RigGPUCtx *gpu, uint32_t prog, const RigMatEntry *e){
    if(!gpu || !e) return;
    SRBackend *sr = sr_get(gpu);
    if(!sr) return;
    SRUniform *u;
#define _M3(N,A) do{ u=sr_uset(sr,prog,N,SR_UNIF_VEC3);  if(u){ u->val.v3[0]=(A)[0]; u->val.v3[1]=(A)[1]; u->val.v3[2]=(A)[2]; } }while(0)
#define _M1(N,V) do{ u=sr_uset(sr,prog,N,SR_UNIF_FLOAT); if(u) u->val.f=(V); }while(0)
    _M3("uAlbedo",      e->pbr.alb);
    _M3("u_albedo",     e->pbr.alb);
    _M1("uMetallic",    e->pbr.metal);
    _M1("uMetalVal",    e->pbr.metal);
    _M1("uRoughness",   e->pbr.rough);
    _M1("uRoughVal",    e->pbr.rough);
    _M1("uSSS",         e->pbr.sss);
    _M3("uEmissive",    e->pbr.ec);
    _M1("uEmissionStr", e->pbr.es);
    _M1("uIridescence", e->pbr.iri);
    _M3("uRimColor",    e->pbr.rim);
    _M1("uClearcoat",   e->pbr.cc);
#undef _M3
#undef _M1
}

int rig_mat_set(RigGPUCtx *gpu, uint32_t prog, const char *name){
    const RigMatEntry *e = rig_mat_find(name);
    if(!e) return -1;
    rig_mat_apply(gpu, prog, e);
    return 0;
}

