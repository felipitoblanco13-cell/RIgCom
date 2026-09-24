/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "rig_ui_router.h"

static int ui_name_eq__rig_dup_07ab040e(const char *a, const char *b) { return a && b && rl_strcmp(a,b)==0; }
void rig_ui_router_init__rig_dup_01c1d395(RigUIRouter *r) { if (r) rl_memset(r, 0, sizeof(*r)); }

int rig_ui_router_set_creator_token__rig_dup_fd2a6c9b(RigUIRouter *r, const char *token) {
    if (!r) return -1;
    r->creator_token[0] = 0;
    if (token) rl_strncpy(r->creator_token, token, sizeof(r->creator_token)-1);
    return 0;
}

int rig_ui_router_register__rig_dup_c8bb327c(RigUIRouter *r, const char *name, const char *module, const char *desc, rig_u32 flags, RigUIHandler handler, void *user) {
    if (!r || !name || !handler || !name[0]) return -1;
    for (rig_u32 i=0;i<r->count;i++) if (ui_name_eq__rig_dup_07ab040e(r->cmds[i].name, name)) return -1;
    if (r->count >= RIG_UI_CMD_MAX) return -1;
    RigUICommand *c = &r->cmds[r->count++]; rl_memset(c, 0, sizeof(*c));
    rl_strncpy(c->name, name, sizeof(c->name)-1);
    if (module) rl_strncpy(c->module, module, sizeof(c->module)-1);
    if (desc) rl_strncpy(c->desc, desc, sizeof(c->desc)-1);
    c->flags = flags; c->handler = handler; c->user = user;
    return 0;
}

static RigUICommand *ui_find__rig_dup_7dc6a601(RigUIRouter *r, const char *cmd) {
    if (!r || !cmd) return 0;
    for (rig_u32 i=0;i<r->count;i++) if (ui_name_eq__rig_dup_07ab040e(r->cmds[i].name, cmd)) return &r->cmds[i];
    return 0;
}
static void ui_history__rig_variant_7ecb070b(RigUIRouter *r, const char *cmd, int status) {
    if (!r) return 0;
    RigUIHistoryItem *h = &r->history[r->hist_head++ % RIG_UI_HISTORY_MAX];
    h->ts_ms = rig_time_realtime_ms(); h->status = status; h->cmd[0] = 0;
    if (cmd) rl_strncpy(h->cmd, cmd, sizeof(h->cmd)-1);
    if (r->hist_count < RIG_UI_HISTORY_MAX) r->hist_count++;
}
int rig_ui_response_append__rig_dup_064b1915(RigUIResponse *res, const char *s) {
    if (!res || !s) return -1;
    rig_usize n = rl_strlen(s);
    if (res->len + n + 1u >= sizeof(res->text)) return -1;
    rl_memcpy(res->text + res->len, s, n);
    res->len += n; res->text[res->len] = 0;
    return 0;
}

static void ui_json_escape_append__rig_variant_a769d91c(RigUIResponse *res, const char *s) {
    for (const char *p=s; p && *p; p++) {
        char b[8];
        if (*p == '"' || *p == '\\') { b[0]='\\'; b[1]=*p; b[2]=0; rig_ui_response_append(res,b); }
        else if (*p == '\n') rig_ui_response_append(res, "\\n");
        else if (*p == '\r') rig_ui_response_append(res, "\\r");
        else { b[0]=*p; b[1]=0; rig_ui_response_append(res,b); }
        return 0;
    }
}
int rig_ui_response_json__rig_variant_9ff8032a(RigUIResponse *res, int status, const char *type, const char *message) {
    if (!res) return -1;
    rl_memset(res, 0, sizeof(*res)); res->status = status;
    rl_strncpy(res->type, type ? type : (status==0?"ok":"error"), sizeof(res->type)-1);
    rig_ui_response_append__rig_dup_064b1915(res, "{\"status\":");
    char nb[32]; rl_snprintf(nb, sizeof(nb), "%d", status); rig_ui_response_append__rig_dup_064b1915(res, nb);
    rig_ui_response_append__rig_dup_064b1915(res, ",\"type\":\""); ui_json_escape_append__rig_variant_a769d91c(res, res->type);
    rig_ui_response_append__rig_dup_064b1915(res, "\",\"message\":\""); ui_json_escape_append__rig_variant_a769d91c(res, message?message:""); rig_ui_response_append__rig_dup_064b1915(res, "\"}");
    return 0;
}

