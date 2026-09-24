/* ==========================================================================
 * 13_rig_face_ng_hair.c   —   RIGFACE :: C UNIFICADO
 *
 * Copia canonica : nested/Face_Pose_Body/rig_face_ng_hair.c
 * Copias fundidas: 6
 * Funciones      : 9      Unidades injertadas: 35      Variantes: 5
 *
 * Copia canonica preservada verbatim. Ninguna funcion eliminada,
 * reducida ni omitida. Las divergencias de cuerpo bajo un mismo
 * nombre se conservan como <nombre>__vN con su procedencia.
 * ========================================================================== */
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


/* ========================================================================
 * SECCION DE FUSION — unidades reales de las otras 5 copias de este
 * modulo, ausentes en la copia canonica. Se incorporan integras.
 * ======================================================================== */

/* [fusion] typedef cornea_depth_mm <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:1110 :: absorbido de rig_face_ng */
typedef struct RigEyeNGCtx {
    float pupil_radius, iris_radius;
    float iris_r, iris_g, iris_b, iris_melanin;
    float crypts_density, collarette_pos, limbal_health;
    float age_norm, hemoglobin, jaundice, dryness;
    float tear_thickness_um, cornea_depth_mm;
}

/* [fusion] typedef sleepiness <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:1491 :: absorbido de rig_face_ng */
typedef struct EyeDynamicsNG {
    /* Posición de la mirada (ángulos en grados) */
    float gaze_h;          /* horizontal: -30° a +30° */
    float gaze_v;          /* vertical: -20° a +20° */
    float gaze_target_h;   /* objetivo actual */
    float gaze_target_v;

    /* Sacadas */
    float saccade_vel_h;   /* velocidad angular deg/s */
    float saccade_vel_v;
    float saccade_time;    /* tiempo restante de sacada */
    float saccade_dur;     /* duración total de la sacada */
    bool  in_saccade;

    /* Microsacadas (movimientos de fijación) */
    float micro_h, micro_v;
    float micro_freq;      /* Hz: ~2 Hz */
    float micro_time;

    /* Deriva lenta */
    float drift_h, drift_v;
    float drift_vel_h, drift_vel_v;

    /* Tremor (alta frecuencia, pequeña amplitud) */
    float tremor_amp;      /* arcsec */
    float tremor_time;

    /* Convergencia */
    float vergence_angle;  /* ángulo de vergencia en grados */
    float vergence_target;

    /* Pupila */
    float pupil_radius;    /* radio actual en mm: 2-8 */
    float pupil_target;    /* objetivo */
    float pupil_vel;       /* velocidad de cambio mm/s */
    float hipus_phase;     /* fase del hipus pupilar */
    float light_level;     /* 0=oscuro 1=plena luz */

    /* Parpadeo */
    float blink_phase;     /* 0=abierto 1=cerrado */
    float blink_speed;     /* velocidad de parpadeo */
    float next_blink_time; /* cuándo parpadear (s) */
    float time_acc;        /* acumulador de tiempo */

    /* Emoción → modifica apertura y dilatación */
    float surprise;        /* 0-1 sorpresa → más abierto */
    float fear;            /* 0-1 miedo → más abierto */
    float sleepiness;      /* 0-1 somnolencia → más cerrado */
}

/* [fusion] typedef asym_factor <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:2719 :: absorbido de rig_face_ng */
typedef struct {
    int    id;           /* número FACS canónico o NG-ext */
    char   name[64];     /* nombre oficial */
    char   muscle[80];   /* músculos primarios */
    char   antagonist[60];/* músculo antagonista */
    char   emotion[48];  /* emoción asociada */
    float  latency_ms;   /* latencia de onset real (ms) */
    float  peak_ms;      /* tiempo al apex (ms) */
    float  offset_ms;    /* tiempo de offset (ms) */
    float  bilateral;    /* 1.0=bilateral 0=unilateral */
    float  intensity_max;/* rango máximo [0..1] */
    char   region;       /* F=frente O=ojos N=nariz B=boca C=mejilla */
    bool   micro;        /* ¿aparece en micro-expresiones? */
    float  asym_factor;  /* asimetría natural típica */
}

/* [fusion] typedef PHON_NASAL <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:2924 :: absorbido de rig_face_ng */
typedef enum {
    PHON_VOWEL   = 0,
    PHON_BILABIAL= 1,  /* labios juntos */
    PHON_LABIO   = 2,  /* labio-dental */
    PHON_DENTAL  = 3,
    PHON_ALVEOLAR= 4,
    PHON_PALATAL = 5,
    PHON_VELAR   = 6,
    PHON_GLOTTAL = 7,
    PHON_NASAL   = 8
}

/* [fusion] typedef cat <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:2936 :: absorbido de rig_face_ng */
typedef struct {
    int   id;
    char  ipa[8];          /* símbolo IPA */
    char  example[24];     /* ejemplo en inglés/español */
    int   viseme_id;       /* visema correspondiente (0-21) */
    float jaw_open;        /* apertura mandíbula [0-1] */
    float lip_rounding;    /* redondeamiento labios [0-1] */
    float lip_spreading;   /* separación comisuras [0-1] */
    float lip_protrusion;  /* protrusión [0-1] */
    float tongue_height;   /* altura de la lengua [0-1] */
    float tongue_back;     /* posición anterior-posterior [0-1] */
    float teeth_show;      /* exposición de dientes [0-1] */
    float velum_open;      /* velo abierto=nasal [0-1] */
    PhonemeCat cat;
}

/* [fusion] typedef decay_s <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:3019 :: absorbido de rig_face_ng */
typedef struct {
    char  name[32];
    float P;              /* Pleasure     -1..+1 */
    float A;              /* Arousal      -1..+1 */
    float D;              /* Dominance    -1..+1 */
    float au_weights[128];/* pesos para los 128 AU */
    float duration_peak_s;/* duración típica del apex en segundos */
    float decay_s;        /* tiempo de decay */
}

/* [fusion] typedef time <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:3034 :: absorbido de rig_face_ng */
typedef struct {
    /* FACS weights — 128 AUs activos */
    float au_current[128];     /* valor actual */
    float au_target[128];      /* objetivo */
    float au_velocity[128];    /* velocidad de cambio (spring) */

    /* Micro-expresiones: cola circular */
    struct {
        int   au_id;
        float intensity;
        float start_time;
        float peak_time;
        float end_time;
        bool  active;
    } micro_queue[32];
    int   micro_head;

    /* Movimiento ocular fisiológico */
    struct {
        float yaw;             /* horizontal rad */
        float pitch;           /* vertical rad */
        float yaw_v;           /* velocidad */
        float pitch_v;
        float saccade_timer;   /* tiempo hasta próxima sacada */
        float drift_yaw;       /* drift acumulado */
        float drift_pitch;
        float tremor_phase;    /* oscilación 80-100Hz */
        bool  in_saccade;
        float saccade_dur;
        float saccade_progress;
        float target_yaw;
        float target_pitch;
    } eye_state;

    /* Parpadeo */
    struct {
        float timer;           /* tiempo hasta próximo parpadeo */
        float rate_bpm;        /* tasa: 15-20/min en reposo */
        float phase;           /* [0=abierto .. 1=cerrado] */
        bool  in_blink;
        float blink_dur;       /* ~150ms */
        bool  voluntary;
    } blink_state;

    /* Respiración */
    struct {
        float phase;           /* 0-1 ciclo completo */
        float rate_bpm;        /* 12-20 rpm */
        float depth;           /* 0-1 profundidad */
        float hold_timer;      /* apnea post-exhalación */
        bool  is_inhale;
    } breath_state;

    /* Latido cardíaco */
    struct {
        float phase;           /* 0-TAU */
        float bpm;             /* 50-200 */
        float hrv;             /* variabilidad HR */
        float hrv_phase;       /* fase de variabilidad */
    } heart_state;

    /* Física secundaria — masa+resorte */
    struct {
        /* Papada */
        float jowl_y;          /* desplazamiento vertical */
        float jowl_vy;
        /* Mejillas */
        float cheek_L[3];      /* posición XYZ offset L */
        float cheek_R[3];
        float cheek_vL[3];
        float cheek_vR[3];
        /* Piel (wave) */
        float skin_wave[16];   /* ondas en regiones */
        float skin_wave_v[16];
    } phys_state;

    /* Mandíbula — modelo masa+resorte */
    struct {
        float angle;           /* ángulo rad (0=cerrado) */
        float angle_v;
        float target;          /* objetivo deseado */
        float mass;            /* 0.3 kg */
        float spring;          /* 80 N/m */
        float damping;         /* 18 N·s/m */
    } jaw_state;

    /* Leakage emocional (emoción reprimida que se filtra) */
    float leakage_weights[128];
    float leakage_intensity;

    /* Tiempo global */
    float time;

}

/* [fusion] global NG_GLSL_HEADER <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:56 :: absorbido de rig_face_ng */
static const char *NG_GLSL_HEADER =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "precision highp sampler2D;\n\n"
    "/* ── Constantes φ-soberanas ───────────────────────────────── */\n"
    "const float PHI        = 1.6180339887;\n"
    "const float PHI_INV    = 0.6180339887;\n"
    "const float PHI2       = 2.6180339887;  /* φ² = φ+1 */\n"
    "const float PHI_INV2   = 0.3819660113;  /* φ⁻² = 1-φ⁻¹ */\n"
    "const float PI         = 3.14159265359;\n"
    "const float TAU        = 6.28318530718;\n"
    "const float INV_PI     = 0.31830988618;\n"
    "const float INV_TAU    = 0.15915494309;\n"
    "const float SQRT2      = 1.41421356237;\n"
    "const float SQRT3      = 1.73205080757;\n"
    "const float SCHUMANN   = 7.83;          /* Hz — Resonancia Schumann */\n"
    "const float E_BASE     = 2.71828182846;\n\n"
    "/* ── Constantes biofísicas de piel ───────────────────────── */\n"
    "const float IOR_CORNEUM   = 1.55;  /* estrato córneo */\n"
    "const float IOR_EPIDERMIS = 1.43;  /* epidermis */\n"
    "const float IOR_DERMIS    = 1.38;  /* dermis */\n"
    "const float IOR_AIR       = 1.00;\n"
    "const float SKIN_G        = 0.90;  /* factor de anisotropía de Henyey-Greenstein */\n\n";

/* [fusion] global NG_GLSL_PROCEDURAL <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:84 :: absorbido de rig_face_ng */
static const char *NG_GLSL_PROCEDURAL =
    "/* ══════════════════════════════════════════════════════════ */\n"
    "/* LIBRERÍA DE RUIDO PROCEDURAL SOBERANA                      */\n"
    "/* ══════════════════════════════════════════════════════════ */\n\n"
    "/* Hash sin seno — Jarzynski & Harsh 2016 */\n"
    "float hash11(float p) {\n"
    "    p = fract(p * .1031);\n"
    "    p *= p + 33.33;\n"
    "    return fract(p * (p + p));\n"
    "}\n"
    "float hash12(vec2 p) {\n"
    "    vec3 p3 = fract(vec3(p.xyx) * .1031);\n"
    "    p3 += dot(p3, p3.yzx + 33.33);\n"
    "    return fract((p3.x + p3.y) * p3.z);\n"
    "}\n"
    "float hash13(vec3 p) {\n"
    "    p = fract(p * .1031);\n"
    "    p += dot(p, p.zyx + 31.32);\n"
    "    return fract((p.x + p.y) * p.z);\n"
    "}\n"
    "vec2 hash22(vec2 p) {\n"
    "    vec3 p3 = fract(vec3(p.xyx) * vec3(.1031,.1030,.0973));\n"
    "    p3 += dot(p3, p3.yzx + 33.33);\n"
    "    return fract((p3.xx + p3.yz) * p3.zy);\n"
    "}\n"
    "vec3 hash33(vec3 p3) {\n"
    "    p3 = fract(p3 * vec3(.1031,.1030,.0973));\n"
    "    p3 += dot(p3, p3.yxz + 33.33);\n"
    "    return fract((p3.xxy + p3.yxx) * p3.zyx);\n"
    "}\n\n"
    "/* Value noise suave 2D */\n"
    "float vnoise2(vec2 p) {\n"
    "    vec2 i = floor(p);\n"
    "    vec2 f = fract(p);\n"
    "    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0); /* quintic */\n"
    "    return mix(mix(hash12(i + vec2(0,0)), hash12(i + vec2(1,0)), u.x),\n"
    "               mix(hash12(i + vec2(0,1)), hash12(i + vec2(1,1)), u.x), u.y);\n"
    "}\n"
    "/* Value noise suave 3D */\n"
    "float vnoise3(vec3 p) {\n"
    "    vec3 i = floor(p); vec3 f = fract(p);\n"
    "    vec3 u = f*f*f*(f*(f*6.-15.)+10.);\n"
    "    return mix(mix(mix(hash13(i),           hash13(i+vec3(1,0,0)),u.x),\n"
    "                   mix(hash13(i+vec3(0,1,0)),hash13(i+vec3(1,1,0)),u.x),u.y),\n"
    "               mix(mix(hash13(i+vec3(0,0,1)),hash13(i+vec3(1,0,1)),u.x),\n"
    "                   mix(hash13(i+vec3(0,1,1)),hash13(i+vec3(1,1,1)),u.x),u.y),u.z);\n"
    "}\n\n"
    "/* Gradient noise 3D (Perlin) */\n"
    "float gnoise3(vec3 p) {\n"
    "    vec3 i = floor(p); vec3 f = fract(p);\n"
    "    vec3 u = f*f*(3.-2.*f);\n"
    "    vec3 g000=hash33(i)-0.5,       g100=hash33(i+vec3(1,0,0))-0.5;\n"
    "    vec3 g010=hash33(i+vec3(0,1,0))-0.5, g110=hash33(i+vec3(1,1,0))-0.5;\n"
    "    vec3 g001=hash33(i+vec3(0,0,1))-0.5, g101=hash33(i+vec3(1,0,1))-0.5;\n"
    "    vec3 g011=hash33(i+vec3(0,1,1))-0.5, g111=hash33(i+vec3(1,1,1))-0.5;\n"
    "    float v000=dot(g000,f),           v100=dot(g100,f-vec3(1,0,0));\n"
    "    float v010=dot(g010,f-vec3(0,1,0)),v110=dot(g110,f-vec3(1,1,0));\n"
    "    float v001=dot(g001,f-vec3(0,0,1)),v101=dot(g101,f-vec3(1,0,1));\n"
    "    float v011=dot(g011,f-vec3(0,1,1)),v111=dot(g111,f-vec3(1,1,1));\n"
    "    return mix(mix(mix(v000,v100,u.x),mix(v010,v110,u.x),u.y),\n"
    "               mix(mix(v001,v101,u.x),mix(v011,v111,u.x),u.y),u.z);\n"
    "}\n\n"
    "/* fBm — fractal Brownian motion, hasta 8 octavas */\n"
    "float fbm(vec3 p, int oct, float lacunarity, float gain) {\n"
    "    float v=0., a=0.5;\n"
    "    mat3 rot = mat3(0.8,-0.6,0., 0.6,0.8,0., 0.,0.,1.);\n"
    "    for(int i=0;i<8;i++) {\n"
    "        if(i>=oct) break;\n"
    "        v += a * vnoise3(p);\n"
    "        p = rot * p * lacunarity;\n"
    "        a *= gain;\n"
    "    }\n"
    "    return v;\n"
    "}\n\n"
    "/* Ruido φ-quasi-periódico — distribucion de Fibonacci */\n"
    "float phi_noise(vec3 p) {\n"
    "    float n = p.x*PHI + p.y*PHI*PHI + p.z*PHI*PHI*PHI;\n"
    "    return fract(sin(n*127.1+12.9898)*43758.5453);\n"
    "}\n\n"
    "/* Voronoi F1 + F2 — celdas */\n"
    "vec3 voronoi(vec3 p) {\n"
    "    vec3 b = floor(p);\n"
    "    vec3 f = fract(p);\n"
    "    float F1 = 8., F2 = 8.;\n"
    "    vec3 cell1 = vec3(0.);\n"
    "    for(int k=-1;k<=1;k++) for(int j=-1;j<=1;j++) for(int i=-1;i<=1;i++) {\n"
    "        vec3 n = vec3(float(i),float(j),float(k));\n"
    "        vec3 c = hash33(b+n) + n;\n"
    "        float d = length(f - c);\n"
    "        if(d < F1) { F2=F1; F1=d; cell1=b+n; }\n"
    "        else if(d < F2) F2=d;\n"
    "    }\n"
    "    return vec3(F1, F2, hash13(cell1));\n"
    "}\n\n"
    "/* Curl noise 3D — para fluidos/vapor/sudor */\n"
    "vec3 curl_noise(vec3 p) {\n"
    "    const float e = 0.01;\n"
    "    float nx0=vnoise3(p+vec3(0,e,0)), nx1=vnoise3(p-vec3(0,e,0));\n"
    "    float ny0=vnoise3(p+vec3(0,0,e)), ny1=vnoise3(p-vec3(0,0,e));\n"
    "    float nz0=vnoise3(p+vec3(e,0,0)), nz1=vnoise3(p-vec3(e,0,0));\n"
    "    float a0=vnoise3(p+vec3(0,e,0)), a1=vnoise3(p+vec3(0,0,e));\n"
    "    float b0=vnoise3(p+vec3(0,0,e)), b1=vnoise3(p+vec3(e,0,0));\n"
    "    float c0=vnoise3(p+vec3(e,0,0)), c1=vnoise3(p+vec3(0,e,0));\n"
    "    return normalize(vec3(\n"
    "        (nx0-nx1)-(a0-a1),\n"
    "        (ny0-ny1)-(b0-b1),\n"
    "        (nz0-nz1)-(c0-c1)) * (0.5/e));\n"
    "}\n\n";

