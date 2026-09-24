/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#define _POSIX_C_SOURCE 200809L

#include "rig_display.h"
#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_syscall.h"
#include "rig_syscall.h"

#if defined(__linux__) && !defined(RIG_GPU_STUB)
  #define RIG_HAS_DRM 1
  #include "rig_syscall.h"
  #include "rig_syscall.h"
  #include "rig_syscall.h"
  #include "rig_syscall.h"
  #include "rig_syscall.h"
#endif

#if defined(__ANDROID__) && defined(RIG_USE_ANATIVE)
  #define RIG_HAS_ANATIVE 1
/* [SOBERANO] rigdeps/android/native_window.h eliminado — no hay equivalente soberano o no disponible en build soberano */
#endif

#ifdef RIG_HAS_DRM

#define RIG_DRM_MODE_CONNECTED       1u
#define RIG_DRM_MODE_DISCONNECTED    2u
#define RIG_DRM_MODE_TYPE_PREFERRED  (1u << 3)
#define RIG_DRM_PAGE_FLIP_EVENT      (1u << 0)
#define RIG_DRM_DISPLAY_MODE_LEN     32

#pragma pack(push,1)

typedef struct {
    uint32_t clock;
    uint16_t hdisplay, hsync_start, hsync_end, htotal, hskew;
    uint16_t vdisplay, vsync_start, vsync_end, vtotal, vscan;
    uint32_t vrefresh;
    uint32_t flags;
    uint32_t type;
    char     name[RIG_DRM_DISPLAY_MODE_LEN];
} RigDrmModeInfo;

typedef struct {
    uint64_t fb_id_ptr;
    uint64_t crtc_id_ptr;
    uint64_t connector_id_ptr;
    uint64_t encoder_id_ptr;
    uint32_t count_fbs;
    uint32_t count_crtcs;
    uint32_t count_connectors;
    uint32_t count_encoders;
    uint32_t min_width;  uint32_t max_width;
    uint32_t min_height; uint32_t max_height;
} RigDrmCardRes;

typedef struct {
    uint64_t encoders_ptr;
    uint64_t modes_ptr;
    uint64_t props_ptr;
    uint64_t prop_values_ptr;
    uint32_t count_modes;
    uint32_t count_props;
    uint32_t count_encoders;
    uint32_t encoder_id;
    uint32_t connector_id;
    uint32_t connector_type;
    uint32_t connector_type_id;
    uint32_t connection;
    uint32_t mm_width;
    uint32_t mm_height;
    uint32_t subpixel;
    uint32_t pad;
} RigDrmGetConnector;

typedef struct {
    uint32_t encoder_id;
    uint32_t encoder_type;
    uint32_t crtc_id;
    uint32_t possible_crtcs;
    uint32_t possible_clones;
} RigDrmGetEncoder;

typedef struct {
    uint64_t set_connectors_ptr;
    uint32_t count_connectors;
    uint32_t crtc_id;
    uint32_t fb_id;
    uint32_t x, y;
    uint32_t gamma_size;
    uint32_t mode_valid;
    RigDrmModeInfo mode;
} RigDrmModeCrtc;

typedef struct {
    uint32_t fb_id;
    uint32_t width, height;
    uint32_t pitch;
    uint32_t bpp;
    uint32_t depth;
    uint32_t handle;
} RigDrmModeFbCmd;

typedef struct {
    uint32_t height;
    uint32_t width;
    uint32_t bpp;
    uint32_t flags;
    uint32_t handle;
    uint32_t pitch;
    uint64_t size;
} RigDrmCreateDumb;

typedef struct {
    uint32_t handle;
    uint32_t pad;
    uint64_t offset;
} RigDrmMapDumb;

typedef struct {
    uint32_t handle;
} RigDrmDestroyDumb;

typedef struct {
    uint32_t crtc_id;
    uint32_t fb_id;
    uint32_t flags;
    uint32_t reserved;
    uint64_t user_data;
} RigDrmPageFlip;

#pragma pack(pop)

#define RIG_DRM_BASE  ((unsigned long)'d')
#define _RIG_IOWR(nr, tsz) \
    (((unsigned long)3 << 30) | (RIG_DRM_BASE << 8) | (unsigned long)(nr) | ((tsz) << 16))
