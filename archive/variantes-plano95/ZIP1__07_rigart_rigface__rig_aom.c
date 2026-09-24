/* ═══════════════════════════════════════════════════════════════════════════
 * rig_aom.c — MOTOR HOLOGRÁFICO VOLUMÉTRICO AOM · RIGCOM MASTER
 *
 * El color vuelve. La oclusión vuelve. Y el háptico, por fin, sabe si
 * estás tocando algo o si tienes la mano en el vacío.
 *
 * C11 · cero libc · φ
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rig_aom.h"

extern float rl_sqrtf(float);
extern float rl_sinf(float);
extern float rl_cosf(float);
extern float rl_asinf(float);
extern float rl_fabsf(float);
extern void *rl_malloc(unsigned long);
extern void  rl_free(void *);
extern void *rl_memset(void *, int, unsigned long);

#define AOM_PI 3.14159265358979324f

static float ao_clamp(float x, float a, float b){ return x<a?a:(x>b?b:x); }
static float ao_min(float a, float b){ return a<b?a:b; }
static float ao_max(float a, float b){ return a>b?a:b; }
static int   ao_clampi(int x, int a, int b){ return x<a?a:(x>b?b:x); }


/* ═══════════════════════════════════════════════════════════════════════════
 * §1  ★ LA FÍSICA DEL AOM ★
 *
 * Esto es lo que aom_modulate_channel() debería haber sido siempre.
 * Antes era:   eff = (offset != 0) ? 0.95 : 1.0;
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Condición de Bragg:  sin θ = λ_óptica / (2Λ),  con Λ = v_ac / f_ac
 *
 * λ en nm, f en MHz.
 *   Λ [m]  = v [m/s] / (f [MHz] · 1e6)
 *   λ [m]  = λ [nm] · 1e-9
 */
float aom_bragg_angle(float lambda_nm, float f_mhz)
{
    if (f_mhz < 1e-3f) return 0.0f;

    float lambda_m = lambda_nm * 1.0e-9f;
    float Lambda_m = AOM_CRYSTAL_V_AC / (f_mhz * 1.0e6f);

    float s = lambda_m / (2.0f * Lambda_m);
    s = ao_clamp(s, -1.0f, 1.0f);
    return rl_asinf(s);
}

/* ★ La inversa: ¿qué frecuencia acústica lleva ESTE color al ángulo objetivo?
 *
 *      sin θ = λ / (2Λ)  =  λ·f / (2·v)
 *      ⇒  f = 2·v·sin θ / λ            ⇒   f ∝ 1/λ
 *
 * Y de aquí sale el color del AOM: tres λ, tres f, un solo ángulo.
 */
float aom_freq_for_lambda(float lambda_nm, float theta_target)
{
    float lambda_m = lambda_nm * 1.0e-9f;
    if (lambda_m < 1e-12f) return AOM_F_CENTER_MHZ;

    float f_hz = (2.0f * AOM_CRYSTAL_V_AC * rl_sinf(theta_target)) / lambda_m;
    return f_hz * 1.0e-6f;      /* → MHz */
}

/* Eficiencia de difracción — Klein-Cook, régimen de Bragg:
 *
 *      η = sin²( (π/λ) · √( M₂ · L · P / (2·H) ) )
 *
 * NO es lineal en P: SATURA (y con potencia excesiva incluso decae, porque
 * el seno pasa de π/2). Y depende de λ, así que cada canal necesita distinta
 * potencia para difractar con la misma eficiencia. Eso es lo que hay que
 * compensar para que el blanco salga blanco.
 */
float aom_diffraction_efficiency(float lambda_nm, float rf_power_w)
{
    if (rf_power_w <= 0.0f) return 0.0f;

    float lambda_m = lambda_nm * 1.0e-9f;
    float L_m      = AOM_INTERACT_L_MM   * 1.0e-3f;
    float H_m      = AOM_TRANSDUCER_H_MM * 1.0e-3f;

    float inner = (AOM_CRYSTAL_M2 * L_m * rf_power_w) / (2.0f * H_m);
    if (inner < 0.0f) inner = 0.0f;

    float arg = (AOM_PI / lambda_m) * rl_sqrtf(inner);

    float s = rl_sinf(arg);
    return ao_clamp(s * s, 0.0f, 1.0f);
}

