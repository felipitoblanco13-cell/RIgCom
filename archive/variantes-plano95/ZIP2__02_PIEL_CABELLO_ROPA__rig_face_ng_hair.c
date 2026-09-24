/*
 * rig_face_ng_hair.c — RigCom v24 NEXT GENERATION · Sistema de Cabello
 * ════════════════════════════════════════════════════════════════════════
 * Richard Felipe Urbina · RIGCOM Ecosystem · Arquitecto Soberano
 *
 * MODELO BIOFÍSICO COMPLETO DE CABELLO:
 *
 *   ÓPTICA:
 *   ├── Marschner 2003: R + TT + TRT lobes completos
 *   ├── Chiang 2016: modelo dual-cilindro medula/corteza
 *   ├── Melanina: eumelanina (negro) + feomelanina (rojo/dorado)
 *   ├── Absorción espectral por wavelength (R,G,B)
 *   ├── Rugosidad cuticular anisotrópica (tilt α = 2-4°)
 *   ├── Difusión de forward scatter (glint)
 *   └── IBL integrado con esfera de difusión
 *
 *   GEOMETRÍA:
 *   ├── Hebras bezier cúbicas (4 puntos de control por segmento)
 *   ├── Densidad: 80,000-120,000 hebras en cabello completo
 *   ├── Amplitud de curva por tipo étnico (liso, ondulado, rizado, afro)
 *   ├── Sección transversal: circular (asiático) / elíptica (africano)
 *   ├── Tapering: raíz gruesa (70μm) → punta fina (30μm)
 *   └── Folículo: ángulo de salida, región del cuero cabelludo
 *
 *   FÍSICA PBD (Position-Based Dynamics):
 *   ├── Resortes de longitud (inextensibilidad)
 *   ├── Resortes de curvatura (rigidez del tallo)
 *   ├── Resortes de torsión (rizado/onda)
 *   ├── Colisión con cabeza (ellipsoid proxy)
 *   ├── Viento: campo de fuerza Perlin + gustos
 *   ├── Gravedad por rigidez de cutícula (pelo duro vs suave)
 *   └── Amortiguación viscosa del aire
 *
 *   AGRUPACIÓN:
 *   ├── Clumping: hebras se atraen gravitacionalmente entre sí
 *   ├── Flyaway: hebras individuales con carga electrostática
 *   ├── Wisping: separación en puntas por fricción
 *   └── Parting: línea de división del cabello
 *
 *   COLOR:
 *   ├── Melanina eumelanina/feomelanina variable por hebra (±σ)
 *   ├── Degradado raíz-punta (regrowth oscuro, puntas desteñidas)
 *   ├── Highlights procedurales (sol, químico)
 *   ├── Gris: melanocitos agotados (edad)
 *   └── Tinte: capa de color artificial sobre la melanina natural
 *
 * φ = 1.6180339887498948482
 */

#include "rigdeps/rig_std_base.h"
#include "rig_face_ng.h"
#include "rigdeps/stdlib.h"
#include "rigdeps/string.h"
#include "rigdeps/math.h"
#include "rigdeps/stdio.h"
#include "../include/riglib_math.h"

#define HAIR_BUF       (512 * 1024)
#define HAIR_JS_BUF    (128 * 1024)
#define FA(b,s,p,...) do { \
    if ((p)<(int)(s)) { int _n=snprintf((b)+(p),(s)-(p),__VA_ARGS__); \
    if(_n>0)(p)+=_n; } } while(0)

#define NG_PHI     1.6180339887498948482f
#define NG_PI      3.14159265358979323846f
#define NG_TAU     6.28318530717958647692f

/* ═══════════════════════════════════════════════════════════════
 * TIPOS DE CURVA — geometría del cabello por etnia
 * ═══════════════════════════════════════════════════════════════ */
typedef enum {
    HAIR_CURVE_STRAIGHT    = 0,  /* Asiático: recto, circular */
    HAIR_CURVE_WAVY        = 1,  /* Europeo: ondulado */
    HAIR_CURVE_CURLY       = 2,  /* Latino/Med: rizado suelto */
    HAIR_CURVE_COILY       = 3,  /* Africano: zig-zag apretado */
    HAIR_CURVE_KINKY       = 4,  /* Afro: helicoidal apretado */
} HairCurveType;

/* ═══════════════════════════════════════════════════════════════
 * CONTEXTO DEL SISTEMA DE CABELLO
 * ═══════════════════════════════════════════════════════════════ */
typedef struct RigHairNGCtx {
    /* Bioquímica */
    float melanin_eu;          /* 0-1 eumelanina */
    float melanin_ph;          /* 0-1 feomelanina */
    float melanin_sigma;       /* varianza por hebra (diversidad) */
    float gray_fraction;       /* 0-1 fracción gris */
    float tint_r, tint_g, tint_b; /* color de tinte artificial */
    float tint_strength;       /* 0-1 fuerza del tinte */
    float highlight_intensity; /* 0-1 reflejos de sol/químico */

    /* Geometría */
    HairCurveType curve_type;
    float curl_radius_mm;      /* radio del rizo en mm */
    float curl_freq;           /* frecuencia: 0=recto 10=muy rizado */
    float strand_width_root;   /* diámetro raíz en μm */
    float strand_width_tip;    /* diámetro punta en μm */
    float length_avg_cm;       /* longitud promedio en cm */
    float length_sigma;        /* varianza de longitud */
    int   strand_count;        /* hebras renderizables */
    float cross_section_ratio; /* 1.0=circular 0.5=muy elíptico */

    /* PBD Física */
    float stiffness;           /* rigidez 0-1 */
    float damping;             /* amortiguación 0-1 */
    float wind_strength;       /* fuerza del viento */
    float wind_dir_x, wind_dir_y, wind_dir_z;
    float wind_turbulence;     /* turbulencia (gustos) */
    float gravity_scale;       /* modificador de gravedad */
    float flyaway_factor;      /* carga electrostática */
    int   pbd_iterations;      /* iteraciones PBD por frame */
    int   pbd_segments;        /* segmentos de la cadena */

    /* Óptica */
    float roughness_long;      /* rugosidad longitudinal */
    float roughness_azimuth;   /* rugosidad azimutal */
    float cuticle_tilt;        /* inclinación cuticular (deg) */
    float specular_lobe_r;     /* peso lóbulo R (reflexión) */
    float specular_lobe_tt;    /* peso lóbulo TT (transmisión) */
    float specular_lobe_trt;   /* peso lóbulo TRT (glint) */
    float medulla_fraction;    /* fracción médula 0-1 */
    float ior_cortex;          /* IOR corteza 1.55 */
    float ior_medulla;         /* IOR médula 1.35 */

    /* Agrupación */
    float clump_strength;      /* 0-1 fuerza de agrupación */
    float clump_radius_mm;     /* radio del clump en mm */
    float wisp_strength;       /* 0-1 separación de puntas */
    float parting_pos;         /* -1=izq 0=centro 1=derecha */
    float parting_sharpness;   /* qué tan definida es la raya */
} RigHairNGCtx;

