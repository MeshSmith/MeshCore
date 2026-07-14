#pragma once

#include <Arduino.h>
#include <MeshCore.h>
#include <helpers/NRF52Board.h>

#ifndef USER_BTN_PRESSED
#define USER_BTN_PRESSED LOW
#endif

class PhotonNrf52Board : public NRF52BoardDCDC {
  uint16_t _cached_batt_millivolts = 0;
  float _cached_batt_charge_rate_pct_per_hour = NAN;
  bool _fuel_gauge_detected = false;

protected:
#ifdef NRF52_POWER_MANAGEMENT
  void initiateShutdown(uint8_t reason) override;
#endif

public:
  PhotonNrf52Board() : NRF52Board("PHOTON_NRF_OTA") {}
  void begin();

#if defined(P_LORA_TX_LED)
  void onBeforeTransmit() override {
    digitalWrite(P_LORA_TX_LED, LOW);
  }
  void onAfterTransmit() override {
    digitalWrite(P_LORA_TX_LED, HIGH);
  }
#endif

  void setFuelGaugeReading(uint16_t millivolts, float charge_rate_pct_per_hour, bool detected) {
    _cached_batt_millivolts = millivolts;
    _cached_batt_charge_rate_pct_per_hour = charge_rate_pct_per_hour;
    _fuel_gauge_detected = detected;
  }

  bool hasFuelGauge() const {
    return _fuel_gauge_detected;
  }

  uint16_t getBattMilliVolts() override;

  float getBattChargeRatePctPerHour() {
    return _cached_batt_charge_rate_pct_per_hour;
  }

  const char* getManufacturerName() const override {
    return "Meshsmith Photon NRF52";
  }

  void powerOff() override {
    digitalWrite(PIN_LED, LOW);
#ifdef PIN_USER_BTN
    while (digitalRead(PIN_USER_BTN) == USER_BTN_PRESSED);
#endif
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(PIN_LED, HIGH);

#ifdef PIN_USER_BTN
    nrf_gpio_cfg_sense_input(digitalPinToInterrupt(g_ADigitalPinMap[PIN_USER_BTN]), NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);
#endif

    sd_power_system_off();
  }
};
