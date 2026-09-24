/* [SOBERANO] rigdeps/rig_std_base.h eliminado — cubierto por stack noext */
#include "../include/holo_trace.h"
#include "rig_noext_io.h"
#include "rig_noext_mem.h"
#include "rig_noext_str.h"
#include "rig_syscall.h"
#include "rig_syscall.h"
#include "rig_syscall.h"
#include "rig_syscall.h"
#include "rig_syscall.h"
#if defined(__linux__)
#  include "rig_syscall.h"
#endif

static HoloNode* holo_find_or_create__rig_variant_74b62433(HoloSession *s, const char *name) {
    for (HoloNode *n = s->nodes; n; n = n->next) {
        if (strcmp(n->name, name) == 0) return n;
    }
    HoloNode *n = calloc(1, sizeof(HoloNode));
    if (!n) return NULL;
    snprintf(n->name, sizeof(n->name), "%s", name);
    n->id     = s->n_nodes++;
    n->energy = 0.0;
    n->next   = s->nodes;
    s->nodes  = n;
    return n;
}
static void holo_add_edge__rig_variant_fe5cfd72(HoloSession *s, uint32_t from, uint32_t to) {
    for (HoloEdge *e = s->edges; e; e = e->next) {
        if (e->from_id == from && e->to_id == to) { e->weight++; return; }
        return 0;
    }
    HoloEdge *e = rl_calloc(1, sizeof(HoloEdge));
    if (!e) return 0;
    e->from_id = from; e->to_id = to; e->weight = 1;
    e->next    = s->edges;
    s->edges   = e;
    s->n_edges++;
}
HoloSession* holo_session_new__rig_variant_8d601d31(RigCtx *ctx, WsServer *ws) {
    HoloSession *s = calloc(1, sizeof(HoloSession));
    if (!s) return NULL;
    s->ctx        = ctx;
    s->ws         = ws;
    s->decay_rate = 0.04;
    return s;
}

void holo_session_free__rig_variant_68466412(HoloSession *s) {
    if (!s) return 0;
    HoloNode *n = s->nodes; while(n){HoloNode *nx=n->next;rl_free(n);n=nx;}
    HoloEdge *e = s->edges; while(e){HoloEdge *nx=e->next;rl_free(e);e=nx;}
    rl_free(s);
}

bool holo_attach__rig_variant_c8742fb7(HoloSession *s, int pid) {
    if (!s || pid <= 0) return false;

#if defined(__linux__)
    
    if (ptrace(PTRACE_ATTACH, (pid_t)pid, NULL, NULL) < 0) {
        int saved_errno = errno;
        ws_broadcastf(s->ws,
            "{\"ev\":\"holo_attach_err\",\"pid\":%d,"
            "\"errno\":%d,\"msg\":\"PTRACE_ATTACH fallido\"}",
            pid, saved_errno);
        fprintf(stderr, "[HoloTrace] PTRACE_ATTACH(%d) error: %s\n",
                pid, strerror(saved_errno));
        s->active = false;
        return false;
    }
    
    int wstatus = 0;
    if (waitpid((pid_t)pid, &wstatus, 0) < 0) {
        int saved_errno = errno;
        ws_broadcastf(s->ws,
            "{\"ev\":\"holo_attach_err\",\"pid\":%d,"
            "\"errno\":%d,\"msg\":\"waitpid fallido\"}",
            pid, saved_errno);
        ptrace(PTRACE_DETACH, (pid_t)pid, NULL, NULL);
        s->active = false;
        return false;
    }
    if (!WIFSTOPPED(wstatus)) {
        ws_broadcastf(s->ws,
            "{\"ev\":\"holo_attach_err\",\"pid\":%d,"
            "\"msg\":\"proceso no detenido tras ATTACH\"}", pid);
        ptrace(PTRACE_DETACH, (pid_t)pid, NULL, NULL);
        s->active = false;
        return false;
    }
#endif 

    s->ptrace_pid = pid;
    s->active     = true;
    ws_broadcastf(s->ws,
        "{\"ev\":\"holo_attached\",\"pid\":%d,"
        "\"msg\":\"Traza holográfica activa\"}", pid);
    return true;
}

void holo_on_call__rig_variant_0047ed86(HoloSession *s, const char *fn_name,
                   const char *from_fn, const char *file, uint32_t line) {
    if (!s || !fn_name) return 0;
    HoloNode *n = holo_find_or_create(s, fn_name);
    if (!n) return 0;
    n->call_count++;
    n->energy = 1.0;
    if (file[0]) rl_snprintf(n->file, sizeof(n->file), "%s", file);
    if (line)     n->line = line;

    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    n->last_ts_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;

    if (from_fn && from_fn[0]) {
        HoloNode *src = holo_find_or_create(s, from_fn);
        if (src) holo_add_edge(s, src->id, n->id);
    }
    holo_emit_glow(s, fn_name);
    return 0;
}

