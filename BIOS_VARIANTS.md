BIOS Variants & uso

Este proyecto soporta la selección de varias variantes de BIOS en tiempo de compilación.

Variantes previstas (por ahora):

- `default`  — la BIOS que venía con el proyecto (archivo `biosrom.h`).
- `ibmpc`    — IBM PC original (usa CPU ~4.772727 MHz).
- `compaq`   — Compaq DeskPro BIOS (usuario debe proporcionar binario).
- `amstrad`  — Amstrad PC1512 BIOS (usuario debe proporcionar binario).

Ficheros esperados (cabeceras):

- `biosrom.h`       (default)
- `bios_ibmpc.h`    (IBM PC)
- `bios_compaq.h`   (Compaq DeskPro)
- `bios_amstrad.h`  (Amstrad PC1512)

Cómo generar una cabecera desde la BIOS binaria:

1. Copia tu BIOS binaria (por ejemplo `IBMPC_BIOS.bin`) al directorio del proyecto.
2. Ejecuta:

```sh
./bin2h.sh IBMPC_BIOS.bin bios_ibmpc.h
```

Esto generará `bios_ibmpc.h` con la lista de bytes adecuada para incluir dentro de
`static const uint8_t biosrom[] = { /* aquí */ };` en `bios.cpp`.

Seleccionar variante en tiempo de compilación:

- Edita `emu_config.h` y cambia `BIOS_VARIANT` a uno de los valores (`BIOS_VARIANT_IBMPC`, etc.).
- O bien define macros al compilar: por ejemplo

```sh
# usar IBM PC BIOS y fijar la frecuencia emulada a 4.772727 MHz
g++ -DBIOS_VARIANT=1 -DEMU_CPU_FREQ_HZ=4772727 ...
```

Nota sobre frecuencias: la macro `EMU_CPU_FREQ_HZ` controla la velocidad emulada. Por defecto
está en ~4.772727 MHz; puedes cambiarla con `-DEMU_CPU_FREQ_HZ=<Hz>`.

No se incluyen BIOS propietarias en este repositorio. Debes proporcionar los binarios
legalmente obtenidos y convertirlos a header con `bin2h.sh`.