#define _RIG_IOW(nr, tsz) \
    (((unsigned long)1 << 30) | (RIG_DRM_BASE << 8) | (unsigned long)(nr) | ((tsz) << 16))

#define RIG_IOCTL_MODE_GETRESOURCES  _RIG_IOWR(0xa0, sizeof(RigDrmCardRes))
#define RIG_IOCTL_MODE_GETCRTC       _RIG_IOWR(0xa1, sizeof(RigDrmModeCrtc))
#define RIG_IOCTL_MODE_SETCRTC       _RIG_IOWR(0xa2, sizeof(RigDrmModeCrtc))
#define RIG_IOCTL_MODE_GETCONNECTOR  _RIG_IOWR(0xa7, sizeof(RigDrmGetConnector))
#define RIG_IOCTL_MODE_GETENCODER    _RIG_IOWR(0xa6, sizeof(RigDrmGetEncoder))
#define RIG_IOCTL_MODE_ADDFB         _RIG_IOWR(0xae, sizeof(RigDrmModeFbCmd))
#define RIG_IOCTL_MODE_RMFB          _RIG_IOWR(0xaf, sizeof(unsigned int))
#define RIG_IOCTL_MODE_PAGE_FLIP     _RIG_IOWR(0xb0, sizeof(RigDrmPageFlip))
#define RIG_IOCTL_MODE_CREATE_DUMB   _RIG_IOWR(0xb2, sizeof(RigDrmCreateDumb))
#define RIG_IOCTL_MODE_MAP_DUMB      _RIG_IOWR(0xb3, sizeof(RigDrmMapDumb))
#define RIG_IOCTL_MODE_DESTROY_DUMB  _RIG_IOWR(0xb4, sizeof(RigDrmDestroyDumb))

static uint8_t *g_drm_staging = NULL;
static size_t   g_drm_staging_sz = 0;

#endif

static inline uint64_t disp_now_ns__rig_dup_4b0aba2d(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}
static void disp_err__rig_variant_49ea3468(RigDisplayCtx *d, const char *msg)
{
    rl_snprintf(d->error, sizeof(d->error), "[RigDisplay] %s (errno=%d)", msg, errno);
    rl_dprintf(2, "%s\n", d->error);
    d->state = RIG_DISPLAY_ERROR;
    return 0;
}
static void sort_surfaces__rig_variant_bf7b4055(RigDisplayCtx *d)
{
    for (uint32_t i = 1; i < d->n_surfaces; i++) {
        RigDisplaySurface tmp = d->surfaces[i];
        int j = (int)i - 1;
        while (j >= 0 && d->surfaces[j].zorder > tmp.zorder) {
            d->surfaces[j+1] = d->surfaces[j];
            j--;
        }
        d->surfaces[j+1] = tmp;
    }
    return 0;
}
int rig_drm_open__rig_variant_7657ae35(RigDRMCtx *drm, const char *device)
{
#ifndef RIG_HAS_DRM
    (void)drm; (void)device;
    fprintf(stderr, "[RigDisplay] DRM/KMS no disponible en esta plataforma.\n");
    return -1;
#else
    memset(drm, 0, sizeof(*drm));
    drm->fd = -1;

    const char *dev = device ? device : "/dev/dri/card0";
    drm->fd = open(dev, O_RDWR | O_CLOEXEC);
    if (drm->fd < 0) {
        fprintf(stderr, "[RigDisplay] open(%s) falló: %s\n", dev, strerror(errno));
        return -1;
    }

    RigDrmCardRes res;
    memset(&res, 0, sizeof(res));
    if (ioctl(drm->fd, RIG_IOCTL_MODE_GETRESOURCES, &res) < 0) {
        fprintf(stderr, "[RigDisplay] GETRESOURCES falló: %s\n", strerror(errno));
        close(drm->fd); drm->fd = -1;
        return -1;
    }

    if (res.count_connectors == 0 || res.count_crtcs == 0) {
        fprintf(stderr, "[RigDisplay] Sin conectores/CRTCs disponibles.\n");
        close(drm->fd); drm->fd = -1;
        return -1;
    }

    uint32_t *conn_ids = calloc(res.count_connectors, sizeof(uint32_t));
    uint32_t *crtc_ids = calloc(res.count_crtcs,      sizeof(uint32_t));
    if (!conn_ids || !crtc_ids) { free(conn_ids); free(crtc_ids); return -1; }

    res.connector_id_ptr = (uint64_t)(uintptr_t)conn_ids;
    res.crtc_id_ptr      = (uint64_t)(uintptr_t)crtc_ids;
    if (ioctl(drm->fd, RIG_IOCTL_MODE_GETRESOURCES, &res) < 0) {
        free(conn_ids); free(crtc_ids);
        close(drm->fd); drm->fd = -1;
        return -1;
    }

    bool found = false;
    for (uint32_t ci = 0; ci < res.count_connectors && !found; ci++) {
        RigDrmGetConnector gc;
        memset(&gc, 0, sizeof(gc));
        gc.connector_id = conn_ids[ci];
        if (ioctl(drm->fd, RIG_IOCTL_MODE_GETCONNECTOR, &gc) < 0) continue;
        if (gc.connection != RIG_DRM_MODE_CONNECTED) continue;
        if (gc.count_modes == 0) continue;

        drm->connector_id = gc.connector_id;

        if (gc.encoder_id) {
            RigDrmGetEncoder ge;
            memset(&ge, 0, sizeof(ge));
            ge.encoder_id = gc.encoder_id;
            if (ioctl(drm->fd, RIG_IOCTL_MODE_GETENCODER, &ge) == 0 && ge.crtc_id) {
                drm->encoder_id = ge.encoder_id;
                drm->crtc_id    = ge.crtc_id;
                found = true;
            }
        }

        if (!found && res.count_crtcs > 0) {
            drm->crtc_id = crtc_ids[0];
            found = true;
        }
    }

    free(conn_ids);
    free(crtc_ids);

    if (!found) {
        fprintf(stderr, "[RigDisplay] Sin conector activo con CRTC.\n");
        close(drm->fd); drm->fd = -1;
        return -1;
    }

    return 0;
#endif
}

