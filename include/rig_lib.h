#pragma once
/* rig_lib.h — capa base rl_* del ecosistema Rig (reconstruida)
 *
 * Aliases de libc con prefijo rl_ y utilidades. Las variantes con sufijo
 * __rig_dup_XXXXXXXX / __rig_variant_XXXXXXXX que aparecen en los .c son
 * nombres únicos anti-colisión de las mismas funciones (estilo del
 * proyecto: sin static, renombrado para evitar choque de símbolos).
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

typedef uint8_t  rl_u8;
typedef uint16_t rl_u16;
typedef uint32_t rl_u32;
typedef uint64_t rl_u64;
typedef int16_t  rl_i16;
typedef int32_t  rl_i32;
typedef size_t   rl_size;
typedef bool     rl_bool;

/* ── mem ── */
#define rl_malloc  malloc
#define rl_calloc  calloc
#define rl_realloc realloc
#define rl_free    free
#define rl_memcpy  memcpy
#define rl_memset  memset

/* ── string ── */
#define rl_snprintf snprintf
#define rl_strcmp   strcmp
#define rl_strncmp  strncmp
#define rl_strncpy  strncpy
#define rl_strncat  strncat
#define rl_strlen   strlen
#define rl_strchr   strchr
#define rl_strrchr  strrchr
#define rl_strstr   strstr
#define rl_atoi     atoi
#define rl_strtod   strtod
#define rl_strtoul  strtoul
#define rl_dprintf  (void)

/* ── math ── */
#define rl_sinf    sinf
#define rl_cosf    cosf
#define rl_tanf    tanf
#define rl_sqrtf   sqrtf
#define rl_fabsf   fabsf
#define rl_floorf  floorf
#define rl_powf    powf
#define rl_expf   expf
#define rl_exp2f  exp2f
#define rl_logf   logf
#define rl_log10f log10f
#define rl_fmodf  fmodf
#define rl_acosf  acosf
#define rl_asinf  asinf
#define rl_atan2f atan2f

static inline rl_u64 rl_now_ns__base(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (rl_u64)ts.tv_sec * 1000000000ull + (rl_u64)ts.tv_nsec;
}
static inline rl_u64 rl_now_ms__base(void) { return rl_now_ns__base() / 1000000ull; }
#define rl_now_ns rl_now_ns__base
#define rl_now_ms rl_now_ms__base

#include "riglib_math.h"

/* ── vec/mat sobre Mat4/Vec3f de riglib_math ── */
static inline void rl_mat4_identity(Mat4 *m) {
    memset(m, 0, sizeof(*m));
    m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.0f;
}
static inline void rl_mat4_mul(Mat4 *o, const Mat4 *a, const Mat4 *b) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += a->m[k * 4 + j] * b->m[i * 4 + k];
            o->m[i * 4 + j] = s;
        }
}
static inline Vec3f rl_mat4_mul_v3(const Mat4 *m, Vec3f v) {
    return (Vec3f){
        m->m[0]*v.x + m->m[4]*v.y + m->m[8]*v.z  + m->m[12],
        m->m[1]*v.x + m->m[5]*v.y + m->m[9]*v.z  + m->m[13],
        m->m[2]*v.x + m->m[6]*v.y + m->m[10]*v.z + m->m[14]
    };
}
static inline void rl_mat4_perspective(Mat4 *m, float fov_deg, float aspect, float zn, float zf) {
    float f = 1.0f / tanf(fov_deg * 3.14159265f / 360.0f);
    memset(m, 0, sizeof(*m));
    m->m[0] = f / aspect; m->m[5] = f;
    m->m[10] = (zf + zn) / (zn - zf); m->m[11] = -1.0f;
    m->m[14] = 2.0f * zf * zn / (zn - zf);
}
static inline void rl_mat4_lookat(Mat4 *m, Vec3f eye, Vec3f at, Vec3f up) {
    Vec3f z = vec3_normalize(vec3_sub(eye, at));
    Vec3f x = vec3_normalize(vec3_cross(up, z));
    Vec3f y = vec3_cross(z, x);
    memset(m, 0, sizeof(*m));
    m->m[0] = x.x; m->m[4] = x.y; m->m[8]  = x.z;
    m->m[1] = y.x; m->m[5] = y.y; m->m[9]  = y.z;
    m->m[2] = z.x; m->m[6] = z.y; m->m[10] = z.z;
    m->m[12] = -vec3_dot(x, eye); m->m[13] = -vec3_dot(y, eye);
    m->m[14] = -vec3_dot(z, eye); m->m[15] = 1.0f;
}
static inline void rl_mat4_translate(Mat4 *m, float x, float y, float z) {
    rl_mat4_identity(m);
    m->m[12] = x; m->m[13] = y; m->m[14] = z;
}

/* ── v3 helpers ── */
#define rl_v3add   vec3_add
#define rl_v3sub   vec3_sub
#define rl_v3dot   vec3_dot
#define rl_v3cross vec3_cross
#define rl_v3len   vec3_len
#define rl_v3norm  vec3_normalize
