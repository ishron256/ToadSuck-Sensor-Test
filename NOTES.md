# TSI SmartVent V3 - Project Notes

Handoff file between chat sessions. Update it at the end of every session.
Paste it (or tell the new chat to read it) at the start of the next one.

## Hardware
- Module: ESP32-S3-WROOM-N4 (4 MB quad flash, no PSRAM). GPIO26-32 are used by flash.
- GPIO4: VBUS sense through a divider (VBUS is 5 V, GPIO is 3.3 V only).
- SD card over SPI (not SDMMC): MISO 37, SCK 36, MOSI 35, CS 10, SPI2 host.
  - These pins go through the GPIO matrix, so the clock is capped at ~40 MHz; we use 20 MHz.
  - MISO has a pull-up (confirmed). Never call the SDMMC host.
  - GPIO35-37 are only free on N4 modules (octal PSRAM/flash modules use them).
- Fans (active-high, N-FET/P-FET stage passing 12 V): GPIO13 = Fan 1, GPIO14 = Fan 2 + external fans.
  - Fan 1 runs alone only during the start-up procedure; otherwise both run together.
  - Fan-gate pull-downs and P-FET gate pull-up are already on the production board (owner to confirm from schematic).
- GPIO5 "Snsr 2": spare, unused.
- Strapping pins to avoid: GPIO0, 3, 45, 46.

## Netlist review (SmartVent_V3_netlist_0522.net, 2026-10-24)
- WARNING: the netlist is dated 2026-05-22, but `SmartVent_V3.kicad_sch` is dated 2026-06-19 and has SPI_CS/SCK/MOSI/MISO labels the netlist lacks. The netlist is likely STALE. Findings below must be re-verified against a fresh netlist export (KiCad: File > Export > Netlist, or `kicad-cli sch export netlist`). kicad-cli is not installed on this PC.
- Hardware files are in `Hardware/kicad/` (Gerbers, netlist, BOM, PDFs, .kicad_pro). `untitled.kicad_sch` is an EMPTY stub: the real schematic is still needed.
- CONFIRMED fans: GPIO13 -> R13 series -> Q2 (2N7002K) -> R6 -> Q1 (P-FET) gate, R5 pulls gate to +12V, R14 10k pull-down on GPIO13. GPIO14 -> R7 -> Q3 -> R16 -> Q4 gate, R4 pull-up to +12V, R8 pull-down. High = fan on, boot/reset = off. Fan 1 out J17, Fan 2 out J15.
- CONFIRMED GPIO4 VBUS divider: R23 (VBUS) / R24 (GND).
- CONFIRMED GPIO5 = "Snsr 2 Data" to J5.3 (spare).
- CORRECTION: the BME280 (U3) is on SPI, not I2C. It shares the bus: CSB = GPIO10, SDI = GPIO35, SDO = GPIO37, SCK = GPIO36 via R25 100R (R30 10k pull-up to 3.3V on the SCK net).
- CORRECTION: the SD card CS (J3 DAT3) is on GPIO15, not GPIO10.
- POSSIBLE SCHEMATIC BUG (verify): net "J3-CLK" ties J3 pins 5 and 7 (CLK and DAT0) together with GPIO37 (MISO) and pull-ups R20/R21. SD CLK does not appear to connect to GPIO36. Check against the real schematic/PCB.
- J4: jumper/switch input on GPIO46 (strapping pin; R26 pull-down, R15 to 3.3V). Purpose to confirm.
- Unused GPIOs left unconnected: 0,1,2,3,6,7,8,9,11,12,18,21,38-42,43,44,45,47,48.
- GPIO16/GPIO17 drive LEDs D5/D9.

## Behaviour decisions
- Humidity unit: absolute humidity (g/m3), computed from BME280 temperature + RH (Magnus formula).
- One BME280. "Outside" reading is taken after a 12-minute fans-off settle (method inherited from the earlier public-domain SmartVent paper).
- Cold lockout: below 35 F no normal fan operation; sleep 6 h between readings; fans only "stir" the air every 6 h.
- Time: uptime since reset only. No RTC, no NTP (Wi-Fi is an AP with no upstream). "24 h" and "30 days" mean uptime.
- Persistence across power loss: nice to have, not required.
- Power-on test and 12-minute wait run on EVERY reset.
- Watchdog: ~30 s task watchdog fed by the main loop, plus a state-progress monitor.
- SD failure must never affect fan control or the web page (page served from RAM).
- 10-day chart data is kept in RAM, accumulating from start of operation.
- Wi-Fi: AP with a hard-coded password (WPA2).

## Code status
- Framework DECIDED: PlatformIO with `framework = espidf` (ESP-IDF 5.3.x via PlatformIO's espressif32 platform).
  Reason: best control of watchdog/SD/Wi-Fi AP; Arduino can be added later as a component if needed.
- Layout: `platformio.ini`, root `CMakeLists.txt`, `sdkconfig.defaults`, sources in `src/`
  (app_main.c state machine, fans, humidity, sensor = STUB, storage = SD over SPI, web = AP + page, config.h).
- BUILD VERIFIED: `pio run` succeeds. RAM 10.7%, Flash 85.7% of the 1 MB default app partition.
  - Flash is near the limit (Wi-Fi + HTTP + FATFS). Next: add a custom `partitions.csv` with a larger app partition (4 MB flash total).
  - Fix needed once on this PC: `%USERPROFILE%\.platformio\penv\Scripts\python.exe -m pip install intelhex` (already done).
  - Build command: `%USERPROFILE%\.platformio\penv\Scripts\pio.exe run` (first build ~6 min, later ~15 s).
- Not yet tested on hardware. Sensor driver is a stub, so fans never run until BME280 is implemented.
- Commits: 35eb819 ESP-IDF skeleton, 1a14b96 NOTES.md, then PlatformIO conversion.
- Do NOT use the Espressif VS Code extension build on this folder; use PlatformIO only.
- KiCad files will be added to the repo by the owner (next session: review fan gates, pull-ups, BME280 pins).
- Environment: ESP-IDF 5.3.2 is installed at `C:\Users\ishro\esp\v5.3.2\esp-idf` (VS Code Espressif extension 2.3.0). PlatformIO extension and its espressif32 platform are also installed.

## Open items
- [ ] Add custom partition table (flash is 85.7% of 1 MB app partition).
- [ ] BME280 I2C pins (GPIO8/9 are placeholders in config.h).
- [ ] AP SSID/password (placeholder in config.h).
- [ ] Confirm the delta rule: code uses fans ON when (inside AH - outside AH) >= 0.5 g/m3. Spec said "-0.5".
- [ ] Start-up Fan 1 test duration (placeholder 10 s).
- [ ] Cold-stir duration (placeholder 2 min) and idle recheck period (placeholder 6 h).
- [ ] Outside-reading retry/timeout rules and measured settle times.
- [ ] Write the BME280 driver.
- [ ] Review KiCad schematic/PCB (owner to supply).

## What to bring to a new chat
- This NOTES.md and the original prompt/spec (283 lines).
- The KiCad schematic (.kicad_sch) and PCB (.kicad_pcb), or a PDF export.
- Any answers to the open items above.
- The ESP32-S3 TRM PDF only if register-level checks are needed again.
