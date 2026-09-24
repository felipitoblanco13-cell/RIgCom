/* ═══════════════════════════════════════════════════════════════════════════
 * rig_env.c — MOTOR DE ENTORNO SOBERANO · RIGCOM MASTER
 *
 * ESTO ES LO QUE FALTABA. La causa raíz de que todo se viera de plástico.
 *
 * El cerebro no reconoce un material por su color, sino por CÓMO DEFORMA
 * EL ENTORNO EN SU REFLEJO. El oro pulido y el plástico blanco pulido dan
 * el MISMO punto brillante bajo una luz puntual. Lo que los distingue es
 * lo que se refleja ALREDEDOR de ese punto: el oro refleja la habitación
 * entera, teñida de dorado, deformada por su curvatura.
 *
 * Con `ambient = color × strength × albedo` y metallic=1 (que hace kD=0),
 * un metal NO TIENE NADA QUE REFLEJAR. Es una esfera negra con dos puntitos.
 *
 * Sin entorno, el material no tiene dónde reflejarse.
 * Y sin reflejo, no hay material.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * TRES PIEZAS, TODAS PROCEDURALES:
 *
 *   1. RADIANCIA        — una FUNCIÓN. Cielo + sol + softboxes. Sin assets.
 *   2. IRRADIANCIA SH   — 9 coeficientes vec3. CERO samplers. Todo el
 *                         difuso ambiental del universo cabe en 27 floats.
 *   3. ESPECULAR MIPS   — 6 niveles GGX-convolucionados. mip0=espejo,
 *                         mip5=mate. El material elige su mip por rugosidad.
 *
 * Y el cuarto término (el BRDF ambiental) NO NECESITA TEXTURA: es analítico.
 *
 * C11 · cero libc · cero assets · φ
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rig_env.h"

extern float rl_sqrtf(float);
extern float rl_powf(float, float);
extern float rl_expf(float);
extern float rl_logf(float);
extern float rl_sinf(float);
extern float rl_cosf(float);
extern float rl_acosf(float);
extern float rl_atan2f(float, float);
extern float rl_fabsf(float);
extern float rl_exp2f(float);
extern void *rl_malloc(unsigned long);
extern void  rl_free(void *);
extern void *rl_memset(void *, int, unsigned long);

static float re_clamp(float x, float a, float b){ return x<a?a:(x>b?b:x); }
static float re_min(float a, float b){ return a<b?a:b; }
static float re_max(float a, float b){ return a>b?a:b; }

static void  v3_set(float o[3], float x, float y, float z){ o[0]=x;o[1]=y;o[2]=z; }
static float v3_dot(const float a[3], const float b[3]){
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
static void  v3_cross(const float a[3], const float b[3], float o[3]){
    float x = a[1]*b[2] - a[2]*b[1];
    float y = a[2]*b[0] - a[0]*b[2];
    float z = a[0]*b[1] - a[1]*b[0];
    o[0]=x; o[1]=y; o[2]=z;
}
static void  v3_norm(float v[3]){
    float l = rl_sqrtf(v3_dot(v,v));
    if (l > 1e-9f) { v[0]/=l; v[1]/=l; v[2]/=l; }
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §1  TEMPERATURA DE COLOR → RGB   (cuerpo negro, Planck)
 *
 * Un softbox de 5600 K (luz de día) y uno de 3200 K (tungsteno) no son
 * "blanco frío" y "blanco cálido": son puntos concretos del locus de Planck.
 * Esto los calcula de verdad, sin tabla.
 * ═══════════════════════════════════════════════════════════════════════════ */
