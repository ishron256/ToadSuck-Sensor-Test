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

## Hardware review (from latest PCB file SmartVent_V3.kicad_pcb, 2026-07-01)
- Source of truth: `Hardware/kicad/SmartVent_V3.kicad_pcb` (newest). The `_netlist_0522.net` is STALE (superseded). Original KiCad project folder: `C:\Users\ishro\Documents\GitHub\TSI_SmartVent_V3_May26\SmartVent_V3`. kicad-cli is not installed.
- CONFIRMED fans: GPIO13 -> R13 series -> Q2 (2N7002K) -> R6 -> Q1 (P-FET) gate, R5 pulls gate to +12V, R14 10k pull-down on GPIO13. GPIO14 -> R7 -> Q3 -> R16 -> Q4 gate, R4 pull-up to +12V, R8 pull-down. High = fan on, boot/reset = off. Fan 1 out J17, Fan 2 out J15.
- CONFIRMED GPIO4 VBUS divider: R23 (VBUS) / R24 (GND).
- CONFIRMED GPIO5 = "Snsr 2 Data" to J5.3 (spare).
- CONFIRMED SPI bus (PCB): GPIO35 MOSI -> SD J3.3 (R19 pull-up); GPIO37 MISO -> SD J3.7 + BME280 SDO (R21 pull-up); GPIO36 -> SD CLK J3.5 (R20/R30 pull-ups) and via R25 100R to net SPI_SCK (BME280 SCK + J8.3).
- CONFIRMED chip selects: BME280 (U3) CSB = GPIO10 (net SPI_CS, also J8.5). SD card CS (J3.2, R10 pull-up) = GPIO15, NOT GPIO10. config.h updated (PIN_SD_CS=15, PIN_BME280_CS=10). BME280 is on SPI, not I2C.
- J8 "Snsr Conn" (6-pin external sensor header): 1=3.3V, 2=MISO, 3=SCK, 4=Net-(U3-SDI), 5=CS, 6=GND.
- RESOLVED (owner confirmed): BME280 SDI (U3.3) is on J8.4, which a cable connects to J6.4 = SPI_MOSI (GPIO35). J6 "Snsr Conn" pinout: 1=3.3V, 2=MISO, 3=SCK, 4=MOSI, 5=CS, 6=GND. So the BME280 is on a separate sensor board/cable (J8 side) and needs that cable fitted.
- Owner confirmed: SD CLK net pull-ups (R20/R30) and R25 100R isolation are intentional.
- J4 (GPIO46 strapping select): J4.1 -> 3.3V via R15, J4.2 -> GPIO46 (R26 pull-down), J4.3 -> GND. Pins 2-3 shorted = GPIO46 LOW (use for USB flashing; TRM lists GPIO0=0 + GPIO46=1 as invalid). Pins 1-2 shorted = GPIO46 HIGH. Recommended: keep 2-3 (LOW) always. Owner said "jumper 1&2 for low" - this contradicts the netlist; pins 1-2 give HIGH. CONFIRM before flashing.
- GPIO0 is unconnected on the PCB (internal pull-up); EN (pin 3) is a separate reset switch S2. USB flashing relies on the S3 USB-Serial-JTAG auto-reset (untested).
- AP security: prototype, no Internet, owner accepts hard-coded password for now.
- Unused GPIOs left unconnected: 0,1,2,3,6,7,8,9,11,12,18,21,38-42,43,44,45,47,48.
- GPIO16/GPIO17 drive LEDs D5/D9.

## Bring-up findings (2026-10-25) - 3V3 RAIL FAULT (superseded below for flashing/BME280)
- No ESP32-S3 USB device ever enumerated on the PC (no VID 303A, no new COM port). Only Bluetooth COM4/COM5 plus, later, a known-good plain ESP32 dev board on COM3 (CP210x, ESP32-D0WD-V3; NOT an S3, so it cannot run this firmware). Nothing was flashed.
- Measured 3V3 rail on the PCBs: 2.5 V and 2.7 V on the first two, then 4.2 / 4.1 / 4.0 / 4.0 V on four more boards (pin 2 = OUT). U11 pin 3 (IN) = 12 V; pin 1 (ADJ) about 2.5 V. OUT-ADJ is 1.5-1.7 V, not the 1.25 V of a regulating LM317.
- U11 LM317 circuit: IN = +12V, OUT = +3.3V, R11 330 R (OUT-ADJ), R12 560 R (ADJ-GND), C6 1 uF across R11. Ideal Vout is about 3.4 V.
- Diagnosis (likely, not yet bench-confirmed):
  1. The divider draws only about 4.5 mA, below the LM317 minimum load (about 3.5 mA typ, up to 10 mA max; datasheet suggests R1 = 240 R), so the output floats high when the S3 is not drawing current. 4.2 V exceeds the S3 3.6 V absolute max.
  2. 12 V to 3.3 V drops about 8.7 V across a small SOT-223 LM317, which means 3-4 W at Wi-Fi peaks of 300-500 mA. It will overheat and sag (earlier 2.5-2.7 V readings).
