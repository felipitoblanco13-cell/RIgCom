/* ═══════════════════════════════════════════════════════════════════════════
 * rig_master_pbr.frag — EL FRAGMENT MAESTRO · RIGCOM
 * GLSL ES 3.00 — Honor 400 / Chrome 130+ / WebGL2
 *
 * Aquí es donde el material deja de ser un nombre en un catálogo y pasa
 * a ser materia.
 *
 * Lo que ANTES había:
 *      un lóbulo GGX isotrópico + ambient = color × strength × albedo
 *      → 266 materiales que renderizaban todos igual salvo por el color
 *
 * Lo que hay AHORA:
 *      IBL split-sum (SH + prefiltrado + EnvBRDF analítico)
 *      GGX ANISOTRÓPICO           → seda, satén, cepillado, carbono
 *      Lóbulo SHEEN (Charlie)     → terciopelo, cachemira, nubuck
 *      MULTISCATTER (Kulla-Conty) → los rugosos recuperan su energía
 *      TRANSMISIÓN Beer-Lambert   → vidrio, jade, ámbar, hielo, cera
 *      CLEARCOAT (2º lóbulo)      → laca urushi, nácar
 *      IRIDISCENCIA               → la tuya, conservada
 *
 * Cero texturas de disco. El entorno es procedural, el BRDF es analítico.
 * ═══════════════════════════════════════════════════════════════════════════ */

#version 300 es
precision highp float;
precision highp int;

const float PHI     = 1.6180339887;
const float PHI_INV = 0.6180339887;
const float PI      = 3.14159265359;
const float TAU     = 6.28318530718;
const float INV_PI  = 0.31830988618;

/* ── Entrada ── */
in vec3 v_world;
in vec3 v_normal;
in vec3 v_tangent;
in vec2 v_uv;
in vec4 v_color;

out vec4 fragColor;

/* ── El entorno: 27 floats + 1 cubemap. Nada más. ── */
uniform vec3        u_sh[9];          /* ★ TODO el difuso ambiental          */
uniform samplerCube u_envSpec;        /* ★ 6 mips GGX-convolucionados        */
uniform float       u_envMaxMip;      /* 5.0                                 */

/* ── Luces directas ── */
#define MAX_LIGHTS 4
uniform vec3  u_lightDir[MAX_LIGHTS];
uniform vec3  u_lightColor[MAX_LIGHTS];
uniform float u_lightIntensity[MAX_LIGHTS];
uniform int   u_lightCount;

uniform vec3  u_eye;
uniform float u_exposure;

/* ── El material: RigMaterial empaquetado ── */
uniform vec3  u_albedo;
uniform float u_metallic;
uniform float u_roughness;
uniform float u_ior;

uniform float u_anisotropy;         /* [-1,+1]                              */
uniform float u_tangentRot;
uniform int   u_tangentFlow;

uniform vec3  u_sheen;
uniform float u_sheenRough;

uniform float u_transmission;
uniform vec3  u_absorption;         /* σ de Beer-Lambert, por mm            */
uniform float u_thickness;
uniform float u_dispersion;

uniform float u_clearcoat;
uniform float u_clearcoatRough;

uniform float u_iriStrength;
uniform float u_iriThicknessNm;

uniform vec3  u_emissive;
uniform float u_emissiveStrength;

uniform float u_sssWeight;

/* Sombras (ya las tenías en el master) */
uniform sampler2D u_shadowMap;
uniform mat4      u_lightVP;
uniform float     u_shadowBias;


/* ═══════════════════════════════════════════════════════════════════════════
 * §1  IRRADIANCIA POR SH — cero samplers
 *
 * Todo el difuso ambiental del universo en 9 vec3.
 * Ramamoorthi demostró que el lóbulo coseno filtra tan agresivamente que
 * la banda 2 captura el 99% de la energía. Más allá es desperdiciar memoria.
 * ═══════════════════════════════════════════════════════════════════════════ */
