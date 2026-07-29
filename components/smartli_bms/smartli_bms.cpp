#include "smartli_bms.h"

#include <algorithm>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace smartli_bms {

static const char *const TAG = "smartli_bms";

void SmartliBms::setup() {
  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->setup();
    this->flow_control_pin_->digital_write(false);
  }
  this->frame_.reserve(MAX_FRAME_SIZE);
}

void SmartliBms::dump_config() {
  ESP_LOGCONFIG(TAG, "SmartLi BMS:");
  ESP_LOGCONFIG(TAG, "  Address: %u", this->address_);
  ESP_LOGCONFIG(TAG, "  Update interval: %u ms", this->get_update_interval());
  LOG_PIN("  RS485 flow control pin: ", this->flow_control_pin_);
  this->check_uart_settings(9600);
}

void SmartliBms::update() { this->send_read_request_(); }

void SmartliBms::send_read_request_() {
  const uint8_t check = static_cast<uint8_t>(0U - this->address_ - 0x01U);
  const uint8_t request[] = {0x7E, this->address_, 0x01, 0x00, check, 0x0D};

  this->reset_frame_();
  uint8_t stale_byte;
  while (this->available())
    this->read_byte(&stale_byte);

  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->digital_write(true);
    delayMicroseconds(100);
  }

  this->write_array(request, sizeof(request));
  this->flush();

  if (this->flow_control_pin_ != nullptr) {
    delayMicroseconds(200);
    this->flow_control_pin_->digital_write(false);
  }

  ESP_LOGV(TAG, "Requested telemetry from address %u", this->address_);
}

void SmartliBms::loop() {
  while (this->available()) {
    uint8_t byte;
    if (!this->read_byte(&byte))
      break;
    this->process_byte_(byte);
  }

  if (!this->frame_.empty() && millis() - this->last_byte_at_ > FRAME_TIMEOUT_MS) {
    ESP_LOGW(TAG, "Discarding incomplete frame (%u bytes)", this->frame_.size());
    this->reset_frame_();
  }
}

void SmartliBms::process_byte_(uint8_t byte) {
  this->last_byte_at_ = millis();

  if (this->frame_.empty()) {
    if (byte != 0x7E)
      return;
    this->frame_.push_back(byte);
    return;
  }

  this->frame_.push_back(byte);

  if (this->frame_.size() == 4) {
    this->expected_frame_length_ = 6U + this->frame_[3];
    if (this->expected_frame_length_ > MAX_FRAME_SIZE) {
      ESP_LOGW(TAG, "Invalid payload length: %u", this->frame_[3]);
      this->reset_frame_();
      return;
    }
  }

  if (this->expected_frame_length_ != 0 &&
      this->frame_.size() == this->expected_frame_length_) {
    this->process_frame_();
    this->reset_frame_();
  } else if (this->frame_.size() >= MAX_FRAME_SIZE) {
    ESP_LOGW(TAG, "Frame exceeded maximum size");
    this->reset_frame_();
  }
}

void SmartliBms::process_frame_() {
  if (this->frame_.size() < 6 || this->frame_.back() != 0x0D) {
    ESP_LOGW(TAG, "Invalid frame terminator");
    return;
  }

  const uint8_t address = this->frame_[1];
  const uint8_t command = this->frame_[2];
  const uint8_t payload_length = this->frame_[3];

  if (address != this->address_) {
    ESP_LOGVV(TAG, "Ignoring frame for address %u", address);
    return;
  }
  if (command != 0x01 || payload_length == 0) {
    ESP_LOGVV(TAG, "Ignoring command 0x%02X with %u payload bytes", command, payload_length);
    return;
  }

  this->parse_telemetry_(&this->frame_[4], payload_length);
}

void SmartliBms::parse_telemetry_(const uint8_t *payload, size_t payload_length) {
  size_t offset = 0;
  bool cells_received = false;
  std::array<uint16_t, 15> cells{};

  while (offset + 2 <= payload_length) {
    const uint8_t field_id = payload[offset++];
    const uint8_t count = payload[offset++];
    const uint8_t width = this->field_width_(field_id);

    if (width == 0) {
      ESP_LOGW(TAG, "Unknown telemetry field 0x%02X", field_id);
      return;
    }

    const size_t data_length = static_cast<size_t>(count) * width;
    if (offset + data_length > payload_length) {
      ESP_LOGW(TAG, "Incomplete telemetry field 0x%02X", field_id);
      return;
    }

    if (field_id == 0x01 && width == 2) {
      const size_t cell_count = std::min<size_t>(count, cells.size());
      for (size_t i = 0; i < cell_count; i++) {
        cells[i] = this->read_u16_(&payload[offset + i * 2]);
        if (this->cell_voltage_sensors_[i] != nullptr)
          this->cell_voltage_sensors_[i]->publish_state(cells[i] / 1000.0f);
      }
      cells_received = cell_count > 0;
    } else if (count >= 1 && field_id == 0x02 && this->rated_capacity_sensor_ != nullptr) {
      this->rated_capacity_sensor_->publish_state(this->read_u16_(&payload[offset]) / 100.0f);
    } else if (count >= 1 && field_id == 0x03 && this->pack_voltage_sensor_ != nullptr) {
      this->pack_voltage_sensor_->publish_state(this->read_u16_(&payload[offset]) / 100.0f);
    } else if (count >= 1 && field_id == 0x04 && this->state_of_charge_sensor_ != nullptr) {
      this->state_of_charge_sensor_->publish_state(this->read_u16_(&payload[offset]) / 100.0f);
    } else if (count >= 1 && field_id == 0x09 && this->state_of_health_sensor_ != nullptr) {
      this->state_of_health_sensor_->publish_state(this->read_u16_(&payload[offset]) / 100.0f);
    }

    offset += data_length;
  }

  if (offset != payload_length) {
    ESP_LOGW(TAG, "Telemetry ended with %u unparsed bytes", payload_length - offset);
    return;
  }

  if (cells_received) {
    const auto minimum = *std::min_element(cells.begin(), cells.end());
    const auto maximum = *std::max_element(cells.begin(), cells.end());

    if (this->cell_min_voltage_sensor_ != nullptr)
      this->cell_min_voltage_sensor_->publish_state(minimum / 1000.0f);
    if (this->cell_max_voltage_sensor_ != nullptr)
      this->cell_max_voltage_sensor_->publish_state(maximum / 1000.0f);
    if (this->cell_delta_voltage_sensor_ != nullptr)
      this->cell_delta_voltage_sensor_->publish_state((maximum - minimum) / 1000.0f);
  }

  ESP_LOGD(TAG, "Telemetry received from address %u", this->address_);
}

uint16_t SmartliBms::read_u16_(const uint8_t *data) const {
  return (static_cast<uint16_t>(data[0]) << 8) | data[1];
}

uint32_t SmartliBms::read_u32_(const uint8_t *data) const {
  return (static_cast<uint32_t>(data[0]) << 24) |
         (static_cast<uint32_t>(data[1]) << 16) |
         (static_cast<uint32_t>(data[2]) << 8) | data[3];
}

uint8_t SmartliBms::field_width_(uint8_t field_id) const {
  if (field_id >= 0x01 && field_id <= 0x0A)
    return 2;
  if (field_id >= 0x0B && field_id <= 0x10)
    return 4;
  if (field_id == 0x11 || field_id == 0x12)
    return 2;
  return 0;
}

void SmartliBms::reset_frame_() {
  this->frame_.clear();
  this->expected_frame_length_ = 0;
}

}  // namespace smartli_bms
}  // namespace esphome