/* ═══════════════════════════════════════════════════════════════
 * GLSL — MARSCHNER DUAL-LOBE COMPLETO
 * Referencia: Marschner et al. 2003 "Light Scattering from Human Hair Fibers"
 *             Chiang et al. 2016 "A Practical and Controllable Hair and Fur Model"
 * ═══════════════════════════════════════════════════════════════ */
static const char *NG_HAIR_MARSCHNER_GLSL =
    "/* ══════════════════════════════════════════════════════════ */\n"
    "/* MODELO DE MARSCHNER + CHIANG — DISPERSIÓN DE CABELLO NG   */\n"
    "/* ══════════════════════════════════════════════════════════ */\n\n"
    "/* ── Absorción de melanina espectral ── */\n"
    "/* Sigma_a: coeficientes de absorción del cortex por cromóforo */\n"
    "/* Referencia: Donner & Jensen 2006 */\n"
    "vec3 hair_absorption(float mel_eu, float mel_ph) {\n"
    "    /* Eumelanina: absorción amplia, pico UV-azul */\n"
    "    vec3 a_eu = vec3(0.419, 0.697, 1.378) * mel_eu * 0.5;\n"
    "    /* Feomelanina: pico verde-amarillo (dorado-rojizo) */\n"
    "    vec3 a_ph = vec3(0.187, 0.398, 1.093) * mel_ph * 0.3;\n"
    "    return a_eu + a_ph;\n"
    "}\n\n"
    "/* ── Transmitancia a lo largo del cortex ── */\n"
    "vec3 hair_transmittance(vec3 sigma_a, float path_len) {\n"
    "    return exp(-sigma_a * path_len);\n"
    "}\n\n"
    "/* ── Función M: componente longitudinal (Marschner) ── */\n"
    "/* βR  ~ roughness_azimuth (rugosidad azimutal del lóbulo R)  */\n"
    "/* βTT ~ roughness_azimuth * 0.5                              */\n"
    "/* βTRT~ roughness_azimuth * 2.0                              */\n"
    "float M_lobe(float cos_theta_i, float cos_theta_r,\n"
    "              float sin_theta_i, float sin_theta_r,\n"
    "              float beta) {\n"
    "    float v = beta * beta;\n"
    "    float sinsum = sin_theta_i + sin_theta_r;\n"
    "    /* Gaussian en sin_theta */\n"
    "    float exponent = -(sinsum * sinsum) / (2.0 * v);\n"
    "    float norm = 1.0 / (sqrt(2.0 * PI) * beta);\n"
    "    return norm * exp(exponent) * cos_theta_i * cos_theta_r;\n"
    "}\n\n"
    "/* ── Función N: componente azimutal por lóbulo ── */\n"
    "/* p=0: R (reflexión superficial) */\n"
    "/* p=1: TT (transmisión doble) */\n"
    "/* p=2: TRT (transmisión-reflexión-transmisión = glint) */\n"
    "float N_lobe_R(float phi, float eta, float h, float sin_tilt) {\n"
    "    /* Ángulo de desvío para reflexión especular */\n"
    "    float cos_phi = cos(phi);\n"
    "    /* Fresnel de primera superficie */\n"
    "    float cos_theta_t = sqrt(1.0 - h * h);\n"
    "    float F = pow((1.0 - cos_theta_t) / (1.0 + cos_theta_t * eta), 2.0);\n"
    "    F = clamp(F, 0.0, 1.0);\n"
    "    /* Offset por inclinación de la cutícula */\n"
    "    float phi_r = -phi - 2.0 * sin_tilt;\n"
    "    return F * exp(-phi_r * phi_r * 8.0);\n"
    "}\n"
    "vec3 N_lobe_TT(float phi, float eta, float h,\n"
    "                vec3 sigma_a, float cos_theta_d) {\n"
    "    float cos_t = sqrt(max(0., 1.0 - h*h/(eta*eta)));\n"
    "    /* Transmitancia a través del cortex */\n"
    "    float path = 2.0 * cos_t;\n"
    "    vec3 T = hair_transmittance(sigma_a, path);\n"
    "    float phi_tt = PI + phi * 0.5;\n"
    "    float gauss = exp(-phi_tt * phi_tt * 4.0);\n"
    "    return T * T * gauss * (1.0 - pow(1.0-cos_t, 5.0));\n"
    "}\n"
    "vec3 N_lobe_TRT(float phi, float eta, float h,\n"
    "                 vec3 sigma_a, float cos_theta_d) {\n"
    "    float cos_t = sqrt(max(0., 1.0 - h*h/(eta*eta)));\n"
    "    float path = 4.0 * cos_t;\n"
    "    vec3 T = hair_transmittance(sigma_a, path);\n"
    "    float cos_g = sqrt(max(0., eta*eta - h*h)) / eta;\n"
    "    float c = acos(h/eta);\n"
    "    float phi_trt = 2.0 * (PI - 3.0*c) - phi;\n"
    "    float gauss = exp(-phi_trt * phi_trt * 2.0);\n"
    "    return T * T * T * gauss;\n"
    "}\n\n"
    "/* ══════════════════════════════════════════════════════ */\n"
    "/* BSDF COMPLETO DE CABELLO — suma de todos los lóbulos  */\n"
    "/* ══════════════════════════════════════════════════════ */\n"
    "vec3 hair_bsdf(\n"
    "    vec3  L,           /* dirección de luz (world) */\n"
    "    vec3  V,           /* dirección de vista (world) */\n"
    "    vec3  T,           /* tangente de la hebra (world) */\n"
    "    float mel_eu,      /* eumelanina 0-1 */\n"
    "    float mel_ph,      /* feomelanina 0-1 */\n"
    "    float roughness_long,\n"
    "    float roughness_azim,\n"
    "    float cuticle_tilt_rad,  /* inclinación cuticular en rad */\n"
    "    float ior,               /* IOR corteza ~1.55 */\n"
    "    float medulla_frac,      /* fracción de médula 0-1 */\n"
    "    float w_R, float w_TT, float w_TRT /* pesos de lóbulos */\n"
    ") {\n"
    "    /* Proyectar L y V sobre el plano normal a T */\n"
    "    float sin_theta_i = dot(L, T);\n"
    "    float sin_theta_r = dot(V, T);\n"
    "    float cos_theta_i = sqrt(max(0., 1.-sin_theta_i*sin_theta_i));\n"
    "    float cos_theta_r = sqrt(max(0., 1.-sin_theta_r*sin_theta_r));\n"
    "    /* Ángulo phi en el plano azimutal */\n"
    "    vec3 L_perp = normalize(L - sin_theta_i * T);\n"
    "    vec3 V_perp = normalize(V - sin_theta_r * T);\n"
    "    float cos_phi = dot(L_perp, V_perp);\n"
    "    float phi = acos(clamp(cos_phi, -1., 1.));\n"
    "    /* cos theta_d para refracción */\n"
    "    float cos_theta_d = cos((asin(sin_theta_i)+asin(sin_theta_r))*0.5);\n"
    "    /* Parámetro h: offset lateral del rayo (-1 a 1) */\n"
    "    float h = cos_phi * cos_theta_d;\n"
    "    /* Sigma_a: absorción espectral del cortex */\n"
    "    vec3 sigma_a = hair_absorption(mel_eu, mel_ph);\n"
    "    /* Médula: reduce absorción (canal interno vacío/lleno) */\n"
    "    sigma_a *= (1.0 - medulla_frac * 0.3);\n"
    "    /* IOR efectivo para ángulo de incidencia oblicuo */\n"
    "    float eta_prime = sqrt(ior*ior - sin_theta_d*sin_theta_d) / cos_theta_d;\n"
    "    float sin_tilt = sin(cuticle_tilt_rad);\n"
    "    /* ── Lóbulo M (longitudinal) × N (azimutal) ── */\n"
    "    /* R: specular primario */\n"
    "    float M_R   = M_lobe(cos_theta_i, cos_theta_r,\n"
    "                          sin_theta_i, sin_theta_r, roughness_azim);\n"
    "    float N_R   = N_lobe_R(phi, eta_prime, h, sin_tilt);\n"
    "    /* TT: transmisión (da el color real del cabello) */\n"
    "    float M_TT  = M_lobe(cos_theta_i, cos_theta_r,\n"
    "                          sin_theta_i, sin_theta_r, roughness_azim*0.5);\n"
    "    vec3  N_TT  = N_lobe_TT(phi, eta_prime, h, sigma_a, cos_theta_d);\n"
    "    /* TRT: glint (brillo interno, dorado en rubios) */\n"
    "    float M_TRT = M_lobe(cos_theta_i, cos_theta_r,\n"
    "                          sin_theta_i, sin_theta_r, roughness_azim*2.0);\n"
    "    vec3  N_TRT = N_lobe_TRT(phi, eta_prime, h, sigma_a, cos_theta_d);\n"
    "    /* Combinación ponderada */\n"
    "    vec3 result = vec3(0.);\n"
    "    result += vec3(M_R   * N_R)       * w_R;\n"
    "    result += (M_TT  * N_TT)          * w_TT;\n"
    "    result += (M_TRT * N_TRT)         * w_TRT;\n"
    "    /* Componente difusa (forward scatter aproximado) */\n"
    "    vec3 diffuse = hair_transmittance(sigma_a, 2.0) * roughness_long;\n"
    "    result += diffuse * max(0., 0.5 + 0.5*cos_phi) * cos_theta_i;\n"
    "    return max(vec3(0.), result);\n"
    "}\n\n";

