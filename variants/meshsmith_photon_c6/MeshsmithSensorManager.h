#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_INA219.h>
#include <Adafruit_INA260.h>
#include <Adafruit_SHTC3.h>
#include <Adafruit_VL53L0X.h>
#include <SensirionI2cSht4x.h>
#include <helpers/SensorManager.h>
#include "XiaoC6Board.h"

class MeshsmithSensorManager : public SensorManager {
  static constexpr uint8_t MAX_ACTIVE_SENSORS = 8;
  static constexpr float SEALEVEL_PRESSURE_HPA = 1013.25f;
  static constexpr uint8_t FUEL_GAUGE_I2C_ADDRESS = 0x36;
  static constexpr uint8_t FUEL_GAUGE_VCELL_REGISTER = 0x02;
  static constexpr uint8_t FUEL_GAUGE_CRATE_REGISTER = 0x16;
  static constexpr uint32_t FUEL_GAUGE_UPDATE_INTERVAL_MS = 5000;

  enum SensorKind : uint8_t {
    SENSOR_AHTX0,
    SENSOR_BME280,
    SENSOR_BMP280,
    SENSOR_SHTC3,
    SENSOR_SHT4X,
    SENSOR_INA219,
    SENSOR_INA260,
    SENSOR_VL53L0X,
  };

  struct ActiveSensor {
    SensorKind kind;
    uint8_t channel;
  };

  LocationProvider* _location;
  XiaoC6Board* _board;
  ActiveSensor _active_sensors[MAX_ACTIVE_SENSORS];
  uint8_t _active_sensor_count = 0;
  bool _gps_detected = false;
  bool _gps_active = false;
  bool _i2c_ready = false;
  bool _i2c_fault = false;
  bool _fuel_gauge_detected = false;
  uint16_t _fuel_millivolts = 0;
  float _fuel_charge_rate_pct_per_hour = NAN;
  uint32_t _gps_update_interval_sec = 1;
  char _sensor_list[96] = "none";

  Adafruit_AHTX0 _ahtx0;
  Adafruit_BME280 _bme280;
  Adafruit_BMP280 _bmp280;
  Adafruit_INA219 _ina219;
  Adafruit_INA260 _ina260;
  Adafruit_SHTC3 _shtc3;
  Adafruit_VL53L0X _vl53l0x;
  SensirionI2cSht4x _sht4x;

  void startGPS() {
    _gps_active = true;
    _location->begin();
    _location->reset();
  }

  void stopGPS() {
    _gps_active = false;
    _location->stop();
  }

  void beginGPS() {
    Serial1.setPins(PIN_GPS_TX, PIN_GPS_RX);
#ifdef GPS_BAUD_RATE
    Serial1.begin(GPS_BAUD_RATE);
#else
    Serial1.begin(9600);
#endif

    startGPS();
    delay(1000);

#ifdef ENV_SKIP_GPS_DETECT
    _gps_detected = true;
#else
    _gps_detected = Serial1.available() > 0;
#endif

    if (_gps_detected) {
#if defined(PERSISTANT_GPS) || defined(PERSISTENT_GPS)
      _gps_active = true;
#else
      stopGPS();
#endif
    } else {
      stopGPS();
    }
  }

  bool probeAddress(uint8_t address) {
    Wire.beginTransmission(address);
    uint8_t result = Wire.endTransmission();
    if (result == 0) {
      return true;
    }
    if (result == 5) {
      _i2c_fault = true;
    }
    return false;
  }

  bool readFuelGaugeRegister(uint8_t reg, uint16_t& value) {
    Wire.beginTransmission(FUEL_GAUGE_I2C_ADDRESS);
    Wire.write(reg);
    uint8_t result = Wire.endTransmission();
    if (result != 0) {
      if (result == 5) {
        _i2c_fault = true;
      }
      return false;
    }

    if (Wire.requestFrom(FUEL_GAUGE_I2C_ADDRESS, (uint8_t)2) != 2) {
      return false;
    }

    value = ((uint16_t)Wire.read() << 8) | Wire.read();
    return true;
  }

  void clearFuelGauge() {
    _fuel_gauge_detected = false;
    _fuel_millivolts = 0;
    _fuel_charge_rate_pct_per_hour = NAN;
    _board->setFuelGaugeReading(_fuel_millivolts, _fuel_charge_rate_pct_per_hour, false);
  }

