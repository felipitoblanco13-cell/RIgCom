/* ═══════════════════════════════════════════════════════════════════════════
 * rig_headtrack.c — LA VENTANA · RIGCOM MASTER · BLOQUE 4
 *
 * De una cámara frontal y una constante biológica (IPD = 63 mm) sale la
 * posición exacta de tu ojo en el espacio. Y de ahí, un frustum que hace
 * que el objeto se quede quieto en el mundo mientras tú te mueves.
 *
 * Sin ToF. Sin estéreo. Sin ARCore. Sin MediaPipe. Sin nada.
 *
 * C11 · cero libc · φ
 * ═══════════════════════════════════════════════════════════════════════════ */

#include "rig_headtrack.h"

extern float rl_sqrtf(float);
extern float rl_fabsf(float);
extern float rl_tanf(float);
extern float rl_expf(float);
extern void *rl_memset(void *, int, unsigned long);

#define HT_PI      3.14159265358979324f
#define HT_DEG2RAD 0.01745329251994330f

static float ht_clamp(float x, float a, float b){ return x<a?a:(x>b?b:x); }
static float ht_max(float a, float b){ return a>b?a:b; }


/* ═══════════════════════════════════════════════════════════════════════════
 * §1  FILTRO ONE-EURO
 *
 * El truco: el corte del paso-bajo es FUNCIÓN DE LA VELOCIDAD.
 *
 *      cutoff = min_cutoff + β · |ẋ|
 *
 * Quieto  → |ẋ| ≈ 0 → cutoff bajo  → filtra fuerte → CERO jitter
 * Rápido  → |ẋ| alto → cutoff alto → filtra poco   → CERO lag
 *
 * Sin esto, o el objeto tiembla o el objeto "nada" detrás de tu cabeza.
 * Con esto, se queda clavado en el espacio.
 * ═══════════════════════════════════════════════════════════════════════════ */

static float ht_alpha(float cutoff, float dt)
{
    /* α = 1 / (1 + τ/Te),  con τ = 1/(2π·fc)  y  Te = dt */
    float tau = 1.0f / (2.0f * HT_PI * ht_max(cutoff, 1e-4f));
    return 1.0f / (1.0f + tau / ht_max(dt, 1e-5f));
}

void rig_oneeuro_init(RigOneEuro *f, float min_cutoff, float beta, float d_cutoff)
{
    if (!f) return;
    rl_memset(f, 0, sizeof(*f));
    f->min_cutoff = (min_cutoff > 0.0f) ? min_cutoff : 1.0f;
    f->beta       = (beta       > 0.0f) ? beta       : 0.008f;
    f->d_cutoff   = (d_cutoff   > 0.0f) ? d_cutoff   : 1.0f;
    f->init       = false;
}

