# RIGFACE / RIGCOM — Archivo maestro (C)

Colección de archivos del proyecto **RIGFACE / RIGCOM**: un motor de rigging facial y corporal en C (95 módulos), con motor soberano, síntesis de voz, visión, UI de consola y exportadores ARKit/VRM. El repositorio almacena los archivos en 4 ZIPs; verificados: el C **compila y ejecuta** correctamente (probado con gcc en Linux).

## Contenido de los ZIPs

| ZIP | Contenido |
|---|---|
| `RIGFACE_Estudio_Soberano_Master.zip` | Estudio "master" consolidado: `rig_face_master.c/.h/.o`, `rig_master_sovereign_engine.c`, `rig_aom_physical.h`, tests, binarios de prueba, inventarios JSON (`INVENTARIO_RIGFACE_95.json`), matriz de funciones CSV y UI HTML. |
| `RIGFACE_C_UNIFICADO.zip` | 45 fuentes C unificados (`src/`): engine, export ARKit/VRM, ojos, audio soberano, archetypes, codegen, etc. |
| `RIGART_RIGFACE_TODO_PLANO_95_ARCHIVOS(1).zip` | Los 95 archivos del plano completo: AOM, env, timeline de edad, dentición, dermal dynamics, cloth, codegen, exportadores… |
| `RigFaceyArte.zip` | 63 archivos mezclando motor y arte: `rigcom_ui.c`, `catedral_ui.c`, `rig_voz.c`, `rigart_v4_art.c`, hair growth, skin, vision soberana… |

## Compilación y ejecución (verificado)

```bash
unzip RIGFACE_Estudio_Soberano_Master.zip -d master && cd master
gcc rig_master_sovereign_engine.c -o engine   # OK
gcc test_rigface_master.c rig_face_master.c -o test  # OK
./engine   # → "RIGFACE Master Engine Initialized Successfully for Honor 400 DNY-NX9!"
```

Los 95 archivos del plano completo aún no tienen un Makefile unificado; los módulos son semi-independientes (cabeceras `.h` con API común). El `.o` y binarios incluidos son de una compilación previa.

## Estado del repositorio

- Solo contiene ZIPs + este README; no hay estructura de fuentes extraída ni CI.
- `rigcom7` y `pepper-clear-slate-cactus` (otros repos del mismo espacio) están **vacíos** (0 commits). La aplicación web funcional del mismo ecosistema vive en `comet-pine-crystal-sand` (ver su README).
