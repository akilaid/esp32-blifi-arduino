<!-- GENERATED FILE — do not edit in the mirror repo. -->
> ⚙️ **Automatically generated mirror.** This repository is a flat mirror of
> [`arduino/Blifi/`](https://github.com/akilaid/esp32-blifi/tree/main/arduino/Blifi)
> in [`akilaid/esp32-blifi`](https://github.com/akilaid/esp32-blifi), published at the
> repo root so the **Arduino Library Manager** can index it. It is regenerated on
> every change — **do not edit here**; open issues and pull requests in the source
> repository instead.

---

# Blifi — Arduino-style Wi-Fi provisioning for ESP32

Provision an ESP32's Wi-Fi credentials over an encrypted **BLE** session (no
hotspot or captive portal), from a ~10-line sketch. `Blifi` is a thin C++ wrapper
over the [`blifi`](../../firmware/components/blifi) ESP-IDF component — it turns
the component's events into friendly Arduino callbacks. No provisioning/crypto/BLE
logic is duplicated here.

```cpp
#include "Blifi.h"

void setup() {
  Serial.begin(115200);
  Blifi.onProvisioned([](IPAddress ip) {
    Serial.print("Online! IP: ");
    Serial.println(ip);
  });
  Blifi.begin();
  Serial.print("PoP: ");
  Serial.println(Blifi.pop());   // show this to whoever provisions the device
}
void loop() {}
```

## Why PlatformIO (and not the plain Arduino IDE)?

The `blifi` component uses **NimBLE**, but the Arduino IDE's stock ESP32 core is
built with **Bluedroid** and ships a *precompiled* ESP-IDF — so it can neither
build an external IDF component nor switch the BLE stack. This project therefore
builds through **PlatformIO** with `framework = arduino, espidf`, which compiles
ESP-IDF (and the `blifi` component) from source with NimBLE enabled. You still
write ordinary Arduino code (`setup()`/`loop()`, `Serial`, `IPAddress`).

## Build & upload

1. Install **VS Code** + the **PlatformIO** extension.
2. Open this folder (`arduino/Blifi`) as a PlatformIO project.
3. Click **Upload** (or `pio run -t upload`). The first build downloads the
   toolchain and compiles ESP-IDF from source — it takes a few minutes; later
   builds are fast.
4. Open the **Serial Monitor** at 115200. It prints the Proof-of-Possession.
5. Provision with the blifi phone app (connect → enter the PoP → pick your
   network). The monitor prints `Online! IP: …`.

The `blifi` component is pulled automatically from the monorepo via
`src/idf_component.yml` (a relative path — no symlinks, works on any OS).

## Examples

Three sketches under [`examples/`](examples), smallest first — copy one into
`src/main.cpp` (or point `src_dir` at it) when using this folder as a
PlatformIO project:

- [`Basic`](examples/Basic) — provision + print the PoP, ~10 lines.
- [`StatusCallbacks`](examples/StatusCallbacks) — named device, live status
  stream, `isProvisioned()` branching.
- [`FullConfig`](examples/FullConfig) — `BlifiConfig` (device name + indicator
  pin), hard-reset hooks, and a serial command for the software reset.

## API

| Call | Purpose |
|------|---------|
| `Blifi.begin()` / `Blifi.begin("my-name")` | Initialise + start provisioning (optional BLE name). |
| `Blifi.onProvisioned(cb)` | `cb(IPAddress ip)` when the device comes online. |
| `Blifi.onStatusChanged(cb)` | `cb(blifi_status_t)` on every status change. |
| `Blifi.isProvisioned()` | Whether Wi-Fi credentials are stored. |
| `Blifi.resetCredentials()` | Forget Wi-Fi and re-enter provisioning (software reset). |
| `Blifi.pop()` | The device's Proof-of-Possession string. |
| `Blifi.statusString(s)` | Human-readable name for a status code. |
| `Blifi.onDataResetRequested(cb)` / `Blifi.wasHardReset()` | Reset-pin hard-reset hooks — see below. |

## Reset-pin hard reset (enabled)

This project **enables** the bootloader factory reset (something the plain Arduino
IDE can't do). Hold **GPIO13 (D13) → GND for 3 seconds** as the board powers on and
the bootloader erases the `blifi_nvs` partition — clearing the Wi-Fi credentials
before the app even starts. The **PoP survives** (it lives in the default `nvs`
partition), so a printed QR/sticker keeps working, and the device returns to
provisioning.

> **Known limitation on this build:** the bootloader *erase* works, but the
> **app-side detection does not fire** here — `wasHardReset()` stays false,
> `onDataResetRequested()` and the `HARD_RESET_TRIGGERED` event don't run, and the
> §6.2 indicator pin won't light. On the Arduino/PlatformIO (ESP-IDF 5.5) stack the
> bootloader's RTC-retain flag is clobbered before the app reads it; the full
> detection chain works only when the `blifi` component is built with a standalone
> **ESP-IDF (`idf.py`)** project. Use `Blifi.resetCredentials()` (software) for an
> in-code "forget Wi-Fi" that works regardless.

Configured in `sdkconfig.defaults` (`CONFIG_BOOTLOADER_FACTORY_RESET`, GPIO13, low,
3 s, erase `blifi_nvs`) + the `blifi_nvs` row in `partitions.csv`. To change the pin
or hold time, edit those and **rebuild clean** (`pio run -t fullclean` first —
linker/memory changes need a clean build). Design details:
[component hard-reset guide](../../firmware/components/blifi/README.md).

## Hard-reset indicator pin (optional)

Drive a GPIO active when a hard reset is detected (an LED / relay / optocoupler
"credentials were wiped" signal). Two steps:

1. Compile the feature in — add to `sdkconfig.defaults` (it's a Kconfig option, so
   it can't be set from `begin()` alone) and rebuild clean:
   ```
   CONFIG_BLIFI_RESET_INDICATOR_ENABLE=y
   ```
2. Set the pin/level/pulse at runtime in the sketch:
   ```cpp
   BlifiConfig cfg;
   cfg.resetIndicator.enable = true;
   cfg.resetIndicator.gpio = 2;          // onboard LED on many boards
   cfg.resetIndicator.activeLevel = HIGH;
   cfg.resetIndicator.pulseMs = 2000;    // 0 = hold until re-provisioned
   Blifi.begin(cfg);
   ```
   (Or set `CONFIG_BLIFI_RESET_INDICATOR_GPIO` / `_ACTIVE_*` / `_PULSE_MS` in
   `sdkconfig.defaults` instead of the runtime fields.)

Fully opt-in — with `CONFIG_BLIFI_RESET_INDICATOR_ENABLE` off (default) it compiles
out.

> Like the other app-side hard-reset hooks, the indicator fires only on a
> standalone **ESP-IDF** build; app-side detection is unavailable on the
> Arduino/PlatformIO stack (see the limitation above).

## Notes

- Requires the **pioarduino** platform (arduino-esp32 3.x / ESP-IDF 5.5), pinned
  in `platformio.ini`. The blifi component (written for ESP-IDF 6.0) builds
  unchanged on 5.5.
- Board defaults to `esp32dev` (classic ESP32); adjust `board` in
  `platformio.ini` for other modules.
