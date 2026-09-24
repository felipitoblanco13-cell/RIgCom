#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"


int rig_env_bake(const RigEnvDesc *e, RigEnvCube *c);
int rig_env_build(const RigEnvDesc *desc, RigEnvGPU *gpu, uint32_t cube_size, uint32_t samples);
int rig_env_cube_alloc(RigEnvCube *c, uint32_t size, uint32_t mips);
int rig_env_prefilter(const RigEnvCube *src, RigEnvCube *dst, uint32_t samples);
int rig_env_preset(RigEnvDesc *e, RigEnvPreset p);
int rig_env_project_sh(const RigEnvCube *c, float sh[9][3]);
void rig_env_brdf_approx(float NoV, float rough, float *scale, float *bias);
void rig_env_cube_dir(uint32_t face, float u, float v, float d[3]);
void rig_env_cube_free(RigEnvCube *c);
void rig_env_cube_sample(const RigEnvCube *c, uint32_t mip, const float d[3], float out[3]);
void rig_env_gpu_free(RigEnvGPU *gpu);
void rig_env_irradiance_sh(const float sh[9][3], const float n[3], float out[3]);
void rig_env_kelvin_to_rgb(float kelvin, float out[3]);
void rig_env_multiscatter(const float F0[3], float scale, float bias, float out_gain[3]);
void rig_env_radiance(const RigEnvDesc *e, const float d[3], float out[3]);