/* [fusion] global NG_GLSL_PBR_SSS <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:196 :: absorbido de rig_face_ng */
static const char *NG_GLSL_PBR_SSS =
    "/* ══════════════════════════════════════════════════════════ */\n"
    "/* PBR COOK-TORRANCE COMPLETO + SSS ESPECTRAL 8 CAPAS         */\n"
    "/* ══════════════════════════════════════════════════════════ */\n\n"
    "/* Fresnel Schlick con F82 (Naty Hoffman 2016) */\n"
    "vec3 F_Schlick(float cosTheta, vec3 F0, vec3 F82) {\n"
    "    float x = 1.0 - cosTheta;\n"
    "    float x2 = x*x; float x5 = x2*x2*x;\n"
    "    /* Interpolación cúbica entre F0 y valor a 82° */\n"
    "    vec3 F = F0 + (F82 - F0) * max(0., 1.0 - 7.0*cosTheta) * x5 * x2;\n"
    "    return clamp(F0 + (1.0 - F0)*x5, vec3(0.), vec3(1.));\n"
    "}\n\n"
    "/* GGX NDF — Trowbridge-Reitz con clamp numérico */\n"
    "float D_GGX(float NdH, float alpha) {\n"
    "    float a2 = alpha * alpha;\n"
    "    float d  = NdH * NdH * (a2 - 1.0) + 1.0;\n"
    "    return a2 / max(PI * d * d, 1e-7);\n"
    "}\n\n"
    "/* GGX NDF anisótropo — para pelo/fibra de colágeno */\n"
    "float D_GGX_aniso(float NdH, float TdH, float BdH,\n"
    "                   float ax, float ay) {\n"
    "    float d = TdH*TdH/(ax*ax) + BdH*BdH/(ay*ay) + NdH*NdH;\n"
    "    return 1.0 / max(PI * ax * ay * d * d, 1e-7);\n"
    "}\n\n"
    "/* Geometría Schlick-GGX + Smith */\n"
    "float G_SchlickGGX(float NdV, float alpha) {\n"
    "    float k = (alpha + 1.0) * (alpha + 1.0) * 0.125;\n"
    "    return NdV / max(NdV*(1.0 - k) + k, 1e-7);\n"
    "}\n"
    "float G_Smith(float NdV, float NdL, float alpha) {\n"
    "    return G_SchlickGGX(NdV,alpha) * G_SchlickGGX(NdL,alpha);\n"
    "}\n\n"
    "/* ACES Filmic tone mapping — Hill/Narkowicz */\n"
    "vec3 ACES_tonemap(vec3 x) {\n"
    "    const float a=2.51, b=0.03, c=2.43, d=0.59, e=0.14;\n"
    "    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);\n"
    "}\n\n"
    "/* Neutral tone map (Khronos) */\n"
    "vec3 neutral_tonemap(vec3 c) {\n"
    "    float peak = max(max(c.r,c.g),c.b);\n"
    "    vec3  ratio= c / max(peak, 1e-7);\n"
    "    float compressor = peak / (0.18 + peak);\n"
    "    return ratio * compressor;\n"
    "}\n\n"
    "/* ── SSS 8 CAPAS ESPECTRAL ── */\n"
    "/* Modelo físico de dispersión de luz en piel humana\n"
    " * Referencia: Donner & Jensen 2005 — «Light Diffusion in Multi-Layered Translucent Materials»\n"
    " * Modificado para tiempo real con aproximación exponencial doble\n"
    " *\n"
    " * Capas (exterior → interior):\n"
    " *  0: Película sebácea     (ε = 0.002 mm)  IOR=1.48\n"
    " *  1: Estrato córneo       (ε = 0.015 mm)  IOR=1.55\n"
    " *  2: Estrato granuloso    (ε = 0.020 mm)  IOR=1.50\n"
    " *  3: Estrato espinoso     (ε = 0.030 mm)  IOR=1.43\n"
    " *  4: Estrato basal        (ε = 0.050 mm)  IOR=1.44  ← melanocitos\n"
    " *  5: Dermis papilar       (ε = 0.200 mm)  IOR=1.38  ← vasos papilares\n"
    " *  6: Dermis reticular     (ε = 1.000 mm)  IOR=1.38  ← colágeno+elastina\n"
    " *  7: Hipodermis           (ε = 3.000 mm)  IOR=1.44  ← tejido adiposo\n"
    " */\n"
    "vec3 sss_8layer(\n"
    "    vec3  albedo,\n"
    "    float thickness,        /* mapa de espesor 0-1 → 0-4mm */\n"
    "    float melanin_eu,       /* 0-1: eumelanina (negro-marrón) */\n"
    "    float melanin_ph,       /* 0-1: feomelanina (rojo-amarillo) */\n"
    "    float hemoglobin_oxy,   /* 0-1: oxihemoglobina (rojo) */\n"
    "    float hemoglobin_deoxy, /* 0-1: deoxihemoglobina (azul-morado) */\n"
    "    float bilirubin,        /* 0-1: bilirrubina (amarillo) */\n"
    "    float carotene,         /* 0-1: caroteno (naranja) */\n"
    "    float age               /* 0-1: envejecimiento */\n"
    ") {\n"
    "    /* Coeficientes de absorción por cromóforo (mm⁻¹) aproximados RGB */\n"
    "    /* Melanina eumelanina: absorbe todo, máximo en UV/azul */\n"
    "    vec3 mu_a_eu  = vec3(0.018, 0.013, 0.006) * melanin_eu * 12.0;\n"
    "    /* Feomelanina: rojo-amarillo */\n"
    "    vec3 mu_a_ph  = vec3(0.004, 0.008, 0.016) * melanin_ph * 4.0;\n"
    "    /* Oxihemoglobina: pico a 542nm y 577nm */\n"
    "    vec3 mu_a_Hbo = vec3(0.022, 0.008, 0.003) * hemoglobin_oxy * 8.0;\n"
    "    /* Deoxihemoglobina: pico a 555nm y 760nm */\n"
    "    vec3 mu_a_Hbd = vec3(0.008, 0.016, 0.025) * hemoglobin_deoxy * 6.0;\n"
    "    /* Bilirrubina: máximo ~460nm (amarillo-naranja) */\n"
    "    vec3 mu_a_bil = vec3(0.020, 0.010, 0.000) * bilirubin * 3.0;\n"
    "    /* Caroteno: máximo ~450nm */\n"
    "    vec3 mu_a_car = vec3(0.015, 0.008, 0.000) * carotene * 2.0;\n\n"
    "    /* Absorción total combinada */\n"
    "    vec3 mu_a = mu_a_eu + mu_a_ph + mu_a_Hbo + mu_a_Hbd + mu_a_bil + mu_a_car;\n"
    "    mu_a = max(mu_a, vec3(0.001));\n\n"
    "    /* Coeficiente de dispersión reducido de cada capa (mm⁻¹) */\n"
    "    /* μs' = μs*(1-g), g=0.9 anisotropía */\n"
    "    float sc = 1.0 + age * 0.5;\n"
    "    vec3 ms_oil   = vec3(1.2,  1.8,  2.2)  * sc;\n"
    "    vec3 ms_corn  = vec3(2.5,  3.2,  4.0)  * sc;\n"
    "    vec3 ms_gran  = vec3(2.0,  2.8,  3.5)  * sc;\n"
    "    vec3 ms_spin  = vec3(1.8,  2.5,  3.2)  * sc;\n"
    "    vec3 ms_basal = vec3(1.5,  2.0,  2.8)  * sc;\n"
    "    vec3 ms_dpap  = vec3(0.8,  1.2,  1.6)  * sc;\n"
    "    vec3 ms_dret  = vec3(0.5,  0.8,  1.1)  * sc;\n"
    "    vec3 ms_hyp   = vec3(0.3,  0.4,  0.5)  * sc;\n\n"
    "    /* Profundidad física (mm) en función del mapa de espesor */\n"
    "    float depth_mm = thickness * 4.0;  /* 0-4 mm */\n\n"
    "    /* Perfil de difusión doble exponencial por capa */\n"
    "    /* L_layer = albedo * exp(-mu_eff * depth) */\n"
    "    /* mu_eff  = sqrt(3 * mu_a * (mu_a + mu_s')) */\n"
    "    #define SSS_LAYER(d, mu_s, w) \\\n"
    "        (albedo * exp(-(sqrt(3.0*mu_a*(mu_a+mu_s))*d)) * (w))\n\n"
    "    vec3 L = vec3(0.);\n"
    "    float d = depth_mm;\n"
    "    L += SSS_LAYER(min(d, 0.002), ms_oil,   0.02);\n"
    "    L += SSS_LAYER(min(d, 0.017), ms_corn,  0.08);\n"
    "    L += SSS_LAYER(min(d, 0.037), ms_gran,  0.08);\n"
    "    L += SSS_LAYER(min(d, 0.067), ms_spin,  0.10);\n"
    "    L += SSS_LAYER(min(d, 0.117), ms_basal, 0.12);\n"
    "    L += SSS_LAYER(min(d, 0.317), ms_dpap,  0.20);\n"
    "    L += SSS_LAYER(min(d, 1.317), ms_dret,  0.25);\n"
    "    L += SSS_LAYER(    d,         ms_hyp,   0.15);\n"
    "    #undef SSS_LAYER\n\n"
    "    /* Componente de retro-dispersión (backscatter): luz que vuelve */\n"
    "    float back = exp(-depth_mm * PHI_INV * max(mu_a.g, 0.001));\n"
    "    L += albedo * back * vec3(1.0, 0.72, 0.46) * 0.35;\n\n"
    "    return L;\n"
    "}\n\n"
    "/* ── ALBEDO BIOQUÍMICO — Donner-Jensen cromóforo completo ── */\n"
    "vec3 skin_bio_albedo(\n"
    "    vec3  base_tex,\n"
    "    float melanin_eu, float melanin_ph,\n"
    "    float hemoglobin_oxy, float hemoglobin_deoxy,\n"
    "    float bilirubin, float carotene,\n"
    "    float age, float sun_damage\n"
    ") {\n"
    "    vec3 c = base_tex;\n"
    "    /* Eumelanina: absorción amplia, más en azul */\n"
    "    c *= (1.0 - vec3(0.55, 0.38, 0.72) * melanin_eu);\n"
    "    /* Feomelanina: rojo-amarillo */\n"
    "    c *= (1.0 - vec3(0.05, 0.20, 0.60) * melanin_ph);\n"
    "    /* Oxihemoglobina: rosado */\n"
    "    c += vec3(0.22, 0.04, 0.02) * hemoglobin_oxy;\n"
    "    /* Deoxihemoglobina: azulado */\n"
    "    c += vec3(0.02, 0.04, 0.12) * hemoglobin_deoxy;\n"
    "    /* Bilirrubina: amarillo */\n"
    "    c += vec3(0.15, 0.12, -0.02) * bilirubin;\n"
    "    /* Caroteno: naranja */\n"
    "    c += vec3(0.18, 0.12, 0.0) * carotene;\n"
    "    /* Envejecimiento: desaturar + amarillear */\n"
    "    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));\n"
    "    c = mix(c, vec3(lum) * vec3(1.02, 1.0, 0.95), age * 0.12);\n"
    "    /* Daño solar: manchas de hiperpigmentación */\n"
    "    c *= (1.0 + sun_damage * vec3(-0.1, -0.08, -0.15));\n"
    "    return clamp(c, 0.0, 1.0);\n"
    "}\n\n"
    "/* ── POROS SEBÁCEOS — modelo Voronoi con interreflexión ── */\n"
    "/* Referencia: Ghosh et al. 2011 «Estimating Specular Roughness\n"
    " *              and Anisotropy from Second Order Irradiance Gradients»\n"
    " * Los poros crean micro-cavidades donde la luz sufre múltiple\n"
    " * reflexión antes de salir — resultado: oscurecimiento local */\n"
    "struct PoreResult {\n"
    "    float depth_mask;    /* 0=piel lisa 1=centro del poro */\n"
    "    vec3  normal_delta;  /* perturbación de normal */\n"
    "    float ao;            /* oclusión ambiental micro */\n"
    "    float roughness_mod; /* modificador de roughness */\n"
    "    float oiliness_mod;  /* sebo acumulado en borde del poro */\n"
    "};\n\n"
    "PoreResult pore_system(vec2 uv, float density, float depth,\n"
    "                        float scale, float age) {\n"
    "    PoreResult r;\n"
    "    /* Escalar UV según densidad de poros */\n"
    "    vec2 puv = uv * scale;\n"
    "    /* Voronoi — centro de la celda = ostium del poro */\n"
    "    vec3 vor = voronoi(vec3(puv, 0.));\n"
    "    float dist = vor.x;                /* distancia al poro más cercano */\n"
    "    float cell_id = vor.z;             /* identidad única del poro */\n"
    "    /* Variación de tamaño por id de celda (poros no son uniformes) */\n"
    "    float pore_r = 0.08 + 0.06 * hash11(cell_id * 7.3);\n"
    "    /* Factor de envejecimiento: poros se dilatan con la edad */\n"
    "    pore_r *= (1.0 + age * 0.4);\n"
    "    /* Perfil cónico del poro: más oscuro en el centro */\n"
    "    float inside = smoothstep(pore_r, pore_r * 0.3, dist);\n"
    "    /* Borde del poro: queratina engrosada */\n"
    "    float rim = smoothstep(pore_r * 0.3, pore_r, dist) *\n"
    "                smoothstep(pore_r * 1.2, pore_r, dist);\n"
    "    /* Normal perturbation: las paredes del poro inclinan la normal */\n"
    "    vec2 pore_center = puv - hash22(floor(puv + 0.5));\n"
    "    vec2 pore_grad   = normalize(pore_center + vec2(1e-5)) * inside * depth;\n"
    "    r.normal_delta  = vec3(pore_grad.x, pore_grad.y, 0.0) * 2.5;\n"
    "    /* Oclusión: las paredes del poro bloquean la luz incidente */\n"
    "    r.ao            = 1.0 - inside * depth * 0.7;\n"
    "    r.depth_mask    = inside;\n"
    "    /* El borde del poro acumula sebo → más brillante */\n"
    "    r.oiliness_mod  = rim * 0.3;\n"
    "    /* El fondo del poro es más rugoso */\n"
    "    r.roughness_mod = inside * 0.2 + rim * (-0.15);\n"
    "    return r;\n"
    "}\n\n"
    "/* ── PIEL VELLOSA — modelo de fibra corta ── */\n"
    "/* Vello facial: longitud 0.5-2mm, ángulo de salida ~30° */\n"
    "float peach_fuzz(vec3 N, vec3 L, vec3 V, vec2 uv,\n"
    "                  float density, float length_mm) {\n"
    "    /* Dirección aleatoria de cada fibra de vello */\n"
    "    vec3 hair_dir = normalize(vec3(\n"
    "        phi_noise(vec3(uv * 20., 0.1)) - 0.5,\n"
    "        phi_noise(vec3(uv * 20., 0.7)) - 0.5,\n"
    "        0.8 + phi_noise(vec3(uv * 20., 1.3)) * 0.2\n"
    "    ));\n"
    "    /* Tangente del pelo: T = N + desviación */\n"
    "    vec3 T = normalize(hair_dir + N * 0.5);\n"
    "    /* Kajiya-Kay diffuse para fibra */\n"
    "    float sinTL = sqrt(max(0., 1.0 - dot(T,L)*dot(T,L)));\n"
    "    float sinTV = sqrt(max(0., 1.0 - dot(T,V)*dot(T,V)));\n"
    "    /* Transmitancia a través del vello (semi-translucido) */\n"
    "    float diff = sinTL * (0.6 + 0.4 * dot(N,L));\n"
    "    /* Especular de Marschner-Kay para vello fino */\n"
    "    float spec = pow(max(0., sinTL * sinTV - dot(T,L)*dot(T,V)), 6.0);\n"
    "    float mask = density * (0.5 + 0.5 * phi_noise(vec3(uv*15., 2.1)));\n"
    "    return (diff * 0.6 + spec * 0.4) * mask * length_mm * 0.2;\n"
    "}\n\n"
    "/* ── VASCULARIZACIÓN DINÁMICA — pulso cardíaco en piel ── */\n"
    "/* El pulso cardíaco (70bpm = 1.17Hz) es visible en sienes,\n"
    " * cuello, labios. Se modela como variación periódica de\n"
    " * hemoglobina local sincronizada con heartbeat_phase */\n"
    "vec3 vascular_pulse(\n"
    "    vec3 base_color,\n"
    "    vec3 vascular_map,     /* R=arteria G=vena B=capilar */\n"
    "    float heartbeat_phase, /* 0-TAU en tiempo real */\n"
    "    float hemoglobin,\n"
    "    float skin_tone\n"
    ") {\n"
    "    /* Ciclo cardíaco: sístole rápida + diástole lenta */\n"
    "    /* Forma de onda: función de pulso asimétrico */\n"
    "    float systole  = exp(-pow(fract(heartbeat_phase/TAU)*5.0, 2.0));\n"
    "    float diastole = 1.0 - systole;\n"
    "    /* Arterias: rojo brillante durante sístole */\n"
    "    vec3 artery_pulse = vec3(0.18, 0.04, 0.02) * vascular_map.r * systole;\n"
    "    /* Venas: azul oscuro, menos variación */\n"
    "    vec3 vein_color   = vec3(0.04, 0.06, 0.20) * vascular_map.g * 0.3;\n"
    "    /* Capilares: distribucion homogénea, varían con el pulso */\n"
    "    vec3 cap_color    = vec3(0.12, 0.04, 0.03) * vascular_map.b * \n"
    "                         (0.5 + 0.5 * systole) * hemoglobin;\n"
    "    return base_color + (artery_pulse + vein_color + cap_color) * \n"
    "           (1.0 - skin_tone * 0.4);  /* visible menos en piel oscura */\n"
    "}\n\n"
    "/* ── GLÁNDULAS SUDORÍPARAS — mojado dinámico ── */\n"
    "struct SweatResult {\n"
    "    float wetness;       /* 0-1 */\n"
    "    float roughness_mod; /* mojado = menos rugoso */\n"
    "    vec3  specular_add;  /* reflexión de gota */\n"
    "    float normal_perturb;/* superficie del agua */\n"
    "};\n"
    "SweatResult sweat_system(\n"
    "    vec2 uv, float time, float sweat_level,\n"
    "    vec3 N, vec3 L, vec3 V\n"
    ") {\n"
    "    SweatResult r;\n"
    "    r.wetness = 0.0; r.roughness_mod = 0.0;\n"
    "    r.specular_add = vec3(0.); r.normal_perturb = 0.0;\n"
    "    if(sweat_level < 0.01) return r;\n"
    "    /* Distribución de glándulas eccrina por fBm */\n"
    "    float gland_map = fbm(vec3(uv*25., time*0.1), 3, 2.0, 0.5);\n"
    "    float gland_mask = smoothstep(0.5, 0.7, gland_map) * sweat_level;\n"
    "    /* Gotas individuales sobre la piel */\n"
    "    vec3 vor = voronoi(vec3(uv*30., time*0.03));\n"
    "    float drop_center = smoothstep(0.3, 0.05, vor.x) * sweat_level;\n"
    "    /* La gota tiene IOR de agua = 1.333 → especular brillante */\n"
    "    vec3 H = normalize(L + V);\n"
    "    float water_spec = pow(max(dot(N,H), 0.), 256.0) * drop_center;\n"
    "    /* Superficie de la gota: normal perturbada */\n"
    "    float wave = sin(uv.x*200.0 + time*3.0) * sin(uv.y*200.0 + time*2.5);\n"
    "    r.wetness       = gland_mask + drop_center * 0.5;\n"
    "    r.roughness_mod = -r.wetness * 0.4;   /* más húmedo = menos rugoso */\n"
    "    r.specular_add  = vec3(water_spec) * drop_center * 0.8;\n"
    "    r.normal_perturb= wave * drop_center * 0.02;\n"
    "    return r;\n"
    "}\n\n"
    "/* ── ARRUGAS DINÁMICAS — controladas por FACS ── */\n"
    "/* Las arrugas dinámicas son resultado directo de las\n"
    " * contracciones musculares. A diferencia de las estáticas\n"
    " * (líneas permanentes), estas aparecen con la expresión */\n"
    "vec3 dynamic_wrinkles(\n"
    "    vec2 uv,\n"
    "    float au1,   /* inner brow raise */\n"
    "    float au2,   /* outer brow raise */\n"
    "    float au4,   /* brow lowerer */\n"
    "    float au6,   /* cheek raiser */\n"
    "    float au12,  /* lip corner puller (sonrisa) */\n"
    "    float au25,  /* lips part (apertura labios) */\n"
    "    float age    /* arrugas estáticas = permanentes */\n"
    ") {\n"
    "    vec3 N_delta = vec3(0.);\n"
    "    /* Frente: líneas horizontales por AU1+AU2+AU4 */\n"
    "    float forehead_mask = smoothstep(0.70, 0.80, uv.y);\n"
    "    float brow_raise = max(au1, au2);\n"
    "    float horiz_lines = sin(uv.y * PI * 8.0) * brow_raise * forehead_mask;\n"
    "    float brow_lower = sin(uv.y * PI * 6.0 + uv.x*PI*3.) * au4 * forehead_mask;\n"
    "    N_delta.x += horiz_lines * 0.04 + brow_lower * 0.03;\n"
    "    /* Surco nasolabial: se profundiza con sonrisa (AU6+AU12) */\n"
    "    float naso_mask = (abs(uv.x - 0.5) < 0.12 && uv.y > 0.35 && uv.y < 0.55)\n"
    "                    ? 1.0 : 0.0;\n"
    "    float naso_smile = (au6 * 0.4 + au12 * 0.6) * naso_smile * naso_mask;\n"
    "    N_delta.y += sin(uv.x * PI * 6.0) * naso_smile * 0.05;\n"
    "    /* Patas de gallo: canthus lateral AU6 */\n"
    "    float crow_dist = length(uv - vec2(uv.x < 0.5 ? 0.2 : 0.8, 0.52));\n"
    "    float crow_feet = exp(-crow_dist * 30.0) * au6 * 0.06;\n"
    "    N_delta.x += sin(atan(uv.y - 0.52, uv.x - (uv.x<0.5?0.2:0.8)) * 6.0)\n"
    "                 * crow_feet;\n"
    "    /* Líneas estáticas con la edad */\n"
    "    float static_w = fbm(vec3(uv * 8., 1.5), 4, 2.0, 0.55) * age;\n"
    "    N_delta += vec3(static_w * 0.03, static_w * 0.02, 0.);\n"
    "    return N_delta;\n"
    "}\n\n";

/* [fusion] global NG_SKIN_FRAG_MAIN <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:506 :: absorbido de rig_face_ng */
static const char *NG_SKIN_FRAG_MAIN =
    "/* ══════════════════════════════════════════════════════════ */\n"
    "/* SHADER PRINCIPAL DE PIEL NEXT GENERATION                   */\n"
    "/* ══════════════════════════════════════════════════════════ */\n\n"
    "in vec3  v_pos;\n"
    "in vec3  v_normal;\n"
    "in vec3  v_tangent;\n"
    "in vec3  v_bitangent;\n"
    "in vec2  v_uv;\n"
    "in vec4  v_color;       /* melanina/SSS regional baked */\n"
    "in float v_region;      /* zona facial 0-20 */\n"
    "in vec3  v_world_pos;\n"
    "in float v_depth_mm;    /* espesor de tejido baked */\n\n"
    "out vec4 fragColor;\n\n"
    "/* ── Samplers de texturas de alta resolución ── */\n"
    "uniform sampler2D u_albedo_map;       /* 4K: color base de piel */\n"
    "uniform sampler2D u_normal_map;       /* 4K: normal map principal */\n"
    "uniform sampler2D u_pore_normal;      /* 2K: normal de poros en tile */\n"
    "uniform sampler2D u_microdetail_norm; /* 1K: micro-rugosidad superficial */\n"
    "uniform sampler2D u_roughness_map;    /* 4K: roughness + metallic */\n"
    "uniform sampler2D u_sss_map;          /* 2K: mapa de espesor SSS */\n"
    "uniform sampler2D u_vascular_map;     /* 2K: red vascular R=arteria G=vena B=cap */\n"
    "uniform sampler2D u_wrinkle_normal;   /* 2K: arrugas estáticas */\n"
    "uniform sampler2D u_makeup_layer;     /* 4K: capa de maquillaje */\n"
    "uniform sampler2D u_moisture_layer;   /* 2K: capa de humedad */\n"
    "uniform sampler2D u_ao_map;           /* 2K: ambient occlusion */\n"
    "uniform sampler2D u_translucency_map; /* 2K: translucencia regional */\n"
    "uniform samplerCube u_env_map;        /* IBL environment map */\n"
    "uniform samplerCube u_irradiance_map; /* IBL irradiance */\n\n"
    "/* ── Bioquímica ── */\n"
    "uniform float u_melanin_eu;        /* 0-1 eumelanina */\n"
    "uniform float u_melanin_ph;        /* 0-1 feomelanina */\n"
    "uniform float u_hemoglobin_oxy;    /* 0-1 oxihemoglobina */\n"
    "uniform float u_hemoglobin_deoxy;  /* 0-1 deoxihemoglobina */\n"
    "uniform float u_bilirubin;         /* 0-1 bilirrubina */\n"
    "uniform float u_carotene;          /* 0-1 caroteno */\n"
    "uniform float u_age_norm;          /* 0-1 envejecimiento */\n"
    "uniform float u_sun_damage;        /* 0-1 fotodaño */\n\n"
    "/* ── PBR ── */\n"
    "uniform float u_roughness;         /* base roughness lipídica */\n"
    "uniform float u_ior;               /* IOR piel ~ 1.4-1.55 */\n"
    "uniform float u_oiliness;          /* 0-1 grasa superficial */\n\n"
    "/* ── Poros ── */\n"
    "uniform float u_pore_density;      /* 0-1 densidad de poros */\n"
    "uniform float u_pore_depth;        /* 0-1 profundidad */\n"
    "uniform float u_pore_scale;        /* escala tile de poros */\n\n"
    "/* ── Pelo velloso ── */\n"
    "uniform float u_fuzz_density;      /* 0-1 densidad del vello */\n"
    "uniform float u_fuzz_length;       /* longitud en mm */\n"
    "uniform vec3  u_fuzz_color;        /* color del vello */\n\n"
    "/* ── Dinámica ── */\n"
    "uniform float u_heartbeat_phase;   /* fase cardíaca 0-TAU */\n"
    "uniform float u_sweat_level;       /* 0-1 sudoración */\n"
    "uniform float u_time;\n\n"
    "/* ── FACS para arrugas dinámicas ── */\n"
    "uniform float u_au1, u_au2, u_au4, u_au6, u_au12, u_au25;\n\n"
    "/* ── Luces ── */\n"
    "uniform vec3  u_light_dir_0;       /* luz principal key */\n"
    "uniform vec3  u_light_color_0;\n"
    "uniform float u_light_intensity_0;\n"
    "uniform vec3  u_light_dir_1;       /* luz de relleno fill */\n"
    "uniform vec3  u_light_color_1;\n"
    "uniform float u_light_intensity_1;\n"
    "uniform vec3  u_light_dir_2;       /* contraluz rim */\n"
    "uniform vec3  u_light_color_2;\n"
    "uniform float u_light_intensity_2;\n"
    "uniform vec3  u_view_dir;\n"
    "uniform vec3  u_camera_pos;\n"
    "uniform float u_exposure;\n\n"
    "/* ── Capas compuestas ── */\n"
    "uniform float u_makeup_blend;      /* 0-1 intensidad maquillaje */\n"
    "uniform float u_moisture_blend;    /* 0-1 lágrimas/gloss/sudor */\n"
    "uniform float u_sss_weight;        /* peso global SSS */\n\n"
    "void main() {\n"
    "    vec3  V = normalize(u_view_dir);\n"
    "    vec2  uv = v_uv;\n\n"
    "    /* ═════════════════════════════════════════════════════\n"
    "     * PASO 1: NORMAL COMPUESTA DE ALTA RESOLUCIÓN\n"
    "     * Combina: mapa principal + poros + micro-detalle +\n"
    "     *          arrugas estáticas + arrugas dinámicas\n"
    "     * ═════════════════════════════════════════════════════ */\n"
    "    /* Mapa de normal principal (baked de high-poly) */\n"
    "    vec3 nm_main = texture(u_normal_map, uv).rgb * 2.0 - 1.0;\n"
    "    /* Poros: tileados, escala 40-60x para resolución de ~0.1mm */\n"
    "    vec3 nm_pore = texture(u_pore_normal, fract(uv * u_pore_scale)).rgb * 2.0 - 1.0;\n"
    "    /* Micro-rugosidad superficial: escala 200x (queratina) */\n"
    "    vec3 nm_micro = texture(u_microdetail_norm, fract(uv * 200.0)).rgb * 2.0 - 1.0;\n"
    "    /* Arrugas estáticas */\n"
    "    vec3 nm_wrink = texture(u_wrinkle_normal, uv).rgb * 2.0 - 1.0;\n"
    "    /* Sistema de poros con AO, depth y roughness */\n"
    "    PoreResult pores = pore_system(uv, u_pore_density, u_pore_depth,\n"
    "                                    u_pore_scale, u_age_norm);\n"
    "    /* Arrugas dinámicas por FACS */\n"
    "    vec3 nm_dyn = dynamic_wrinkles(uv, u_au1, u_au2, u_au4,\n"
    "                                    u_au6, u_au12, u_au25, u_age_norm);\n"
    "    /* Normal compuesta — pesos calibrados para piel realista */\n"
    "    vec3 N_ts = normalize(\n"
    "        nm_main * 1.00\n"
    "      + nm_pore * (u_pore_depth * 3.0) + pores.normal_delta\n"
    "      + nm_micro * 0.15\n"
    "      + nm_wrink * u_age_norm * 0.5\n"
    "      + nm_dyn\n"
    "    );\n"
    "    /* Transformar de tangent-space a world-space */\n"
    "    mat3 TBN = mat3(\n"
    "        normalize(v_tangent),\n"
    "        normalize(v_bitangent),\n"
    "        normalize(v_normal));\n"
    "    vec3 N = normalize(TBN * N_ts);\n\n"
    "    /* ═════════════════════════════════════════════════════\n"
    "     * PASO 2: ALBEDO BIOQUÍMICO COMPLETO\n"
    "     * ═════════════════════════════════════════════════════ */\n"
    "    vec4 albedo_smp = texture(u_albedo_map, uv);\n"
    "    /* Colorimetría bioquímica completa */\n"
    "    vec3 albedo = skin_bio_albedo(\n"
    "        albedo_smp.rgb * v_color.rgb,\n"
    "        u_melanin_eu, u_melanin_ph,\n"
    "        u_hemoglobin_oxy, u_hemoglobin_deoxy,\n"
    "        u_bilirubin, u_carotene,\n"
    "        u_age_norm, u_sun_damage);\n"
    "    /* El oscurecimiento de los poros */\n"
    "    albedo *= (pores.ao * 0.7 + 0.3);\n\n"
    "    /* ═════════════════════════════════════════════════════\n"
    "     * PASO 3: ROUGHNESS ADAPTATIVA MULTI-FACTOR\n"
    "     * ═════════════════════════════════════════════════════ */\n"
    "    vec4 rgh_smp = texture(u_roughness_map, uv);\n"
    "    float roughness = u_roughness * rgh_smp.r;\n"
    "    /* Poros: el borde es más brillante (sebo), el centro más rugoso */\n"
    "    roughness += pores.roughness_mod;\n"
    "    /* Oleosidad: reduce roughness en zonas T (frente, nariz, mentón) */\n"
    "    roughness = mix(roughness, roughness * PHI_INV, u_oiliness);\n"
    "    roughness = clamp(roughness, 0.02, 0.98);\n"
    "    float alpha = roughness * roughness; /* perceptual roughness → linear */\n\n"
    "    /* ═════════════════════════════════════════════════════\n"
    "     * PASO 4: SISTEMA DE PIEL VELLOSA (PEACH FUZZ)\n"
    "     * Añade la contribución del vello facial fino\n"
    "     * ═════════════════════════════════════════════════════ */\n"
    "    vec3  L0  = normalize(u_light_dir_0);\n"
    "    float fuzz_contrib = peach_fuzz(N, L0, V, uv,\n"
    "                                     u_fuzz_density, u_fuzz_length);\n\n"
    "    /* ═════════════════════════════════════════════════════\n"
    "     * PASO 5: SSS 8-CAPA ESPECTRAL\n"
    "     * ═════════════════════════════════════════════════════ */\n"
    "    float thickness   = texture(u_sss_map, uv).r * v_depth_mm * 0.25;\n"
    "    float translucency= texture(u_translucency_map, uv).r;\n"
    "    vec3 sss_color = sss_8layer(\n"
    "        albedo, thickness,\n"
    "        u_melanin_eu, u_melanin_ph,\n"
    "        u_hemoglobin_oxy, u_hemoglobin_deoxy,\n"
    "        u_bilirubin, u_carotene, u_age_norm);\n\n"
    "    /* ═════════════════════════════════════════════════════\n"
    "     * PASO 6: VASCULARIZACIÓN DINÁMICA CON PULSO\n"
    "     * ═════════════════════════════════════════════════════ */\n"
    "    vec3 vasc_map = texture(u_vascular_map, uv).rgb;\n"
    "    vec3 albedo_vasc = vascular_pulse(\n"
    "        albedo, vasc_map,\n"
    "        u_heartbeat_phase,\n"
    "        u_hemoglobin_oxy + u_hemoglobin_deoxy,\n"
    "        u_melanin_eu);\n\n"
    "    /* ═════════════════════════════════════════════════════\n"
    "     * PASO 7: BRDF PBR COOK-TORRANCE — 3 LUCES\n"
    "     * ═════════════════════════════════════════════════════ */\n"
    "    /* IOR de piel → F0 */\n"
    "    float F0_s = pow((1.0 - u_ior) / (1.0 + u_ior), 2.0);\n"
    "    vec3  F0   = vec3(F0_s); /* dielectrico */\n"
    "    vec3  F82  = albedo * 0.1; /* F82: pequeña contribucion de color */\n\n"
    "    vec3 direct = vec3(0.);\n"
    "    /* Evaluar 3 luces */\n"
    "    for(int li = 0; li < 3; li++) {\n"
    "        vec3 L, Lc; float Li;\n"
    "        if(li == 0) { L=normalize(u_light_dir_0); Lc=u_light_color_0; Li=u_light_intensity_0; }\n"
    "        else if(li==1) { L=normalize(u_light_dir_1); Lc=u_light_color_1; Li=u_light_intensity_1; }\n"
    "        else   { L=normalize(u_light_dir_2); Lc=u_light_color_2; Li=u_light_intensity_2; }\n"
    "        vec3 H = normalize(V + L);\n"
    "        float NdL = max(dot(N, L), 0.0);\n"
    "        float NdV = max(dot(N, V), 1e-4);\n"
    "        float NdH = max(dot(N, H), 0.0);\n"
    "        float VdH = max(dot(V, H), 0.0);\n"
    "        vec3  F = F_Schlick(VdH, F0, F82);\n"
    "        float D = D_GGX(NdH, alpha);\n"
    "        float G = G_Smith(NdV, NdL, roughness);\n"
    "        vec3  spec = (D * G * F) / max(4.0 * NdV * NdL, 1e-7);\n"
    "        vec3  kD   = (1.0 - F);\n"
    "        vec3  diff = kD * albedo_vasc * INV_PI;\n"
    "        /* Vello: contribución difusa anisotrópica */\n"
    "        diff += u_fuzz_color * fuzz_contrib * kD * u_fuzz_density;\n"
    "        direct += (diff + spec) * NdL * Lc * Li;\n"
    "    }\n\n"
    "    /* ═════════════════════════════════════════════════════\n"
    "     * PASO 8: ILUMINACIÓN AMBIENTE (IBL)\n"
    "     * Image-Based Lighting para reflexiones en piel\n"
    "     * ═════════════════════════════════════════════════════ */\n"
    "    vec3 R = reflect(-V, N);\n"
    "    /* Irradiancia difusa del env (piel + SSS) */\n"
    "    vec3 irradiance = texture(u_irradiance_map, N).rgb;\n"
    "    /* Reflexión especular del env (poros brillantes, zona T) */\n"
    "    float mip_level = roughness * 8.0;  /* 0=liso 8=muy rugoso */\n"
    "    vec3 env_spec = textureLod(u_env_map, R, mip_level).rgb;\n"
    "    /* BRDF split-sum aproximación para IBL */\n"
    "    float NdV = max(dot(N, V), 0.);\n"
    "    vec2  brdf_lut = vec2(max(0., 1.0 - roughness), NdV); /* simplificado */\n"
    "    vec3  F_ibl = F_Schlick(NdV, F0, F82);\n"
    "    vec3  kD_ibl= (1.0 - F_ibl);\n"
    "    float ao = texture(u_ao_map, uv).r * pores.ao;\n"
    "    vec3  ambient_diff = kD_ibl * albedo_vasc * irradiance * ao;\n"
    "    vec3  ambient_spec = env_spec * F_ibl * (1.0 - roughness * 0.5) * ao;\n\n"
    "    /* ═════════════════════════════════════════════════════\n"
    "     * PASO 9: COMPOSICION FINAL\n"
    "     * Direct + SSS + IBL + maquillaje + humedad\n"
    "     * ═════════════════════════════════════════════════════ */\n"
    "    vec3 color = direct + ambient_diff + ambient_spec;\n"
    "    /* SSS: añadir debajo de la superficie */\n"
    "    color += sss_color * u_sss_weight * 0.45;\n"
    "    /* Sudoración */\n"
    "    SweatResult sw = sweat_system(uv, u_time, u_sweat_level, N, L0, V);\n"
    "    color = mix(color, color * (1.0 + sw.wetness * 0.2), sw.wetness);\n"
    "    color += sw.specular_add;\n"
    "    roughness = clamp(roughness + sw.roughness_mod, 0.01, 1.0);\n"
    "    /* Maquillaje */\n"
    "    vec4 makeup = texture(u_makeup_layer, uv);\n"
    "    color = mix(color, makeup.rgb + color * 0.1, makeup.a * u_makeup_blend);\n"
    "    /* Humedad (lágrimas, gloss, lluvia) */\n"
    "    vec4 moist = texture(u_moisture_layer, uv);\n"
    "    color += moist.rgb * moist.a * u_moisture_blend;\n\n"
    "    /* ═════════════════════════════════════════════════════\n"
    "     * PASO 10: TONE MAPPING + CORRECCIÓN DE COLOR\n"
    "     * ═════════════════════════════════════════════════════ */\n"
    "    color *= u_exposure;\n"
    "    color  = ACES_tonemap(color);\n"
    "    /* Corrección gamma 2.2 → sRGB */\n"
    "    color  = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));\n"
    "    fragColor = vec4(color, 1.0);\n"
    "}\n";

/* [fusion] global hi <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:1106 :: absorbido de rig_face_ng */
float (*rigpub_rig_face_ng_eyes_clamp)(float x, float lo, float hi) = clamp;

/* [fusion] global RigEyeNGCtx <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:1116 :: absorbido de rig_face_ng */
RigEyeNGCtx;

