#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace smartli_bms {

class SmartliBms : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  void set_address(uint8_t address) { this->address_ = address; }
  void set_flow_control_pin(InternalGPIOPin *pin) { this->flow_control_pin_ = pin; }

  void set_pack_voltage_sensor(sensor::Sensor *sensor) { this->pack_voltage_sensor_ = sensor; }
  void set_state_of_charge_sensor(sensor::Sensor *sensor) { this->state_of_charge_sensor_ = sensor; }
  void set_state_of_health_sensor(sensor::Sensor *sensor) { this->state_of_health_sensor_ = sensor; }
  void set_rated_capacity_sensor(sensor::Sensor *sensor) { this->rated_capacity_sensor_ = sensor; }
  void set_cell_min_voltage_sensor(sensor::Sensor *sensor) { this->cell_min_voltage_sensor_ = sensor; }
  void set_cell_max_voltage_sensor(sensor::Sensor *sensor) { this->cell_max_voltage_sensor_ = sensor; }
  void set_cell_delta_voltage_sensor(sensor::Sensor *sensor) { this->cell_delta_voltage_sensor_ = sensor; }
  void set_cell_voltage_sensor(size_t index, sensor::Sensor *sensor) {
    if (index < this->cell_voltage_sensors_.size())
      this->cell_voltage_sensors_[index] = sensor;
  }

 protected:
  static constexpr size_t MAX_FRAME_SIZE = 260;
  static constexpr uint32_t FRAME_TIMEOUT_MS = 500;

  void send_read_request_();
  void reset_frame_();
  void process_byte_(uint8_t byte);
  void process_frame_();
  void parse_telemetry_(const uint8_t *payload, size_t payload_length);
  uint16_t read_u16_(const uint8_t *data) const;
  uint32_t read_u32_(const uint8_t *data) const;
  uint8_t field_width_(uint8_t field_id) const;

  uint8_t address_{1};
  InternalGPIOPin *flow_control_pin_{nullptr};
  std::vector<uint8_t> frame_;
  size_t expected_frame_length_{0};
  uint32_t last_byte_at_{0};

  sensor::Sensor *pack_voltage_sensor_{nullptr};
  sensor::Sensor *state_of_charge_sensor_{nullptr};
  sensor::Sensor *state_of_health_sensor_{nullptr};
  sensor::Sensor *rated_capacity_sensor_{nullptr};
  sensor::Sensor *cell_min_voltage_sensor_{nullptr};
  sensor::Sensor *cell_max_voltage_sensor_{nullptr};
  sensor::Sensor *cell_delta_voltage_sensor_{nullptr};
  std::array<sensor::Sensor *, 15> cell_voltage_sensors_{};
};

}  // namespace smartli_bms
}  // namespace esphome