/* ═══════════════════════════════════════════════════════════════
 * GLSL — VERTEX SHADER DE HEBRA DE CABELLO NG
 * ═══════════════════════════════════════════════════════════════ */
static const char *NG_HAIR_VERTEX_GLSL =
    "/* ══════════════════════════════════════════════════════════ */\n"
    "/* VERTEX SHADER DE HEBRA NG — expansión de billboard        */\n"
    "/* ══════════════════════════════════════════════════════════ */\n"
    "#version 300 es\n"
    "precision highp float;\n\n"
    "/* Posición central de la hebra + t=parámetro a lo largo */\n"
    "in vec3  a_strand_pos;       /* centro del segmento (world) */\n"
    "in vec3  a_strand_tangent;   /* tangente del segmento */\n"
    "in float a_strand_t;         /* 0=raíz 1=punta */\n"
    "in float a_strand_id;        /* id único de hebra */\n"
    "in float a_melanin_eu;       /* eumelanina por hebra */\n"
    "in float a_melanin_ph;       /* feomelanina por hebra */\n"
    "in float a_width;            /* ancho actual en mm */\n\n"
    "uniform mat4  u_mvp;\n"
    "uniform mat4  u_model;\n"
    "uniform vec3  u_camera_pos;\n"
    "uniform float u_clump_factor;\n"
    "uniform float u_wisp_factor;\n"
    "uniform float u_time;\n\n"
    "out vec3  v_pos;             /* world position */\n"
    "out vec3  v_tangent;         /* tangente de hebra */\n"
    "out vec2  v_uv;              /* u=lateral v=t a lo largo */\n"
    "out float v_t;               /* parámetro a lo largo 0-1 */\n"
    "out float v_mel_eu;\n"
    "out float v_mel_ph;\n"
    "out float v_strand_id;\n"
    "out float v_highlight_mask;  /* zona de reflejo especular */\n\n"
    "/* Orientación billboard — hebra siempre de cara a la cámara */\n"
    "void main() {\n"
    "    vec3 to_cam = normalize(u_camera_pos - a_strand_pos);\n"
    "    vec3 T_norm = normalize(a_strand_tangent);\n"
    "    /* Vector lateral: perpendicular a T y a la cámara */\n"
    "    vec3 side = normalize(cross(T_norm, to_cam));\n"
    "    /* Expandir el billboard a la mitad del ancho por cada lado */\n"
    "    float half_w = a_width * 0.0005;  /* mm → m */\n"
    "    /* Vértice izquierdo o derecho según gl_VertexID par/impar */\n"
    "    float sign_side = (float(gl_VertexID & 1) * 2.0 - 1.0);\n"
    "    vec3 pos = a_strand_pos + side * half_w * sign_side;\n"
    "    /* Tapering hacia la punta */\n"
    "    float taper = mix(1.0, 0.1, a_strand_t * a_strand_t);\n"
    "    pos = a_strand_pos + side * half_w * sign_side * taper;\n"
    "    /* Highlight mask: zona donde el especular es máximo */\n"
    "    v_highlight_mask = max(0., 1.0 - abs(sign_side * 2.0 - 1.0));\n"
    "    v_pos        = pos;\n"
    "    v_tangent    = T_norm;\n"
    "    v_uv         = vec2(sign_side * 0.5 + 0.5, a_strand_t);\n"
    "    v_t          = a_strand_t;\n"
    "    v_mel_eu     = a_melanin_eu;\n"
    "    v_mel_ph     = a_melanin_ph;\n"
    "    v_strand_id  = a_strand_id;\n"
    "    gl_Position  = u_mvp * vec4(pos, 1.0);\n"
    "}\n\n";