int rig_drm_alloc_buffers__rig_variant_07177637(RigDRMCtx *drm, uint32_t w, uint32_t h)
{
#ifndef RIG_HAS_DRM
    (void)drm; (void)w; (void)h; return -1;
#else
    for (int i = 0; i < 2; i++) {

        RigDrmCreateDumb cd;
        memset(&cd, 0, sizeof(cd));
        cd.width  = w;
        cd.height = h;
        cd.bpp    = 32;

        if (ioctl(drm->fd, RIG_IOCTL_MODE_CREATE_DUMB, &cd) < 0) {
            fprintf(stderr, "[RigDisplay] CREATE_DUMB[%d] falló: %s\n", i, strerror(errno));
            return -1;
        }

        drm->bufs[i].handle = cd.handle;
        drm->bufs[i].pitch  = cd.pitch;
        drm->bufs[i].size   = cd.size;

        RigDrmModeFbCmd fb;
        memset(&fb, 0, sizeof(fb));
        fb.width  = w;
        fb.height = h;
        fb.pitch  = cd.pitch;
        fb.bpp    = 32;
        fb.depth  = 24;
        fb.handle = cd.handle;

        if (ioctl(drm->fd, RIG_IOCTL_MODE_ADDFB, &fb) < 0) {
            fprintf(stderr, "[RigDisplay] ADDFB[%d] falló: %s\n", i, strerror(errno));
            return -1;
        }
        drm->bufs[i].fb_id = fb.fb_id;

        RigDrmMapDumb md;
        memset(&md, 0, sizeof(md));
        md.handle = cd.handle;
        if (ioctl(drm->fd, RIG_IOCTL_MODE_MAP_DUMB, &md) < 0) {
            fprintf(stderr, "[RigDisplay] MAP_DUMB[%d] falló: %s\n", i, strerror(errno));
            return -1;
        }

        drm->bufs[i].map = mmap(NULL, cd.size,
                                 PROT_READ | PROT_WRITE, MAP_SHARED,
                                 drm->fd, (off_t)md.offset);
        if (drm->bufs[i].map == MAP_FAILED) {
            fprintf(stderr, "[RigDisplay] mmap[%d] falló: %s\n", i, strerror(errno));
            return -1;
        }

        memset(drm->bufs[i].map, 0, cd.size);
    }

    drm->cur_buf = 0;

    g_drm_staging_sz = (size_t)w * h * 4;
    g_drm_staging = (uint8_t*)realloc(g_drm_staging, g_drm_staging_sz);
    if (!g_drm_staging) return -1;

    return 0;
#endif
}