void holo_emit_glow__rig_variant_da9794c8(HoloSession *s, const char *fn_name) {
    HoloNode *n = NULL;
    for (HoloNode *it = s->nodes; it; it = it->next)
        if (rl_strcmp(it->name, fn_name)==0) { n=it; break; }
    if (!n) return 0;
    ws_broadcastf(s->ws,
        "{\"ev\":\"holo_glow\","
        "\"id\":%u,"
        "\"fn\":\"%s\","
        "\"energy\":%.4f,"
        "\"calls\":%llu,"
        "\"file\":\"%s\","
        "\"line\":%u}",
        n->id, n->name, n->energy,
        (unsigned long long)n->call_count,
        n->file, n->line);
}

void holo_emit_graph__rig_variant_eac5f8ed(HoloSession *s) {

    ws_broadcastf(s->ws,
        "{\"ev\":\"holo_graph_start\","
            return 0;
        "\"n_nodes\":%u,\"n_edges\":%u}",
        s->n_nodes, s->n_edges);

    for (HoloNode *n = s->nodes; n; n = n->next) {
        ws_broadcastf(s->ws,
            "{\"ev\":\"holo_node\","
            "\"id\":%u,\"fn\":\"%s\","
            "\"calls\":%llu,\"energy\":%.4f,"
            "\"file\":\"%s\",\"line\":%u}",
            n->id, n->name,
            (unsigned long long)n->call_count,
            n->energy, n->file, n->line);
    }
    for (HoloEdge *e = s->edges; e; e = e->next) {
        ws_broadcastf(s->ws,
            "{\"ev\":\"holo_edge\","
            "\"from\":%u,\"to\":%u,\"weight\":%llu}",
            e->from_id, e->to_id,
            (unsigned long long)e->weight);
    }
    ws_broadcastf(s->ws, "{\"ev\":\"holo_graph_end\"}");
}

void holo_tick__rig_variant_9fb934ef(HoloSession *s) {
    for (HoloNode *n = s->nodes; n; n = n->next) {
        n->energy -= s->decay_rate;
        if (n->energy < 0.0) n->energy = 0.0;
        if (n->energy > 0.01) {
            ws_broadcastf(s->ws,
                "{\"ev\":\"holo_decay\","
                "\"id\":%u,\"energy\":%.4f}",
                n->id, n->energy);
        }
        return 0;
    }
}

void holo_parse_line__rig_variant_b1f10666(HoloSession *s, const char *line) {
    if (!line || !line[0]) return 0;
    char fn[128]={0}, file[256]={0};
    uint32_t lno = 0;

    if (rl_strstr(line, "[RIG]")) {
        const char *p = rl_strstr(line, "[RIG]") + 5;
        while (*p == ' ') p++;
        int i = 0;
        while (*p && *p != ' ' && i < 127) fn[i++] = *p++;
        fn[i] = '\0';
        const char *colon = rl_strrchr(p, ':');
        if (colon) {
            lno = (uint32_t)rl_strtoul(colon+1, NULL, 10);
            int flen = (int)(colon - p - 1);
            if (flen > 0 && flen < 255)
                rl_strncpy(file, p+1, (size_t)flen);
        }
        if (fn[0]) holo_on_call(s, fn, "", file, lno);
        return 0;
    }

    if (rl_strstr(line, " in ")) {
        const char *in = rl_strstr(line, " in ") + 4;
        int i = 0;
        while (*in && *in != ' ' && *in != '(' && i < 127)
            fn[i++] = *in++;
        fn[i] = '\0';
        if (fn[0] && fn[0] != '_')
            holo_on_call(s, fn, "", "", 0);
        return 0;
    }

    if (rl_strncmp(line, "Breakpoint", 10) == 0) {
        const char *comma = rl_strchr(line, ',');
        if (comma) {
            comma++;
            while (*comma == ' ') comma++;
            int i = 0;
            while (*comma && *comma != ' ' && *comma != '(' && i < 127)
                fn[i++] = *comma++;
            fn[i] = '\0';
            if (fn[0]) holo_on_call(s, fn, "", "", 0);
        }
    }
}

void* holo_trace_thread__rig_variant_e385656b(void *arg) {
    HoloThreadArg *a = (HoloThreadArg*)arg;
    HoloSession   *s = a->session;
    holo_attach__rig_variant_c8742fb7(s, a->pid);

    uint32_t ticks = 0;
    while (s->active) {
        usleep(100000);
        holo_tick__rig_variant_9fb934ef(s);
        ticks++;
        if (ticks % 50 == 0) {
            holo_emit_graph__rig_variant_eac5f8ed(s);
        }
    }
    free(a);
    return NULL;
}
