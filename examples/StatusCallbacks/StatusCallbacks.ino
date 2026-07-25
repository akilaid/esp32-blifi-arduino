/*
 * StatusCallbacks - a named device with full status reporting.
 *
 * Adds to Basic:
 *  - a custom BLE device name (shows up in the phone app's scan list)
 *  - onStatusChanged: every provisioning/Wi-Fi state change, as it happens
 *    (advertising, BLE connected, credentials received, connecting, online,
 *    auth failure, ...)
 *  - isProvisioned(): branch your startup logic on whether credentials exist
 *
 * Use this pattern when your device has a display or log where provisioning
 * progress should be visible.
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
    // Start your network code here (MQTT connect, HTTP server, OTA, ...).
  });

  Blifi.begin("my-sensor");  // BLE name shown in the app

  if (Blifi.isProvisioned()) {
    Serial.println("Credentials stored - connecting to Wi-Fi...");
  } else {
    Serial.print("Not provisioned - connect with the app. PoP: ");
    Serial.println(Blifi.pop());
  }
}

void loop() {
  delay(1000);
}