int rig_drm_page_flip__rig_variant_4e9fd61b(RigDRMCtx *drm)
{
#ifndef RIG_HAS_DRM
    (void)drm; return -1;
#else

    uint32_t conn = drm->connector_id;

    RigDrmModeCrtc crtc;
    memset(&crtc, 0, sizeof(crtc));
    crtc.crtc_id             = drm->crtc_id;
    crtc.fb_id               = drm->bufs[drm->cur_buf].fb_id;
    crtc.set_connectors_ptr  = (uint64_t)(uintptr_t)&conn;
    crtc.count_connectors    = 1;
    crtc.mode_valid          = 0;

    if (ioctl(drm->fd, RIG_IOCTL_MODE_SETCRTC, &crtc) < 0) {
        fprintf(stderr, "[RigDisplay] SETCRTC flip falló: %s\n", strerror(errno));
        return -1;
    }

    drm->cur_buf ^= 1;
    return 0;
#endif
}

void rig_drm_close__rig_variant_242f6a72(RigDRMCtx *drm)
{
#ifndef RIG_HAS_DRM
    (void)drm;
#else
    if (drm->fd < 0) return 0;
    for (int i = 0; i < 2; i++) {
        if (drm->bufs[i].map && drm->bufs[i].map != MAP_FAILED) {
            munmap(drm->bufs[i].map, drm->bufs[i].size);
            drm->bufs[i].map = NULL;
        }
        if (drm->bufs[i].fb_id) {
            unsigned int fbid = drm->bufs[i].fb_id;
            ioctl(drm->fd, RIG_IOCTL_MODE_RMFB, &fbid);
        }
        if (drm->bufs[i].handle) {
            RigDrmDestroyDumb dd = { .handle = drm->bufs[i].handle };
            ioctl(drm->fd, RIG_IOCTL_MODE_DESTROY_DUMB, &dd);
        }
    }
    close(drm->fd);
    drm->fd = -1;
    rl_free(g_drm_staging);
    g_drm_staging    = NULL;
    g_drm_staging_sz = 0;
#endif
    return 0;
}

RigDisplayBackend rig_display_detect_backend__rig_dup_0545c040(void)
{
#ifdef RIG_HAS_DRM

    struct stat st;
    if (stat("/dev/dri/card0", &st) == 0)
        return RIG_DISPLAY_BACKEND_DRM_KMS;
#endif
#ifdef RIG_HAS_ANATIVE
    return RIG_DISPLAY_BACKEND_ANATIVE;
#endif
    return RIG_DISPLAY_BACKEND_OFFSCREEN;  /* framebuffer en RAM — soberano */
}