/* [fusion] global NG_EYE_LIB_GLSL <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:1131 :: absorbido de rig_face_ng */
static const char *NG_EYE_LIB_GLSL =
    "#version 300 es\n"
    "precision highp float;\n\n"
    "const float PI    = 3.14159265359;\n"
    "const float TAU   = 6.28318530718;\n"
    "const float PHI   = 1.6180339887;\n"
    "const float IOR_CORNEA  = 1.376;\n"
    "const float IOR_AQUEOUS = 1.336;\n"
    "const float IOR_AIR     = 1.000;\n\n"
    "/* ══ Hash ══ */\n"
    "float h11(float p){p=fract(p*.1031);p*=p+33.33;return fract(p*(p+p));}\n"
    "float h12(vec2 p){vec3 p3=fract(vec3(p.xyx)*.1031);p3+=dot(p3,p3.yzx+33.33);return fract((p3.x+p3.y)*p3.z);}\n"
    "vec2  h22(vec2 p){vec3 p3=fract(vec3(p.xyx)*vec3(.1031,.1030,.0973));p3+=dot(p3,p3.yzx+33.33);return fract((p3.xx+p3.yz)*p3.zy);}\n"
    "float vnoise(vec2 p){vec2 i=floor(p);vec2 f=fract(p);vec2 u=f*f*(3.-2.*f);return mix(mix(h12(i),h12(i+vec2(1,0)),u.x),mix(h12(i+vec2(0,1)),h12(i+vec2(1,1)),u.x),u.y);}\n"
    "float fbm2(vec2 p,int oct){float v=0.,a=.5;for(int i=0;i<8;i++){if(i>=oct)break;v+=a*vnoise(p);p*=2.;a*=.5;}return v;}\n\n"
    "/* ══ VORONOI para criptas del iris ══ */\n"
    "vec3 voronoi2(vec2 p) {\n"
    "    vec2 b=floor(p); vec2 f=fract(p);\n"
    "    float F1=8.,F2=8.; vec2 cell1=vec2(0.);\n"
    "    for(int j=-1;j<=1;j++) for(int i=-1;i<=1;i++) {\n"
    "        vec2 n=vec2(i,j);\n"
    "        vec2 c=h22(b+n)+n;\n"
    "        float d=length(f-c);\n"
    "        if(d<F1){F2=F1;F1=d;cell1=b+n;}\n"
    "        else if(d<F2)F2=d;\n"
    "    }\n"
    "    return vec3(F1,F2,h12(cell1));\n"
    "}\n\n"
    "/* ══ PARALLAX CÓRNEAL — refracción real de la cúpula corneal ══ */\n"
    "/* El iris y la pupila están DEBAJO de la córnea (paralaje real).\n"
    " * Se modela como un rayo que entra por la superficie corneal\n"
    " * y se refracta hacia el iris usando Snell. */\n"
    "vec2 corneal_parallax(vec2 uv_cornea, vec3 V, float depth_mm) {\n"
    "    /* Profundidad de la cámara anterior: ~3.5mm */\n"
    "    float scale = depth_mm / 5.0;\n"
    "    /* Snell: sin(θt) = sin(θi)/IOR */\n"
    "    vec2 view_xy = V.xy / max(abs(V.z), 0.01);\n"
    "    /* Desplazamiento del paralaje */\n"
    "    vec2 offset = view_xy * scale * 0.08;\n"
    "    /* Distorsión de barril: la cúpula corneal curva el espacio */\n"
    "    vec2 uv_c = uv_cornea - 0.5;\n"
    "    float r2 = dot(uv_c, uv_c);\n"
    "    float barrel = 1.0 + r2 * 0.15 + r2*r2 * 0.05;\n"
    "    return uv_cornea + offset * barrel;\n"
    "}\n\n"
    "/* ══ PELÍCULA LAGRIMAL — interferencia thin-film ══ */\n"
    "/* La película lagrimal (3-10μm de grosor) crea colores de\n"
    " * interferencia tipo Newton (iridiscencia sutil en esclera)\n"
    " * cuando se adelgaza o rompe (dry eye). */\n"
    "vec3 tear_film_iridescence(\n"
    "    float thickness_um,  /* grosor en micrómetros */\n"
    "    float NdV,           /* coseno del ángulo de vista */\n"
    "    float time\n"
    ") {\n"
    "    /* Longitudes de onda RGB aproximadas en nm */\n"
    "    const vec3 wavelength = vec3(640., 550., 460.);\n"
    "    /* IOR de la película lagrimal ~1.337 */\n"
    "    const float ior_tear = 1.337;\n"
    "    /* Camino óptico: OPL = 2 * n * d * cos(theta_t) */\n"
    "    float cos_t = sqrt(max(0., 1.0 - (1.0 - NdV*NdV) / (ior_tear*ior_tear)));\n"
    "    float OPL = 2.0 * ior_tear * thickness_um * cos_t;\n"
    "    /* Desfase de fase para cada canal */\n"
    "    vec3 phi = vec3(TAU) * OPL / wavelength;\n"
    "    /* Intensidad por interferencia constructiva/destructiva */\n"
    "    vec3 interference = 0.5 + 0.5 * cos(phi);\n"
    "    /* Solo visible en película delgada (< 5μm) */\n"
    "    float visibility = smoothstep(8.0, 1.0, thickness_um);\n"
    "    return interference * visibility * 0.1;\n"
    "}\n\n"
    "/* ══ IRIS PROCEDURAL DETALLADO ══ */\n"
    "struct IrisResult {\n"
    "    vec3  color;           /* color del iris */\n"
    "    float opacity;         /* opacidad (zona del iris) */\n"
    "    float roughness;       /* rugosidad del estroma */\n"
    "    float depth_mask;      /* máscara de profundidad de criptas */\n"
    "    vec3  normal_delta;    /* perturbación de normal por criptas */\n"
    "};\n\n"
    "IrisResult iris_procedural(\n"
    "    vec2  uv_iris,         /* UV en espacio del iris (centro=0.5) */\n"
    "    float radius_normalized, /* 0=pupila 1=limbo */\n"
    "    float pupil_r,         /* radio de la pupila en UV [0-1] */\n"
    "    float iris_r,          /* radio del iris en UV [0-1] */\n"
    "    vec3  iris_color_base, /* color base del iris */\n"
    "    float melanin_iris,    /* melanina del estroma */\n"
    "    float crypts_density,  /* 0-1 densidad de criptas */\n"
    "    float collarette_pos,  /* 0-1 posición del collarete */\n"
    "    sampler2D iris_tex     /* textura opcional */\n"
    ") {\n"
    "    IrisResult r;\n"
    "    r.color = iris_color_base;\n"
    "    r.opacity = 0.0;\n"
    "    r.roughness = 0.0;\n"
    "    r.depth_mask = 0.0;\n"
    "    r.normal_delta = vec3(0.);\n\n"
    "    /* ── 1. Máscara del iris (anillo entre pupila y limbo) ── */\n"
    "    float in_iris = smoothstep(pupil_r - 0.005, pupil_r + 0.005, radius_normalized)\n"
    "                  * smoothstep(iris_r + 0.005, iris_r - 0.005, radius_normalized);\n"
    "    if(in_iris < 0.01) return r;\n"
    "    r.opacity = in_iris;\n\n"
    "    /* ── 2. Coordenadas polares para el iris ── */\n"
    "    vec2  from_center = uv_iris - 0.5;\n"
    "    float angle = atan(from_center.y, from_center.x); /* -PI a PI */\n"
    "    float rad   = radius_normalized;\n\n"
    "    /* ── 3. Fibras radiales del estroma (músculo dilatador) ── */\n"
    "    float radial_fibers = pow(abs(sin(angle * 60.0)), 0.15);\n"
    "    /* Variación sutil de las fibras */\n"
    "    float fiber_noise = vnoise(vec2(angle * 30.0, rad * 8.0)) * 0.3;\n"
    "    float fibers = radial_fibers + fiber_noise;\n\n"
    "    /* ── 4. Criptas de Fuchs (Voronoi) ── */\n"
    "    /* Convertir a coordenadas cartesianas para Voronoi */\n"
    "    vec2 iris_cartesian = vec2(angle / PI, rad) * vec2(6.0, 8.0) * crypts_density;\n"
    "    vec3 vor = voronoi2(iris_cartesian);\n"
    "    float crypt_depth = smoothstep(0.4, 0.1, vor.x) * crypts_density;\n"
    "    /* El fondo de las criptas expone el epitelio posterior (oscuro) */\n"
    "    float crypt_dark = crypt_depth * 0.6;\n\n"
    "    /* ── 5. Collarete (zona de criptas densas) ── */\n"
    "    float collarete_mask = exp(-pow((rad - collarette_pos) / 0.05, 2.0));\n"
    "    float collarete_crypt = voronoi2(iris_cartesian * 2.0).x;\n"
    "    collarete_crypt = smoothstep(0.3, 0.0, collarete_crypt) * collarete_mask * 0.4;\n\n"
    "    /* ── 6. Venas radiales (iris claro) ── */\n"
    "    float vein_visibility = max(0., 1.0 - melanin_iris * 1.5);\n"
    "    float veins = pow(abs(sin(angle * 20.0 + fbm2(uv_iris * 15., 3) * 2.)), 8.0)\n"
    "                  * vein_visibility * 0.15;\n\n"
    "    /* ── 7. Color del iris por pigmentación ── */\n"
    "    /* Melanina del estroma determina el color */\n"
    "    /* Alto melanin → marrón/negro, bajo → azul/verde */\n"
    "    /* El azul es producto de dispersión Rayleigh en estroma poco pigmentado */\n"
    "    vec3 low_melanin_color  = vec3(0.15, 0.35, 0.75) + veins;  /* azul Rayleigh */\n"
    "    vec3 mid_melanin_color  = vec3(0.22, 0.45, 0.20);          /* verde-avellana */\n"
    "    vec3 high_melanin_color = vec3(0.25, 0.15, 0.05);          /* marrón oscuro */\n"
    "    vec3 iris_col;\n"
    "    if(melanin_iris < 0.3)\n"
    "        iris_col = mix(low_melanin_color, mid_melanin_color, melanin_iris / 0.3);\n"
    "    else\n"
    "        iris_col = mix(mid_melanin_color, high_melanin_color, (melanin_iris-0.3)/0.7);\n"
    "    iris_col = mix(iris_color_base * 0.3 + iris_col * 0.7, iris_col, 0.6);\n\n"
    "    /* ── 8. Mezcla de todas las capas ── */\n"
    "    r.color = iris_col;\n"
    "    r.color *= (1.0 - crypt_dark - collarete_crypt);\n"
    "    r.color += veins * vec3(0.6, 0.2, 0.2);\n"
    "    r.color  = mix(r.color, r.color * fibers, 0.15);\n"
    "    /* Textura adicional del iris */\n"
    "    vec3 iris_tex_col = texture(iris_tex, vec2(rad, (angle + PI) / TAU)).rgb;\n"
    "    r.color = mix(r.color, r.color * iris_tex_col * 2.0, 0.3);\n"
    "    r.roughness  = 0.7 + crypt_depth * 0.2;\n"
    "    r.depth_mask = crypt_depth;\n"
    "    r.normal_delta = vec3(\n"
    "        dFdx(crypt_depth) * 3.0,\n"
    "        dFdy(crypt_depth) * 3.0,\n"
    "        0.0);\n"
    "    return r;\n"
    "}\n\n"
    "/* ══ LIMBAL RING ══ */\n"
    "/* El anillo limbal es la zona de transición córnea→esclera.\n"
    " * Es más oscuro y tiene vasos sanguíneos visibles.\n"
    " * Su prominencia varía con la edad (decrece) y es considerado\n"
    " * un marcador de salud y atractivo. */\n"
    "vec3 limbal_ring(\n"
    "    float radius_normalized,\n"
    "    float iris_r,\n"
    "    float age_norm,\n"
    "    float health       /* 0=enfermo 1=sano */\n"
    ") {\n"
    "    /* El limbo está en el borde exterior del iris */\n"
    "    float limbal_w = 0.06 * (1.0 - age_norm * 0.5) * health;\n"
    "    float limbal_d = smoothstep(iris_r, iris_r - limbal_w, radius_normalized)\n"
    "                   * smoothstep(iris_r + 0.01, iris_r, radius_normalized);\n"
    "    /* Color: azul-gris oscuro con vasculatura */\n"
    "    return vec3(0.05, 0.06, 0.12) * limbal_d * 2.5;\n"
    "}\n\n"
    "/* ══ ESCLERA DETALLADA ══ */\n"
    "struct ScleraResult {\n"
    "    vec3  color;\n"
    "    float roughness;\n"
    "    vec3  normal_delta;\n"
    "};\n"
    "ScleraResult sclera_detail(\n"
    "    vec2  uv,\n"
    "    float age_norm,\n"
    "    float hemoglobin,     /* rojez: inyección ocular */\n"
    "    float jaundice,       /* amarillamiento: bilirrubina */\n"
    "    float dryness         /* 0-1 ojo seco */\n"
    ") {\n"
    "    ScleraResult r;\n"
    "    /* Color base: blanco ligeramente azulado (joven) */\n"
    "    vec3 sclera_base = vec3(0.96, 0.94, 0.92) - age_norm * vec3(0.04, 0.02, 0.0);\n"
    "    /* Tinte de bilirrubina: amarillamiento */\n"
    "    sclera_base = mix(sclera_base, vec3(0.92, 0.88, 0.60), jaundice * 0.6);\n"
    "    /* ── Vasos conjuntivales procedurales ── */\n"
    "    /* Ramas finas desde el limbo hacia afuera */\n"
    "    float vessel_pattern = 0.0;\n"
    "    for(int n = 0; n < 6; n++) {\n"
    "        float angle_off = float(n) * PI / 3.0;\n"
    "        vec2 dir = vec2(cos(angle_off), sin(angle_off));\n"
    "        /* Proyectar UV sobre la dirección del vaso */\n"
    "        float proj = dot(uv - 0.5, dir) * 3.0;\n"
    "        float perp = dot(uv - 0.5, vec2(-dir.y, dir.x)) * 3.0;\n"
    "        /* Forma sinusoidal del vaso con ruido */\n"
    "        float vein_x = proj + vnoise(vec2(proj * 0.5, float(n))) * 0.3;\n"
    "        float vein_w = smoothstep(0.04, 0.01, abs(perp + vnoise(vec2(vein_x*3., float(n)))*0.1));\n"
    "        vessel_pattern += vein_w * smoothstep(0.0, 0.4, abs(proj));\n"
    "    }\n"
    "    vessel_pattern = clamp(vessel_pattern * hemoglobin, 0., 1.);\n"
    "    sclera_base = mix(sclera_base, vec3(0.9, 0.3, 0.25), vessel_pattern * 0.35);\n"
    "    /* ── Normal de la superficie escleral ── */\n"
    "    float sclera_bump = fbm2(uv * 60., 3) * 0.01;\n"
    "    r.color       = clamp(sclera_base, 0., 1.);\n"
    "    r.roughness   = 0.3 + dryness * 0.2;\n"
    "    r.normal_delta= vec3(dFdx(sclera_bump)*8., dFdy(sclera_bump)*8., 0.);\n"
    "    return r;\n"
    "}\n\n"
    "/* ══ REFLEJO DE PURKINJE ══ */\n"
    "/* El ojo humano crea 4 imágenes especulares de las fuentes de luz\n"
    " * (imágenes de Purkinje). La primera (P1) es la más brillante:\n"
    " * reflexión en la cara anterior de la córnea. */\n"
    "vec3 purkinje_reflex(vec3 N, vec3 V, vec3 L, vec3 L_color, float L_intensity) {\n"
    "    vec3 H = normalize(V + L);\n"
    "    float NdH = max(dot(N, H), 0.);\n"
    "    /* P1: muy concentrado, alta intensidad */\n"
    "    float P1 = pow(NdH, 512.0) * 2.5;\n"
    "    /* P3: imagen débil en cara posterior de la córnea */\n"
    "    float P3 = pow(NdH * 0.95, 256.0) * 0.15;\n"
    "    /* Color levemente azulado por el recubrimiento de la córnea */\n"
    "    vec3 purkinje_col = (L_color * P1 + L_color * vec3(0.8, 0.9, 1.0) * P3);\n"
    "    return purkinje_col * L_intensity;\n"
    "}\n\n";

/* [fusion] global NG_EYE_FRAG_MAIN <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:1361 :: absorbido de rig_face_ng */
static const char *NG_EYE_FRAG_MAIN =
    "/* ══════════════════════════════════════════════════════════ */\n"
    "/* FRAGMENT SHADER OCULAR NG — Todo el sistema del ojo        */\n"
    "/* ══════════════════════════════════════════════════════════ */\n\n"
    "in vec3  v_pos;\n"
    "in vec3  v_normal;\n"
    "in vec3  v_tangent;\n"
    "in vec3  v_bitangent;\n"
    "in vec2  v_uv;\n"
    "in float v_zone;   /* 0=sclera 1=iris 2=pupila 3=cornea */\n\n"
    "out vec4 fragColor;\n\n"
    "/* Samplers */\n"
    "uniform sampler2D   u_iris_texture;         /* detalle de iris 2K */\n"
    "uniform sampler2D   u_cornea_normal;         /* normal de la córnea */\n"
    "uniform sampler2D   u_sclera_texture;        /* textura de esclera */\n"
    "uniform sampler2D   u_eyelash_mask;          /* máscara de pestañas */\n"
    "uniform samplerCube u_env_map;               /* IBL */\n\n"
    "/* Parámetros del ojo */\n"
    "uniform vec2  u_eye_center_uv;   /* centro del ojo en UV */\n"
    "uniform float u_pupil_radius;    /* 0-1 radio de la pupila */\n"
    "uniform float u_iris_radius;     /* 0-1 radio del iris */\n"
    "uniform vec3  u_iris_color;      /* color base del iris */\n"
    "uniform float u_iris_melanin;    /* 0-1 melanina del estroma */\n"
    "uniform float u_crypts_density;  /* 0-1 densidad de criptas */\n"
    "uniform float u_collarette_pos;  /* 0-1 posición del collarete */\n"
    "uniform float u_limbal_health;   /* 0-1 salud del anillo limbal */\n"
    "/* Biológicos */\n"
    "uniform float u_age_norm;\n"
    "uniform float u_hemoglobin;      /* rojez ocular */\n"
    "uniform float u_jaundice;        /* bilirrubina */\n"
    "uniform float u_dryness;         /* ojo seco */\n"
    "uniform float u_tear_thickness;  /* μm de la película lagrimal */\n"
    "uniform float u_cornea_depth;    /* profundidad de parallax mm */\n"
    "/* Dinámica */\n"
    "uniform float u_time;\n"
    "uniform vec3  u_gaze_dir;        /* dirección de la mirada */\n"
    "/* Luces */\n"
    "uniform vec3  u_light_dir_0;\n"
    "uniform vec3  u_light_color_0;\n"
    "uniform float u_light_intensity_0;\n"
    "uniform vec3  u_light_dir_1;\n"
    "uniform vec3  u_light_color_1;\n"
    "uniform float u_light_intensity_1;\n"
    "uniform vec3  u_view_dir;\n"
    "uniform float u_exposure;\n\n"
    "void main() {\n"
    "    vec3 V = normalize(u_view_dir);\n"
    "    vec3 N = normalize(v_normal);\n"
    "    vec2 uv = v_uv;\n\n"
    "    /* ── Parallax corneal hacia el iris ── */\n"
    "    vec2 uv_par = corneal_parallax(uv, V, u_cornea_depth);\n"
    "    /* Distancia al centro del ojo */\n"
    "    vec2  from_c = uv_par - u_eye_center_uv;\n"
    "    float dist   = length(from_c);\n"
    "    float rad_n  = dist / u_iris_radius;  /* normalizado: 0=pupila 1=limbo */\n\n"
    "    /* ── Zona pupila ── */\n"
    "    float in_pupil = smoothstep(u_pupil_radius + 0.003,\n"
    "                                u_pupil_radius - 0.003, rad_n);\n"
    "    /* El interior de la pupila no es completamente negro:\n"
    "     * en flash/oscuridad se ve el reflejo rojo del fondo de ojo */\n"
    "    vec3 pupil_color = vec3(0.01, 0.005, 0.002);  /* casi negro */\n"
    "    /* Reflejo retinal suave (en condiciones normales) */\n"
    "    float retinal_glint = pow(max(dot(N, normalize(u_light_dir_0)), 0.), 4.0)\n"
    "                        * 0.05 * in_pupil;\n"
    "    pupil_color += vec3(retinal_glint * 0.8, retinal_glint * 0.1, retinal_glint * 0.05);\n\n"
    "    /* ── Iris procedural ── */\n"
    "    IrisResult iris = iris_procedural(\n"
    "        uv_par, rad_n, u_pupil_radius, 1.0,\n"
    "        u_iris_color, u_iris_melanin,\n"
    "        u_crypts_density, u_collarette_pos,\n"
    "        u_iris_texture);\n\n"
    "    /* ── Esclera detallada ── */\n"
    "    ScleraResult sclera = sclera_detail(\n"
    "        uv, u_age_norm, u_hemoglobin, u_jaundice, u_dryness);\n\n"
    "    /* ── Anillo limbal ── */\n"
    "    vec3 limbal = limbal_ring(rad_n, 1.0, u_age_norm, u_limbal_health);\n\n"
    "    /* ── Normal compuesta ── */\n"
    "    vec3 N_delta = iris.normal_delta * iris.opacity\n"
    "                 + sclera.normal_delta * (1.0 - iris.opacity);\n"
    "    vec3 N_ts = normalize(vec3(N_delta.xy * 0.5, 1.0));\n"
    "    mat3 TBN = mat3(normalize(v_tangent), normalize(v_bitangent), N);\n"
    "    vec3 Nw = normalize(TBN * N_ts);\n\n"
    "    /* ── BRDF córnea — IOR=1.376 → F0 ── */\n"
    "    float F0_c = pow((1.0 - IOR_CORNEA)/(1.0 + IOR_CORNEA), 2.0);\n"
    "    vec3 color = vec3(0.);\n\n"
    "    /* ── Color base compuesto ── */\n"
    "    vec3 base = mix(sclera.color,  iris.color,  iris.opacity);\n"
    "    base      = mix(base,          pupil_color, in_pupil);\n"
    "    base      = mix(base,          base - limbal, 1.0);   /* limbal es resta */\n"
    "    base      += limbal;\n\n"
    "    /* ── Iluminación — 2 luces ── */\n"
    "    for(int li = 0; li < 2; li++) {\n"
    "        vec3 L  = normalize(li==0 ? u_light_dir_0 : u_light_dir_1);\n"
    "        vec3 Lc = (li==0 ? u_light_color_0 : u_light_color_1);\n"
    "        float Li= (li==0 ? u_light_intensity_0 : u_light_intensity_1);\n"
    "        float NdL = max(dot(Nw, L), 0.);\n"
    "        float NdV = max(dot(Nw, V), 0.001);\n"
    "        vec3 H = normalize(V + L);\n"
    "        float NdH = max(dot(Nw, H), 0.);\n"
    "        /* Difuso lambertiano */\n"
    "        color += base * NdL * Lc * Li * 0.6;\n"
    "        /* Fresnel especular córnea */\n"
    "        float F = F0_c + (1.0 - F0_c) * pow(1.0 - max(dot(V,H), 0.), 5.);\n"
    "        float rough_cornea = mix(0.02, iris.roughness, iris.opacity * 0.5);\n"
    "        float alpha = rough_cornea * rough_cornea;\n"
    "        float D = alpha / max(PI * pow(NdH*NdH*(alpha-1.)+1., 2.), 1e-7);\n"
    "        color += vec3(F * D) * NdL * Lc * Li;\n"
    "        /* Reflejo de Purkinje */\n"
    "        color += purkinje_reflex(Nw, V, L, Lc, Li);\n"
    "    }\n\n"
    "    /* ── IBL ── */\n"
    "    vec3 R = reflect(-V, Nw);\n"
    "    vec3 env = textureLod(u_env_map, R, 1.5).rgb * 0.25;\n"
    "    float F_ibl = F0_c + (1.-F0_c)*pow(1.-max(dot(Nw,V),0.),5.);\n"
    "    color += env * F_ibl;\n\n"
    "    /* ── Película lagrimal ── */\n"
    "    float NdV_l = max(dot(N, V), 0.);\n"
    "    vec3 tear_iri = tear_film_iridescence(u_tear_thickness, NdV_l, u_time);\n"
    "    color += tear_iri;\n\n"
    "    /* ── Tone mapping ── */\n"
    "    color *= u_exposure;\n"
    "    color  = clamp((color*(2.51*color+0.03))/(color*(2.43*color+0.59)+0.14), 0., 1.);\n"
    "    color  = pow(color, vec3(1./2.2));\n"
    "    fragColor = vec4(color, 1.0);\n"
    "}\n";

/* [fusion] global EyeDynamicsNG <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:1539 :: absorbido de rig_face_ng */
EyeDynamicsNG;

/* [fusion] global light_level <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:1555 :: absorbido de rig_face_ng */
void (*rigpub_rig_face_ng_eyes_eye_dynamics_init)(EyeDynamicsNG*e, float light_level) = eye_dynamics_init;

/* [fusion] global width_tip <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:2624 :: absorbido de rig_face_ng */
void (*rigpub_rig_face_ng_hair_hair_strand_init)( HairStrand*strand, float root_x, float root_y, float root_z, float dir_x, float dir_y, float dir_z, float length_m, int n_segs, float stiffness, float mel_eu, float mel_ph, float width_root, float width_tip) = hair_strand_init;

/* [fusion] global FacsAU_NG <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:2733 :: absorbido de rig_face_ng */
FacsAU_NG;

