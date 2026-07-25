/*
 * FullConfig - every configuration surface of the library, plus a serial
 * command to forget Wi-Fi at runtime.
 *
 * Shows:
 *  - BlifiConfig: device name + the optional hard-reset indicator pin
 *  - onDataResetRequested / wasHardReset: the reset-pin hard-reset hooks
 *  - resetCredentials(): software "forget Wi-Fi" (type 'r' in the monitor)
 *
 * Reset pin: this project enables the bootloader factory reset - hold
 * GPIO13 (D13) to GND for 3 s while the board powers on and the bootloader
 * erases the Wi-Fi credentials (the PoP survives, so a printed QR keeps
 * working). Pin/hold-time live in sdkconfig.defaults, not in the sketch.
 *
 * KNOWN LIMITATION on the Arduino/PlatformIO build (see README): the
 * bootloader ERASE works, but app-side detection does not fire here -
 * wasHardReset() stays false, onDataResetRequested() doesn't run, and the
 * indicator pin won't light. They're wired up below because the same sketch
 * gains those behaviors when the blifi component is used from a standalone
 * ESP-IDF project. resetCredentials() (software) works everywhere.
 *
 * Indicator pin: also requires CONFIG_BLIFI_RESET_INDICATOR_ENABLE=y in
 * sdkconfig.defaults (a compile-time option); the fields below then override
 * the Kconfig defaults at runtime.
 */
#include <Arduino.h>
#include "Blifi.h"

void setup() {
  Serial.begin(115200);

  Blifi.onStatusChanged([](blifi_status_t s) {
    Serial.print("[blifi] ");
    Serial.println(Blifi.statusString(s));
  });

  Blifi.onProvisioned([](IPAddress ip) {
    Serial.print("[blifi] online, IP: ");
    Serial.println(ip);
  });

  // On the boot after a reset-pin hard reset: credentials are already gone;
  // erase your app's own data (preferences, files, ...) here.
  Blifi.onDataResetRequested([]() {
    Serial.println("[blifi] hard reset - erasing app data");
    // Preferences prefs; prefs.begin("myapp"); prefs.clear(); prefs.end();
  });

  BlifiConfig cfg;
  cfg.deviceName = "my-device";
  cfg.resetIndicator.enable = true;   // needs CONFIG_BLIFI_RESET_INDICATOR_ENABLE=y
  cfg.resetIndicator.gpio = 2;        // onboard LED on many devkits
  cfg.resetIndicator.activeLevel = HIGH;
  cfg.resetIndicator.pulseMs = 2000;  // 0 = hold until re-provisioned
  Blifi.begin(cfg);

  Serial.print("Was hard reset this boot: ");
  Serial.println(Blifi.wasHardReset() ? "yes" : "no");
  Serial.print(Blifi.isProvisioned() ? "Provisioned." : "Awaiting provisioning.");
  Serial.print(" PoP: ");
  Serial.println(Blifi.pop());
  Serial.println("Type 'r' + Enter to forget Wi-Fi and re-enter provisioning.");
}

void loop() {
  if (Serial.available() && Serial.read() == 'r') {
    Serial.println("[blifi] forgetting Wi-Fi (software reset)...");
    Blifi.resetCredentials();
    Serial.print("Back in provisioning mode. PoP: ");
    Serial.println(Blifi.pop());
  }
  delay(50);
}
