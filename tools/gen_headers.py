#!/usr/bin/env python3
"""Genera headers de declaración para los módulos de src/ de RIgCom.

Los .c originales dependían de headers de proyecto que no venían en ningún
zip (rig_face_sovereign.h, rig_face_ng.h, etc.). Este script reconstruye
esa capa extrayendo las firmas públicas (funciones y tipos) de cada .c,
en el orden de dependencia, y escribiendo include/*.h.

Los headers regenerados marcan cada declaración con /* GEN */ para
distinguirlos de la capa original si algún día se recupera.
"""
import re, os, sys, glob

SRC = os.path.join(os.path.dirname(__file__), "..", "src")
OUT = os.path.join(os.path.dirname(__file__), "..", "include")

PRAGMA_ONCE = "#pragma once\n"

# ── módulo → header que lo declara ─────────────────────────────────
MODULE_HEADERS = {
    "rig_face_engine":       "rig_face_engine.h",
    "rig_face_engine_v2":   "rig_face_engine_v2.h",
    "rig_face_v2":           "rig_face_v2.h",
    "rig_face_v2_bridge":    "rig_face_v2_bridge.h",
    "rig_face_ng":           "rig_face_ng.h",
    "rig_face_ng_anim":      "rig_face_ng.h",
    "rig_face_ng_assembly":  "rig_face_ng.h",
    "rig_face_ng_eyes":      "rig_face_ng.h",
    "rig_face_ng_hair":      "rig_face_ng.h",
    "rig_face_ng_skin":      "rig_face_ng.h",
    "rig_face_archetypes":   "rig_face_archetypes.h",
    "rig_face_codegen":      "rig_face_codegen.h",
    "rig_face_sovereign_audio":  "rig_face_sovereign.h",
    "rig_face_sovereign_body":   "rig_face_sovereign.h",
    "rig_face_sovereign_camera": "rig_face_sovereign.h",
    "rig_face_sovereign_cli":    "rig_face_sovereign.h",
    "rig_face_sovereign_core":   "rig_face_sovereign.h",
    "rig_face_sovereign_gltf":   "rig_face_sovereign.h",
    "rig_face_sovereign_pathtrace": "rig_face_sovereign.h",
    "rig_face_sovereign_vision":  "rig_face_sovereign.h",
    "rig_face_sovereign":    "rig_face_sovereign.h",
    "rig_face_pose_body_suite": "rig_face_pose_body_suite.h",
    "rig_face_pose_body_main": None,
    "rig_face_suite_compat": "rig_face_v2_bridge.h",
    "rig_aom": "rig_aom.h",
    "rig_env": "rig_env.h",
    "rig_haptic": "rig_haptic.h",
    "rig_headtrack": "rig_headtrack.h",
    "rig_height3d": "rig_height3d.h",
    "rig_material": "rig_material.h",
    "rig_parametric": "rig_parametric.h",
    "rig_render_loop": "rig_render_loop.h",
    "rig_ui_router": "rig_ui_router.h",
    "rig_display": "rig_display.h",
    "rig_voz": "rig_voz.h",
    "rigart_v4_art": "rigart_v4_art.h",
    "rigart_geo_extra": "rigart_v3_3d.h",
    "rigart_v3": "rigart_v3.h",
    "rigart_v3_3d": "rigart_v3_3d.h",
    "rigart_v3_cont": "rigart_v3.h",
    "rigart_v5_mobile": "rigart_v5_mobile.h",
    "rig_gpu_old": "rig_gpu.h",
    "rig_gpu_stack": "rig_gpu.h",
    "rig_master": "rig_master.h",
    "rig_math": "rig_math.h",
}

NOEXT = {
    "rig_noext_io.h": ["stdio.h", "stdarg.h"],
    "rig_noext_str.h": ["string.h"],
    "rig_noext_mem.h": ["stdlib.h"],
    "rig_noext_types.h": ["stdint.h", "stddef.h", "float.h", "limits.h", "stdbool.h"],
    "rig_syscall.h": ["sys/types.h", "sys/stat.h", "unistd.h", "errno.h", "fcntl.h", "time.h"],
    "rig_math.h": ["math.h"],
}