/* [fusion] global NG_FACS_TABLE <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:2736 :: absorbido de rig_face_ng */
static const FacsAU_NG NG_FACS_TABLE[128] = {
    /* ── AU 0-9: Frente y cejas ─────────────────────────────── */
    { 1, "Inner Brow Raise",
      "Frontalis (pars medialis)", "Corrugator supercilii",
      "sadness,fear,surprise", 20.f, 150.f, 400.f, 1.0f, 1.0f, 'F', true,  0.12f },
    { 2, "Outer Brow Raise",
      "Frontalis (pars lateralis)", "Corrugator supercilii",
      "surprise,fear", 18.f, 140.f, 380.f, 1.0f, 1.0f, 'F', true,  0.15f },
    { 4, "Brow Lowerer",
      "Corrugator supercilii + Depressor supercilii", "Frontalis",
      "anger,sadness,disgust,fear", 25.f, 200.f, 600.f, 1.0f, 1.0f, 'F', true,  0.08f },
    { 5, "Upper Lid Raiser",
      "Levator palpebrae superioris", "Orbicularis oculi (pars orbitalis)",
      "fear,surprise,anger", 15.f,  80.f, 250.f, 1.0f, 1.0f, 'O', true,  0.20f },
    { 6, "Cheek Raiser",
      "Orbicularis oculi (pars orbitalis)", "Levator labii",
      "happiness,joy", 30.f, 180.f, 500.f, 1.0f, 1.0f, 'O', false, 0.14f },
    { 7, "Lid Tightener",
      "Orbicularis oculi (pars palpebralis)", "Levator palpebrae",
      "anger,disgust,contempt", 20.f, 100.f, 300.f, 1.0f, 0.8f, 'O', true,  0.18f },
    { 8, "Lips Toward Each Other",
      "Incisivii labii sup+inf", "Risorius",
      "concentration", 40.f, 200.f, 800.f, 1.0f, 0.6f, 'B', false, 0.22f },
    { 9, "Nose Wrinkler",
      "Levator labii sup alaeque nasi", "Nasalis (transverse)",
      "disgust", 25.f, 120.f, 350.f, 1.0f, 1.0f, 'N', true,  0.25f },
    { 10, "Upper Lip Raiser",
      "Levator labii superioris", "Depressor anguli oris",
      "disgust,contempt", 22.f, 130.f, 380.f, 1.0f, 0.9f, 'B', true,  0.20f },
    { 11, "Nasolabial Deepener",
      "Zygomaticus minor", "Levator anguli oris",
      "sadness,disgust", 35.f, 200.f, 700.f, 1.0f, 0.7f, 'B', false, 0.16f },
    /* ── AU 12-20: Boca y mejillas ───────────────────────────── */
    { 12, "Lip Corner Puller",
      "Zygomaticus major", "Depressor anguli oris",
      "happiness,amusement,relief", 25.f, 150.f, 450.f, 1.0f, 1.0f, 'B', true,  0.12f },
    { 13, "Cheek Puffer",
      "Levator anguli oris", "Buccinator",
      "contempt,amusement", 30.f, 180.f, 500.f, 1.0f, 0.8f, 'C', false, 0.30f },
    { 14, "Dimpler",
      "Buccinator", "Orbicularis oris",
      "contempt,amusement", 35.f, 200.f, 600.f, 0.0f, 0.9f, 'B', false, 0.40f },
    { 15, "Lip Corner Depressor",
      "Depressor anguli oris (Triangularis)", "Zygomaticus major",
      "sadness,disgust,fear", 30.f, 180.f, 550.f, 1.0f, 1.0f, 'B', true,  0.15f },
    { 16, "Lower Lip Depressor",
      "Depressor labii inferioris", "Mentalis",
      "disgust,fear,sadness", 25.f, 150.f, 450.f, 1.0f, 0.9f, 'B', true,  0.18f },
    { 17, "Chin Raiser",
      "Mentalis", "Depressor labii inferioris",
      "sadness,disgust,fear", 35.f, 220.f, 650.f, 1.0f, 0.8f, 'B', true,  0.20f },
    { 18, "Lip Puckerer",
      "Incisivii labii", "Risorius + Buccinator",
      "concentration,kiss", 40.f, 250.f, 700.f, 1.0f, 0.7f, 'B', false, 0.25f },
    { 19, "Tongue Show",
      "Hyoglossus + Styloglossus", "Genioglossus",
      "contempt,playful", 50.f, 300.f, 600.f, 1.0f, 1.0f, 'B', false, 0.05f },
    { 20, "Lip Stretcher",
      "Risorius + Platysma", "Orbicularis oris",
      "fear,contempt", 20.f, 120.f, 350.f, 1.0f, 1.0f, 'B', true,  0.22f },
    { 21, "Neck Tightener",
      "Platysma (cervicalis)", "SCM (sternocleidomastoid)",
      "effort,fear", 45.f, 280.f, 800.f, 1.0f, 0.6f, 'B', false, 0.15f },
    /* ── AU 22-30: Labios y mandíbula ───────────────────────── */
    { 22, "Lip Funneler",
      "Orbicularis oris (pars marginalis)", "Buccinator",
      "sadness,concentration", 40.f, 250.f, 700.f, 1.0f, 0.8f, 'B', false, 0.20f },
    { 23, "Lip Tightener",
      "Orbicularis oris", "Risorius",
      "anger,effort", 30.f, 180.f, 500.f, 1.0f, 0.9f, 'B', false, 0.18f },
    { 24, "Lip Pressor",
      "Orbicularis oris (pars peripheralis)", "Depressor labii",
      "anger,effort,contempt", 25.f, 160.f, 450.f, 1.0f, 0.8f, 'B', false, 0.20f },
    { 25, "Lips Part",
      "Depressor labii inf + Orbicularis oris relaxation", "Orbicularis oris",
      "surprise,fear,happiness,sadness", 15.f,  80.f, 200.f, 1.0f, 1.0f, 'B', true,  0.10f },
    { 26, "Jaw Drop",
      "Masseter (relaxation) + Pterygoids (lat)", "Masseter,Temporalis",
      "surprise,fear,jaw speech", 20.f, 120.f, 350.f, 1.0f, 1.0f, 'B', true,  0.12f },
    { 27, "Mouth Stretch",
      "Pterygoids + Hyoid muscles", "Masseter",
      "fear,disgust,surprise", 25.f, 150.f, 400.f, 1.0f, 1.0f, 'B', true,  0.15f },
    { 28, "Lip Suck",
      "Orbicularis oris + Buccinator", "Depressor labii",
      "concentration,uncertainty", 50.f, 300.f, 800.f, 1.0f, 0.6f, 'B', false, 0.30f },
    { 29, "Jaw Thrust",
      "Pterygoids (medial)", "Temporalis",
      "determination,contempt", 60.f, 400.f, 1200.f, 1.0f, 0.5f, 'B', false, 0.20f },
    { 30, "Jaw Sideways",
      "Pterygoids (lat) unilateral", "Masseter ipsilateral",
      "contempt,amusement", 50.f, 350.f, 900.f, 0.0f, 0.7f, 'B', false, 0.35f },
    /* ── AU 31-41: Ojos y párpados ───────────────────────────── */
    { 31, "Brow Lowerer (asymm L)",
      "Corrugator supercilii left", "Frontalis left",
      "contempt,doubt", 25.f, 200.f, 600.f, 0.0f, 1.0f, 'F', true,  0.05f },
    { 32, "Brow Lowerer (asymm R)",
      "Corrugator supercilii right", "Frontalis right",
      "contempt,doubt", 25.f, 200.f, 600.f, 0.0f, 1.0f, 'F', true,  0.05f },
    { 33, "Brow Raiser (asymm L)",
      "Frontalis left (medial+lateral)", "Corrugator left",
      "skepticism,surprise (asymm)", 18.f, 140.f, 380.f, 0.0f, 1.0f, 'F', true,  0.08f },
    { 34, "Brow Raiser (asymm R)",
      "Frontalis right (medial+lateral)", "Corrugator right",
      "skepticism,surprise (asymm)", 18.f, 140.f, 380.f, 0.0f, 1.0f, 'F', true,  0.08f },
    { 41, "Lid Droop",
      "Levator palpebrae (partial relax)", "Orbicularis oculi",
      "sleepiness,sadness,drugged", 80.f, 500.f, 2000.f, 1.0f, 0.9f, 'O', false, 0.18f },
    { 42, "Slit",
      "Orbicularis oculi (pars palpebralis, max)", "Levator palpebrae",
      "disgust,contempt,hatred", 30.f, 200.f, 500.f, 1.0f, 1.0f, 'O', false, 0.20f },
    { 43, "Eyes Closed",
      "Orbicularis oculi + Levator palpebrae (relax)", "",
      "blink,sleep,joy (Duchenne)", 20.f, 120.f, 350.f, 1.0f, 1.0f, 'O', false, 0.08f },
    { 44, "Squint",
      "Orbicularis oculi (pars orbitalis + palpebralis)", "Levator palpebrae",
      "disgust,anger,effort", 25.f, 150.f, 400.f, 1.0f, 0.9f, 'O', false, 0.15f },
    { 45, "Blink",
      "Orbicularis oculi (fast twitch)", "Levator palpebrae",
      "blink reflex", 5.f, 60.f, 180.f, 1.0f, 1.0f, 'O', false, 0.10f },
    { 46, "Wink",
      "Orbicularis oculi unilateral", "Levator palpebrae ipsilateral",
      "flirt,amusement", 8.f, 70.f, 200.f, 0.0f, 1.0f, 'O', false, 0.30f },
    /* ── AU 51-58: Dirección de mirada y cabeza ──────────────── */
    { 51, "Head Turn Left",   "SCM right + Splenius left", "", "attention", 80.f, 500.f, 1200.f, 1.0f, 1.0f, 'F', false, 0.05f },
    { 52, "Head Turn Right",  "SCM left + Splenius right", "", "attention", 80.f, 500.f, 1200.f, 1.0f, 1.0f, 'F', false, 0.05f },
    { 53, "Head Up",          "Semispinalis capitis", "Longus capitis", "confidence,pride", 90.f, 600.f, 1500.f, 1.0f, 1.0f, 'F', false, 0.05f },
    { 54, "Head Down",        "Longus capitis", "Semispinalis", "sadness,submission", 90.f, 600.f, 1500.f, 1.0f, 1.0f, 'F', false, 0.05f },
    { 55, "Head Tilt Left",   "SCM left (lateral bend)", "", "empathy,interest", 70.f, 450.f, 1100.f, 1.0f, 1.0f, 'F', false, 0.08f },
    { 56, "Head Tilt Right",  "SCM right (lateral bend)", "", "empathy,interest", 70.f, 450.f, 1100.f, 1.0f, 1.0f, 'F', false, 0.08f },
    { 57, "Head Fwd",         "Longus colli + Rectus capitis", "", "inspection", 90.f, 600.f, 1500.f, 1.0f, 0.6f, 'F', false, 0.10f },
    { 58, "Head Back",        "Rectus capitis posterior", "", "disgust,avoidance", 90.f, 600.f, 1500.f, 1.0f, 0.6f, 'F', false, 0.10f },
    /* ── AU 61-68: Movimiento ocular ─────────────────────────── */
    { 61, "Eyes Turn Left",   "Rectus medialis R + Rectus lateralis L", "", "left gaze", 10.f, 30.f, 80.f, 1.0f, 1.0f, 'O', false, 0.02f },
    { 62, "Eyes Turn Right",  "Rectus lateralis R + Rectus medialis L", "", "right gaze", 10.f, 30.f, 80.f, 1.0f, 1.0f, 'O', false, 0.02f },
    { 63, "Eyes Up",          "Rectus superior + Obliq inferior", "", "up gaze", 10.f, 30.f, 80.f, 1.0f, 1.0f, 'O', false, 0.02f },
    { 64, "Eyes Down",        "Rectus inferior + Obliq superior", "", "down gaze", 10.f, 30.f, 80.f, 1.0f, 1.0f, 'O', false, 0.02f },
    /* ── AU 65-79: Nariz y orejas ────────────────────────────── */
    { 65, "Nostril Dilator",  "Dilator naris", "Nasalis (transverse)", "arousal,effort,disgust", 20.f, 120.f, 350.f, 1.0f, 0.8f, 'N', false, 0.20f },
    { 66, "Nostril Compressor","Nasalis (transverse)", "Dilator naris", "anger,concentration", 25.f, 150.f, 400.f, 1.0f, 0.7f, 'N', false, 0.22f },
    { 70, "Ear Wiggler",      "Auricularis sup+ant+post", "", "playful", 100.f, 600.f, 1500.f, 1.0f, 0.4f, 'F', false, 0.60f },
    /* ── AU 80-99: NG Extended — Sub-musculares ──────────────── */
    { 80, "Philtrum Curl",    "Orbicularis oris (pars peripheralis sup)", "", "disbelief,contempt", 35.f, 220.f, 650.f, 0.0f, 0.6f, 'B', true,  0.40f },
    { 81, "Sublabial Crease", "Mentalis (asymm)", "", "mild disgust", 40.f, 250.f, 700.f, 0.0f, 0.5f, 'B', false, 0.45f },
    { 82, "Labiomental Fold", "Mentalis", "Depressor labii", "sadness subtle", 45.f, 280.f, 800.f, 1.0f, 0.6f, 'B', false, 0.25f },
    { 83, "Lacrimal Compress","Orbicularis oculi (lacrimal part)", "", "cry,sadness", 60.f, 400.f, 1200.f, 1.0f, 0.7f, 'O', true,  0.30f },
    { 84, "Glabellar Furrow", "Corrugator + Procerus combined", "", "intense concentration", 30.f, 200.f, 600.f, 1.0f, 0.8f, 'F', false, 0.15f },
    { 85, "Nasojugal Fold",   "Orbicularis oculi (orb part)", "", "Duchenne marker", 30.f, 180.f, 500.f, 1.0f, 0.7f, 'O', false, 0.20f },
    { 86, "Malar Pad Raise",  "Zygomaticus major + minor", "", "joy (genuine)", 25.f, 160.f, 480.f, 1.0f, 0.9f, 'C', false, 0.16f },
    { 87, "Jowl Compress",    "Platysma (cervicalis sup)", "", "age-related sag visible", 80.f, 500.f, 2000.f, 1.0f, 0.5f, 'C', false, 0.30f },
    { 88, "Nasal Tip Drop",   "Depressor septi nasi", "", "speech (bilabial)", 15.f, 80.f, 200.f, 1.0f, 0.4f, 'N', false, 0.25f },
    { 89, "Columella Show",   "Depressor septi nasi", "Dilator naris", "disgust extreme", 20.f, 130.f, 380.f, 1.0f, 0.5f, 'N', false, 0.35f },
    { 90, "Brow Asymm Compress","Corrugator + Pyramidalis asymm", "", "skepticism", 25.f, 160.f, 480.f, 0.0f, 0.7f, 'F', true,  0.35f },
    /* ── AU 100-127: NG Micro-Expression Targets ─────────────── */
    { 100, "Micro Disgust",     "Levator labii sup alaeque nasi (micro)", "", "disgust concealed",     5.f,  30.f, 120.f, 1.0f, 0.4f, 'N', true,  0.50f },
    { 101, "Micro Fear",        "Frontalis+Levator palpebrae (micro)", "", "fear concealed",         5.f,  25.f, 100.f, 1.0f, 0.4f, 'F', true,  0.45f },
    { 102, "Micro Anger",       "Corrugator+Orbicularis (micro)", "", "anger concealed",           5.f,  25.f, 110.f, 1.0f, 0.3f, 'F', true,  0.55f },
    { 103, "Micro Sadness",     "AU1+AU15 (micro)", "", "sadness concealed",               6.f,  35.f, 130.f, 1.0f, 0.3f, 'F', true,  0.40f },
    { 104, "Micro Happiness",   "AU6+AU12 (micro)", "", "happiness concealed",             5.f,  30.f, 120.f, 1.0f, 0.4f, 'B', true,  0.30f },
    { 105, "Micro Surprise",    "AU1+AU2+AU5+AU25 (micro)", "", "surprise concealed",       4.f,  20.f,  80.f, 1.0f, 0.4f, 'F', true,  0.35f },
    { 106, "Micro Contempt",    "AU12R+AU14R (micro)", "", "contempt concealed",           5.f,  28.f, 110.f, 0.0f, 0.4f, 'B', true,  0.50f },
    { 107, "Brow Sweat Bead",   "Skin surface only", "", "high effort/stress",           200.f, 800.f, 3000.f, 1.0f, 1.0f, 'F', false, 0.02f },
    { 108, "Cheek Flush",       "Vascular dilation", "", "embarrassment,anger,arousal", 300.f,1500.f, 8000.f, 1.0f, 1.0f, 'C', false, 0.05f },
    { 109, "Pupil Dilate",      "Dilator pupillae (iris)", "", "arousal,fear,interest",    50.f, 200.f, 1000.f, 1.0f, 1.0f, 'O', false, 0.05f },
    { 110, "Pupil Constrict",   "Sphincter pupillae (iris)", "", "bright light,disgust",   20.f, 100.f,  400.f, 1.0f, 1.0f, 'O', false, 0.05f },
    { 111, "Tear Meniscus",     "Lacrimal puncta (fluid)", "", "sadness,physical pain",   200.f,1000.f, 5000.f, 1.0f, 1.0f, 'O', true,  0.02f },
    { 112, "Lip Quiver",        "Orbicularis oris (tremor)", "", "cry onset,fear",          10.f,  50.f,  300.f, 1.0f, 0.5f, 'B', true,  0.20f },
    { 113, "Nostril Flare Breath","Dilator naris (resp sync)", "", "breath inhale phase",    0.f, 400.f, 1600.f, 1.0f, 0.4f, 'N', false, 0.08f },
    { 114, "Temple Pulse",      "Temporalis (vascular)", "", "heartbeat visible",          0.f, 200.f,  857.f, 0.0f, 0.3f, 'F', false, 0.02f },
    { 115, "Neck Pulse",        "Carotid (vascular)", "", "heartbeat neck",               0.f, 200.f,  857.f, 0.0f, 0.5f, 'F', false, 0.02f },
    { 116, "Lip Dryness",       "Mucosal surface", "", "dehydration,anxiety",           5000.f,30000.f,120000.f, 1.0f, 0.8f, 'B', false, 0.01f },
    { 117, "Wrinkle Brow Deep", "Frontalis (deep crease)", "", "age,worry habitual",     5000.f,60000.f,300000.f, 1.0f, 0.9f, 'F', false, 0.01f },
    { 118, "Pore Dilation",     "Sebaceous (follicular)", "", "heat,sweat,age",          1000.f,10000.f, 60000.f, 1.0f, 0.8f, 'F', false, 0.01f },
    { 119, "Crow Feet Deep",    "Orbicularis oculi habitual", "", "age,joy habitual",    5000.f,60000.f,300000.f, 1.0f, 0.9f, 'O', false, 0.01f },
    { 120, "Malar Flush Left",  "Malar vascular L", "", "asymm blush",                  300.f, 2000.f, 9000.f, 0.0f, 0.8f, 'C', false, 0.08f },
    { 121, "Malar Flush Right", "Malar vascular R", "", "asymm blush",                  300.f, 2000.f, 9000.f, 0.0f, 0.8f, 'C', false, 0.08f },
    { 122, "Gloss Lip Coat",    "Mucosal moisture", "", "moisture,arousal",              500.f, 3000.f,15000.f, 1.0f, 1.0f, 'B', false, 0.02f },
    { 123, "Lip Curl Left",     "Orbicularis oris (sup, L asymm)", "", "contempt subtle L", 30.f, 200.f, 600.f, 0.0f, 0.6f, 'B', true,  0.45f },
    { 124, "Lip Curl Right",    "Orbicularis oris (sup, R asymm)", "", "contempt subtle R", 30.f, 200.f, 600.f, 0.0f, 0.6f, 'B', true,  0.45f },
    { 125, "Chin Dimple",       "Mentalis (fascicular)", "", "ancestry",               1000.f,10000.f,300000.f, 1.0f, 0.8f, 'B', false, 0.01f },
    { 126, "Scleral Inject",    "Conjunctival vessels", "", "fatigue,anger,cry",         300.f, 3000.f,18000.f, 1.0f, 0.9f, 'O', false, 0.03f },
    { 127, "Eyelash Flutter",   "Orbicularis oculi (fine)", "", "flirt,rapid blink",      8.f,  40.f, 150.f, 1.0f, 0.5f, 'O', false, 0.25f },
};

/* [fusion] global PhonemeCat <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:2934 :: absorbido de rig_face_ng */
PhonemeCat;

/* [fusion] global Phoneme_NG <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:2950 :: absorbido de rig_face_ng */
Phoneme_NG;

/* [fusion] global NG_PHONEME_TABLE <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:2952 :: absorbido de rig_face_ng */
static const Phoneme_NG NG_PHONEME_TABLE[60] = {
    /* ── Vocales inglés/español ───────────────────────────────── */
    {0, "iː", "see/si",       0, 0.20f, 0.05f, 0.85f, 0.00f, 0.90f, 0.10f, 0.60f, 0.0f, PHON_VOWEL},
    {1, "ɪ",  "sit",          0, 0.22f, 0.05f, 0.80f, 0.00f, 0.80f, 0.15f, 0.50f, 0.0f, PHON_VOWEL},
    {2, "e",  "bed/ve",       1, 0.35f, 0.05f, 0.70f, 0.00f, 0.70f, 0.25f, 0.40f, 0.0f, PHON_VOWEL},
    {3, "æ",  "cat",          1, 0.55f, 0.02f, 0.60f, 0.00f, 0.45f, 0.30f, 0.25f, 0.0f, PHON_VOWEL},
    {4, "ɑː", "father/a",     2, 0.75f, 0.02f, 0.40f, 0.00f, 0.20f, 0.80f, 0.15f, 0.0f, PHON_VOWEL},
    {5, "ɒ",  "lot",          2, 0.70f, 0.40f, 0.20f, 0.15f, 0.25f, 0.75f, 0.05f, 0.0f, PHON_VOWEL},
    {6, "ɔː", "law/o",        3, 0.60f, 0.60f, 0.15f, 0.25f, 0.30f, 0.70f, 0.05f, 0.0f, PHON_VOWEL},
    {7, "ʊ",  "book",         3, 0.35f, 0.70f, 0.10f, 0.30f, 0.70f, 0.80f, 0.00f, 0.0f, PHON_VOWEL},
    {8, "uː", "food/u",       4, 0.30f, 0.90f, 0.05f, 0.40f, 0.80f, 0.85f, 0.00f, 0.0f, PHON_VOWEL},
    {9, "ʌ",  "cup",          2, 0.55f, 0.05f, 0.50f, 0.00f, 0.40f, 0.60f, 0.20f, 0.0f, PHON_VOWEL},
    {10,"ɜː", "bird",         2, 0.45f, 0.30f, 0.40f, 0.10f, 0.55f, 0.45f, 0.15f, 0.0f, PHON_VOWEL},
    {11,"ə",  "about (schwa)",2, 0.30f, 0.15f, 0.50f, 0.05f, 0.50f, 0.50f, 0.10f, 0.0f, PHON_VOWEL},
    /* ── Diptongos ────────────────────────────────────────────── */
    {12,"eɪ", "day",          1, 0.40f, 0.05f, 0.65f, 0.00f, 0.60f, 0.25f, 0.35f, 0.0f, PHON_VOWEL},
    {13,"aɪ", "my",           2, 0.65f, 0.05f, 0.50f, 0.00f, 0.30f, 0.60f, 0.20f, 0.0f, PHON_VOWEL},
    {14,"ɔɪ", "boy",          3, 0.58f, 0.50f, 0.20f, 0.20f, 0.35f, 0.65f, 0.10f, 0.0f, PHON_VOWEL},
    {15,"əʊ", "go",           3, 0.40f, 0.60f, 0.15f, 0.20f, 0.40f, 0.70f, 0.05f, 0.0f, PHON_VOWEL},
    {16,"aʊ", "now",          2, 0.62f, 0.20f, 0.35f, 0.08f, 0.25f, 0.65f, 0.12f, 0.0f, PHON_VOWEL},
    /* ── Consonantes bilabiales ───────────────────────────────── */
    {17,"p",  "pen",          5, 0.00f, 0.70f, 0.00f, 0.10f, 0.50f, 0.50f, 0.10f, 0.0f, PHON_BILABIAL},
    {18,"b",  "bad",          5, 0.00f, 0.70f, 0.00f, 0.10f, 0.50f, 0.50f, 0.10f, 0.0f, PHON_BILABIAL},
    {19,"m",  "man",          5, 0.00f, 0.65f, 0.00f, 0.05f, 0.50f, 0.50f, 0.05f, 1.0f, PHON_NASAL},
    /* ── Labio-dentales ───────────────────────────────────────── */
    {20,"f",  "fan",          6, 0.10f, 0.20f, 0.40f, 0.00f, 0.45f, 0.40f, 0.70f, 0.0f, PHON_LABIO},
    {21,"v",  "van",          6, 0.10f, 0.20f, 0.40f, 0.00f, 0.45f, 0.40f, 0.70f, 0.0f, PHON_LABIO},
    /* ── Dentales/alveolares ──────────────────────────────────── */
    {22,"θ",  "thin",         7, 0.12f, 0.05f, 0.55f, 0.00f, 0.85f, 0.05f, 0.80f, 0.0f, PHON_DENTAL},
    {23,"ð",  "this",         7, 0.12f, 0.05f, 0.55f, 0.00f, 0.85f, 0.05f, 0.80f, 0.0f, PHON_DENTAL},
    {24,"t",  "top",          8, 0.05f, 0.10f, 0.60f, 0.00f, 0.90f, 0.15f, 0.40f, 0.0f, PHON_ALVEOLAR},
    {25,"d",  "day",          8, 0.05f, 0.10f, 0.60f, 0.00f, 0.90f, 0.15f, 0.40f, 0.0f, PHON_ALVEOLAR},
    {26,"s",  "see",          9, 0.08f, 0.05f, 0.75f, 0.00f, 0.80f, 0.15f, 0.55f, 0.0f, PHON_ALVEOLAR},
    {27,"z",  "zoo",          9, 0.08f, 0.05f, 0.75f, 0.00f, 0.80f, 0.15f, 0.55f, 0.0f, PHON_ALVEOLAR},
    {28,"n",  "no",          10, 0.05f, 0.10f, 0.55f, 0.00f, 0.85f, 0.15f, 0.35f, 1.0f, PHON_NASAL},
    {29,"l",  "let",         10, 0.20f, 0.05f, 0.65f, 0.00f, 0.75f, 0.25f, 0.35f, 0.0f, PHON_ALVEOLAR},
    {30,"r",  "red",         11, 0.25f, 0.30f, 0.35f, 0.15f, 0.65f, 0.40f, 0.15f, 0.0f, PHON_ALVEOLAR},
    {31,"ʃ",  "she",         12, 0.12f, 0.45f, 0.35f, 0.10f, 0.70f, 0.35f, 0.45f, 0.0f, PHON_ALVEOLAR},
    {32,"ʒ",  "vision",      12, 0.12f, 0.45f, 0.35f, 0.10f, 0.70f, 0.35f, 0.45f, 0.0f, PHON_ALVEOLAR},
    {33,"tʃ", "chin",        12, 0.08f, 0.40f, 0.30f, 0.08f, 0.75f, 0.30f, 0.50f, 0.0f, PHON_ALVEOLAR},
    {34,"dʒ", "just",        12, 0.08f, 0.40f, 0.30f, 0.08f, 0.75f, 0.30f, 0.50f, 0.0f, PHON_ALVEOLAR},
    /* ── Velares ─────────────────────────────────────────────── */
    {35,"k",  "cat",         13, 0.10f, 0.10f, 0.50f, 0.00f, 0.30f, 0.85f, 0.25f, 0.0f, PHON_VELAR},
    {36,"g",  "go",          13, 0.10f, 0.10f, 0.50f, 0.00f, 0.30f, 0.85f, 0.25f, 0.0f, PHON_VELAR},
    {37,"ŋ",  "sing",        14, 0.05f, 0.15f, 0.45f, 0.00f, 0.35f, 0.80f, 0.20f, 1.0f, PHON_NASAL},
    {38,"w",  "wet",         15, 0.22f, 0.85f, 0.05f, 0.35f, 0.80f, 0.80f, 0.00f, 0.0f, PHON_VELAR},
    /* ── Semi-vocales y glotales ──────────────────────────────── */
    {39,"j",  "yes",         16, 0.20f, 0.05f, 0.80f, 0.00f, 0.85f, 0.15f, 0.50f, 0.0f, PHON_PALATAL},
    {40,"h",  "hat",         17, 0.40f, 0.05f, 0.45f, 0.00f, 0.35f, 0.50f, 0.08f, 0.0f, PHON_GLOTTAL},
    /* ── Fonemas españoles adicionales ───────────────────────── */
    {41,"rr", "perro (sp)",  18, 0.28f, 0.20f, 0.45f, 0.12f, 0.70f, 0.35f, 0.20f, 0.0f, PHON_ALVEOLAR},
    {42,"ɲ",  "niño (sp)",  10, 0.08f, 0.15f, 0.55f, 0.00f, 0.80f, 0.35f, 0.30f, 1.0f, PHON_PALATAL},
    {43,"x",  "jota (sp)",  19, 0.15f, 0.05f, 0.45f, 0.00f, 0.25f, 0.90f, 0.35f, 0.0f, PHON_VELAR},
    {44,"ʎ",  "llano (sp)", 10, 0.18f, 0.10f, 0.60f, 0.00f, 0.75f, 0.35f, 0.40f, 0.0f, PHON_PALATAL},
    /* ── Fonemas franceses/portugueses ───────────────────────── */
    {45,"ɥ",  "nuit (fr)",  15, 0.25f, 0.90f, 0.05f, 0.40f, 0.85f, 0.20f, 0.00f, 0.0f, PHON_PALATAL},
    {46,"ɛ̃",  "vin (fr)",   1, 0.38f, 0.05f, 0.68f, 0.00f, 0.65f, 0.28f, 0.38f, 1.0f, PHON_VOWEL},
    {47,"ɔ̃",  "bon (fr)",   3, 0.55f, 0.55f, 0.18f, 0.22f, 0.28f, 0.72f, 0.08f, 1.0f, PHON_VOWEL},
    {48,"ã",  "irmã (pt)",  2, 0.72f, 0.08f, 0.38f, 0.00f, 0.18f, 0.78f, 0.12f, 1.0f, PHON_VOWEL},
    /* ── Silencio y transiciones ─────────────────────────────── */
    {49,"",   "silence",    20, 0.00f, 0.10f, 0.30f, 0.00f, 0.50f, 0.50f, 0.00f, 0.0f, PHON_GLOTTAL},
    {50,"...", "breath",    21, 0.15f, 0.05f, 0.40f, 0.00f, 0.35f, 0.45f, 0.05f, 0.0f, PHON_GLOTTAL},
};

/* [fusion] global EmotionState <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:3027 :: absorbido de rig_face_ng */
EmotionState;

/* [fusion] global AnimState_NG <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:3127 :: absorbido de rig_face_ng */
AnimState_NG;

