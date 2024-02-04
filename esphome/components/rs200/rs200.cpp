#include "rs200.h"
#include "esphome/core/log.h"

namespace esphome {
namespace rs200 {

static const char *const TAG = "rs200.sensor";
static const int FRAME_LENGTH_BYTES = 5;
static const uint8_t FRAME_START = 0x3A;

void RS200Component::dump_config() {
  this->check_uart_settings(115200, 1, esphome::uart::UART_CONFIG_PARITY_NONE, 8);
  ESP_LOGCONFIG(TAG, "rs200:");
  LOG_SENSOR("  ", "Rainfall status", this->rain_);
  LOG_SENSOR("  ", "Realtime rain", this->realtime_rain_);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Connection with rs200 failed!");
  }

  //LOG_UPDATE_INTERVAL(this);

  int i = 0;
#define RS200_LOG_SENSOR(s) \
  if (this->sensors_[i++] != nullptr) { \
    LOG_SENSOR("  ", #s, this->sensors_[i - 1]); \
  }
}

void RS200Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up rs200...");
  // ask firmware version
  this->request_value(0);
  this->schedule_reboot_();
}

void RS200Component::send_settings() {
  ESP_LOGCONFIG(TAG, "Configuring rs200");
  // system status
  this->request_value(2);
  // start calibration
  this->request_value(3);
  this->set_value(3, 0);
  // rainfall reporting every n x 50ms
  this->set_value(5,0);
}

void RS200Component::loop() {
  uint8_t data;
  while (this->available() > 0) {
    if (this->read_byte(&data)) {
      if (data == FRAME_START)
        bytes_received = 0;
      buffer_[bytes_received] = data;
      bytes_received++;
      if (bytes_received == FRAME_LENGTH_BYTES) {
        // complete line received
        this->process_line_();
        for (size_t i = 0; i < sizeof(buffer_); ++i) {
            buffer_[i] = 0;
        }
        bytes_received = 0;
      }
    }
  }
}

void RS200Component::update() {
  if (this->boot_count_ > 0) {
    this->request_value(1);
  }
}

void RS200Component::schedule_reboot_() {
  this->boot_count_ = 0;
  this->set_interval("reboot", 5000, [this]() {
    if (this->boot_count_ < 0) {
      ESP_LOGW(TAG, "rs200 failed to connect %d times", -this->boot_count_);
    }
    this->boot_count_--;
    this->request_value(0);
    if (this->boot_count_ < -5) {
      ESP_LOGE(TAG, "rs200 can't connect, giving up");
      rain_->publish_state(NAN);
      this->mark_failed();
    }
  });
}

void RS200Component::request_value(uint8_t command) {
  this->send_command(command & 0x7F, 0);
}

void RS200Component::set_value(uint8_t command, uint16_t dataPayload) {
  this->send_command(command | 0x01, dataPayload);
}

void RS200Component::send_command(uint8_t command, uint16_t dataPayload) {
  buffer_[0] = 0x3A;
  buffer_[1] = static_cast<uint8_t>(command);
  buffer_[2] = static_cast<uint8_t>((dataPayload >> 8) & 0xFF);
  buffer_[3] = static_cast<uint8_t>(dataPayload & 0xFF);
  buffer_[4] = calculate_crc(buffer_);

  write_array(buffer_, 5);
}

uint8_t RS200Component::calculate_crc(const uint8_t* data) {
  uint8_t crc;
  crc = 0xFF;

  for (size_t i = 1; i < 4; ++i) {
      crc ^= data[i];
      for (int j = 0; j < 8; ++j) {
          crc = (crc & 0x80) ? (crc << 1) ^ 0x0131 : crc << 1;
      }
  }
  return crc;
}

void RS200Component::process_line_() {
  if (this->buffer_[0] != FRAME_START)
    return;

  char flagsByte = this->buffer_[1];

  if (!flagsByte & 0x01)
    return;

  uint8_t crc = calculate_crc(this->buffer_);
  if (this->buffer_[4] != crc) {
    ESP_LOGE(TAG, "ERROR! CRC mismatch, discarding frame: read %02X, expected %02X", this->buffer_[4], crc);
    return;
  }

  int functionIndex = flagsByte & 0x7F;
  uint16_t data = static_cast<uint16_t>(buffer_[3]) << 8 | buffer_[2];
  //ESP_LOGD(TAG, "call function %X", functionIndex);
  switch (functionIndex) {
      case 0:  
        ESP_LOGD(TAG, "firmware version: %d.%d", this->buffer_[2], this->buffer_[3]);
        this->sw_version_ = std::to_string(static_cast<int>(buffer_[3])) + '.' + std::to_string(static_cast<int>(buffer_[2]));
        this->cancel_interval("reboot");
        this->no_response_count_ = 0;
        if (this->boot_count_ < 0) {
          this->boot_count_ = 1;
          this->send_settings();
        }
        break;

      case 1: {
        ESP_LOGD(TAG, "rainfall status: %d", data);
        if (rain_ != nullptr) {
          double rain_percent = static_cast<double>(round(static_cast<double>(data)* 33.33));
          rain_->publish_state(rain_percent);
        }
        break;
      }
      case 2:  ESP_LOGD(TAG, "sensor status: %d", data); break;
      case 3:  ESP_LOGD(TAG, "optics status: %d", data); break;
      case 4:  {
        int32_t current_millis = millis();
        if (current_millis - last_rain_millis_ < this->throttle_)
          break;
        last_rain_millis_ = current_millis;
        ESP_LOGD(TAG, "realtime rain: %d", data); 
        if (realtime_rain_ != nullptr) {
          realtime_rain_->publish_state(static_cast<double>(data) );
        }
        break;
      }
      /*case 5:  ESP_LOGD(TAG, "sensor status: %d", data); break;
      case 6:  ESP_LOGD(TAG, "sensor status: %d", data); break;
      case 7:  ESP_LOGD(TAG, "sensor status: %d", data); break;*/
      default: ESP_LOGD(TAG, "Unknown function index: %d", functionIndex);
  }
}

float RS200Component::get_setup_priority() const { return setup_priority::DATA; }

}  // namespace rs200
}  // namespace esphome