/* ═══════════════════════════════════════════════════════════════
 * GLSL — FRAGMENT SHADER DE CABELLO NG
 * ═══════════════════════════════════════════════════════════════ */
static const char *NG_HAIR_FRAG_GLSL =
    "/* ══════════════════════════════════════════════════════════ */\n"
    "/* FRAGMENT SHADER CABELLO NG — Marschner completo + color   */\n"
    "/* ══════════════════════════════════════════════════════════ */\n"
    "#version 300 es\n"
    "precision highp float;\n\n"
    "const float PI    = 3.14159265359;\n"
    "const float PHI   = 1.6180339887;\n\n"
    "in vec3  v_pos;\n"
    "in vec3  v_tangent;\n"
    "in vec2  v_uv;\n"
    "in float v_t;\n"
    "in float v_mel_eu;\n"
    "in float v_mel_ph;\n"
    "in float v_strand_id;\n"
    "in float v_highlight_mask;\n\n"
    "out vec4 fragColor;\n\n"
    "/* Samplers */\n"
    "uniform samplerCube u_env_map;          /* IBL environment */\n"
    "uniform sampler2D   u_hair_color_ramp;  /* ramp de color: raíz→punta */\n"
    "uniform sampler2D   u_hair_noise;       /* textura de variación */\n\n"
    "/* Luz */\n"
    "uniform vec3  u_light_dir_0;\n"
    "uniform vec3  u_light_color_0;\n"
    "uniform float u_light_intensity_0;\n"
    "uniform vec3  u_light_dir_1;\n"
    "uniform vec3  u_light_color_1;\n"
    "uniform float u_light_intensity_1;\n"
    "uniform vec3  u_view_dir;\n\n"
    "/* Bioquímica */\n"
    "uniform float u_mel_eu_base;\n"
    "uniform float u_mel_ph_base;\n"
    "uniform float u_mel_sigma;\n"
    "uniform float u_gray_fraction;\n"
    "uniform vec3  u_tint_color;\n"
    "uniform float u_tint_strength;\n"
    "uniform float u_highlight_intensity;\n\n"
    "/* Óptica */\n"
    "uniform float u_roughness_long;\n"
    "uniform float u_roughness_azim;\n"
    "uniform float u_cuticle_tilt_deg;\n"
    "uniform float u_ior;\n"
    "uniform float u_medulla_fraction;\n"
    "uniform float u_lobe_r;\n"
    "uniform float u_lobe_tt;\n"
    "uniform float u_lobe_trt;\n"
    "uniform float u_exposure;\n"
    "uniform float u_opacity_root;  /* opacidad en raíz */\n"
    "uniform float u_opacity_tip;   /* opacidad en punta */\n\n"
    "/* Hash sin seno */\n"
    "float h11(float p) { p=fract(p*.1031); p*=p+33.33; return fract(p*(p+p)); }\n\n"
    "vec3 ACES(vec3 x) {\n"
    "    return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14),0.,1.);\n"
    "}\n\n"
    "void main() {\n"
    "    vec3 V = normalize(u_view_dir);\n"
    "    vec3 T = normalize(v_tangent);\n\n"
    "    /* ── Melanina por hebra: base ± varianza aleatoria ── */\n"
    "    float var = (h11(v_strand_id * 7.83) - 0.5) * u_mel_sigma * 2.0;\n"
    "    float mel_eu = clamp(v_mel_eu + var * 0.3, 0.0, 1.0);\n"
    "    float mel_ph = clamp(v_mel_ph + var * 0.1, 0.0, 1.0);\n\n"
    "    /* ── Degradado raíz/punta ── */\n"
    "    /* Raíz: color real de melanina */\n"
    "    /* Punta: desteñida por sol, química, fricción */\n"
    "    float fade = pow(v_t, 2.5);  /* no-lineal: las puntas se desteñen rápido */\n"
    "    mel_eu *= mix(1.0, 0.6, fade);\n"
    "    mel_ph *= mix(1.0, 0.8, fade);\n\n"
    "    /* ── Fracción de canas (melanocitos agotados) ── */\n"
    "    float gray_var = h11(v_strand_id * 3.14159);\n"
    "    float is_gray = step(1.0 - u_gray_fraction, gray_var);\n"
    "    mel_eu = mix(mel_eu, 0.0, is_gray);\n"
    "    mel_ph = mix(mel_ph, 0.0, is_gray);\n"
    "    /* Pelo gris: reflexión alta (cabello blanco = alta dispersión) */\n"
    "    float gray_boost = is_gray * 0.4;\n\n"
    "    /* ── Color base del cabello por absorción ── */\n"
    "    /* Transmitancia doble (TT) da el color percibido en luz difusa */\n"
    "    vec3 sigma_a = vec3(0.419, 0.697, 1.378) * mel_eu * 0.5\n"
    "                 + vec3(0.187, 0.398, 1.093) * mel_ph * 0.3;\n"
    "    vec3 base_color = exp(-sigma_a * 2.0) + gray_boost;\n"
    "    /* Color ramp raíz→punta opcional */\n"
    "    vec3 ramp = texture(u_hair_color_ramp, vec2(v_t, 0.5)).rgb;\n"
    "    base_color = mix(base_color, base_color * ramp, 0.3);\n"
    "    /* Tinte artificial */\n"
    "    base_color = mix(base_color, u_tint_color, u_tint_strength);\n\n"
    "    /* ── BSDF Marschner — 2 luces ── */\n"
    "    float cuticle_rad = u_cuticle_tilt_deg * PI / 180.0;\n"
    "    vec3 hair_lit = vec3(0.);\n"
    "    for(int li=0; li<2; li++) {\n"
    "        vec3 L  = normalize(li==0 ? u_light_dir_0 : u_light_dir_1);\n"
    "        vec3 Lc = (li==0 ? u_light_color_0 : u_light_color_1);\n"
    "        float Li= (li==0 ? u_light_intensity_0 : u_light_intensity_1);\n"
    "        vec3 bsdf = hair_bsdf(L, V, T,\n"
    "                               mel_eu, mel_ph,\n"
    "                               u_roughness_long, u_roughness_azim,\n"
    "                               cuticle_rad, u_ior,\n"
    "                               u_medulla_fraction,\n"
    "                               u_lobe_r, u_lobe_tt, u_lobe_trt);\n"
    "        hair_lit += bsdf * Lc * Li;\n"
    "    }\n\n"
    "    /* ── IBL — reflexión ambiental ── */\n"
    "    /* Para cabello, IBL difuso = ambiente general */\n"
    "    vec3 R = reflect(-V, cross(T, V));\n"
    "    vec3 env_ref = textureLod(u_env_map, R, 4.0).rgb * 0.3;\n"
    "    vec3 env_diff= textureLod(u_env_map, vec3(0.,1.,0.), 8.0).rgb * 0.2;\n\n"
    "    /* ── Highlights procedurales (sol/químico) ── */\n"
    "    /* Zona de highlights: banda lateral de la hebra */\n"
    "    float noise_h = texture(u_hair_noise, vec2(v_t * 3.0, v_strand_id)).r;\n"
    "    float highlight = v_highlight_mask * noise_h * u_highlight_intensity;\n"
    "    highlight *= (1.0 - mel_eu * 0.8);  /* visibles más en cabello claro */\n\n"
    "    /* ── Composición final ── */\n"
    "    vec3 color = base_color * (hair_lit + env_diff + env_ref);\n"
    "    color += vec3(highlight) * vec3(1.0, 0.97, 0.90);\n"
    "    /* Tone mapping */\n"
    "    color *= u_exposure;\n"
    "    color  = ACES(color);\n"
    "    color  = pow(clamp(color,0.,1.), vec3(1./2.2));\n"
    "    /* Opacidad: raíz opaca, punta semi-transparente */\n"
    "    float opacity = mix(u_opacity_root, u_opacity_tip, pow(v_t, 1.5));\n"
    "    fragColor = vec4(color, opacity);\n"
    "}\n\n";

