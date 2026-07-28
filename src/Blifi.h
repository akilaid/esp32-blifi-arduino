/**
 * @file Blifi.h
 * @brief Arduino wrapper for the blifi BLE Wi-Fi provisioning component.
 *
 * A thin C++ layer over the `blifi` ESP-IDF component (vendored under src/blifi):
 * it turns the component's esp_event callbacks into Arduino std::function
 * callbacks. No provisioning/crypto/BLE logic is duplicated here.
 *
 * Requires the arduino-esp32 core 3.x. Usage:
 *
 *   #include <Blifi.h>
 *   void setup() {
 *     Serial.begin(115200);
 *     Blifi.onProvisioned([](IPAddress ip){ Serial.println(ip); });
 *     Blifi.begin();
 *   }
 *
 * Note: the reset-pin (bootloader) hard reset is NOT available in the Arduino IDE
 * build - see the README. wasHardReset() therefore returns false here; the
 * software resetCredentials() works everywhere.
 */
#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#include <functional>

extern "C" {
// Angle brackets (not quotes) so the preprocessor skips this file's own
// directory - on case-insensitive filesystems `"blifi.h"` would otherwise
// collide with this wrapper's `Blifi.h`. Resolves to the blifi component's header.
#include <blifi.h>
}

/** Optional hard-reset indicator pin (see the README). Only takes effect when the
 *  component is built with `CONFIG_BLIFI_RESET_INDICATOR_ENABLE=y` in
 *  `sdkconfig.defaults`; these fields then override the Kconfig defaults. */
struct BlifiResetIndicator {
  bool enable = false;      ///< drive the pin on a hard reset
  int gpio = -1;            ///< output GPIO
  int activeLevel = HIGH;   ///< asserted level (HIGH or LOW)
  uint32_t pulseMs = 2000;  ///< assert duration; 0 = hold until re-provisioned
};

/** Config for `Blifi.begin(config)`. */
struct BlifiConfig {
  const char *deviceName = nullptr;   ///< BLE name; null → auto "blifi-XXXX"
  const char *pop = nullptr;          ///< fixed PoP: exactly 8 Crockford base32
                                      ///< chars (0-9, A-Z minus I/L/O/U); null =
                                      ///< auto-generate. Invalid → begin() fails.
  BlifiResetIndicator resetIndicator; ///< optional hard-reset indicator pin
  bool stopBleAfterProvisioning = false; ///< tear BLE down once provisioning
                                      ///< succeeds and the phone has the IP
                                      ///< (frees RAM, closes the attack surface).
};

class BlifiClass {
 public:
  /** Initialise + start provisioning. Uses the auto "blifi-XXXX" BLE name. */
  bool begin();
  /** Initialise + start with a custom BLE device name. */
  bool begin(const char *deviceName);
  /** Initialise + start with full config (device name + hard-reset indicator). */
  bool begin(const BlifiConfig &config);

  /** Called on every status change (connecting, connected, errors…). */
  void onStatusChanged(std::function<void(blifi_status_t)> cb);
  /** Called once the device is online, with its IP address. */
  void onProvisioned(std::function<void(IPAddress)> cb);
  /** Called on the boot after a hard reset so you can wipe your own data.
   *  NOTE: app-side hard-reset detection does NOT fire on this Arduino/PlatformIO
   *  build (the bootloader still erases credentials; only the standalone ESP-IDF
   *  build detects it in-app). See the README. */
  void onDataResetRequested(std::function<void()> cb);

  /** True if this boot followed a reset-pin hard reset. Always false on the
   *  Arduino/PlatformIO build (app-side detection is unavailable there); works
   *  only on a standalone ESP-IDF build. */
  bool wasHardReset();
  /** True if Wi-Fi credentials are stored. */
  bool isProvisioned();
  /** Erase stored credentials and return to provisioning (software reset). */
  void resetCredentials();
  /** Tear the BLE stack down (stop advertising, disconnect, free the NimBLE host
   *  RAM). Safe to call anytime; a later resetCredentials() brings BLE back. */
  void stopBle();
  /** The device's Proof-of-Possession string (show it to the user). */
  const char *pop();
  /** Human-readable name for a status code (for logs). */
  const char *statusString(blifi_status_t status);

 private:
  bool beginCfg(const blifi_config_t &cfg);
  static void eventHandler(void *arg, esp_event_base_t base, int32_t id, void *data);
  static void dataResetCb(void *arg);

  std::function<void(blifi_status_t)> _onStatus;
  std::function<void(IPAddress)> _onProvisioned;
  std::function<void()> _onDataReset;
  bool _started = false;
};

/** Global instance, à la `WiFi` / `Serial`. */
extern BlifiClass Blifi;
