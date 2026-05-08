/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2022 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


 /* Instructions:

    - to run this application you need an ESP32 with PSRAM installed and an SD-CARD slot (ie TTGO VGA32 v1.4 or FabGL Development Board with WROVER)
    - open this with Arduino and make sure PSRAM is DISABLED
    - partition scheme must be: Huge App
    - compile and upload the sketch
    - copy disk images A.img, B.img, C.img, D.img to SD card root
    - A.img and C.img correspond to bootable drives (floppy A: or hard disk C:)

 */


#pragma message "This sketch requires Tools->Partition Scheme = Huge APP"


#include <memory>

#include "esp32-hal-psram.h"
extern "C" {
#include "esp_spiram.h"
}

#include "esp_sntp.h"

#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "fabgl.h"

#include "emu_config.h"
#include "bios_platform.h"
#include "mconf.h"
#include "machine.h"



// UART Pins for USB serial
#define UART_URX 3
#define UART_UTX 1



using std::unique_ptr;

using fabgl::StringList;
using fabgl::imin;
using fabgl::imax;



Preferences   preferences;
InputBox      ibox;
Machine     * machine;


// noinit! Used to maintain datetime between reboots
__NOINIT_ATTR static timeval savedTimeValue;


static bool wifiConnected = false;
static bool downloadOK    = false;


// try to connected using saved parameters
bool tryToConnect()
{
  bool connected = WiFi.status() == WL_CONNECTED;
  if (!connected) {
    char SSID[32] = "";
    char psw[32]  = "";
    if (preferences.getString("SSID", SSID, sizeof(SSID)) && preferences.getString("WiFiPsw", psw, sizeof(psw))) {
      printf("[WiFi] Attempting connection to SSID: %s\n", SSID);
      ibox.progressBox("", "Abort", true, 200, [&](fabgl::ProgressForm * form) {
        WiFi.begin(SSID, psw);
        for (int i = 0; i < 32 && WiFi.status() != WL_CONNECTED; ++i) {
          if (!form->update(i * 100 / 32, "Connecting to %s...", SSID))
            break;
          delay(500);
          if (i == 16)
            WiFi.reconnect();
        }
        connected = (WiFi.status() == WL_CONNECTED);
        printf("[WiFi] Connection %s\n", connected ? "succeeded" : "failed");
      });
      // show to user the connection state
      if (!connected) {
        WiFi.disconnect();
        ibox.message("", "WiFi Connection failed!");
      }
    }
  }
  return connected;
}


