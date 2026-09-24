# RIgCom — Ecosistema RigFace / RigArt

Repositorio del ecosistema de rostro y arte generativo en C:
**RigFace** (rostro procedural: anatomía, genética, piel, cabello, ojos, dentición,
path-tracing, export ARKit/VRM/GLB) y **RigArt / RigCom** (UI soberana, arte
paramétrico, visor 3D por WebSocket).

## Estructura

```
src/                                  Fuente vigente (63 módulos .c)
  rig_face_*.c                          Rostro: engine, v2, ng (nueva gen), sovereign
  rigart_*.c                            Arte paramétrico v3–v5
  rig_*.c                               Infra: GPU, display, voz, haptics, env, master
  rigcom_ui.c · catedral_ui.c · ...     UI/visor 3D (WS JSON, puerto 61803)
  rig_face_pose_body_main.c             main() de la suite Pose Body

archive/                              Sets históricos íntegros (solo consulta)
  rigface-c-unificado/                  Unificación curada 2026-07-23 (33 módulos + inventario)
  rigface-estudio-soberano-master/      Master del estudio 2026-08-03 (incluye binarios viejos)
  variantes-plano95/                    Dump plano de 87 archivos 2026-08-03 (ZIP1/ZIP2)

docs/
  MANIFIESTO_SHA256.txt                 Hash de cada archivo (sha256sum -c)
  INVENTARIO_SRC.md                     Tabla de módulos de src/
  PROCEDENCIA.md                        De dónde sale cada cosa

rigcom-js/                            Monolito JS de RigCom (rama claude/… unificada aquí)
```

## Procedencia

Los cuatro zips que estaban en la raíz se descomprimieron y ordenaron:

| Origen (zip) | Fecha | Destino |
|---|---|---|
| `RigFaceyArte.zip` | 2026-08-04 | `src/` — **set vigente**, el más reciente y completo |
| `RIGFACE_C_UNIFICADO.zip` | 2026-07-23 | `archive/rigface-c-unificado/` — curaduría previa, con su propio `inventario/` |
| `RIGFACE_Estudio_Soberano_Master.zip` | 2026-08-03 | `archive/rigface-estudio-soberano-master/` |
| `RIGART_RIGFACE_TODO_PLANO_95_ARCHIVOS(1).zip` | 2026-08-03 | `archive/variantes-plano95/` |

Los `.zip` originales siguen disponibles en el historial de git
(commit `8893be8`), no hace falta clonarlos en el árbol.

## Estado de compilación

Los módulos dependen de una capa de runtime **no incluida en ningún zip**
(`rigdeps/`: `rig_math.h`, `rig_noext_*.h`, `rig_syscall.h`, `rig_face_sovereign.h`,
`rig_face_ng.h`, `../include/*.h`, …). Hasta rescatar esos headers, `src/`
no compila completo; los sets de `archive/` están en la misma situación.

Para compilar en Termux (ARM64) una vez recuperados los headers:

```sh
pkg install clang git
git clone https://github.com/felipitoblanco13-cell/RIgCom
cd RIgCom
clang -I<ruta-a-rigdeps> -Iinclude src/*.c -lm -o rigcom
```

## Nota sobre versiones

Cuando un módulo existe en varios sets con contenido distinto
(p. ej. `rig_face_v2.c` aparece con 3 tamaños diferentes), `src/` conserva
la versión de `RigFaceyArte.zip` (la más reciente) y las variantes
anteriores quedan en `archive/`. La curaduría de `archive/rigface-c-unificado/`
documentó 3 funciones corruptas en cuarentena; las versiones de `src/`
contienen las formas reparadas (`0xC47ED1A1`, `arena_reset` funcional).
