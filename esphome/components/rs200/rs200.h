#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/components/sensor/sensor.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace rs200 {

#define RS200_IGNORE_LIST(F, SEP) F("Emitters") SEP F("Event") SEP F("Reset")

class RS200Component : public PollingComponent, public uart::UARTDevice {
 public:
  void set_rain(sensor::Sensor *rain) { rain_ = rain; }
  void set_realtime_rain(sensor::Sensor *realtime_rain) { realtime_rain_ = realtime_rain; }
#ifdef USE_BINARY_SENSOR
  void set_lens_bad_sensor(binary_sensor::BinarySensor *sensor) { this->lens_bad_sensor_ = sensor; }
  void set_raining_sensor(binary_sensor::BinarySensor *sensor) { this->raining_sensor_ = sensor; }
#endif
  void set_request_temperature(bool b) { request_temperature_ = b; }

  /// Schedule data readings.
  void update() override;
  /// Read data once available
  void loop() override;
  /// Setup the sensor and test for a connection.
  void setup() override;
  void dump_config() override;

  float get_setup_priority() const override;

  void set_disable_led(bool disable_led) { this->disable_led_ = disable_led; }

 protected:
  void process_line_();
  void schedule_reboot_();
  void send_command(uint8_t command, uint16_t dataPayload);
  void set_value(uint8_t command, uint16_t dataPayload);
  void request_value(uint8_t command);
  uint8_t calculate_crc(const uint8_t* data);
  void send_settings();

  sensor::Sensor *rain_{nullptr};
  sensor::Sensor *realtime_rain_{nullptr};
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *lens_bad_sensor_{nullptr};
  binary_sensor::BinarySensor *raining_sensor_{nullptr};
#endif

  int16_t boot_count_ = 0;
  int16_t throttle_ = 450; //ms
  int32_t last_rain_millis_ = millis();
  int16_t no_response_count_ = 0;
  uint8_t buffer_[5];
  int16_t bytes_received = 0;
  std::string sw_version_ = "";
  bool raining_ = false;
  bool lens_bad_ = false;
  bool request_temperature_ = false;
  bool disable_led_ = false;
};

class RS200BinaryComponent : public Component {
 public:
  RS200BinaryComponent(RS200Component *parent) {}
};

}  // namespace rs200
}  // namespace esphome
