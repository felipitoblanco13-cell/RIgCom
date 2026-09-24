#pragma once
/* riglib_math.h — matemáticas del ecosistema Rig (reconstruido) */
#include <math.h>

typedef struct { float x, y, z; } Vec3f;
typedef struct { float x, y; } Vec2f;
typedef struct { float x, y, z, w; } Vec4f;
typedef struct { float m[16]; } Mat4;
typedef struct { float x, y, z, w; } Quat;

static inline Vec3f vec3(float x, float y, float z) { Vec3f r = { x, y, z }; return r; }
static inline Vec3f vec3_add(Vec3f a, Vec3f b) { return (Vec3f){ a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline Vec3f vec3_sub(Vec3f a, Vec3f b) { return (Vec3f){ a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline Vec3f vec3_scale(Vec3f a, float s) { return (Vec3f){ a.x * s, a.y * s, a.z * s }; }
static inline float vec3_dot(Vec3f a, Vec3f b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline Vec3f vec3_cross(Vec3f a, Vec3f b) {
    return (Vec3f){ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
static inline float vec3_len(Vec3f a) { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }
static inline Vec3f vec3_normalize(Vec3f a) {
    float l = vec3_len(a);
    return l > 1e-8f ? (Vec3f){ a.x / l, a.y / l, a.z / l } : (Vec3f){ 0.0f, 0.0f, 0.0f };
}
static inline float vec3_dist(Vec3f a, Vec3f b) { return vec3_len(vec3_sub(a, b)); }
static inline Vec3f vec3_lerp(Vec3f a, Vec3f b, float t) {
    return (Vec3f){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

static inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static inline float smoothstepf(float e0, float e1, float x) {
    float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

#ifndef RIG_PHI
#define RIG_PHI 1.6180339887498948482f
#endif
static inline float phi_lerp__rig_base(float a, float b, float t) { return a + (b - a) * t; }
#define phi_lerp phi_lerp__rig_base