  bool updateFuelGauge() {
    if (!_fuel_gauge_detected) {
      return false;
    }

    uint16_t vcell = 0;
    uint16_t crate = 0;
    bool voltage_ok = readFuelGaugeRegister(FUEL_GAUGE_VCELL_REGISTER, vcell);
    bool rate_ok = readFuelGaugeRegister(FUEL_GAUGE_CRATE_REGISTER, crate);

    if (voltage_ok) {
      // MAX17048 VCELL uses 78.125uV LSB units.
      _fuel_millivolts = (uint32_t)(vcell * 5) / 64;
    }
    if (rate_ok) {
      // MAX17048 CRATE is a signed 16-bit value with 0.208%/hr LSB units.
      _fuel_charge_rate_pct_per_hour = (float)((int16_t)crate) * 0.208f;
    }

    bool ok = voltage_ok || rate_ok;
    if (ok) {
      _board->setFuelGaugeReading(_fuel_millivolts, _fuel_charge_rate_pct_per_hour, true);
    }
    return ok;
  }

  void detectFuelGauge() {
    _fuel_gauge_detected = probeAddress(FUEL_GAUGE_I2C_ADDRESS);
    if (!_fuel_gauge_detected) {
      clearFuelGauge();
      return;
    }

    if (!updateFuelGauge()) {
      clearFuelGauge();
    }
  }

  void appendSensorName(const char* name) {
    if (_active_sensor_count == 0) {
      snprintf(_sensor_list, sizeof(_sensor_list), "%s", name);
      return;
    }

    size_t used = strlen(_sensor_list);
    if (used + 1 < sizeof(_sensor_list)) {
      _sensor_list[used++] = ',';
      _sensor_list[used] = 0;
      snprintf(_sensor_list + used, sizeof(_sensor_list) - used, "%s", name);
    }
  }

  bool activateSensor(SensorKind kind, const char* name) {
    if (_active_sensor_count >= MAX_ACTIVE_SENSORS) {
      return false;
    }

    appendSensorName(name);
    _active_sensors[_active_sensor_count] = { kind, (uint8_t)(TELEM_CHANNEL_SELF + 1 + _active_sensor_count) };
    _active_sensor_count++;
    return true;
  }

  void resetI2CSensorState() {
    _active_sensor_count = 0;
    snprintf(_sensor_list, sizeof(_sensor_list), "none");
  }

  void detectAHTX0() {
    if (probeAddress(0x38) && _ahtx0.begin(&Wire, 0, 0x38)) {
      activateSensor(SENSOR_AHTX0, "ahtx0");
    }
  }

  void detectPressureSensors() {
    const uint8_t addresses[] = { 0x76, 0x77 };
    bool bme280_found = false;
    bool bmp280_found = false;

    for (uint8_t i = 0; i < sizeof(addresses); i++) {
      uint8_t address = addresses[i];
      if (!probeAddress(address)) {
        continue;
      }

      if (!bme280_found && _bme280.begin(address, &Wire)) {
        _bme280.setSampling(Adafruit_BME280::MODE_FORCED,
                            Adafruit_BME280::SAMPLING_X1,
                            Adafruit_BME280::SAMPLING_X1,
                            Adafruit_BME280::SAMPLING_X1,
                            Adafruit_BME280::FILTER_OFF,
                            Adafruit_BME280::STANDBY_MS_1000);
        activateSensor(SENSOR_BME280, address == 0x76 ? "bme280" : "bme280@77");
        bme280_found = true;
        continue;
      }

      if (!bmp280_found && _bmp280.begin(address)) {
        activateSensor(SENSOR_BMP280, address == 0x76 ? "bmp280" : "bmp280@77");
        bmp280_found = true;
      }
    }
  }

  void detectSHTC3() {
    if (probeAddress(0x70) && _shtc3.begin(&Wire)) {
      activateSensor(SENSOR_SHTC3, "shtc3");
    }
  }

  void detectSHT4X() {
    const uint8_t addresses[] = { 0x44, 0x45 };

    for (uint8_t i = 0; i < sizeof(addresses); i++) {
      uint8_t address = addresses[i];
      if (!probeAddress(address)) {
        continue;
      }

      _sht4x.begin(Wire, address);
      uint32_t serial = 0;
      if (_sht4x.serialNumber(serial) == 0) {
        activateSensor(SENSOR_SHT4X, address == 0x44 ? "sht4x" : "sht4x@45");
        return;
      }
    }
  }

