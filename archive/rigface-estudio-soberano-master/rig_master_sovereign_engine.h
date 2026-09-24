#ifndef RIG_MASTER_SOVEREIGN_ENGINE_H
#define RIG_MASTER_SOVEREIGN_ENGINE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define RIG_PHI 1.61803398874989484820
#define RIG_SCHUMANN_HZ 7.83
#define RIG_TARGET_DEVICE "Honor 400 DNY-NX9 (Snapdragon 7 Gen 3 / Adreno 720 / Android 16)"
#define RIGCOM_PUBLIC

typedef struct {
    uint64_t session_id;
    char project_name[128];
    char user_curp[19];
    float age_years;
    float gender_dimorphism;
    uint32_t active_archetype_id;
    float genetics_pca[48];
    uint8_t facs_au[128];
    float aom_nits;
    float aom_bragg_deg;
    float aom_pressure_pa;
    float aom_acoustic_hz;
    uint64_t timestamp_ns;
} RigSovereignState;

RIGCOM_PUBLIC void rig_master_init_sovereign(RigSovereignState *state);
RIGCOM_PUBLIC void rig_master_execute_aom_triple(RigSovereignState *state, float x, float y, float z);
RIGCOM_PUBLIC void rig_master_process_camera_landmarks(RigSovereignState *state, const float *landmarks_68x2);
RIGCOM_PUBLIC void rig_master_export_manifest_sha256(const char *output_path);

#endif // RIG_MASTER_SOVEREIGN_ENGINE_H