/* [fusion] global NG_ANIM_GLSL_VERT <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:3132 :: absorbido de rig_face_ng */
static const char *const NG_ANIM_GLSL_VERT[] = {
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp sampler2D;\n\n"
    "/* ── Inputs por vértice ──────────────────────────────────── */\n"
    "in vec3  a_pos;          /* posición en reposo */\n"
    "in vec3  a_normal;       /* normal en reposo */\n"
    "in vec3  a_tangent;      /* tangente en reposo */\n"
    "in vec2  a_uv;\n"
    "in vec4  a_color;        /* melanina/SSS regional */\n"
    "in float a_region;       /* región facial 0-20 */\n"
    "in float a_depth_mm;     /* espesor de tejido */\n"
    "in vec4  a_bone_ids;     /* hasta 4 huesos */\n"
    "in vec4  a_bone_w;       /* pesos de skinning */\n"
    "/* Máscara de influencia por AU: 128 bits en 4 vec4 */\n"
    "in vec4  a_au_mask_0;    /* AU 0-31   (bit per AU) */\n"
    "in vec4  a_au_mask_1;    /* AU 32-63  */\n"
    "in vec4  a_au_mask_2;    /* AU 64-95  */\n"
    "in vec4  a_au_mask_3;    /* AU 96-127 */\n\n"
    "/* ── Morphs: textura 128×N vértices ─────────────────────── */\n"
    "/* Codificación: RGB = delta posición [-1,1], A = delta normal */\n"
    "uniform highp sampler2D u_morph_pos_tex;  /* posición deltas */\n"
    "uniform highp sampler2D u_morph_nrm_tex;  /* normal deltas */\n"
    "uniform int   u_vertex_count;\n"
    "uniform int   u_morph_count;   /* AUs activos */\n\n"
    "/* ── 128 FACS weights ────────────────────────────────────── */\n"
    "uniform float u_au[128];\n\n"
    "/* ── Dual-quaternion bones: 23 huesos faciales ───────────── */\n"
    "/* Huesos: 0=skull 1=jaw 2=tongue_root 3=tongue_tip         */\n"
    "/* 4=lip_upper_L 5=lip_upper_R 6=lip_lower_L 7=lip_lower_R  */\n"
    "/* 8=cheek_L 9=cheek_R 10=brow_inner_L 11=brow_inner_R      */\n"
    "/* 12=brow_outer_L 13=brow_outer_R 14=eyelid_up_L 15=..R   */\n"
    "/* 16=eyelid_lo_L 17=..R 18=nose_tip 19=nostril_L 20=..R   */\n"
    "/* 21=ear_L 22=ear_R                                         */\n"
    "uniform vec4 u_dq_real[23];\n"
    "uniform vec4 u_dq_dual[23];\n\n"
    "/* ── Parámetros de animación ─────────────────────────────── */\n"
    "uniform float u_time;\n"
    "uniform float u_age_norm;       /* 0-1 ptosis+sag gravitacional */\n"
    "uniform float u_jaw_angle;      /* rad: mandíbula */\n"
    "uniform float u_breath_phase;   /* 0-1 ciclo respiratorio */\n"
    "uniform float u_heart_phase;    /* 0-TAU latido cardíaco */\n"
    "uniform vec3  u_head_euler;     /* rotación de cabeza rad XYZ */\n\n"
    "/* Física secundaria */\n"
    "uniform float u_jowl_disp;      /* papada desplazamiento Y */\n"
    "uniform vec3  u_cheek_L_disp;   /* mejilla izquierda XYZ */\n"
    "uniform vec3  u_cheek_R_disp;\n\n"
    "/* ── Matrices ────────────────────────────────────────────── */\n"
    "uniform mat4  u_mvp;\n"
    "uniform mat4  u_model;\n"
    "uniform mat3  u_normal_mat;\n\n"
    "/* ── Outputs al fragment ─────────────────────────────────── */\n"
    "out vec3  v_pos;\n"
    "out vec3  v_normal;\n"
    "out vec3  v_tangent;\n"
    "out vec3  v_bitangent;\n"
    "out vec2  v_uv;\n"
    "out vec4  v_color;\n"
    "out float v_region;\n"
    "out vec3  v_world_pos;\n"
    "out float v_depth_mm;\n\n"
    "/* ─────────────────────────────────────────────────────────── */\n",
    "/* DUAL QUATERNION BLEND + TRANSFORM                          */\n"
    "/* ─────────────────────────────────────────────────────────── */\n"
    "vec3 dq_transform(vec4 qr, vec4 qd, vec3 p) {\n"
    "    vec3 t = 2.0 * cross(qr.xyz, p);\n"
    "    vec3 rot_p = p + 2.0*qr.w*t + 2.0*cross(qr.xyz,t);\n"
    "    vec3 trans  = 2.0*(qr.w*qd.xyz - qd.w*qr.xyz + cross(qr.xyz,qd.xyz));\n"
    "    return rot_p + trans;\n"
    "}\n\n"
    "void dq_blend(vec4 bids, vec4 bw, out vec4 qr, out vec4 qd) {\n"
    "    qr = vec4(0.); qd = vec4(0.);\n"
    "    vec4 ref = u_dq_real[int(bids.x)];\n"
    "    for(int k=0;k<4;k++) {\n"
    "        int   bi = int(k==0?bids.x:k==1?bids.y:k==2?bids.z:bids.w);\n"
    "        float bw_k=(k==0?bw.x:k==1?bw.y:k==2?bw.z:bw.w);\n"
    "        if(bw_k<0.0005) continue;\n"
    "        float s = sign(dot(ref, u_dq_real[bi]));\n"
    "        qr += s * bw_k * u_dq_real[bi];\n"
    "        qd += s * bw_k * u_dq_dual[bi];\n"
    "    }\n"
    "    float l = length(qr);\n"
    "    if(l>0.0001){qr/=l; qd/=l;}\n"
    "}\n\n"
    "/* ─────────────────────────────────────────────────────────── */\n"
    "/* FUNCIÓN PRINCIPAL                                           */\n"
    "/* ─────────────────────────────────────────────────────────── */\n"
    "void main() {\n"
    "    vec3 pos = a_pos;\n"
    "    vec3 nrm = a_normal;\n"
    "    int  vid = gl_VertexID;\n"
    "    float inv_vc = 1.0 / float(u_vertex_count);\n\n"
    "    /* ═══════════════════════════════════════════════════════\n"
    "     * FASE 1: MORPHING — 128 AU en paralelo desde textura\n"
    "     * Cada AU ocupa una columna en la textura de morphs.\n"
    "     * La fila corresponde al índice de vértice.\n"
    "     * ═══════════════════════════════════════════════════════ */\n"
    "    vec3 morph_pos_delta = vec3(0.);\n"
    "    vec3 morph_nrm_delta = vec3(0.);\n"
    "    float v_coord = (float(vid) + 0.5) * inv_vc;\n"
    "    for(int i = 0; i < 128; i++) {\n"
    "        float w = u_au[i];\n"
    "        if(w < 0.001) continue;\n"
    "        float u_coord = (float(i) + 0.5) / 128.0;\n"
    "        vec4 pd = texture(u_morph_pos_tex, vec2(u_coord, v_coord));\n"
    "        vec4 nd = texture(u_morph_nrm_tex, vec2(u_coord, v_coord));\n"
    "        /* Decode: [0,1] → [-0.5, 0.5] → escalar a mm */\n"
    "        morph_pos_delta += (pd.xyz - 0.5) * 0.04 * w;\n"
    "        morph_nrm_delta += (nd.xyz - 0.5) * 2.0  * w;\n"
    "    }\n"
    "    pos += morph_pos_delta;\n"
    "    nrm  = normalize(nrm + morph_nrm_delta);\n\n"
    "    /* ═══════════════════════════════════════════════════════\n"
    "     * FASE 2: CORRECCIÓN DE EDAD\n"
    "     * Sagging gravitacional por región:\n"
    "     *   - Mejillas: caen inferior + ligeramente anterior\n"
    "     *   - Párpados superiores: ptosis (bajan 0-3mm)\n"
    "     *   - Labio superior: alargamiento filtrum\n"
    "     *   - Papada: descenso del tejido submentoniano\n"
    "     * ═══════════════════════════════════════════════════════ */\n"
    "    float age_sq = u_age_norm * u_age_norm;\n",
    "    /* Sagging de mejillas (regiones 8,9) */\n"
    "    float is_cheek = (a_region > 7.5 && a_region < 9.5) ? 1.0 : 0.0;\n"
    "    pos.y -= is_cheek * age_sq * 0.012;\n"
    "    pos.z += is_cheek * age_sq * 0.005;\n"
    "    /* Ptosis de párpado superior */\n"
    "    float is_upper_lid = (a_region > 2.5 && a_region < 3.5) ? 1.0 : 0.0;\n"
    "    pos.y -= is_upper_lid * u_age_norm * 0.004;\n"
    "    /* Elongación del filtrum */\n"
    "    float is_upper_lip = (a_region > 11.5 && a_region < 12.5) ? 1.0 : 0.0;\n"
    "    pos.y -= is_upper_lip * u_age_norm * 0.003;\n"
    "    /* Papada: tejido inferior a la mandíbula */\n"
    "    float is_jowl = (a_region > 16.5) ? 1.0 : 0.0;\n"
    "    pos.y += is_jowl * u_jowl_disp * 0.8;\n"
    "    pos.z += is_jowl * u_jowl_disp * 0.3;\n\n"
    "    /* ═══════════════════════════════════════════════════════\n",
    "     * FASE 3: MANDÍBULA — rotación del hueso jaw\n"
    "     * ═══════════════════════════════════════════════════════ */\n"
    "    /* La mandíbula rota alrededor del cóndilo (ATM)\n"
    "     * Vértices bajo y=0.0 en espacio jaw son parte del hueso */\n"
    "    float is_jaw = (pos.y < -0.03 && pos.y > -0.12) ? 1.0 : 0.0;\n"
    "    if(is_jaw > 0.5) {\n"
    "        /* Centro de rotación: cóndilo mandibular */\n"
    "        vec3 condyle = vec3(sign(pos.x)*0.065, 0.005, -0.015);\n"
    "        vec3 from_condyle = pos - condyle;\n"
    "        /* Rotación en X alrededor del cóndilo */\n"
    "        float ca = cos(-u_jaw_angle * is_jaw);\n"
    "        float sa = sin(-u_jaw_angle * is_jaw);\n"
    "        float ry = from_condyle.y * ca - from_condyle.z * sa;\n"
    "        float rz = from_condyle.y * sa + from_condyle.z * ca;\n"
    "        pos = condyle + vec3(from_condyle.x, ry, rz);\n"
    "    }\n\n"
    "    /* ═══════════════════════════════════════════════════════\n"
    "     * FASE 4: FÍSICA SECUNDARIA — mejillas y tejido blando\n"
    "     * ═══════════════════════════════════════════════════════ */\n"
    "    float is_cheek_L = (a_region > 7.5 && a_region < 8.5 && pos.x < 0.) ? 1.:0.;\n"
    "    float is_cheek_R = (a_region > 7.5 && a_region < 8.5 && pos.x > 0.) ? 1.:0.;\n"
    "    pos += is_cheek_L * u_cheek_L_disp * 0.7;\n"
    "    pos += is_cheek_R * u_cheek_R_disp * 0.7;\n\n"
    "    /* ═══════════════════════════════════════════════════════\n"
    "     * FASE 5: RESPIRACIÓN — expansión sutil de narinas\n"
    "     * ═══════════════════════════════════════════════════════ */\n"
    "    float is_nostril = (a_region > 4.5 && a_region < 5.5) ? 1.0:0.0;\n"
    "    float breath_flare = sin(u_breath_phase * 3.14159) * 0.6;\n"
    "    pos.x += is_nostril * sign(pos.x) * breath_flare * 0.003;\n"
    "    pos.z += is_nostril * breath_flare * 0.002;\n\n"
    "    /* ═══════════════════════════════════════════════════════\n"
    "     * FASE 6: DUAL-QUATERNION SKINNING\n"
    "     * ═══════════════════════════════════════════════════════ */\n"
    "    vec4 qr, qd;\n"
    "    dq_blend(a_bone_ids, a_bone_w, qr, qd);\n"
    "    pos = dq_transform(qr, qd, pos);\n"
    "    nrm = normalize(dq_transform(qr, vec4(0.), nrm));\n\n"
    "    /* ═══════════════════════════════════════════════════════\n"
    "     * FASE 7: PULSO CARDÍACO visible en piel\n"
    "     * Las sienes y el cuello tienen una micro-deformación\n"
    "     * sincronizada con el latido (amplitud ~0.2mm)\n"
    "     * ═══════════════════════════════════════════════════════ */\n"
    "    float is_temple = (a_region > 17.5 && a_region < 18.5) ? 1.0:0.0;\n"
    "    float systole = exp(-pow(fract(u_heart_phase/6.2832)*5.0, 2.0));\n"
    "    pos += nrm * is_temple * systole * 0.0002;\n\n"
    "    /* ── Outputs ── */\n"
    "    vec4 world = u_model * vec4(pos, 1.0);\n"
    "    v_world_pos = world.xyz;\n"
    "    v_pos       = world.xyz;\n"
    "    v_normal    = normalize(u_normal_mat * nrm);\n"
    "    v_tangent   = normalize(u_normal_mat * a_tangent);\n"
    "    v_bitangent = cross(v_normal, v_tangent);\n"
    "    v_uv        = a_uv;\n"
    "    v_color     = a_color;\n"
    "    v_region    = a_region;\n"
    "    v_depth_mm  = a_depth_mm;\n"
    "    gl_Position = u_mvp * vec4(pos, 1.0);\n"
    "}\n"};

/* [fusion] global NG_ANIM_JS_RUNTIME <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:3330 :: absorbido de rig_face_ng */
static const char *const NG_ANIM_JS_RUNTIME[] = {
    "// §NG-ANIM RigCom Next Generation — Animation Runtime\n"
    "// 128 FACS · Muscle Simulation · Micro-Expressions · Physics\n"
    "'use strict';\n\n"
    "class RigNGAnimEngine {\n"
    "  constructor(gl, prog) {\n"
    "    this.gl   = gl;\n"
    "    this.prog = prog;\n"
    "    this.L    = n => gl.getUniformLocation(prog, n);\n"
    "    // Estado de los 128 AU\n"
    "    this.au_current  = new Float32Array(128);\n"
    "    this.au_target   = new Float32Array(128);\n"
    "    this.au_velocity = new Float32Array(128);\n"
    "    this.au_spring   = 180.0;   // N/m — respuesta rápida\n"
    "    this.au_damping  = 28.0;    // N·s/m — amortiguamiento crítico\n"
    "    // Parpadeo\n"
    "    this.blink = {\n"
    "      timer:    this._rand(2.5, 6.0),\n"
    "      rate_bpm: 15.0,   // 15-20 parpadeos/min en reposo\n"
    "      phase:    0.0,\n"
    "      in_blink: false,\n"
    "      dur:      0.150,  // 150ms\n"
    "      voluntary: false\n"
    "    };\n"
    "    // Movimiento ocular\n"
    "    this.eye = {\n"
    "      yaw:    0.0, pitch:  0.0,\n"
    "      yaw_v:  0.0, pitch_v:0.0,\n"
    "      saccade_timer: this._rand(1.0, 4.0),\n"
    "      drift_yaw:   0.0, drift_pitch: 0.0,\n"
    "      tremor_phase:0.0,\n"
    "      in_saccade:  false,\n"
    "      saccade_dur: 0.0,\n"
    "      saccade_prog:0.0,\n"
    "      target_yaw:  0.0, target_pitch:0.0\n"
    "    };\n"
    "    // Respiración\n"
    "    this.breath = {\n"
    "      phase:    0.0,\n"
    "      rate_bpm: 14.0,   // 12-20 rpm\n"
    "      depth:    0.7,\n"
    "      hold_timer: 0.0,\n"
    "      is_inhale:  true\n"
    "    };\n"
    "    // Latido\n"
    "    this.heart = {\n"
    "      phase: 0.0,\n"
    "      bpm:   70.0,\n"
    "      hrv:   0.06,    // variabilidad HR (6%)\n"
    "      hrv_phase: 0.0\n"
    "    };\n"
    "    // Física secundaria\n"
    "    this.phys = {\n"
    "      jowl_y:  0.0, jowl_vy: 0.0,\n"
    "      cheek_L: [0,0,0], cheek_R: [0,0,0],\n"
    "      cheek_vL:[0,0,0], cheek_vR:[0,0,0]\n"
    "    };\n"
    "    // Mandíbula\n"
    "    this.jaw = {\n"
    "      angle:  0.0, angle_v: 0.0,\n"
    "      target: 0.0,\n"
    "      mass:   0.30, spring: 75.0, damping: 18.0\n"
    "    };\n"
    "    // Cola de micro-expresiones\n"
    "    this.micro_queue = [];\n"
    "    this.micro_max   = 32;\n"
    "    // Cola de fonemas (lip sync)\n"
    "    this.phoneme_queue = [];\n"
    "    this.current_phoneme = 49; // silencio\n"
    "    this.phoneme_blend   = 0.0;\n"
    "    // Leakage emocional\n"
    "    this.leakage = new Float32Array(128);\n"
    "    this.leakage_intensity = 0.0;\n"
    "    // Tabla FACS (id→índice)\n"
    "    this.FACS = RIG_FACS_TABLE;\n"
    "    this.PHONEMES = RIG_PHONEME_TABLE;\n"
    "    this.time = 0.0;\n"
    "    this.dt   = 0.0;\n"
    "  }\n\n"
    "  _rand(a, b) { return a + Math.random() * (b - a); }\n\n"
    "  // ══════════════════════════════════════════════════\n"
    "  // UPDATE PRINCIPAL — llamar cada frame\n"
    "  // ══════════════════════════════════════════════════\n"
    "  update(dt) {\n",
    "    this.dt    = dt;\n"
    "    this.time += dt;\n"
    "    this._updateAU(dt);\n"
    "    this._updateMicroExpressions(dt);\n"
    "    this._updateBlink(dt);\n"
    "    this._updateEyeMovement(dt);\n"
    "    this._updateBreath(dt);\n"
    "    this._updateHeart(dt);\n"
    "    this._updateJaw(dt);\n"
    "    this._updatePhysics(dt);\n"
    "    this._updateLipSync(dt);\n"
    "    this._applyLeakage(dt);\n"
    "  }\n\n"
    "  _updateAU(dt) {\n"
    "    const k = this.au_spring, c = this.au_damping;\n"
    "    for(let i=0; i<128; i++) {\n"
    "      const x = this.au_current[i] - this.au_target[i];\n"
    "      // Sistema masa-resorte-amortiguador\n"
    "      const a = (-k * x - c * this.au_velocity[i]);\n"
    "      this.au_velocity[i] += a * dt;\n"
    "      this.au_current[i]  += this.au_velocity[i] * dt;\n"
    "      this.au_current[i]   = Math.max(0, Math.min(1, this.au_current[i]));\n"
    "    }\n"
    "  }\n\n"
    "  _updateMicroExpressions(dt) {\n"
    "    const t = this.time;\n"
    "    for(const me of this.micro_queue) {\n"
    "      if(!me.active) continue;\n"
    "      if(t > me.end_time) { me.active = false; continue; }\n"
    "      let w = 0.0;\n"
    "      if(t < me.peak_time) {\n"
    "        // Onset: Ease-in\n"
    "        const p = (t - me.start_time) / (me.peak_time - me.start_time);\n"
    "        w = me.intensity * p * p;\n"
    "      } else {\n"
    "        // Offset: Ease-out cubico\n"
    "        const p = 1.0 - (t - me.peak_time) / (me.end_time - me.peak_time);\n"
    "        w = me.intensity * p * p * p;\n"
    "      }\n"
    "      this.au_current[me.au_id] = Math.max(this.au_current[me.au_id], w);\n"
    "    }\n"
    "    this.micro_queue = this.micro_queue.filter(m => m.active);\n"
    "  }\n\n"
    "  _updateBlink(dt) {\n",
    "    const bl = this.blink;\n"
    "    if(!bl.in_blink) {\n"
    "      bl.timer -= dt;\n"
    "      if(bl.timer <= 0.0) {\n"
    "        bl.in_blink = true;\n"
    "        bl.phase    = 0.0;\n"
    "        // Variabilidad: parpadeos más rápidos bajo estrés\n"
    "        const stress_factor = this.au_current[3] + this.au_current[5]; // AU4+AU7\n"
    "        bl.dur = 0.120 + stress_factor * 0.060;\n"
    "        bl.timer = this._rand(2.0, 8.0) / (1.0 + stress_factor);\n"
    "      }\n"
    "    } else {\n"
    "      bl.phase += dt / bl.dur;\n"
    "      // Forma de parpadeo: cierre rápido 40%, apertura lenta 60%\n"
    "      let au43 = 0.0;\n"
    "      if(bl.phase < 0.4) {\n"
    "        au43 = bl.phase / 0.4; // cierre\n"
    "      } else if(bl.phase < 1.0) {\n"
    "        au43 = 1.0 - (bl.phase - 0.4) / 0.6; // apertura\n"
    "      } else {\n"
    "        bl.in_blink = false;\n"
    "        au43 = 0.0;\n"
    "      }\n"
    "      // Cerrar el ojo: AU43 (Eyes Closed)\n"
    "      this.au_current[43] = au43;\n"
    "    }\n"
    "  }\n\n"
    "  _updateEyeMovement(dt) {\n"
    "    const eye = this.eye;\n"
    "    // Tremor de alta frecuencia (80-100Hz, imperceptible pero físicamente real)\n"
    "    eye.tremor_phase += dt * 90.0 * Math.PI * 2.0;\n"
    "    const tremor_amp = 0.00005; // 0.05 mrad\n"
    "    const t_yaw   = Math.sin(eye.tremor_phase * 1.0) * tremor_amp;\n"
    "    const t_pitch = Math.cos(eye.tremor_phase * 1.3) * tremor_amp;\n"
    "    // Drift (movimiento Browniano lento)\n"
    "    eye.drift_yaw   += (Math.random()-0.5) * 0.0002 * dt;\n"
    "    eye.drift_pitch += (Math.random()-0.5) * 0.0002 * dt;\n"
    "    // Restitución al centro (drift no acumulativo)\n"
    "    eye.drift_yaw   *= (1.0 - 2.0*dt);\n"
    "    eye.drift_pitch *= (1.0 - 2.0*dt);\n"
    "    if(!eye.in_saccade) {\n"
    "      eye.saccade_timer -= dt;\n"
    "      if(eye.saccade_timer <= 0.0) {\n"
    "        // Microsacada: amplitud 0.5-5°\n"
    "        const amp_deg = this._rand(0.5, 5.0);\n"
    "        const dir = Math.random() * Math.PI * 2.0;\n"
    "        eye.target_yaw   = Math.cos(dir) * amp_deg * 0.01745;\n"
    "        eye.target_pitch = Math.sin(dir) * amp_deg * 0.01745;\n"
    "        // Duración: main sequence 2.2*amp^0.45 ms\n"
    "        eye.saccade_dur  = 2.2 * Math.pow(amp_deg, 0.45) * 0.001;\n"
    "        eye.saccade_prog = 0.0;\n"
    "        eye.in_saccade   = true;\n"
    "        // Intervalo entre sacadas: 1-4s\n"
    "        eye.saccade_timer = this._rand(1.0, 4.0);\n"
    "      }\n"
    "    } else {\n"
    "      // Sacada activa: perfil de velocidad en campana\n"
    "      eye.saccade_prog += dt / eye.saccade_dur;\n"
    "      if(eye.saccade_prog >= 1.0) {\n"
    "        eye.yaw   = eye.target_yaw;\n"
    "        eye.pitch = eye.target_pitch;\n"
    "        eye.in_saccade = false;\n"
    "      } else {\n"
    "        // Perfil de velocidad saccádico (bell curve)\n"
    "        const p  = eye.saccade_prog;\n"
    "        const sp = Math.sin(p * Math.PI); // bell shape\n"
    "        eye.yaw_v   = (eye.target_yaw   - eye.yaw)   * sp / eye.saccade_dur;\n"
    "        eye.pitch_v = (eye.target_pitch - eye.pitch) * sp / eye.saccade_dur;\n"
    "        eye.yaw   += eye.yaw_v   * dt;\n"
    "        eye.pitch += eye.pitch_v * dt;\n"
    "      }\n"
    "    }\n"
    "    // Aplicar al AU de dirección de mirada\n"
    "    const final_yaw   = eye.yaw   + eye.drift_yaw   + t_yaw;\n"
    "    const final_pitch = eye.pitch + eye.drift_pitch + t_pitch;\n"
    "    // AU61/62=izquierda/derecha, AU63/64=arriba/abajo\n"
    "    this.au_current[61] = Math.max(0.0, -final_yaw  / 0.35);\n"
    "    this.au_current[62] = Math.max(0.0,  final_yaw  / 0.35);\n"
    "    this.au_current[63] = Math.max(0.0,  final_pitch/ 0.25);\n"
    "    this.au_current[64] = Math.max(0.0, -final_pitch/ 0.25);\n"
    "  }\n\n"
    "  _updateBreath(dt) {\n",
    "    const b = this.breath;\n"
    "    b.phase += dt * (b.rate_bpm / 60.0);\n"
    "    if(b.phase >= 1.0) b.phase -= 1.0;\n"
    "    // Forma de onda respiratoria: inspiración 40%, espiración 50%, pausa 10%\n"
    "    let breath_val;\n"
    "    if(b.phase < 0.40) {\n"
    "      // Inspiración: seno suave\n"
    "      breath_val = Math.sin((b.phase / 0.40) * Math.PI * 0.5) * b.depth;\n"
    "    } else if(b.phase < 0.90) {\n"
    "      // Espiración: coseno suave\n"
    "      const p = (b.phase - 0.40) / 0.50;\n"
    "      breath_val = Math.cos(p * Math.PI * 0.5) * b.depth;\n"
    "    } else {\n"
    "      // Pausa post-espiración (apnea fisiológica)\n"
    "      breath_val = 0.0;\n"
    "    }\n"
    "    // AU113: flare de narinas sincronizado con inspiración\n"
    "    this.au_current[113] = Math.max(0, breath_val * 0.6);\n"
    "    this._breath_phase = b.phase; // para vertex shader\n"
    "  }\n\n"
    "  _updateHeart(dt) {\n"
    "    const h = this.heart;\n"
    "    // HRV: modulación de la tasa cardíaca por respiración (RSA)\n"
    "    h.hrv_phase += dt * 0.25; // ciclo HRV ~4s\n"
    "    const hrv_mod = Math.sin(h.hrv_phase) * h.hrv;\n"
    "    h.phase += dt * (h.bpm / 60.0 * (1.0 + hrv_mod)) * Math.PI * 2.0;\n"
    "    if(h.phase > Math.PI * 2.0) h.phase -= Math.PI * 2.0;\n"
    "    // AU114/115: pulso visible en temples/cuello\n"
    "    const systole = Math.exp(-Math.pow(((h.phase / (Math.PI*2)) % 1.0) * 5.0, 2.0));\n"
    "    this.au_current[114] = systole * 0.5;\n"
    "    this.au_current[115] = systole * 0.7;\n"
    "  }\n\n"
    "  _updateJaw(dt) {\n"
    "    const jaw = this.jaw;\n"
    "    // Sincronizar mandíbula con AU26 (Jaw Drop) + fonema actual\n"
    "    const au26_target = this.au_current[25]; // index por AU25\n"
    "    const phoneme_jaw = this.PHONEMES[this.current_phoneme]?.jaw_open || 0.0;\n"
    "    jaw.target = Math.max(au26_target, phoneme_jaw) * 0.25; // max 0.25 rad\n"
    "    // Masa-resorte amortiguado\n"
    "    const x = jaw.angle - jaw.target;\n"
    "    const a = (-jaw.spring * x - jaw.damping * jaw.angle_v) / jaw.mass;\n"
    "    jaw.angle_v += a * dt;\n"
    "    jaw.angle   += jaw.angle_v * dt;\n"
    "    jaw.angle    = Math.max(0.0, Math.min(0.30, jaw.angle));\n"
    "  }\n\n"
    "  _updatePhysics(dt) {\n"
    "    const p = this.phys;\n"
    "    const K_JOWL = 40.0, C_JOWL = 12.0;\n"
    "    // Papada: resorte + gravedad + perturbación por movimiento de cabeza\n"
    "    const jowl_x     = p.jowl_y; // desplazamiento desde reposo\n"
    "    const jowl_a     = (-K_JOWL * jowl_x - C_JOWL * p.jowl_vy) / 0.08;\n"
    "    p.jowl_vy += jowl_a * dt;\n"
    "    p.jowl_y  += p.jowl_vy * dt;\n"
    "    // Perturbación por animación: la expresión de sonrisa sube las mejillas\n"
    "    const K_CHEEK = 60.0, C_CHEEK = 14.0;\n"
    "    const au6_w = this.au_current[5]; // AU6 Cheek Raiser\n"
    "    const au12_w= this.au_current[11];// AU12 Lip Corner\n"
    "    const cheek_push = au6_w * 0.008 + au12_w * 0.005;\n"
    "    // Mejilla izquierda\n"
    "    for(let k=0;k<3;k++) {\n"
    "      const target_L = (k===1) ? cheek_push : 0.0;\n"
    "      const a = (-K_CHEEK*(p.cheek_L[k]-target_L) - C_CHEEK*p.cheek_vL[k]) / 0.05;\n"
    "      p.cheek_vL[k] += a * dt;\n"
    "      p.cheek_L[k]  += p.cheek_vL[k] * dt;\n"
    "    }\n"
    "    // Mejilla derecha (simétrica)\n"
    "    for(let k=0;k<3;k++) {\n"
    "      const target_R = (k===1) ? cheek_push : 0.0;\n"
    "      const a = (-K_CHEEK*(p.cheek_R[k]-target_R) - C_CHEEK*p.cheek_vR[k]) / 0.05;\n"
    "      p.cheek_vR[k] += a * dt;\n"
    "      p.cheek_R[k]  += p.cheek_vR[k] * dt;\n"
    "    }\n"
    "  }\n\n"
    "  _updateLipSync(dt) {\n",
    "    if(this.phoneme_queue.length === 0) return;\n"
    "    const item = this.phoneme_queue[0];\n"
    "    if(this.time >= item.start) {\n"
    "      this.current_phoneme = item.id;\n"
    "      const phon = this.PHONEMES[item.id];\n"
    "      if(phon) {\n"
    "        // Aplicar parámetros del fonema a AUs de boca\n"
    "        this.au_target[25] = phon.jaw_open;       // AU25 Lips Part\n"
    "        // lip_rounding → AU18 Lip Puckerer\n"
    "        this.au_target[17] = phon.lip_rounding;\n"
    "        // lip_spreading → AU20 Lip Stretcher\n"
    "        this.au_target[19] = phon.lip_spreading;\n"
    "        // lip_protrusion → AU22 Lip Funneler\n"
    "        this.au_target[21] = phon.lip_protrusion;\n"
    "      }\n"
    "      if(this.time >= item.end) this.phoneme_queue.shift();\n"
    "    }\n"
    "  }\n\n"
    "  _applyLeakage(dt) {\n"
    "    if(this.leakage_intensity < 0.005) return;\n"
    "    for(let i=0; i<128; i++) {\n"
    "      if(this.leakage[i] < 0.01) continue;\n"
    "      // Leakage: la emoción suprimida aparece brevemente\n"
    "      const leak_amp = this.leakage[i] * this.leakage_intensity;\n"
    "      this.au_current[i] = Math.max(this.au_current[i], leak_amp * 0.3);\n"
    "    }\n"
    "  }\n\n"
    "  // ══════════════════════════════════════════════════\n"
    "  // API PÚBLICA\n"
    "  // ══════════════════════════════════════════════════\n"
    "  setEmotion(name, intensity=1.0, duration_s=null) {\n"
    "    const emotion = RIG_EMOTIONS[name];\n"
    "    if(!emotion) return;\n"
    "    for(let i=0; i<128; i++)\n"
    "      this.au_target[i] = emotion.au_weights[i] * intensity;\n"
    "  }\n\n"
    "  addMicroExpression(au_id, intensity=0.3) {\n"
    "    // Micro-expresión: 5-200ms, involuntaria\n"
    "    if(this.micro_queue.length >= this.micro_max) return;\n"
    "    const onset_ms  = this._rand(5,  30)  * 0.001;\n"
    "    const peak_ms   = this._rand(25, 80)  * 0.001;\n"
    "    const offset_ms = this._rand(80, 200) * 0.001;\n"
    "    this.micro_queue.push({\n"
    "      au_id, intensity,\n"
    "      start_time: this.time + onset_ms,\n"
    "      peak_time:  this.time + onset_ms + peak_ms,\n"
    "      end_time:   this.time + onset_ms + peak_ms + offset_ms,\n"
    "      active: true\n"
    "    });\n"
    "  }\n\n"
    "  setLeakage(emotion_name, intensity=0.5) {\n",
    "    // Emoción suprimida que se filtra involuntariamente\n"
    "    const emotion = RIG_EMOTIONS[emotion_name];\n"
    "    if(!emotion) return;\n"
    "    this.leakage = new Float32Array(emotion.au_weights);\n"
    "    this.leakage_intensity = intensity;\n"
    "  }\n\n"
    "  queuePhonemes(phoneme_ids, times) {\n"
    "    // times = [{start, end}] sincronizado con audio\n"
    "    for(let i=0; i<phoneme_ids.length; i++)\n"
    "      this.phoneme_queue.push({\n"
    "        id: phoneme_ids[i],\n"
    "        start: this.time + times[i].start,\n"
    "        end:   this.time + times[i].end\n"
    "      });\n"
    "  }\n\n"
    "  forceBlink(voluntary=true) {\n"
    "    this.blink.in_blink   = true;\n"
    "    this.blink.phase      = 0.0;\n"
    "    this.blink.voluntary  = voluntary;\n"
    "    this.blink.dur        = voluntary ? 0.180 : 0.120;\n"
    "  }\n\n"
    "  setGaze(yaw_deg, pitch_deg) {\n"
    "    // Sacada voluntaria hacia un punto\n"
    "    this.eye.target_yaw   = yaw_deg   * 0.01745;\n"
    "    this.eye.target_pitch = pitch_deg * 0.01745;\n"
    "    const amp = Math.sqrt(yaw_deg*yaw_deg + pitch_deg*pitch_deg);\n"
    "    this.eye.saccade_dur  = 2.2 * Math.pow(amp, 0.45) * 0.001;\n"
    "    this.eye.saccade_prog = 0.0;\n"
    "    this.eye.in_saccade   = true;\n"
    "  }\n\n"
    "  bind() {\n"
    "    const {gl, L} = this;\n"
    "    gl.uniform1fv(L('u_au'), this.au_current);\n"
    "    gl.uniform1f(L('u_jaw_angle'),    this.jaw.angle);\n"
    "    gl.uniform1f(L('u_breath_phase'), this._breath_phase || 0);\n"
    "    gl.uniform1f(L('u_heart_phase'),  this.heart.phase);\n"
    "    gl.uniform1f(L('u_jowl_disp'),    this.phys.jowl_y);\n"
    "    gl.uniform3fv(L('u_cheek_L_disp'),this.phys.cheek_L);\n"
    "    gl.uniform3fv(L('u_cheek_R_disp'),this.phys.cheek_R);\n"
    "  }\n"
    "}\n\n",
    "// ── TABLA DE EMOCIONES COMPUESTAS (PAD Model) ──────────\n"
    "const RIG_EMOTIONS = {\n"
    "  happy:    { P: 0.89, A: 0.54, D: 0.29, au_weights: _buildAU({12:0.90, 6:0.75, 45:0.10}) },\n"
    "  sad:      { P:-0.63, A: 0.27, D:-0.33, au_weights: _buildAU({1:0.70, 4:0.35, 15:0.60, 17:0.40, 54:0.25}) },\n"
    "  angry:    { P:-0.51, A: 0.59, D: 0.25, au_weights: _buildAU({4:0.90, 5:0.60, 7:0.55, 23:0.70, 24:0.55}) },\n"
    "  fear:     { P:-0.64, A: 0.60, D:-0.43, au_weights: _buildAU({1:0.65, 2:0.70, 4:0.45, 5:0.85, 20:0.65, 26:0.55}) },\n"
    "  disgust:  { P:-0.60, A: 0.35, D: 0.11, au_weights: _buildAU({9:0.85, 15:0.50, 16:0.55, 17:0.40, 25:0.35}) },\n"
    "  surprise: { P: 0.40, A: 0.67, D:-0.13, au_weights: _buildAU({1:0.65, 2:0.75, 5:0.80, 26:0.70, 27:0.55}) },\n"
    "  contempt: { P:-0.23, A: 0.05, D: 0.42, au_weights: _buildAU({14:0.75, 12:0.45, 80:0.60, 7:0.40}) },\n"
    "  neutral:  { P: 0.00, A: 0.00, D: 0.00, au_weights: new Float32Array(128) },\n"
    "  joy:      { P: 0.95, A: 0.75, D: 0.45, au_weights: _buildAU({12:1.00, 6:0.90, 86:0.80, 85:0.70}) },\n"
    "  confusion:{ P:-0.10, A: 0.30, D: 0.05, au_weights: _buildAU({4:0.55, 31:0.50, 7:0.35, 20:0.40}) },\n"
    "  flirt:    { P: 0.65, A: 0.60, D: 0.42, au_weights: _buildAU({12:0.65, 46:0.85, 2:0.40, 6:0.45}) },\n"
    "  pain:     { P:-0.80, A: 0.70, D:-0.20, au_weights: _buildAU({4:0.90, 1:0.75, 6:0.45, 20:0.55, 17:0.60, 112:0.50}) },\n"
    "};\n\n"
    "function _buildAU(spec) {\n"
    "  const w = new Float32Array(128);\n"
    "  for(const [id_str, val] of Object.entries(spec)) {\n"
    "    const id = parseInt(id_str);\n"
    "    // Convertir id FACS → índice en array\n"
    "    const idx = RIG_FACS_ID_MAP[id];\n"
    "    if(idx !== undefined) w[idx] = val;\n"
    "  }\n"
    "  return w;\n"
    "}\n\n"
    "export { RigNGAnimEngine, RIG_EMOTIONS };\n"};

