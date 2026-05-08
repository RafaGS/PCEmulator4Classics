/*
  emu_config.h

  Configuración de emulación por macros. Permite seleccionar el BIOS
  que se incluye en tiempo de compilación y fijar la frecuencia de CPU
  emulada.

  Para sobrescribir desde la línea de compilación:
    - Para usar otra cabecera de BIOS: -DBIOS_ROM_HEADER=\"mi_biosrom.h\"
    - Para fijar otra frecuencia CPU: -DEMU_CPU_FREQ_HZ=10000000

  Valores por defecto razonables para comenzar:
    EMU_CPU_FREQ_HZ = 4772727 (IBM PC original ~4.772727 MHz)
    BIOS_ROM_HEADER = "biosrom.h"
*/

#pragma once

#ifndef EMU_CONFIG_H
#define EMU_CONFIG_H

// ---------------------------------------------------------------------------
// BIOS variants
// ---------------------------------------------------------------------------
// Define one of these values to select a BIOS variant at compile time.
#define BIOS_VARIANT_DEFAULT  0
#define BIOS_VARIANT_IBMPC    1
#define BIOS_VARIANT_COMPAQ   2
#define BIOS_VARIANT_AMSTRAD  3

// Select variant (override with -DBIOS_VARIANT=<n> if desired)
#ifndef BIOS_VARIANT
#define BIOS_VARIANT BIOS_VARIANT_DEFAULT
#endif

// BIOS_ROM_HEADER: the FabGL custom BIOS that provides all functional INT handlers
// (disk, video, keyboard, etc.) via soft-traps.  Always "biosrom.h".
// The real IBM/Compaq/Amstrad ROMs cannot run directly in the emulator because
// there is no hardware port emulation for their specific chipsets.
#ifndef BIOS_ROM_HEADER
#define BIOS_ROM_HEADER "biosrom.h"
#endif

// BIOS_IDENTITY_HEADER: optional platform ROM loaded as an IDENTITY OVERLAY at
// BIOS_ROM_LOAD_ADDR (defined in bios_platform.h).  It provides IBM-compatible
// identity bytes (date, machine ID, copyright, checksum) so that diagnostic
// tools see the correct machine.  The custom BIOS still runs at F000:0100.
#if BIOS_VARIANT == BIOS_VARIANT_IBMPC && !defined(BIOS_IDENTITY_HEADER)
#define BIOS_IDENTITY_HEADER "bios_ibmpc.h"
#endif

// Per-variant default CPU frequencies (also overridable with -DEMU_CPU_FREQ_HZ=n)
#if BIOS_VARIANT == BIOS_VARIANT_IBMPC && !defined(EMU_CPU_FREQ_HZ)
#define EMU_CPU_FREQ_HZ 4772727UL
#elif BIOS_VARIANT == BIOS_VARIANT_COMPAQ && !defined(EMU_CPU_FREQ_HZ)
#define EMU_CPU_FREQ_HZ 7168000UL
#elif BIOS_VARIANT == BIOS_VARIANT_AMSTRAD && !defined(EMU_CPU_FREQ_HZ)
#define EMU_CPU_FREQ_HZ 8000000UL
#endif


// ---------------------------------------------------------------------------
// Emulated CPU frequency
// ---------------------------------------------------------------------------
// Default frequency in Hz for the emulated CPU. Can be overridden with
// -DEMU_CPU_FREQ_HZ=<value> at compile time. The default value emulates
// approximately the IBM PC clock (4.772727 MHz).
#ifndef EMU_CPU_FREQ_HZ
#define EMU_CPU_FREQ_HZ 4772727UL
#endif

// ---------------------------------------------------------------------------
// CPU timing calibration
// ---------------------------------------------------------------------------
// i8086::step() advances one instruction, not one real 8088 bus cycle.
// The effective speed therefore must be calibrated empirically against DOS
// benchmarks, not only from nominal 8088 cycle tables.
//
// Effective step rate = EMU_CPU_FREQ_HZ * DEN / NUM
// Default IBM PC calibration: about 38.75 effective cycles per step.
#ifndef EMU_CPU_CYCLES_PER_STEP_NUM
#define EMU_CPU_CYCLES_PER_STEP_NUM 155ULL
#endif

#ifndef EMU_CPU_CYCLES_PER_STEP_DEN
#define EMU_CPU_CYCLES_PER_STEP_DEN 4ULL
#endif

#endif // EMU_CONFIG_H
