/* 
 * RIGFACE · Acoustic-Optical Modulator (AOM) Physical Synthesis Header
 * Hardware Target: Honor 400 DNY-NX9 (Snapdragon 7 Gen 3 / Adreno 720 / Android 16)
 * Architect: Richard Felipe Urbina
 */

#ifndef RIG_AOM_PHYSICAL_H
#define RIG_AOM_PHYSICAL_H

#include <stdint.h>
#include <stdbool.h>

#define RIG_AOM_MATRIX_ROWS 8
#define RIG_AOM_MATRIX_COLS 8
#define RIG_AOM_ULTRASONIC_FREQ_HZ 40000.0f
#define RIG_AOM_AUDIO_SAMPLE_RATE 48000
#define RIG_AOM_SCHUMANN_FREQ_HZ 7.83f

/* 1. Unified Master Clock & Physical Event Frame */
typedef struct {
    uint64_t event_id;
    uint64_t timestamp_ns;
    uint64_t execution_time_ns;
    uint32_t duration_ms;
    
    /* Origin Traceability */
    const char *source_file;
    const char *source_function;
    uint32_t source_line;
    
    /* Latency & Sync Delta */
    float optical_latency_ms;
    float acoustic_latency_ms;
    float ultrasonic_latency_ms;
    float sync_delta_ms;
    
    bool active;
    bool visible;
} RigAomClockEvent;

/* 2. Optical Output State (Adreno 720 Screen Emission) */
typedef struct {
    uint32_t pixel_x;
    uint32_t pixel_y;
    float color_rgb[3];
    float intensity_nits;
    
    /* Material & Optical Properties */
    float bragg_angle_rad;
    float diffraction_efficiency;
    float normal[3];
    float roughness;
    float metallic;
    float transmission;
    float absorption_beer_lambert[3];
    float sss_radius[3];
    float iridescence;
    
    /* Head & Eye Correction */
    float head_pos_cm[3];
    float eye_gaze_dir[3];
    float off_axis_projection[16];
} RigAomOpticalOutput;

/* 3. Ultrasonic & Haptic Physical Output (8x8 Matrix @ 40 kHz) */
typedef struct {
    float focal_point_3d[3];      /* Focal position relative to Honor 400 (cm) */
    float contact_point_2d[2];    /* Screen contact (x, y) */
    
    /* 8x8 Transducer Array Control */
    float element_phase[RIG_AOM_MATRIX_ROWS][RIG_AOM_MATRIX_COLS];     /* Radians */
    float element_amplitude[RIG_AOM_MATRIX_ROWS][RIG_AOM_MATRIX_COLS]; /* 0.0 to 1.0 */
    
    /* Modulation & Forces */
    float am_freq_hz;
    float stm_freq_hz;
    float acoustic_pressure_pa;
    float tactile_force_n;
    
    /* Material Coupling */
    float friction_coefficient;
    float surface_relief_um;
    float finger_velocity_cms;
} RigAomUltrasonicOutput;

/* 4. Acoustic & Audible Output (Honor 400 Audio Hardware) */
typedef struct {
    float glottal_lf_rd;
    float pitch_hz;
    float formants_hz[5];
    float formant_bandwidths_hz[5];
    
    /* Surface & Friction Sound */
    float surface_noise_level;
    float friction_freq_hz;
    
    /* Voice PCM Buffer (48 kHz mono PCM16) */
    int16_t pcm_buffer[1024];
    uint32_t pcm_samples_count;
    
    /* Viseme & Phoneme Sync */
    char current_phoneme[8];
    uint8_t current_viseme_id;
} RigAomAcousticOutput;

/* 5. Master AOM Synchronized State */
typedef struct {
    RigAomClockEvent master_clock;
    RigAomOpticalOutput optical;
    RigAomUltrasonicOutput ultrasonic;
    RigAomAcousticOutput acoustic;
    
    /* Voxel Grid State */
    uint32_t active_voxel_id;
    float voxel_density_rgb[3];
    const char *active_material_name;
} RigAomMasterState;

/* Physical AOM Control API */
void rig_aom_init(RigAomMasterState *state);
void rig_aom_process_event(RigAomMasterState *state, float touch_x, float touch_y, float touch_force);
void rig_aom_update_head_tracking(RigAomMasterState *state, const float head_cm[3], const float gaze_dir[3]);
void rig_aom_execute_hardware_emissions(RigAomMasterState *state);

#endif /* RIG_AOM_PHYSICAL_H */