  void detectCurrentSensors() {
    const uint8_t addresses[] = { 0x40, 0x41 };
    bool ina260_found = false;
    bool ina219_found = false;

    for (uint8_t i = 0; i < sizeof(addresses); i++) {
      uint8_t address = addresses[i];
      if (!probeAddress(address)) {
        continue;
      }

      if (!ina260_found && _ina260.begin(address, &Wire)) {
        activateSensor(SENSOR_INA260, address == 0x40 ? "ina260" : "ina260@41");
        ina260_found = true;
        continue;
      }

      if (address == 0x40 && !ina219_found && _ina219.begin(&Wire)) {
        activateSensor(SENSOR_INA219, "ina219");
        ina219_found = true;
      }
    }
  }

  void detectVL53L0X() {
    if (probeAddress(0x29) && _vl53l0x.begin(0x29, false, &Wire)) {
      activateSensor(SENSOR_VL53L0X, "vl53l0x");
    }
  }

  void detectI2CSensors() {
    resetI2CSensorState();

    detectVL53L0X();
    detectAHTX0();
    detectSHT4X();
    detectSHTC3();
    detectPressureSensors();
    detectCurrentSensors();
  }

  void scanI2CDevices() {
    _i2c_fault = false;
    detectFuelGauge();
    detectI2CSensors();
  }

  void beginI2C() {
    Wire.setClock(100000);
    Wire.setTimeOut(50);
    _i2c_ready = true;
    scanI2CDevices();
  }

  bool queryActiveSensor(const ActiveSensor& sensor, CayenneLPP& telemetry) {
    switch (sensor.kind) {
      case SENSOR_AHTX0: {
        sensors_event_t humidity;
        sensors_event_t temp;
        _ahtx0.getEvent(&humidity, &temp);
        telemetry.addTemperature(sensor.channel, temp.temperature);
        telemetry.addRelativeHumidity(sensor.channel, humidity.relative_humidity);
        return true;
      }
      case SENSOR_BME280: {
        if (!_bme280.takeForcedMeasurement()) {
          return false;
        }
        telemetry.addTemperature(sensor.channel, _bme280.readTemperature());
        telemetry.addRelativeHumidity(sensor.channel, _bme280.readHumidity());
        telemetry.addBarometricPressure(sensor.channel, _bme280.readPressure() / 100.0f);
        telemetry.addAltitude(sensor.channel, _bme280.readAltitude(SEALEVEL_PRESSURE_HPA));
        return true;
      }
      case SENSOR_BMP280:
        telemetry.addTemperature(sensor.channel, _bmp280.readTemperature());
        telemetry.addBarometricPressure(sensor.channel, _bmp280.readPressure() / 100.0f);
        telemetry.addAltitude(sensor.channel, _bmp280.readAltitude(SEALEVEL_PRESSURE_HPA));
        return true;
      case SENSOR_SHTC3: {
        sensors_event_t humidity;
        sensors_event_t temp;
        _shtc3.getEvent(&humidity, &temp);
        telemetry.addTemperature(sensor.channel, temp.temperature);
        telemetry.addRelativeHumidity(sensor.channel, humidity.relative_humidity);
        return true;
      }
      case SENSOR_SHT4X: {
        float temperature = 0;
        float humidity = 0;
        if (_sht4x.measureLowestPrecision(temperature, humidity) != 0) {
          return false;
        }
        telemetry.addTemperature(sensor.channel, temperature);
        telemetry.addRelativeHumidity(sensor.channel, humidity);
        return true;
      }
      case SENSOR_INA219:
        telemetry.addVoltage(sensor.channel, _ina219.getBusVoltage_V());
        telemetry.addCurrent(sensor.channel, _ina219.getCurrent_mA() / 1000.0f);
        telemetry.addPower(sensor.channel, _ina219.getPower_mW() / 1000.0f);
        return true;
      case SENSOR_INA260:
        telemetry.addVoltage(sensor.channel, _ina260.readBusVoltage() / 1000.0f);
        telemetry.addCurrent(sensor.channel, _ina260.readCurrent() / 1000.0f);
        telemetry.addPower(sensor.channel, _ina260.readPower() / 1000.0f);
        return true;
      case SENSOR_VL53L0X: {
        VL53L0X_RangingMeasurementData_t measure;
        _vl53l0x.rangingTest(&measure, false);
        telemetry.addDistance(sensor.channel, measure.RangeStatus != 4 ? measure.RangeMilliMeter / 1000.0f : 0.0f);
        return true;
      }
    }

    return false;
  }