bool checkWiFi()
{
  wifiConnected = tryToConnect();
  if (!wifiConnected) {

    // configure WiFi?
    if (ibox.message("WiFi Configuration", "Configure WiFi?", "No", "Yes") == InputResult::Enter) {

      // repeat until connected or until user cancels
      do {

        // yes, scan for networks showing a progress dialog box
        int networksCount = 0;
        ibox.progressBox("", nullptr, false, 200, [&](fabgl::ProgressForm * form) {
          form->update(0, "Scanning WiFi networks...");
          networksCount = WiFi.scanNetworks();
        });

        // are there available WiFi?
        if (networksCount > 0) {

          // yes, show a selectable list
          StringList list;
          for (int i = 0; i < networksCount; ++i)
            list.appendFmt("%s (%d dBm)", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
          int s = ibox.menu("WiFi Configuration", "Please select a WiFi network", &list);

          // user selected something?
          if (s > -1) {
            // yes, ask for WiFi password
            char psw[32] = "";
            if (ibox.textInput("WiFi Configuration", "Insert WiFi password", psw, 31, "Cancel", "OK", true) == InputResult::Enter) {
              // user pressed OK, connect to WiFi...
              preferences.putString("SSID", WiFi.SSID(s).c_str());
              preferences.putString("WiFiPsw", psw);
              wifiConnected = tryToConnect();
              // show to user the connection state
              if (wifiConnected)
                ibox.message("", "Connection succeeded!");
            } else
              break;
          } else
            break;
        } else {
          // there is no WiFi
          ibox.message("", "No WiFi network found!");
          break;
        }
        WiFi.scanDelete();

      } while (!wifiConnected);

    }

  }

  return wifiConnected;
}


// handle soft restart
void shutdownHandler()
{
  // save current datetime into Preferences
  gettimeofday(&savedTimeValue, nullptr);
}


void updateDateTime()
{
  // Set timezone
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  // get datetime from savedTimeValue? (noinit section)
  if (esp_reset_reason() == ESP_RST_SW) {
    // adjust time taking account elapsed time since ESP32 started
    savedTimeValue.tv_usec += (int) esp_timer_get_time();
    savedTimeValue.tv_sec  += savedTimeValue.tv_usec / 1000000;
    savedTimeValue.tv_usec %= 1000000;
    settimeofday(&savedTimeValue, nullptr);
    return;
  }

  if (checkWiFi()) {
    // we need time right now
    ibox.progressBox("", nullptr, true, 200, [&](fabgl::ProgressForm * form) {
      sntp_setoperatingmode(SNTP_OPMODE_POLL);
      sntp_setservername(0, (char*)"pool.ntp.org");
      sntp_init();
      for (int i = 0; i < 12 && sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED; ++i) {
        form->update(i * 100 / 12, "Getting date-time from SNTP...");
        delay(500);
      }
      sntp_stop();
      
      ibox.setAutoOK(2);
      ibox.message("", "Date and Time updated. Restarting...");
      #ifndef FABGL_EMULATED
      esp_restart();
      #endif
    });

  } else {
    // set default time
    auto tm = (struct tm){ .tm_sec  = 0, .tm_min  = 0, .tm_hour = 8, .tm_mday = 14, .tm_mon  = 7, .tm_year = 84 };
    auto now = (timeval){ .tv_sec = mktime(&tm) };
    settimeofday(&now, nullptr);

  }
}


// Auto-detect disk images on SD card
// Returns true if at least one bootable disk (A or C) exists.
// For each drive we first look for a corresponding TXT file (A.txt, B.txt, ...)
// that contains the image filename to load. If not present we fall back to A.img, etc.
bool autoDetectDisks(char const ** diskFilename)
{
  FileBrowser fb(SD_MOUNT_PATH);
  // Disk image filenames to check (fallback)
  const char * diskNames[DISKCOUNT] = { "A.img", "B.img", "C.img", "D.img" };
  const char * txtNames[DISKCOUNT]  = { "A.txt", "B.txt", "C.txt", "D.txt" };

  static char foundNamesBuf[DISKCOUNT][256];

  bool hasBootableDisk = false;

  for (int i = 0; i < DISKCOUNT; ++i) {
    diskFilename[i] = nullptr;
    foundNamesBuf[i][0] = 0;

    // Check for TXT file (preferred)
    if (fb.exists(txtNames[i], false)) {
      FILE * f = fb.openFile(txtNames[i], "r");
      if (f) {
        char buf[256];
        if (fgets(buf, sizeof(buf), f)) {
          // trim leading/trailing whitespace
          char * s = buf;
          while (*s && isspace((unsigned char)*s)) ++s;
          char * e = s + strlen(s) - 1;
          while (e >= s && isspace((unsigned char)*e)) { *e = 0; --e; }
          // remove surrounding quotes if present
          if (s[0] == '"' && s[strlen(s) - 1] == '"') {
            s[strlen(s) - 1] = 0;
            ++s;
          }
          // Accept shell-escaped filenames in TXT files, eg:
          //   COMPAQ\ -\ DOS\ Version\ 1.10.img
          // and convert them to literal SD filenames.
          char normalized[256];
          int w = 0;
          for (int r = 0; s[r] && w < (int)sizeof(normalized) - 1; ++r) {
            if (s[r] == '\\' && s[r + 1])
              ++r;
            normalized[w++] = s[r];
          }
          normalized[w] = 0;

          // Normalize optional leading '/': FileBrowser expects SD-root relative path.
          char * normalizedPath = normalized;
          while (*normalizedPath == '/')
            ++normalizedPath;

          // Use TXT value only if target exists.
          if (normalizedPath[0] && fb.exists(normalizedPath, false)) {
            strncpy(foundNamesBuf[i], normalizedPath, sizeof(foundNamesBuf[i]) - 1);
            foundNamesBuf[i][sizeof(foundNamesBuf[i]) - 1] = 0;
            diskFilename[i] = foundNamesBuf[i];
            if (i == 0 || i == 2) hasBootableDisk = true;
          } else if (fb.exists(diskNames[i], false)) {
            // TXT present but invalid/stale: fallback to canonical A.img/B.img/C.img/D.img
            diskFilename[i] = diskNames[i];
            if (i == 0 || i == 2) hasBootableDisk = true;
          }
        }
        fclose(f);
      }

    // Fallback: look for A.img / B.img / C.img / D.img
    } else if (fb.exists(diskNames[i], false)) {
      diskFilename[i] = diskNames[i];
      if (i == 0 || i == 2) hasBootableDisk = true;
    }
  }

  return hasBootableDisk;
}


// user pressed SYSREQ (ALT + PRINTSCREEN)
void sysReqCallback()
{
  machine->graphicsAdapter()->enableVideo(false);
  ibox.begin(VGA_640x480_60Hz, 500, 400, 4);

  int s = ibox.menu("", "Select a command", "Restart Emulator;Restart Machine;Mount Disk;Continue");
  switch (s) {

    // Restart Emulator
    case 0:
      esp_restart();
      break;
      
    // Restart Machine
    case 1:
      machine->trigReset();
      break;

    // Mount Disk
    case 2:
    {
      int s = ibox.menu("", "Select Drive", "Floppy A (fd0);Floppy B (fd1);Hard Disk C (hd0);Hard Disk D (hd1)");
      if (s > -1) {
        constexpr int MAXNAMELEN = 256;
        unique_ptr<char[]> dir(new char[MAXNAMELEN + 1] { '/', 'S', 'D', 0 } );
        unique_ptr<char[]> filename(new char[MAXNAMELEN + 1] { 0 } );
        if (machine->diskFilename(s))
          strcpy(filename.get(), machine->diskFilename(s));
        if (ibox.fileSelector("Select Disk Image", "Image Filename", dir.get(), MAXNAMELEN, filename.get(), MAXNAMELEN) == InputResult::Enter) {
          machine->setDriveImage(s, filename.get());
        }
      }
      break;
    }

    // Continue
    default:
      break;
  }

  ibox.end();
  PS2Controller::keyboard()->enableVirtualKeys(false, false); // don't use virtual keys
  machine->graphicsAdapter()->enableVideo(true);
}


void setup()
{
  Serial.begin(115200); delay(500); printf("\n\n\nReset\n\n");// DEBUG ONLY


  disableCore0WDT();
  delay(100); // experienced crashes without this delay!
  disableCore1WDT();

  preferences.begin("PCEmulator", false);

  // uncomment to clear preferences
  //preferences.clear();
  
  // save some space reducing UI queue
  fabgl::BitmappedDisplayController::queueSize = 128;

  ibox.begin(VGA_640x480_60Hz, 500, 400, 4);
  ibox.setBackgroundColor(RGB888(0, 0, 0));

  ibox.onPaint = [&](Canvas * canvas) { drawInfo(canvas); };

  // we need PSRAM for this app, but we will handle it manually, so please DO NOT enable PSRAM on your development env
  #ifdef BOARD_HAS_PSRAM
  ibox.message("Warning!", "Please disable PSRAM to improve performance!");
  #endif

  // note: we use just 2MB of PSRAM so the infamous PSRAM bug should not happen. But to avoid gcc compiler hack (-mfix-esp32-psram-cache-issue)
  // we enable PSRAM at runtime, otherwise the hack slows down CPU too much (PSRAM_HACK is no more required).
  if (esp_spiram_init() != ESP_OK)
    ibox.message("Error!", "This app requires a board with PSRAM!", nullptr, nullptr);

  #ifndef BOARD_HAS_PSRAM
  esp_spiram_init_cache();
  #endif

  if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 8))   // @TODO: reduce to 4?
    ibox.message("Error!", "This app requires a SD-CARD!", nullptr, nullptr);

  // uncomment to format SD!
  //FileBrowser::format(fabgl::DriveType::SDCard, 0);

  esp_register_shutdown_handler(shutdownHandler);

  updateDateTime();

  // Auto-detect disk images
  char const * diskFilename[DISKCOUNT];
  bool hasBootableDisk = autoDetectDisks(diskFilename);

  if (!hasBootableDisk) {
#if (defined(BIOS_VARIANT) && (BIOS_VARIANT == BIOS_VARIANT_IBMPC)) && defined(BIOS_HAS_ROM_BASIC) && BIOS_HAS_ROM_BASIC
    // IBM PC sin disco: arrancar ROM BASIC via INT 18h.
    ibox.setAutoOK(3);
    ibox.message("No disk found", "No bootable disk.\nStarting ROM BASIC...");
#else
    // No bootable disks found
    ibox.message("Error!", "No bootable disk images found!\n\nPlace A.img (floppy) or C.img (HDD)\non the SD card root.", nullptr, nullptr);
    
    // Show detected disks for debugging
    String msg = "Detected disks:\n";
    const char * diskLabels[DISKCOUNT] = { "A: (fd0)", "B: (fd1)", "C: (hd0)", "D: (hd1)" };
    for (int i = 0; i < DISKCOUNT; ++i) {
      msg += diskLabels[i];
      msg += ": ";
      msg += diskFilename[i] ? diskFilename[i] : "Not found";
      msg += "\n";
    }
    ibox.message("Disk Detection", msg.c_str());
    
    esp_restart();
#endif
  }

  // Show which disks were found
  String foundDisks = "Found disk images:\n";
  const char * diskLabels2[DISKCOUNT] = { "A: (fd0)", "B: (fd1)", "C: (hd0)", "D: (hd1)" };
  int foundCount = 0;
  for (int i = 0; i < DISKCOUNT; ++i) {
    if (diskFilename[i]) {
      foundDisks += diskLabels2[i];
      foundDisks += " - ";
      foundDisks += diskFilename[i];
      foundDisks += "\n";
      foundCount++;
    }
  }
  
  if (foundCount > 0) {
    ibox.setAutoOK(3);  // Auto-close after 3 seconds
    ibox.message("Disk Images", foundDisks.c_str());
  }

  ibox.end();
  
  // Set SD card to maximum speed
  FileBrowser::setSDCardMaxFreqKHz(SDMMC_FREQ_DEFAULT);
  FileBrowser::remountSDCard();

  machine = new Machine;

  machine->setBaseDirectory(SD_MOUNT_PATH);
  
  // Mount detected disks with auto geometry detection
  for (int i = 0; i < DISKCOUNT; ++i) {
    if (diskFilename[i]) {
      machine->setDriveImage(i, diskFilename[i], 0, 0, 0);  // Auto-detect geometry
    }
  }

  // Set boot drive: prefer C: (hd0) if available, otherwise A: (fd0)
  uint8_t bootDrive = diskFilename[2] ? 2 : 0;  // 2=hd0 (C:), 0=fd0 (A:)
  machine->setBootDrive(bootDrive);
  
  auto serial1 = new SerialPort;
  serial1->setSignals(UART_URX, UART_UTX);
  machine->setCOM1(serial1);

  /*
  printf("MALLOC_CAP_32BIT : %d bytes (largest %d bytes)\r\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
  printf("MALLOC_CAP_8BIT  : %d bytes (largest %d bytes)\r\n", heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  printf("MALLOC_CAP_DMA   : %d bytes (largest %d bytes)\r\n\n", heap_caps_get_free_size(MALLOC_CAP_DMA), heap_caps_get_largest_free_block(MALLOC_CAP_DMA));

  heap_caps_dump_all();
  */

  machine->setSysReqCallback(sysReqCallback);

  machine->run();
}



#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
namespace fabgl {
  extern volatile uint64_t s_vgapalctrlcycles;
}
using fabgl::s_vgapalctrlcycles;
#endif



void loop()
{

#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
  static uint32_t tcpu = 0, s1 = 0, count = 0;
  tcpu = machine->ticksCounter();
  s_vgapalctrlcycles = 0;
  s1 = fabgl::getCycleCount();
  delay(1000);
  printf("%d\tCPU: %d", count, machine->ticksCounter() - tcpu);
  printf("   Graph: %lld / %d   (%d%%)\n", s_vgapalctrlcycles, fabgl::getCycleCount() - s1, (int)((double)s_vgapalctrlcycles/240000000*100));
  ++count;
#else

  vTaskDelete(NULL);

#endif
}
