/* ═══════════════════════════════════════════════════════════════════════════
 * rig_height3d.c — EL RELIEVE · RIGCOM MASTER · BLOQUE 3
 *
 * Once generadores. Cero texturas. Cero bytes en disco.
 * La misma función alimenta el POM (vista), el háptico (tacto) y el
 * sintetizador de fricción (oído). Un solo generador, tres sentidos.
 *
 * C11 · cero libc · φ
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rig_height3d.h"

extern float rl_sqrtf(float);
extern float rl_sinf(float);
extern float rl_cosf(float);
extern float rl_fabsf(float);
extern float rl_floorf(float);
extern float rl_expf(float);
extern void *rl_memset(void *, int, unsigned long);

#define RH_PI    3.14159265358979324f
#define RH_TAU   6.28318530717958648f
#define RH_PHI   1.6180339887498948f

static float rh_clamp(float x, float a, float b){ return x<a?a:(x>b?b:x); }
static float rh_min(float a, float b){ return a<b?a:b; }
static float rh_max(float a, float b){ return a>b?a:b; }
static float rh_fract(float x){ return x - rl_floorf(x); }
static float rh_lerp(float a, float b, float t){ return a + (b-a)*t; }
static float rh_smooth(float t){ return t*t*(3.0f - 2.0f*t); }


/* ═══════════════════════════════════════════════════════════════════════════
 * §1  HASH Y RUIDO SOBERANOS
 * ═══════════════════════════════════════════════════════════════════════════ */

static float hash13(float x, float y, float z)
{
    float px = rh_fract(x * 0.1031f);
    float py = rh_fract(y * 0.1031f);
    float pz = rh_fract(z * 0.1031f);
    float d  = px*(pz + 31.32f) + py*(px + 31.32f) + pz*(py + 31.32f);
    px += d; py += d; pz += d;
    return rh_fract((px + py) * pz);
}

static void hash33(float x, float y, float z, float o[3])
{
    float px = rh_fract(x * 0.1031f);
    float py = rh_fract(y * 0.1030f);
    float pz = rh_fract(z * 0.0973f);
    float d  = px*(py + 33.33f) + py*(px + 33.33f) + pz*(px + 33.33f);
    px += d; py += d; pz += d;
    o[0] = rh_fract((px + py) * pz);
    o[1] = rh_fract((py + pz) * px);
    o[2] = rh_fract((pz + px) * py);
}

/* Ruido de valor 3D con interpolación quíntica (C²: sin artefactos en el
 * gradiente, que es lo que el POM y la normal necesitan) */
static float vnoise3(float x, float y, float z)
{
    float ix = rl_floorf(x), iy = rl_floorf(y), iz = rl_floorf(z);
    float fx = x - ix, fy = y - iy, fz = z - iz;

    float ux = fx*fx*fx*(fx*(fx*6.0f - 15.0f) + 10.0f);
    float uy = fy*fy*fy*(fy*(fy*6.0f - 15.0f) + 10.0f);
    float uz = fz*fz*fz*(fz*(fz*6.0f - 15.0f) + 10.0f);

    float c000 = hash13(ix,      iy,      iz     );
    float c100 = hash13(ix+1.0f, iy,      iz     );
    float c010 = hash13(ix,      iy+1.0f, iz     );
    float c110 = hash13(ix+1.0f, iy+1.0f, iz     );
    float c001 = hash13(ix,      iy,      iz+1.0f);
    float c101 = hash13(ix+1.0f, iy,      iz+1.0f);
    float c011 = hash13(ix,      iy+1.0f, iz+1.0f);
    float c111 = hash13(ix+1.0f, iy+1.0f, iz+1.0f);

    float x00 = rh_lerp(c000, c100, ux);
    float x10 = rh_lerp(c010, c110, ux);
    float x01 = rh_lerp(c001, c101, ux);
    float x11 = rh_lerp(c011, c111, ux);
    float y0  = rh_lerp(x00, x10, uy);
    float y1  = rh_lerp(x01, x11, uy);
    return rh_lerp(y0, y1, uz);
}

