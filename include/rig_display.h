#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"

typedef struct { uint32_t clock; uint16_t hdisplay, hsync_start, hsync_end, htotal, hskew; uint16_t vdisplay, vsync_start, vsync_end, vtotal, vscan; uint32_t vrefresh; uint32_t flags; uint32_t type; char name[RIG_DRM_DISPLAY_MODE_LEN]; } RigDrmModeInfo;
typedef struct { uint64_t fb_id_ptr; uint64_t crtc_id_ptr; uint64_t connector_id_ptr; uint64_t encoder_id_ptr; uint32_t count_fbs; uint32_t count_crtcs; uint32_t count_connectors; uint32_t count_encoders; uint32_t min_width; uint32_t max_width; uint32_t min_height; uint32_t max_height; } RigDrmCardRes;
typedef struct { uint64_t encoders_ptr; uint64_t modes_ptr; uint64_t props_ptr; uint64_t prop_values_ptr; uint32_t count_modes; uint32_t count_props; uint32_t count_encoders; uint32_t encoder_id; uint32_t connector_id; uint32_t connector_type; uint32_t connector_type_id; uint32_t connection; uint32_t mm_width; uint32_t mm_height; uint32_t subpixel; uint32_t pad; } RigDrmGetConnector;
typedef struct { uint32_t encoder_id; uint32_t encoder_type; uint32_t crtc_id; uint32_t possible_crtcs; uint32_t possible_clones; } RigDrmGetEncoder;
typedef struct { uint64_t set_connectors_ptr; uint32_t count_connectors; uint32_t crtc_id; uint32_t fb_id; uint32_t x, y; uint32_t gamma_size; uint32_t mode_valid; RigDrmModeInfo mode; } RigDrmModeCrtc;
typedef struct { uint32_t fb_id; uint32_t width, height; uint32_t pitch; uint32_t bpp; uint32_t depth; uint32_t handle; } RigDrmModeFbCmd;
typedef struct { uint32_t height; uint32_t width; uint32_t bpp; uint32_t flags; uint32_t handle; uint32_t pitch; uint64_t size; } RigDrmCreateDumb;
typedef struct { uint32_t handle; uint32_t pad; uint64_t offset; } RigDrmMapDumb;
typedef struct { uint32_t handle; } RigDrmDestroyDumb;
typedef struct { uint32_t crtc_id; uint32_t fb_id; uint32_t flags; uint32_t reserved; uint64_t user_data; } RigDrmPageFlip;

RigDisplayBackend rig_display_detect_backend__rig_dup_0545c040(void);
const char* rig_display_backend_name__rig_dup_f0bbf2db(RigDisplayBackend b);
int rig_display_enable_hdr__rig_dup_0928162d(RigDisplayCtx *disp, float max_nits);
int rig_display_enum_modes__rig_variant_43341523(RigDisplayCtx *disp);
int rig_display_init__rig_variant_bcbfaf77(RigDisplayCtx *disp, RigGPUCtx *gpu, RigDisplayBackend backend);
int rig_display_present__rig_dup_059a895a(RigDisplayCtx *disp);
int rig_display_set_mode__rig_dup_92918bb3(RigDisplayCtx *disp, uint32_t w, uint32_t h, uint32_t hz);
int rig_display_submit_frame__rig_dup_95fff949(RigDisplayCtx *disp, const RigComFrame *frame);
int rig_drm_alloc_buffers__rig_variant_07177637(RigDRMCtx *drm, uint32_t w, uint32_t h);
int rig_drm_open__rig_variant_7657ae35(RigDRMCtx *drm, const char *device);
int rig_drm_page_flip__rig_variant_4e9fd61b(RigDRMCtx *drm);
uint32_t rig_display_create_surface__rig_variant_6496cec0(RigDisplayCtx *disp, const char *name, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void rig_display_destroy__rig_variant_7b47f0ca(RigDisplayCtx *disp);
void rig_display_destroy_surface__rig_variant_89c8d5be(RigDisplayCtx *disp, uint32_t surf_id);
void rig_display_emit_status__rig_variant_239687f8(const RigDisplayCtx *disp, char *out, size_t outsz);
void rig_display_update_fps__rig_variant_bc09c3eb(RigDisplayCtx *disp);
void rig_display_wait_vsync__rig_variant_d2be28e8(RigDisplayCtx *disp);
void rig_drm_close__rig_variant_242f6a72(RigDRMCtx *drm);
