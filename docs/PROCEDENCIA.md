# Procedencia de cada grupo de archivos

## src/ — set vigente

Proviene íntegro de `RigFaceyArte.zip` (2026-08-04), 63 módulos `.c`.

Al ser el set más reciente, sus versiones prevalecen sobre los homónimos
de los otros sets cuando difieren. Los 33 módulos que también existen en
`archive/rigface-c-unificado/src/` tienen **contenido distinto** (ver
matriz abajo); los 8 `rig_*.c` compartidos con `variantes-plano95/` ZIP1
son byte-idénticos.

## archive/rigface-c-unificado/ — curaduría 2026-07-23

Fuente: `RIGFACE_C_UNIFICADO.zip`. Unificación documentada de
`TODoRigFace.zip` (7 ZIP anidados, 124 `.c` + 23 `.h`) en 33 módulos
ordenados por dependencia. Su `inventario/` registra 522 cuerpos de
función, 519 conservados, 3 en cuarentena:

- `arena_reset` no reseteaba la arena (CUERPO_SUSTITUIDO)
- `rig_sov_export_glb` con macros sin nombre (entrega HEDERSFACE)
- `rig_face_build_follicle_map` con sufijo vandalizado `0xC4TEDRAL1`

Las versiones de `src/` contienen las formas reparadas.

## archive/rigface-estudio-soberano-master/ — master 2026-08-03

Fuente: `RIGFACE_Estudio_Soberano_Master.zip`. Contiene el master del
estudio (7 fuentes propias: `rig_face_master.c/.h`,
`rig_master_sovereign_engine.c/.h`, `rig_face_session.h`,
`rig_aom_physical.h`, `test_rigface_master.c`), un panel HTML
(`rigface_studio_master.html`) y binarios viejos precompilados
(`test_rigface_master`, `test_master_engine`, `rig_face_master.o`)
más sus manifiestos SHA-256 e inventarios JSON.

## archive/variantes-plano95/ — dump plano 2026-08-03

Fuente: `RIGART_RIGFACE_TODO_PLANO_95_ARCHIVOS(1).zip`, 87 archivos
con prefijos ZIP1/ZIP2 de agrupación temática. Único lugar con:
`identity_engine.c`, `rig_identity.c` y `rig_master_pbr.frag`.

## rigcom-js/

Monolito JS unificado, traído de la rama
`claude/unificacion-sin-duplicados-w2x3rr` (merge, sin duplicados).

## Verificación de integridad

```
sha256sum -c docs/MANIFIESTO_SHA256.txt
```
