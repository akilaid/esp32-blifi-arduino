/*
 * BasicProvisioning — provision an ESP32's Wi-Fi over BLE with blifi.
 *
 * 1. Open this folder in VS Code + PlatformIO and click Upload.
 * 2. Open the Serial Monitor (115200) and note the printed Proof-of-Possession.
 * 3. Open the blifi phone app, connect to this device, enter the PoP, and pick
 *    your Wi-Fi network. When it connects, the monitor prints "Online!".
 */
#include <Arduino.h>
#include "Blifi.h"

void setup() {
  Serial.begin(115200);
  delay(300);

  Blifi.onProvisioned([](IPAddress ip) {
    Serial.print("Online! IP: ");
    Serial.println(ip);
  });

  Blifi.onStatusChanged([](blifi_status_t s) {
    Serial.print("Status: ");
    Serial.println(Blifi.statusString(s));
  });

  // Fires on the boot after a reset-pin hard reset (hold GPIO13 to GND for 3 s).
  // Wi-Fi credentials are already gone; erase your own data here if needed.
  Blifi.onDataResetRequested([]() {
    Serial.println("[hard-reset] app data would be erased here");
  });

  Blifi.begin();

  Serial.print("Was hard reset: ");
  Serial.println(Blifi.wasHardReset() ? "yes" : "no");
  Serial.print("Ready to provision over BLE. PoP: ");
  Serial.println(Blifi.pop());
}

void loop() {
  // blifi runs in the background; keep the loop task idle-friendly.
  delay(1000);
}
