# RIGFACE — C UNIFICADO

Origen: `TODoRigFace.zip` — 7 ZIP anidados, 124 `.c` + 23 `.h`.
Salida: **33 módulos `.c`** ordenados por dependencia.

| | |
|---|---|
| Cuerpos de función distintos en origen | 522 |
| Conservados | 519 |
| En cuarentena | 3 |
| **Perdidos** | **0** |

## Selección de canónica

No por tamaño ni por score estructural: ambos criterios eligen el cuerpo
corrupto en `arena_reset`. La canónica exige que el cuerpo **toque el estado
compartido que tocan sus hermanos**.

## Cuarentena (3) — apartadas con procedencia, no borradas

- `nested/HEDERSFACE/src/rig_face_engine.c:59` — `arena_reset` → **CUERPO_SUSTITUIDO**
- `nested/HEDERSFACE/src/rig_face_sovereign_gltf.c:19` — `rig_sov_export_glb` → **MACRO_SIN_NOMBRE**
- `src_raw/rig_face_engine_v2.c:703` — `rig_face_build_follicle_map` → **SUFIJO_INVALIDO**

- `CUERPO_SUSTITUIDO`: `arena_reset` no reseteaba la arena; devolvía una
  constante desde una local. El hermano correcto escribe `g_arena.used`.
- `MACRO_SIN_NOMBRE`: toda la entrega `HEDERSFACE` tiene los nombres de macro
  sustituidos por `...`.
- `SUFIJO_INVALIDO`: `0xC47ED1A1u` vandalizado a `0xC4TEDRAL1u`.

## Módulos

| # | archivo | funciones | copias | injertadas | variantes |
|---|---|---|---|---|---|
| 1 | `01_rig_face_cloth.c` | 17 | 1 | 0 | 0 |
| 2 | `02_rig_face_dentition.c` | 9 | 1 | 0 | 0 |
| 3 | `03_rig_face_dermal_dynamics.c` | 14 | 1 | 0 | 0 |
| 4 | `04_rig_face_engine.c` | 76 | 6 | 44 | 44 |
| 5 | `05_rig_face_export_arkit.c` | 5 | 1 | 0 | 0 |
| 6 | `06_rig_face_export_vrm.c` | 7 | 1 | 0 | 0 |
| 7 | `07_rig_face_eye_vergence.c` | 8 | 1 | 0 | 0 |
| 8 | `08_rig_face_genetics.c` | 19 | 1 | 0 | 0 |
| 9 | `09_rig_face_hair_growth.c` | 12 | 1 | 0 | 0 |
| 10 | `10_rig_face_ng_anim.c` | 2 | 6 | 3 | 3 |
| 11 | `11_rig_face_ng_assembly.c` | 9 | 6 | 7 | 7 |
| 12 | `12_rig_face_ng_eyes.c` | 9 | 6 | 4 | 4 |
| 13 | `13_rig_face_ng_hair.c` | 9 | 6 | 35 | 5 |
| 14 | `14_rig_face_ng_skin.c` | 1 | 6 | 0 | 0 |
| 15 | `15_rig_face_pathtrace_multi.c` | 23 | 2 | 0 | 0 |
| 16 | `16_rig_face_soft_body.c` | 15 | 1 | 0 | 0 |
| 17 | `17_rig_face_sovereign_audio.c` | 10 | 5 | 0 | 0 |
| 18 | `18_rig_face_suite_compat.c` | 4 | 1 | 0 | 0 |
| 19 | `19_rig_face_voice_synth.c` | 13 | 1 | 0 | 0 |
| 20 | `20_rig_face_archetypes.c` | 15 | 7 | 3 | 3 |
| 21 | `21_rig_face_codegen.c` | 9 | 7 | 2 | 2 |
| 22 | `22_rig_face_sovereign_body.c` | 20 | 5 | 0 | 0 |
| 23 | `23_rig_face_sovereign_core.c` | 35 | 5 | 0 | 0 |
| 24 | `24_rig_face_sovereign_gltf.c` | 8 | 5 | 0 | 0 |
| 25 | `25_rig_face_sovereign_vision.c` | 13 | 5 | 0 | 0 |
| 26 | `26_rig_face_sovereign_camera.c` | 10 | 5 | 0 | 0 |
| 27 | `27_rig_face_recon_bridge.c` | 8 | 1 | 0 | 0 |
| 28 | `28_rig_face_sovereign_pathtrace.c` | 34 | 4 | 0 | 0 |
| 29 | `29_rig_face_v2.c` | 81 | 7 | 23 | 22 |
| 30 | `30_rig_face_v2_bridge.c` | 13 | 5 | 5 | 5 |
| 31 | `31_rig_face_sovereign_cli.c` | 7 | 5 | 0 | 0 |
| 32 | `32_rig_face_pose_body_suite.c` | 13 | 1 | 0 | 0 |
| 33 | `33_rig_face_pose_body_main.c` | 3 | 1 | 0 | 0 |

## Inventario

- `inventario/funciones.csv` — 531 definiciones con archivo y línea
- `inventario/variantes.csv` — 95 variantes con procedencia
- `inventario/cuarentena.csv` — las 3 apartadas
- `inventario/fuentes_leidas.txt` — los 147 leídos
- `inventario/inventario.json` · `inventario.csv` · `conteos.txt` · `manifest.sha256

