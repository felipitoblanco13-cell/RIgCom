/*
 * rig_face_ng_anim.c — RigCom v24 NEXT GENERATION · Motor de Animación Facial
 * ══════════════════════════════════════════════════════════════════════════════
 * Richard Felipe Urbina · RIGCOM Ecosystem · Arquitecto Soberano
 *
 * SISTEMA DE ANIMACIÓN FACIAL DE PRÓXIMA GENERACIÓN:
 *
 *   FACS EXTENDIDO (128 Action Units):
 *   ├── 52 AU canónicos de Ekman/Friesen
 *   ├── 30 AU de alta definición (detalles sub-musculares)
 *   ├── 26 AU de asimetría bilateral (expresión real ≠ perfecta)
 *   ├── 20 AU de micro-expresiones (6-200ms duración)
 *   └── Blending temporal con curvas Bezier de control
 *
 *   SIMULACIÓN MUSCULAR ANATÓMICA:
 *   ├── 43 músculos faciales activos modelados
 *   ├── Fibras: origen, inserción, vector de contracción
 *   ├── Deformación: lattice 3D controlada por músculo
 *   ├── Antagonistas: pares agonista-antagonista
 *   ├── Fatiga muscular: reducción de amplitud con tiempo
 *   └── Tono basal: tensión mínima en reposo (0.05-0.12)
 *
 *   MICRO-EXPRESIONES:
 *   ├── 7 emociones base (Ekman 1969)
 *   ├── 14 emociones compuestas (PAD model)
 *   ├── Micro-expresiones auténticas (<200ms, involuntarias)
 *   ├── Leakage: fuga emocional en expresión controlada
 *   ├── Asimetría: control bilateral independiente
 *   └── Timing: onset 0-30ms / apex / offset curvas
 *
 *   FONEMAS Y VISEMAS:
 *   ├── 44 fonemas del inglés (IPA completo)
 *   ├── 16 fonemas adicionales (español, portugués, francés)
 *   ├── 22 visemas base + 18 demi-visemas de transición
 *   ├── Coarticulación: influencia de fonemas adyacentes
 *   ├── Jaw dynamics: masa+resorte para mandíbula
 *   ├── Lip sync: anticipación 60-80ms (precondicionamiento)
 *   └── Sincronización a audio FFT en tiempo real
 *
 *   FÍSICA SECUNDARIA:
 *   ├── Tejido graso: masa+resorte con amortiguamiento crítico
 *   ├── Piel: wave propagation al colisionar
 *   ├── Papada: péndulo doble con restricción de volumen
 *   ├── Mejillas: desplazamiento orbital con compresión
 *   └── Orbiculares: contracción real del párpado
 *
 *   MOVIMIENTO OCULAR FISIOLÓGICO:
 *   ├── Sacadas: 250-600°/s, duración 10-80ms
 *   ├── Microsacadas: 10-120°/s, cada 1-4s
 *   ├── Drift: movimiento suave + ruido Browniano
 *   ├── Tremor: oscilación 80-100Hz (imperceptible, pero real)
 *   ├── Vergencia: convergencia/divergencia binocular
 *   └── Reflejo vestíbulo-ocular (VOR)
 *
 *   RESPIRACIÓN Y LATIDO:
 *   ├── Ciclo respiratorio 16 fases (12-20 rpm)
 *   ├── Efectos: narinas, labios, cuello, hombros
 *   ├── Pulso cardíaco 70bpm en temples/cuello/labios
 *   └── Sincronización latido-respiración (armonía HRV)
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

#define NG_PHI      1.6180339887498948482f
#define NG_PI       3.14159265358979323846f
#define NG_TAU      6.28318530717958647692f
#define ANIM_BUF    (320 * 1024)
#define FA(b,s,p,...) do { \
    if((p)<(int)(s)){int _n=rl_snprintf((b)+(p),(s)-(p),__VA_ARGS__); \
    if(_n>0)(p)+=_n;} } while(0)

/* ════════════════════════════════════════════════════════════════
 * TABLA COMPLETA DE 128 ACTION UNITS — FACS EXTENDIDO NG
 * ════════════════════════════════════════════════════════════════ */

/* Estructura de Action Unit individual */
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
} FacsAU_NG;

/* Los 128 AU definidos con datos anatómicos reales */
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

/* ════════════════════════════════════════════════════════════════
 * TABLA DE 44 FONEMAS + 22 VISEMAS — Sistema de lip sync
 * ════════════════════════════════════════════════════════════════ */

/* Tipo de fonema (clasificación IPA) */
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
} PhonemeCat;

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
} Phoneme_NG;

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

/* ════════════════════════════════════════════════════════════════
 * EMOCIONES COMPUESTAS — PAD Model (Pleasure-Arousal-Dominance)
 * ════════════════════════════════════════════════════════════════ */
typedef struct {
    char  name[32];
    float P;              /* Pleasure     -1..+1 */
    float A;              /* Arousal      -1..+1 */
    float D;              /* Dominance    -1..+1 */
    float au_weights[128];/* pesos para los 128 AU */
    float duration_peak_s;/* duración típica del apex en segundos */
    float decay_s;        /* tiempo de decay */
} EmotionState;