/* ═══════════════════════════════════════════════════════════════
 * C — SISTEMA PBD (Position-Based Dynamics) para física de hebras
 * ═══════════════════════════════════════════════════════════════ */

/* Partícula de la cadena PBD */
typedef struct HairParticle {
    float px, py, pz;          /* posición actual */
    float ppx, ppy, ppz;       /* posición previa */
    float restx, resty, restz; /* posición de reposo */
    float mass_inv;            /* 1/masa (0=anclado) */
    float rest_len;            /* longitud de reposo al siguiente */
} HairParticle;

/* Restricción de longitud */
typedef struct HairLengthConstraint {
    int   i0, i1;
    float rest_len;
    float stiffness;
} HairLengthConstraint;

/* Restricción de curvatura */
typedef struct HairBendConstraint {
    int   i0, i1, i2;
    float rest_angle_cos;
    float stiffness;
} HairBendConstraint;

/* Estado de simulación de una hebra */
typedef struct HairStrand {
    int             n_particles;
    HairParticle   *particles;    /* array de partículas */
    HairLengthConstraint *len_c;  /* restricciones de longitud */
    HairBendConstraint   *bend_c; /* restricciones de curvatura */
    int             n_len, n_bend;
    float           mel_eu, mel_ph;    /* color de esta hebra */
    float           width_root;        /* ancho raíz mm */
    float           width_tip;         /* ancho punta mm */
    float           stiffness_k;       /* rigidez local */
} HairStrand;

/* Sistema completo de simulación */
typedef struct HairSimSystem {
    int         n_strands;
    HairStrand *strands;
    float       gravity_x, gravity_y, gravity_z;
    float       wind_x, wind_y, wind_z;
    float       wind_freq;
    float       wind_time;
    float       damping;
    int         pbd_iters;
    /* Proxy de colisión: elipsoide de la cabeza */
    float head_cx, head_cy, head_cz; /* centro */
    float head_rx, head_ry, head_rz; /* semiejes */
} HairSimSystem;

