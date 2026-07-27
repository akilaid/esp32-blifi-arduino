/**
 * @file Blifi.cpp
 * @brief Implementation of the Arduino Blifi wrapper (see Blifi.h).
 */
#include "Blifi.h"

extern "C" {
#include "nvs_flash.h"
}

// Tell the Arduino core that Bluetooth IS in use. blifi uses the ESP-IDF NimBLE
// stack directly (not Arduino's BLE library), so Arduino's bleInUse() is false
// and initArduino() would otherwise call btMemRelease(BT_MODE_BLE) at startup -
// freeing the BLE controller memory and making nimble_port_init() fail with
// ESP_ERR_INVALID_STATE. Overriding this weak symbol keeps the BLE memory.
extern "C" bool btInUse() { return true; }

BlifiClass Blifi;

bool BlifiClass::begin() {
  blifi_config_t cfg = BLIFI_DEFAULT_CONFIG();
  return beginCfg(cfg);
}

bool BlifiClass::begin(const char *deviceName) {
  blifi_config_t cfg = BLIFI_DEFAULT_CONFIG();
  if (deviceName) cfg.device_name = deviceName;
  return beginCfg(cfg);
}

bool BlifiClass::begin(const BlifiConfig &config) {
  blifi_config_t cfg = BLIFI_DEFAULT_CONFIG();
  cfg.device_name = config.deviceName;
  // Only override when set, so a build-time CONFIG_BLIFI_FIXED_POP default is
  // not clobbered by the (unset) default nullptr.
  if (config.pop) cfg.fixed_pop = config.pop;
  cfg.reset_indicator.enable       = config.resetIndicator.enable;
  cfg.reset_indicator.gpio         = (int8_t)config.resetIndicator.gpio;
  cfg.reset_indicator.active_level = (uint8_t)(config.resetIndicator.activeLevel ? 1 : 0);
  cfg.reset_indicator.pulse_ms     = config.resetIndicator.pulseMs;
  return beginCfg(cfg);
}

bool BlifiClass::beginCfg(const blifi_config_t &cfg) {
  if (_started) return true;

  // The Arduino core normally initialises NVS already; be defensive here and
  // tell blifi_init not to touch it again (manage_nvs = false).
  esp_err_t nerr = nvs_flash_init();
  if (nerr == ESP_ERR_NVS_NO_FREE_PAGES || nerr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }
  blifi_config_t c = cfg;
  c.manage_nvs = false;

  // Register the data-reset trampoline BEFORE blifi_init (init caches the flag).
  blifi_register_data_reset_callback(&BlifiClass::dataResetCb, this);

  if (blifi_init(&c) != ESP_OK) return false;

  esp_event_handler_instance_register(BLIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &BlifiClass::eventHandler, this, nullptr);

  if (blifi_start() != ESP_OK) return false;
  _started = true;
  return true;
}

void BlifiClass::onStatusChanged(std::function<void(blifi_status_t)> cb) { _onStatus = cb; }
void BlifiClass::onProvisioned(std::function<void(IPAddress)> cb) { _onProvisioned = cb; }
void BlifiClass::onDataResetRequested(std::function<void()> cb) { _onDataReset = cb; }

bool BlifiClass::wasHardReset() { return blifi_was_hard_reset(); }
bool BlifiClass::isProvisioned() { return blifi_is_provisioned(); }
void BlifiClass::resetCredentials() { blifi_reset_credentials(); }
const char *BlifiClass::pop() { return blifi_get_pop(); }
const char *BlifiClass::statusString(blifi_status_t status) { return blifi_status_str(status); }

void BlifiClass::eventHandler(void *arg, esp_event_base_t base, int32_t id, void *data) {
  (void)base;
  BlifiClass *self = static_cast<BlifiClass *>(arg);
  const blifi_event_data_t *e = static_cast<const blifi_event_data_t *>(data);
  if (e && self->_onStatus) self->_onStatus(e->status);
  if (id == BLIFI_EVENT_WIFI_CONNECTED && e && self->_onProvisioned) {
    self->_onProvisioned(IPAddress(e->ip.addr));
  }
}

void BlifiClass::dataResetCb(void *arg) {
  BlifiClass *self = static_cast<BlifiClass *>(arg);
  if (self->_onDataReset) self->_onDataReset();
}