/* ════════════════════════════════════════════════════════════════
 * NÚCLEO DEL MOTOR — AnimState y sistemas de física
 * ════════════════════════════════════════════════════════════════ */

/* Estado interno del motor de animación */
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

} AnimState_NG;

/* ════════════════════════════════════════════════════════════════
 * GLSL — VERTEX SHADER DE ANIMACIÓN (morph + skeleton)
 * ════════════════════════════════════════════════════════════════ */
static const char *NG_ANIM_GLSL_VERT =
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
    "/* ─────────────────────────────────────────────────────────── */\n"
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
    "    float age_sq = u_age_norm * u_age_norm;\n"
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
    "    /* ═══════════════════════════════════════════════════════\n"
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
    "}\n";

/* ════════════════════════════════════════════════════════════════
 * MOTOR DE ANIMACIÓN JS — Runtime completo
 * ════════════════════════════════════════════════════════════════ */
static const char *NG_ANIM_JS_RUNTIME =
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
    "  update(dt) {\n"
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
    "  _updateBlink(dt) {\n"
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
    "  _updateBreath(dt) {\n"
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
    "  _updateLipSync(dt) {\n"
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
    "  setLeakage(emotion_name, intensity=0.5) {\n"
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
    "}\n\n"
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
    "export { RigNGAnimEngine, RIG_EMOTIONS };\n";

/* ════════════════════════════════════════════════════════════════
 * rig_face_ng_anim_generate__rig_dup_062ff6e7() — Función principal de generación
 * ════════════════════════════════════════════════════════════════ */
int rig_face_ng_anim_generate__rig_dup_062ff6e7(const RigFaceNGAnimCtx *ctx, RigArtResultNG *out)
{
    if (!ctx || !out) return -1;

    char *js   = (char*)rl_malloc(ANIM_BUF);
    char *vert = (char*)rl_malloc(ANIM_BUF / 2);
    if (!js || !vert) { rl_free(js); rl_free(vert); return -1; }
    int jp=0, vp=0;
    int jsz=ANIM_BUF, vsz=ANIM_BUF/2;

    /* ── FACS Table JS export ── */
    FA(js, jsz, jp, "// §NG-ANIM — FACS Table Export (128 AU)\n");
    FA(js, jsz, jp, "const RIG_FACS_TABLE = [\n");
    for (int i = 0; i < 128; i++) {
        const FacsAU_NG *au = &NG_FACS_TABLE[i];
        if (au->id == 0 && i > 0) break;
        FA(js, jsz, jp,
           "  { idx:%d, id:%d, name:\"%s\", muscle:\"%s\",\n"
           "    emotion:\"%s\", latency:%.1f, peak:%.1f, offset:%.1f,\n"
           "    bilateral:%.1f, intensity_max:%.2f, region:'%c',\n"
           "    micro:%s, asym:%.2f },\n",
           i, au->id, au->name, au->muscle,
           au->emotion, au->latency_ms, au->peak_ms, au->offset_ms,
           au->bilateral, au->intensity_max, au->region,
           au->micro ? "true" : "false", au->asym_factor);
    }
    FA(js, jsz, jp, "];\n\n");

    /* ── FACS ID→index map ── */
    FA(js, jsz, jp, "const RIG_FACS_ID_MAP = {\n");
    for (int i = 0; i < 128; i++) {
        const FacsAU_NG *au = &NG_FACS_TABLE[i];
        if (au->id == 0 && i > 0) break;
        FA(js, jsz, jp, "  %d: %d,\n", au->id, i);
    }
    FA(js, jsz, jp, "};\n\n");

    /* ── Phoneme Table JS export ── */
    FA(js, jsz, jp, "const RIG_PHONEME_TABLE = [\n");
    for (int i = 0; i < 60; i++) {
        const Phoneme_NG *p = &NG_PHONEME_TABLE[i];
        FA(js, jsz, jp,
           "  { id:%d, ipa:\"%s\", ex:\"%s\", viseme:%d,\n"
           "    jaw:%.3f, round:%.3f, spread:%.3f, prot:%.3f,\n"
           "    tongue_h:%.3f, tongue_b:%.3f, teeth:%.3f, velum:%.3f },\n",
           p->id, p->ipa, p->example, p->viseme_id,
           p->jaw_open, p->lip_rounding, p->lip_spreading, p->lip_protrusion,
           p->tongue_height, p->tongue_back, p->teeth_show, p->velum_open);
    }
    FA(js, jsz, jp, "];\n\n");

    /* ── Runtime JS ── */
    FA(js, jsz, jp, "%s", NG_ANIM_JS_RUNTIME);

    /* ── Vertex Shader ── */
    FA(vert, vsz, vp, "// §NG-ANIM Vertex Shader — RigCom NG\n");
    FA(vert, vsz, vp, "%s", NG_ANIM_GLSL_VERT);

    out->js        = js;
    out->glsl_vert = vert;
    out->ok        = true;
    return jp + vp;
}