def generate_noext(out_dir):
    d = os.path.join(out_dir, "rigdeps")
    os.makedirs(d, exist_ok=True)
    for name, system_headers in NOEXT.items():
        p = os.path.join(d, name)
        if os.path.exists(p):
            continue
        body = PRAGMA_ONCE + "\n/* GEN */\n"
        body += "".join(f'#include <{h}>\n' for h in system_headers)
        open(p, "w").write(body)

FUNC_RE = re.compile(
    r'^(?P<ret>[A-Za-z_][A-Za-z0-9_ \*]*?)\s+(?P<name>rig_[A-Za-z0-9_]+)\s*\((?P<args>[^;{)]*)\)\s*(?:\{|;)'
    , re.M)

def extract_public_functions(path):
    txt = open(path, errors="replace").read()
    txt = re.sub(r'/\*.*?\*/', '', txt, flags=re.S)
    out = []
    for m in FUNC_RE.finditer(txt):
        ret, name, args = m.group('ret').strip(), m.group('name'), m.group('args').strip()
        if 'static' in ret:
            continue
        ret = re.sub(r'\s+', ' ', ret)
        args = re.sub(r'\s+', ' ', args)
        if args in ("", "void"):
            args = "void"
        out.append((ret, name, args))
    return out

def extract_typedefs(path):
    txt = open(path, errors="replace").read()
    txt = re.sub(r'/\*.*?\*/', '', txt, flags=re.S)
    txt = re.sub(r'\n\s*\n', '\n', txt)
    mdefs = []
    # typedef struct {...} Name;
    for m in re.finditer(r'typedef\s+struct\s*\{[^}]*\}\s*(Rig[A-Za-z0-9_]+)\s*;', txt):
        block = re.sub(r'\s+', ' ', m.group(0))
        mdefs.append(block)
    for m in re.finditer(r'typedef\s+enum\s*\{[^}]*\}\s*(Rig[A-Za-z0-9_]+)\s*;', txt):
        block = re.sub(r'\s+', ' ', m.group(0))
        mdefs.append(all_strip := block)
    for m in re.finditer(r'typedef\s+[A-Za-z0-9_]+\s+(Rig[A-Za-z0-9_]+)\s*;', txt):
        mdefs.append(re.sub(r'\s+', ' ', m.group(0)))
    return mdefs

def main():
    os.makedirs(OUT, exist_ok=True)
    generate_noext(OUT)
    header_to_decls = {}
    typedefs_seen = {}
    for c in sorted(glob.glob(os.path.join(SRC, "*.c"))):
        mod = os.path.basename(c)[:-2]
        hdr = MODULE_HEADERS.get(mod)
        if not hdr:
            continue
        for ret, name, args in extract_public_functions(c):
            header_to_decls.setdefault(hdr, {})[name] = f"{ret} {name}({args});"
        for td in extract_typedefs(c):
            m = re.search(r'\b(Rig[A-Za-z0-9_]+)\s*;', td)
            if m:
                typedefs_seen.setdefault(hdr, {})[m.group(1)] = td

    for hdr, decls in header_to_decls.items():
        p = os.path.join(OUT, hdr)
        typedefs = typedefs_seen.get(hdr, {})
        body = [PRAGMA_ONCE, "/* GEN */",
                '#include "rigdeps/rig_noext_types.h"',
                '#include "rigdeps/rig_math.h"', ""]
        for td in typedefs.values():
            body.append(td)
        body.append("")
        for sig in sorted(decls.values()):
            body.append(sig)
        open(p, "w").write("\n".join(body) + "\n")
        print(f"include/{hdr}: {len(decls)} funciones, {len(typedefs)} tipos")

if __name__ == "__main__":
    main()
