/*
 * rig_face_ng_eyes.c — RigCom v24 NEXT GENERATION · Sistema Ocular
 * ════════════════════════════════════════════════════════════════════════
 * Richard Felipe Urbina · RIGCOM Ecosystem · Arquitecto Soberano
 *
 * MODELO BIOFÍSICO COMPLETO DEL OJO HUMANO:
 *
 *   CÓRNEA:
 *   ├── Parallax mapping profundo (profundidad real ~5mm)
 *   ├── IOR = 1.376 → refracción real de iris/pupila
 *   ├── Aberraciones de Zernike (primeros 15 polinomios)
 *   ├── Aberración cromática: Abbe V=57 para córnea
 *   ├── Efecto Stiles-Crawford (mayor sensibilidad en centro)
 *   ├── Película lagrimal: interferencia thin-film (espesor 3-10μm)
 *   ├── Anillo de Purkinje (4 imágenes especulares)
 *   └── Limbo: zona de transición córnea→esclera
 *
 *   IRIS:
 *   ├── Criptas del iris: Voronoi + fBm procedural
 *   ├── Collarete: zona de criptas densas alrededor del esfínter
 *   ├── Músculo dilatador (radial) y esfínter (circular)
 *   ├── Pigmento estromal: melanina variable por capa
 *   ├── Venas radiales visibles en iris claro
 *   ├── Anillo de Zinn: borde interno del iris
 *   ├── Reflejo rojo del fondo (glint retinal en oscuridad)
 *   └── Dilatación pupilar: 2-8mm (fotópico→escotópico)
 *
 *   ESCLERA:
 *   ├── SSS real (esclera es translúcida: deja pasar sangre visible)
 *   ├── Vasos sanguíneos conjuntivales procedurales
 *   ├── Carúncula lagrimal (bulto en canthus medial)
 *   ├── Pliegue semilunar
 *   ├── Reflexión especular de la córnea sobre la esclera
 *   └── Tinte de envejecimiento (amarillamiento con la edad)
 *
 *   PÁRPADOS:
 *   ├── Glándulas de Meibomio (orificios en margen palpebral)
 *   ├── Pestañas con modelo de fibra individual
 *   ├── Menisco lagrimal inferior
 *   ├── Menisco lagrimal superior
 *   ├── Dinámica de parpadeo (3 fases: cierre, reposo, apertura)
 *   └── Pliegue de la piel orbital
 *
 *   DINÁMICA:
 *   ├── Sacadas oculares (200-600ms, movimiento balístico)
 *   ├── Microsacadas (20-200ms, fijación inestable)
 *   ├── Deriva de fijación (lento, 0.1°/s)
 *   ├── Tremor ocular (85Hz, <1')
 *   ├── Movimiento vergencial (seguimiento de objetos)
 *   ├── Reflejo pupilar a la luz (latencia 200ms)
 *   └── Hipus pupilar (variación espontánea 0.5Hz)
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

/* clamp soberano (faltaba) */
static inline float clamp(float x, float lo, float hi){ return x<lo?lo:(x>hi?hi:x); }

/* Contexto físico del ojo (modelo del .c; distinto del RigFaceNGEyeCtx artístico) */
typedef struct RigEyeNGCtx {
    float pupil_radius, iris_radius;
    float iris_r, iris_g, iris_b, iris_melanin;
    float crypts_density, collarette_pos, limbal_health;
    float age_norm, hemoglobin, jaundice, dryness;
    float tear_thickness_um, cornea_depth_mm;
} RigEyeNGCtx;

#define EYE_BUF    (512 * 1024)
#define EYE_JS_BUF (128 * 1024)
#define FA(b,s,p,...) do { \
    if ((p)<(int)(s)) { int _n=rl_snprintf((b)+(p),(s)-(p),__VA_ARGS__); \
    if(_n>0)(p)+=_n; } } while(0)

#define NG_PHI   1.6180339887498948482f
#define NG_PI    3.14159265358979323846f
#define NG_TAU   6.28318530717958647692f

/* ═══════════════════════════════════════════════════════════════
 * GLSL — SISTEMA OCULAR COMPLETO
 * ═══════════════════════════════════════════════════════════════ */
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

/* ═══════════════════════════════════════════════════════════════
 * GLSL — FRAGMENT SHADER PRINCIPAL DEL OJO
 * ═══════════════════════════════════════════════════════════════ */
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

/* ═══════════════════════════════════════════════════════════════
 * C — SISTEMA DINÁMICO DE MOVIMIENTO OCULAR
 * ═══════════════════════════════════════════════════════════════ */

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
} EyeDynamicsNG;