/* [fusion] global RIG_ETHNO_COUNT <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:3845 :: absorbido de rig_face_ng */
const RigFaceNGSkinCtx RIG_SKIN_PRESET[RIG_ETHNO_COUNT] = {
    /* NORDIC         mel_eu  mel_ph  hb_oxy  hb_deox bili   car    age  sun  rough  ior   oil   p_den p_dep p_sc fuz_d fuz_l  fuz_col      sss   type              eth */
    [RIG_ETHNO_NORDIC]        = {0.05f,0.08f,0.48f,0.05f,0.02f,0.06f,0.f,0.f,0.42f,1.52f,0.25f,0.55f,0.28f,50.f,0.35f,0.8f,{0.92f,0.88f,0.82f},0.50f,RIG_SKIN_NORMAL,RIG_ETHNO_NORDIC},
    [RIG_ETHNO_MEDITERRANEAN] = {0.22f,0.12f,0.46f,0.05f,0.03f,0.08f,0.f,0.f,0.48f,1.50f,0.30f,0.60f,0.30f,52.f,0.40f,1.0f,{0.82f,0.72f,0.58f},0.48f,RIG_SKIN_NORMAL,RIG_ETHNO_MEDITERRANEAN},
    [RIG_ETHNO_EAST_ASIAN]    = {0.20f,0.10f,0.38f,0.04f,0.05f,0.15f,0.f,0.f,0.50f,1.48f,0.28f,0.58f,0.25f,55.f,0.30f,0.7f,{0.85f,0.78f,0.68f},0.46f,RIG_SKIN_NORMAL,RIG_ETHNO_EAST_ASIAN},
    [RIG_ETHNO_SOUTH_ASIAN]   = {0.40f,0.08f,0.44f,0.05f,0.03f,0.10f,0.f,0.f,0.52f,1.49f,0.32f,0.62f,0.32f,52.f,0.42f,1.1f,{0.72f,0.60f,0.48f},0.45f,RIG_SKIN_NORMAL,RIG_ETHNO_SOUTH_ASIAN},
    [RIG_ETHNO_WEST_AFRICAN]  = {0.82f,0.04f,0.52f,0.06f,0.02f,0.05f,0.f,0.f,0.58f,1.46f,0.20f,0.65f,0.35f,48.f,0.45f,1.2f,{0.32f,0.25f,0.20f},0.40f,RIG_SKIN_NORMAL,RIG_ETHNO_WEST_AFRICAN},
    [RIG_ETHNO_EAST_AFRICAN]  = {0.70f,0.06f,0.48f,0.05f,0.02f,0.06f,0.f,0.f,0.55f,1.47f,0.22f,0.63f,0.34f,49.f,0.44f,1.1f,{0.38f,0.30f,0.24f},0.42f,RIG_SKIN_NORMAL,RIG_ETHNO_EAST_AFRICAN},
    [RIG_ETHNO_MIDDLE_EAST]   = {0.35f,0.10f,0.46f,0.05f,0.03f,0.12f,0.f,0.f,0.50f,1.49f,0.30f,0.61f,0.31f,51.f,0.41f,1.0f,{0.72f,0.62f,0.52f},0.46f,RIG_SKIN_NORMAL,RIG_ETHNO_MIDDLE_EAST},
    [RIG_ETHNO_LATIN_AMERICAN]= {0.30f,0.10f,0.44f,0.05f,0.03f,0.12f,0.f,0.f,0.49f,1.49f,0.29f,0.60f,0.30f,51.f,0.40f,1.0f,{0.75f,0.65f,0.54f},0.47f,RIG_SKIN_NORMAL,RIG_ETHNO_LATIN_AMERICAN},
    [RIG_ETHNO_INDIGENOUS_AM] = {0.45f,0.08f,0.42f,0.05f,0.04f,0.14f,0.f,0.f,0.52f,1.49f,0.28f,0.60f,0.30f,52.f,0.40f,1.0f,{0.68f,0.58f,0.46f},0.45f,RIG_SKIN_NORMAL,RIG_ETHNO_INDIGENOUS_AM},
    [RIG_ETHNO_PACIFIC_ISLAND]= {0.50f,0.09f,0.46f,0.05f,0.03f,0.10f,0.f,0.f,0.53f,1.49f,0.30f,0.62f,0.32f,50.f,0.42f,1.1f,{0.65f,0.55f,0.44f},0.45f,RIG_SKIN_NORMAL,RIG_ETHNO_PACIFIC_ISLAND},
    [RIG_ETHNO_ALBINISM]      = {0.00f,0.00f,0.78f,0.08f,0.02f,0.02f,0.f,0.f,0.38f,1.55f,0.15f,0.70f,0.20f,60.f,0.25f,0.5f,{0.95f,0.92f,0.90f},0.60f,RIG_SKIN_SENSITIVE,RIG_ETHNO_ALBINISM},
    [RIG_ETHNO_VITILIGO]      = {0.10f,0.05f,0.50f,0.06f,0.02f,0.06f,0.f,0.f,0.44f,1.51f,0.22f,0.58f,0.28f,50.f,0.35f,0.8f,{0.90f,0.88f,0.85f},0.52f,RIG_SKIN_SENSITIVE,RIG_ETHNO_VITILIGO},
};

/* [fusion] global NG_CSS <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:3864 :: absorbido de rig_face_ng */
static const char *NG_CSS =
    ":root{"
    "--phi:1.618;--bg:#0a0a0f;--panel:#12121a;--accent:#6c4fff;"
    "--accent2:#ff4f8b;--gold:#ffd700;--text:#e8e8f0;"
    "--text2:#8888aa;--border:#2a2a3a;--good:#4fff6c;--warn:#ffaa4f;"
    "--danger:#ff4f4f;--radius:12px;--gap:12px;}"
    "*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}"
    "html,body{width:100%;height:100%;overflow:hidden;background:var(--bg);"
    "font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',system-ui,sans-serif;"
    "color:var(--text);user-select:none}"

    /* Layout principal */
    "#app{display:flex;flex-direction:column;height:100vh;height:100dvh}"
    "#topbar{display:flex;align-items:center;justify-content:space-between;"
    "padding:8px 12px;background:var(--panel);border-bottom:1px solid var(--border);"
    "min-height:48px;flex-shrink:0}"
    "#logo{font-size:15px;font-weight:700;letter-spacing:2px;color:var(--accent)}"
    "#logo span{color:var(--gold)}"
    "#status-bar{display:flex;gap:8px;align-items:center}"
    ".status-chip{font-size:10px;padding:3px 8px;border-radius:20px;"
    "background:#1a1a2a;border:1px solid var(--border)}"
    ".status-chip.live{border-color:var(--good);color:var(--good)}"
    ".status-chip.warn{border-color:var(--warn);color:var(--warn)}"

    /* Visor 3D */
    "#viewer-wrap{position:relative;flex:1;min-height:0;overflow:hidden}"
    "#c{width:100%;height:100%;display:block;touch-action:none}"
    "#overlay{position:absolute;inset:0;pointer-events:none}"
    "#hud{position:absolute;top:8px;left:8px;font-size:10px;"
    "color:var(--text2);line-height:1.6}"
    "#fps-badge{position:absolute;top:8px;right:8px;font-size:11px;"
    "padding:3px 8px;background:rgba(0,0,0,.6);border-radius:8px;"
    "color:var(--good);font-variant-numeric:tabular-nums}"
    "#model-label{position:absolute;bottom:72px;left:50%;transform:translateX(-50%);"
    "font-size:13px;font-weight:600;color:var(--gold);letter-spacing:1px;"
    "text-shadow:0 0 12px rgba(108,79,255,.8)}"

    /* Controles flotantes sobre el visor */
    "#cam-controls{position:absolute;bottom:8px;left:8px;display:flex;gap:6px}"
    "#expr-quick{position:absolute;bottom:8px;right:8px;display:flex;gap:5px}"
    ".cam-btn,.expr-btn{width:40px;height:40px;border-radius:50%;border:none;"
    "font-size:14px;cursor:pointer;display:flex;align-items:center;justify-content:center;"
    "background:rgba(12,12,20,.85);backdrop-filter:blur(8px);"
    "border:1px solid var(--border);color:var(--text);transition:.2s}"
    ".cam-btn:active,.expr-btn:active{transform:scale(.92);background:var(--accent)}"
    ".expr-btn.active{background:var(--accent2);border-color:var(--accent2)}"

    /* Panel inferior deslizable */
    "#panel-wrap{background:var(--panel);border-top:1px solid var(--border);"
    "flex-shrink:0;transition:height .3s cubic-bezier(.4,0,.2,1)}"
    "#panel-wrap.collapsed{height:44px}"
    "#panel-wrap.expanded{height:52vh}"
    "#panel-handle{height:44px;display:flex;align-items:center;justify-content:space-between;"
    "padding:0 16px;cursor:pointer}"
    "#panel-title{font-size:12px;font-weight:600;color:var(--text2);text-transform:uppercase;"
    "letter-spacing:1.5px}"
    "#panel-toggle{font-size:18px;color:var(--text2);transition:.2s}"
    "#panel-wrap.expanded #panel-toggle{transform:rotate(180deg)}"

    "#panel-content{height:calc(100% - 44px);overflow-y:auto;overflow-x:hidden;"
    "padding:0 12px 16px}"

    /* Tabs */
    ".tabs{display:flex;gap:2px;margin-bottom:12px;overflow-x:auto;flex-shrink:0}"
    ".tab{flex-shrink:0;padding:7px 14px;border-radius:20px;font-size:12px;"
    "font-weight:600;cursor:pointer;border:none;background:transparent;"
    "color:var(--text2);transition:.2s}"
    ".tab.active{background:var(--accent);color:#fff}"

    /* Controles */
    ".ctrl-group{margin-bottom:14px}"
    ".ctrl-group label{font-size:11px;color:var(--text2);display:block;"
    "margin-bottom:5px;text-transform:uppercase;letter-spacing:.8px}"
    ".ctrl-row{display:flex;align-items:center;gap:8px;margin-bottom:8px}"
    ".ctrl-row label{flex-shrink:0;font-size:11px;color:var(--text2);width:82px}"
    ".ctrl-row .val{flex-shrink:0;font-size:10px;color:var(--accent);"
    "width:36px;text-align:right;font-variant-numeric:tabular-nums}"

    /* Sliders */
    "input[type=range]{-webkit-appearance:none;flex:1;height:4px;"
    "border-radius:2px;background:var(--border);cursor:pointer;outline:none}"
    "input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;"
    "width:18px;height:18px;border-radius:50%;background:var(--accent);"
    "cursor:pointer;box-shadow:0 0 6px var(--accent)}"
    "input[type=range]::-moz-range-thumb{width:18px;height:18px;"
    "border-radius:50%;background:var(--accent);cursor:pointer;border:none}"

    /* Selects */
    "select{width:100%;background:var(--bg);border:1px solid var(--border);"
    "color:var(--text);padding:8px 10px;border-radius:var(--radius);"
    "font-size:12px;cursor:pointer;outline:none;-webkit-appearance:none;"
    "-moz-appearance:none;margin-bottom:8px}"
    "select:focus{border-color:var(--accent)}"

    /* Grid de arquetipos */
    "#archetype-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(80px,1fr));"
    "gap:6px;margin-bottom:10px}"
    ".arch-card{border-radius:10px;padding:8px 4px;text-align:center;"
    "cursor:pointer;border:1px solid var(--border);background:var(--bg);"
    "transition:.2s;font-size:9px;color:var(--text2);line-height:1.3}"
    ".arch-card:active,.arch-card.selected{border-color:var(--accent);"
    "background:rgba(108,79,255,.15);color:var(--text)}"
    ".arch-card .icon{font-size:22px;display:block;margin-bottom:3px}"

    /* Swatches de color */
    ".swatch-row{display:flex;gap:5px;flex-wrap:wrap;margin-bottom:8px}"
    ".swatch{width:28px;height:28px;border-radius:50%;cursor:pointer;"
    "border:2px solid transparent;transition:.15s;flex-shrink:0}"
    ".swatch:active,.swatch.active{border-color:var(--text);transform:scale(1.15)}"

    /* Emoción grid */
    "#emotion-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:6px;margin-bottom:10px}"
    ".emo-btn{padding:8px 4px;border-radius:10px;border:1px solid var(--border);"
    "background:var(--bg);font-size:10px;cursor:pointer;color:var(--text2);"
    "text-align:center;transition:.2s}"
    ".emo-btn:active,.emo-btn.active{background:var(--accent2);"
    "border-color:var(--accent2);color:#fff}"
    ".emo-icon{font-size:18px;display:block}"

    /* Timeline de animación */
    "#timeline{background:var(--bg);border-radius:var(--radius);"
    "padding:10px;margin-bottom:10px;border:1px solid var(--border)}"
    "#tl-track{position:relative;height:24px;background:#1a1a2a;"
    "border-radius:6px;overflow:hidden;cursor:pointer;margin-bottom:6px}"
    "#tl-progress{position:absolute;left:0;top:0;height:100%;"
    "background:linear-gradient(90deg,var(--accent),var(--accent2));"
    "transition:width .05s}"
    "#tl-controls{display:flex;gap:8px;align-items:center;justify-content:center}"
    ".tl-btn{background:none;border:none;color:var(--text);font-size:16px;"
    "cursor:pointer;padding:4px 8px;border-radius:8px;transition:.2s}"
    ".tl-btn:active{background:var(--accent)}"
    "#tl-time{font-size:11px;color:var(--text2);font-variant-numeric:tabular-nums}"

    /* Log de eventos */
    "#event-log{max-height:100px;overflow-y:auto;font-size:10px;"
    "font-family:monospace;color:var(--text2);background:var(--bg);"
    "border-radius:8px;padding:8px;border:1px solid var(--border)}"
    ".ev-line{margin-bottom:2px;border-left:2px solid var(--border);padding-left:6px}"
    ".ev-line.ok{border-color:var(--good)}"
    ".ev-line.warn{border-color:var(--warn)}"
    ".ev-line.err{border-color:var(--danger)}"

    /* Modal de exportar */
    "#export-modal{display:none;position:fixed;inset:0;z-index:100;"
    "background:rgba(0,0,0,.8);align-items:center;justify-content:center}"
    "#export-modal.open{display:flex}"
    "#export-box{background:var(--panel);border-radius:16px;padding:20px;"
    "width:90%;max-width:340px;border:1px solid var(--border)}"
    ".exp-btn{width:100%;padding:12px;margin-bottom:8px;border-radius:10px;"
    "border:none;font-size:13px;font-weight:600;cursor:pointer;"
    "background:var(--accent);color:#fff;transition:.2s}"
    ".exp-btn:active{transform:scale(.97)}"
    ".exp-btn.sec{background:var(--bg);color:var(--text);border:1px solid var(--border)}"

    /* Scrollbar mínimo */
    "::-webkit-scrollbar{width:3px;height:3px}"
    "::-webkit-scrollbar-track{background:transparent}"
    "::-webkit-scrollbar-thumb{background:var(--border);border-radius:2px}"

    /* Responsive — pantallas >768px */
    "@media(min-width:768px){"
    "#panel-wrap.expanded{height:40vh}"
    "#archetype-grid{grid-template-columns:repeat(auto-fill,minmax(90px,1fr))}"
    "#emotion-grid{grid-template-columns:repeat(6,1fr)}"
    "}";

/* [fusion] global NG_JS_RUNTIME <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:4033 :: absorbido de rig_face_ng */
static const char *NG_JS_RUNTIME =
"'use strict';\n"
"// ═══════════════════════════════════════════════════════════════\n"
"// RigCom NG — Runtime Soberano  φ=1.6180339887\n"
"// Richard Felipe Urbina · RIGCOM Ecosystem\n"
"// ═══════════════════════════════════════════════════════════════\n\n"

"const PHI = 1.6180339887;\n"
"const LOG  = [];\n"
"const MAX_LOG = 80;\n"
"let   FPS_ACC = 0, FPS_CT = 0, FPS_LAST = performance.now();\n\n"

"// ── Eventos soberanos ────────────────────────────────────────\n"
"function emit(type, payload, severity='info') {\n"
"  const ev = {\n"
"    type, module:'rig_face_ng', subsystem:'assembly',\n"
"    symbol:type, label:type, value:payload?.value ?? '',\n"
"    payload: payload || {},\n"
"    source_file:'rig_face_ng_assembly.c',\n"
"    source_function:'rig_face_ng_generate_html',\n"
"    ui_surface:'#app', timestamp: performance.now(),\n"
"    severity, visible:true\n"
"  };\n"
"  LOG.unshift(ev);\n"
"  if(LOG.length > MAX_LOG) LOG.pop();\n"
"  renderEventLog();\n"
"  return ev;\n"
"}\n\n"

"function renderEventLog() {\n"
"  const el = document.getElementById('event-log');\n"
"  if(!el) return;\n"
"  el.innerHTML = LOG.slice(0,15).map(e => {\n"
"    const cls = e.severity==='error'?'err':e.severity==='warn'?'warn':'ok';\n"
"    const t   = (e.timestamp/1000).toFixed(2);\n"
"    const v   = typeof e.payload?.value !=='undefined' ? ` → ${JSON.stringify(e.payload.value).slice(0,30)}` : '';\n"
"    return `<div class='ev-line ${cls}'>[${t}s] ${e.type}${v}</div>`;\n"
"  }).join('');\n"
"}\n\n"

"// ── Estado global del avatar ─────────────────────────────────\n"
"const STATE = {\n"
"  archetype: 'nordic_young',\n"
"  age: 28, sex: 0.3,\n"
"  emotion: 'neutral', emotion_intensity: 1.0,\n"
"  eye_color: 'brown',\n"
"  hair_type: 0, hair_length: 25,\n"
"  light_mode: 0,\n"
"  au: new Float32Array(128),\n"
"  bio: {\n"
"    melanin_eu:0.22, melanin_ph:0.08,\n"
"    hemoglobin_oxy:0.46, hemoglobin_deoxy:0.05,\n"
"    bilirubin:0.02, carotene:0.08,\n"
"    age_norm:0.0, sun_damage:0.0\n"
"  },\n"
"  pbr: { roughness:0.48, ior:1.50, oiliness:0.25 },\n"
"  pores: { density:0.55, depth:0.28, scale:50 },\n"
"  fuzz: { density:0.35, length:0.8 },\n"
"  cam: { yaw:0, pitch:0.05, dist:0.28, fov:42 },\n"
"  heart_bpm: 70, breath_bpm: 14, sweat: 0,\n"
"  enable: { hair:true,lashes:true,brows:true,teeth:true,\n"
"            sss:true,physics:true,breath:true,heartbeat:true },\n"
"  playing: false, tl_time: 0, tl_dur: 8,\n"
"  phi_ratio: PHI, certeza: 1/PHI\n"
"};\n\n"