int rig_display_init__rig_variant_bcbfaf77(RigDisplayCtx *disp, RigGPUCtx *gpu, RigDisplayBackend backend)
{
    memset(disp, 0, sizeof(*disp));
    disp->state          = RIG_DISPLAY_UNINITIALIZED;
    disp->vsync_enabled  = true;
    disp->hdr_max_luminance = 100.0f;
    disp->hdr_min_luminance = 0.05f;

    if (backend == RIG_DISPLAY_BACKEND_AUTO)
        backend = rig_display_detect_backend__rig_dup_0545c040();
    disp->backend = backend;

    if (gpu) {
        disp->gpu = gpu;
    } else {

        disp->gpu = (RigGPUCtx*)calloc(1, sizeof(RigGPUCtx));
        if (!disp->gpu) { disp_err__rig_variant_49ea3468(disp, "OOM al crear RigGPUCtx"); return -1; }
        bool offscreen = (backend == RIG_DISPLAY_BACKEND_OFFSCREEN ||
                          backend == RIG_DISPLAY_BACKEND_STUB);
        if (rig_gpu_init(disp->gpu, NULL, NULL, offscreen) != 0) {
            disp_err__rig_variant_49ea3468(disp, "rig_gpu_init falló"); return -1;
        }
    }

    switch (backend) {

    case RIG_DISPLAY_BACKEND_DRM_KMS: {
#ifndef RIG_HAS_DRM
        disp_err__rig_variant_49ea3468(disp, "DRM no disponible; compilar en Linux"); return -1;
#else
        if (rig_drm_open__rig_variant_7657ae35(&disp->drm, NULL) != 0) {
            disp_err__rig_variant_49ea3468(disp, "rig_drm_open falló"); return -1;
        }

        rig_display_enum_modes__rig_variant_43341523(disp);
        if (disp->n_modes > 0) {
            RigDisplayMode *m = &disp->modes[0];
            rig_drm_alloc_buffers__rig_variant_07177637(&disp->drm, m->width, m->height);
            disp->mode = *m;
        }
        break;
#endif
    }

    case RIG_DISPLAY_BACKEND_ANATIVE: {
#ifndef RIG_HAS_ANATIVE
        disp_err__rig_variant_49ea3468(disp, "ANativeWindow no disponible; requiere Android NDK"); return -1;
#else

        if (!disp->anative_window) {
            disp_err__rig_variant_49ea3468(disp, "anative_window es NULL"); return -1;
        }
        break;
#endif
    }

    case RIG_DISPLAY_BACKEND_OFFSCREEN:
    case RIG_DISPLAY_BACKEND_STUB:

        disp->mode.width      = 390;
        disp->mode.height     = 844;
        disp->mode.refresh_hz = 60;
        disp->mode.bit_depth  = 8;
        break;

    default:
        disp_err__rig_variant_49ea3468(disp, "Backend desconocido"); return -1;
    }

    disp->frame_interval_ns = (uint64_t)(1e9 / (double)(disp->mode.refresh_hz ? disp->mode.refresh_hz : 60));
    disp->last_vsync_ns     = disp_now_ns__rig_dup_4b0aba2d();
    disp->state             = RIG_DISPLAY_READY;
    return 0;
}

void rig_display_destroy__rig_variant_7b47f0ca(RigDisplayCtx *disp)
{
    if (disp->state == RIG_DISPLAY_UNINITIALIZED) return 0;
    if (disp->backend == RIG_DISPLAY_BACKEND_DRM_KMS)
        rig_drm_close(&disp->drm);

    if (disp->gpu) {
        rig_gpu_destroy(disp->gpu);

    }

    disp->state = RIG_DISPLAY_UNINITIALIZED;
    disp->n_surfaces = 0;
    return 0;
}

int rig_display_enum_modes__rig_variant_43341523(RigDisplayCtx *disp)
{
#ifndef RIG_HAS_DRM
    (void)disp;
    return 0;
#else
    if (disp->backend != RIG_DISPLAY_BACKEND_DRM_KMS) return 0;
    if (disp->drm.fd < 0) return -1;

    RigDrmCardRes res;
    memset(&res, 0, sizeof(res));
    if (ioctl(disp->drm.fd, RIG_IOCTL_MODE_GETRESOURCES, &res) < 0) return -1;
    if (!res.count_connectors) return 0;

    uint32_t *cids = calloc(res.count_connectors, sizeof(uint32_t));
    if (!cids) return -1;
    res.connector_id_ptr = (uint64_t)(uintptr_t)cids;
    ioctl(disp->drm.fd, RIG_IOCTL_MODE_GETRESOURCES, &res);

    disp->n_modes = 0;

    for (uint32_t ci = 0; ci < res.count_connectors; ci++) {
        RigDrmGetConnector gc;
        memset(&gc, 0, sizeof(gc));
        gc.connector_id = cids[ci];
        ioctl(disp->drm.fd, RIG_IOCTL_MODE_GETCONNECTOR, &gc);

        if (gc.connection != RIG_DRM_MODE_CONNECTED || !gc.count_modes) continue;

        RigDrmModeInfo *modes = calloc(gc.count_modes, sizeof(RigDrmModeInfo));
        if (!modes) continue;
        gc.modes_ptr = (uint64_t)(uintptr_t)modes;
        ioctl(disp->drm.fd, RIG_IOCTL_MODE_GETCONNECTOR, &gc);

        for (uint32_t mi = 0; mi < gc.count_modes && disp->n_modes < 16; mi++) {
            RigDrmModeInfo *mo = &modes[mi];
            RigDisplayMode *dm = &disp->modes[disp->n_modes++];
            dm->width      = mo->hdisplay;
            dm->height     = mo->vdisplay;
            dm->refresh_hz = mo->vrefresh;
            dm->bit_depth  = 8;
            dm->hdr_capable = false;

            if ((mo->type & RIG_DRM_MODE_TYPE_PREFERRED) && disp->n_modes > 1) {
                RigDisplayMode tmp = *dm;
                *dm = disp->modes[0];
                disp->modes[0] = tmp;
                disp->n_modes--;
                disp->n_modes++;
            }
        }
        free(modes);
        break;
    }

    free(cids);
    return (int)disp->n_modes;
#endif
}