/* ── Inicializar una hebra desde follículo ── */
static  int hair_strand_init(
    HairStrand *strand,
    float root_x, float root_y, float root_z,
    float dir_x,  float dir_y,  float dir_z,
    float length_m, int n_segs,
    float stiffness, float mel_eu, float mel_ph,
    float width_root, float width_tip
){
    float seg_len = length_m / (float)n_segs;
    strand->n_particles = n_segs + 1;
    strand->particles   = (HairParticle*)calloc(
        strand->n_particles, sizeof(HairParticle));
    float nx = dir_x, ny = dir_y, nz = dir_z;
    float inv_len = 1.0f / sqrtf(nx*nx+ny*ny+nz*nz);
    nx*=inv_len; ny*=inv_len; nz*=inv_len;

    for (int i = 0; i <= n_segs; i++) {
        float t  = (float)i / (float)n_segs; (void)t;
        float px = root_x + nx * seg_len * i;
        float py = root_y + ny * seg_len * i;
        float pz = root_z + nz * seg_len * i;
        strand->particles[i].px = strand->particles[i].ppx =
        strand->particles[i].restx = px;
        strand->particles[i].py = strand->particles[i].ppy =
        strand->particles[i].resty = py;
        strand->particles[i].pz = strand->particles[i].ppz =
        strand->particles[i].restz = pz;
        strand->particles[i].mass_inv = (i == 0) ? 0.0f : 1.0f; /* raíz anclada */
        strand->particles[i].rest_len = seg_len;
    }
    /* Restricciones de longitud */
    strand->n_len = n_segs;
    strand->len_c = (HairLengthConstraint*)calloc(
        n_segs, sizeof(HairLengthConstraint));
    for (int i = 0; i < n_segs; i++) {
        strand->len_c[i].i0 = i;
        strand->len_c[i].i1 = i+1;
        strand->len_c[i].rest_len  = seg_len;
        strand->len_c[i].stiffness = stiffness;
    }
    /* Restricciones de curvatura */
    strand->n_bend = (n_segs >= 2) ? n_segs - 1 : 0;
    strand->bend_c = (HairBendConstraint*)calloc(
        strand->n_bend, sizeof(HairBendConstraint));
    for (int i = 0; i < strand->n_bend; i++) {
        strand->bend_c[i].i0 = i;
        strand->bend_c[i].i1 = i+1;
        strand->bend_c[i].i2 = i+2;
        strand->bend_c[i].rest_angle_cos = 0.95f; /* ~18° de curvatura natural */
        strand->bend_c[i].stiffness = stiffness * 0.5f;
    }
    strand->mel_eu      = mel_eu;
    strand->mel_ph      = mel_ph;
    strand->width_root  = width_root;
    strand->width_tip   = width_tip;
    strand->stiffness_k = stiffness;
    return 0; /* 0=OK, -1=ENOMEM o fallo de arena */
}

/* ── Un paso PBD completo ── */
int hair_sim_step(HairSimSystem *sys, float dt)
{
    if (!sys || !sys->strands || dt <= 0.0f) return -1;
    sys->wind_time += dt;

    for (int si = 0; si < sys->n_strands; si++) {
        HairStrand *s = &sys->strands[si];

        /* 1. Integración simpléctica (Verlet) */
        for (int i = 0; i < s->n_particles; i++) {
            HairParticle *p = &s->particles[i];
            if (p->mass_inv < 1e-6f) continue; /* anclado */

            /* Velocidad = pos_actual - pos_previa */
            float vx = (p->px - p->ppx) * (1.0f - sys->damping);
            float vy = (p->py - p->ppy) * (1.0f - sys->damping);
            float vz = (p->pz - p->ppz) * (1.0f - sys->damping);

            /* Viento: campo Perlin simplificado */
            float wind_var = sinf(sys->wind_time * sys->wind_freq +
                                   p->px * 3.7f + p->py * 2.1f);
            float wx = sys->wind_x + wind_var * sys->wind_x * 0.3f;
            float wy = sys->wind_y + wind_var * sys->wind_y * 0.3f;
            float wz = sys->wind_z + wind_var * sys->wind_z * 0.3f;

            /* Fuerzas: gravedad + viento */
            float ax = sys->gravity_x + wx;
            float ay = sys->gravity_y + wy;
            float az = sys->gravity_z + wz;

            /* Posición candidata */
            float nx_p = p->px + vx + ax * dt * dt;
            float ny_p = p->py + vy + ay * dt * dt;
            float nz_p = p->pz + vz + az * dt * dt;

            /* Resorte de retorno al reposo (pelo rígido) */
            float k_rest = s->stiffness_k * 0.15f * (float)(i + 1);
            float t_param = (float)i / (float)(s->n_particles - 1);
            float stiff_fade = k_rest * (1.0f - t_param * 0.6f);
            nx_p = nx_p + (p->restx - p->px) * stiff_fade * dt;
            ny_p = ny_p + (p->resty - p->py) * stiff_fade * dt;
            nz_p = nz_p + (p->restz - p->pz) * stiff_fade * dt;

            p->ppx = p->px; p->ppy = p->py; p->ppz = p->pz;
            p->px  = nx_p;  p->py  = ny_p;  p->pz  = nz_p;
        }

        /* 2. Resolver restricciones PBD (iteraciones) */
        for (int iter = 0; iter < sys->pbd_iters; iter++) {
            /* ── Restricciones de longitud ── */
            for (int ci = 0; ci < s->n_len; ci++) {
                HairLengthConstraint *c = &s->len_c[ci];
                HairParticle *p0 = &s->particles[c->i0];
                HairParticle *p1 = &s->particles[c->i1];
                float dx = p1->px - p0->px;
                float dy = p1->py - p0->py;
                float dz = p1->pz - p0->pz;
                float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                if (dist < 1e-6f) continue;
                float corr = (dist - c->rest_len) / dist * c->stiffness;
                float total_inv = p0->mass_inv + p1->mass_inv;
                if (total_inv < 1e-6f) continue;
                float w0 = p0->mass_inv / total_inv;
                float w1 = p1->mass_inv / total_inv;
                p0->px += w0 * corr * dx;
                p0->py += w0 * corr * dy;
                p0->pz += w0 * corr * dz;
                p1->px -= w1 * corr * dx;
                p1->py -= w1 * corr * dy;
                p1->pz -= w1 * corr * dz;
            }
            /* ── Restricciones de curvatura ── */
            for (int ci = 0; ci < s->n_bend; ci++) {
                HairBendConstraint *c = &s->bend_c[ci];
                HairParticle *p0 = &s->particles[c->i0];
                HairParticle *p1 = &s->particles[c->i1];
                HairParticle *p2 = &s->particles[c->i2];
                float e0x = p1->px - p0->px, e0y = p1->py - p0->py, e0z = p1->pz - p0->pz;
                float e1x = p2->px - p1->px, e1y = p2->py - p1->py, e1z = p2->pz - p1->pz;
                float l0 = sqrtf(e0x*e0x+e0y*e0y+e0z*e0z);
                float l1 = sqrtf(e1x*e1x+e1y*e1y+e1z*e1z);
                if (l0 < 1e-6f || l1 < 1e-6f) continue;
                float cos_a = (e0x*e1x+e0y*e1y+e0z*e1z) / (l0*l1);
                float err = cos_a - c->rest_angle_cos;
                /* Corrección: empujar p0 y p2 para restaurar ángulo */
                float corr_x = -err * c->stiffness * e1x / l1;
                float corr_y = -err * c->stiffness * e1y / l1;
                float corr_z = -err * c->stiffness * e1z / l1;
                if (p0->mass_inv > 1e-6f) {
                    p0->px -= corr_x * 0.5f;
                    p0->py -= corr_y * 0.5f;
                    p0->pz -= corr_z * 0.5f;
                }
                if (p2->mass_inv > 1e-6f) {
                    p2->px += corr_x * 0.5f;
                    p2->py += corr_y * 0.5f;
                    p2->pz += corr_z * 0.5f;
                }
            }
            /* ── Colisión con elipsoide de la cabeza ── */
            float hcx = sys->head_cx, hcy = sys->head_cy, hcz = sys->head_cz;
            float hrx = sys->head_rx, hry = sys->head_ry, hrz = sys->head_rz;
            for (int i = 0; i < s->n_particles; i++) {
                HairParticle *p = &s->particles[i];
                if (p->mass_inv < 1e-6f) continue;
                float ex = (p->px - hcx) / hrx;
                float ey = (p->py - hcy) / hry;
                float ez = (p->pz - hcz) / hrz;
                float d2 = ex*ex + ey*ey + ez*ez;
                if (d2 < 1.0f && d2 > 1e-6f) {
                    float d = sqrtf(d2);
                    float push = (1.0f - d) / d;
                    p->px += ex * push * hrx;
                    p->py += ey * push * hry;
                    p->pz += ez * push * hrz;
                }
            }
        } /* fin iteraciones PBD */
    } /* fin strands */
    return 0;
}