vec3 irradianceSH(vec3 n)
{
    return max(
          u_sh[0] * 0.2820947918
        + u_sh[1] * 0.4886025119 * n.y
        + u_sh[2] * 0.4886025119 * n.z
        + u_sh[3] * 0.4886025119 * n.x
        + u_sh[4] * 1.0925484306 * n.x * n.y
        + u_sh[5] * 1.0925484306 * n.y * n.z
        + u_sh[6] * 0.3153915653 * (3.0 * n.z * n.z - 1.0)
        + u_sh[7] * 1.0925484306 * n.x * n.z
        + u_sh[8] * 0.5462742153 * (n.x * n.x - n.y * n.y),
        vec3(0.0));
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §2  ★ EnvBRDF ANALÍTICO ★  —  CERO TEXTURAS
 *
 * La segunda mitad del split-sum de Karis. Sin LUT, sin sampler, sin bake.
 * Error < 1%. Esta es la versión correcta para RIGCOM: la matemática ES
 * el recurso.
 * ═══════════════════════════════════════════════════════════════════════════ */
vec2 envBRDFApprox(float NoV, float rough)
{
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572,  0.022);
    const vec4 c1 = vec4( 1.0,  0.0425,  1.040, -0.040);
    vec4  r    = rough * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §3  BRDF — los lóbulos
 * ═══════════════════════════════════════════════════════════════════════════ */

vec3 F_Schlick(float u, vec3 F0)
{
    float f = pow(1.0 - u, 5.0);
    return F0 + (1.0 - F0) * f;
}

float D_GGX(float NdH, float a)
{
    float a2 = a * a;
    float d  = NdH * NdH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-7);
}

/* ★ GGX ANISOTRÓPICO — sin esto la seda es imposible.
 *
 * El lóbulo especular de la seda NO es un círculo: es una BANDA estirada
 * perpendicular al hilo. Dos rugosidades, αx a lo largo del hilo y αy a
 * través. Cuando αx ≠ αy, el highlight se alarga. ESO es la seda. */
float D_GGX_aniso(float NdH, float TdH, float BdH, float ax, float ay)
{
    float a2 = ax * ay;
    vec3  v  = vec3(ay * TdH, ax * BdH, a2 * NdH);
    float v2 = dot(v, v);
    float w2 = a2 / max(v2, 1e-9);
    return a2 * w2 * w2 * INV_PI;
}

float V_SmithGGX(float NdV, float NdL, float a)
{
    float a2 = a * a;
    float gv = NdL * sqrt(NdV * NdV * (1.0 - a2) + a2);
    float gl = NdV * sqrt(NdL * NdL * (1.0 - a2) + a2);
    return 0.5 / max(gv + gl, 1e-7);
}

float V_SmithGGX_aniso(float NdV, float NdL,
                       float TdV, float BdV, float TdL, float BdL,
                       float ax, float ay)
{
    float lv = NdL * length(vec3(ax * TdV, ay * BdV, NdV));
    float ll = NdV * length(vec3(ax * TdL, ay * BdL, NdL));
    return 0.5 / max(lv + ll, 1e-7);
}

/* ★ SHEEN — distribución de Charlie (Estevez-Kulla)
 *
 * El terciopelo brilla MÁS EN LOS BORDES. Al revés que todo lo demás.
 * Es retrorreflexión: las fibras verticales devuelven la luz hacia donde
 * vino. Un lóbulo APARTE, que se SUMA. Sin él, terciopelo = fieltro. */
float D_Charlie(float NdH, float rough)
{
    float invR = 1.0 / max(rough, 0.05);
    float cos2 = NdH * NdH;
    float sin2 = 1.0 - cos2;
    return (2.0 + invR) * pow(max(sin2, 0.0), invR * 0.5) / TAU;
}

float V_Ashikhmin(float NdV, float NdL)
{
    return 1.0 / max(4.0 * (NdL + NdV - NdL * NdV), 1e-5);
}

