# rigcom-monolith.js — fusión unificada, sin duplicados

Este directorio contiene la unificación de los 4 archivos JS del pipeline
RIGCOM (compilador C → ARM64 → ELF ejecutable, más los conversores
TFLite ⇄ GGUF) recibidos como entradas separadas:

| Archivo de entrada                | Rol                                                              | ¿Dónde terminó?                                   |
|------------------------------------|-------------------------------------------------------------------|----------------------------------------------------|
| `rigcom-monolith.js` (v16)         | Monolito con TODOS los módulos, incluyendo IR→ELF y ops RIGADIEL  | Base de la fusión                                   |
| `rigcom-monolith.fixed.js` (v14)   | Snapshot anterior (sin IR→ELF ni ops RIGADIEL) con bugs reales corregidos en el compilador y los selftests | Sus fixes reales se **portaron** a la base v16      |
| `rig_native_pipeline.js`           | Puente HL-IR/IR → ARM64 → ELF ejecutable                          | Ya estaba embebido **idéntico byte a byte** en el monolito v16 (módulo `rig_native_pipeline.js`) — no se duplicó |
| `rig_converter_universal.js`       | Conversor TFLite ⇄ GGUF (v1.3.0, con ops custom RIGADIEL)         | Ya estaba embebido **idéntico byte a byte** en el monolito v16 (módulo `rig_converter_universal.js`) — no se duplicó |

## Qué se verificó antes de fusionar

Se compararon los 4 archivos con `diff` para no perder ni duplicar nada:

- `rig_native_pipeline.js` y `rig_converter_universal.js` resultaron ser
  **extracciones exactas** de los módulos ya embebidos en
  `rigcom-monolith.js` (v16) — mismo contenido, solo sin el wrapper
  `__rigRunModule(...)`. No aportaban código nuevo, así que no se agregaron
  como archivos sueltos (eso sí hubiera sido duplicar).
- `rigcom-monolith.fixed.js` resultó ser un snapshot **anterior** (v14) al
  que le faltan 3 módulos completos que sí tiene v16
  (`rig_elf64_io.js`, `rig_codegen_arm64.js`, `rig_native_pipeline.js`) y la
  extensión de ops custom RIGADIEL en el conversor — pero tiene **bugs reales
  corregidos** en el compilador (`rig_compiler_soberano.js`) y en los
  selftests que v16 no tiene.

## Fixes reales portados de `.fixed.js` a la base v16

Todos verificados compilando y ejecutando el resultado (ver "Verificación" abajo):

1. **RegAlloc / ALLOCA**: reserva por adelantado un hueco de pila único por
   cada `IR.ALLOCA`, en vez de dejar `spill_offsets` en `-1` para variables
   que no fueron espolvoreadas por presión de registros (antes: todas las
   variables locales sin spill compartían la misma dirección de pila).
2. **Aliasing de operandos IR compartidos**: en la eliminación de código
   muerto y en el plegado de `NEG`/`NOT`, se reemplazan los objetos
   `{id,type}` en vez de mutarlos en sitio (los objetos se comparten por
   referencia entre instrucciones — mutar uno mutaba todos sus usos).
3. **`++` / `--` (prefijo y postfijo)**: no tenían caso propio en el
   lowering y caían al lector genérico sin escribir nunca el nuevo valor —
   un `for` con `i++` quedaba en loop infinito. Ahora emiten
   `LOAD → ADD/SUB #1 → STORE` real para identificadores locales.
4. **Parámetros reasignados dentro de un bucle**: los parámetros vivían en
   un vreg directo; si el cuerpo los reasignaba, la condición del bucle
   (bajada una sola vez, antes del cuerpo) seguía leyendo el registro físico
   original y nunca veía la actualización. Ahora todo parámetro recibe su
   propio slot de memoria desde la entrada de la función, igual que un local.
5. **Smoke test Android (zip/aar) con IO en memoria**: si nadie llamó
   `setIO()` antes del smoke test, se instala un IO temporal 100% en memoria
   (no simula el dispositivo real, solo evita que el test dependa de disco).
6. **Selftests honestos**: `tflite_runtime.js` y `tflite_converter.js` ahora
   marcan `skip:true` explícito cuando una verificación no corrió (p.ej. sin
   VM o sin runtime disponible) en vez de reportar `ok:true` falso.

## Qué NO se portó (a propósito)

- La eliminación de las ops custom RIGADIEL (`RIGADIEL_HEAD`,
  `RIG_CERTEZA_GATE`, `RIG_VOLUNTAD_GATE`, `RIG_PHI_MIX`) del conversor
  TFLite⇄GGUF en `.fixed.js` — es una regresión de un snapshot anterior a
  que existiera esa función, no un fix. Se mantuvo la versión completa (v16).
- Los 3 módulos ausentes en `.fixed.js` (`rig_elf64_io.js`,
  `rig_codegen_arm64.js`, `rig_native_pipeline.js`) — están completos en la
  base v16 y no había nada que fusionar.

## Verificación

```
node --check rigcom-monolith.js   # sintaxis OK

node -e "
global.self = global;
require('./rigcom-monolith.js');
console.log(global.RigNativePipeline.selfTest().pass);        // true
console.log(global.RigTFLiteRuntime.selftest().pass);         // true
console.log(global.RigTFLiteConverter.selftest().pass);       // true
console.log(global.RigConverterUniversal.VERSION);            // 1.3.0 (RIGADIEL intacto)
"
```

Además se compilaron y certificaron (IR → ARM64 → ELF, roundtrip bit-exacto)
dos programas que ejercitan directamente los fixes portados:

- `int fact(int n){int r=1; while(n>1){r=r*n; n=n-1;} return r;}` (parámetro
  reasignado dentro de un bucle).
- `int sum10(){int s=0; for(int i=0;i<10;i++) s=s+i; return s;}` (`++` en un
  `for`).

Ambos: `certified: true`.

`RIGCOM_MONOLITH.version` quedó en `'v17-unified'` para distinguirlo de las
dos entradas (`v16-cycles-closed`, `v14-final`).