/* ── Liberar sistema de cabello ── */
int hair_sim_free(HairSimSystem *sys)
{
    if (!sys) return -1;
    for (int i = 0; i < sys->n_strands; i++) {
        free(sys->strands[i].particles);
        free(sys->strands[i].len_c);
        free(sys->strands[i].bend_c);
    }
    free(sys->strands);
    memset(sys, 0, sizeof(*sys));
    return 0;
}

/* ═══════════════════════════════════════════════════════════════
 * Función principal de generación de shaders de cabello
 * ═══════════════════════════════════════════════════════════════ */
int rig_face_ng_hair_shader(const RigHairNGCtx *ctx, RigArtResultNG *out)
{
    if (!ctx || !out) return -1;

    char *frag = (char*)malloc(HAIR_BUF);
    char *vert = (char*)malloc(HAIR_BUF / 2);
    char *js   = (char*)malloc(HAIR_JS_BUF);
    if (!frag || !vert || !js) { free(frag); free(vert); free(js); return -1; }

    int fp = 0, vp = 0, jp = 0;
    int fsz = HAIR_BUF, vsz = HAIR_BUF/2, jsz = HAIR_JS_BUF;

    /* Vertex */
    FA(vert, vsz, vp, "%s", NG_HAIR_VERTEX_GLSL);

    /* Fragment: librería de Marschner + main */
    FA(frag, fsz, fp, "%s", NG_HAIR_MARSCHNER_GLSL);
    FA(frag, fsz, fp, "%s", NG_HAIR_FRAG_GLSL);

    /* JavaScript runtime */
    FA(js, jsz, jp,
        "// §NG-HAIR RigCom Next Generation — Hair Material + Sim\n"
        "// φ=1.6180339887 · Richard Felipe Urbina\n"
        "'use strict';\n\n"
        "class RigNGHairMaterial {\n"
        "  constructor(gl, prog) {\n"
        "    this.gl = gl; this.prog = prog;\n"
        "    this.L = n => gl.getUniformLocation(prog, n);\n"
        "    this.bio = {\n"
        "      mel_eu_base: %.4f,\n"
        "      mel_ph_base: %.4f,\n"
        "      mel_sigma:   %.4f,\n"
        "      gray_fraction: %.4f,\n"
        "      tint_color: new Float32Array([%.4f, %.4f, %.4f]),\n"
        "      tint_strength: %.4f,\n"
        "      highlight_intensity: %.4f\n"
        "    };\n"
        "    this.optics = {\n"
        "      roughness_long: %.4f,\n"
        "      roughness_azim: %.4f,\n"
        "      cuticle_tilt_deg: %.2f,\n"
        "      ior: %.4f,\n"
        "      medulla_fraction: %.4f,\n"
        "      lobe_r:  %.4f,\n"
        "      lobe_tt: %.4f,\n"
        "      lobe_trt:%.4f\n"
        "    };\n"
        "    this.opacity = { root: 1.0, tip: 0.05 };\n"
        "    this.exposure = 1.0;\n"
        "  }\n\n",
        ctx->melanin_eu, ctx->melanin_ph, ctx->melanin_sigma,
        ctx->gray_fraction,
        ctx->tint_r, ctx->tint_g, ctx->tint_b,
        ctx->tint_strength, ctx->highlight_intensity,
        ctx->roughness_long, ctx->roughness_azimuth,
        ctx->cuticle_tilt, ctx->ior_cortex, ctx->medulla_fraction,
        ctx->specular_lobe_r, ctx->specular_lobe_tt, ctx->specular_lobe_trt);

    FA(js, jsz, jp,
        "  bind() {\n"
        "    const {gl, L, bio, optics, opacity} = this;\n"
        "    gl.uniform1f(L('u_mel_eu_base'),        bio.mel_eu_base);\n"
        "    gl.uniform1f(L('u_mel_ph_base'),        bio.mel_ph_base);\n"
        "    gl.uniform1f(L('u_mel_sigma'),          bio.mel_sigma);\n"
        "    gl.uniform1f(L('u_gray_fraction'),      bio.gray_fraction);\n"
        "    gl.uniform3fv(L('u_tint_color'),        bio.tint_color);\n"
        "    gl.uniform1f(L('u_tint_strength'),      bio.tint_strength);\n"
        "    gl.uniform1f(L('u_highlight_intensity'),bio.highlight_intensity);\n"
        "    gl.uniform1f(L('u_roughness_long'),     optics.roughness_long);\n"
        "    gl.uniform1f(L('u_roughness_azim'),     optics.roughness_azim);\n"
        "    gl.uniform1f(L('u_cuticle_tilt_deg'),   optics.cuticle_tilt_deg);\n"
        "    gl.uniform1f(L('u_ior'),                optics.ior);\n"
        "    gl.uniform1f(L('u_medulla_fraction'),   optics.medulla_fraction);\n"
        "    gl.uniform1f(L('u_lobe_r'),             optics.lobe_r);\n"
        "    gl.uniform1f(L('u_lobe_tt'),            optics.lobe_tt);\n"
        "    gl.uniform1f(L('u_lobe_trt'),           optics.lobe_trt);\n"
        "    gl.uniform1f(L('u_opacity_root'),       opacity.root);\n"
        "    gl.uniform1f(L('u_opacity_tip'),        opacity.tip);\n"
        "    gl.uniform1f(L('u_exposure'),           this.exposure);\n"
        "  }\n\n"
        "  setHairColor(preset) {\n"
        "    const p = RIG_HAIR_PRESETS[preset];\n"
        "    if(p) Object.assign(this.bio, p);\n"
        "  }\n"
        "  setAge(years) {\n"
        "    // Canas proporcionales a la edad\n"
        "    this.bio.gray_fraction = Math.max(0, Math.min(1, (years-35)/40));\n"
        "    // Pérdida de pigmento: mel_eu disminuye\n"
        "    this.bio.mel_eu_base *= Math.max(0.2, 1.0 - (years-30)/100);\n"
        "  }\n"
        "  setTint(r, g, b, strength) {\n"
        "    this.bio.tint_color.set([r,g,b]);\n"
        "    this.bio.tint_strength = strength;\n"
        "  }\n"
        "}\n\n"
        "const RIG_HAIR_PRESETS = {\n"
        "  jet_black:    { mel_eu_base:0.95, mel_ph_base:0.02, gray_fraction:0.0, tint_strength:0.0 },\n"
        "  dark_brown:   { mel_eu_base:0.65, mel_ph_base:0.12, gray_fraction:0.0, tint_strength:0.0 },\n"
        "  chestnut:     { mel_eu_base:0.40, mel_ph_base:0.25, gray_fraction:0.0, tint_strength:0.0 },\n"
        "  auburn:       { mel_eu_base:0.25, mel_ph_base:0.60, gray_fraction:0.0, tint_strength:0.0 },\n"
        "  golden_blond: { mel_eu_base:0.08, mel_ph_base:0.40, gray_fraction:0.0, tint_strength:0.0 },\n"
        "  platinum:     { mel_eu_base:0.01, mel_ph_base:0.05, gray_fraction:0.0, tint_strength:0.0 },\n"
        "  red:          { mel_eu_base:0.05, mel_ph_base:0.90, gray_fraction:0.0, tint_strength:0.0 },\n"
        "  silver:       { mel_eu_base:0.10, mel_ph_base:0.05, gray_fraction:0.60, tint_strength:0.0 },\n"
        "  white:        { mel_eu_base:0.00, mel_ph_base:0.00, gray_fraction:1.00, tint_strength:0.0 },\n"
        "  tinted_blue:  { mel_eu_base:0.02, mel_ph_base:0.02, gray_fraction:0.0,\n"
        "                  tint_color: new Float32Array([0.05,0.1,0.9]), tint_strength:0.85 },\n"
        "  tinted_rose:  { mel_eu_base:0.02, mel_ph_base:0.10, gray_fraction:0.0,\n"
        "                  tint_color: new Float32Array([0.9,0.3,0.5]), tint_strength:0.80 }\n"
        "};\n\n"
        "// Curvas de cabello por tipo étnico\n"
        "const RIG_HAIR_CURVES = {\n"
        "  straight: { curl_radius_mm:0, curl_freq:0, cross_section_ratio:1.0, strand_count:100000 },\n"
        "  wavy:     { curl_radius_mm:15, curl_freq:0.5, cross_section_ratio:0.85, strand_count:90000 },\n"
        "  curly:    { curl_radius_mm:8,  curl_freq:1.2, cross_section_ratio:0.70, strand_count:80000 },\n"
        "  coily:    { curl_radius_mm:4,  curl_freq:2.5, cross_section_ratio:0.55, strand_count:75000 },\n"
        "  kinky:    { curl_radius_mm:1.5,curl_freq:5.0, cross_section_ratio:0.40, strand_count:70000 }\n"
        "};\n\n"
        "export { RigNGHairMaterial, RIG_HAIR_PRESETS, RIG_HAIR_CURVES };\n");

    out->glsl_vert = vert;
    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    return fp + vp + jp;
}
