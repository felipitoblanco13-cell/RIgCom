#!/usr/bin/env bash
# Build soberano de RIgCom — capas que compilan limpio (C11, sin deps externas).
# Los módulos periféricos con headers perdidos (catedral_ui, holo_trace,
# rigart_v3*, rig_master, rig_gpu_stack, ...) quedan fuera hasta que se
# reconstruyan sus tipos.
set -euo pipefail
cd "$(dirname "$0")/.."

CC="${CC:-gcc}"
CFLAGS="-std=c11 -D_GNU_SOURCE -Iinclude -Iinclude/rigdeps -O2 -Wall -Wno-unused-variable"

SOVEREIGN="src/rig_face_sovereign_core.c src/rig_face_sovereign_body.c src/rig_face_sovereign_gltf.c src/rig_face_sovereign_vision.c src/rig_face_sovereign_audio.c src/rig_face_sovereign_pathtrace.c src/rig_face_sovereign_camera.c src/rig_face_sovereign_cli.c"

CORE="src/rig_face_engine.c src/rig_face_engine_v2.c src/rig_face_archetypes.c src/rig_face_v2.c src/rig_face_v2_bridge.c src/rig_face_codegen.c src/rig_face_ng.c src/rig_face_ng_eyes.c src/rig_face_ng_hair.c src/rig_face_ng_skin.c src/rig_face_ng_anim.c src/rig_face_ng_assembly.c"

mkdir -p build
echo "[1/3] Binario sovereign (selftest incluido)..."
$CC $CFLAGS -o build/rig_sovereign $SOVEREIGN \
    tools/rig_sov_main.c -lm
echo "[2/3] Objetos core (engine/v2/ng)..."
mkdir -p build/obj
for f in $CORE; do
    $CC $CFLAGS -c "$f" -o "build/obj/$(basename "$f" .c).o"
done
echo "[3/3] OK — build/rig_sovereign listo; objetos core en build/obj/"