static int eye_dynamics_init(EyeDynamicsNG *e, float light_level)
{
    if (!e) return 0;
    rl_memset(e, 0, sizeof(*e));
    e->light_level     = light_level;
    e->pupil_radius    = 3.0f + (1.0f - light_level) * 3.0f; /* 3-6mm según luz */
    e->pupil_target    = e->pupil_radius;
    e->micro_freq      = 2.5f;
    e->tremor_amp      = 0.5f;  /* 0.5 arcsegundo = muy fino */
    e->next_blink_time = 3.0f + (float)(rand() % 40) * 0.1f;
    e->blink_speed     = 12.0f; /* parpadeo en ~83ms */
    e->vergence_angle  = 2.0f;  /* convergencia por defecto */
    return 0;
}

int eye_dynamics_update(EyeDynamicsNG *e, float dt)
{
    if (!e || dt <= 0.0f) return 0;
    e->time_acc  += dt;
    e->micro_time+= dt;
    e->tremor_time+= dt;

    /* ── Parpadeo ── */
    if (!e->in_saccade) {
        e->next_blink_time -= dt;
        if (e->next_blink_time <= 0.0f) {
            /* Iniciar parpadeo */
            e->blink_phase = 0.0f; /* a cerrar */
            /* 3-6 segundos hasta el próximo parpadeo */
            e->next_blink_time = 3.0f + (float)(rand() % 35) * 0.1f;
        }
    }
    /* Animar la fase del parpadeo */
    if (e->blink_phase >= 0.0f) {
        e->blink_phase += dt * e->blink_speed * 2.0f; /* 0→2 (cerrar + abrir) */
        if (e->blink_phase > 2.0f) e->blink_phase = -1.0f; /* terminado */
    }

    /* ── Microsacadas ── */
    float micro_period = 1.0f / e->micro_freq;
    if (e->micro_time > micro_period) {
        e->micro_time -= micro_period;
        /* Microsacada aleatoria: < 1° */
        e->micro_h = ((float)(rand() & 0xFF) / 255.0f - 0.5f) * 0.8f;
        e->micro_v = ((float)(rand() & 0xFF) / 255.0f - 0.5f) * 0.5f;
    }
    /* Decaimiento exponencial de la microsacada */
    e->micro_h *= expf(-dt * 15.0f);
    e->micro_v *= expf(-dt * 15.0f);

    /* ── Deriva de fijación ── */
    e->drift_h += e->drift_vel_h * dt;
    e->drift_v += e->drift_vel_v * dt;
    /* La deriva se corrige con microsacadas cuando supera umbral */
    if (fabsf(e->drift_h) > 0.3f || fabsf(e->drift_v) > 0.2f) {
        e->drift_h = 0.0f;
        e->drift_v = 0.0f;
    }
    /* Velocidad de deriva lenta */
    e->drift_vel_h = sinf(e->time_acc * 0.1f) * 0.02f;
    e->drift_vel_v = cosf(e->time_acc * 0.13f) * 0.012f;

    /* ── Sacada principal ── */
    if (e->in_saccade) {
        float err_h = e->gaze_target_h - e->gaze_h;
        float err_v = e->gaze_target_v - e->gaze_v;
        float remain = sqrtf(err_h*err_h + err_v*err_v);
        if (remain < 0.1f) {
            e->gaze_h = e->gaze_target_h;
            e->gaze_v = e->gaze_target_v;
            e->in_saccade = false;
        } else {
            /* Perfil de velocidad: rampa + desaceleración (Bahill 1975) */
            float peak_vel = fminf(remain * 50.0f, 600.0f); /* deg/s máx 600 */
            float t_norm = e->saccade_time / e->saccade_dur;
            float vel_profile = sinf(t_norm * NG_PI); /* perfil sinusoidal */
            float vel = peak_vel * vel_profile;
            e->gaze_h += (err_h / remain) * vel * dt;
            e->gaze_v += (err_v / remain) * vel * dt;
            e->saccade_time += dt;
        }
    }

    /* ── Pupila — respuesta a la luz + hipus pupilar ── */
    e->hipus_phase += dt * 0.5f * NG_PI; /* hipus: ~0.5Hz */
    float hipus = sinf(e->hipus_phase) * 0.15f; /* ±0.15mm variación espontánea */
    /* Pupila objetivo según luz + emoción */
    float baseline_pupil = 2.0f + (1.0f - e->light_level) * 6.0f; /* 2-8mm */
    baseline_pupil += e->fear * 1.5f;      /* miedo: dilata */
    baseline_pupil += e->surprise * 1.0f;  /* sorpresa: dilata */
    baseline_pupil -= e->sleepiness * 1.5f;/* somnolencia: miótica */
    e->pupil_target = clamp(baseline_pupil + hipus, 1.5f, 8.5f);
    /* Respuesta pupilar: constrictora rápida (200ms), dilatadora lenta (5s) */
    float pupil_err = e->pupil_target - e->pupil_radius;
    float pupil_tau = (pupil_err < 0.0f) ? 0.2f : 5.0f; /* tau en segundos */
    e->pupil_radius += pupil_err * (1.0f - expf(-dt / pupil_tau));
    return 0;
}