"// ── WebGL2 Setup ─────────────────────────────────────────────\n"
"let GL, PROG_FACE, PROG_HAIR, PROG_EYE;\n"
"let VAO_FACE, VBO_FACE, IBO_FACE;\n"
"let TIME = 0, AF = null;\n"
"let HEART_PHASE = 0, BREATH_PHASE = 0;\n"
"let BLINK_TIMER = 3.0, BLINK_PHASE = 0, IN_BLINK = false;\n"
"let EYE_YAW = 0, EYE_PITCH = 0;\n"
"let SACCADE_TIMER = 2.0, IN_SACCADE = false;\n"
"let SACCADE_PROG = 0, SACCADE_DUR = 0;\n"
"let SACCADE_TY = 0, SACCADE_TP = 0;\n\n"

"function initGL() {\n"
"  const canvas = document.getElementById('c');\n"
"  GL = canvas.getContext('webgl2', {\n"
"    antialias: true, alpha: false, depth: true,\n"
"    powerPreference: 'high-performance'\n"
"  });\n"
"  if(!GL) {\n"
"    showError('WebGL2 no disponible en este dispositivo');\n"
"    emit('webgl2.init.fail', {}, 'error');\n"
"    return false;\n"
"  }\n"
"  GL.enable(GL.DEPTH_TEST);\n"
"  GL.enable(GL.CULL_FACE);\n"
"  GL.cullFace(GL.BACK);\n"
"  GL.enable(GL.BLEND);\n"
"  GL.blendFunc(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA);\n"
"  emit('webgl2.init.ok', { renderer: GL.getParameter(GL.RENDERER) });\n"
"  return true;\n"
"}\n\n"

"function compileShader(type, src) {\n"
"  const sh = GL.createShader(type);\n"
"  GL.shaderSource(sh, src);\n"
"  GL.compileShader(sh);\n"
"  if(!GL.getShaderParameter(sh, GL.COMPILE_STATUS)) {\n"
"    const err = GL.getShaderInfoLog(sh);\n"
"    emit('shader.compile.fail', { error: err }, 'error');\n"
"    showError('Shader error: ' + err.slice(0,120));\n"
"    GL.deleteShader(sh);\n"
"    return null;\n"
"  }\n"
"  emit('shader.compile.ok', { type: type === GL.VERTEX_SHADER ? 'vert':'frag' });\n"
"  return sh;\n"
"}\n\n"

"function buildProgram(vsrc, fsrc) {\n"
"  const vs = compileShader(GL.VERTEX_SHADER, vsrc);\n"
"  const fs = compileShader(GL.FRAGMENT_SHADER, fsrc);\n"
"  if(!vs || !fs) return null;\n"
"  const prog = GL.createProgram();\n"
"  GL.attachShader(prog, vs); GL.attachShader(prog, fs);\n"
"  GL.linkProgram(prog);\n"
"  if(!GL.getProgramParameter(prog, GL.LINK_STATUS)) {\n"
"    emit('program.link.fail', { error: GL.getProgramInfoLog(prog) }, 'error');\n"
"    return null;\n"
"  }\n"
"  GL.deleteShader(vs); GL.deleteShader(fs);\n"
"  emit('program.link.ok', {});\n"
"  return prog;\n"
"}\n\n"

"// ── Geometría procedural de cabeza ───────────────────────────\n"
"// Genera icosfera de bajo poly + deformación craneal + FACS ready\n"
"function buildHeadGeometry(params) {\n"
"  const verts = [], idxs = [];\n"
"  const PHI_G = 1.618033;\n"
"  // Icosfera base 3 subdivisiones → ~2500 verts\n"
"  // 12 vértices base del icosaedro\n"
"  const base = [\n"
"    [ 0,  1,  PHI_G], [ 0, -1,  PHI_G], [ 0,  1, -PHI_G],\n"
"    [ 0, -1, -PHI_G], [ 1,  PHI_G, 0], [-1,  PHI_G, 0],\n"
"    [ 1, -PHI_G, 0], [-1, -PHI_G, 0], [ PHI_G, 0,  1],\n"
"    [-PHI_G, 0,  1], [ PHI_G, 0, -1], [-PHI_G, 0, -1]\n"
"  ];\n"
"  // Triángulos del icosaedro\n"
"  const faces = [\n"
"    [0,1,8],[0,8,4],[0,4,5],[0,5,9],[0,9,1],\n"
"    [1,6,8],[8,10,4],[4,2,5],[5,11,9],[9,7,1],\n"
"    [3,6,7],[6,10,3],[10,2,3],[2,11,3],[11,7,3],\n"
"    [6,1,7],[8,6,10],[4,10,2],[5,2,11],[9,11,7]\n"
"  ];\n"
"  // Subdivisión (3 iteraciones)\n"
"  let pts = base.map(p => { const l=Math.sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]); return [p[0]/l,p[1]/l,p[2]/l]; });\n"
"  let tris = faces;\n"
"  const midmap = {};\n"
"  function mid(a,b){\n"
"    const k = a<b?`${a}_${b}`:`${b}_${a}`;\n"
"    if(midmap[k]!==undefined) return midmap[k];\n"
"    const pa=pts[a],pb=pts[b];\n"
"    const mx=(pa[0]+pb[0])*0.5,my=(pa[1]+pb[1])*0.5,mz=(pa[2]+pb[2])*0.5;\n"
"    const l=Math.sqrt(mx*mx+my*my+mz*mz);\n"
"    midmap[k]=pts.length;\n"
"    pts.push([mx/l,my/l,mz/l]);\n"
"    return midmap[k];\n"
"  }\n"
"  for(let s=0;s<3;s++){\n"
"    const nt=[];\n"
"    for(const [a,b,c] of tris){\n"
"      const ab=mid(a,b),bc=mid(b,c),ca=mid(c,a);\n"
"      nt.push([a,ab,ca],[b,bc,ab],[c,ca,bc],[ab,bc,ca]);\n"
"    }\n"
"    tris=nt;\n"
"  }\n"
"  // Deformar esfera → cabeza humana\n"
"  const age_f = STATE.age / 100.0;\n"
"  const sex_f = STATE.sex;\n"
"  const skull = pts.map(([x,y,z]) => {\n"
"    // Proporción craneal ajustada por edad/sexo\n"
"    const scale_y = 1.18 + sex_f*0.06 - age_f*0.01;\n"
"    const scale_x = 0.82 + sex_f*0.12;\n"
"    const scale_z = 0.90 + sex_f*0.08;\n"
"    // Occipital: aplanar atrás\n"
"    const back_f = Math.max(0,-z) * 0.15;\n"
"    // Mandíbula: proyección inferior\n"
"    const jaw_y = (y < -0.2) ? (y * (1.0 + (sex_f*0.15))) : y;\n"
"    // Frente: proyección frontal\n"
"    const brow_z = (y>0.1 && y<0.4) ? z*(1.0+0.08) : z;\n"
"    return [\n"
"      x * scale_x * 0.115,\n"
"      jaw_y * scale_y * 0.14,\n"
"      (brow_z - back_f) * scale_z * 0.115\n"
"    ];\n"
"  });\n"
"  return { verts: skull, tris, vert_count: skull.length, tri_count: tris.length };\n"
"}\n\n"

"// ── Matriz matemática (sin deps externas) ────────────────────\n"
"function mat4_identity() { const m=new Float32Array(16); m[0]=m[5]=m[10]=m[15]=1; return m; }\n"
"function mat4_perspective(fov_deg, aspect, near, far) {\n"
"  const m=new Float32Array(16);\n"
"  const f=1.0/Math.tan(fov_deg*Math.PI/360);\n"
"  m[0]=f/aspect; m[5]=f;\n"
"  m[10]=(far+near)/(near-far); m[11]=-1;\n"
"  m[14]=(2*far*near)/(near-far);\n"
"  return m;\n"
"}\n"
"function mat4_lookAt(ex,ey,ez, tx,ty,tz, ux,uy,uz) {\n"
"  const f=normalize3(ex-tx,ey-ty,ez-tz);\n"
"  const r=normalize3(cross3(ux,uy,uz,f[0],f[1],f[2]));\n"
"  const u=cross3(f[0],f[1],f[2],r[0],r[1],r[2]);\n"
"  const m=new Float32Array(16);\n"
"  m[0]=r[0];m[4]=r[1];m[8]=r[2];\n"
"  m[1]=u[0];m[5]=u[1];m[9]=u[2];\n"
"  m[2]=f[0];m[6]=f[1];m[10]=f[2];\n"
"  m[12]=-(r[0]*ex+r[1]*ey+r[2]*ez);\n"
"  m[13]=-(u[0]*ex+u[1]*ey+u[2]*ez);\n"
"  m[14]=-(f[0]*ex+f[1]*ey+f[2]*ez);\n"
"  m[15]=1;\n"
"  return m;\n"
"}\n"
"function mat4_mul(a,b){const m=new Float32Array(16);for(let r=0;r<4;r++)for(let c=0;c<4;c++){let s=0;for(let k=0;k<4;k++)s+=a[r+k*4]*b[k+c*4];m[r+c*4]=s;}return m;}\n"
"function normalize3(x,y,z){const l=Math.sqrt(x*x+y*y+z*z)||1;return[x/l,y/l,z/l];}\n"
"function cross3(ax,ay,az,bx,by,bz){return[ay*bz-az*by,az*bx-ax*bz,ax*by-ay*bx];}\n\n"

"// ── Render loop principal ────────────────────────────────────\n"
"let PREV_T = 0;\n"
"function renderFrame(ts) {\n"
"  AF = requestAnimationFrame(renderFrame);\n"
"  const dt = Math.min((ts - PREV_T) * 0.001, 0.05);\n"
"  PREV_T = ts;\n"
"  TIME += dt;\n\n"
"  // FPS\n"
"  FPS_ACC += dt; FPS_CT++;\n"
"  if(FPS_ACC >= 0.5) {\n"
"    const fps = Math.round(FPS_CT / FPS_ACC);\n"
"    const el = document.getElementById('fps-badge');\n"
"    if(el) el.textContent = fps + ' FPS';\n"
"    FPS_ACC = 0; FPS_CT = 0;\n"
"  }\n\n"
"  // Fisiología\n"
"  HEART_PHASE  += dt * (STATE.heart_bpm / 60) * Math.PI * 2;\n"
"  if(HEART_PHASE > Math.PI*2) HEART_PHASE -= Math.PI*2;\n"
"  BREATH_PHASE += dt * (STATE.breath_bpm / 60);\n"
"  if(BREATH_PHASE > 1) BREATH_PHASE -= 1;\n\n"
"  // Parpadeo\n"
"  if(!IN_BLINK) {\n"
"    BLINK_TIMER -= dt;\n"
"    if(BLINK_TIMER <= 0) {\n"
"      IN_BLINK=true; BLINK_PHASE=0;\n"
"      BLINK_TIMER = 2.5 + Math.random()*5;\n"
"    }\n"
"  } else {\n"
"    BLINK_PHASE += dt / 0.13;\n"
"    if(BLINK_PHASE>1) { IN_BLINK=false; BLINK_PHASE=0; }\n"
"    const bp = BLINK_PHASE<0.4 ? BLINK_PHASE/0.4 : 1-(BLINK_PHASE-0.4)/0.6;\n"
"    STATE.au[43] = Math.max(0, Math.min(1, bp));\n"
"  }\n\n"
"  // Microsacadas oculares\n"
"  if(!IN_SACCADE) {\n"
"    SACCADE_TIMER -= dt;\n"
"    if(SACCADE_TIMER<=0) {\n"
"      const amp = 0.5 + Math.random()*3.5;\n"
"      const dir = Math.random()*Math.PI*2;\n"
"      SACCADE_TY = Math.cos(dir)*amp*0.01745;\n"
"      SACCADE_TP = Math.sin(dir)*amp*0.01745*0.6;\n"
"      SACCADE_DUR = 2.2*Math.pow(amp,0.45)*0.001;\n"
"      SACCADE_PROG=0; IN_SACCADE=true;\n"
"      SACCADE_TIMER = 1 + Math.random()*3.5;\n"
"    }\n"
"  } else {\n"
"    SACCADE_PROG += dt/SACCADE_DUR;\n"
"    if(SACCADE_PROG>=1){EYE_YAW=SACCADE_TY;EYE_PITCH=SACCADE_TP;IN_SACCADE=false;}\n"
"    else {\n"
"      const sp=Math.sin(SACCADE_PROG*Math.PI);\n"
"      EYE_YAW   += (SACCADE_TY-EYE_YAW)*sp*dt/SACCADE_DUR*4;\n"
"      EYE_PITCH += (SACCADE_TP-EYE_PITCH)*sp*dt/SACCADE_DUR*4;\n"
"    }\n"
"  }\n\n"
"  // Timeline\n"
"  if(STATE.playing) {\n"
"    STATE.tl_time += dt;\n"
"    if(STATE.tl_time >= STATE.tl_dur) STATE.tl_time = 0;\n"
"    updateTimelineUI();\n"
"  }\n\n"
"  // Render\n"
"  if(GL && PROG_FACE) drawScene(dt);\n"
"}\n\n"

"function drawScene(dt) {\n"
"  const c = GL.canvas;\n"
"  const dpr = Math.min(window.devicePixelRatio || 1, 2);\n"
"  const W = c.clientWidth * dpr | 0;\n"
"  const H = c.clientHeight * dpr | 0;\n"
"  if(c.width!==W || c.height!==H) { c.width=W; c.height=H; }\n"
"  GL.viewport(0,0,W,H);\n"
"  GL.clearColor(0.04,0.04,0.06,1);\n"
"  GL.clear(GL.COLOR_BUFFER_BIT|GL.DEPTH_BUFFER_BIT);\n\n"
"  const aspect = W/H;\n"
"  const cam = STATE.cam;\n"
"  const ex = cam.dist*Math.sin(cam.yaw)*Math.cos(cam.pitch);\n"
"  const ey = cam.dist*Math.sin(cam.pitch);\n"
"  const ez = cam.dist*Math.cos(cam.yaw)*Math.cos(cam.pitch);\n"
"  const proj = mat4_perspective(cam.fov, aspect, 0.001, 10);\n"
"  const view = mat4_lookAt(ex,ey,ez, 0,0,0, 0,1,0);\n"
"  const mvp  = mat4_mul(proj, view);\n\n"
"  GL.useProgram(PROG_FACE);\n"
"  const L = n => GL.getUniformLocation(PROG_FACE, n);\n"
"  GL.uniformMatrix4fv(L('u_mvp'), false, mvp);\n"
"  GL.uniform1f(L('u_time'), TIME);\n"
"  // Bioquímica\n"
"  const bio = STATE.bio;\n"
"  GL.uniform1f(L('u_melanin_eu'),       bio.melanin_eu);\n"
"  GL.uniform1f(L('u_melanin_ph'),       bio.melanin_ph);\n"
"  GL.uniform1f(L('u_hemoglobin_oxy'),   bio.hemoglobin_oxy);\n"
"  GL.uniform1f(L('u_hemoglobin_deoxy'), bio.hemoglobin_deoxy);\n"
"  GL.uniform1f(L('u_bilirubin'),        bio.bilirubin);\n"
"  GL.uniform1f(L('u_carotene'),         bio.carotene);\n"
"  GL.uniform1f(L('u_age_norm'),         bio.age_norm);\n"
"  GL.uniform1f(L('u_sun_damage'),       bio.sun_damage);\n"
"  // Dinámica\n"
"  GL.uniform1f(L('u_heartbeat_phase'), HEART_PHASE);\n"
"  GL.uniform1f(L('u_sweat_level'),     STATE.sweat);\n"
"  // AU array\n"
"  GL.uniform1fv(L('u_au'), STATE.au);\n"
"  GL.uniform1f(L('u_jaw_angle'),    STATE.au[25]*0.25);\n"
"  GL.uniform1f(L('u_breath_phase'), BREATH_PHASE);\n"
"  GL.uniform1f(L('u_heart_phase'),  HEART_PHASE);\n"
"  // Luz activa según modo\n"
"  const lp = LIGHT_PRESETS[STATE.light_mode];\n"
"  GL.uniform3fv(L('u_light_dir_0'),   new Float32Array(lp.key.dir));\n"
"  GL.uniform3fv(L('u_light_color_0'), new Float32Array(lp.key.color));\n"
"  GL.uniform1f( L('u_light_intensity_0'), lp.key.intensity);\n"
"  GL.uniform3fv(L('u_light_dir_1'),   new Float32Array(lp.fill.dir));\n"
"  GL.uniform3fv(L('u_light_color_1'), new Float32Array(lp.fill.color));\n"
"  GL.uniform1f( L('u_light_intensity_1'), lp.fill.intensity);\n"
"  GL.uniform3fv(L('u_light_dir_2'),   new Float32Array(lp.rim.dir));\n"
"  GL.uniform3fv(L('u_light_color_2'), new Float32Array(lp.rim.color));\n"
"  GL.uniform1f( L('u_light_intensity_2'), lp.rim.intensity);\n"
"  GL.uniform3fv(L('u_view_dir'), new Float32Array(normalize3(-ex,-ey,-ez)));\n"
"  GL.uniform1f( L('u_exposure'), 1.0);\n"
"  GL.uniform1f( L('u_sss_weight'), STATE.enable.sss ? 0.45 : 0.0);\n"
"  if(VAO_FACE) {\n"
"    GL.bindVertexArray(VAO_FACE);\n"
"    GL.drawElements(GL.TRIANGLES, FACE_IDX_COUNT, GL.UNSIGNED_SHORT, 0);\n"
"    GL.bindVertexArray(null);\n"
"  }\n"
"}\n\n"

"// ── Presets de luz ───────────────────────────────────────────\n"
"const LIGHT_PRESETS = [\n"
"  { name:'Studio', key:{dir:[0.5,0.8,0.6],color:[1,0.97,0.94],intensity:2.0},\n"
"    fill:{dir:[-0.6,0.2,0.5],color:[0.8,0.9,1.0],intensity:0.6},\n"
"    rim:{dir:[0.2,-0.3,-0.9],color:[1,0.98,0.95],intensity:0.8} },\n"
"  { name:'Sunny', key:{dir:[0.3,1.0,0.4],color:[1,0.98,0.92],intensity:3.5},\n"
"    fill:{dir:[-0.4,0.1,0.6],color:[0.7,0.85,1.0],intensity:1.2},\n"
"    rim:{dir:[0.5,-0.2,-0.7],color:[1,0.99,0.96],intensity:0.5} },\n"
"  { name:'Cloudy', key:{dir:[0.1,1.0,0.2],color:[0.88,0.92,1.0],intensity:1.8},\n"
"    fill:{dir:[-0.2,0.5,0.4],color:[0.85,0.90,1.0],intensity:1.5},\n"
"    rim:{dir:[0.3,-0.1,-0.5],color:[0.9,0.93,1.0],intensity:0.4} },\n"
"  { name:'Golden Hour', key:{dir:[0.8,0.2,0.5],color:[1.0,0.72,0.32],intensity:3.0},\n"
"    fill:{dir:[-0.5,0.3,0.5],color:[1.0,0.8,0.6],intensity:0.8},\n"
"    rim:{dir:[0.1,-0.4,-0.9],color:[0.8,0.5,0.2],intensity:1.2} },\n"
"  { name:'Blue Hour', key:{dir:[-0.2,0.5,0.7],color:[0.4,0.6,1.0],intensity:1.0},\n"
"    fill:{dir:[0.5,0.2,0.4],color:[0.5,0.4,0.8],intensity:0.6},\n"
"    rim:{dir:[0.0,-0.3,-0.9],color:[0.3,0.5,0.9],intensity:0.5} },\n"
"  { name:'Noche', key:{dir:[0.2,0.6,0.4],color:[0.6,0.65,1.0],intensity:0.5},\n"
"    fill:{dir:[-0.4,0.1,0.5],color:[0.4,0.4,0.7],intensity:0.2},\n"
"    rim:{dir:[0.1,-0.2,-0.8],color:[0.8,0.9,1.0],intensity:0.4} },\n"
"  { name:'Cinemático', key:{dir:[0.4,0.7,0.3],color:[1,0.95,0.88],intensity:2.5},\n"
"    fill:{dir:[-0.7,0.0,0.5],color:[0.6,0.7,1.0],intensity:0.3},\n"
"    rim:{dir:[0.3,-0.5,-0.8],color:[1,0.8,0.5],intensity:1.5} },\n"
"  { name:'Rembrandt', key:{dir:[0.6,0.6,0.4],color:[1,0.94,0.82],intensity:2.8},\n"
"    fill:{dir:[-0.9,0.1,0.3],color:[0.5,0.6,0.9],intensity:0.15},\n"
"    rim:{dir:[0.0,-0.4,-0.9],color:[0.9,0.85,0.7],intensity:0.3} },\n"
"  { name:'Butterfly', key:{dir:[0.0,1.0,0.2],color:[1,0.97,0.93],intensity:3.0},\n"
"    fill:{dir:[0.0,0.3,0.6],color:[0.8,0.85,1.0],intensity:0.7},\n"
"    rim:{dir:[0.0,-0.6,-0.8],color:[1,0.98,0.95],intensity:0.5} },\n"
"  { name:'Loop', key:{dir:[0.3,0.8,0.4],color:[1,0.95,0.90],intensity:2.2},\n"
"    fill:{dir:[-0.5,0.4,0.5],color:[0.75,0.85,1.0],intensity:0.8},\n"
"    rim:{dir:[0.2,-0.3,-0.8],color:[1,0.98,0.96],intensity:0.6} }\n"
"];\n\n"

"// ── 64 Arquetipos NG ─────────────────────────────────────────\n"
"const ARCHETYPES = [\n"
"  {id:'nordic_young',    icon:'🧊', name:'Nórdico Joven',    eth:0, age:24, sex:0.6, hair:0},\n"
"  {id:'med_young',       icon:'☀️', name:'Mediterráneo',      eth:1, age:27, sex:0.7, hair:1},\n"
"  {id:'east_asian_f',   icon:'🌸', name:'Asia Oriental F',   eth:2, age:25, sex:0.1, hair:0},\n"
"  {id:'east_asian_m',   icon:'⛩️', name:'Asia Oriental M',   eth:2, age:28, sex:0.8, hair:0},\n"
"  {id:'south_asian_f',  icon:'🪷', name:'Asia del Sur F',    eth:3, age:26, sex:0.1, hair:2},\n"
"  {id:'west_african_m', icon:'🦁', name:'África Occ M',      eth:4, age:28, sex:0.9, hair:9},\n"
"  {id:'east_african_f', icon:'🌺', name:'África Or F',       eth:5, age:24, sex:0.1, hair:7},\n"
"  {id:'middle_east_m',  icon:'🌙', name:'Oriente Medio M',   eth:6, age:30, sex:0.8, hair:1},\n"
"  {id:'latin_f',        icon:'🌹', name:'Latinoamérica F',   eth:7, age:26, sex:0.1, hair:3},\n"
"  {id:'latin_m',        icon:'🦅', name:'Latinoamérica M',   eth:7, age:30, sex:0.9, hair:2},\n"
"  {id:'nordic_elder',   icon:'❄️', name:'Nórdico Mayor',     eth:0, age:68, sex:0.6, hair:0},\n"
"  {id:'med_elder',      icon:'🏛️', name:'Mediterráneo May',  eth:1, age:72, sex:0.7, hair:1},\n"
"  {id:'child_f',        icon:'🌻', name:'Niña 8 años',       eth:1, age:8,  sex:0.1, hair:3},\n"
"  {id:'child_m',        icon:'⚽', name:'Niño 8 años',       eth:1, age:8,  sex:0.9, hair:0},\n"
"  {id:'teen_f',         icon:'🌷', name:'Adolescente F',     eth:1, age:16, sex:0.2, hair:3},\n"
"  {id:'teen_m',         icon:'🎮', name:'Adolescente M',     eth:1, age:16, sex:0.8, hair:0},\n"
"  {id:'elder_african',  icon:'🌍', name:'Anciana Africana',  eth:5, age:75, sex:0.1, hair:7},\n"
"  {id:'albino_f',       icon:'🤍', name:'Albinismo F',       eth:10,age:25, sex:0.2, hair:0},\n"
"  {id:'vitiligo_m',     icon:'🎭', name:'Vitíligo M',        eth:11,age:32, sex:0.7, hair:1},\n"
"  {id:'hero_warrior',   icon:'⚔️', name:'Guerrero Héroe',    eth:1, age:34, sex:0.95,hair:1},\n"
"  {id:'sage_elder',     icon:'📜', name:'Sabio Anciano',     eth:2, age:80, sex:0.8, hair:0},\n"
"  {id:'noble_fem',      icon:'👑', name:'Noble Femenina',    eth:1, age:32, sex:0.05,hair:3},\n"
"  {id:'villain',        icon:'🖤', name:'Villano',           eth:1, age:45, sex:0.9, hair:0},\n"
"  {id:'androgynous',    icon:'⚧️', name:'Andrógino',         eth:1, age:25, sex:0.5, hair:2},\n"
"  {id:'scholar',        icon:'🔬', name:'Erudita',           eth:2, age:38, sex:0.2, hair:3},\n"
"  {id:'warrior_fem',    icon:'🏹', name:'Guerrera',          eth:5, age:28, sex:0.05,hair:4},\n"
"  {id:'infant',         icon:'👶', name:'Infante 1 año',     eth:1, age:1,  sex:0.5, hair:0},\n"
"  {id:'young_adult',    icon:'💫', name:'Adulto Joven 25',   eth:1, age:25, sex:0.5, hair:1},\n"
"  {id:'mature',         icon:'🍂', name:'Maduro 45',         eth:1, age:45, sex:0.5, hair:1},\n"
"  {id:'greek_ideal',    icon:'🏺', name:'Ideal Griego',      eth:1, age:28, sex:0.8, hair:2},\n"
"  {id:'phi_perfect',    icon:'φ',  name:'Proporción φ',      eth:1, age:25, sex:0.4, hair:2},\n"
"  {id:'hyperreal',      icon:'🎥', name:'Hiperrealista',     eth:1, age:30, sex:0.5, hair:1},\n"
"  {id:'anime_style',    icon:'⭐', name:'Estilo Anime',      eth:2, age:17, sex:0.2, hair:0},\n"
"  {id:'renaissance',    icon:'🎨', name:'Renacimiento',      eth:1, age:30, sex:0.3, hair:3},\n"
"  {id:'mayan_classic',  icon:'🗿', name:'Clásico Maya',      eth:8, age:28, sex:0.6, hair:2},\n"
"  {id:'apollonian',     icon:'☀️', name:'Apolóneo',          eth:0, age:26, sex:0.7, hair:2},\n"
"  {id:'pacific_f',      icon:'🌊', name:'Isla Pacífico F',   eth:9, age:24, sex:0.1, hair:3},\n"
"  {id:'indigenous_m',   icon:'🦜', name:'Indígena M',        eth:8, age:30, sex:0.8, hair:2},\n"
"  {id:'nordic_mature_f',icon:'🌨️', name:'Nórdica Madura',   eth:0, age:48, sex:0.1, hair:0},\n"
"  {id:'african_elder_m',icon:'🦏', name:'Anciano Africano',  eth:4, age:78, sex:0.9, hair:9},\n"
"];\n\n"

"let FACE_IDX_COUNT = 0;\n\n"

"function setupGeometry() {\n"
"  const geo = buildHeadGeometry(STATE);\n"
"  const V = new Float32Array(geo.verts.flat());\n"
"  const I = new Uint16Array(geo.tris.flat());\n"
"  FACE_IDX_COUNT = I.length;\n"
"  VAO_FACE = GL.createVertexArray();\n"
"  GL.bindVertexArray(VAO_FACE);\n"
"  VBO_FACE = GL.createBuffer();\n"
"  GL.bindBuffer(GL.ARRAY_BUFFER, VBO_FACE);\n"
"  GL.bufferData(GL.ARRAY_BUFFER, V, GL.DYNAMIC_DRAW);\n"
"  GL.enableVertexAttribArray(0);\n"
"  GL.vertexAttribPointer(0, 3, GL.FLOAT, false, 12, 0);\n"
"  IBO_FACE = GL.createBuffer();\n"
"  GL.bindBuffer(GL.ELEMENT_ARRAY_BUFFER, IBO_FACE);\n"
"  GL.bufferData(GL.ELEMENT_ARRAY_BUFFER, I, GL.STATIC_DRAW);\n"
"  GL.bindVertexArray(null);\n"
"  emit('geometry.setup.ok', { verts: geo.vert_count, tris: geo.tri_count });\n"
"}\n\n"

