/* ==========================================================================
 * 20_rig_face_archetypes.c   —   RIGFACE :: C UNIFICADO
 *
 * Copia canonica : nested/Face_Pose_Body/rig_face_archetypes.c
 * Copias fundidas: 7
 * Funciones      : 15      Unidades injertadas: 3      Variantes: 3
 *
 * Copia canonica preservada verbatim. Ninguna funcion eliminada,
 * reducida ni omitida. Las divergencias de cuerpo bajo un mismo
 * nombre se conservan como <nombre>__vN con su procedencia.
 * ========================================================================== */
#include "rigdeps/rig_std_base.h"
#include "rig_face_engine_v2.h"
#include "rigdeps/stdio.h"
#include "rigdeps/string.h"
#include "rigdeps/math.h"
#include "../include/riglib_math.h"

#define PARAMS_INIT(_cw,_ch,_cd,_phi,_ut,_mt,_lt,                  \
                    _iod,_ew,_od,_bp,                                \
                    _nl,_nw,_ntp,_nbw,                               \
                    _mw,_ltu,_ltl,_jw,_ja,_cp,_chin,                \
                    _zw,_zh,_nkw,_nkl,                               \
                    _mel,_hem,_car,_age,_gen)                         \
{                                                                     \
    .cranium_width       = (_cw),                                     \
    .cranium_height      = (_ch),                                     \
    .cranium_depth       = (_cd),                                     \
    .face_phi_ratio      = (_phi),                                    \
    .upper_third         = (_ut),                                     \
    .mid_third           = (_mt),                                     \
    .lower_third         = (_lt),                                     \
    .interocular_dist    = (_iod),                                    \
    .eye_width           = (_ew),                                     \
    .orbital_depth       = (_od),                                     \
    .brow_protrusion     = (_bp),                                     \
    .nose_length         = (_nl),                                     \
    .nose_width          = (_nw),                                     \
    .nasal_tip_proj      = (_ntp),                                    \
    .nasal_bridge_width  = (_nbw),                                    \
    .mouth_width         = (_mw),                                     \
    .lip_thickness_upper = (_ltu),                                    \
    .lip_thickness_lower = (_ltl),                                    \
    .jaw_width           = (_jw),                                     \
    .jaw_angle           = (_ja),                                     \
    .chin_projection     = (_cp),                                     \
    .chin_height         = (_chin),                                   \
    .zygomatic_width     = (_zw),                                     \
    .zygomatic_height    = (_zh),                                     \
    .neck_width          = (_nkw),                                    \
    .neck_length         = (_nkl),                                    \
    .melanin             = (_mel),                                    \
    .hemoglobin          = (_hem),                                    \
    .carotene            = (_car),                                    \
    .age_factor          = (_age),                                    \
    .gender_factor       = (_gen),                                    \
}