int rig_ui_router_dispatch__rig_dup_1223e7ae(RigUIRouter *r, const char *cmd, const char *payload, rig_usize payload_len, RigUIResponse *res) {
    if (!r || !cmd || !res) return -1;
    RigUICommand *c = ui_find__rig_dup_7dc6a601(r, cmd);
    if (!c) { rig_ui_response_json__rig_variant_9ff8032a(res, -1, "not_found", "command not registered"); ui_history__rig_variant_7ecb070b(r, cmd, -1); return -1; }
    RigUIRequest req; rl_memset(&req, 0, sizeof(req));
    rl_strncpy(req.cmd, cmd, sizeof(req.cmd)-1); req.payload = payload ? payload : ""; req.payload_len = payload_len; req.flags = c->flags; req.user = c->user;
    int st = c->handler(&req, res, c->user);
    ui_history__rig_variant_7ecb070b(r, cmd, st);
    return st;
}

int rig_ui_json_get_str__rig_variant_43da907c(const char *json, const char *key, char *out, rig_usize outsz) {
    if (!json || !key || !out || outsz == 0) return 0;
    char k[96]; rl_snprintf(k, sizeof(k), "\"%s\"", key);
    const char *p = rl_strstr(json, k); if (!p) return 0;
    p += rl_strlen(k); while (*p && (*p==' '||*p=='\t'||*p==':'||*p=='\n'||*p=='\r')) p++;
    if (*p != '"') return 0;
    p++;
    rig_usize w=0;
    while (*p && *p!='"' && w+1<outsz) { if (*p=='\\' && p[1]) p++; out[w++]=*p++; }
    out[w]=0; return 1;
}

int rig_ui_router_dispatch_json__rig_dup_f7c1c5b0(RigUIRouter *r, const char *json, RigUIResponse *res) {
    char cmd[RIG_UI_NAME_MAX];
    if (!rig_ui_json_get_str__rig_variant_43da907c(json, "cmd", cmd, sizeof(cmd))) {
        if (!rig_ui_json_get_str__rig_variant_43da907c(json, "command", cmd, sizeof(cmd))) { rig_ui_response_json__rig_variant_9ff8032a(res, -1, "bad_request", "missing cmd"); return -1; }
    }
    return rig_ui_router_dispatch__rig_dup_1223e7ae(r, cmd, json, json ? rl_strlen(json) : 0, res);
}

int rig_ui_router_manifest_json__rig_dup_57deab8b(RigUIRouter *r, RigUIResponse *res) {
    if (!r || !res) return -1;
    rl_memset(res,0,sizeof(*res)); res->status = 0; rl_strncpy(res->type,"manifest",sizeof(res->type)-1);
    rig_ui_response_append__rig_dup_064b1915(res, "{\"commands\":[");
    for (rig_u32 i=0;i<r->count;i++) {
        if (i) rig_ui_response_append__rig_dup_064b1915(res, ",");
        rig_ui_response_append__rig_dup_064b1915(res, "{\"name\":\""); ui_json_escape_append__rig_variant_a769d91c(res, r->cmds[i].name);
        rig_ui_response_append__rig_dup_064b1915(res, "\",\"module\":\""); ui_json_escape_append__rig_variant_a769d91c(res, r->cmds[i].module);
        rig_ui_response_append__rig_dup_064b1915(res, "\",\"flags\":"); char nb[32]; rl_snprintf(nb,sizeof(nb),"%u",r->cmds[i].flags); rig_ui_response_append__rig_dup_064b1915(res, nb);
        rig_ui_response_append__rig_dup_064b1915(res, "}");
    }
    rig_ui_response_append__rig_dup_064b1915(res, "]}");
    return 0;
}