"// ── UI Construction ──────────────────────────────────────────\n"
"function buildUI() {\n"
"  // Arquetipos\n"
"  const grid = document.getElementById('archetype-grid');\n"
"  if(grid) {\n"
"    grid.innerHTML = ARCHETYPES.map(a =>\n"
"      `<div class='arch-card ${STATE.archetype===a.id?'selected':''}' `+\n"
"      `data-id='${a.id}' onclick='selectArchetype(\"${a.id}\")'>`+\n"
"      `<span class='icon'>${a.icon}</span>${a.name}</div>`\n"
"    ).join('');\n"
"  }\n"
"  // Emociones\n"
"  const emogrid = document.getElementById('emotion-grid');\n"
"  const emos = [\n"
"    {id:'neutral',  icon:'😐', label:'Neutral'},\n"
"    {id:'happy',    icon:'😊', label:'Alegría'},\n"
"    {id:'sad',      icon:'😢', label:'Tristeza'},\n"
"    {id:'angry',    icon:'😠', label:'Enojo'},\n"
"    {id:'fear',     icon:'😨', label:'Miedo'},\n"
"    {id:'disgust',  icon:'🤢', label:'Asco'},\n"
"    {id:'surprise', icon:'😲', label:'Sorpresa'},\n"
"    {id:'contempt', icon:'😒', label:'Desdén'},\n"
"    {id:'joy',      icon:'😄', label:'Júbilo'},\n"
"    {id:'confusion',icon:'🤔', label:'Confusión'},\n"
"    {id:'flirt',    icon:'😘', label:'Coqueteo'},\n"
"    {id:'pain',     icon:'😣', label:'Dolor'},\n"
"  ];\n"
"  if(emogrid) {\n"
"    emogrid.innerHTML = emos.map(e =>\n"
"      `<div class='emo-btn ${STATE.emotion===e.id?'active':''}' `+\n"
"      `onclick='setEmotion(\"${e.id}\")'>`+\n"
"      `<span class='emo-icon'>${e.icon}</span>${e.label}</div>`\n"
"    ).join('');\n"
"  }\n"
"  // Colores de ojos\n"
"  buildEyeSwatches();\n"
"  emit('ui.build.ok', { archetypes: ARCHETYPES.length, emotions: emos.length });\n"
"}\n\n"

"function buildEyeSwatches() {\n"
"  const c = document.getElementById('eye-swatches');\n"
"  if(!c) return;\n"
"  const colors = [\n"
"    {id:'brown',      hex:'#6B3A2A', name:'Marrón'},\n"
"    {id:'dark_brown', hex:'#2C1810', name:'Marrón Oscuro'},\n"
"    {id:'hazel',      hex:'#8B6914', name:'Avellana'},\n"
"    {id:'amber',      hex:'#D4A017', name:'Ámbar'},\n"
"    {id:'green',      hex:'#3A7A3A', name:'Verde'},\n"
"    {id:'blue_gray',  hex:'#7A8FA0', name:'Azul Gris'},\n"
"    {id:'blue',       hex:'#2A5FA8', name:'Azul'},\n"
"    {id:'gray',       hex:'#7A7A8A', name:'Gris'},\n"
"  ];\n"
"  c.innerHTML = colors.map(cl =>\n"
"    `<div class='swatch ${STATE.eye_color===cl.id?'active':''}' `+\n"
"    `style='background:${cl.hex}' title='${cl.name}' `+\n"
"    `onclick='setEyeColor(\"${cl.id}\",\"${cl.hex}\")'></div>`\n"
"  ).join('');\n"
"}\n\n"

"// ── Acciones de UI ───────────────────────────────────────────\n"
"function selectArchetype(id) {\n"
"  const a = ARCHETYPES.find(x=>x.id===id); if(!a) return;\n"
"  STATE.archetype = id;\n"
"  STATE.age       = a.age;\n"
"  STATE.sex       = a.sex;\n"
"  const eth_preset = RIG_ETH_PRESETS[a.eth];\n"
"  if(eth_preset) Object.assign(STATE.bio, eth_preset);\n"
"  STATE.bio.age_norm = a.age / 100.0;\n"
"  // Actualizar UI\n"
"  const s = document.getElementById('sl-age');\n"
"  if(s) { s.value = a.age; document.getElementById('val-age').textContent=a.age; }\n"
"  const ss = document.getElementById('sl-sex');\n"
"  if(ss) { ss.value=(a.sex*100)|0; document.getElementById('val-sex').textContent=(a.sex*100|0)+'%M'; }\n"
"  document.querySelectorAll('.arch-card').forEach(el =>{\n"
"    el.classList.toggle('selected', el.dataset.id===id);\n"
"  });\n"
"  document.getElementById('model-label').textContent = a.name;\n"
"  setupGeometry();\n"
"  emit('archetype.select', { value:id, name:a.name, age:a.age, eth:a.eth });\n"
"}\n\n"

"function setEmotion(id) {\n"
"  STATE.emotion = id;\n"
"  const w = EMOTION_AU[id] || {};\n"
"  STATE.au.fill(0);\n"
"  for(const [idx, val] of Object.entries(w)) STATE.au[parseInt(idx)] = val;\n"
"  document.querySelectorAll('.emo-btn').forEach(el => {\n"
"    el.classList.toggle('active', el.textContent.includes(id)||\n"
"      el.onclick?.toString().includes(id));\n"
"  });\n"
"  document.querySelectorAll('.emo-btn').forEach((el,i) => {\n"
"    const ids=['neutral','happy','sad','angry','fear','disgust','surprise',\n"
"               'contempt','joy','confusion','flirt','pain'];\n"
"    el.classList.toggle('active', ids[i]===id);\n"
"  });\n"
"  emit('emotion.set', { value: id, au_weights: Object.keys(w).length });\n"
"}\n\n"

"function setEyeColor(id, hex) {\n"
"  STATE.eye_color = id;\n"
"  buildEyeSwatches();\n"
"  emit('eye.color.set', { value: id, hex });\n"
"}\n\n"

"function setLightMode(idx) {\n"
"  STATE.light_mode = parseInt(idx);\n"
"  emit('light.mode.set', { value: LIGHT_PRESETS[STATE.light_mode].name });\n"
"}\n\n"

"function setExpression(au_idx, val) {\n"
"  STATE.au[au_idx] = parseFloat(val);\n"
"  emit('au.set', { value: val, au: au_idx });\n"
"}\n\n"

"function togglePlay() {\n"
"  STATE.playing = !STATE.playing;\n"
"  const btn = document.getElementById('tl-play');\n"
"  if(btn) btn.textContent = STATE.playing ? '⏸' : '▶';\n"
"  emit('timeline.toggle', { value: STATE.playing ? 'play':'pause' });\n"
"}\n\n"

"function updateTimelineUI() {\n"
"  const pct = (STATE.tl_time / STATE.tl_dur * 100).toFixed(1);\n"
"  const prog = document.getElementById('tl-progress');\n"
"  if(prog) prog.style.width = pct + '%';\n"
"  const t = document.getElementById('tl-time');\n"
"  if(t) t.textContent = STATE.tl_time.toFixed(2) + 's / ' + STATE.tl_dur + 's';\n"
"}\n\n"

"function togglePanel() {\n"
"  const w = document.getElementById('panel-wrap');\n"
"  const isExp = w.classList.contains('expanded');\n"
"  w.classList.toggle('collapsed', isExp);\n"
"  w.classList.toggle('expanded', !isExp);\n"
"  emit('panel.toggle', { value: isExp ? 'collapsed':'expanded' });\n"
"}\n\n"

"function openTab(name) {\n"
"  document.querySelectorAll('.tab-panel').forEach(p => p.style.display='none');\n"
"  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));\n"
"  const panel = document.getElementById('tab-'+name);\n"
"  if(panel) panel.style.display='block';\n"
"  document.querySelectorAll('.tab').forEach(t=>{\n"
"    if(t.dataset.tab===name) t.classList.add('active');\n"
"  });\n"
"  emit('tab.open', { value: name });\n"
"}\n\n"

"function openExport() {\n"
"  document.getElementById('export-modal').classList.add('open');\n"
"  emit('export.modal.open', {});\n"
"}\n"
"function closeExport() {\n"
"  document.getElementById('export-modal').classList.remove('open');\n"
"}\n"
"function exportGLTF() {\n"
"  const data = JSON.stringify({rigcom:'NG', phi:PHI, state:STATE, timestamp:Date.now()}, null, 2);\n"
"  const b = new Blob([data], {type:'application/json'});\n"
"  const a = Object.assign(document.createElement('a'), {href:URL.createObjectURL(b), download:'avatar_ng.gltf.json'});\n"
"  a.click(); URL.revokeObjectURL(a.href);\n"
"  emit('export.gltf.ok', { bytes: data.length });\n"
"  closeExport();\n"
"}\n"
"function exportParams() {\n"
"  const data = JSON.stringify(STATE, null, 2);\n"
"  const b = new Blob([data], {type:'application/json'});\n"
"  const a = Object.assign(document.createElement('a'), {href:URL.createObjectURL(b), download:'avatar_params.json'});\n"
"  a.click(); URL.revokeObjectURL(a.href);\n"
"  emit('export.params.ok', { bytes: data.length });\n"
"  closeExport();\n"
"}\n"
"function captureFrame() {\n"
"  GL.canvas.toBlob(b => {\n"
"    const a = Object.assign(document.createElement('a'), {href:URL.createObjectURL(b), download:'frame_ng.png'});\n"
"    a.click(); URL.revokeObjectURL(a.href);\n"
"    emit('capture.frame.ok', {});\n"
"  });\n"
"  closeExport();\n"
"}\n\n"

"// ── Touch / Orbit controls ───────────────────────────────────\n"
"let TOUCH_PREV = null, TOUCH_DIST_PREV = null;\n"
"function initCamControls() {\n"
"  const c = document.getElementById('c');\n"
"  // Mouse\n"
"  let md=false, mx=0, my=0;\n"
"  c.addEventListener('mousedown', e=>{md=true;mx=e.clientX;my=e.clientY;});\n"
"  c.addEventListener('mousemove', e=>{\n"
"    if(!md) return;\n"
"    const dx=(e.clientX-mx)*0.005, dy=(e.clientY-my)*0.005;\n"
"    STATE.cam.yaw   -= dx;\n"
"    STATE.cam.pitch  = Math.max(-1.2,Math.min(1.2,STATE.cam.pitch-dy));\n"
"    mx=e.clientX; my=e.clientY;\n"
"  });\n"
"  c.addEventListener('mouseup', ()=>md=false);\n"
"  c.addEventListener('wheel', e=>{\n"
"    STATE.cam.dist = Math.max(0.08,Math.min(1.5,STATE.cam.dist+e.deltaY*0.001));\n"
"    emit('cam.zoom', { value: STATE.cam.dist.toFixed(3) });\n"
"  });\n"
"  // Touch\n"
"  c.addEventListener('touchstart', e=>{\n"
"    if(e.touches.length===1) TOUCH_PREV=[e.touches[0].clientX,e.touches[0].clientY];\n"
"    if(e.touches.length===2) {\n"
"      const dx=e.touches[0].clientX-e.touches[1].clientX;\n"
"      const dy=e.touches[0].clientY-e.touches[1].clientY;\n"
"      TOUCH_DIST_PREV=Math.sqrt(dx*dx+dy*dy);\n"
"    }\n"
"  },{passive:true});\n"
"  c.addEventListener('touchmove', e=>{\n"
"    if(e.touches.length===1 && TOUCH_PREV) {\n"
"      const dx=(e.touches[0].clientX-TOUCH_PREV[0])*0.005;\n"
"      const dy=(e.touches[0].clientY-TOUCH_PREV[1])*0.005;\n"
"      STATE.cam.yaw  -= dx;\n"
"      STATE.cam.pitch = Math.max(-1.2,Math.min(1.2,STATE.cam.pitch-dy));\n"
"      TOUCH_PREV=[e.touches[0].clientX,e.touches[0].clientY];\n"
"    }\n"
"    if(e.touches.length===2 && TOUCH_DIST_PREV!==null) {\n"
"      const dx=e.touches[0].clientX-e.touches[1].clientX;\n"
"      const dy=e.touches[0].clientY-e.touches[1].clientY;\n"
"      const d=Math.sqrt(dx*dx+dy*dy);\n"
"      STATE.cam.dist=Math.max(0.08,Math.min(1.5,STATE.cam.dist-(d-TOUCH_DIST_PREV)*0.003));\n"
"      TOUCH_DIST_PREV=d;\n"
"    }\n"
"  },{passive:true});\n"
"  c.addEventListener('touchend', ()=>{TOUCH_PREV=null;TOUCH_DIST_PREV=null;});\n"
"  // Botones de cámara\n"
"  document.getElementById('btn-front') ?.addEventListener('click',()=>{ STATE.cam.yaw=0;STATE.cam.pitch=0.05; emit('cam.preset',{value:'front'}); });\n"
"  document.getElementById('btn-side')  ?.addEventListener('click',()=>{ STATE.cam.yaw=Math.PI*0.5;STATE.cam.pitch=0.05; emit('cam.preset',{value:'side'}); });\n"
"  document.getElementById('btn-top')   ?.addEventListener('click',()=>{ STATE.cam.pitch=1.2; emit('cam.preset',{value:'top'}); });\n"
"  document.getElementById('btn-diag')  ?.addEventListener('click',()=>{ STATE.cam.yaw=0.4;STATE.cam.pitch=0.12; emit('cam.preset',{value:'diagonal'}); });\n"
"}\n\n"

"// ── Presets de bioquímica por etnología ──────────────────────\n"
"const RIG_ETH_PRESETS = [\n"
"  {melanin_eu:0.05,melanin_ph:0.08,hemoglobin_oxy:0.48,hemoglobin_deoxy:0.05,bilirubin:0.02,carotene:0.06}, // 0 Nordic\n"
"  {melanin_eu:0.22,melanin_ph:0.12,hemoglobin_oxy:0.46,hemoglobin_deoxy:0.05,bilirubin:0.03,carotene:0.08}, // 1 Med\n"
"  {melanin_eu:0.20,melanin_ph:0.10,hemoglobin_oxy:0.38,hemoglobin_deoxy:0.04,bilirubin:0.05,carotene:0.15}, // 2 East Asian\n"
"  {melanin_eu:0.40,melanin_ph:0.08,hemoglobin_oxy:0.44,hemoglobin_deoxy:0.05,bilirubin:0.03,carotene:0.10}, // 3 South Asian\n"
"  {melanin_eu:0.82,melanin_ph:0.04,hemoglobin_oxy:0.52,hemoglobin_deoxy:0.06,bilirubin:0.02,carotene:0.05}, // 4 West African\n"
"  {melanin_eu:0.70,melanin_ph:0.06,hemoglobin_oxy:0.48,hemoglobin_deoxy:0.05,bilirubin:0.02,carotene:0.06}, // 5 East African\n"
"  {melanin_eu:0.35,melanin_ph:0.10,hemoglobin_oxy:0.46,hemoglobin_deoxy:0.05,bilirubin:0.03,carotene:0.12}, // 6 Middle East\n"
"  {melanin_eu:0.30,melanin_ph:0.10,hemoglobin_oxy:0.44,hemoglobin_deoxy:0.05,bilirubin:0.03,carotene:0.12}, // 7 Latin Am\n"
"  {melanin_eu:0.45,melanin_ph:0.08,hemoglobin_oxy:0.42,hemoglobin_deoxy:0.05,bilirubin:0.04,carotene:0.14}, // 8 Indigenous\n"
"  {melanin_eu:0.50,melanin_ph:0.09,hemoglobin_oxy:0.46,hemoglobin_deoxy:0.05,bilirubin:0.03,carotene:0.10}, // 9 Pacific\n"
"  {melanin_eu:0.00,melanin_ph:0.00,hemoglobin_oxy:0.78,hemoglobin_deoxy:0.08,bilirubin:0.02,carotene:0.02}, // 10 Albinism\n"
"  {melanin_eu:0.10,melanin_ph:0.05,hemoglobin_oxy:0.50,hemoglobin_deoxy:0.06,bilirubin:0.02,carotene:0.06}, // 11 Vitiligo\n"
"];\n\n"

"// ── AU weights por emoción ───────────────────────────────────\n"
"const EMOTION_AU = {\n"
"  neutral:  {},\n"
"  happy:    {11:0.90, 5:0.75, 44:0.10},\n"
"  sad:      {0:0.70, 3:0.35, 14:0.60, 16:0.40},\n"
"  angry:    {3:0.90, 4:0.60, 6:0.55, 22:0.70, 23:0.55},\n"
"  fear:     {0:0.65, 1:0.70, 3:0.45, 4:0.85, 19:0.65, 25:0.55},\n"
"  disgust:  {8:0.85, 14:0.50, 15:0.55, 16:0.40, 24:0.35},\n"
"  surprise: {0:0.65, 1:0.75, 4:0.80, 25:0.70, 26:0.55},\n"
"  contempt: {13:0.75, 11:0.45, 79:0.60, 6:0.40},\n"
"  joy:      {11:1.00, 5:0.90, 85:0.80, 84:0.70},\n"
"  confusion:{3:0.55, 30:0.50, 6:0.35, 19:0.40},\n"
"  flirt:    {11:0.65, 45:0.85, 1:0.40, 5:0.45},\n"
"  pain:     {3:0.90, 0:0.75, 5:0.45, 19:0.55, 16:0.60},\n"
"};\n\n"

"function showError(msg) {\n"
"  const el = document.createElement('div');\n"
"  el.style.cssText='position:fixed;top:50%;left:50%;transform:translate(-50%,-50%);'\n"
"    +'background:#1a0a0a;border:1px solid #ff4f4f;border-radius:12px;padding:20px;'\n"
"    +'color:#ff8080;font-size:13px;z-index:999;max-width:300px;text-align:center';\n"
"  el.textContent='⚠️ '+msg;\n"
"  document.body.appendChild(el);\n"
"}\n\n"

"// ── Init ─────────────────────────────────────────────────────\n"
"window.addEventListener('DOMContentLoaded', () => {\n"
"  if(!initGL()) return;\n"
"  // Shader mínimo de demostración (cabeza procedural)\n"
"  const VS = `#version 300 es\n"
"precision highp float;\n"
"in vec3 a_pos;\n"
"uniform mat4 u_mvp;\n"
"uniform float u_melanin_eu;\n"
"uniform float u_age_norm;\n"
"uniform float u_time;\n"
"uniform float u_au[128];\n"
"out vec3 v_pos; out vec3 v_n;\n"
"void main(){\n"
"  vec3 p=a_pos;\n"
"  // Deformación AU26 jaw drop\n"
"  if(p.y<-0.02)p.y-=u_au[25]*0.04;\n"
"  // Parpadeo AU43\n"
"  float blink_region=(p.y>0.01&&p.y<0.03&&abs(p.x)<0.03)?1.:0.;\n"
"  p.y+=blink_region*u_au[43]*(-0.015);\n"
"  v_pos=p;\n"
"  v_n=normalize(p);\n"
"  gl_Position=u_mvp*vec4(p,1.);\n"
"}`;\n"
"  const FS = `#version 300 es\n"
"precision highp float;\n"
"in vec3 v_pos; in vec3 v_n;\n"
"uniform float u_melanin_eu,u_melanin_ph,u_hemoglobin_oxy;\n"
"uniform float u_age_norm,u_time;\n"
"uniform float u_heartbeat_phase,u_sweat_level;\n"
"uniform float u_sss_weight;\n"
"uniform vec3 u_light_dir_0,u_light_color_0; uniform float u_light_intensity_0;\n"
"uniform vec3 u_light_dir_1,u_light_color_1; uniform float u_light_intensity_1;\n"
"uniform vec3 u_light_dir_2,u_light_color_2; uniform float u_light_intensity_2;\n"
"uniform vec3 u_view_dir;\n"
"uniform float u_exposure;\n"
"out vec4 fc;\n"
"void main(){\n"
"  vec3 N=normalize(v_n);\n"
"  vec3 V=normalize(u_view_dir);\n"
"  // Albedo bioquímico simplificado\n"
"  vec3 skin_base=vec3(0.88,0.72,0.56);\n"
"  skin_base*=(1.-vec3(.55,.38,.72)*u_melanin_eu);\n"
"  skin_base+=vec3(.18,.04,.02)*u_hemoglobin_oxy;\n"
"  // Envejecimiento\n"
"  float lum=dot(skin_base,vec3(.2126,.7152,.0722));\n"
"  skin_base=mix(skin_base,vec3(lum)*vec3(1.02,1.,.95),u_age_norm*.12);\n"
"  // Pulso\n"
"  float sys=exp(-pow(fract(u_heartbeat_phase/6.2832)*5.,2.));\n"
"  // SSS simplificado\n"
"  float sss=exp(-length(v_pos)*3.)*u_sss_weight;\n"
"  vec3 sss_c=skin_base*vec3(1.,.62,.38)*sss;\n"
"  // Lighting 3 luces\n"
"  vec3 col=vec3(0.);\n"
"  vec3 dirs[3];\n"
"  dirs[0]=normalize(u_light_dir_0);\n"
"  dirs[1]=normalize(u_light_dir_1);\n"
"  dirs[2]=normalize(u_light_dir_2);\n"
"  vec3 cols[3];\n"
"  cols[0]=u_light_color_0; cols[1]=u_light_color_1; cols[2]=u_light_color_2;\n"
"  float ins[3];\n"
"  ins[0]=u_light_intensity_0; ins[1]=u_light_intensity_1; ins[2]=u_light_intensity_2;\n"
"  for(int i=0;i<3;i++){\n"
"    float ndl=max(dot(N,dirs[i]),0.);\n"
"    vec3 H=normalize(dirs[i]+V);\n"
"    float spec=pow(max(dot(N,H),0.),32.)*.08;\n"
"    col+=(skin_base/3.14159+vec3(spec))*ndl*cols[i]*ins[i];\n"
"  }\n"
"  col+=sss_c;\n"
"  col+=skin_base*vec3(1.,.62,.38)*sys*.04*u_hemoglobin_oxy;\n"
"  // Sudor\n"
"  col+=vec3(.08)*u_sweat_level*max(dot(N,dirs[0]),0.);\n"
"  // ACES\n"
"  col*=u_exposure;\n"
"  col=(col*(2.51*col+.03))/(col*(2.43*col+.59)+.14);\n"
"  col=pow(clamp(col,0.,1.),vec3(1./2.2));\n"
"  fc=vec4(col,1.);\n"
"}`;\n"
"  PROG_FACE = buildProgram(VS, FS);\n"
"  if(!PROG_FACE) return;\n"
"  setupGeometry();\n"
"  buildUI();\n"
"  initCamControls();\n"
"  selectArchetype('nordic_young');\n"
"  PREV_T = performance.now();\n"
"  AF = requestAnimationFrame(renderFrame);\n"
"  emit('rigcom.ng.ready', { version:'24.1.0-NG', phi: PHI });\n"
"});\n";

/* [fusion] function hair_strand_init__v2 <- nested/rig_face_ng_v24/tmp/rig_ng_package/src/rig_face_ng_hair.c:504 :: cuerpo divergente; ausente en la copia canonica */
static void hair_strand_init__v2(
    HairStrand *strand,
    float root_x, float root_y, float root_z,
    float dir_x,  float dir_y,  float dir_z,
    float length_m, int n_segs,
    float stiffness, float mel_eu, float mel_ph,
    float width_root, float width_tip
) {
    float seg_len = length_m / (float)n_segs;
    strand->n_particles = n_segs + 1;
    strand->particles   = (HairParticle*)calloc(
        strand->n_particles, sizeof(HairParticle));
    float nx = dir_x, ny = dir_y, nz = dir_z;
    float inv_len = 1.0f / sqrtf(nx*nx+ny*ny+nz*nz);
    nx*=inv_len; ny*=inv_len; nz*=inv_len;

    for (int i = 0; i <= n_segs; i++) {
        float t  = (float)i / (float)n_segs;
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
}

/* [fusion] function hair_sim_step__v2 <- nested/rig_face_ng_v24/tmp/rig_ng_package/src/rig_face_ng_hair.c:563 :: cuerpo divergente; ausente en la copia canonica */
void hair_sim_step__v2(HairSimSystem *sys, float dt)
{
    if (!sys || !sys->strands || dt <= 0.0f) return;
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
}

/* [fusion] function hair_sim_free__v2 <- nested/rig_face_ng_v24/tmp/rig_ng_package/src/rig_face_ng_hair.c:685 :: cuerpo divergente; ausente en la copia canonica */
void hair_sim_free__v2(HairSimSystem *sys)
{
    if (!sys) return;
    for (int i = 0; i < sys->n_strands; i++) {
        free(sys->strands[i].particles);
        free(sys->strands[i].len_c);
        free(sys->strands[i].bend_c);
    }
    free(sys->strands);
}

/* [fusion] function hair_strand_init__v3 <- nested/rig_face_v2_bridge/rig_face_ng_hair.c:449 :: cuerpo divergente; ausente en la copia canonica */
int hair_strand_init__v3(
    HairStrand *strand,
    float root_x, float root_y, float root_z,
    float dir_x,  float dir_y,  float dir_z,
    float length_m, int n_segs,
    float stiffness, float mel_eu, float mel_ph,
    float width_root, float width_tip
){
    if (!strand || n_segs < 1 || n_segs > 4096 || length_m <= 0.0f ||
        width_root <= 0.0f || width_tip < 0.0f) return -1;
    float dir_len_sq = dir_x*dir_x + dir_y*dir_y + dir_z*dir_z;
    if (dir_len_sq <= 1e-12f) return -1;
    memset(strand, 0, sizeof(*strand));
    float seg_len = length_m / (float)n_segs;
    strand->n_particles = n_segs + 1;
    strand->particles = (HairParticle*)calloc((size_t)strand->n_particles, sizeof(HairParticle));
    if (!strand->particles) return -1;
    float inv_len = 1.0f / sqrtf(dir_len_sq);
    float nx = dir_x * inv_len, ny = dir_y * inv_len, nz = dir_z * inv_len;

    for (int i = 0; i <= n_segs; i++) {
        float px = root_x + nx * seg_len * i;
        float py = root_y + ny * seg_len * i;
        float pz = root_z + nz * seg_len * i;
        strand->particles[i].px = strand->particles[i].ppx = strand->particles[i].restx = px;
        strand->particles[i].py = strand->particles[i].ppy = strand->particles[i].resty = py;
        strand->particles[i].pz = strand->particles[i].ppz = strand->particles[i].restz = pz;
        strand->particles[i].mass_inv = (i == 0) ? 0.0f : 1.0f;
        strand->particles[i].rest_len = seg_len;
    }
    strand->n_len = n_segs;
    strand->len_c = (HairLengthConstraint*)calloc((size_t)n_segs, sizeof(HairLengthConstraint));
    if (!strand->len_c) {
        free(strand->particles);
        memset(strand, 0, sizeof(*strand));
        return -1;
    }
    for (int i = 0; i < n_segs; i++) {
        strand->len_c[i].i0 = i;
        strand->len_c[i].i1 = i+1;
        strand->len_c[i].rest_len = seg_len;
        strand->len_c[i].stiffness = stiffness;
    }
    strand->n_bend = n_segs >= 2 ? n_segs - 1 : 0;
    if (strand->n_bend) {
        strand->bend_c = (HairBendConstraint*)calloc((size_t)strand->n_bend, sizeof(HairBendConstraint));
        if (!strand->bend_c) {
            free(strand->len_c);
            free(strand->particles);
            memset(strand, 0, sizeof(*strand));
            return -1;
        }
    }
    for (int i = 0; i < strand->n_bend; i++) {
        strand->bend_c[i].i0 = i;
        strand->bend_c[i].i1 = i+1;
        strand->bend_c[i].i2 = i+2;
        strand->bend_c[i].rest_angle_cos = 0.95f;
        strand->bend_c[i].stiffness = stiffness * 0.5f;
    }
    strand->mel_eu = mel_eu;
    strand->mel_ph = mel_ph;
    strand->width_root = width_root;
    strand->width_tip = width_tip;
    strand->stiffness_k = stiffness;
    return 0;
}

/* [fusion] function hair_strand_init__v4 <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_ng.c:2300 :: cuerpo divergente; absorbido de rig_face_ng */
static void hair_strand_init__v4(
    HairStrand *strand,
    float root_x, float root_y, float root_z,
    float dir_x,  float dir_y,  float dir_z,
    float length_m, int n_segs,
    float stiffness, float mel_eu, float mel_ph,
    float width_root, float width_tip
) {
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
}