int rig_display_set_mode__rig_dup_92918bb3(RigDisplayCtx *disp, uint32_t w, uint32_t h, uint32_t hz)
{

    for (uint8_t i = 0; i < disp->n_modes; i++) {
        RigDisplayMode *m = &disp->modes[i];
        if (m->width == w && m->height == h && m->refresh_hz == hz) {
            disp->mode = *m;
            disp->frame_interval_ns = (uint64_t)(1e9 / (double)hz);
            return 0;
        }
    }

    disp->mode.width      = w;
    disp->mode.height     = h;
    disp->mode.refresh_hz = hz ? hz : 60;
    disp->frame_interval_ns = (uint64_t)(1e9 / (double)disp->mode.refresh_hz);
    return 0;
}

int rig_display_enable_hdr__rig_dup_0928162d(RigDisplayCtx *disp, float max_nits)
{
    if (max_nits < 100.0f) return -1;
    disp->hdr_enabled       = true;
    disp->hdr_max_luminance = max_nits;
    disp->mode.bit_depth    = 10;
    disp->mode.hdr_capable  = true;
    return 0;
}

uint32_t rig_display_create_surface__rig_variant_6496cec0(RigDisplayCtx *disp, const char *name,
                                     uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    if (disp->n_surfaces >= 32) return (uint32_t)-1;

    RigDisplaySurface *s = &disp->surfaces[disp->n_surfaces];
    memset(s, 0, sizeof(*s));
    s->id      = disp->n_surfaces + 1;
    s->x = x; s->y = y; s->w = w; s->h = h;
    s->zorder  = disp->n_surfaces;
    s->visible = true;
    if (name) strncpy(s->name, name, 63);
    disp->n_surfaces++;

    sort_surfaces__rig_variant_bf7b4055(disp);
    return s->id;
}

void rig_display_set_render_cb__rig_variant_eee5a411(RigDisplayCtx *disp, uint32_t surf_id,
                                void (*cb)(RigDisplaySurface*, void*), void *userdata)
{
    for (uint32_t i = 0; i < disp->n_surfaces; i++) {
        if (disp->surfaces[i].id == surf_id) {
            disp->surfaces[i].on_render = cb;
            disp->surfaces[i].userdata  = userdata;
            return 0;
        }
    }
    return 0;
}

void rig_display_destroy_surface__rig_variant_89c8d5be(RigDisplayCtx *disp, uint32_t surf_id)
{
    for (uint32_t i = 0; i < disp->n_surfaces; i++) {
        if (disp->surfaces[i].id == surf_id) {

            for (uint32_t j = i; j < disp->n_surfaces - 1; j++)
                disp->surfaces[j] = disp->surfaces[j+1];
            disp->n_surfaces--;
            return 0;
        }
    }
    return 0;
}

int rig_display_submit_frame__rig_dup_95fff949(RigDisplayCtx *disp, const RigComFrame *frame)
{
    if (!frame || frame->magic != RIG_FRAME_MAGIC) return -1;

    uint8_t next = (uint8_t)((disp->frame_tail + 1) % 8);
    if (next == disp->frame_head) return -1;

    disp->frame_queue[disp->frame_tail] = *frame;
    disp->frame_tail = next;
    return 0;
}