static float fbm3(float x, float y, float z, int oct, float lac, float gain)
{
    float v = 0.0f, a = 0.5f, norm = 0.0f;
    for (int i = 0; i < oct && i < 8; i++) {
        v += a * vnoise3(x, y, z);
        norm += a;
        x *= lac; y *= lac; z *= lac;
        a *= gain;
    }
    return (norm > 1e-6f) ? v / norm : 0.0f;
}

/* Voronoi 3D. Devuelve F1 (distancia al más cercano), F2, y el id de celda. */
static void voronoi3(float x, float y, float z,
                     float *F1, float *F2, float *cell_id)
{
    float bx = rl_floorf(x), by = rl_floorf(y), bz = rl_floorf(z);
    float fx = x - bx, fy = y - by, fz = z - bz;

    float f1 = 8.0f, f2 = 8.0f, id = 0.0f;

    for (int k = -1; k <= 1; k++) {
        for (int j = -1; j <= 1; j++) {
            for (int i = -1; i <= 1; i++) {
                float nx = (float)i, ny = (float)j, nz = (float)k;
                float r[3];
                hash33(bx+nx, by+ny, bz+nz, r);

                float dx = nx + r[0] - fx;
                float dy = ny + r[1] - fy;
                float dz = nz + r[2] - fz;
                float d  = rl_sqrtf(dx*dx + dy*dy + dz*dz);

                if (d < f1) {
                    f2 = f1;
                    f1 = d;
                    id = hash13(bx+nx, by+ny, bz+nz);
                } else if (d < f2) {
                    f2 = d;
                }
            }
        }
    }
    if (F1)      *F1 = f1;
    if (F2)      *F2 = f2;
    if (cell_id) *cell_id = id;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §2  LOS ONCE GENERADORES
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ── POROS SEBÁCEOS (piel, cuero anilina, nubuck) ─────────────────────────
 * Voronoi con perfil CÓNICO. Cada poro tiene un radio distinto (hash de la
 * celda) — como en la piel real, donde no hay dos poros iguales.
 * Frecuencia espacial ≈ 3 ciclos/mm → el dedo a 100 mm/s vibra a 300 Hz.
 * Justo el pico de los corpúsculos de Pacini. Por eso la piel se SIENTE. */
float rig_h_pores(const float p[3], float freq)
{
    float x = p[0]*freq, y = p[1]*freq, z = p[2]*freq;
    float F1, F2, id;
    voronoi3(x, y, z, &F1, &F2, &id);

    /* Radio variable por celda: 0.10 .. 0.28 */
    float r = 0.10f + 0.18f * id;

    if (F1 >= r) return 1.0f;               /* fuera del poro: superficie */

    /* Perfil cónico suavizado:  h = 1 − (1 − t²)   con t = F1/r */
    float t = F1 / rh_max(r, 1e-4f);
    float depth = 1.0f - t*t;               /* 1 en el centro, 0 en el borde */

    /* Reborde sebáceo: los poros tienen un pequeño labio elevado */
    float rim = rh_smooth(rh_clamp((F1 - r*0.85f) / (r*0.25f), 0.0f, 1.0f))
              * rh_smooth(rh_clamp((r*1.15f - F1) / (r*0.25f), 0.0f, 1.0f));

    return rh_clamp(1.0f - depth * 0.85f + rim * 0.12f, 0.0f, 1.0f);
}

/* ── TRAMA (seda, lino, brocado) ──────────────────────────────────────────
 * Seno cruzado: urdimbre × trama. Los hilos se cruzan y donde uno pasa por
 * encima del otro, hay una cresta. Eso es un tejido.
 * dir orienta el hilo — sin eso, no hay anisotropía coherente. */
float rig_h_weave(const float p[3], float freq, const float dir[3])
{
    float dx = 1.0f, dy = 0.0f;
    if (dir) { dx = dir[0]; dy = dir[2]; }
    float dl = rl_sqrtf(dx*dx + dy*dy);
    if (dl > 1e-6f) { dx /= dl; dy /= dl; }

    /* Proyectar sobre los ejes del tejido */
    float u = (p[0]*dx + p[2]*dy) * freq;
    float v = (p[0]*(-dy) + p[2]*dx) * freq;

    /* Urdimbre y trama, en cuadratura */
    float warp = rl_sinf(u * RH_TAU) * 0.5f + 0.5f;
    float weft = rl_sinf(v * RH_TAU) * 0.5f + 0.5f;

    /* Donde uno cruza por encima del otro: máximo local */
    float over = rh_max(warp * (1.0f - weft*0.55f),
                        weft * (1.0f - warp*0.55f));

    /* Irregularidad del hilo (ningún tejido es perfecto) */
    float slub = vnoise3(u*0.35f, v*0.35f, p[1]*freq*0.2f) * 0.14f;

    return rh_clamp(over * 0.86f + slub + 0.07f, 0.0f, 1.0f);
}

/* ── fBm (roca volcánica, estuco, hormigón) ── */
float rig_h_fbm(const float p[3], float freq)
{
    float v = fbm3(p[0]*freq, p[1]*freq, p[2]*freq, 5, 2.07f, 0.52f);
    /* Ridged: valor absoluto invertido → filos, no ondulaciones */
    float ridged = 1.0f - rl_fabsf(v * 2.0f - 1.0f);
    return rh_clamp(v * 0.55f + ridged * 0.45f, 0.0f, 1.0f);
}

/* ── ESCAMAS (reptil, nácar, piña) ── */
float rig_h_scales(const float p[3], float freq)
{
    /* Voronoi anisótropo: escamas más anchas que altas */
    float x = p[0]*freq, y = p[1]*freq*1.9f, z = p[2]*freq;
    float F1, F2, id;
    voronoi3(x, y, z, &F1, &F2, &id);

    /* Borde de la escama: F2 − F1 pequeño = estás en la frontera */
    float edge = rh_smooth(rh_clamp((F2 - F1) / 0.16f, 0.0f, 1.0f));

    /* Cada escama es una cúpula, y su altura varía */
    float dome = (1.0f - F1 * 1.4f);
    dome = rh_clamp(dome, 0.0f, 1.0f);

    return rh_clamp(edge * (0.55f + 0.45f * dome) * (0.75f + 0.25f * id),
                    0.0f, 1.0f);
}

/* ── CEPILLADO (metal, mokumé) ────────────────────────────────────────────
 * Surcos MUY finos y MUY paralelos. Frecuencia espacial altísima (8 c/mm):
 * ópticamente da la anisotropía, y al tacto da un susurro rápido. */
float rig_h_brushed(const float p[3], float freq, const float dir[3])
{
    float dx = 1.0f, dz = 0.0f;
    if (dir) { dx = dir[0]; dz = dir[2]; }
    float dl = rl_sqrtf(dx*dx + dz*dz);
    if (dl > 1e-6f) { dx /= dl; dz /= dl; }

    /* Coordenada PERPENDICULAR al surco: es la que varía */
    float perp = (p[0]*(-dz) + p[2]*dx) * freq;
    /* Coordenada A LO LARGO del surco: apenas varía (el surco es continuo) */
    float alng = (p[0]*dx + p[2]*dz) * freq * 0.06f;

    /* Surcos de anchura variable — un cepillado real no es un peine */
    float g1 = rl_sinf(perp * RH_TAU);
    float g2 = rl_sinf(perp * RH_TAU * RH_PHI + 1.7f) * 0.42f;
    float jitter = vnoise3(perp*0.28f, alng, 0.0f) * 0.30f;

    float h = (g1 + g2) * 0.5f * (0.72f + jitter);
    return rh_clamp(h * 0.5f + 0.5f, 0.0f, 1.0f);
}

/* ── MARTILLADO (oro, cobre) ──────────────────────────────────────────────
 * Hoyos grandes y suaves — cada golpe de martillo deja una cúpula cóncava. */
float rig_h_hammered(const float p[3], float freq)
{
    float F1, F2, id;
    voronoi3(p[0]*freq, p[1]*freq, p[2]*freq, &F1, &F2, &id);

    /* Cada hoyo tiene profundidad distinta (cada martillazo es distinto) */
    float depth = 0.55f + 0.45f * id;

    /* Cúpula suave (coseno) — no cónica: el martillo deforma plásticamente */
    float t = rh_clamp(F1 / 0.62f, 0.0f, 1.0f);
    float dome = 0.5f - 0.5f * rl_cosf(t * RH_PI);

    return rh_clamp(1.0f - (1.0f - dome) * depth * 0.72f, 0.0f, 1.0f);
}

/* ── MOLETEADO (agarre metálico) ──────────────────────────────────────────
 * ★ EL CASO INTERESANTE: ópticamente liso (rough ≈ 0.34) pero
 * táctilmente ÁSPERO. Solo un sistema donde el tacto salga del RELIEVE
 * y no del BRDF puede expresar esa contradicción. */
float rig_h_knurl(const float p[3], float freq, const float dir[3])
{
    float dx = 0.7071f, dz = 0.7071f;         /* 45° por defecto */
    if (dir) { dx = dir[0]; dz = dir[2];
        float dl = rl_sqrtf(dx*dx + dz*dz);
        if (dl > 1e-6f) { dx /= dl; dz /= dl; }
    }

    /* Dos familias de surcos cruzados a ±45° → rombos */
    float a = (p[0]*dx + p[2]*dz) * freq;
    float b = (p[0]*(-dz) + p[2]*dx) * freq;

    float ga = rl_fabsf(rl_sinf(a * RH_PI));
    float gb = rl_fabsf(rl_sinf(b * RH_PI));

    /* El rombo es la intersección: pirámides truncadas */
    float pyr = rh_min(ga, gb);
    pyr = rh_clamp(pyr * 1.35f, 0.0f, 1.0f);

    return rh_clamp(1.0f - pyr, 0.0f, 1.0f);
}

/* ── CRAQUELADO (laca, cerámica, hielo, barniz viejo) ────────────────────
 * Voronoi RIDGED: la grieta está en la FRONTERA entre celdas (F2−F1 → 0). */
float rig_h_crackle(const float p[3], float freq)
{
    float F1, F2, id;
    voronoi3(p[0]*freq, p[1]*freq, p[2]*freq, &F1, &F2, &id);

    /* La grieta: donde F2 − F1 es pequeño */
    float d = F2 - F1;
    float crack = 1.0f - rh_smooth(rh_clamp(d / 0.10f, 0.0f, 1.0f));

    /* Los bordes de la grieta se levantan un poco (el barniz se curva) */
    float lift = rh_smooth(rh_clamp((d - 0.08f) / 0.09f, 0.0f, 1.0f))
               * rh_smooth(rh_clamp((0.22f - d) / 0.10f, 0.0f, 1.0f));

    return rh_clamp(1.0f - crack * 0.90f + lift * 0.10f, 0.0f, 1.0f);
}

/* ── FIBRA (terciopelo, cachemira, nubuck) ────────────────────────────────
 * Fibras cortas y densas, casi verticales, con una ligera inclinación
 * preferente (el "pelo" del terciopelo tiene sentido: acaricia distinto
 * a favor que a contrapelo).
 * 12 ciclos/mm → susurro de alta frecuencia. */
float rig_h_fiber(const float p[3], float freq, const float dir[3])
{
    float x = p[0]*freq, y = p[1]*freq, z = p[2]*freq;

    /* Cada fibra es un punto de un Voronoi muy denso */
    float F1, F2, id;
    voronoi3(x, y * 0.35f, z, &F1, &F2, &id);

    /* La punta de la fibra es más alta que la base */
    float tip = rh_clamp(1.0f - F1 * 2.6f, 0.0f, 1.0f);

    /* Longitud variable — un terciopelo real no es un césped inglés */
    float len = 0.55f + 0.45f * id;

    /* Inclinación preferente (el "pelo") */
    float lean = 0.0f;
    if (dir) {
        float dl = rl_sqrtf(dir[0]*dir[0] + dir[2]*dir[2]);
        if (dl > 1e-6f) {
            float proj = (p[0]*dir[0] + p[2]*dir[2]) / dl;
            lean = rl_sinf(proj * freq * 0.5f) * 0.10f;
        }
    }
    return rh_clamp(tip * len + lean + 0.06f, 0.0f, 1.0f);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §3  EL DESPACHADOR
 * ═══════════════════════════════════════════════════════════════════════════ */
float rig_height3d(const float p[3], RigHeightFn fn, float freq,
                   const float dir[3])
{
    if (!p) return 1.0f;
    if (freq <= 1e-5f) freq = 1.0f;

    switch (fn) {
    case RIG_HFN_NONE:     return 1.0f;
    case RIG_HFN_PORES:    return rig_h_pores   (p, freq);
    case RIG_HFN_WEAVE:    return rig_h_weave   (p, freq, dir);
    case RIG_HFN_FBM:      return rig_h_fbm     (p, freq);
    case RIG_HFN_SCALES:   return rig_h_scales  (p, freq);
    case RIG_HFN_BRUSHED:  return rig_h_brushed (p, freq, dir);
    case RIG_HFN_HAMMERED: return rig_h_hammered(p, freq);
    case RIG_HFN_KNURL:    return rig_h_knurl   (p, freq, dir);
    case RIG_HFN_CRACKLE:  return rig_h_crackle (p, freq);
    case RIG_HFN_FIBER:    return rig_h_fiber   (p, freq, dir);
    case RIG_HFN_IMPASTO:  return rig_h_fbm     (p, freq * 0.7f); /* fallback:
                              el impasto real necesita el campo de dabs      */
    default:               return 1.0f;
    }
}

void rig_height3d_normal(const float p[3], RigHeightFn fn, float freq,
                         const float dir[3], float eps, float out_n[3])
{
    if (!p || !out_n) return;
    if (eps <= 1e-6f) eps = 0.004f;

    float q[3];
    float hx0, hx1, hy0, hy1, hz0, hz1;

    q[0]=p[0]-eps; q[1]=p[1];     q[2]=p[2];     hx0 = rig_height3d(q,fn,freq,dir);
    q[0]=p[0]+eps;                               hx1 = rig_height3d(q,fn,freq,dir);
    q[0]=p[0];     q[1]=p[1]-eps;                hy0 = rig_height3d(q,fn,freq,dir);
                   q[1]=p[1]+eps;                hy1 = rig_height3d(q,fn,freq,dir);
                   q[1]=p[1];     q[2]=p[2]-eps; hz0 = rig_height3d(q,fn,freq,dir);
                                  q[2]=p[2]+eps; hz1 = rig_height3d(q,fn,freq,dir);

    float gx = (hx1 - hx0) / (2.0f * eps);
    float gy = (hy1 - hy0) / (2.0f * eps);
    float gz = (hz1 - hz0) / (2.0f * eps);

    /* La normal apunta CONTRA el gradiente (hacia arriba desde el valle) */
    float nx = -gx, ny = 1.0f, nz = -gz;
    (void)gy;

    float l = rl_sqrtf(nx*nx + ny*ny + nz*nz);
    if (l < 1e-8f) { out_n[0]=0; out_n[1]=1; out_n[2]=0; return; }
    out_n[0] = nx/l; out_n[1] = ny/l; out_n[2] = nz/l;
}

/* ★ Lo que el HÁPTICO necesita: la aspereza del punto EXACTO de contacto.
 * Un cuero tiene zonas con poros grandes y zonas casi lisas. El dedo lo nota.
 * Esto muestrea la varianza local del height field. */
float rig_height3d_local_roughness(const float p[3], RigHeightFn fn,
                                   float freq, float radius)
{
    if (!p) return 0.0f;
    if (radius <= 1e-6f) radius = 0.02f;

    /* 8 muestras en un patrón de Fibonacci (φ) alrededor del punto */
    float sum = 0.0f, sum2 = 0.0f;
    const int N = 8;

    for (int i = 0; i < N; i++) {
        float t   = ((float)i + 0.5f) / (float)N;
        float ang = (float)i * 2.39996323f;         /* ★ ángulo áureo */
        float r   = radius * rl_sqrtf(t);

        float q[3] = { p[0] + rl_cosf(ang) * r,
                       p[1],
                       p[2] + rl_sinf(ang) * r };

        float h = rig_height3d(q, fn, freq, 0);
        sum  += h;
        sum2 += h * h;
    }
    float mean = sum / (float)N;
    float var  = rh_max(sum2 / (float)N - mean*mean, 0.0f);

    /* La desviación típica ES la aspereza percibida */
    return rh_clamp(rl_sqrtf(var) * 3.4f, 0.0f, 1.0f);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §4  ★ IMPASTO — EL PUENTE CON RIGART ★
 *
 * Cuando el pincel de óleo deposita pigmento, deja RELIEVE FÍSICO.
 *
 * Antes ese relieve solo existía como color (un "fake" de iluminación en el
 * canvas 2D). Ahora existe como GEOMETRÍA: entra en el POM, proyecta sombra
 * dentro de sus propios surcos, y —esto es lo importante— SE PUEDE TOCAR.
 *
 * Pasas el dedo por un impasto y sientes la pincelada.
 * ═══════════════════════════════════════════════════════════════════════════ */

int rig_impasto_init(RigImpastoField *f)
{
    if (!f) return -1;
    rl_memset(f, 0, sizeof(*f));
    f->global_scale = 1.0f;
    return 0;
}

int rig_impasto_add(RigImpastoField *f, float x, float y, float r,
                    float h, float visc)
{
    if (!f || f->count >= RIG_IMPASTO_MAX) return -1;

    RigImpastoDab *d = &f->dabs[f->count];
    d->x         = x;
    d->y         = y;
    d->radius    = rh_max(r, 1e-4f);
    d->height    = h;
    d->viscosity = rh_clamp(visc, 0.0f, 1.0f);
    /* Semilla determinista a partir de la posición: el mismo trazo produce
     * siempre el mismo relieve. Reproducibilidad soberana. */
    d->seed      = (uint32_t)(hash13(x*127.1f, y*311.7f, (float)f->count) * 4294967295.0f);

    f->count++;
    return 0;
}

float rig_impasto_height(const RigImpastoField *f, float x, float y)
{
    if (!f || f->count == 0) return 0.0f;

    float total = 0.0f;

    for (uint32_t i = 0; i < f->count; i++) {
        const RigImpastoDab *d = &f->dabs[i];

        float dx = x - d->x;
        float dy = y - d->y;
        float dist = rl_sqrtf(dx*dx + dy*dy);

        if (dist > d->radius) continue;

        float t = dist / d->radius;

        /* Cúpula base del daba */
        float dome = 1.0f - t*t;
        dome = rh_max(dome, 0.0f);

        /* ★ RELIEVE VORONOI-φ del óleo viscoso.
         * El pigmento espeso no se aplana: forma crestas irregulares al
         * levantar el pincel. Esta es la fórmula del impasto de RigArt:
         *
         *      0.5 + 0.5·sin(φ · (i + nb))
         *
         * portada al espacio continuo. */
        float sx = x * 180.0f + (float)(d->seed & 0xFFFFu) * 0.013f;
        float sy = y * 180.0f + (float)((d->seed >> 16) & 0xFFFFu) * 0.011f;

        float ridge = 0.5f + 0.5f * rl_sinf(RH_PHI * (sx + sy));
        float ridge2= 0.5f + 0.5f * rl_sinf(RH_PHI * RH_PHI * (sx - sy*0.6f));
        float relief = (ridge * 0.62f + ridge2 * 0.38f);

        /* ★ El BORDE se levanta más que el centro.
         * Cuando levantas el pincel, la viscosidad tira del pigmento hacia
         * arriba en el perímetro del trazo. Es lo que hace que un impasto
         * tenga esos bordes afilados que capturan la luz. */
        float rim = rh_smooth(rh_clamp((t - 0.55f) / 0.35f, 0.0f, 1.0f))
                  * rh_smooth(rh_clamp((1.0f - t) / 0.22f, 0.0f, 1.0f));

        float h = d->height * (
              dome * (0.62f + 0.38f * relief * d->viscosity)
            + rim  * d->viscosity * 0.55f
        );

        /* Los trazos se ACUMULAN — el óleo se apila */
        total += h;
    }

    return rh_clamp(total * f->global_scale, 0.0f, 4.0f);
}
