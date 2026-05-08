# PC Emulator 4 Classics

## Cambios realizados

Esta es una versión modificada del emulador de PC de FabGL que ajusta su comportamiento al de los sistemas:
- IBM PC original (modelo 5150).
- Compaq Deskpro (model 1).

### Principales modificaciones:

1. **Inserción de particularidades de BIOS originales**
   - Detectable por herramientas tipo Norton Utilities y PC Tools

2. **Auto-detección de discos**
   - Nueva función `autoDetectDisks()` que busca automáticamente:
     - `A.img` → Disquete A: (fd0)
     - `B.img` → Disquete B: (fd1)
     - `C.img` → Disco duro C: (hd0)
     - `D.img` → Disco duro D: (hd1)
   - Monta automáticamente todas las imágenes encontradas
   - Selección automática de unidad de arranque (C: si existe, sino A:)

## Uso

### Requisitos:
- ESP32 con PSRAM (ej: TTGO VGA32 v1.4)
- Tarjeta microSD
- Arduino IDE configurado con:
  - PSRAM: **DISABLED**
  - Partition Scheme: **Huge APP**

### Preparación de la microSD:

1. Formatea la microSD en FAT32

2. Copia las imágenes de disco a la raíz de la microSD:
   - `A.img` - Disquete de arranque (1.44MB típicamente)
   - `B.img` - Disquete secundario (opcional)
   - `C.img` - Disco duro de arranque (opcional, pero recomendado)
   - `D.img` - Disco duro secundario (opcional)

3. **Mínimo requerido**: Debes tener al menos `A.img` o `C.img` para poder arrancar

### Comportamiento al arrancar:

1. El emulador detecta automáticamente qué imágenes están presentes
2. Muestra un mensaje con los discos encontrados (se cierra automáticamente en 3 segundos)
3. Si encuentra `C.img`, arranca desde el disco duro
4. Si no encuentra `C.img` pero sí `A.img`, arranca desde disquete
5. Si no encuentra ningún disco booteable, muestra un error y reinicia

### Funciones de teclado:

- **ALT + PrintScreen (SYSREQ)**: Abre el menú del sistema con opciones para:
  - Reiniciar el emulador
  - Reiniciar la máquina
  - Montar/desmontar discos manualmente
  - Continuar

### Creación de imágenes de disco:

#### Para disquetes (A.img, B.img):
- Tamaño: 5,25" y 3,5"
- Formato: Imagen RAW de disquete DOS/Windows

#### Para discos duros (C.img, D.img):
- Tamaños comunes:
  - 8MB: Para sistemas pequeños (DOS, FreeDOS)
  - 20MB: Para sistemas con aplicaciones
  - 40MB+: Para sistemas más completos
- Formato: Imagen RAW de disco duro

## Notas técnicas:

- El código mantiene compatibilidad con el hardware original
- La velocidad de la SD card se establece al máximo después de inicializar
- La geometría de los discos se detecta automáticamente (CHS)
- Puerto serie COM1 está configurado en los pines UART estándar (RX=3, TX=1)

## Créditos:

- Código original: Fabrizio Di Vittorio (FabGL)
- Modificaciones: Rafa Gomez (RafaGS) para Minibots <https://minibots.wordpress.com>