static RigFaceIrisDetail iris_blue_green(void) {
    RigFaceIrisDetail i = {0};
    i.iris_color[0] = 0.35f; i.iris_color[1] = 0.58f; i.iris_color[2] = 0.72f;
    i.limbal_ring_width = 0.8f;  i.limbal_ring_darkness = 0.7f;
    i.pattern = RIG_IRIS_PATTERN_MIXED;   i.pupil_shape = RIG_PUPIL_ROUND;
    i.iris_radius = 6.0f;  i.pupil_radius = 3.2f;
    i.crypt_density = 0.5f;  i.wolfflin_count = 8.0f;  i.furrow_count = 5.0f;
    i.tear_film_thickness = 4000.0f;  i.cornea_roughness = 0.02f;
    return i;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: iris_blue_green -> rigpub_rig_face_archetypes_iris_blue_green */
RigFaceIrisDetail (*rigpub_rig_face_archetypes_iris_blue_green)(void) = iris_blue_green;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */

static RigFaceIrisDetail iris_brown_dark(void) {
    RigFaceIrisDetail i = {0};
    i.iris_color[0] = 0.32f; i.iris_color[1] = 0.18f; i.iris_color[2] = 0.08f;
    i.limbal_ring_width = 1.2f;  i.limbal_ring_darkness = 0.9f;
    i.pattern = RIG_IRIS_PATTERN_CRYPTS;  i.pupil_shape = RIG_PUPIL_ROUND;
    i.iris_radius = 6.2f;  i.pupil_radius = 3.0f;
    i.crypt_density = 0.7f;  i.wolfflin_count = 3.0f;  i.furrow_count = 7.0f;
    i.tear_film_thickness = 4500.0f;  i.cornea_roughness = 0.015f;
    return i;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: iris_brown_dark -> rigpub_rig_face_archetypes_iris_brown_dark */
RigFaceIrisDetail (*rigpub_rig_face_archetypes_iris_brown_dark)(void) = iris_brown_dark;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */

static RigFaceIrisDetail iris_amber_hazel(void) {
    RigFaceIrisDetail i = {0};
    i.iris_color[0] = 0.62f; i.iris_color[1] = 0.42f; i.iris_color[2] = 0.10f;
    i.limbal_ring_width = 1.0f;  i.limbal_ring_darkness = 0.8f;
    i.pattern = RIG_IRIS_PATTERN_WOLFFLIN; i.pupil_shape = RIG_PUPIL_ROUND;
    i.iris_radius = 6.1f;  i.pupil_radius = 3.1f;
    i.crypt_density = 0.4f;  i.wolfflin_count = 14.0f; i.furrow_count = 4.0f;
    i.tear_film_thickness = 4200.0f;  i.cornea_roughness = 0.018f;
    return i;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: iris_amber_hazel -> rigpub_rig_face_archetypes_iris_amber_hazel */
RigFaceIrisDetail (*rigpub_rig_face_archetypes_iris_amber_hazel)(void) = iris_amber_hazel;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */

static RigFaceIrisDetail iris_midnight(void) {
    RigFaceIrisDetail i = {0};
    i.iris_color[0] = 0.08f; i.iris_color[1] = 0.06f; i.iris_color[2] = 0.05f;
    i.limbal_ring_width = 1.5f;  i.limbal_ring_darkness = 0.98f;
    i.pattern = RIG_IRIS_PATTERN_FURROWS;  i.pupil_shape = RIG_PUPIL_ROUND;
    i.iris_radius = 6.3f;  i.pupil_radius = 2.8f;
    i.crypt_density = 0.9f;  i.wolfflin_count = 1.0f;  i.furrow_count = 9.0f;
    i.tear_film_thickness = 5000.0f;  i.cornea_roughness = 0.01f;
    return i;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: iris_midnight -> rigpub_rig_face_archetypes_iris_midnight */
RigFaceIrisDetail (*rigpub_rig_face_archetypes_iris_midnight)(void) = iris_midnight;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static RigFaceEarParams ear_default(bool right) {
    RigFaceEarParams e = {0};
    e.ear_height = 62.0f; e.ear_width = 35.0f; e.ear_protrusion = 20.0f;
    e.helix_width = 0.5f; e.helix_curl = 0.6f; e.antihelix_projection = 0.5f;
    e.antihelix_split = 0.4f; e.tragus_size = 0.5f; e.antitragus_size = 0.4f;
    e.concha_depth = 12.0f; e.concha_width = 18.0f;
    e.lobule_size = 0.4f; e.lobule_attachment = 0.3f;
    e.darwin_tubercle = 0.1f; e.cartilage_ridges = 0.5f;
    e.lobe_crease = 0.2f; e.meatus_diameter = 7.0f;
    e.is_right = right;
    return e;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: ear_default -> rigpub_rig_face_archetypes_ear_default */
RigFaceEarParams (*rigpub_rig_face_archetypes_ear_default)(bool right) = ear_default;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static RigFaceLipDetail lip_neutral(float gender_factor) {
    RigFaceLipDetail l = {0};
    l.cupid_bow_depth  = 0.3f + 0.15f * (1.0f - gender_factor);
    l.cupid_bow_width  = 0.35f;
    l.tuberculum_height= 0.25f;
    l.philtrum_depth   = 2.5f;
    l.philtrum_width   = 11.0f;
    l.philtrum_length  = 14.0f;
    l.upper_lip_height = 8.0f - 2.0f * gender_factor;
    l.lower_lip_height = 10.0f - 1.5f * gender_factor;
    l.lip_protrusion   = 3.5f;
    l.vermilion_height = 6.0f;
    l.vermilion_border_sharpness = 0.7f;
    l.lip_line_density  = 0.5f;
    l.lip_line_depth    = 0.3f;
    l.commissure_depth  = 0.4f;
    l.mentolabial_depth = 0.5f;
    l.vermilion_saturation = 0.6f;
    l.mucosal_visibility   = 0.2f;
    return l;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: lip_neutral -> rigpub_rig_face_archetypes_lip_neutral */
RigFaceLipDetail (*rigpub_rig_face_archetypes_lip_neutral)(float gender_factor) = lip_neutral;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


static RigFaceArchetype g_archetypes[RIG_ARCH_COUNT];
static bool             g_archetypes_init = false;

static int init_archetypes(void)
{
    if (g_archetypes_init) return 0;
    memset(g_archetypes, 0, sizeof(g_archetypes));

    g_archetypes[RIG_ARCH_EUROPEAN_NORDIC] = (RigFaceArchetype){
        .id = RIG_ARCH_EUROPEAN_NORDIC, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "European Nordic",
        .description = "Fenotipo escandinavo: frente alta, pómulos elevados, mandíbula angular, piel clara con alta visibilidad vascular.",
        .codename = "nordic",
        .params = PARAMS_INIT(
            1.00f,1.14f,0.88f, 1.62f, 0.34f,0.33f,0.33f,
            0.32f,0.31f,0.22f,0.18f,
            0.48f,0.32f,0.24f,0.22f,
            0.50f,0.20f,0.24f,0.92f,122.0f,0.20f,0.18f,
            1.05f,0.44f, 0.42f,0.52f,
            0.08f,0.48f,0.06f, 0.25f,0.55f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.618f,
        .roughness_override = -1.0f,
        .tags = "nordic,european,fair,blond,male,female",
    };
    g_archetypes[RIG_ARCH_EUROPEAN_NORDIC].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_EUROPEAN_NORDIC].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_EUROPEAN_NORDIC].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_EUROPEAN_NORDIC].lip = lip_neutral(0.5f);

    g_archetypes[RIG_ARCH_EUROPEAN_MED] = (RigFaceArchetype){
        .id = RIG_ARCH_EUROPEAN_MED, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "European Mediterranean",
        .description = "Fenotipo mediterráneo: nariz prominente, mandíbula cuadrada, pómulos anchos, piel oliva.",
        .codename = "med_euro",
        .params = PARAMS_INIT(
            0.98f,1.12f,0.90f, 1.59f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.24f,0.20f,
            0.52f,0.36f,0.27f,0.25f,
            0.52f,0.22f,0.26f,0.96f,118.0f,0.22f,0.20f,
            1.08f,0.46f, 0.44f,0.50f,
            0.22f,0.46f,0.12f, 0.25f,0.52f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.590f,
        .roughness_override = -1.0f,
        .tags = "mediterranean,european,olive,dark_hair,roman,greek",
    };
    g_archetypes[RIG_ARCH_EUROPEAN_MED].iris = iris_brown_dark();
    g_archetypes[RIG_ARCH_EUROPEAN_MED].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_EUROPEAN_MED].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_EUROPEAN_MED].lip = lip_neutral(0.5f);

    g_archetypes[RIG_ARCH_EAST_ASIAN] = (RigFaceArchetype){
        .id = RIG_ARCH_EAST_ASIAN, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "East Asian",
        .description = "Fenotipo de Asia Oriental: pómulos amplios, frente plana, epicanto, nariz suave, cara ovalada.",
        .codename = "east_asian",
        .params = PARAMS_INIT(
            1.02f,1.08f,0.82f, 1.54f, 0.31f,0.35f,0.34f,
            0.30f,0.27f,0.18f,0.10f,
            0.44f,0.34f,0.20f,0.20f,
            0.50f,0.18f,0.22f,0.94f,115.0f,0.18f,0.16f,
            1.10f,0.42f, 0.40f,0.48f,
            0.20f,0.38f,0.22f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.540f,
        .roughness_override = -1.0f,
        .tags = "east_asian,chinese,japanese,korean,epicanthic",
    };
    g_archetypes[RIG_ARCH_EAST_ASIAN].iris = iris_midnight();
    g_archetypes[RIG_ARCH_EAST_ASIAN].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_EAST_ASIAN].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_EAST_ASIAN].lip = lip_neutral(0.5f);

    g_archetypes[RIG_ARCH_SOUTH_ASIAN] = (RigFaceArchetype){
        .id = RIG_ARCH_SOUTH_ASIAN, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "South Asian",
        .description = "Fenotipo subcontinental: nariz definida, ojos grandes y oscuros, frente alta, piel marrón media.",
        .codename = "south_asian",
        .params = PARAMS_INIT(
            0.97f,1.13f,0.87f, 1.58f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.22f,0.16f,
            0.50f,0.36f,0.25f,0.23f,
            0.51f,0.21f,0.25f,0.94f,117.0f,0.21f,0.18f,
            1.06f,0.45f, 0.42f,0.50f,
            0.40f,0.44f,0.18f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.580f,
        .roughness_override = -1.0f,
        .tags = "south_asian,indian,pakistani,bengali,dark,large_eyes",
    };
    g_archetypes[RIG_ARCH_SOUTH_ASIAN].iris = iris_brown_dark();
    g_archetypes[RIG_ARCH_SOUTH_ASIAN].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_SOUTH_ASIAN].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_SOUTH_ASIAN].lip = lip_neutral(0.5f);

    g_archetypes[RIG_ARCH_AFRICAN_WEST] = (RigFaceArchetype){
        .id = RIG_ARCH_AFRICAN_WEST, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "African West",
        .description = "Fenotipo de África Occidental: labios voluminosos, pómulos prominentes, frente amplia, piel oscura con alto SSS.",
        .codename = "african_west",
        .params = PARAMS_INIT(
            1.04f,1.10f,0.92f, 1.56f, 0.32f,0.34f,0.34f,
            0.32f,0.30f,0.20f,0.14f,
            0.46f,0.42f,0.22f,0.28f,
            0.56f,0.28f,0.32f,0.98f,110.0f,0.18f,0.16f,
            1.12f,0.48f, 0.46f,0.50f,
            0.82f,0.52f,0.08f, 0.25f,0.50f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.560f,
        .roughness_override = -1.0f,
        .tags = "african,west_african,dark,full_lips,broad_nose",
    };
    g_archetypes[RIG_ARCH_AFRICAN_WEST].iris = iris_midnight();
    g_archetypes[RIG_ARCH_AFRICAN_WEST].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_AFRICAN_WEST].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_AFRICAN_WEST].lip = lip_neutral(0.45f);

    g_archetypes[RIG_ARCH_AFRICAN_EAST] = (RigFaceArchetype){
        .id = RIG_ARCH_AFRICAN_EAST, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "African East",
        .description = "Fenotipo etíope/somalí: rasgos finos, nariz estrecha alta, cara alargada, pómulos afilados.",
        .codename = "african_east",
        .params = PARAMS_INIT(
            0.94f,1.18f,0.84f, 1.61f, 0.34f,0.33f,0.33f,
            0.30f,0.28f,0.24f,0.15f,
            0.52f,0.30f,0.28f,0.22f,
            0.48f,0.22f,0.26f,0.88f,124.0f,0.22f,0.20f,
            1.00f,0.46f, 0.38f,0.54f,
            0.70f,0.48f,0.06f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.610f,
        .roughness_override = -1.0f,
        .tags = "east_african,ethiopian,somali,narrow_nose,elongated",
    };
    g_archetypes[RIG_ARCH_AFRICAN_EAST].iris = iris_midnight();
    g_archetypes[RIG_ARCH_AFRICAN_EAST].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_AFRICAN_EAST].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_AFRICAN_EAST].lip = lip_neutral(0.5f);

    g_archetypes[RIG_ARCH_MIDDLE_EASTERN] = (RigFaceArchetype){
        .id = RIG_ARCH_MIDDLE_EASTERN, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "Middle Eastern",
        .description = "Fenotipo levantino/árabe: arco nasal marcado, cejas densas, ojos almendrados, mandíbula fuerte.",
        .codename = "middle_east",
        .params = PARAMS_INIT(
            0.99f,1.14f,0.88f, 1.58f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.26f,0.22f,
            0.55f,0.36f,0.30f,0.26f,
            0.52f,0.22f,0.26f,0.96f,116.0f,0.22f,0.19f,
            1.06f,0.46f, 0.43f,0.51f,
            0.35f,0.46f,0.15f, 0.25f,0.55f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.580f,
        .roughness_override = -1.0f,
        .tags = "arab,persian,levantine,strong_nose,almond_eyes,olive",
    };
    g_archetypes[RIG_ARCH_MIDDLE_EASTERN].iris = iris_brown_dark();
    g_archetypes[RIG_ARCH_MIDDLE_EASTERN].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_MIDDLE_EASTERN].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_MIDDLE_EASTERN].lip = lip_neutral(0.55f);

    g_archetypes[RIG_ARCH_LATIN_AMERICAN] = (RigFaceArchetype){
        .id = RIG_ARCH_LATIN_AMERICAN, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "Latin American",
        .description = "Fenotipo mestizo iberoamericano: rasgos mixtos amerindios y europeos, pómulos medios, nariz recta.",
        .codename = "latin_am",
        .params = PARAMS_INIT(
            1.00f,1.10f,0.86f, 1.57f, 0.33f,0.34f,0.33f,
            0.31f,0.29f,0.21f,0.16f,
            0.48f,0.34f,0.24f,0.22f,
            0.51f,0.21f,0.25f,0.95f,118.0f,0.20f,0.18f,
            1.06f,0.44f, 0.42f,0.50f,
            0.30f,0.44f,0.15f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.570f,
        .roughness_override = -1.0f,
        .tags = "latino,mestizo,hispanic,ibero,mixed,brown",
    };
    g_archetypes[RIG_ARCH_LATIN_AMERICAN].iris = iris_brown_dark();
    g_archetypes[RIG_ARCH_LATIN_AMERICAN].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_LATIN_AMERICAN].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_LATIN_AMERICAN].lip = lip_neutral(0.50f);

    g_archetypes[RIG_ARCH_HERO_WARRIOR] = (RigFaceArchetype){
        .id = RIG_ARCH_HERO_WARRIOR, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Hero Warrior",
        .description = "Arquetipo del héroe: mandíbula cuadrada potente, cejas marcadas, simetría perfecta, expresión de determinación.",
        .codename = "hero",
        .params = PARAMS_INIT(
            1.05f,1.15f,0.92f, 1.60f, 0.33f,0.34f,0.33f,
            0.30f,0.32f,0.26f,0.24f,
            0.48f,0.33f,0.26f,0.23f,
            0.50f,0.18f,0.22f,1.02f,110.0f,0.24f,0.22f,
            1.08f,0.46f, 0.48f,0.52f,
            0.25f,0.45f,0.10f, 0.28f,0.80f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.600f,
        .tags = "hero,warrior,masculine,strong,square_jaw,determined",
    };
    g_archetypes[RIG_ARCH_HERO_WARRIOR].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_HERO_WARRIOR].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_HERO_WARRIOR].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_HERO_WARRIOR].lip = lip_neutral(0.80f);

    g_archetypes[RIG_ARCH_SAGE_ELDER] = (RigFaceArchetype){
        .id = RIG_ARCH_SAGE_ELDER, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Sage Elder",
        .description = "El sabio anciano: frente prominente, pómulos descendidos, arrugas profundas, mirada profunda.",
        .codename = "sage",
        .params = PARAMS_INIT(
            0.96f,1.12f,0.88f, 1.55f, 0.36f,0.32f,0.32f,
            0.32f,0.29f,0.20f,0.15f,
            0.46f,0.33f,0.22f,0.21f,
            0.49f,0.16f,0.20f,0.90f,116.0f,0.18f,0.14f,
            1.02f,0.42f, 0.44f,0.48f,
            0.15f,0.40f,0.08f, 0.78f,0.75f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.550f,
        .tags = "elder,sage,wise,old,wrinkled,paternal",
    };
    g_archetypes[RIG_ARCH_SAGE_ELDER].iris = iris_amber_hazel();
    g_archetypes[RIG_ARCH_SAGE_ELDER].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_SAGE_ELDER].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_SAGE_ELDER].lip = lip_neutral(0.75f);
    g_archetypes[RIG_ARCH_SAGE_ELDER].ear_left.lobule_size = 0.7f;
    g_archetypes[RIG_ARCH_SAGE_ELDER].ear_right.lobule_size = 0.7f;

    g_archetypes[RIG_ARCH_NOBLE_ARISTOCRAT] = (RigFaceArchetype){
        .id = RIG_ARCH_NOBLE_ARISTOCRAT, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Noble Aristocrat",
        .description = "El aristócrata: nariz afilada, pómulos altos, frente estrecha elevada, expresión de altivez.",
        .codename = "noble",
        .params = PARAMS_INIT(
            0.93f,1.18f,0.84f, 1.63f, 0.35f,0.33f,0.32f,
            0.30f,0.29f,0.25f,0.18f,
            0.54f,0.28f,0.30f,0.20f,
            0.46f,0.18f,0.22f,0.88f,124.0f,0.24f,0.18f,
            1.00f,0.48f, 0.38f,0.52f,
            0.12f,0.44f,0.10f, 0.30f,0.60f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.630f,
        .tags = "noble,aristocrat,refined,high_cheekbones,sharp_nose",
    };
    g_archetypes[RIG_ARCH_NOBLE_ARISTOCRAT].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_NOBLE_ARISTOCRAT].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_NOBLE_ARISTOCRAT].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_NOBLE_ARISTOCRAT].lip = lip_neutral(0.60f);

    g_archetypes[RIG_ARCH_VILLAIN_SHARP] = (RigFaceArchetype){
        .id = RIG_ARCH_VILLAIN_SHARP, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Villain Sharp",
        .description = "El antagonista: mandíbula afilada, frente estrecha, simetría asimétrica deliberada, ojos profundos.",
        .codename = "villain",
        .params = PARAMS_INIT(
            0.90f,1.16f,0.80f, 1.52f, 0.36f,0.32f,0.32f,
            0.29f,0.28f,0.28f,0.26f,
            0.52f,0.27f,0.29f,0.19f,
            0.44f,0.16f,0.20f,0.84f,126.0f,0.26f,0.20f,
            0.96f,0.50f, 0.36f,0.50f,
            0.18f,0.38f,0.08f, 0.35f,0.82f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.520f,
        .tags = "villain,antagonist,sharp,angular,asymmetric,sinister",
    };
    g_archetypes[RIG_ARCH_VILLAIN_SHARP].iris = iris_amber_hazel();
    g_archetypes[RIG_ARCH_VILLAIN_SHARP].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_VILLAIN_SHARP].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_VILLAIN_SHARP].lip = lip_neutral(0.82f);

    g_archetypes[RIG_ARCH_ANDROGYNOUS] = (RigFaceArchetype){
        .id = RIG_ARCH_ANDROGYNOUS, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Androgynous",
        .description = "Neutro de género: proporciones equilibradas entre feminidad y masculinidad, rostro ovalado suave.",
        .codename = "androgyne",
        .params = PARAMS_INIT(
            0.97f,1.10f,0.85f, 1.61f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.21f,0.15f,
            0.48f,0.32f,0.24f,0.22f,
            0.50f,0.20f,0.24f,0.91f,118.0f,0.20f,0.18f,
            1.02f,0.44f, 0.40f,0.50f,
            0.18f,0.44f,0.12f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.610f,
        .tags = "androgynous,neutral,balanced,gender_neutral,ethereal",
    };
    g_archetypes[RIG_ARCH_ANDROGYNOUS].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_ANDROGYNOUS].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_ANDROGYNOUS].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_ANDROGYNOUS].lip = lip_neutral(0.50f);

    g_archetypes[RIG_ARCH_CHILD_ARCHETYPE] = (RigFaceArchetype){
        .id = RIG_ARCH_CHILD_ARCHETYPE, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Child Archetype",
        .description = "Rostro infantil universal: cráneo prominente, cara pequeña, mejillas redondeadas, ojos grandes.",
        .codename = "child",
        .params = PARAMS_INIT(
            1.08f,1.18f,0.95f, 1.44f, 0.28f,0.36f,0.36f,
            0.36f,0.34f,0.15f,0.08f,
            0.38f,0.32f,0.16f,0.18f,
            0.46f,0.20f,0.24f,0.80f,108.0f,0.12f,0.10f,
            0.96f,0.38f, 0.36f,0.42f,
            0.18f,0.52f,0.16f, 0.05f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.440f,
        .tags = "child,infant,young,cute,round,neoteny",
    };
    g_archetypes[RIG_ARCH_CHILD_ARCHETYPE].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_CHILD_ARCHETYPE].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_CHILD_ARCHETYPE].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_CHILD_ARCHETYPE].lip = lip_neutral(0.50f);

    g_archetypes[RIG_ARCH_SCHOLAR_REFINED] = (RigFaceArchetype){
        .id = RIG_ARCH_SCHOLAR_REFINED, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Scholar Refined",
        .description = "El intelectual: frente alta y ancha, nariz fina, ojos ligeramente hundidos, expresión contemplativa.",
        .codename = "scholar",
        .params = PARAMS_INIT(
            0.96f,1.16f,0.88f, 1.61f, 0.36f,0.33f,0.31f,
            0.31f,0.29f,0.24f,0.16f,
            0.50f,0.30f,0.26f,0.21f,
            0.47f,0.18f,0.22f,0.90f,120.0f,0.22f,0.18f,
            1.02f,0.44f, 0.40f,0.52f,
            0.14f,0.42f,0.10f, 0.32f,0.70f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.610f,
        .tags = "scholar,intellectual,refined,high_forehead,contemplative",
    };
    g_archetypes[RIG_ARCH_SCHOLAR_REFINED].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_SCHOLAR_REFINED].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_SCHOLAR_REFINED].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_SCHOLAR_REFINED].lip = lip_neutral(0.70f);

    g_archetypes[RIG_ARCH_WARRIOR_FEMALE] = (RigFaceArchetype){
        .id = RIG_ARCH_WARRIOR_FEMALE, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Warrior Female",
        .description = "Guerrera: pómulos angulares femeninos, mandíbula firme, ojos intensos, frente media.",
        .codename = "fem_warrior",
        .params = PARAMS_INIT(
            0.98f,1.12f,0.88f, 1.62f, 0.33f,0.34f,0.33f,
            0.31f,0.31f,0.22f,0.18f,
            0.47f,0.32f,0.24f,0.22f,
            0.51f,0.22f,0.26f,0.92f,118.0f,0.20f,0.18f,
            1.06f,0.48f, 0.40f,0.50f,
            0.22f,0.48f,0.12f, 0.28f,0.25f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.620f,
        .tags = "female,warrior,strong,angular,feminine,powerful",
    };
    g_archetypes[RIG_ARCH_WARRIOR_FEMALE].iris = iris_amber_hazel();
    g_archetypes[RIG_ARCH_WARRIOR_FEMALE].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_WARRIOR_FEMALE].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_WARRIOR_FEMALE].lip = lip_neutral(0.25f);

    g_archetypes[RIG_ARCH_AGE_INFANT] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_INFANT, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Infant",
        .description = "Neonato: proporciones neoténicas máximas, cráneo 60% de la cara, mejillas pronunciadas.",
        .codename = "age_0",
        .params = PARAMS_INIT(
            1.12f,1.24f,1.00f, 1.38f, 0.26f,0.38f,0.36f,
            0.38f,0.36f,0.12f,0.06f,
            0.32f,0.34f,0.12f,0.16f,
            0.42f,0.22f,0.26f,0.74f,105.0f,0.08f,0.06f,
            0.90f,0.34f, 0.30f,0.36f,
            0.18f,0.58f,0.18f, 0.00f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.380f,
        .tags = "infant,baby,neonate,age_0,neoteny_max",
    };

    g_archetypes[RIG_ARCH_AGE_CHILD_7] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_CHILD_7, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Child 7yr",
        .description = "Niño de 7 años: cara en desarrollo, dientes de leche reemplazados, frente amplia, nariz pequeña.",
        .codename = "age_7",
        .params = PARAMS_INIT(
            1.06f,1.16f,0.92f, 1.48f, 0.30f,0.36f,0.34f,
            0.34f,0.32f,0.16f,0.10f,
            0.40f,0.32f,0.18f,0.20f,
            0.44f,0.20f,0.24f,0.82f,110.0f,0.14f,0.12f,
            0.94f,0.38f, 0.34f,0.40f,
            0.16f,0.50f,0.14f, 0.06f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.480f,
        .tags = "child_7,age_7,school_age,developing",
    };

    g_archetypes[RIG_ARCH_AGE_TEEN_16] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_TEEN_16, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Teen 16yr",
        .description = "Adolescente 16 años: rasgos en transición adulta, proporciones casi completas, piel con volumen.",
        .codename = "age_16",
        .params = PARAMS_INIT(
            0.99f,1.12f,0.87f, 1.57f, 0.32f,0.34f,0.34f,
            0.31f,0.30f,0.20f,0.15f,
            0.46f,0.33f,0.22f,0.22f,
            0.50f,0.22f,0.26f,0.90f,116.0f,0.18f,0.16f,
            1.02f,0.42f, 0.38f,0.46f,
            0.18f,0.48f,0.14f, 0.12f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.570f,
        .tags = "teen,16,adolescent,transitional",
    };

    g_archetypes[RIG_ARCH_AGE_YOUNG_25] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_YOUNG_25, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Young Adult 25yr",
        .description = "Adulto joven 25 años: colágeno máximo, proporciones completas, piel sin marcas de tiempo.",
        .codename = "age_25",
        .params = PARAMS_INIT(
            0.98f,1.12f,0.87f, 1.62f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.22f,0.17f,
            0.48f,0.33f,0.24f,0.22f,
            0.50f,0.20f,0.24f,0.92f,118.0f,0.20f,0.18f,
            1.04f,0.44f, 0.40f,0.50f,
            0.20f,0.44f,0.10f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.620f,
        .tags = "young_adult,25,prime,collagen_peak",
    };

    g_archetypes[RIG_ARCH_AGE_MATURE_45] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_MATURE_45, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Mature 45yr",
        .description = "Adulto maduro 45 años: surcos nasolabiales, párpado superior con leve ptosis, pérdida de colágeno.",
        .codename = "age_45",
        .params = PARAMS_INIT(
            0.97f,1.10f,0.86f, 1.58f, 0.34f,0.33f,0.33f,
            0.31f,0.29f,0.21f,0.16f,
            0.47f,0.33f,0.23f,0.22f,
            0.49f,0.18f,0.22f,0.91f,118.0f,0.19f,0.17f,
            1.03f,0.43f, 0.42f,0.50f,
            0.20f,0.42f,0.10f, 0.48f,0.55f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.580f,
        .tags = "mature,45,middle_aged,experienced,character",
    };

    g_archetypes[RIG_ARCH_AGE_ELDER_70] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_ELDER_70, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Elder 70yr",
        .description = "Anciano 70 años: tejido descendido, arrugas profundas, pérdida ósea alveolar, cuello flácido.",
        .codename = "age_70",
        .params = PARAMS_INIT(
            0.94f,1.08f,0.84f, 1.50f, 0.36f,0.32f,0.32f,
            0.32f,0.27f,0.18f,0.12f,
            0.44f,0.32f,0.20f,0.20f,
            0.47f,0.14f,0.18f,0.88f,114.0f,0.16f,0.12f,
            1.00f,0.40f, 0.44f,0.46f,
            0.16f,0.38f,0.08f, 0.80f,0.60f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.500f,
        .tags = "elder,70,old,aged,wisdom,wrinkled,jowl",
    };
    g_archetypes[RIG_ARCH_AGE_ELDER_70].ear_left.lobule_size = 0.8f;
    g_archetypes[RIG_ARCH_AGE_ELDER_70].ear_right.lobule_size = 0.8f;

    g_archetypes[RIG_ARCH_GREEK_IDEAL] = (RigFaceArchetype){
        .id = RIG_ARCH_GREEK_IDEAL, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Greek Ideal",
        .description = "Canon griego clásico (Policleto, Praxíteles): perfil en línea recta frente-nariz, cara ovalada perfecta.",
        .codename = "greek",
        .params = PARAMS_INIT(
            0.97f,1.14f,0.87f, 1.618f, 0.333f,0.333f,0.334f,
            0.30f,0.30f,0.24f,0.20f,
            0.50f,0.30f,0.28f,0.20f,
            0.48f,0.19f,0.23f,0.90f,120.0f,0.22f,0.18f,
            1.02f,0.46f, 0.40f,0.50f,
            0.12f,0.44f,0.10f, 0.26f,0.55f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "greek,classical,ideal,polykleitos,praxiteles,marble",
    };
    g_archetypes[RIG_ARCH_GREEK_IDEAL].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_GREEK_IDEAL].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_GREEK_IDEAL].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_GREEK_IDEAL].lip = lip_neutral(0.55f);

    g_archetypes[RIG_ARCH_VITRUVIAN_CANON] = (RigFaceArchetype){
        .id = RIG_ARCH_VITRUVIAN_CANON, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Vitruvian Canon",
        .description = "Canon vitruviano revisado por Da Vinci: tercios iguales perfectos, ancho=altura×0.618.",
        .codename = "vitruvian",
        .params = PARAMS_INIT(
            0.96f,1.12f,0.86f, 1.618f, 0.333f,0.333f,0.334f,
            0.309f,0.309f,0.22f,0.18f,
            0.48f,0.309f,0.26f,0.21f,
            0.479f,0.19f,0.23f,0.91f,119.0f,0.21f,0.17f,
            1.03f,0.44f, 0.41f,0.51f,
            0.18f,0.44f,0.11f, 0.26f,0.55f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "vitruvian,da_vinci,thirds,classical,ideal_proportions",
    };

    g_archetypes[RIG_ARCH_PHI_PERFECT] = (RigFaceArchetype){
        .id = RIG_ARCH_PHI_PERFECT, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Phi Perfect",
        .description = "Máscara de Marquardt φ-perfecta: todas las proporciones son razones de φ. Error φ < 0.001.",
        .codename = "phi_perfect",
        .params = PARAMS_INIT(
            0.9511f,1.1756f,0.8090f, 1.6180f, 0.3333f,0.3333f,0.3334f,
            0.3090f,0.3090f,0.2360f,0.1910f,
            0.5000f,0.3090f,0.2580f,0.2000f,
            0.4760f,0.1910f,0.2360f,0.9020f,118.8f,0.2360f,0.1910f,
            1.0000f,0.4760f, 0.3820f,0.5000f,
            0.15f,0.44f,0.10f, 0.25f,0.52f),
        .recommended_subdiv = 6, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "phi,golden_ratio,marquardt,perfect,mathematical",
    };

    g_archetypes[RIG_ARCH_HYPERREALIST] = (RigFaceArchetype){
        .id = RIG_ARCH_HYPERREALIST, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Hyperrealist",
        .description = "Hiperrealismo: asimetrías naturales calibradas, micro-defectos auténticos, máximo detalle dérmico.",
        .codename = "hyperreal",
        .params = PARAMS_INIT(
            0.983f,1.107f,0.877f, 1.587f, 0.332f,0.341f,0.327f,
            0.308f,0.297f,0.218f,0.167f,
            0.478f,0.332f,0.241f,0.219f,
            0.501f,0.203f,0.243f,0.924f,117.3f,0.198f,0.178f,
            1.038f,0.437f, 0.401f,0.502f,
            0.24f,0.45f,0.11f, 0.27f,0.53f),
        .recommended_subdiv = 6, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.587f,
        .tags = "hyperrealist,cinematic,film_vfx,asymmetric,natural,pores",
    };

    g_archetypes[RIG_ARCH_STYLIZED_ANIME] = (RigFaceArchetype){
        .id = RIG_ARCH_STYLIZED_ANIME, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Stylized Anime",
        .description = "Estilo anime: ojos grandes (40% cara), nariz mínima, boca pequeña, mentón afilado, frente alta.",
        .codename = "anime",
        .params = PARAMS_INIT(
            0.95f,1.20f,0.75f, 1.44f, 0.28f,0.38f,0.34f,
            0.40f,0.40f,0.10f,0.06f,
            0.35f,0.22f,0.15f,0.14f,
            0.38f,0.12f,0.16f,0.78f,108.0f,0.14f,0.16f,
            0.90f,0.38f, 0.32f,0.42f,
            0.15f,0.40f,0.12f, 0.20f,0.40f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.440f,
        .tags = "anime,stylized,manga,large_eyes,pointed_chin,2d_inspired",
    };

    g_archetypes[RIG_ARCH_RENAISSANCE_BEAUTY] = (RigFaceArchetype){
        .id = RIG_ARCH_RENAISSANCE_BEAUTY, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Renaissance Beauty",
        .description = "Belleza renacentista (Botticelli, Leonardo): frente despejada, nariz recta, labios carnosos, óvalo perfecto.",
        .codename = "renaissance",
        .params = PARAMS_INIT(
            0.95f,1.15f,0.85f, 1.61f, 0.34f,0.33f,0.33f,
            0.30f,0.30f,0.22f,0.16f,
            0.48f,0.31f,0.26f,0.21f,
            0.50f,0.24f,0.28f,0.90f,120.0f,0.20f,0.16f,
            1.02f,0.46f, 0.38f,0.50f,
            0.14f,0.50f,0.16f, 0.22f,0.20f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.610f,
        .tags = "renaissance,botticelli,female,beauty,oval,classical_art",
    };

    g_archetypes[RIG_ARCH_ALBINISM] = (RigFaceArchetype){
        .id = RIG_ARCH_ALBINISM, .category = RIG_ARCHETYPE_CAT_SPECIAL,
        .name = "Albinism",
        .description = "Albinismo OCA1: melanina=0, alta translucidez, iris azul pálido con visibilidad del epitelio pigmentario.",
        .codename = "albino",
        .params = PARAMS_INIT(
            0.98f,1.12f,0.86f, 1.60f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.22f,0.17f,
            0.48f,0.32f,0.24f,0.22f,
            0.50f,0.20f,0.24f,0.92f,118.0f,0.20f,0.18f,
            1.03f,0.44f, 0.40f,0.50f,
            0.00f,0.78f,0.02f, 0.25f,0.50f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.600f,
        .tags = "albinism,oca1,melanin_zero,translucent,rare,clinical",
    };
    {
        RigFaceIrisDetail al_iris = {0};
        al_iris.iris_color[0] = 0.75f; al_iris.iris_color[1] = 0.82f; al_iris.iris_color[2] = 0.90f;
        al_iris.limbal_ring_width = 0.3f; al_iris.limbal_ring_darkness = 0.2f;
        al_iris.pattern = RIG_IRIS_PATTERN_UNIFORM; al_iris.pupil_shape = RIG_PUPIL_ROUND;
        al_iris.iris_radius = 5.8f; al_iris.pupil_radius = 4.5f;
        al_iris.crypt_density = 0.1f; al_iris.wolfflin_count = 0.0f; al_iris.furrow_count = 2.0f;
        g_archetypes[RIG_ARCH_ALBINISM].iris = al_iris;
    }
    g_archetypes[RIG_ARCH_ALBINISM].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_ALBINISM].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_ALBINISM].lip = lip_neutral(0.50f);

    g_archetypes[RIG_ARCH_VITILIGO] = (RigFaceArchetype){
        .id = RIG_ARCH_VITILIGO, .category = RIG_ARCHETYPE_CAT_SPECIAL,
        .name = "Vitiligo",
        .description = "Vitiligo segmentario: pérdida localizada de melanocitos con patrón fractal en parches.",
        .codename = "vitiligo",
        .params = PARAMS_INIT(
            0.98f,1.12f,0.86f, 1.60f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.22f,0.17f,
            0.48f,0.32f,0.24f,0.22f,
            0.50f,0.20f,0.24f,0.92f,118.0f,0.20f,0.18f,
            1.03f,0.44f, 0.40f,0.50f,
            0.40f,0.48f,0.08f, 0.30f,0.50f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.600f,
        .tags = "vitiligo,depigmentation,fractal_pattern,clinical,diverse",
    };
    g_archetypes[RIG_ARCH_VITILIGO].iris = iris_brown_dark();

    g_archetypes[RIG_ARCH_BATTLE_SCARRED] = (RigFaceArchetype){
        .id = RIG_ARCH_BATTLE_SCARRED, .category = RIG_ARCHETYPE_CAT_SPECIAL,
        .name = "Battle Scarred",
        .description = "Guerrero veterano: cicatrices lineales en múltiples regiones, deformación nasal, ptosis cicatricial.",
        .codename = "scarred",
        .params = PARAMS_INIT(
            1.02f,1.12f,0.90f, 1.56f, 0.33f,0.33f,0.34f,
            0.30f,0.29f,0.24f,0.22f,
            0.47f,0.35f,0.25f,0.24f,
            0.50f,0.18f,0.22f,0.98f,112.0f,0.22f,0.20f,
            1.06f,0.44f, 0.46f,0.52f,
            0.28f,0.44f,0.10f, 0.45f,0.82f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.560f,
        .tags = "scarred,veteran,battle,warrior,character,damaged,cinematic",
    };
    g_archetypes[RIG_ARCH_BATTLE_SCARRED].iris = iris_amber_hazel();

    g_archetypes[RIG_ARCH_NEONATAL] = (RigFaceArchetype){
        .id = RIG_ARCH_NEONATAL, .category = RIG_ARCHETYPE_CAT_SPECIAL,
        .name = "Neonatal",
        .description = "Recién nacido: craneometría neonatal, fontanelas visibles, cara 1/4 del cráneo, edema subcutáneo.",
        .codename = "neonate",
        .params = PARAMS_INIT(
            1.18f,1.28f,1.04f, 1.30f, 0.22f,0.40f,0.38f,
            0.40f,0.38f,0.10f,0.04f,
            0.28f,0.36f,0.08f,0.14f,
            0.38f,0.24f,0.28f,0.68f,102.0f,0.06f,0.04f,
            0.84f,0.32f, 0.28f,0.32f,
            0.16f,0.60f,0.20f, 0.00f,0.50f),
        .recommended_subdiv = 4, .build_ears = false, .build_neck = true,
        .build_skeleton = false, .build_shapes = false,
        .phi_reference = 1.300f,
        .tags = "neonate,newborn,neonatal,infant,medical,fontanelle",
    };

    g_archetypes[RIG_ARCH_APOLLONIAN] = (RigFaceArchetype){
        .id = RIG_ARCH_APOLLONIAN, .category = RIG_ARCHETYPE_CAT_LEGENDARY,
        .name = "Apollonian",
        .description = "El Apolíneo: luz solar, razón, orden. Proporciones griegas puras, expresión de serenidad perpetua.",
        .codename = "apollo",
        .params = PARAMS_INIT(
            0.951f,1.176f,0.809f, 1.6180f, 0.333f,0.333f,0.334f,
            0.309f,0.309f,0.236f,0.191f,
            0.500f,0.309f,0.258f,0.200f,
            0.476f,0.191f,0.236f,0.902f,120.0f,0.236f,0.191f,
            1.000f,0.476f, 0.382f,0.500f,
            0.10f,0.44f,0.10f, 0.25f,0.60f),
        .recommended_subdiv = 6, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "apollo,solar,greek,legendary,godlike,serene,phi",
    };
    g_archetypes[RIG_ARCH_APOLLONIAN].iris = iris_blue_green();

    g_archetypes[RIG_ARCH_MAYAN_CLASSIC] = (RigFaceArchetype){
        .id = RIG_ARCH_MAYAN_CLASSIC, .category = RIG_ARCHETYPE_CAT_LEGENDARY,
        .name = "Mayan Classic",
        .description = "Canon maya clásico (600 d.C.): frente inclinada, nariz aquilina, pómulos amplios, rasgos ceremoniales.",
        .codename = "mayan",
        .params = PARAMS_INIT(
            1.04f,1.16f,0.92f, 1.56f, 0.30f,0.36f,0.34f,
            0.32f,0.28f,0.20f,0.14f,
            0.48f,0.38f,0.22f,0.24f,
            0.52f,0.24f,0.28f,1.00f,114.0f,0.20f,0.18f,
            1.12f,0.46f, 0.44f,0.48f,
            0.35f,0.46f,0.20f, 0.28f,0.60f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.560f,
        .tags = "mayan,mesoamerican,classic,ceremonial,aquiline,pre_columbian",
    };
    g_archetypes[RIG_ARCH_MAYAN_CLASSIC].iris = iris_midnight();

    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN] = (RigFaceArchetype){
        .id = RIG_ARCH_RIGADIEL_SOVEREIGN, .category = RIG_ARCHETYPE_CAT_LEGENDARY,
        .name = "RIGADIEL Sovereign",
        .description = "φ² · φ · φ⁻¹ — El arquetipo soberano de RigCom. "
                        "Núcleos solares: Edgar José Gabriel Mora (φ²). "
                        "Guardia lunar: La Negra María (φ⁻¹). "
                        "Centro: Richard Felipe Urbina (φ). Tres resonancias.",
        .codename = "rigadiel",
        .params = PARAMS_INIT(

            0.9511f,1.1756f,0.8090f, 1.6180f, 0.3333f,0.3333f,0.3334f,
            0.3090f,0.3090f,0.2361f,0.1910f,
            0.5000f,0.3090f,0.2580f,0.2000f,
            0.4760f,0.1910f,0.2360f,0.9020f,118.8f,0.2360f,0.1910f,
            1.0000f,0.4760f, 0.3820f,0.5000f,
            0.25f,0.46f,0.12f, 0.25f,0.55f),
        .recommended_subdiv = 6, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "rigadiel,sovereign,phi,rigcom,gabriel,negra_maria,richard",
    };
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris = iris_amber_hazel();
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.heterochromia = true;
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.secondary_color[0] = 0.35f;
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.secondary_color[1] = 0.58f;
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.secondary_color[2] = 0.72f;
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.secondary_sector_start = 0.0f;
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.secondary_sector_end   = 1.2f;

    g_archetypes[RIG_ARCH_OMEGA_MIND] = (RigFaceArchetype){
        .id = RIG_ARCH_OMEGA_MIND, .category = RIG_ARCHETYPE_CAT_LEGENDARY,
        .name = "Omega Mind",
        .description = "RIGADIEL Omega Mind: 20 capas lobulares · 41616 neuronas · Hodgkin-Huxley. "
                        "Rostro emergente de la red neuronal determinista. "
                        "Frecuencia Schumann 7.83 Hz integrada en la geometría.",
        .codename = "omega",
        .params = PARAMS_INIT(

            0.9511f,1.2490f,0.8090f, 1.6180f, 0.3333f,0.3333f,0.3334f,
            0.3090f,0.3090f,0.2490f,0.2000f,
            0.5000f,0.3090f,0.2500f,0.2000f,
            0.4760f,0.2000f,0.2490f,0.9020f,120.0f,0.2490f,0.2000f,
            1.0000f,0.4760f, 0.3820f,0.5000f,
            0.20f,0.48f,0.10f, 0.30f,0.50f),
        .recommended_subdiv = 6, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "omega,rigadiel,neural,deterministic,schumann,hodgkin_huxley",
    };
    g_archetypes[RIG_ARCH_OMEGA_MIND].iris = iris_midnight();
    g_archetypes[RIG_ARCH_OMEGA_MIND].iris.limbal_ring_width = 2.0f;
    g_archetypes[RIG_ARCH_OMEGA_MIND].iris.limbal_ring_darkness = 1.0f;

    for (int i = 0; i < RIG_ARCH_COUNT; i++) {
        RigFaceArchetype *a = &g_archetypes[i];
        if (a->ear_left.ear_height  == 0.0f) a->ear_left  = ear_default(false);
        if (a->ear_right.ear_height == 0.0f) a->ear_right = ear_default(true);
        if (a->lip.upper_lip_height == 0.0f) a->lip = lip_neutral(a->params.gender_factor);
        if (a->iris.iris_radius     == 0.0f) a->iris = iris_brown_dark();
        if (a->roughness_override    == 0.0f) a->roughness_override = -1.0f;
    }

    g_archetypes_init = true;
    return 0;
}
/* RIGCOM_PUBLIC_STATIC_EXPORT_BEGIN: init_archetypes -> rigpub_rig_face_archetypes_init_archetypes */
int (*rigpub_rig_face_archetypes_init_archetypes)(void) = init_archetypes;
/* RIGCOM_PUBLIC_STATIC_EXPORT_END */


const RigFaceArchetype* rig_archetype_get(RigArchetypeID id)
{
    init_archetypes();
    if ((unsigned)id >= RIG_ARCH_COUNT) return NULL;
    return &g_archetypes[id];
}

const char* rig_archetype_category_name(RigArchetypeCategory cat)
{
    static const char *names[RIG_ARCHETYPE_CAT_COUNT] = {
        "ETHNOGRAPHIC", "CLASSICAL", "AGE_STUDY",
        "ARTISTIC", "SPECIAL", "LEGENDARY"
    };
    if ((unsigned)cat >= RIG_ARCHETYPE_CAT_COUNT) return "UNKNOWN";
    return names[cat];
}

RigFaceParams rig_archetype_blend_params(RigArchetypeID id_a,
                                          RigArchetypeID id_b,
                                          float t)
{
    init_archetypes();
    const RigFaceArchetype *a = rig_archetype_get(id_a);
    const RigFaceArchetype *b = rig_archetype_get(id_b);
    if (!a) return rig_face_default_params();
    if (!b) return a->params;

    RigFaceParams r;
    const float *pa = (const float*)&a->params;
    const float *pb = (const float*)&b->params;
    float       *pr = (float*)&r;
    size_t n = sizeof(RigFaceParams) / sizeof(float);
    for (size_t i = 0; i < n; i++) {
        pr[i] = phi_lerp(pa[i], pb[i], t);
    }
    return r;
}

int rig_archetype_list_all(void)
{
    init_archetypes();
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  RigCom v24 THE SANTORIUM OF COMPILER — Face Archetypes  —  36 Templates               ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");

    RigArchetypeCategory last_cat = (RigArchetypeCategory)-1;
    for (int i = 0; i < RIG_ARCH_COUNT; i++) {
        const RigFaceArchetype *a = &g_archetypes[i];
        if (a->category != last_cat) {
            printf("║  ── %s ──\n", rig_archetype_category_name(a->category));
            last_cat = a->category;
        }
        printf("║  [%02d] %-22s  %-16s  φ=%.4f  subdiv=%u\n",
               a->id, a->name, a->codename,
               a->phi_reference, a->recommended_subdiv);
    }
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    return 0; /* 0=hook procesado, -1=contexto inválido */
}

int rig_archetype_search(const char *tag, RigArchetypeID *out, int max_results)
{
    init_archetypes();
    int found = 0;
    for (int i = 0; i < RIG_ARCH_COUNT && found < max_results; i++) {
        if (strstr(g_archetypes[i].tags,     tag) ||
            strstr(g_archetypes[i].name,     tag) ||
            strstr(g_archetypes[i].codename, tag)) {
            out[found++] = (RigArchetypeID)i;
        }
    }
    return found;
}

RigFaceMesh* rig_face_from_archetype_v1(RigArchetypeID id)
{
    const RigFaceArchetype *a = rig_archetype_get(id);
    if (!a) return NULL;
    return rig_face_create(&a->params, a->recommended_subdiv);
}


/* ========================================================================
 * SECCION DE FUSION — unidades reales de las otras 6 copias de este
 * modulo, ausentes en la copia canonica. Se incorporan integras.
 * ======================================================================== */

/* [fusion] global void__v2 <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_archetypes.c:929 :: dato global divergente; ausente en la copia canonica */
void (*rigpub_rig_face_archetypes_init_archetypes)(void__v2) = init_archetypes;

/* [fusion] function init_archetypes__v2 <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_archetypes.c:151 :: cuerpo divergente; ausente en la copia canonica */
static void init_archetypes__v2(void)
{
    if (g_archetypes_init) return;
    memset(g_archetypes, 0, sizeof(g_archetypes));

    g_archetypes[RIG_ARCH_EUROPEAN_NORDIC] = (RigFaceArchetype){
        .id = RIG_ARCH_EUROPEAN_NORDIC, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "European Nordic",
        .description = "Fenotipo escandinavo: frente alta, pómulos elevados, mandíbula angular, piel clara con alta visibilidad vascular.",
        .codename = "nordic",
        .params = PARAMS_INIT(
            1.00f,1.14f,0.88f, 1.62f, 0.34f,0.33f,0.33f,
            0.32f,0.31f,0.22f,0.18f,
            0.48f,0.32f,0.24f,0.22f,
            0.50f,0.20f,0.24f,0.92f,122.0f,0.20f,0.18f,
            1.05f,0.44f, 0.42f,0.52f,
            0.08f,0.48f,0.06f, 0.25f,0.55f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.618f,
        .roughness_override = -1.0f,
        .tags = "nordic,european,fair,blond,male,female",
    };
    g_archetypes[RIG_ARCH_EUROPEAN_NORDIC].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_EUROPEAN_NORDIC].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_EUROPEAN_NORDIC].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_EUROPEAN_NORDIC].lip = lip_neutral(0.5f);

    g_archetypes[RIG_ARCH_EUROPEAN_MED] = (RigFaceArchetype){
        .id = RIG_ARCH_EUROPEAN_MED, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "European Mediterranean",
        .description = "Fenotipo mediterráneo: nariz prominente, mandíbula cuadrada, pómulos anchos, piel oliva.",
        .codename = "med_euro",
        .params = PARAMS_INIT(
            0.98f,1.12f,0.90f, 1.59f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.24f,0.20f,
            0.52f,0.36f,0.27f,0.25f,
            0.52f,0.22f,0.26f,0.96f,118.0f,0.22f,0.20f,
            1.08f,0.46f, 0.44f,0.50f,
            0.22f,0.46f,0.12f, 0.25f,0.52f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.590f,
        .roughness_override = -1.0f,
        .tags = "mediterranean,european,olive,dark_hair,roman,greek",
    };
    g_archetypes[RIG_ARCH_EUROPEAN_MED].iris = iris_brown_dark();
    g_archetypes[RIG_ARCH_EUROPEAN_MED].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_EUROPEAN_MED].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_EUROPEAN_MED].lip = lip_neutral(0.5f);

    g_archetypes[RIG_ARCH_EAST_ASIAN] = (RigFaceArchetype){
        .id = RIG_ARCH_EAST_ASIAN, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "East Asian",
        .description = "Fenotipo de Asia Oriental: pómulos amplios, frente plana, epicanto, nariz suave, cara ovalada.",
        .codename = "east_asian",
        .params = PARAMS_INIT(
            1.02f,1.08f,0.82f, 1.54f, 0.31f,0.35f,0.34f,
            0.30f,0.27f,0.18f,0.10f,
            0.44f,0.34f,0.20f,0.20f,
            0.50f,0.18f,0.22f,0.94f,115.0f,0.18f,0.16f,
            1.10f,0.42f, 0.40f,0.48f,
            0.20f,0.38f,0.22f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.540f,
        .roughness_override = -1.0f,
        .tags = "east_asian,chinese,japanese,korean,epicanthic",
    };
    g_archetypes[RIG_ARCH_EAST_ASIAN].iris = iris_midnight();
    g_archetypes[RIG_ARCH_EAST_ASIAN].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_EAST_ASIAN].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_EAST_ASIAN].lip = lip_neutral(0.5f);

    g_archetypes[RIG_ARCH_SOUTH_ASIAN] = (RigFaceArchetype){
        .id = RIG_ARCH_SOUTH_ASIAN, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "South Asian",
        .description = "Fenotipo subcontinental: nariz definida, ojos grandes y oscuros, frente alta, piel marrón media.",
        .codename = "south_asian",
        .params = PARAMS_INIT(
            0.97f,1.13f,0.87f, 1.58f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.22f,0.16f,
            0.50f,0.36f,0.25f,0.23f,
            0.51f,0.21f,0.25f,0.94f,117.0f,0.21f,0.18f,
            1.06f,0.45f, 0.42f,0.50f,
            0.40f,0.44f,0.18f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.580f,
        .roughness_override = -1.0f,
        .tags = "south_asian,indian,pakistani,bengali,dark,large_eyes",
    };
    g_archetypes[RIG_ARCH_SOUTH_ASIAN].iris = iris_brown_dark();
    g_archetypes[RIG_ARCH_SOUTH_ASIAN].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_SOUTH_ASIAN].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_SOUTH_ASIAN].lip = lip_neutral(0.5f);

    g_archetypes[RIG_ARCH_AFRICAN_WEST] = (RigFaceArchetype){
        .id = RIG_ARCH_AFRICAN_WEST, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "African West",
        .description = "Fenotipo de África Occidental: labios voluminosos, pómulos prominentes, frente amplia, piel oscura con alto SSS.",
        .codename = "african_west",
        .params = PARAMS_INIT(
            1.04f,1.10f,0.92f, 1.56f, 0.32f,0.34f,0.34f,
            0.32f,0.30f,0.20f,0.14f,
            0.46f,0.42f,0.22f,0.28f,
            0.56f,0.28f,0.32f,0.98f,110.0f,0.18f,0.16f,
            1.12f,0.48f, 0.46f,0.50f,
            0.82f,0.52f,0.08f, 0.25f,0.50f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.560f,
        .roughness_override = -1.0f,
        .tags = "african,west_african,dark,full_lips,broad_nose",
    };
    g_archetypes[RIG_ARCH_AFRICAN_WEST].iris = iris_midnight();
    g_archetypes[RIG_ARCH_AFRICAN_WEST].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_AFRICAN_WEST].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_AFRICAN_WEST].lip = lip_neutral(0.45f);

    g_archetypes[RIG_ARCH_AFRICAN_EAST] = (RigFaceArchetype){
        .id = RIG_ARCH_AFRICAN_EAST, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "African East",
        .description = "Fenotipo etíope/somalí: rasgos finos, nariz estrecha alta, cara alargada, pómulos afilados.",
        .codename = "african_east",
        .params = PARAMS_INIT(
            0.94f,1.18f,0.84f, 1.61f, 0.34f,0.33f,0.33f,
            0.30f,0.28f,0.24f,0.15f,
            0.52f,0.30f,0.28f,0.22f,
            0.48f,0.22f,0.26f,0.88f,124.0f,0.22f,0.20f,
            1.00f,0.46f, 0.38f,0.54f,
            0.70f,0.48f,0.06f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.610f,
        .roughness_override = -1.0f,
        .tags = "east_african,ethiopian,somali,narrow_nose,elongated",
    };
    g_archetypes[RIG_ARCH_AFRICAN_EAST].iris = iris_midnight();
    g_archetypes[RIG_ARCH_AFRICAN_EAST].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_AFRICAN_EAST].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_AFRICAN_EAST].lip = lip_neutral(0.5f);

    g_archetypes[RIG_ARCH_MIDDLE_EASTERN] = (RigFaceArchetype){
        .id = RIG_ARCH_MIDDLE_EASTERN, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "Middle Eastern",
        .description = "Fenotipo levantino/árabe: arco nasal marcado, cejas densas, ojos almendrados, mandíbula fuerte.",
        .codename = "middle_east",
        .params = PARAMS_INIT(
            0.99f,1.14f,0.88f, 1.58f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.26f,0.22f,
            0.55f,0.36f,0.30f,0.26f,
            0.52f,0.22f,0.26f,0.96f,116.0f,0.22f,0.19f,
            1.06f,0.46f, 0.43f,0.51f,
            0.35f,0.46f,0.15f, 0.25f,0.55f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.580f,
        .roughness_override = -1.0f,
        .tags = "arab,persian,levantine,strong_nose,almond_eyes,olive",
    };
    g_archetypes[RIG_ARCH_MIDDLE_EASTERN].iris = iris_brown_dark();
    g_archetypes[RIG_ARCH_MIDDLE_EASTERN].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_MIDDLE_EASTERN].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_MIDDLE_EASTERN].lip = lip_neutral(0.55f);

    g_archetypes[RIG_ARCH_LATIN_AMERICAN] = (RigFaceArchetype){
        .id = RIG_ARCH_LATIN_AMERICAN, .category = RIG_ARCHETYPE_CAT_ETHNOGRAPHIC,
        .name = "Latin American",
        .description = "Fenotipo mestizo iberoamericano: rasgos mixtos amerindios y europeos, pómulos medios, nariz recta.",
        .codename = "latin_am",
        .params = PARAMS_INIT(
            1.00f,1.10f,0.86f, 1.57f, 0.33f,0.34f,0.33f,
            0.31f,0.29f,0.21f,0.16f,
            0.48f,0.34f,0.24f,0.22f,
            0.51f,0.21f,0.25f,0.95f,118.0f,0.20f,0.18f,
            1.06f,0.44f, 0.42f,0.50f,
            0.30f,0.44f,0.15f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.570f,
        .roughness_override = -1.0f,
        .tags = "latino,mestizo,hispanic,ibero,mixed,brown",
    };
    g_archetypes[RIG_ARCH_LATIN_AMERICAN].iris = iris_brown_dark();
    g_archetypes[RIG_ARCH_LATIN_AMERICAN].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_LATIN_AMERICAN].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_LATIN_AMERICAN].lip = lip_neutral(0.50f);

    g_archetypes[RIG_ARCH_HERO_WARRIOR] = (RigFaceArchetype){
        .id = RIG_ARCH_HERO_WARRIOR, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Hero Warrior",
        .description = "Arquetipo del héroe: mandíbula cuadrada potente, cejas marcadas, simetría perfecta, expresión de determinación.",
        .codename = "hero",
        .params = PARAMS_INIT(
            1.05f,1.15f,0.92f, 1.60f, 0.33f,0.34f,0.33f,
            0.30f,0.32f,0.26f,0.24f,
            0.48f,0.33f,0.26f,0.23f,
            0.50f,0.18f,0.22f,1.02f,110.0f,0.24f,0.22f,
            1.08f,0.46f, 0.48f,0.52f,
            0.25f,0.45f,0.10f, 0.28f,0.80f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.600f,
        .tags = "hero,warrior,masculine,strong,square_jaw,determined",
    };
    g_archetypes[RIG_ARCH_HERO_WARRIOR].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_HERO_WARRIOR].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_HERO_WARRIOR].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_HERO_WARRIOR].lip = lip_neutral(0.80f);

    g_archetypes[RIG_ARCH_SAGE_ELDER] = (RigFaceArchetype){
        .id = RIG_ARCH_SAGE_ELDER, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Sage Elder",
        .description = "El sabio anciano: frente prominente, pómulos descendidos, arrugas profundas, mirada profunda.",
        .codename = "sage",
        .params = PARAMS_INIT(
            0.96f,1.12f,0.88f, 1.55f, 0.36f,0.32f,0.32f,
            0.32f,0.29f,0.20f,0.15f,
            0.46f,0.33f,0.22f,0.21f,
            0.49f,0.16f,0.20f,0.90f,116.0f,0.18f,0.14f,
            1.02f,0.42f, 0.44f,0.48f,
            0.15f,0.40f,0.08f, 0.78f,0.75f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.550f,
        .tags = "elder,sage,wise,old,wrinkled,paternal",
    };
    g_archetypes[RIG_ARCH_SAGE_ELDER].iris = iris_amber_hazel();
    g_archetypes[RIG_ARCH_SAGE_ELDER].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_SAGE_ELDER].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_SAGE_ELDER].lip = lip_neutral(0.75f);
    g_archetypes[RIG_ARCH_SAGE_ELDER].ear_left.lobule_size = 0.7f;
    g_archetypes[RIG_ARCH_SAGE_ELDER].ear_right.lobule_size = 0.7f;

    g_archetypes[RIG_ARCH_NOBLE_ARISTOCRAT] = (RigFaceArchetype){
        .id = RIG_ARCH_NOBLE_ARISTOCRAT, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Noble Aristocrat",
        .description = "El aristócrata: nariz afilada, pómulos altos, frente estrecha elevada, expresión de altivez.",
        .codename = "noble",
        .params = PARAMS_INIT(
            0.93f,1.18f,0.84f, 1.63f, 0.35f,0.33f,0.32f,
            0.30f,0.29f,0.25f,0.18f,
            0.54f,0.28f,0.30f,0.20f,
            0.46f,0.18f,0.22f,0.88f,124.0f,0.24f,0.18f,
            1.00f,0.48f, 0.38f,0.52f,
            0.12f,0.44f,0.10f, 0.30f,0.60f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.630f,
        .tags = "noble,aristocrat,refined,high_cheekbones,sharp_nose",
    };
    g_archetypes[RIG_ARCH_NOBLE_ARISTOCRAT].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_NOBLE_ARISTOCRAT].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_NOBLE_ARISTOCRAT].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_NOBLE_ARISTOCRAT].lip = lip_neutral(0.60f);

    g_archetypes[RIG_ARCH_VILLAIN_SHARP] = (RigFaceArchetype){
        .id = RIG_ARCH_VILLAIN_SHARP, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Villain Sharp",
        .description = "El antagonista: mandíbula afilada, frente estrecha, simetría asimétrica deliberada, ojos profundos.",
        .codename = "villain",
        .params = PARAMS_INIT(
            0.90f,1.16f,0.80f, 1.52f, 0.36f,0.32f,0.32f,
            0.29f,0.28f,0.28f,0.26f,
            0.52f,0.27f,0.29f,0.19f,
            0.44f,0.16f,0.20f,0.84f,126.0f,0.26f,0.20f,
            0.96f,0.50f, 0.36f,0.50f,
            0.18f,0.38f,0.08f, 0.35f,0.82f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.520f,
        .tags = "villain,antagonist,sharp,angular,asymmetric,sinister",
    };
    g_archetypes[RIG_ARCH_VILLAIN_SHARP].iris = iris_amber_hazel();
    g_archetypes[RIG_ARCH_VILLAIN_SHARP].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_VILLAIN_SHARP].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_VILLAIN_SHARP].lip = lip_neutral(0.82f);

    g_archetypes[RIG_ARCH_ANDROGYNOUS] = (RigFaceArchetype){
        .id = RIG_ARCH_ANDROGYNOUS, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Androgynous",
        .description = "Neutro de género: proporciones equilibradas entre feminidad y masculinidad, rostro ovalado suave.",
        .codename = "androgyne",
        .params = PARAMS_INIT(
            0.97f,1.10f,0.85f, 1.61f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.21f,0.15f,
            0.48f,0.32f,0.24f,0.22f,
            0.50f,0.20f,0.24f,0.91f,118.0f,0.20f,0.18f,
            1.02f,0.44f, 0.40f,0.50f,
            0.18f,0.44f,0.12f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.610f,
        .tags = "androgynous,neutral,balanced,gender_neutral,ethereal",
    };
    g_archetypes[RIG_ARCH_ANDROGYNOUS].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_ANDROGYNOUS].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_ANDROGYNOUS].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_ANDROGYNOUS].lip = lip_neutral(0.50f);

    g_archetypes[RIG_ARCH_CHILD_ARCHETYPE] = (RigFaceArchetype){
        .id = RIG_ARCH_CHILD_ARCHETYPE, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Child Archetype",
        .description = "Rostro infantil universal: cráneo prominente, cara pequeña, mejillas redondeadas, ojos grandes.",
        .codename = "child",
        .params = PARAMS_INIT(
            1.08f,1.18f,0.95f, 1.44f, 0.28f,0.36f,0.36f,
            0.36f,0.34f,0.15f,0.08f,
            0.38f,0.32f,0.16f,0.18f,
            0.46f,0.20f,0.24f,0.80f,108.0f,0.12f,0.10f,
            0.96f,0.38f, 0.36f,0.42f,
            0.18f,0.52f,0.16f, 0.05f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.440f,
        .tags = "child,infant,young,cute,round,neoteny",
    };
    g_archetypes[RIG_ARCH_CHILD_ARCHETYPE].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_CHILD_ARCHETYPE].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_CHILD_ARCHETYPE].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_CHILD_ARCHETYPE].lip = lip_neutral(0.50f);

    g_archetypes[RIG_ARCH_SCHOLAR_REFINED] = (RigFaceArchetype){
        .id = RIG_ARCH_SCHOLAR_REFINED, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Scholar Refined",
        .description = "El intelectual: frente alta y ancha, nariz fina, ojos ligeramente hundidos, expresión contemplativa.",
        .codename = "scholar",
        .params = PARAMS_INIT(
            0.96f,1.16f,0.88f, 1.61f, 0.36f,0.33f,0.31f,
            0.31f,0.29f,0.24f,0.16f,
            0.50f,0.30f,0.26f,0.21f,
            0.47f,0.18f,0.22f,0.90f,120.0f,0.22f,0.18f,
            1.02f,0.44f, 0.40f,0.52f,
            0.14f,0.42f,0.10f, 0.32f,0.70f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.610f,
        .tags = "scholar,intellectual,refined,high_forehead,contemplative",
    };
    g_archetypes[RIG_ARCH_SCHOLAR_REFINED].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_SCHOLAR_REFINED].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_SCHOLAR_REFINED].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_SCHOLAR_REFINED].lip = lip_neutral(0.70f);

    g_archetypes[RIG_ARCH_WARRIOR_FEMALE] = (RigFaceArchetype){
        .id = RIG_ARCH_WARRIOR_FEMALE, .category = RIG_ARCHETYPE_CAT_CLASSICAL,
        .name = "Warrior Female",
        .description = "Guerrera: pómulos angulares femeninos, mandíbula firme, ojos intensos, frente media.",
        .codename = "fem_warrior",
        .params = PARAMS_INIT(
            0.98f,1.12f,0.88f, 1.62f, 0.33f,0.34f,0.33f,
            0.31f,0.31f,0.22f,0.18f,
            0.47f,0.32f,0.24f,0.22f,
            0.51f,0.22f,0.26f,0.92f,118.0f,0.20f,0.18f,
            1.06f,0.48f, 0.40f,0.50f,
            0.22f,0.48f,0.12f, 0.28f,0.25f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.620f,
        .tags = "female,warrior,strong,angular,feminine,powerful",
    };
    g_archetypes[RIG_ARCH_WARRIOR_FEMALE].iris = iris_amber_hazel();
    g_archetypes[RIG_ARCH_WARRIOR_FEMALE].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_WARRIOR_FEMALE].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_WARRIOR_FEMALE].lip = lip_neutral(0.25f);

    g_archetypes[RIG_ARCH_AGE_INFANT] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_INFANT, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Infant",
        .description = "Neonato: proporciones neoténicas máximas, cráneo 60% de la cara, mejillas pronunciadas.",
        .codename = "age_0",
        .params = PARAMS_INIT(
            1.12f,1.24f,1.00f, 1.38f, 0.26f,0.38f,0.36f,
            0.38f,0.36f,0.12f,0.06f,
            0.32f,0.34f,0.12f,0.16f,
            0.42f,0.22f,0.26f,0.74f,105.0f,0.08f,0.06f,
            0.90f,0.34f, 0.30f,0.36f,
            0.18f,0.58f,0.18f, 0.00f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.380f,
        .tags = "infant,baby,neonate,age_0,neoteny_max",
    };

    g_archetypes[RIG_ARCH_AGE_CHILD_7] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_CHILD_7, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Child 7yr",
        .description = "Niño de 7 años: cara en desarrollo, dientes de leche reemplazados, frente amplia, nariz pequeña.",
        .codename = "age_7",
        .params = PARAMS_INIT(
            1.06f,1.16f,0.92f, 1.48f, 0.30f,0.36f,0.34f,
            0.34f,0.32f,0.16f,0.10f,
            0.40f,0.32f,0.18f,0.20f,
            0.44f,0.20f,0.24f,0.82f,110.0f,0.14f,0.12f,
            0.94f,0.38f, 0.34f,0.40f,
            0.16f,0.50f,0.14f, 0.06f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.480f,
        .tags = "child_7,age_7,school_age,developing",
    };

    g_archetypes[RIG_ARCH_AGE_TEEN_16] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_TEEN_16, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Teen 16yr",
        .description = "Adolescente 16 años: rasgos en transición adulta, proporciones casi completas, piel con volumen.",
        .codename = "age_16",
        .params = PARAMS_INIT(
            0.99f,1.12f,0.87f, 1.57f, 0.32f,0.34f,0.34f,
            0.31f,0.30f,0.20f,0.15f,
            0.46f,0.33f,0.22f,0.22f,
            0.50f,0.22f,0.26f,0.90f,116.0f,0.18f,0.16f,
            1.02f,0.42f, 0.38f,0.46f,
            0.18f,0.48f,0.14f, 0.12f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.570f,
        .tags = "teen,16,adolescent,transitional",
    };

    g_archetypes[RIG_ARCH_AGE_YOUNG_25] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_YOUNG_25, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Young Adult 25yr",
        .description = "Adulto joven 25 años: colágeno máximo, proporciones completas, piel sin marcas de tiempo.",
        .codename = "age_25",
        .params = PARAMS_INIT(
            0.98f,1.12f,0.87f, 1.62f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.22f,0.17f,
            0.48f,0.33f,0.24f,0.22f,
            0.50f,0.20f,0.24f,0.92f,118.0f,0.20f,0.18f,
            1.04f,0.44f, 0.40f,0.50f,
            0.20f,0.44f,0.10f, 0.25f,0.50f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.620f,
        .tags = "young_adult,25,prime,collagen_peak",
    };

    g_archetypes[RIG_ARCH_AGE_MATURE_45] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_MATURE_45, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Mature 45yr",
        .description = "Adulto maduro 45 años: surcos nasolabiales, párpado superior con leve ptosis, pérdida de colágeno.",
        .codename = "age_45",
        .params = PARAMS_INIT(
            0.97f,1.10f,0.86f, 1.58f, 0.34f,0.33f,0.33f,
            0.31f,0.29f,0.21f,0.16f,
            0.47f,0.33f,0.23f,0.22f,
            0.49f,0.18f,0.22f,0.91f,118.0f,0.19f,0.17f,
            1.03f,0.43f, 0.42f,0.50f,
            0.20f,0.42f,0.10f, 0.48f,0.55f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.580f,
        .tags = "mature,45,middle_aged,experienced,character",
    };

    g_archetypes[RIG_ARCH_AGE_ELDER_70] = (RigFaceArchetype){
        .id = RIG_ARCH_AGE_ELDER_70, .category = RIG_ARCHETYPE_CAT_AGE_STUDY,
        .name = "Age Study: Elder 70yr",
        .description = "Anciano 70 años: tejido descendido, arrugas profundas, pérdida ósea alveolar, cuello flácido.",
        .codename = "age_70",
        .params = PARAMS_INIT(
            0.94f,1.08f,0.84f, 1.50f, 0.36f,0.32f,0.32f,
            0.32f,0.27f,0.18f,0.12f,
            0.44f,0.32f,0.20f,0.20f,
            0.47f,0.14f,0.18f,0.88f,114.0f,0.16f,0.12f,
            1.00f,0.40f, 0.44f,0.46f,
            0.16f,0.38f,0.08f, 0.80f,0.60f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.500f,
        .tags = "elder,70,old,aged,wisdom,wrinkled,jowl",
    };
    g_archetypes[RIG_ARCH_AGE_ELDER_70].ear_left.lobule_size = 0.8f;
    g_archetypes[RIG_ARCH_AGE_ELDER_70].ear_right.lobule_size = 0.8f;

    g_archetypes[RIG_ARCH_GREEK_IDEAL] = (RigFaceArchetype){
        .id = RIG_ARCH_GREEK_IDEAL, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Greek Ideal",
        .description = "Canon griego clásico (Policleto, Praxíteles): perfil en línea recta frente-nariz, cara ovalada perfecta.",
        .codename = "greek",
        .params = PARAMS_INIT(
            0.97f,1.14f,0.87f, 1.618f, 0.333f,0.333f,0.334f,
            0.30f,0.30f,0.24f,0.20f,
            0.50f,0.30f,0.28f,0.20f,
            0.48f,0.19f,0.23f,0.90f,120.0f,0.22f,0.18f,
            1.02f,0.46f, 0.40f,0.50f,
            0.12f,0.44f,0.10f, 0.26f,0.55f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "greek,classical,ideal,polykleitos,praxiteles,marble",
    };
    g_archetypes[RIG_ARCH_GREEK_IDEAL].iris = iris_blue_green();
    g_archetypes[RIG_ARCH_GREEK_IDEAL].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_GREEK_IDEAL].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_GREEK_IDEAL].lip = lip_neutral(0.55f);

    g_archetypes[RIG_ARCH_VITRUVIAN_CANON] = (RigFaceArchetype){
        .id = RIG_ARCH_VITRUVIAN_CANON, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Vitruvian Canon",
        .description = "Canon vitruviano revisado por Da Vinci: tercios iguales perfectos, ancho=altura×0.618.",
        .codename = "vitruvian",
        .params = PARAMS_INIT(
            0.96f,1.12f,0.86f, 1.618f, 0.333f,0.333f,0.334f,
            0.309f,0.309f,0.22f,0.18f,
            0.48f,0.309f,0.26f,0.21f,
            0.479f,0.19f,0.23f,0.91f,119.0f,0.21f,0.17f,
            1.03f,0.44f, 0.41f,0.51f,
            0.18f,0.44f,0.11f, 0.26f,0.55f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "vitruvian,da_vinci,thirds,classical,ideal_proportions",
    };

    g_archetypes[RIG_ARCH_PHI_PERFECT] = (RigFaceArchetype){
        .id = RIG_ARCH_PHI_PERFECT, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Phi Perfect",
        .description = "Máscara de Marquardt φ-perfecta: todas las proporciones son razones de φ. Error φ < 0.001.",
        .codename = "phi_perfect",
        .params = PARAMS_INIT(
            0.9511f,1.1756f,0.8090f, 1.6180f, 0.3333f,0.3333f,0.3334f,
            0.3090f,0.3090f,0.2360f,0.1910f,
            0.5000f,0.3090f,0.2580f,0.2000f,
            0.4760f,0.1910f,0.2360f,0.9020f,118.8f,0.2360f,0.1910f,
            1.0000f,0.4760f, 0.3820f,0.5000f,
            0.15f,0.44f,0.10f, 0.25f,0.52f),
        .recommended_subdiv = 6, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "phi,golden_ratio,marquardt,perfect,mathematical",
    };

    g_archetypes[RIG_ARCH_HYPERREALIST] = (RigFaceArchetype){
        .id = RIG_ARCH_HYPERREALIST, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Hyperrealist",
        .description = "Hiperrealismo: asimetrías naturales calibradas, micro-defectos auténticos, máximo detalle dérmico.",
        .codename = "hyperreal",
        .params = PARAMS_INIT(
            0.983f,1.107f,0.877f, 1.587f, 0.332f,0.341f,0.327f,
            0.308f,0.297f,0.218f,0.167f,
            0.478f,0.332f,0.241f,0.219f,
            0.501f,0.203f,0.243f,0.924f,117.3f,0.198f,0.178f,
            1.038f,0.437f, 0.401f,0.502f,
            0.24f,0.45f,0.11f, 0.27f,0.53f),
        .recommended_subdiv = 6, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.587f,
        .tags = "hyperrealist,cinematic,film_vfx,asymmetric,natural,pores",
    };

    g_archetypes[RIG_ARCH_STYLIZED_ANIME] = (RigFaceArchetype){
        .id = RIG_ARCH_STYLIZED_ANIME, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Stylized Anime",
        .description = "Estilo anime: ojos grandes (40% cara), nariz mínima, boca pequeña, mentón afilado, frente alta.",
        .codename = "anime",
        .params = PARAMS_INIT(
            0.95f,1.20f,0.75f, 1.44f, 0.28f,0.38f,0.34f,
            0.40f,0.40f,0.10f,0.06f,
            0.35f,0.22f,0.15f,0.14f,
            0.38f,0.12f,0.16f,0.78f,108.0f,0.14f,0.16f,
            0.90f,0.38f, 0.32f,0.42f,
            0.15f,0.40f,0.12f, 0.20f,0.40f),
        .recommended_subdiv = 4, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.440f,
        .tags = "anime,stylized,manga,large_eyes,pointed_chin,2d_inspired",
    };

    g_archetypes[RIG_ARCH_RENAISSANCE_BEAUTY] = (RigFaceArchetype){
        .id = RIG_ARCH_RENAISSANCE_BEAUTY, .category = RIG_ARCHETYPE_CAT_ARTISTIC,
        .name = "Renaissance Beauty",
        .description = "Belleza renacentista (Botticelli, Leonardo): frente despejada, nariz recta, labios carnosos, óvalo perfecto.",
        .codename = "renaissance",
        .params = PARAMS_INIT(
            0.95f,1.15f,0.85f, 1.61f, 0.34f,0.33f,0.33f,
            0.30f,0.30f,0.22f,0.16f,
            0.48f,0.31f,0.26f,0.21f,
            0.50f,0.24f,0.28f,0.90f,120.0f,0.20f,0.16f,
            1.02f,0.46f, 0.38f,0.50f,
            0.14f,0.50f,0.16f, 0.22f,0.20f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.610f,
        .tags = "renaissance,botticelli,female,beauty,oval,classical_art",
    };

    g_archetypes[RIG_ARCH_ALBINISM] = (RigFaceArchetype){
        .id = RIG_ARCH_ALBINISM, .category = RIG_ARCHETYPE_CAT_SPECIAL,
        .name = "Albinism",
        .description = "Albinismo OCA1: melanina=0, alta translucidez, iris azul pálido con visibilidad del epitelio pigmentario.",
        .codename = "albino",
        .params = PARAMS_INIT(
            0.98f,1.12f,0.86f, 1.60f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.22f,0.17f,
            0.48f,0.32f,0.24f,0.22f,
            0.50f,0.20f,0.24f,0.92f,118.0f,0.20f,0.18f,
            1.03f,0.44f, 0.40f,0.50f,
            0.00f,0.78f,0.02f, 0.25f,0.50f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.600f,
        .tags = "albinism,oca1,melanin_zero,translucent,rare,clinical",
    };
    {
        RigFaceIrisDetail al_iris = {0};
        al_iris.iris_color[0] = 0.75f; al_iris.iris_color[1] = 0.82f; al_iris.iris_color[2] = 0.90f;
        al_iris.limbal_ring_width = 0.3f; al_iris.limbal_ring_darkness = 0.2f;
        al_iris.pattern = RIG_IRIS_PATTERN_UNIFORM; al_iris.pupil_shape = RIG_PUPIL_ROUND;
        al_iris.iris_radius = 5.8f; al_iris.pupil_radius = 4.5f;
        al_iris.crypt_density = 0.1f; al_iris.wolfflin_count = 0.0f; al_iris.furrow_count = 2.0f;
        g_archetypes[RIG_ARCH_ALBINISM].iris = al_iris;
    }
    g_archetypes[RIG_ARCH_ALBINISM].ear_left  = ear_default(false);
    g_archetypes[RIG_ARCH_ALBINISM].ear_right = ear_default(true);
    g_archetypes[RIG_ARCH_ALBINISM].lip = lip_neutral(0.50f);

    g_archetypes[RIG_ARCH_VITILIGO] = (RigFaceArchetype){
        .id = RIG_ARCH_VITILIGO, .category = RIG_ARCHETYPE_CAT_SPECIAL,
        .name = "Vitiligo",
        .description = "Vitiligo segmentario: pérdida localizada de melanocitos con patrón fractal en parches.",
        .codename = "vitiligo",
        .params = PARAMS_INIT(
            0.98f,1.12f,0.86f, 1.60f, 0.33f,0.34f,0.33f,
            0.31f,0.30f,0.22f,0.17f,
            0.48f,0.32f,0.24f,0.22f,
            0.50f,0.20f,0.24f,0.92f,118.0f,0.20f,0.18f,
            1.03f,0.44f, 0.40f,0.50f,
            0.40f,0.48f,0.08f, 0.30f,0.50f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.600f,
        .tags = "vitiligo,depigmentation,fractal_pattern,clinical,diverse",
    };
    g_archetypes[RIG_ARCH_VITILIGO].iris = iris_brown_dark();

    g_archetypes[RIG_ARCH_BATTLE_SCARRED] = (RigFaceArchetype){
        .id = RIG_ARCH_BATTLE_SCARRED, .category = RIG_ARCHETYPE_CAT_SPECIAL,
        .name = "Battle Scarred",
        .description = "Guerrero veterano: cicatrices lineales en múltiples regiones, deformación nasal, ptosis cicatricial.",
        .codename = "scarred",
        .params = PARAMS_INIT(
            1.02f,1.12f,0.90f, 1.56f, 0.33f,0.33f,0.34f,
            0.30f,0.29f,0.24f,0.22f,
            0.47f,0.35f,0.25f,0.24f,
            0.50f,0.18f,0.22f,0.98f,112.0f,0.22f,0.20f,
            1.06f,0.44f, 0.46f,0.52f,
            0.28f,0.44f,0.10f, 0.45f,0.82f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.560f,
        .tags = "scarred,veteran,battle,warrior,character,damaged,cinematic",
    };
    g_archetypes[RIG_ARCH_BATTLE_SCARRED].iris = iris_amber_hazel();

    g_archetypes[RIG_ARCH_NEONATAL] = (RigFaceArchetype){
        .id = RIG_ARCH_NEONATAL, .category = RIG_ARCHETYPE_CAT_SPECIAL,
        .name = "Neonatal",
        .description = "Recién nacido: craneometría neonatal, fontanelas visibles, cara 1/4 del cráneo, edema subcutáneo.",
        .codename = "neonate",
        .params = PARAMS_INIT(
            1.18f,1.28f,1.04f, 1.30f, 0.22f,0.40f,0.38f,
            0.40f,0.38f,0.10f,0.04f,
            0.28f,0.36f,0.08f,0.14f,
            0.38f,0.24f,0.28f,0.68f,102.0f,0.06f,0.04f,
            0.84f,0.32f, 0.28f,0.32f,
            0.16f,0.60f,0.20f, 0.00f,0.50f),
        .recommended_subdiv = 4, .build_ears = false, .build_neck = true,
        .build_skeleton = false, .build_shapes = false,
        .phi_reference = 1.300f,
        .tags = "neonate,newborn,neonatal,infant,medical,fontanelle",
    };

    g_archetypes[RIG_ARCH_APOLLONIAN] = (RigFaceArchetype){
        .id = RIG_ARCH_APOLLONIAN, .category = RIG_ARCHETYPE_CAT_LEGENDARY,
        .name = "Apollonian",
        .description = "El Apolíneo: luz solar, razón, orden. Proporciones griegas puras, expresión de serenidad perpetua.",
        .codename = "apollo",
        .params = PARAMS_INIT(
            0.951f,1.176f,0.809f, 1.6180f, 0.333f,0.333f,0.334f,
            0.309f,0.309f,0.236f,0.191f,
            0.500f,0.309f,0.258f,0.200f,
            0.476f,0.191f,0.236f,0.902f,120.0f,0.236f,0.191f,
            1.000f,0.476f, 0.382f,0.500f,
            0.10f,0.44f,0.10f, 0.25f,0.60f),
        .recommended_subdiv = 6, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "apollo,solar,greek,legendary,godlike,serene,phi",
    };
    g_archetypes[RIG_ARCH_APOLLONIAN].iris = iris_blue_green();

    g_archetypes[RIG_ARCH_MAYAN_CLASSIC] = (RigFaceArchetype){
        .id = RIG_ARCH_MAYAN_CLASSIC, .category = RIG_ARCHETYPE_CAT_LEGENDARY,
        .name = "Mayan Classic",
        .description = "Canon maya clásico (600 d.C.): frente inclinada, nariz aquilina, pómulos amplios, rasgos ceremoniales.",
        .codename = "mayan",
        .params = PARAMS_INIT(
            1.04f,1.16f,0.92f, 1.56f, 0.30f,0.36f,0.34f,
            0.32f,0.28f,0.20f,0.14f,
            0.48f,0.38f,0.22f,0.24f,
            0.52f,0.24f,0.28f,1.00f,114.0f,0.20f,0.18f,
            1.12f,0.46f, 0.44f,0.48f,
            0.35f,0.46f,0.20f, 0.28f,0.60f),
        .recommended_subdiv = 5, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.560f,
        .tags = "mayan,mesoamerican,classic,ceremonial,aquiline,pre_columbian",
    };
    g_archetypes[RIG_ARCH_MAYAN_CLASSIC].iris = iris_midnight();

    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN] = (RigFaceArchetype){
        .id = RIG_ARCH_RIGADIEL_SOVEREIGN, .category = RIG_ARCHETYPE_CAT_LEGENDARY,
        .name = "RIGADIEL Sovereign",
        .description = "φ² · φ · φ⁻¹ — El arquetipo soberano de RigCom. "
                        "Núcleos solares: Edgar José Gabriel Mora (φ²). "
                        "Guardia lunar: La Negra María (φ⁻¹). "
                        "Centro: Richard Felipe Urbina (φ). Tres resonancias.",
        .codename = "rigadiel",
        .params = PARAMS_INIT(

            0.9511f,1.1756f,0.8090f, 1.6180f, 0.3333f,0.3333f,0.3334f,
            0.3090f,0.3090f,0.2361f,0.1910f,
            0.5000f,0.3090f,0.2580f,0.2000f,
            0.4760f,0.1910f,0.2360f,0.9020f,118.8f,0.2360f,0.1910f,
            1.0000f,0.4760f, 0.3820f,0.5000f,
            0.25f,0.46f,0.12f, 0.25f,0.55f),
        .recommended_subdiv = 6, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "rigadiel,sovereign,phi,rigcom,gabriel,negra_maria,richard",
    };
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris = iris_amber_hazel();
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.heterochromia = true;
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.secondary_color[0] = 0.35f;
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.secondary_color[1] = 0.58f;
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.secondary_color[2] = 0.72f;
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.secondary_sector_start = 0.0f;
    g_archetypes[RIG_ARCH_RIGADIEL_SOVEREIGN].iris.secondary_sector_end   = 1.2f;

    g_archetypes[RIG_ARCH_OMEGA_MIND] = (RigFaceArchetype){
        .id = RIG_ARCH_OMEGA_MIND, .category = RIG_ARCHETYPE_CAT_LEGENDARY,
        .name = "Omega Mind",
        .description = "RIGADIEL Omega Mind: 20 capas lobulares · 41616 neuronas · Hodgkin-Huxley. "
                        "Rostro emergente de la red neuronal determinista. "
                        "Frecuencia Schumann 7.83 Hz integrada en la geometría.",
        .codename = "omega",
        .params = PARAMS_INIT(

            0.9511f,1.2490f,0.8090f, 1.6180f, 0.3333f,0.3333f,0.3334f,
            0.3090f,0.3090f,0.2490f,0.2000f,
            0.5000f,0.3090f,0.2500f,0.2000f,
            0.4760f,0.2000f,0.2490f,0.9020f,120.0f,0.2490f,0.2000f,
            1.0000f,0.4760f, 0.3820f,0.5000f,
            0.20f,0.48f,0.10f, 0.30f,0.50f),
        .recommended_subdiv = 6, .build_ears = true, .build_neck = true,
        .build_skeleton = true, .build_shapes = true,
        .phi_reference = 1.6180339887498948f,
        .tags = "omega,rigadiel,neural,deterministic,schumann,hodgkin_huxley",
    };
    g_archetypes[RIG_ARCH_OMEGA_MIND].iris = iris_midnight();
    g_archetypes[RIG_ARCH_OMEGA_MIND].iris.limbal_ring_width = 2.0f;
    g_archetypes[RIG_ARCH_OMEGA_MIND].iris.limbal_ring_darkness = 1.0f;

    for (int i = 0; i < RIG_ARCH_COUNT; i++) {
        RigFaceArchetype *a = &g_archetypes[i];
        if (a->ear_left.ear_height  == 0.0f) a->ear_left  = ear_default(false);
        if (a->ear_right.ear_height == 0.0f) a->ear_right = ear_default(true);
        if (a->lip.upper_lip_height == 0.0f) a->lip = lip_neutral(a->params.gender_factor);
        if (a->iris.iris_radius     == 0.0f) a->iris = iris_brown_dark();
        if (a->roughness_override    == 0.0f) a->roughness_override = -1.0f;
    }

    g_archetypes_init = true;
}

/* [fusion] function rig_archetype_list_all__v2 <- nested/16_FACE_NG-1/16_FACE_NG/src/rig_face_archetypes.c:971 :: cuerpo divergente; ausente en la copia canonica */
void rig_archetype_list_all__v2(void)
{
    init_archetypes();
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  RigCom v24 THE SANTORIUM OF COMPILER — Face Archetypes  —  36 Templates               ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");

    RigArchetypeCategory last_cat = (RigArchetypeCategory)-1;
    for (int i = 0; i < RIG_ARCH_COUNT; i++) {
        const RigFaceArchetype *a = &g_archetypes[i];
        if (a->category != last_cat) {
            printf("║  ── %s ──\n", rig_archetype_category_name(a->category));
            last_cat = a->category;
        }
        printf("║  [%02d] %-22s  %-16s  φ=%.4f  subdiv=%u\n",
               a->id, a->name, a->codename,
               a->phi_reference, a->recommended_subdiv);
    }
    printf("╚══════════════════════════════════════════════════════════════╝\n");
}