  bool queryFuelGauge(CayenneLPP& telemetry) {
    if (!_fuel_gauge_detected || isnan(_fuel_charge_rate_pct_per_hour)) {
      return false;
    }

    telemetry.addAnalogInput(TELEM_CHANNEL_SELF, _fuel_charge_rate_pct_per_hour);
    return true;
  }

public:
  MeshsmithSensorManager(LocationProvider& location, XiaoC6Board& board) : _location(&location), _board(&board) {}

  bool begin() override {
    beginGPS();
    beginI2C();
    return true;
  }

  bool querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) override {
    bool wrote = false;

    if ((requester_permissions & TELEM_PERM_LOCATION) && _gps_active) {
      telemetry.addGPS(TELEM_CHANNEL_SELF, node_lat, node_lon, node_altitude);
      wrote = true;
    }

    if (requester_permissions & (TELEM_PERM_BASE | TELEM_PERM_ENVIRONMENT)) {
      wrote = queryFuelGauge(telemetry) || wrote;
    }

    if (requester_permissions & TELEM_PERM_ENVIRONMENT) {
      for (uint8_t i = 0; i < _active_sensor_count; i++) {
        wrote = queryActiveSensor(_active_sensors[i], telemetry) || wrote;
      }
    }

    return wrote;
  }

  void loop() override {
    static unsigned long next_gps_update = 0;
    static unsigned long next_fuel_update = 0;

    if (_gps_active) {
      _location->loop();
    }

    if (_gps_active && millis() > next_gps_update) {
      if (_location->isValid()) {
        node_lat = ((double)_location->getLatitude()) / 1000000.0;
        node_lon = ((double)_location->getLongitude()) / 1000000.0;
        node_altitude = ((double)_location->getAltitude()) / 1000.0;
      }
      next_gps_update = millis() + (_gps_update_interval_sec * 1000);
    }

    if (_i2c_ready && millis() > next_fuel_update) {
      if (_fuel_gauge_detected) {
        updateFuelGauge();
      } else {
        detectFuelGauge();
      }
      next_fuel_update = millis() + FUEL_GAUGE_UPDATE_INTERVAL_MS;
    }
  }

  int getNumSettings() const override {
    return (_gps_detected ? 2 : 0) + 4;
  }

  const char* getSettingName(int i) const override {
    int index = 0;
    if (_gps_detected) {
      if (i == index++) return "gps";
      if (i == index++) return "gps_interval";
    }
    if (i == index++) return "i2c";
    if (i == index++) return "fuel_mv";
    if (i == index++) return "fuel_rate";
    if (i == index++) return "sensors";
    return NULL;
  }

  const char* getSettingValue(int i) const override {
    static char gps_interval[11];
    static char fuel_millivolts[11];
    static char fuel_rate[16];
    int index = 0;

    if (_gps_detected) {
      if (i == index++) return _gps_active ? "1" : "0";
      if (i == index++) {
        snprintf(gps_interval, sizeof(gps_interval), "%lu", (unsigned long)_gps_update_interval_sec);
        return gps_interval;
      }
    }

    if (i == index++) {
      if (_i2c_fault) return "fault";
      return _i2c_ready ? "ok" : "off";
    }
    if (i == index++) {
      if (!_fuel_gauge_detected) return "none";
      snprintf(fuel_millivolts, sizeof(fuel_millivolts), "%u", _fuel_millivolts);
      return fuel_millivolts;
    }
    if (i == index++) {
      if (!_fuel_gauge_detected || isnan(_fuel_charge_rate_pct_per_hour)) return "none";
      snprintf(fuel_rate, sizeof(fuel_rate), "%.1f", _fuel_charge_rate_pct_per_hour);
      return fuel_rate;
    }
    if (i == index++) return _sensor_list;
    return NULL;
  }

  bool setSettingValue(const char* name, const char* value) override {
    if (strcmp(name, "gps") == 0) {
      if (strcmp(value, "0") == 0) {
        stopGPS();
      } else {
        startGPS();
      }
      return true;
    }

    if (strcmp(name, "gps_interval") == 0) {
      uint32_t interval_seconds = atoi(value);
      _gps_update_interval_sec = interval_seconds > 0 ? interval_seconds : 1;
      return true;
    }

    if (strcmp(name, "i2c") == 0) {
      scanI2CDevices();
      return true;
    }

    if (strcmp(name, "fuel_mv") == 0 || strcmp(name, "fuel_rate") == 0 || strcmp(name, "fuel") == 0) {
      if (_fuel_gauge_detected) {
        updateFuelGauge();
      } else {
        detectFuelGauge();
      }
      return true;
    }

    return false;
  }

  LocationProvider* getLocationProvider() override {
    return _location;
  }
};
