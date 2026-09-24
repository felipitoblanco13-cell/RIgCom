#pragma once

/* GEN */
#include "rigdeps/rig_noext_types.h"
#include "rigdeps/rig_math.h"


int rig_ui_json_get_str__rig_variant_43da907c(const char *json, const char *key, char *out, rig_usize outsz);
int rig_ui_response_append__rig_dup_064b1915(RigUIResponse *res, const char *s);
int rig_ui_response_json__rig_variant_9ff8032a(RigUIResponse *res, int status, const char *type, const char *message);
int rig_ui_router_dispatch__rig_dup_1223e7ae(RigUIRouter *r, const char *cmd, const char *payload, rig_usize payload_len, RigUIResponse *res);
int rig_ui_router_dispatch_json__rig_dup_f7c1c5b0(RigUIRouter *r, const char *json, RigUIResponse *res);
int rig_ui_router_manifest_json__rig_dup_57deab8b(RigUIRouter *r, RigUIResponse *res);
int rig_ui_router_register__rig_dup_c8bb327c(RigUIRouter *r, const char *name, const char *module, const char *desc, rig_u32 flags, RigUIHandler handler, void *user);
int rig_ui_router_set_creator_token__rig_dup_fd2a6c9b(RigUIRouter *r, const char *token);
void rig_ui_router_init__rig_dup_01c1d395(RigUIRouter *r);