int rig_display_present__rig_dup_059a895a(RigDisplayCtx *disp)
{
    if (disp->state == RIG_DISPLAY_UNINITIALIZED) return -1;
    disp->state = RIG_DISPLAY_RENDERING;

    for (uint32_t i = 0; i < disp->n_surfaces; i++) {
        RigDisplaySurface *s = &disp->surfaces[i];
        if (!s->visible || !s->on_render) continue;
        s->on_render(s, s->userdata);
    }

    switch (disp->backend) {

    case RIG_DISPLAY_BACKEND_DRM_KMS: {
#ifdef RIG_HAS_DRM
        if (disp->drm.fd >= 0 && disp->drm.bufs[0].map && g_drm_staging) {
            uint32_t w = disp->mode.width;
            uint32_t h = disp->mode.height;

#ifndef RIG_GPU_STUB

            extern void glReadPixels(int,int,int,int,unsigned int,unsigned int,void*);
            glReadPixels(0, 0, (int)w, (int)h, 0x1908, 0x1401, g_drm_staging);
#endif

            uint32_t *dst    = (uint32_t*)disp->drm.bufs[disp->drm.cur_buf].map;
            const uint8_t *src = g_drm_staging;
            uint32_t pitch32 = disp->drm.bufs[disp->drm.cur_buf].pitch / 4;

            for (uint32_t y = 0; y < h; y++) {

                const uint8_t *row = src + (size_t)(h - 1 - y) * w * 4;
                for (uint32_t x = 0; x < w; x++) {
                    uint8_t r = row[x*4+0];
                    uint8_t g = row[x*4+1];
                    uint8_t b = row[x*4+2];
                    dst[y * pitch32 + x] = ((uint32_t)r << 16) |
                                            ((uint32_t)g <<  8) |
                                            (uint32_t)b;
                }
            }
            rig_drm_page_flip__rig_variant_4e9fd61b(&disp->drm);
        }
#endif
        break;
    }

    case RIG_DISPLAY_BACKEND_ANATIVE:

        rig_gpu_present(disp->gpu);
        break;

    case RIG_DISPLAY_BACKEND_OFFSCREEN:
    case RIG_DISPLAY_BACKEND_STUB:

        break;

    default:
        break;
    }

    disp->frames_presented++;
    rig_display_update_fps__rig_variant_bc09c3eb(disp);
    disp->state = RIG_DISPLAY_READY;
    return 0;
}

void rig_display_wait_vsync__rig_variant_d2be28e8(RigDisplayCtx *disp)
{
    uint64_t now   = disp_now_ns();
    uint64_t next  = disp->last_vsync_ns + disp->frame_interval_ns;

    if (now < next) {
        uint64_t sleep_ns = next - now;
        struct timespec ts = {
            .tv_sec  = (time_t)(sleep_ns / 1000000000ULL),
            .tv_nsec = (long)  (sleep_ns % 1000000000ULL)
        };
        nanosleep(&ts, NULL);
    }
    disp->last_vsync_ns = disp_now_ns();
    return 0;
}

const char* rig_display_backend_name__rig_dup_f0bbf2db(RigDisplayBackend b)
{
    switch (b) {
    case RIG_DISPLAY_BACKEND_DRM_KMS:   return "DRM/KMS";
    case RIG_DISPLAY_BACKEND_ANATIVE:   return "ANativeWindow";
    case RIG_DISPLAY_BACKEND_OFFSCREEN: return "EGL-Offscreen";
    case RIG_DISPLAY_BACKEND_STUB:      return "Stub";
    default:                            return "Auto";
    }
}

void rig_display_emit_status__rig_variant_239687f8(const RigDisplayCtx *disp, char *out, size_t outsz)
{
    rl_snprintf(out, outsz,
        "{"
          "\"backend\":\"%s\","
          "\"width\":%u,"
          "\"height\":%u,"
          "\"refresh_hz\":%u,"
          "\"bit_depth\":%u,"
          "\"hdr\":%s,"
          "\"fps\":%.2f,"
          "\"frames\":%llu,"
          "\"surfaces\":%u,"
          "\"state\":%d"
        "}",
        rig_display_backend_name(disp->backend),
        disp->mode.width,
        disp->mode.height,
        disp->mode.refresh_hz,
        disp->mode.bit_depth,
        disp->hdr_enabled ? "true" : "false",
        disp->fps_measured,
        (unsigned long long)disp->frames_presented,
        disp->n_surfaces,
        (int)disp->state
    );
    return 0;
}

void rig_display_update_fps__rig_variant_bc09c3eb(RigDisplayCtx *disp)
{

    static uint64_t last_ts = 0;
    uint64_t now = disp_now_ns();
    if (last_ts == 0) { last_ts = now; return; }

    double dt_s = (double)(now - last_ts) * 1e-9;
    if (dt_s > 0.0) {
        double instant_fps = 1.0 / dt_s;

        disp->fps_measured = disp->fps_measured * 0.9 + instant_fps * 0.1;
    }
    last_ts = now;
    return 0;
}