float rig_oneeuro_step(RigOneEuro *f, float x, float dt)
{
    if (!f) return x;

    if (!f->init) {
        f->x_hat  = x;
        f->dx_hat = 0.0f;
        f->init   = true;
        return x;
    }
    if (dt <= 1e-6f) return f->x_hat;

    /* 1. Derivada, filtrada con corte fijo */
    float dx = (x - f->x_hat) / dt;
    float ad = ht_alpha(f->d_cutoff, dt);
    f->dx_hat = ad * dx + (1.0f - ad) * f->dx_hat;

    /* 2. ★ El corte ADAPTATIVO */
    float cutoff = f->min_cutoff + f->beta * rl_fabsf(f->dx_hat);

    /* 3. Filtrar la señal con ese corte */
    float a = ht_alpha(cutoff, dt);
    f->x_hat = a * x + (1.0f - a) * f->x_hat;

    return f->x_hat;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §2  CALIBRACIÓN — Honor 400
 *
 * Pantalla 6.55", 1264 × 2736 px.
 *   aspecto  = 1264/2736 = 0.4620
 *   diagonal = 6.55" = 166.4 mm
 *   h = √(166.4² / (1 + 0.4620²)) = 151.1 mm
 *   w = 0.4620 × 151.1            =  69.8 mm
 *
 * El punch-hole está centrado en el borde superior, a ~72 mm del centro.
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_headtrack_default_cal(RigScreenCal *c)
{
    if (!c) return -1;
    rl_memset(c, 0, sizeof(*c));

    c->screen_w_mm = 69.8f;
    c->screen_h_mm = 151.1f;

    c->cam_offset_x_mm =  0.0f;
    c->cam_offset_y_mm = 72.0f;    /* ★ la cámara NO está en el centro */
    c->cam_offset_z_mm =  0.0f;

    c->cam_fov_h_deg = 78.0f;
    c->cam_res_w     = 320.0f;
    c->cam_res_h     = 240.0f;
    c->cam_mirrored  = true;

    c->ipd_mm = 63.0f;             /* IPD media adulta. σ ≈ 3 mm. */
    return 0;
}

int rig_headtrack_init(RigObserver *o)
{
    if (!o) return -1;
    rl_memset(o, 0, sizeof(*o));

    /* β pequeño en X,Y (movimientos laterales suaves).
     * β mayor en Z: la profundidad es la medida más ruidosa (depende de
     * IPD_px, que es una diferencia de dos puntos ruidosos), así que
     * necesita filtrar más fuerte en reposo. */
    rig_oneeuro_init(&o->fx, 1.2f, 0.010f, 1.0f);
    rig_oneeuro_init(&o->fy, 1.2f, 0.010f, 1.0f);
    rig_oneeuro_init(&o->fz, 0.6f, 0.006f, 1.0f);

    /* Posición nominal: de frente, a 400 mm */
    o->eye_mm[0] = 0.0f;
    o->eye_mm[1] = 0.0f;
    o->eye_mm[2] = 400.0f;
    o->distance_mm = 400.0f;
    o->valid = false;
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §3  ★ LA TRIANGULACIÓN ★
 *
 *      z = (IPD_real · focal_px) / IPD_píxeles
 *
 * Triángulos semejantes. La IPD real es una constante biológica
 * (63 ± 3 mm). La IPD en píxeles la mide la cámara. La focal sale del FOV.
 *
 * Ese es todo el sensor de profundidad.
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_headtrack_update(RigObserver *o, const RigScreenCal *cal,
                         const float eyeL_px[2], const float eyeR_px[2],
                         float dt)
{
    if (!o || !cal || !eyeL_px || !eyeR_px) return -1;

    /* ── Focal en píxeles, desde el FOV ── */
    float half_fov = cal->cam_fov_h_deg * 0.5f * HT_DEG2RAD;
    float focal_px = (cal->cam_res_w * 0.5f) / ht_max(rl_tanf(half_fov), 1e-4f);

    /* ── IPD medida en píxeles ── */
    float dx = eyeR_px[0] - eyeL_px[0];
    float dy = eyeR_px[1] - eyeL_px[1];
    float ipd_px = rl_sqrtf(dx*dx + dy*dy);

    if (ipd_px < 4.0f) {                 /* demasiado lejos o mala detección */
        return rig_headtrack_lost(o, cal, dt);
    }
    o->ipd_px = ipd_px;

    /* ── ★ PROFUNDIDAD ── */
    float z_cam = (cal->ipd_mm * focal_px) / ipd_px;
    z_cam = ht_clamp(z_cam, 120.0f, 1200.0f);   /* 12 cm .. 1.2 m           */

    /* ── Centro entre los ojos, en píxeles ── */
    float cx_px = (eyeL_px[0] + eyeR_px[0]) * 0.5f;
    float cy_px = (eyeL_px[1] + eyeR_px[1]) * 0.5f;

    /* ── De píxeles a milímetros, en el sistema de la CÁMARA ──
     * Proyección pinhole invertida: X = (u − cx) · Z / f */
    float u = cx_px - cal->cam_res_w * 0.5f;
    float v = cy_px - cal->cam_res_h * 0.5f;

    float x_cam = (u * z_cam) / focal_px;
    float y_cam = (v * z_cam) / focal_px;

    /* La imagen tiene +y hacia ABAJO; el mundo tiene +y hacia ARRIBA */
    y_cam = -y_cam;

    /* La cámara frontal suele venir espejada */
    if (cal->cam_mirrored) x_cam = -x_cam;

    /* ── ★ De la CÁMARA al CENTRO DE LA PANTALLA ──
     * La cámara está arriba. Si no restas su offset, el objeto se
     * desplaza hacia abajo cuando acercas la cara. */
    float ex = x_cam + cal->cam_offset_x_mm;
    float ey = y_cam + cal->cam_offset_y_mm;
    float ez = z_cam + cal->cam_offset_z_mm;

    /* ── Filtro One-Euro por eje ── */
    o->eye_mm[0] = rig_oneeuro_step(&o->fx, ex, dt);
    o->eye_mm[1] = rig_oneeuro_step(&o->fy, ey, dt);
    o->eye_mm[2] = rig_oneeuro_step(&o->fz, ez, dt);

    /* Nunca dejar que el ojo cruce el plano de la pantalla */
    if (o->eye_mm[2] < 80.0f) o->eye_mm[2] = 80.0f;

    o->distance_mm = o->eye_mm[2];
    o->frames_lost = 0u;
    o->valid       = true;

    /* Confianza: IPD grande = cara cerca = medida fiable */
    o->confidence = ht_clamp((ipd_px - 6.0f) / 40.0f, 0.15f, 1.0f);
    return 0;
}

int rig_headtrack_lost(RigObserver *o, const RigScreenCal *cal, float dt)
{
    if (!o || !cal) return -1;
    o->frames_lost++;

    /* No dar un salto: decaer suavemente hacia la posición nominal.
     * Un corte brusco del tracking es peor que un poco de deriva. */
    if (o->frames_lost > 12u) {
        float k = ht_clamp(dt * 1.8f, 0.0f, 1.0f);
        o->eye_mm[0] += (0.0f   - o->eye_mm[0]) * k;
        o->eye_mm[1] += (0.0f   - o->eye_mm[1]) * k;
        o->eye_mm[2] += (400.0f - o->eye_mm[2]) * k;
        o->distance_mm = o->eye_mm[2];
        o->valid = false;
    }
    o->confidence *= 0.94f;
    return 0;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §4  ★★★  EL FRUSTUM ASIMÉTRICO  ★★★
 *
 * Aquí es donde la pantalla se convierte en ventana.
 *
 * Un frustum normal es simétrico: asume que miras de frente, desde el eje,
 * a distancia fija. Por eso todo el 3D en un móvil parece una IMAGEN de un
 * objeto, y no un objeto.
 *
 * Este frustum lo definen el OJO y las CUATRO ESQUINAS FÍSICAS del cristal.
 * Cuando el ojo se desplaza, l y r dejan de ser simétricos, y el volumen
 * de visión se INCLINA. El objeto, que está fijo en coordenadas de mundo,
 * se ve desde otro ángulo — igual que un objeto real detrás de una ventana.
 *
 * Kooima (2009), "Generalized Perspective Projection".
 * ═══════════════════════════════════════════════════════════════════════════ */
int rig_offaxis_projection(const RigScreenCal *cal, const float eye_mm[3],
                           float near_mm, float far_mm, float out[16])
{
    if (!cal || !eye_mm || !out) return -1;

    float hw = cal->screen_w_mm * 0.5f;
    float hh = cal->screen_h_mm * 0.5f;

    float ex = eye_mm[0];
    float ey = eye_mm[1];
    float ez = ht_max(eye_mm[2], 1.0f);     /* distancia ojo→pantalla */

    /* ★ Las cuatro esquinas de la ventana, proyectadas al plano near.
     *
     *   Si el ojo está a la derecha (ex > 0), la esquina izquierda de la
     *   pantalla queda MÁS lejos angularmente ⇒ |l| crece, r se encoge.
     *   El frustum se inclina. Eso es el paralaje. */
    float n_over_z = near_mm / ez;

    float l = (-hw - ex) * n_over_z;
    float r = ( hw - ex) * n_over_z;
    float b = (-hh - ey) * n_over_z;
    float t = ( hh - ey) * n_over_z;

    float rl = r - l;
    float tb = t - b;
    float fn = far_mm - near_mm;

    if (rl < 1e-6f || tb < 1e-6f || fn < 1e-6f) return -1;

    /* Column-major (OpenGL / WebGL) */
    out[0]  = 2.0f * near_mm / rl;
    out[1]  = 0.0f;
    out[2]  = 0.0f;
    out[3]  = 0.0f;

    out[4]  = 0.0f;
    out[5]  = 2.0f * near_mm / tb;
    out[6]  = 0.0f;
    out[7]  = 0.0f;

    out[8]  = (r + l) / rl;                 /* ★ el desplazamiento en X */
    out[9]  = (t + b) / tb;                 /* ★ el desplazamiento en Y */
    out[10] = -(far_mm + near_mm) / fn;
    out[11] = -1.0f;

    out[12] = 0.0f;
    out[13] = 0.0f;
    out[14] = -2.0f * far_mm * near_mm / fn;
    out[15] = 0.0f;

    return 0;
}

/* La cámara NO rota. La pantalla está fija en el espacio del mundo;
 * es el OJO el que se mueve. Así que la vista es una simple traslación. */
int rig_offaxis_view(const float eye_mm[3], float out[16])
{
    if (!eye_mm || !out) return -1;
    rl_memset(out, 0, sizeof(float) * 16);
    out[0] = 1.0f; out[5] = 1.0f; out[10] = 1.0f; out[15] = 1.0f;
    out[12] = -eye_mm[0];
    out[13] = -eye_mm[1];
    out[14] = -eye_mm[2];
    return 0;
}

int rig_offaxis_model_scale(float s, float out[16])
{
    if (!out) return -1;
    rl_memset(out, 0, sizeof(float) * 16);
    out[0]  = s;
    out[5]  = s;
    out[10] = s;
    out[15] = 1.0f;
    return 0;
}
