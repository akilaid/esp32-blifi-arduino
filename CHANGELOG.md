# Changelog — Blifi (Arduino library)

All notable changes to the Arduino library are documented here. Format based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); this library is
versioned independently under [Semantic Versioning](https://semver.org/).

## [Unreleased]

Nothing yet.

## [0.1.1] — 2026-07-25

### Added
- Example sketches (`examples/`): `Basic` (smallest possible sketch),
  `StatusCallbacks` (named device + full status reporting), and `FullConfig`
  (every config surface: `BlifiConfig`, hard-reset hooks, indicator pin, and a
  serial `r` command for the software credential reset).

## [0.1.0] — 2026-07-25

### Added
- Initial `Blifi` wrapper: a thin C++ Arduino API over the `blifi` ESP-IDF
  component (`begin`, `onProvisioned`, `onStatusChanged`, `isProvisioned`,
  `resetCredentials`, `pop`, `statusString`, plus `onDataResetRequested` /
  `wasHardReset` hooks). Public header `Blifi.h`; global `Blifi` instance.
- Ships as a **PlatformIO** project (`framework = arduino, espidf` on the
  pioarduino platform) with `platformio.ini`, `sdkconfig.defaults` (NimBLE on),
  `partitions.csv`, and a `BasicProvisioning` sketch (`src/main.cpp`). The `blifi`
  component is pulled via a relative `src/idf_component.yml` path dependency.
- README documenting why PlatformIO is required (the stock Arduino IDE core is
  Bluedroid-only and can't build the NimBLE component or change bootloader
  config), build/upload steps, and the API.
- `Blifi.begin(BlifiConfig)` overload exposing the optional hard-reset indicator
  pin (§6.2): `resetIndicator{ enable, gpio, activeLevel, pulseMs }`. Enable the
  feature via `CONFIG_BLIFI_RESET_INDICATOR_ENABLE=y` in `sdkconfig.defaults`, then
  set the pin/level/pulse at runtime. Opt-in; off by default.

### Fixed (Arduino/ESP-IDF-5.5 integration — all in the wrapper/config, not the component)
- **BLE controller wouldn't start** (`nimble_port_init: ESP_ERR_INVALID_STATE`):
  Arduino's `initArduino()` frees the BLE controller memory at boot unless
  `bleInUse()` is true. Since blifi uses the IDF NimBLE stack directly (not
  Arduino's BLE library), the wrapper overrides the weak `btInUse()` to return
  true (`Blifi.cpp`) so the memory is kept.
- **Crash on credential receipt** (`stack overflow in task sys_evt`): blifi runs
  AES-GCM + BLE notify in the default event-loop task; raised
  `CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=6144`.
- **Task-watchdog warnings**: added a `delay()` in the sketch `loop()`.

### Notes
- Verified on hardware: full BLE provisioning (handshake → scan list → credentials
  → Wi-Fi connect → `Online!`), no crash. The IDF-6.0 `blifi` component builds
  unchanged on ESP-IDF 5.5.
- Reset-pin hard reset (bootloader erase) is **enabled** and coexists with BLE:
  hold GPIO13 → GND for 3 s at boot to erase `blifi_nvs` (PoP preserved). Configured
  via `sdkconfig.defaults` + `partitions.csv`. Note: changing linker/memory config
  (like enabling this) needs a clean rebuild (`pio run -t fullclean`).
- **Known limitation:** the bootloader erase works, but app-side hard-reset
  detection (`wasHardReset()`, `onDataResetRequested()`, the `HARD_RESET_TRIGGERED`
  event, and the §6.2 indicator pin) does **not** fire on the Arduino/PlatformIO
  (ESP-IDF 5.5) stack — the bootloader RTC-retain flag is clobbered before the app
  reads it. Those work only on a standalone ESP-IDF (`idf.py`) build. Use
  `resetCredentials()` for an in-code reset.