int aom_optics_configure(AOMOptics *o, float rf_power_w)
{
    if (!o) return -1;

    /* Ángulo objetivo: el que da el verde a la frecuencia central */
    float theta_target = aom_bragg_angle(AOM_LAMBDA_G_NM, AOM_F_CENTER_MHZ);

    /* ★ TRES PORTADORAS — una por canal. f ∝ 1/λ.
     *
     * Con f_G = 100 MHz:
     *      f_R = 100 · (550/680) =  80.9 MHz
     *      f_B = 100 · (550/440) = 125.0 MHz
     *
     * Los tres colores se difractan al MISMO ángulo. El AOM hace color. */
    o->f_r_mhz = aom_freq_for_lambda(AOM_LAMBDA_R_NM, theta_target);
    o->f_g_mhz = aom_freq_for_lambda(AOM_LAMBDA_G_NM, theta_target);
    o->f_b_mhz = aom_freq_for_lambda(AOM_LAMBDA_B_NM, theta_target);

    o->theta_r = aom_bragg_angle(AOM_LAMBDA_R_NM, o->f_r_mhz);
    o->theta_g = aom_bragg_angle(AOM_LAMBDA_G_NM, o->f_g_mhz);
    o->theta_b = aom_bragg_angle(AOM_LAMBDA_B_NM, o->f_b_mhz);

    o->rf_power_w = rf_power_w;
    o->eta_r = aom_diffraction_efficiency(AOM_LAMBDA_R_NM, rf_power_w);
    o->eta_g = aom_diffraction_efficiency(AOM_LAMBDA_G_NM, rf_power_w);
    o->eta_b = aom_diffraction_efficiency(AOM_LAMBDA_B_NM, rf_power_w);

    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §2  INIT / FREE
 * ═══════════════════════════════════════════════════════════════════════════ */
int aom_init(AOMEngine *e, float rf_power_w)
{
    if (!e) return -1;
    rl_memset(e, 0, sizeof(*e));

    unsigned long bytes = (unsigned long)AOM_GRID_CELLS * sizeof(RigVoxel);
    e->grid = (RigVoxel*)rl_malloc(bytes);
    if (!e->grid) return -1;
    rl_memset(e->grid, 0, bytes);

    aom_optics_configure(&e->optics, rf_power_w > 0.0f ? rf_power_w : 1.2f);

    e->volume_mm   = AOM_VOLUME_MM;
    e->initialized = true;
    return 0;
}

void aom_free(AOMEngine *e)
{
    if (!e) return;
    if (e->grid) { rl_free(e->grid); e->grid = 0; }
    rl_memset(e, 0, sizeof(*e));
}

int aom_add_object(AOMEngine *e, AOMPoint *pts, uint32_t n,
                   float x, float y, float z)
{
    if (!e || !e->initialized || !pts) return -1;
    if (e->object_count >= AOM_MAX_OBJECTS) return -1;

    AOMObject *o = &e->objects[e->object_count];
    o->id      = e->object_count;
    o->points  = pts;
    o->count   = n;
    o->pos[0]=x; o->pos[1]=y; o->pos[2]=z;
    o->rot[0]=o->rot[1]=o->rot[2]=0.0f;
    o->scale   = 1.0f;
    o->visible = true;

    return (int)(e->object_count++);
}

int aom_set_transform(AOMEngine *e, int id, float x, float y, float z,
                      float rx, float ry, float rz, float s)
{
    if (!e || id < 0 || id >= (int)e->object_count) return -1;
    AOMObject *o = &e->objects[id];
    o->pos[0]=x; o->pos[1]=y; o->pos[2]=z;
    o->rot[0]=rx; o->rot[1]=ry; o->rot[2]=rz;
    o->scale = s;
    return 0;                       /* ★ ahora SÍ devuelve un valor */
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §3  ★ VOXELIZACIÓN CON SPLATTING TRILINEAL ★
 *
 * Antes:  gx = (uint32_t)(wx * (RES-1))   ← nearest neighbor
 *         Los vóxeles SALTABAN al rotar. Parpadeo, aliasing, hervor.
 *
 * Ahora:  cada punto reparte su energía entre los 8 vóxeles vecinos,
 *         ponderada por distancia. La rotación es continua y suave.
 * ═══════════════════════════════════════════════════════════════════════════ */
static void euler_mat(float rx, float ry, float rz, float m[9])
{
    float cx=rl_cosf(rx), sx=rl_sinf(rx);
    float cy=rl_cosf(ry), sy=rl_sinf(ry);
    float cz=rl_cosf(rz), sz=rl_sinf(rz);
    m[0]= cy*cz;           m[1]=-cy*sz;           m[2]= sy;
    m[3]= sx*sy*cz+cx*sz;  m[4]=-sx*sy*sz+cx*cz;  m[5]=-sx*cy;
    m[6]=-cx*sy*cz+sx*sz;  m[7]= cx*sy*sz+sx*cz;  m[8]= cx*cy;
}

/* ★ Modulación por canal — con la FÍSICA, no con un if.
 * La eficiencia de difracción de cada canal a su propia frecuencia. */
static void aom_channel_gain(const AOMOptics *o, float gain[3])
{
    /* Normalizamos por el verde: es el canal de referencia (frecuencia
     * central). Los otros dos se compensan para que el blanco sea blanco. */
    float ref = ao_max(o->eta_g, 1e-4f);
    gain[0] = ao_clamp(o->eta_r / ref, 0.0f, 1.6f);
    gain[1] = 1.0f;
    gain[2] = ao_clamp(o->eta_b / ref, 0.0f, 1.6f);
}

int aom_voxelize(AOMEngine *e)
{
    if (!e || !e->initialized || !e->grid) return -1;

    rl_memset(e->grid, 0, (unsigned long)AOM_GRID_CELLS * sizeof(RigVoxel));

    float chan[3];
    aom_channel_gain(&e->optics, chan);

    const int   R  = (int)AOM_GRID_RES;
    const float Rf = (float)(AOM_GRID_RES - 1u);

    for (uint32_t oi = 0; oi < e->object_count; oi++) {
        AOMObject *o = &e->objects[oi];
        if (!o->visible || !o->points) continue;

        float m[9];
        euler_mat(o->rot[0], o->rot[1], o->rot[2], m);

        for (uint32_t pi = 0; pi < o->count; pi++) {
            AOMPoint *p = &o->points[pi];

            float lx = p->x * o->scale;
            float ly = p->y * o->scale;
            float lz = p->z * o->scale;

            float wx = m[0]*lx + m[1]*ly + m[2]*lz + o->pos[0];
            float wy = m[3]*lx + m[4]*ly + m[5]*lz + o->pos[1];
            float wz = m[6]*lx + m[7]*ly + m[8]*lz + o->pos[2];

            if (wx < 0.0f || wx > 1.0f ||
                wy < 0.0f || wy > 1.0f ||
                wz < 0.0f || wz > 1.0f) continue;

            /* ★ Coordenada continua en la rejilla */
            float fx = wx * Rf;
            float fy = wy * Rf;
            float fz = wz * Rf;

            int   x0 = (int)fx, y0 = (int)fy, z0 = (int)fz;
            float tx = fx - (float)x0;
            float ty = fy - (float)y0;
            float tz = fz - (float)z0;

            /* ★ SPLATTING: 8 vecinos, pesos trilineales */
            for (int dz = 0; dz < 2; dz++) {
                int zz = ao_clampi(z0 + dz, 0, R - 1);
                float wz2 = dz ? tz : (1.0f - tz);
                for (int dy = 0; dy < 2; dy++) {
                    int yy = ao_clampi(y0 + dy, 0, R - 1);
                    float wy2 = dy ? ty : (1.0f - ty);
                    for (int dx = 0; dx < 2; dx++) {
                        int xx = ao_clampi(x0 + dx, 0, R - 1);
                        float wx2 = dx ? tx : (1.0f - tx);

                        float w = wx2 * wy2 * wz2;
                        if (w < 1e-4f) continue;

                        uint32_t idx = (uint32_t)zz * AOM_GRID_RES * AOM_GRID_RES
                                     + (uint32_t)yy * AOM_GRID_RES
                                     + (uint32_t)xx;
                        RigVoxel *v = &e->grid[idx];

                        /* ★ EL COLOR SE CONSERVA. Cada canal con su ganancia
                         * de difracción — la física del AOM, aplicada. */
                        int nr = (int)v->r + (int)((float)p->r * chan[0] * w);
                        int ng = (int)v->g + (int)((float)p->g * chan[1] * w);
                        int nb = (int)v->b + (int)((float)p->b * chan[2] * w);
                        int na = (int)v->density + (int)((float)p->a * w);

                        v->r = (uint8_t)(nr > 255 ? 255 : nr);
                        v->g = (uint8_t)(ng > 255 ? 255 : ng);
                        v->b = (uint8_t)(nb > 255 ? 255 : nb);
                        v->density = (uint8_t)(na > 255 ? 255 : na);

                        /* El material del punto dominante manda */
                        if ((float)p->a * w > 12.0f) v->material = p->material;
                    }
                }
            }
        }
    }
    e->frame++;
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §4  MUESTREO TRILINEAL — la consulta que el háptico necesita
 * ═══════════════════════════════════════════════════════════════════════════ */
int aom_sample(const AOMEngine *e, const float p[3], RigVoxel *out)
{
    if (!e || !e->grid || !p || !out) return -1;
    rl_memset(out, 0, sizeof(*out));

    if (p[0] < 0.0f || p[0] > 1.0f ||
        p[1] < 0.0f || p[1] > 1.0f ||
        p[2] < 0.0f || p[2] > 1.0f) return 0;      /* fuera: aire */

    const int   R  = (int)AOM_GRID_RES;
    const float Rf = (float)(AOM_GRID_RES - 1u);

    float fx = p[0] * Rf, fy = p[1] * Rf, fz = p[2] * Rf;
    int   x0 = (int)fx, y0 = (int)fy, z0 = (int)fz;
    float tx = fx - (float)x0, ty = fy - (float)y0, tz = fz - (float)z0;

    float ar=0, ag=0, ab=0, ad=0;
    uint8_t best_mat = 0;
    float   best_w   = -1.0f;

    for (int dz = 0; dz < 2; dz++) {
        int zz = ao_clampi(z0 + dz, 0, R - 1);
        float wz = dz ? tz : (1.0f - tz);
        for (int dy = 0; dy < 2; dy++) {
            int yy = ao_clampi(y0 + dy, 0, R - 1);
            float wy = dy ? ty : (1.0f - ty);
            for (int dx = 0; dx < 2; dx++) {
                int xx = ao_clampi(x0 + dx, 0, R - 1);
                float wx = dx ? tx : (1.0f - tx);

                float w = wx * wy * wz;
                uint32_t idx = (uint32_t)zz * AOM_GRID_RES * AOM_GRID_RES
                             + (uint32_t)yy * AOM_GRID_RES
                             + (uint32_t)xx;
                const RigVoxel *v = &e->grid[idx];

                ar += (float)v->r * w;
                ag += (float)v->g * w;
                ab += (float)v->b * w;
                ad += (float)v->density * w;

                float mw = w * (float)v->density;
                if (mw > best_w) { best_w = mw; best_mat = v->material; }
            }
        }
    }
    out->r = (uint8_t)ao_clamp(ar, 0.0f, 255.0f);
    out->g = (uint8_t)ao_clamp(ag, 0.0f, 255.0f);
    out->b = (uint8_t)ao_clamp(ab, 0.0f, 255.0f);
    out->density  = (uint8_t)ao_clamp(ad, 0.0f, 255.0f);
    out->material = best_mat;
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §5  ★★★ RAY-MARCH — EMISIÓN-ABSORCIÓN ★★★
 *
 * Lo que había antes:
 *
 *      for(gz) { if(b > maxb) maxb = b; }        ← Maximum Intensity Projection
 *
 * Eso es la técnica de los TAC. Coge el vóxel más brillante de toda la
 * columna Z. Sin oclusión (lo de atrás se ve a través de lo de delante),
 * sin profundidad, sin paralaje. Una RADIOGRAFÍA, no un holograma.
 *
 * Lo que hay ahora — compositing front-to-back:
 *
 *      C      += (1 − α) · c_i · α_i
 *      α      += (1 − α) · α_i
 *
 * Con esto SÍ hay volumen: lo de delante tapa lo de detrás, la profundidad
 * se lee, y al mover el ojo el paralaje es correcto.
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Intersección rayo-caja unitaria [0,1]³ (slab test) */
static bool box_hit(const float ro[3], const float rd[3],
                    float *t0_out, float *t1_out)
{
    float t0 = -1e30f, t1 = 1e30f;
    for (int i = 0; i < 3; i++) {
        if (rl_fabsf(rd[i]) < 1e-8f) {
            if (ro[i] < 0.0f || ro[i] > 1.0f) return false;
            continue;
        }
        float inv = 1.0f / rd[i];
        float ta  = (0.0f - ro[i]) * inv;
        float tb  = (1.0f - ro[i]) * inv;
        if (ta > tb) { float s = ta; ta = tb; tb = s; }
        if (ta > t0) t0 = ta;
        if (tb < t1) t1 = tb;
        if (t0 > t1) return false;
    }
    if (t1 < 0.0f) return false;
    if (t0 < 0.0f) t0 = 0.0f;
    *t0_out = t0; *t1_out = t1;
    return true;
}

int aom_raymarch(const AOMEngine *e, const float ro[3], const float rd[3],
                 uint32_t steps, float out[4])
{
    if (!e || !e->grid || !ro || !rd || !out) return -1;

    out[0] = out[1] = out[2] = out[3] = 0.0f;

    float t0, t1;
    if (!box_hit(ro, rd, &t0, &t1)) return 0;

    if (steps == 0 || steps > AOM_MAX_STEPS) steps = AOM_MAX_STEPS;
    float dt = (t1 - t0) / (float)steps;
    if (dt <= 1e-6f) return 0;

    /* Corrección de opacidad por longitud de paso: si el paso es más largo,
     * el rayo atraviesa más medio y absorbe más. Sin esto, cambiar el número
     * de pasos cambiaría el brillo. */
    float step_mm  = dt * e->volume_mm;
    float opacity_k = step_mm / (e->volume_mm / (float)AOM_GRID_RES);

    float acc[3] = {0,0,0};
    float alpha  = 0.0f;
    float t = t0 + dt * 0.5f;

    for (uint32_t i = 0; i < steps; i++) {
        if (alpha > 0.995f) break;           /* saturado: early-out */

        float p[3] = { ro[0] + rd[0]*t,
                       ro[1] + rd[1]*t,
                       ro[2] + rd[2]*t };

        RigVoxel v;
        aom_sample(e, p, &v);

        float a = (float)v.density / 255.0f;
        if (a > 1e-3f) {
            a = ao_clamp(a * opacity_k, 0.0f, 1.0f);

            float c[3] = { (float)v.r / 255.0f,
                           (float)v.g / 255.0f,
                           (float)v.b / 255.0f };

            /* ★ FRONT-TO-BACK */
            float w = (1.0f - alpha) * a;
            acc[0] += c[0] * w;
            acc[1] += c[1] * w;
            acc[2] += c[2] * w;
            alpha  += w;
        }
        t += dt;
    }

    out[0] = acc[0];
    out[1] = acc[1];
    out[2] = acc[2];
    out[3] = ao_clamp(alpha, 0.0f, 1.0f);
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §6  ★★★ EL SONDEO — DONDE EL HOLOGRAMA SE VUELVE TÁCTIL ★★★
 *
 * Esta es la función que convierte
 *
 *      "el dedo tocó la pantalla en (x, y)"
 *
 * en
 *
 *      "el dedo tocó SEDA, en este punto, con esta normal, a esta distancia"
 *
 * El rayo NO sale perpendicular a la pantalla. SALE DEL OJO. Por eso sientes
 * que metes el dedo DENTRO del volumen, y no que tocas un cristal.
 *
 * La normal sale del gradiente de densidad — es la superficie implícita del
 * volumen. Y con la normal, el háptico sabe si estás rozando de plano o de
 * canto, y modula la fricción.
 * ═══════════════════════════════════════════════════════════════════════════ */
int aom_probe(const AOMEngine *e, const float ro[3], const float rd[3],
              float thr, AOMHit *out)
{
    if (!e || !e->grid || !ro || !rd || !out) return -1;
    rl_memset(out, 0, sizeof(*out));

    float t0, t1;
    if (!box_hit(ro, rd, &t0, &t1)) return 0;

    if (thr <= 0.0f) thr = 0.06f;

    const uint32_t STEPS = 96u;
    float dt = (t1 - t0) / (float)STEPS;
    if (dt <= 1e-6f) return 0;

    float t = t0;
    RigVoxel v;

    for (uint32_t i = 0; i < STEPS; i++) {
        float p[3] = { ro[0]+rd[0]*t, ro[1]+rd[1]*t, ro[2]+rd[2]*t };
        aom_sample(e, p, &v);

        if ((float)v.density / 255.0f > thr) {

            /* ── Refinamiento por bisección: 4 iteraciones bastan para
             *    clavar la superficie al nivel del vóxel ── */
            float ta = t - dt, tb = t;
            for (int k = 0; k < 4; k++) {
                float tm = (ta + tb) * 0.5f;
                float pm[3] = { ro[0]+rd[0]*tm, ro[1]+rd[1]*tm, ro[2]+rd[2]*tm };
                RigVoxel vm;
                aom_sample(e, pm, &vm);
                if ((float)vm.density / 255.0f > thr) tb = tm;
                else                                   ta = tm;
            }
            t = tb;

            float ph[3] = { ro[0]+rd[0]*t, ro[1]+rd[1]*t, ro[2]+rd[2]*t };
            aom_sample(e, ph, &out->voxel);

            /* ── ★ NORMAL = −∇(densidad).
             * La superficie implícita del volumen. Con esto el háptico sabe
             * si rozas de plano (fricción alta) o de canto (fricción baja). */
            float h = 1.0f / (float)AOM_GRID_RES;
            RigVoxel gx0, gx1, gy0, gy1, gz0, gz1;
            float q[3];

            q[0]=ph[0]-h; q[1]=ph[1];   q[2]=ph[2];   aom_sample(e,q,&gx0);
            q[0]=ph[0]+h;                             aom_sample(e,q,&gx1);
            q[0]=ph[0];   q[1]=ph[1]-h;               aom_sample(e,q,&gy0);
                          q[1]=ph[1]+h;               aom_sample(e,q,&gy1);
                          q[1]=ph[1];   q[2]=ph[2]-h; aom_sample(e,q,&gz0);
                                        q[2]=ph[2]+h; aom_sample(e,q,&gz1);

            float nx = (float)gx0.density - (float)gx1.density;
            float ny = (float)gy0.density - (float)gy1.density;
            float nz = (float)gz0.density - (float)gz1.density;
            float nl = rl_sqrtf(nx*nx + ny*ny + nz*nz);

            if (nl > 1e-4f) {
                out->normal[0] = nx / nl;
                out->normal[1] = ny / nl;
                out->normal[2] = nz / nl;
            } else {
                out->normal[0] = -rd[0];
                out->normal[1] = -rd[1];
                out->normal[2] = -rd[2];
            }

            out->pos[0] = ph[0];
            out->pos[1] = ph[1];
            out->pos[2] = ph[2];
            out->t      = t;
            out->hit    = true;
            return 0;
        }
        t += dt;
    }
    return 0;      /* aire */
}