- Fix options: (a) replace the LM317 with a 12 V to 3.3 V buck converter (best; a buck module on the 3V3 rail works as a bodge); (b) minimum bench fix R11 about 120 R, R12 about 200 R (about 3.33 V, about 10 mA divider current), still thermally limited, Wi-Fi off only.
- Do NOT plug an ESP32-S3 into these boards until 3V3 reads about 3.3 V. Check any parts already powered from the 4 V rail.
- Next steps once the rail is fixed: re-check J4 jumper (pins 2-3), cable J8-J6, find the COM port, flash with PlatformIO, then check BME280 over SPI (CS GPIO10, SD CS GPIO15).

## Bring-up results (2026-10-26 / 2026-11-01) - FLASHING AND EXTERNAL BME280 WORK
- USB flashing works: S3 enumerates as USB Serial/JTAG (VID 303A PID 1001), COM6 on this PC, MAC 10:20:ba:1b:1d:ec. `pio run -e esp32-s3-wroom-n4 -t upload --upload-port COM6` succeeds (18 s incremental; a first build of a new env takes 4-7 min). Flash detected 4 MB quad.
- J4 jumper was on 2-3 (GPIO46 LOW). Boots from flash, Wi-Fi AP comes up at 192.168.4.1, state machine runs STARTUP_TEST -> OUTSIDE_SETTLE.
- Reset caveat: toggling RTS from a serial tool drops the chip into download mode ("waiting for download"). Press the S2 reset switch to boot the app. The port re-enumerates on every reset, so serial capture must reopen COM6.
- ON-BOARD BME280 (U3) via J8-J6 cable: chip id reads 0x00 or 0xFF (expected 0x60), both with the SPI driver and a raw bit-banged read (`pio run -e bme280-bitbang`, src/bb_test.c). So the driver is not at fault; U3 / its path is not responding. Not yet diagnosed (check U3 placement and solder, VDD/VDDIO, CSB).
- EXTERNAL BME280 MODULE wired to J8 (1 VCC 3.3 V, 2 SDO -> GPIO37, 3 SCL via R25 -> GPIO36, 4 SDA -> GPIO35, 5 CSB -> GPIO10, 6 GND): bit-bang chip id = 0x60, status 0xF3 = 0x00. Production firmware init succeeds and logs `T=27.96 C RH=48.1 % AH=13.07 g/m3` at the end of the 4 s start-up test. SPI wiring, CS GPIO10 and the driver are validated.
- SD card: mount fails (0x108 invalid response / 0x107 timeout); CS GPIO15. Not yet checked whether a card was inserted. Non-fatal by design.
- Reminder: the 3V3 LM317 fault above still needs a real fix (buck converter); bench power used for these tests is not the final design.
- sensor.c now logs each valid reading (T, RH, AH) at INFO level.

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
- [x] Partition: single large app partition (1.5 MB) via platformio.ini `board_build.partitions = partitions_singleapp_large.csv`; build uses 58.5%. No OTA, no custom CSV needed.
- [ ] BME280 I2C pins (GPIO8/9 are placeholders in config.h).
- [x] AP password: prototype, security not a concern; keep placeholder (change later).
- [x] Delta rule CONFIRMED: fans run when outside AH is >= 0.5 g/m3 drier than inside (inside - outside >= 0.5).
- [x] Start-up Fan 1 test = 4 s (config.h).
- [x] Cold stir = 20 min (both fans). Idle recheck = 6 h (CONFIRMED, but see open question on 5-min retry).
- [x] Retry rule: if outside is wetter, state RETRY waits 5 min, re-reads, decides again; no timeout, no new 12-min settle. 6 h IDLE is now only used when a reading is invalid.
- [x] Inside/outside readings (owner confirmed): single BME280. INSIDE reading is taken at the end of a fan run (fan cavity full of inside air). OUTSIDE reading is taken after the 12-min fans-off settle. DECIDE compares the stored inside reading with the new outside reading. ASSUMPTION to confirm: the first inside reading is taken at the end of the 4 s start-up Fan 1 test (otherwise there is no inside value on a fresh reset).
- [x] BME280 driver written (src/sensor.c): own SPI driver per Bosch datasheet (Adafruit lib is Arduino-only). Shared SPI2 bus init in src/spibus.c. Builds; NOT tested on hardware (needs sensor cable J8-J6 fitted).
- [ ] Review KiCad schematic/PCB (owner to supply).

## What to bring to a new chat
- This NOTES.md and the original prompt/spec (283 lines).
- The KiCad schematic (.kicad_sch) and PCB (.kicad_pcb), or a PDF export.
- Any answers to the open items above.
- The ESP32-S3 TRM PDF only if register-level checks are needed again.

## System test (2026-11-08)
- env system-test (-DSYSTEM_TEST, 20 s settle, 60 s forced FAN_RUN) flashed to COM6.
- External BME280 on J8 read ~24.4 C / 42-44 %RH; state machine ran STARTUP_TEST > OUTSIDE_SETTLE > DECIDE > FAN_RUN; both fans confirmed running by user.
- An earlier run read T=0/RH=0 with valid=1 (module unseated, MISO low); sensor.c should reject all-zero/0xFF frames (TODO).
- On-board BME280 (U3), SD mount and LM317 3V3 rail still open.
