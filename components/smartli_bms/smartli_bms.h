#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace smartli_bms {

struct SmartliPack {
  uint8_t address{0};
  uint8_t modbus_address{0};
  bool modbus_manual{false};
  std::string pcb_barcode;
  std::string pack_barcode;
  text_sensor::TextSensor *pcb_barcode_sensor{nullptr};
  text_sensor::TextSensor *pack_barcode_sensor{nullptr};
  text_sensor::TextSensor *status_sensor{nullptr};
  std::array<uint16_t, 5> alarm_values{};
  uint32_t last_dcdc_at{0};

  sensor::Sensor *current{nullptr};
  sensor::Sensor *pack_voltage{nullptr};
  sensor::Sensor *bus_voltage{nullptr};
  sensor::Sensor *state_of_charge{nullptr};
  sensor::Sensor *state_of_health{nullptr};
  sensor::Sensor *full_capacity{nullptr};
  sensor::Sensor *remaining_capacity{nullptr};
  sensor::Sensor *total_charged_ah{nullptr};
  sensor::Sensor *total_discharged_ah{nullptr};
  sensor::Sensor *cell_min_voltage{nullptr};
  sensor::Sensor *cell_max_voltage{nullptr};
  sensor::Sensor *cell_delta_voltage{nullptr};
  std::array<sensor::Sensor *, 15> cell_voltages{};

  sensor::Sensor *dcdc_bus_voltage{nullptr};
  sensor::Sensor *dcdc_bus_current{nullptr};
  sensor::Sensor *dcdc_battery_port_voltage{nullptr};
  sensor::Sensor *dcdc_battery_current{nullptr};
  sensor::Sensor *dcdc_bus_negative_voltage{nullptr};
  sensor::Sensor *dcdc_battery_negative_voltage{nullptr};
  sensor::Sensor *dcdc_discharge_bus_voltage_set{nullptr};
  sensor::Sensor *dcdc_discharge_bus_current_set{nullptr};
  sensor::Sensor *dcdc_discharge_bus_power_set{nullptr};
  sensor::Sensor *dcdc_charging_battery_voltage_set{nullptr};
  sensor::Sensor *dcdc_charge_current_set{nullptr};
  sensor::Sensor *dcdc_charging_battery_power_set{nullptr};
  sensor::Sensor *dcdc_bus_voltage_dynamic{nullptr};
  sensor::Sensor *dcdc_bus_voltage_ladder{nullptr};
  sensor::Sensor *dcdc_depth_dod{nullptr};
  sensor::Sensor *dcdc_modbus_address{nullptr};
  sensor::Sensor *dcdc_vbus_set_max_autoself{nullptr};
  std::array<sensor::Sensor *, 5> alarm_status{};
};

class SmartliBms : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  void add_pack(uint8_t address, uint8_t modbus_address);
  void set_dcdc_update_interval(uint32_t interval) { dcdc_update_interval_ = interval; }
  void set_response_timeout(uint32_t timeout) { response_timeout_ = timeout; }
  void set_flow_control_pin(InternalGPIOPin *pin) { flow_control_pin_ = pin; }

#define DECLARE_SETTER(name) void set_##name##_sensor(uint8_t address, sensor::Sensor *value)
  DECLARE_SETTER(current);
  DECLARE_SETTER(pack_voltage);
  DECLARE_SETTER(bus_voltage);
  DECLARE_SETTER(state_of_charge);
  DECLARE_SETTER(state_of_health);
  DECLARE_SETTER(full_capacity);
  DECLARE_SETTER(remaining_capacity);
  DECLARE_SETTER(total_charged_ah);
  DECLARE_SETTER(total_discharged_ah);
  DECLARE_SETTER(cell_min_voltage);
  DECLARE_SETTER(cell_max_voltage);
  DECLARE_SETTER(cell_delta_voltage);
  DECLARE_SETTER(dcdc_bus_voltage);
  DECLARE_SETTER(dcdc_bus_current);
  DECLARE_SETTER(dcdc_battery_port_voltage);
  DECLARE_SETTER(dcdc_battery_current);
  DECLARE_SETTER(dcdc_bus_negative_voltage);
  DECLARE_SETTER(dcdc_battery_negative_voltage);
  DECLARE_SETTER(dcdc_discharge_bus_voltage_set);
  DECLARE_SETTER(dcdc_discharge_bus_current_set);
  DECLARE_SETTER(dcdc_discharge_bus_power_set);
  DECLARE_SETTER(dcdc_charging_battery_voltage_set);
  DECLARE_SETTER(dcdc_charge_current_set);
  DECLARE_SETTER(dcdc_charging_battery_power_set);
  DECLARE_SETTER(dcdc_bus_voltage_dynamic);
  DECLARE_SETTER(dcdc_bus_voltage_ladder);
  DECLARE_SETTER(dcdc_depth_dod);
  DECLARE_SETTER(dcdc_modbus_address);
  DECLARE_SETTER(dcdc_vbus_set_max_autoself);
#undef DECLARE_SETTER
  void set_cell_voltage_sensor(uint8_t address, size_t index, sensor::Sensor *value);
  void set_alarm_status_sensor(uint8_t address, size_t index, sensor::Sensor *value);
  void set_pcb_barcode_text_sensor(uint8_t address,
                                   text_sensor::TextSensor *value);
  void set_pack_barcode_text_sensor(uint8_t address,
                                    text_sensor::TextSensor *value);
  void set_status_text_sensor(uint8_t address,
                              text_sensor::TextSensor *value);

 protected:
  enum class Phase : uint8_t {
    IDLE,
    TELEMETRY,
    DCDC,
    PCB_BARCODE,
    PACK_BARCODE,
  };

  static constexpr size_t MAX_FRAME_SIZE = 300;

  SmartliPack *find_pack_(uint8_t address);
  void begin_pack_();
  void advance_(bool response_received);
  void send_binary_request_(uint8_t address, uint8_t command);
  void send_pack_barcode_request_(uint8_t address);
  void send_dcdc_request_(uint8_t address);
  void send_bytes_(const uint8_t *data, size_t length);
  void reset_frame_();
  void process_byte_(uint8_t byte);
  bool process_binary_frame_();
  bool process_ascii_frame_();
  void parse_telemetry_(SmartliPack &pack, const uint8_t *payload, size_t length);
  void parse_dcdc_(SmartliPack &pack, const uint8_t *payload, size_t length);
  void publish_status_(SmartliPack &pack);
  uint16_t read_u16_(const uint8_t *data) const;
  uint32_t read_u32_(const uint8_t *data) const;
  uint8_t field_width_(uint8_t field_id) const;
  int8_t hex_nibble_(uint8_t value) const;
  bool decode_hex_byte_(size_t offset, uint8_t *value) const;
  std::string normalize_barcode_(const uint8_t *data, size_t length) const;

  std::vector<SmartliPack> packs_;
  size_t pack_index_{0};
  Phase phase_{Phase::IDLE};
  uint32_t phase_started_at_{0};
  uint32_t response_timeout_{700};
  uint32_t dcdc_update_interval_{60000};
  InternalGPIOPin *flow_control_pin_{nullptr};
  std::vector<uint8_t> frame_;
  size_t expected_frame_length_{0};
  bool ascii_frame_{false};
};

}  // namespace smartli_bms
}  // namespace esphome