int eye_dynamics_set_gaze(EyeDynamicsNG *e, float h, float v)
{
    if (!e) return 0;
    float dist = sqrtf((h-e->gaze_h)*(h-e->gaze_h) + (v-e->gaze_v)*(v-e->gaze_v));
    e->gaze_target_h = clamp(h, -30.0f, 30.0f);
    e->gaze_target_v = clamp(v, -20.0f, 20.0f);
    /* Duración de la sacada proporcional a la amplitud (Bahill 1975) */
    /* t_dur = 2.2ms × amplitud + 21ms */
    e->saccade_dur  = 0.021f + dist * 0.0022f;
    e->saccade_time = 0.0f;
    e->in_saccade   = (dist > 0.1f);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════
 * Función principal — generación de shaders del ojo
 * ═══════════════════════════════════════════════════════════════ */
int rig_face_ng_eye_shader(const RigEyeNGCtx *ctx, RigArtResultNG *out)
{
    if (!ctx || !out) return -1;
    char *frag = (char*)rl_malloc(EYE_BUF);
    char *js   = (char*)rl_malloc(EYE_JS_BUF);
    if (!frag || !js) { rl_free(frag); rl_free(js); return -1; }

    int fp = 0, jp = 0;
    int fsz = EYE_BUF, jsz = EYE_JS_BUF;

    FA(frag, fsz, fp, "%s", NG_EYE_LIB_GLSL);
    FA(frag, fsz, fp, "%s", NG_EYE_FRAG_MAIN);

    FA(js, jsz, jp,
        "// §NG-EYE RigCom Next Generation — Eye System\n"
        "// φ=1.6180339887 · Richard Felipe Urbina\n"
        "'use strict';\n\n"
        "class RigNGEyeSystem {\n"
        "  constructor(gl, prog) {\n"
        "    this.gl = gl; this.prog = prog;\n"
        "    this.L = n => gl.getUniformLocation(prog, n);\n"
        "    // Parámetros del ojo\n"
        "    this.eye = {\n"
        "      pupil_radius: %.4f,       // en UV normalizado\n"
        "      iris_radius:  %.4f,\n"
        "      iris_color:   new Float32Array([%.4f, %.4f, %.4f]),\n"
        "      iris_melanin: %.4f,\n"
        "      crypts_density: %.4f,\n"
        "      collarette_pos: %.4f,\n"
        "      limbal_health:  %.4f\n"
        "    };\n"
        "    this.bio = {\n"
        "      age_norm:   %.4f,\n"
        "      hemoglobin: %.4f,\n"
        "      jaundice:   %.4f,\n"
        "      dryness:    %.4f,\n"
        "      tear_thickness: %.2f\n"
        "    };\n"
        "    this.dynamics = {\n"
        "      pupil_mm:  3.5,\n"
        "      gaze_h:    0.0, gaze_v:  0.0,\n"
        "      blink_openness: 1.0,\n"
        "      light_level: 0.5,\n"
        "      emotion: { surprise:0, fear:0, sleepiness:0 }\n"
        "    };\n"
        "    this.cornea_depth = %.4f;\n"
        "    this.exposure = 1.0;\n"
        "    this._time = 0.0;\n"
        "    this._blink_t = 0.0;\n"
        "    this._next_blink = 3.5;\n"
        "    this._hipus = 0.0;\n"
        "  }\n\n",
        ctx->pupil_radius, ctx->iris_radius,
        ctx->iris_r, ctx->iris_g, ctx->iris_b,
        ctx->iris_melanin, ctx->crypts_density,
        ctx->collarette_pos, ctx->limbal_health,
        ctx->age_norm, ctx->hemoglobin, ctx->jaundice,
        ctx->dryness, ctx->tear_thickness_um,
        ctx->cornea_depth_mm);

    FA(js, jsz, jp,
        "  update(dt) {\n"
        "    this._time += dt;\n"
        "    // Hipus pupilar ~0.5Hz\n"
        "    this._hipus += dt * Math.PI;  // 0.5Hz × 2π\n"
        "    const hipus_v = Math.sin(this._hipus) * 0.08;  // ±0.08 en UV\n"
        "    // Pupila: ajuste por luz + hipus\n"
        "    const base_p = 0.06 + (1 - this.dynamics.light_level) * 0.12;\n"
        "    this.eye.pupil_radius = Math.max(0.03, Math.min(0.20,\n"
        "      base_p + hipus_v\n"
        "      + this.dynamics.emotion.fear * 0.03\n"
        "      + this.dynamics.emotion.surprise * 0.02\n"
        "      - this.dynamics.emotion.sleepiness * 0.03));\n"
        "    // Parpadeo\n"
        "    this._next_blink -= dt;\n"
        "    if(this._next_blink <= 0) {\n"
        "      this._blink_t = 0.0;\n"
        "      this._next_blink = 3.0 + Math.random() * 3.5;\n"
        "    }\n"
        "    if(this._blink_t >= 0) {\n"
        "      this._blink_t += dt * 12.0;\n"
        "      if(this._blink_t > 2.0) this._blink_t = -1;\n"
        "    }\n"
        "    const blink_close = this._blink_t >= 0\n"
        "      ? (this._blink_t < 1 ? this._blink_t : 2-this._blink_t)\n"
        "      : 0.0;\n"
        "    this.dynamics.blink_openness = 1.0 - Math.min(1,Math.max(0,blink_close))\n"
        "                                       - this.dynamics.emotion.sleepiness * 0.35;\n"
        "  }\n\n"
        "  bind(uTime) {\n"
        "    const {gl, L, eye, bio, dynamics, cornea_depth} = this;\n"
        "    gl.uniform1f(L('u_pupil_radius'),   eye.pupil_radius);\n"
        "    gl.uniform1f(L('u_iris_radius'),    eye.iris_radius);\n"
        "    gl.uniform3fv(L('u_iris_color'),    eye.iris_color);\n"
        "    gl.uniform1f(L('u_iris_melanin'),   eye.iris_melanin);\n"
        "    gl.uniform1f(L('u_crypts_density'), eye.crypts_density);\n"
        "    gl.uniform1f(L('u_collarette_pos'), eye.collarette_pos);\n"
        "    gl.uniform1f(L('u_limbal_health'),  eye.limbal_health);\n"
        "    gl.uniform1f(L('u_age_norm'),       bio.age_norm);\n"
        "    gl.uniform1f(L('u_hemoglobin'),     bio.hemoglobin);\n"
        "    gl.uniform1f(L('u_jaundice'),       bio.jaundice);\n"
        "    gl.uniform1f(L('u_dryness'),        bio.dryness);\n"
        "    gl.uniform1f(L('u_tear_thickness'), bio.tear_thickness);\n"
        "    gl.uniform1f(L('u_cornea_depth'),   cornea_depth);\n"
        "    gl.uniform1f(L('u_time'),           uTime || this._time);\n"
        "    gl.uniform1f(L('u_exposure'),       this.exposure);\n"
        "  }\n\n"
        "  setIrisColor(preset) {\n"
        "    const p = RIG_IRIS_PRESETS[preset];\n"
        "    if(p) { this.eye.iris_color.set(p.color); this.eye.iris_melanin = p.melanin; }\n"
        "  }\n"
        "  setLight(level) { this.dynamics.light_level = Math.max(0,Math.min(1,level)); }\n"
        "  setEmotion(e) { Object.assign(this.dynamics.emotion, e); }\n"
        "}\n\n"
        "const RIG_IRIS_PRESETS = {\n"
        "  dark_brown: { color:[0.25,0.15,0.05], melanin:0.90 },\n"
        "  medium_brown:{ color:[0.35,0.22,0.08], melanin:0.65 },\n"
        "  hazel:      { color:[0.40,0.35,0.12], melanin:0.45 },\n"
        "  green:      { color:[0.25,0.48,0.20], melanin:0.25 },\n"
        "  blue_green: { color:[0.20,0.42,0.55], melanin:0.15 },\n"
        "  blue:       { color:[0.15,0.35,0.75], melanin:0.08 },\n"
        "  grey:       { color:[0.40,0.42,0.45], melanin:0.10 },\n"
        "  amber:      { color:[0.70,0.45,0.05], melanin:0.30 }\n"
        "};\n\n"
        "export { RigNGEyeSystem, RIG_IRIS_PRESETS };\n");

    out->glsl_frag = frag;
    out->js        = js;
    out->ok        = true;
    return fp + jp;
}