void rig_env_kelvin_to_rgb(float kelvin, float out[3])
{
    float t = re_clamp(kelvin, 1000.0f, 15000.0f) / 100.0f;
    float r, g, b;

    if (t <= 66.0f) {
        r = 255.0f;
        g = 99.4708025861f * rl_logf(t) - 161.1195681661f;
        b = (t <= 19.0f) ? 0.0f
          : 138.5177312231f * rl_logf(t - 10.0f) - 305.0447927307f;
    } else {
        r = 329.698727446f * rl_powf(t - 60.0f, -0.1332047592f);
        g = 288.1221695283f * rl_powf(t - 60.0f, -0.0755148492f);
        b = 255.0f;
    }
    out[0] = re_clamp(r, 0.0f, 255.0f) / 255.0f;
    out[1] = re_clamp(g, 0.0f, 255.0f) / 255.0f;
    out[2] = re_clamp(b, 0.0f, 255.0f) / 255.0f;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §2  ★ LA RADIANCIA ★  —  una función, no un archivo
 *
 * Dada una dirección, devuelve cuánta luz llega de ahí.
 * Esto es TODO el entorno. Cero bytes en disco.
 * ═══════════════════════════════════════════════════════════════════════════ */
void rig_env_radiance(const RigEnvDesc *e, const float d[3], float out[3])
{
    if (!e || !d || !out) return;

    const float y = re_clamp(d[1], -1.0f, 1.0f);

    /* ── 1. GRADIENTE CIELO / SUELO ───────────────────────────────────────
     * El cielo no es un lerp lineal: cambia MUY rápido cerca del horizonte
     * (más masa de aire → más dispersión Rayleigh) y despacio en el cenit.
     * El exponente 0.42 reproduce esa curva. */
    if (y >= 0.0f) {
        float t = rl_powf(y, 0.42f);
        /* Rayleigh: el azul se dispersa ~5.5× más que el rojo (λ⁻⁴).
         * Por eso el cenit es azul y el horizonte es blanquecino. */
        float haze = (1.0f - t) * e->turbidity * 0.18f;
        out[0] = e->horizon[0] + (e->zenith[0] - e->horizon[0]) * t + haze * 0.55f;
        out[1] = e->horizon[1] + (e->zenith[1] - e->horizon[1]) * t + haze * 0.75f;
        out[2] = e->horizon[2] + (e->zenith[2] - e->horizon[2]) * t + haze * 1.00f;
    } else {
        /* Suelo: rebote difuso que se apaga hacia abajo (oclusión propia) */
        float t = rl_powf(-y, 0.55f);
        out[0] = e->horizon[0] + (e->ground[0] - e->horizon[0]) * t;
        out[1] = e->horizon[1] + (e->ground[1] - e->horizon[1]) * t;
        out[2] = e->horizon[2] + (e->ground[2] - e->horizon[2]) * t;
    }

    /* ── 2. DISCO SOLAR ───────────────────────────────────────────────────
     * El sol NO es un punto: subtiende 0.53° (radio angular 0.00465 rad).
     * Ese tamaño finito es lo que da los highlights especulares con forma,
     * y las sombras con penumbra. Un sol puntual da highlights de aguja
     * que se ven falsos. */
    if (e->sun_intensity > 1e-5f) {
        float cd = re_clamp(v3_dot(d, e->sun_dir), -1.0f, 1.0f);
        float ca = rl_cosf(e->sun_angular_radius);

        if (cd > ca) {
            /* DENTRO del disco — con oscurecimiento del limbo.
             * El borde del sol es un ~40% más oscuro que el centro
             * (la línea de visión atraviesa capas más frías). */
            float u    = (cd - ca) / re_max(1.0f - ca, 1e-6f);  /* 0 borde, 1 centro */
            float limb = 0.62f + 0.38f * rl_sqrtf(re_max(u, 0.0f));
            out[0] += e->sun_color[0] * e->sun_intensity * limb;
            out[1] += e->sun_color[1] * e->sun_intensity * limb;
            out[2] += e->sun_color[2] * e->sun_intensity * limb;
        } else {
            /* HALO — dispersión de Mie hacia adelante.
             * Las partículas grandes (polvo, agua) dispersan hacia delante,
             * creando el resplandor alrededor del sol. */
            float ang = rl_acosf(cd);
            float turb = re_max(e->turbidity, 0.5f);
            float mie  = rl_expf(-ang * (14.0f / turb));
            float k    = mie * 0.055f * e->sun_intensity;
            out[0] += e->sun_color[0] * k;
            out[1] += e->sun_color[1] * k;
            out[2] += e->sun_color[2] * k;
        }
    }

    /* ── 3. SOFTBOXES ─────────────────────────────────────────────────────
     * Rectángulos emisivos. ESTO es lo que hace que un metal se vea como
     * metal de estudio: el reflejo de la caja de luz, con su forma
     * rectangular, alargada, con bordes definidos.
     *
     * Intersección rayo-rectángulo desde el origen. */
    for (uint32_t i = 0; i < e->box_count && i < RIG_ENV_MAX_BOXES; i++) {
        const RigSoftbox *b = &e->box[i];
        if (b->intensity <= 1e-5f) continue;

        float ndotd = v3_dot(b->normal, d);
        if (ndotd > -1e-4f) continue;              /* la caja mira al revés */

        float denom = ndotd;
        float t = v3_dot(b->normal, b->pos) / denom;
        if (t <= 1e-4f) continue;                  /* detrás del observador */

        /* Punto de impacto en el plano de la caja */
        float p[3] = { d[0]*t - b->pos[0],
                       d[1]*t - b->pos[1],
                       d[2]*t - b->pos[2] };

        /* Ejes locales */
        float right[3];
        v3_cross(b->up, b->normal, right);
        v3_norm(right);

        float lu = v3_dot(p, right);
        float lv = v3_dot(p, b->up);

        float hw = b->size[0] * 0.5f;
        float hh = b->size[1] * 0.5f;

        if (rl_fabsf(lu) < hw && rl_fabsf(lv) < hh) {
            /* Dentro. Falloff suave en el borde (difusor de la softbox). */
            float eu = 1.0f - rl_powf(rl_fabsf(lu) / hw, 6.0f);
            float ev = 1.0f - rl_powf(rl_fabsf(lv) / hh, 6.0f);
            float edge = re_clamp(eu * ev, 0.0f, 1.0f);

            float col[3];
            if (b->temp_k > 100.0f) rig_env_kelvin_to_rgb(b->temp_k, col);
            else { col[0]=b->color[0]; col[1]=b->color[1]; col[2]=b->color[2]; }

            float k = b->intensity * edge;
            out[0] += col[0] * k;
            out[1] += col[1] * k;
            out[2] += col[2] * k;
        }
    }

    out[0] *= e->exposure;
    out[1] *= e->exposure;
    out[2] *= e->exposure;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §3  CUBEMAP — direcciones y muestreo
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Dirección del centro de un téxel de la cara `face` */
void rig_env_cube_dir(uint32_t face, float u, float v, float d[3])
{
    /* u,v ∈ [-1, 1] */
    switch (face) {
    case 0: v3_set(d,  1.0f,   -v,   -u); break;  /* +X */
    case 1: v3_set(d, -1.0f,   -v,    u); break;  /* -X */
    case 2: v3_set(d,    u,  1.0f,    v); break;  /* +Y */
    case 3: v3_set(d,    u, -1.0f,   -v); break;  /* -Y */
    case 4: v3_set(d,    u,   -v,  1.0f); break;  /* +Z */
    default:v3_set(d,   -u,   -v, -1.0f); break;  /* -Z */
    }
    v3_norm(d);
}

/* Muestreo bilineal del cubemap en una dirección */
static void cube_sample(const RigEnvCube *c, uint32_t mip,
                        const float d[3], float out[3])
{
    float ax = rl_fabsf(d[0]), ay = rl_fabsf(d[1]), az = rl_fabsf(d[2]);
    uint32_t face; float sc, tc, ma;

    if (ax >= ay && ax >= az) {
        ma = ax;
        if (d[0] > 0) { face = 0; sc = -d[2]; tc = -d[1]; }
        else          { face = 1; sc =  d[2]; tc = -d[1]; }
    } else if (ay >= az) {
        ma = ay;
        if (d[1] > 0) { face = 2; sc =  d[0]; tc =  d[2]; }
        else          { face = 3; sc =  d[0]; tc = -d[2]; }
    } else {
        ma = az;
        if (d[2] > 0) { face = 4; sc =  d[0]; tc = -d[1]; }
        else          { face = 5; sc = -d[0]; tc = -d[1]; }
    }
    if (ma < 1e-9f) ma = 1e-9f;

    uint32_t sz = c->size >> mip;
    if (sz < 1) sz = 1;

    float fu = (sc / ma * 0.5f + 0.5f) * (float)sz - 0.5f;
    float fv = (tc / ma * 0.5f + 0.5f) * (float)sz - 0.5f;

    int x0 = (int)(fu < 0 ? fu - 1.0f : fu);
    int y0 = (int)(fv < 0 ? fv - 1.0f : fv);
    float wx = fu - (float)x0;
    float wy = fv - (float)y0;

    const float *P = c->data[face][mip];
    out[0] = out[1] = out[2] = 0.0f;

    for (int j = 0; j < 2; j++) {
        int yy = y0 + j;
        yy = yy < 0 ? 0 : (yy >= (int)sz ? (int)sz - 1 : yy);
        float wj = j ? wy : (1.0f - wy);
        for (int i = 0; i < 2; i++) {
            int xx = x0 + i;
            xx = xx < 0 ? 0 : (xx >= (int)sz ? (int)sz - 1 : xx);
            float wi = i ? wx : (1.0f - wx);
            float w  = wi * wj;
            const float *T = &P[((uint32_t)yy * sz + (uint32_t)xx) * 3u];
            out[0] += T[0] * w;
            out[1] += T[1] * w;
            out[2] += T[2] * w;
        }
    }
}

void rig_env_cube_sample(const RigEnvCube *c, uint32_t mip,
                         const float d[3], float out[3])
{
    if (!c || !d || !out) return;
    if (mip >= c->mips) mip = c->mips - 1u;
    cube_sample(c, mip, d, out);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §4  BAKE — evaluar la función de radiancia en las 6 caras
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_env_cube_alloc(RigEnvCube *c, uint32_t size, uint32_t mips)
{
    if (!c || size == 0 || mips == 0 || mips > RIG_ENV_MAX_MIPS) return -1;
    rl_memset(c, 0, sizeof(*c));
    c->size = size;
    c->mips = mips;

    for (uint32_t f = 0; f < 6; f++) {
        for (uint32_t m = 0; m < mips; m++) {
            uint32_t sz = size >> m;
            if (sz < 1) sz = 1;
            unsigned long bytes = (unsigned long)sz * sz * 3u * sizeof(float);
            c->data[f][m] = (float*)rl_malloc(bytes);
            if (!c->data[f][m]) { rig_env_cube_free(c); return -1; }
            rl_memset(c->data[f][m], 0, bytes);
        }
    }
    return 0;
}

void rig_env_cube_free(RigEnvCube *c)
{
    if (!c) return;
    for (uint32_t f = 0; f < 6; f++)
        for (uint32_t m = 0; m < RIG_ENV_MAX_MIPS; m++)
            if (c->data[f][m]) { rl_free(c->data[f][m]); c->data[f][m] = 0; }
    rl_memset(c, 0, sizeof(*c));
}

int rig_env_bake(const RigEnvDesc *e, RigEnvCube *c)
{
    if (!e || !c || !c->data[0][0]) return -1;
    uint32_t sz = c->size;

    for (uint32_t f = 0; f < 6; f++) {
        float *P = c->data[f][0];
        for (uint32_t y = 0; y < sz; y++) {
            float v = ((float)y + 0.5f) / (float)sz * 2.0f - 1.0f;
            for (uint32_t x = 0; x < sz; x++) {
                float u = ((float)x + 0.5f) / (float)sz * 2.0f - 1.0f;
                float d[3], rad[3];
                rig_env_cube_dir(f, u, v, d);
                rig_env_radiance(e, d, rad);
                float *T = &P[(y * sz + x) * 3u];
                T[0] = rad[0]; T[1] = rad[1]; T[2] = rad[2];
            }
        }
    }
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §5  ★ IRRADIANCIA POR ARMÓNICOS ESFÉRICOS ★
 *
 * Todo el difuso ambiental del universo cabe en 9 vec3 = 27 floats.
 * CERO samplers en el shader. Una función.
 *
 * Ramamoorthi & Hanrahan (2001) demostraron que el difuso lambertiano actúa
 * como un filtro paso-bajo tan agresivo que la banda 2 captura el 99% de la
 * energía. Ir más allá es desperdiciar memoria.
 * ═══════════════════════════════════════════════════════════════════════════ */

static void sh_basis(const float d[3], float Y[9])
{
    const float x = d[0], y = d[1], z = d[2];
    Y[0] = 0.2820947918f;                        /* L0,0   */
    Y[1] = 0.4886025119f * y;                    /* L1,-1  */
    Y[2] = 0.4886025119f * z;                    /* L1,0   */
    Y[3] = 0.4886025119f * x;                    /* L1,1   */
    Y[4] = 1.0925484306f * x * y;                /* L2,-2  */
    Y[5] = 1.0925484306f * y * z;                /* L2,-1  */
    Y[6] = 0.3153915653f * (3.0f*z*z - 1.0f);    /* L2,0   */
    Y[7] = 1.0925484306f * x * z;                /* L2,1   */
    Y[8] = 0.5462742153f * (x*x - y*y);          /* L2,2   */
}

/* Ángulo sólido exacto de un téxel del cubemap.
 * Sin esto, los téxeles de las esquinas (que cubren MENOS ángulo sólido)
 * pesarían igual que los del centro, y la irradiancia saldría sesgada. */
static float area_element(float x, float y)
{
    return rl_atan2f(x * y, rl_sqrtf(x*x + y*y + 1.0f));
}
static float texel_solid_angle(float u, float v, float inv_sz)
{
    float x0 = u - inv_sz, x1 = u + inv_sz;
    float y0 = v - inv_sz, y1 = v + inv_sz;
    return area_element(x0, y0) - area_element(x0, y1)
         - area_element(x1, y0) + area_element(x1, y1);
}

int rig_env_project_sh(const RigEnvCube *c, float sh[9][3])
{
    if (!c || !sh || !c->data[0][0]) return -1;

    for (int i = 0; i < 9; i++) sh[i][0] = sh[i][1] = sh[i][2] = 0.0f;

    uint32_t sz = c->size;
    float inv_sz = 1.0f / (float)sz;
    float total = 0.0f;

    for (uint32_t f = 0; f < 6; f++) {
        const float *P = c->data[f][0];
        for (uint32_t y = 0; y < sz; y++) {
            float v = ((float)y + 0.5f) * inv_sz * 2.0f - 1.0f;
            for (uint32_t x = 0; x < sz; x++) {
                float u = ((float)x + 0.5f) * inv_sz * 2.0f - 1.0f;

                float d[3], Y[9];
                rig_env_cube_dir(f, u, v, d);
                sh_basis(d, Y);

                float dw = texel_solid_angle(u, v, inv_sz);
                total += dw;

                const float *T = &P[(y * sz + x) * 3u];
                for (int i = 0; i < 9; i++) {
                    float w = Y[i] * dw;
                    sh[i][0] += T[0] * w;
                    sh[i][1] += T[1] * w;
                    sh[i][2] += T[2] * w;
                }
            }
        }
    }
    (void)total;   /* debe converger a 4π */

    /* ── CONVOLUCIÓN CON EL COSENO (Ramamoorthi) ─────────────────────────
     * La irradiancia NO es la radiancia: hay que convolucionarla con el
     * lóbulo coseno del BRDF lambertiano. En SH eso es multiplicar cada
     * banda por una constante. Es el paso que casi todo el mundo olvida
     * y que hace que la iluminación difusa salga demasiado contrastada. */
    const float A0 = 3.141593f;   /* π       — banda 0 */
    const float A1 = 2.094395f;   /* 2π/3    — banda 1 */
    const float A2 = 0.785398f;   /* π/4     — banda 2 */

    const float Ahat[9] = { A0, A1, A1, A1, A2, A2, A2, A2, A2 };
    for (int i = 0; i < 9; i++) {
        sh[i][0] *= Ahat[i];
        sh[i][1] *= Ahat[i];
        sh[i][2] *= Ahat[i];
    }
    return 0;
}

/* Evaluación en CPU (para el rasterizador software SR) */
void rig_env_irradiance_sh(const float sh[9][3], const float n[3], float out[3])
{
    float Y[9];
    sh_basis(n, Y);
    out[0] = out[1] = out[2] = 0.0f;
    for (int i = 0; i < 9; i++) {
        out[0] += sh[i][0] * Y[i];
        out[1] += sh[i][1] * Y[i];
        out[2] += sh[i][2] * Y[i];
    }
    out[0] = re_max(out[0], 0.0f);
    out[1] = re_max(out[1], 0.0f);
    out[2] = re_max(out[2], 0.0f);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §6  ★ PREFILTRADO GGX ★  — el especular ambiental
 *
 * Cada mip es el MISMO entorno visto con distinta rugosidad:
 *
 *    mip 0 → rough 0.0   espejo nítido      → Oro Pulido
 *    mip 1 → rough 0.2
 *    mip 2 → rough 0.4
 *    mip 3 → rough 0.6                      → Titanio Mate
 *    mip 4 → rough 0.8
 *    mip 5 → rough 1.0   borroso total      → Roca Volcánica
 *
 * El material solo tiene que decir su rugosidad, y sale del mip correcto.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Van der Corput — la base de la secuencia de Hammersley.
 * Baja discrepancia: cubre el dominio mucho mejor que el random puro,
 * así que converge con muchas menos muestras. */
static float radical_inverse_vdc(uint32_t bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return (float)bits * 2.3283064365386963e-10f;   /* / 2^32 */
}

static void hammersley(uint32_t i, uint32_t N, float *u1, float *u2)
{
    *u1 = (float)i / (float)N;
    *u2 = radical_inverse_vdc(i);
}

/* Muestreo por importancia del lóbulo GGX.
 * Genera un half-vector H distribuido según la NDF de GGX, así que las
 * muestras van donde el BRDF tiene energía en vez de repartirse a ciegas. */
static void importance_sample_ggx(float u1, float u2, float rough,
                                  const float N[3], float H[3])
{
    float a = rough * rough;

    float phi       = RIG_TAU * u1;
    float cos_theta = rl_sqrtf((1.0f - u2) / (1.0f + (a*a - 1.0f) * u2));
    float sin_theta = rl_sqrtf(re_max(0.0f, 1.0f - cos_theta * cos_theta));

    float hx = sin_theta * rl_cosf(phi);
    float hy = sin_theta * rl_sinf(phi);
    float hz = cos_theta;

    /* Base ortonormal alrededor de N */
    float up[3];
    if (rl_fabsf(N[2]) < 0.999f) v3_set(up, 0.0f, 0.0f, 1.0f);
    else                          v3_set(up, 1.0f, 0.0f, 0.0f);

    float T[3], B[3];
    v3_cross(up, N, T); v3_norm(T);
    v3_cross(N, T, B);

    H[0] = T[0]*hx + B[0]*hy + N[0]*hz;
    H[1] = T[1]*hx + B[1]*hy + N[1]*hz;
    H[2] = T[2]*hx + B[2]*hy + N[2]*hz;
    v3_norm(H);
}

int rig_env_prefilter(const RigEnvCube *src, RigEnvCube *dst, uint32_t samples)
{
    if (!src || !dst || !src->data[0][0] || !dst->data[0][0]) return -1;
    if (samples == 0) samples = 128u;

    uint32_t mips = dst->mips;

    for (uint32_t mip = 0; mip < mips; mip++) {

        uint32_t sz = dst->size >> mip;
        if (sz < 1) sz = 1;

        float rough = (mips > 1)
                    ? (float)mip / (float)(mips - 1u)
                    : 0.0f;

        for (uint32_t f = 0; f < 6; f++) {
            float *P = dst->data[f][mip];

            for (uint32_t y = 0; y < sz; y++) {
                float v = ((float)y + 0.5f) / (float)sz * 2.0f - 1.0f;
                for (uint32_t x = 0; x < sz; x++) {
                    float u = ((float)x + 0.5f) / (float)sz * 2.0f - 1.0f;

                    float N[3];
                    rig_env_cube_dir(f, u, v, N);

                    float *T = &P[(y * sz + x) * 3u];

                    /* mip 0 = espejo: copia directa, sin convolucionar */
                    if (mip == 0 || rough < 1e-4f) {
                        float rad[3];
                        cube_sample(src, 0, N, rad);
                        T[0] = rad[0]; T[1] = rad[1]; T[2] = rad[2];
                        continue;
                    }

                    /* Aproximación de Karis: asumir V = R = N.
                     * Se pierde el estiramiento del lóbulo en ángulo
                     * rasante, pero permite precomputar UN cubemap en vez
                     * de uno por ángulo de vista. Es el compromiso que
                     * hizo viable el PBR en tiempo real. */
                    const float *V = N;

                    float sum[3] = {0,0,0};
                    float wsum   = 0.0f;

                    for (uint32_t s = 0; s < samples; s++) {
                        float u1, u2;
                        hammersley(s, samples, &u1, &u2);

                        float H[3];
                        importance_sample_ggx(u1, u2, rough, N, H);

                        /* L = reflejo de V en H */
                        float vdh = v3_dot(V, H);
                        float L[3] = { 2.0f*vdh*H[0] - V[0],
                                       2.0f*vdh*H[1] - V[1],
                                       2.0f*vdh*H[2] - V[2] };
                        v3_norm(L);

                        float ndl = v3_dot(N, L);
                        if (ndl <= 0.0f) continue;

                        float rad[3];
                        cube_sample(src, 0, L, rad);

                        sum[0] += rad[0] * ndl;
                        sum[1] += rad[1] * ndl;
                        sum[2] += rad[2] * ndl;
                        wsum   += ndl;
                    }

                    if (wsum > 1e-6f) {
                        T[0] = sum[0] / wsum;
                        T[1] = sum[1] / wsum;
                        T[2] = sum[2] / wsum;
                    } else {
                        float rad[3];
                        cube_sample(src, 0, N, rad);
                        T[0] = rad[0]; T[1] = rad[1]; T[2] = rad[2];
                    }
                }
            }
        }
    }
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §7  ★ EnvBRDF ANALÍTICO ★  —  CERO TEXTURAS
 *
 * La segunda mitad del split-sum. Karis lo publicó como una LUT 2D, pero
 * también dio un ajuste analítico con error < 1%.
 *
 * Para RIGCOM —soberano, móvil, cero assets— ESTA es la versión correcta.
 * La matemática ES el recurso.
 *
 * Devuelve (scale, bias) tal que:
 *      spec_IBL = prefiltered(R, rough) · (F0 · scale + bias)
 * ═══════════════════════════════════════════════════════════════════════════ */
void rig_env_brdf_approx(float NoV, float rough, float *scale, float *bias)
{
    NoV   = re_clamp(NoV,   0.0f, 1.0f);
    rough = re_clamp(rough, 0.0f, 1.0f);

    const float c0x = -1.0f,  c0y = -0.0275f, c0z = -0.572f, c0w =  0.022f;
    const float c1x =  1.0f,  c1y =  0.0425f, c1z =  1.040f, c1w = -0.040f;

    float rx = rough * c0x + c1x;
    float ry = rough * c0y + c1y;
    float rz = rough * c0z + c1z;
    float rw = rough * c0w + c1w;

    float a004 = re_min(rx * rx, rl_exp2f(-9.28f * NoV)) * rx + ry;

    if (scale) *scale = -1.04f * a004 + rz;
    if (bias)  *bias  =  1.04f * a004 + rw;
}

/* Compensación de energía multiscatter (Kulla-Conty).
 * GGX de una sola dispersión pierde ~25% de energía a rough 0.6 y ~40% a 0.9.
 * Por eso los materiales rugosos salen grises y sucios, y uno culpa a la
 * paleta. Esto lo devuelve, y sale GRATIS del EnvBRDF que ya calculamos. */
void rig_env_multiscatter(const float F0[3], float scale, float bias,
                          float out_gain[3])
{
    float Ess = re_max(scale + bias, 1e-4f);
    float k   = (1.0f - Ess) / Ess;
    out_gain[0] = 1.0f + F0[0] * k;
    out_gain[1] = 1.0f + F0[1] * k;
    out_gain[2] = 1.0f + F0[2] * k;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §8  PRESETS DE ENTORNO
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_env_preset(RigEnvDesc *e, RigEnvPreset p)
{
    if (!e) return -1;
    rl_memset(e, 0, sizeof(*e));
    e->exposure           = 1.0f;
    e->turbidity          = 2.2f;
    e->sun_angular_radius = 0.00465f;    /* 0.53° — el sol real */

    switch (p) {

    case RIG_ENV_STUDIO:
        /* Tres softboxes. El entorno que hace que un metal parezca metal. */
        v3_set(e->zenith,  0.055f, 0.058f, 0.068f);
        v3_set(e->horizon, 0.035f, 0.036f, 0.042f);
        v3_set(e->ground,  0.012f, 0.011f, 0.014f);
        e->sun_intensity = 0.0f;

        e->box_count = 3;
        /* KEY — grande, cálida, arriba-izquierda */
        v3_set(e->box[0].pos,    -1.6f,  1.4f,  1.9f);
        v3_set(e->box[0].normal,  0.55f,-0.45f,-0.70f);
        v3_set(e->box[0].up,      0.0f,  1.0f,  0.0f);
        e->box[0].size[0] = 2.2f; e->box[0].size[1] = 3.0f;
        e->box[0].temp_k    = 5600.0f;
        e->box[0].intensity = 9.0f;
        /* FILL — grande, fría, derecha, suave */
        v3_set(e->box[1].pos,     2.2f,  0.3f,  1.2f);
        v3_set(e->box[1].normal, -0.85f,-0.10f,-0.52f);
        v3_set(e->box[1].up,      0.0f,  1.0f,  0.0f);
        e->box[1].size[0] = 3.0f; e->box[1].size[1] = 3.0f;
        e->box[1].temp_k    = 7200.0f;
        e->box[1].intensity = 2.0f;
        /* RIM — estrecha, detrás, para el contorno */
        v3_set(e->box[2].pos,     0.4f,  1.8f, -2.4f);
        v3_set(e->box[2].normal, -0.10f,-0.55f, 0.83f);
        v3_set(e->box[2].up,      0.0f,  1.0f,  0.0f);
        e->box[2].size[0] = 0.6f; e->box[2].size[1] = 2.6f;
        e->box[2].temp_k    = 4200.0f;
        e->box[2].intensity = 12.0f;
        break;

    case RIG_ENV_DAYLIGHT:
        v3_set(e->zenith,  0.10f, 0.20f, 0.42f);
        v3_set(e->horizon, 0.52f, 0.60f, 0.72f);
        v3_set(e->ground,  0.16f, 0.15f, 0.13f);
        v3_set(e->sun_dir, 0.42f, 0.60f, 0.68f);
        v3_norm(e->sun_dir);
        v3_set(e->sun_color, 1.0f, 0.96f, 0.88f);
        e->sun_intensity = 55.0f;
        e->turbidity     = 2.0f;
        break;

    case RIG_ENV_GOLDEN_HOUR:
        v3_set(e->zenith,  0.10f, 0.14f, 0.30f);
        v3_set(e->horizon, 0.90f, 0.52f, 0.26f);
        v3_set(e->ground,  0.14f, 0.10f, 0.08f);
        v3_set(e->sun_dir, 0.86f, 0.10f, 0.50f);
        v3_norm(e->sun_dir);
        v3_set(e->sun_color, 1.0f, 0.62f, 0.28f);
        e->sun_intensity = 38.0f;
        e->turbidity     = 4.2f;
        break;

    case RIG_ENV_CATHEDRAL:
        /* Vacío cósmico + oro φ — el estilo Aetherium */
        v3_set(e->zenith,  0.039f, 0.020f, 0.063f);
        v3_set(e->horizon, 0.075f, 0.045f, 0.105f);
        v3_set(e->ground,  0.020f, 0.012f, 0.030f);
        e->sun_intensity = 0.0f;

        e->box_count = 2;
        v3_set(e->box[0].pos,    -1.3f,  2.1f,  1.5f);
        v3_set(e->box[0].normal,  0.42f,-0.72f,-0.55f);
        v3_set(e->box[0].up,      0.0f,  1.0f,  0.0f);
        e->box[0].size[0] = 1.1f; e->box[0].size[1] = 2.8f;
        v3_set(e->box[0].color, 0.788f, 0.659f, 0.298f);   /* #c9a84c */
        e->box[0].intensity = 14.0f;

        v3_set(e->box[1].pos,     2.0f, -0.6f, -1.6f);
        v3_set(e->box[1].normal, -0.70f, 0.22f, 0.68f);
        v3_set(e->box[1].up,      0.0f,  1.0f,  0.0f);
        e->box[1].size[0] = 2.4f; e->box[1].size[1] = 1.2f;
        v3_set(e->box[1].color, 0.30f, 0.20f, 0.55f);
        e->box[1].intensity = 3.5f;
        break;

    case RIG_ENV_NIGHT:
    default:
        v3_set(e->zenith,  0.010f, 0.014f, 0.030f);
        v3_set(e->horizon, 0.028f, 0.032f, 0.055f);
        v3_set(e->ground,  0.005f, 0.005f, 0.010f);
        e->sun_intensity = 0.0f;
        break;
    }
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §9  PIPELINE COMPLETO — de un preset a los uniforms de la GPU
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_env_build(const RigEnvDesc *desc, RigEnvGPU *gpu,
                  uint32_t cube_size, uint32_t samples)
{
    RigEnvCube raw;
    int rc;

    if (!desc || !gpu) return -1;
    if (cube_size == 0) cube_size = 128u;

    /* 1. Bake del entorno crudo (solo mip 0) */
    rc = rig_env_cube_alloc(&raw, cube_size, 1u);
    if (rc) return rc;

    rc = rig_env_bake(desc, &raw);
    if (rc) { rig_env_cube_free(&raw); return rc; }

    /* 2. Los 9 coeficientes SH → el difuso entero */
    rc = rig_env_project_sh(&raw, gpu->sh);
    if (rc) { rig_env_cube_free(&raw); return rc; }

    /* 3. Los 6 mips GGX → el especular entero */
    rc = rig_env_cube_alloc(&gpu->spec, cube_size, RIG_ENV_SPEC_MIPS);
    if (rc) { rig_env_cube_free(&raw); return rc; }

    rc = rig_env_prefilter(&raw, &gpu->spec, samples);
    rig_env_cube_free(&raw);
    if (rc) { rig_env_cube_free(&gpu->spec); return rc; }

    gpu->max_mip = (float)(RIG_ENV_SPEC_MIPS - 1u);
    gpu->ready   = true;

    /* 4. El BRDF ambiental NO se hornea: es analítico. Cero texturas. */
    return 0;
}

void rig_env_gpu_free(RigEnvGPU *gpu)
{
    if (!gpu) return;
    rig_env_cube_free(&gpu->spec);
    gpu->ready = false;
}
