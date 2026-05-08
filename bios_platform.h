/*
  bios_platform.h

  Per-platform identity macros consumed by bios.cpp.
  Include ONLY from bios.cpp, after emu_config.h has been included.

  For each supported BIOS variant the following macros are defined:

    BIOS_ROM_LOAD_ADDR      Physical address where the identity ROM overlay is
                            placed (usually 0xFE000 for IBM-compatible BIOSes).
                            Only meaningful when BIOS_IDENTITY_HEADER is defined.

    BIOS_OEM_NAME           Human-readable machine name (for Serial/debug output).

    BIOS_HAS_ROM_BASIC      1 if the platform has a ROM BASIC that must be loaded
                            from the SD card; 0 otherwise.
    BIOS_ROM_BASIC_SEG      Segment of the ROM BASIC entry point (INT 18h).
    BIOS_ROM_BASIC_OFF      Offset of the ROM BASIC entry point (INT 18h).
    BIOS_ROM_BASIC_ADDR     Physical load address of the ROM BASIC (= SEG * 16).
    BIOS_ROM_BASIC_SIZE     Total size in bytes of the ROM BASIC image.
    BIOS_ROM_BASIC_FILE     Path relative to SD_MOUNT_PATH for the combined image.
                            (Concatenate basicc11.f6 + .f8 + .fa + .fc = 32 KB.)

    BIOS_EQUIPMENT_WORD     Default INT 11h equipment word written to BDA 0x40:0x10
                            during BIOS::reset().
*/

#pragma once

#include "emu_config.h"

/* ═══════════════════════════════════════════════════════════════════════════
   IBM PC 5150  (BIOS date 10/27/82, pc102782.bin, 8 KB at F000:E000–FFFF)
   ═══════════════════════════════════════════════════════════════════════════

   Identity bytes extracted from bios/ibmpc/pc102782.bin (verified, sum=0):
     phys 0xFFFF5  "10/27/82"              (8 bytes, no NUL)
     phys 0xFFFFE  0xFF                    machine ID  (5150=0xFF, XT=0xFE)
     phys 0xFFFFF  0x77                    ROM checksum
     phys 0xFE000  "1501476 COPR. IBM 1982"  (22 bytes)

   INT 18h (ROM BASIC) entry confirmed from IVT init code in the ROM:
     phys 0xFE211  C7 06 62 00 00 F6  → MOV [0x0062], 0xF600  (seg)
     phys 0xFF175  C7 06 60 00 07 06  → MOV [0x0060], 0x0607  (offset)
   → INT 18h = F600:0607, physical 0xF6607

   The IBM ROM is loaded as a READ-ONLY identity overlay at 0xFE000.
   The FabGL custom BIOS at F000:0100 provides all functional INT handlers.
   The reset vector at 0xFFFF0 is overwritten to point to the custom BIOS.
   ═══════════════════════════════════════════════════════════════════════════ */
#if BIOS_VARIANT == BIOS_VARIANT_IBMPC

#define BIOS_ROM_LOAD_ADDR          0xFE000u

#define BIOS_OEM_NAME               "IBM PC 5150"

#define BIOS_OEM_DATE               "10/27/82"
#define BIOS_MACHINE_ID             0xFFu
#define BIOS_ROM_CHECKSUM           0x77u

#define BIOS_CONVENTIONAL_RAM_KB 256u

/* INT 18h → ROM BASIC */
#define BIOS_HAS_ROM_BASIC          1
#define BIOS_ROM_BASIC_SEG          0xF600u
#define BIOS_ROM_BASIC_OFF          0x0000u
#define BIOS_ROM_BASIC_ADDR         (BIOS_ROM_BASIC_SEG * 16u)   /* 0xF6000 */
#define BIOS_ROM_BASIC_SIZE         0x8000u   /* 4 × 8 KB chips (F6, F8, FA, FC) */
/* Embedded ROM BASIC image for IBM PC (32 KB = basicc11.f6 || f8 || fa || fc). */
#define BIOS_ROM_BASIC_EMBEDDED_HEADER "bios_basic_ibmpc.h"
/* Optional SD fallback path if embedded header is removed. */
#define BIOS_ROM_BASIC_FILE         "IBMPC/BASIC.ROM"

/* Equipment word → BDA 0x40:0x10
   0x0061 = bit0(floppy IPL) | bits[5:4]=10(80×25 CGA) | bits[7:6]=01(2 drives) */
#define BIOS_EQUIPMENT_WORD         0x0061u

/* ═══════════════════════════════════════════════════════════════════════════
   Compaq Deskpro  (not yet implemented)
   ═══════════════════════════════════════════════════════════════════════════ */
#elif BIOS_VARIANT == BIOS_VARIANT_COMPAQ
#define BIOS_CONVENTIONAL_RAM_KB 640u
#error "BIOS_VARIANT_COMPAQ: bios_platform.h macros not yet defined.  Add a bios_compaq.h and fill in the platform block."

/* ═══════════════════════════════════════════════════════════════════════════
   Amstrad PC1512  (not yet implemented)
   ═══════════════════════════════════════════════════════════════════════════ */
#elif BIOS_VARIANT == BIOS_VARIANT_AMSTRAD
#define BIOS_CONVENTIONAL_RAM_KB 512u
#error "BIOS_VARIANT_AMSTRAD: bios_platform.h macros not yet defined.  Add a bios_amstrad.h and fill in the platform block."

/* ═══════════════════════════════════════════════════════════════════════════
   DEFAULT  (FabGL custom BIOS — no identity overlay, no ROM BASIC)
   ═══════════════════════════════════════════════════════════════════════════ */
#else

#define BIOS_ROM_LOAD_ADDR          BIOS_ADDR   /* unused for DEFAULT */

#define BIOS_OEM_NAME               "FabGL PC"

#define BIOS_OEM_DATE               "01/01/91"
#define BIOS_MACHINE_ID             0xFCu
#define BIOS_ROM_CHECKSUM           0x01u

#define BIOS_CONVENTIONAL_RAM_KB    640u

#define BIOS_HAS_ROM_BASIC          0

/* Equipment word for a generic 1-floppy, 80×25 CGA machine */
#define BIOS_EQUIPMENT_WORD         0x0021u

#endif