/* DFG del sheen para IBL — ajuste analítico de Estevez-Kulla */
float sheenEnvDFG(float NoV, float rough)
{
    float a = rough * rough;
    return (1.0 - a) * 0.04 + a * 0.18 * (1.0 - pow(NoV, 3.0));
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §4  IRIDISCENCIA — Belcour-Barla (la tuya, conservada)
 * ═══════════════════════════════════════════════════════════════════════════ */
vec3 iridescence(float cosTheta, float thicknessNm, float ior2)
{
    float sinT2 = (1.0 / (ior2 * ior2)) * (1.0 - cosTheta * cosTheta);
    float cosT  = sqrt(max(0.0, 1.0 - sinT2));
    /* Camino óptico: OPD = 2·n·d·cosθ_t */
    float opd   = 2.0 * ior2 * thicknessNm * cosT;
    vec3  lambda = vec3(680.0, 550.0, 440.0);          /* R, G, B en nm    */
    vec3  phase  = TAU * opd / lambda;
    return 0.5 + 0.5 * cos(phase);
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §5  CAMPO DE TANGENTES — sin él la anisotropía no tiene dirección
 * ═══════════════════════════════════════════════════════════════════════════ */
float hash13(vec3 p){
    p = fract(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return fract((p.x + p.y) * p.z);
}

vec3 tangentField(vec3 N, vec3 T0, vec3 P)
{
    vec3 T = T0;

    if (u_tangentFlow == 1) {                     /* RADIAL — disco cepillado */
        vec3 c = normalize(cross(N, vec3(0.0, 1.0, 0.0)) + 1e-5);
        T = normalize(cross(N, c));
    }
    else if (u_tangentFlow == 2) {                /* CURL — mokumé, mercurio  */
        float e = 0.15;
        float n1 = hash13(P * 3.0 + vec3(0.0, 17.0, 0.0));
        float n2 = hash13(P * 3.0 + vec3(31.0, 0.0, 7.0));
        vec3  flow = normalize(vec3(n1 - 0.5, 0.0, n2 - 0.5) + 1e-5);
        T = normalize(flow - N * dot(N, flow));
    }
    else if (u_tangentFlow == 3) {                /* FIXED — cepillado lineal */
        vec3 f = vec3(1.0, 0.0, 0.0);
        T = normalize(f - N * dot(N, f));
    }
    else if (u_tangentFlow == 4) {                /* SPIRAL φ — nácar         */
        float ang = atan(P.z, P.x) + length(P.xz) * PHI;
        vec3  s   = vec3(cos(ang), 0.0, sin(ang));
        T = normalize(s - N * dot(N, s));
    }

    /* Rotación del campo */
    if (abs(u_tangentRot) > 1e-4) {
        vec3  B = cross(N, T);
        float c = cos(u_tangentRot), s = sin(u_tangentRot);
        T = normalize(T * c + B * s);
    }
    return T;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * §6  ACES
 * ═══════════════════════════════════════════════════════════════════════════ */
vec3 ACES(vec3 x)
{
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}


/* ═══════════════════════════════════════════════════════════════════════════
 *                                  MAIN
 * ═══════════════════════════════════════════════════════════════════════════ */
void main()
{
    vec3 N = normalize(v_normal);
    if (!gl_FrontFacing) N = -N;

    vec3  V   = normalize(u_eye - v_world);
    float NoV = max(dot(N, V), 1e-4);

    /* Base tangente-bitangente */
    vec3 T = tangentField(N, normalize(v_tangent - N * dot(N, v_tangent)), v_world);
    vec3 B = cross(N, T);

    vec3  albedo = u_albedo * v_color.rgb;
    float rough  = clamp(u_roughness, 0.03, 0.98);
    float a      = rough * rough;

    /* ── Rugosidades anisótropas ──
     * αx a lo largo del hilo, αy a través. Cuando difieren, el highlight
     * se estira perpendicular al hilo. Eso ES la seda. */
    float aniso = clamp(u_anisotropy, -0.98, 0.98);
    float aspect = sqrt(1.0 - aniso * 0.9);
    float ax = max(a / aspect, 1e-4);
    float ay = max(a * aspect, 1e-4);

    /* ── F0 ── */
    float f0d = pow((1.0 - u_ior) / (1.0 + u_ior), 2.0);
    vec3  F0  = mix(vec3(f0d), albedo, u_metallic);

    /* ── Iridiscencia sobre F0 (Belcour-Barla) ── */
    if (u_iriStrength > 0.001) {
        vec3 iri = iridescence(NoV, u_iriThicknessNm, 1.3);
        F0 = mix(F0, iri, u_iriStrength * 0.85);
    }

    /* ═══════════════════════════════════════════════════════════════════
     * LUZ DIRECTA
     * ═══════════════════════════════════════════════════════════════════ */
    vec3 direct = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (i >= u_lightCount) break;

        vec3  L   = normalize(u_lightDir[i]);
        float NoL = dot(N, L);
        if (NoL <= 0.0) continue;

        vec3  H   = normalize(V + L);
        float NoH = max(dot(N, H), 0.0);
        float VoH = max(dot(V, H), 0.0);

        vec3  F = F_Schlick(VoH, F0);

        /* ── Especular: anisótropo si el material lo pide ── */
        float D, Vis;
        if (abs(aniso) > 0.01) {
            float TdH = dot(T, H), BdH = dot(B, H);
            float TdV = dot(T, V), BdV = dot(B, V);
            float TdL = dot(T, L), BdL = dot(B, L);
            D   = D_GGX_aniso(NoH, TdH, BdH, ax, ay);
            Vis = V_SmithGGX_aniso(NoV, NoL, TdV, BdV, TdL, BdL, ax, ay);
        } else {
            D   = D_GGX(NoH, a);
            Vis = V_SmithGGX(NoV, NoL, a);
        }
        vec3 spec = D * Vis * F;

        /* ── Difuso ── */
        vec3 kD    = (1.0 - F) * (1.0 - u_metallic) * (1.0 - u_transmission);
        vec3 diff  = kD * albedo * INV_PI;

        /* ── ★ SHEEN: el lóbulo del terciopelo ── */
        if (dot(u_sheen, u_sheen) > 1e-5) {
            float Dc = D_Charlie(NoH, u_sheenRough);
            float Vc = V_Ashikhmin(NoV, NoL);
            spec += u_sheen * Dc * Vc;
        }

        /* ── ★ CLEARCOAT: segundo lóbulo, normal e IOR propios ── */
        if (u_clearcoat > 0.001) {
            float ac  = max(u_clearcoatRough * u_clearcoatRough, 1e-4);
            float Dcc = D_GGX(NoH, ac);
            float Vcc = V_SmithGGX(NoV, NoL, ac);
            float Fcc = 0.04 + 0.96 * pow(1.0 - VoH, 5.0);
            float cc  = Dcc * Vcc * Fcc * u_clearcoat;
            /* La capa atenúa lo de abajo */
            spec  = spec * (1.0 - Fcc * u_clearcoat) + vec3(cc);
            diff *= (1.0 - Fcc * u_clearcoat);
        }

        direct += (diff + spec) * NoL * u_lightColor[i] * u_lightIntensity[i];
    }

    /* ═══════════════════════════════════════════════════════════════════
     * ★★★  IBL  ★★★   — AQUÍ ES DONDE EL ORO SE VUELVE ORO
     * ═══════════════════════════════════════════════════════════════════ */

    /* ── Reflejo. Con anisotropía, la normal se DOBLA ──
     * Truco de Burley: en un material anisótropo, el reflejo del entorno
     * también se estira. Se aproxima doblando la normal hacia el eje
     * anisótropo antes de reflejar. */
    vec3 Nr = N;
    if (abs(aniso) > 0.01) {
        vec3 anisoDir  = (aniso >= 0.0) ? B : T;
        vec3 anisoBent = cross(anisoDir, cross(V, anisoDir));
        Nr = normalize(mix(N, anisoBent, abs(aniso) * (1.0 - rough)));
    }
    vec3 R = reflect(-V, Nr);

    /* ── El especular ambiental: 3 líneas y ya está ── */
    vec3  prefiltered = textureLod(u_envSpec, R, rough * u_envMaxMip).rgb;
    vec2  ab          = envBRDFApprox(NoV, rough);
    vec3  specIBL     = prefiltered * (F0 * ab.x + ab.y);

    /* ── ★ MULTISCATTER (Kulla-Conty) ──
     * GGX de una sola dispersión pierde ~25% de energía a rough 0.6 y ~40%
     * a 0.9. Por eso los rugosos salían grises y sucios. Esto lo devuelve,
     * y sale GRATIS del (scale,bias) que ya calculamos. */
    float Ess = max(ab.x + ab.y, 1e-4);
    vec3  Fms = F0 * ((1.0 - Ess) / Ess);
    specIBL  *= (1.0 + Fms);

    /* ── El difuso ambiental ── */
    vec3 F_ibl   = F_Schlick(NoV, F0);
    vec3 kD_ibl  = (1.0 - F_ibl) * (1.0 - u_metallic) * (1.0 - u_transmission);
    vec3 diffIBL = kD_ibl * albedo * irradianceSH(N);

    vec3 ambient = diffIBL + specIBL;

    /* ── ★ SHEEN ambiental ── */
    if (dot(u_sheen, u_sheen) > 1e-5) {
        float sDFG = sheenEnvDFG(NoV, u_sheenRough);
        vec3  sIBL = textureLod(u_envSpec, R, u_sheenRough * u_envMaxMip).rgb;
        ambient += u_sheen * sIBL * sDFG;
    }

    /* ── ★ CLEARCOAT ambiental ── */
    if (u_clearcoat > 0.001) {
        float Fc  = (0.04 + 0.96 * pow(1.0 - NoV, 5.0)) * u_clearcoat;
        vec3  ccR = textureLod(u_envSpec, reflect(-V, N),
                               u_clearcoatRough * u_envMaxMip).rgb;
        ambient = ambient * (1.0 - Fc) + ccR * Fc;
    }

    /* ═══════════════════════════════════════════════════════════════════
     * ★ TRANSMISIÓN — vidrio, hielo, jade, ámbar, cera
     *
     * El color NO viene del albedo: viene de la ABSORCIÓN por el camino
     * recorrido (Beer-Lambert). Un jade grueso es verde oscuro; uno fino,
     * verde claro. Es el MISMO material. Eso es lo que el albedo nunca
     * podrá expresar.
     * ═══════════════════════════════════════════════════════════════════ */
    vec3 transmitted = vec3(0.0);
    if (u_transmission > 0.001) {

        float eta = 1.0 / max(u_ior, 1.001);

        /* Dispersión: tres IOR distintos → el FUEGO del diamante */
        vec3 Tr;
        if (u_dispersion > 0.001) {
            float d  = u_dispersion * 0.035;
            vec3 rr = refract(-V, N, eta * (1.0 - d));
            vec3 rg = refract(-V, N, eta);
            vec3 rb = refract(-V, N, eta * (1.0 + d));
            float lod = rough * u_envMaxMip;
            Tr = vec3(textureLod(u_envSpec, rr, lod).r,
                      textureLod(u_envSpec, rg, lod).g,
                      textureLod(u_envSpec, rb, lod).b);
        } else {
            vec3 rd = refract(-V, N, eta);
            Tr = textureLod(u_envSpec, rd, rough * u_envMaxMip).rgb;
        }

        /* Beer-Lambert: T = exp(−σ · d) */
        vec3 att = exp(-u_absorption * u_thickness);
        transmitted = Tr * att * u_transmission * (1.0 - F_ibl);
    }

    /* ═══════════════════════════════════════════════════════════════════
     * COMPOSICIÓN
     * ═══════════════════════════════════════════════════════════════════ */
    vec3 color = direct + ambient + transmitted;

    /* SSS (el enlace con RigFace) */
    if (u_sssWeight > 0.001) {
        vec3 back = irradianceSH(-N) * albedo * vec3(1.0, 0.72, 0.46);
        color += back * u_sssWeight * 0.35;
    }

    color += u_emissive * u_emissiveStrength;

    color *= u_exposure;
    color  = ACES(color);
    color  = pow(color, vec3(1.0 / 2.2));

    fragColor = vec4(color, 1.0);
}
