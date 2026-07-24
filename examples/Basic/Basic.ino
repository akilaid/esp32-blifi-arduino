/*
 * Basic — the smallest Blifi sketch: provision an ESP32's Wi-Fi over BLE.
 *
 * Upload, open the Serial Monitor (115200), and note the printed
 * Proof-of-Possession (PoP). In the blifi phone app: connect to the device,
 * enter the PoP, pick your Wi-Fi network. The monitor prints "Online!" with
 * the IP. Credentials persist — on later boots the device reconnects itself.
 *
 * NOTE: this library builds with PlatformIO (framework = arduino, espidf),
 * not the stock Arduino IDE — see the library README for why.
 */
#include <Arduino.h>
#include "Blifi.h"

void setup() {
  Serial.begin(115200);

  Blifi.onProvisioned([](IPAddress ip) {
    Serial.print("Online! IP: ");
    Serial.println(ip);
  });

  Blifi.begin();  // advertises as "blifi-XXXX" until provisioned

  Serial.print("PoP: ");
  Serial.println(Blifi.pop());  // show this to whoever provisions the device
}

void loop() {
  delay(1000);  // blifi runs in the background
}
