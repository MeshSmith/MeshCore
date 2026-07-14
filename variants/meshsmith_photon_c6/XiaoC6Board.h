#pragma once

#include <Arduino.h>
#include <helpers/ESP32Board.h>

class XiaoC6Board : public ESP32Board {
  uint16_t _cached_batt_millivolts = 0;
  float _cached_batt_charge_rate_pct_per_hour = NAN;
  bool _fuel_gauge_detected = false;

public:
  void begin() {
    ESP32Board::begin();

#ifdef USE_XIAO_ESP32C6_EXTERNAL_ANTENNA
// Connect an external antenna to your XIAO ESP32C6 otherwise, it may be damaged!
    pinMode(3, OUTPUT);
    digitalWrite(3, LOW); // Activate RF switch control

    delay(100);

    pinMode(14, OUTPUT);
    digitalWrite(14, HIGH); // Use external antenna
#endif
  }

  void setFuelGaugeReading(uint16_t millivolts, float charge_rate_pct_per_hour, bool detected) {
    _cached_batt_millivolts = millivolts;
    _cached_batt_charge_rate_pct_per_hour = charge_rate_pct_per_hour;
    _fuel_gauge_detected = detected;
  }

  bool hasFuelGauge() const {
    return _fuel_gauge_detected;
  }

  uint16_t getBattMilliVolts() override {
    return _cached_batt_millivolts;
  }

  float getBattChargeRatePctPerHour() {
    return _cached_batt_charge_rate_pct_per_hour;
  }

  const char* getManufacturerName() const override {
    return "Xiao C6";
  }
};


