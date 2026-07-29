#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace smartli_bms {

class SmartliBms;

enum SmartliSelectType : uint8_t {
  VBUS_DISCHARGE,
  VBUS_DOD,
  IBUS_PERCENT,
  PBUS_PERCENT,
  AVG_CHARGE_PERCENT,
  DOD_PERCENT,
  CHARGING_LOOP,
  DISCHARGE_LOOP,
  MODE_ALL,
};

class SmartliBmsSelect : public select::Select {
 public:
  void set_parent(SmartliBms *parent) { parent_ = parent; }
  void set_address(uint8_t address) { address_ = address; }
  void set_type(SmartliSelectType type) { type_ = type; }

 protected:
  void control(const std::string &value) override;
  SmartliBms *parent_{nullptr};
  uint8_t address_{0};
  SmartliSelectType type_{VBUS_DISCHARGE};
};

struct SmartliPendingWrite {
  uint8_t pack_address{0};
  uint16_t register_address{0};
  uint16_t value{0};
  SmartliBmsSelect *source{nullptr};
  std::string option;
  uint8_t retries{0};
};

struct SmartliPack {
  uint8_t address{0};
  uint8_t modbus_address{0};
  bool modbus_manual{false};
  std::string pcb_barcode;
  std::string pack_barcode;
  std::string modbus_pcb_barcode;
  std::string modbus_pack_barcode;
  text_sensor::TextSensor *pcb_barcode_sensor{nullptr};
  text_sensor::TextSensor *pack_barcode_sensor{nullptr};
  text_sensor::TextSensor *modbus_pcb_barcode_sensor{nullptr};
  text_sensor::TextSensor *modbus_pack_barcode_sensor{nullptr};
  text_sensor::TextSensor *modbus_address_sensor{nullptr};
  text_sensor::TextSensor *status_sensor{nullptr};
  text_sensor::TextSensor *mode_sensor{nullptr};
  text_sensor::TextSensor *last_update_sensor{nullptr};
  uint32_t telemetry_sequence{0};
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
  sensor::Sensor *total_charged_energy{nullptr};
  sensor::Sensor *total_discharged_energy{nullptr};
  sensor::Sensor *cell_min_voltage{nullptr};
  sensor::Sensor *cell_max_voltage{nullptr};
  sensor::Sensor *cell_delta_voltage{nullptr};
  sensor::Sensor *cell_average_voltage{nullptr};
  sensor::Sensor *power{nullptr};
  sensor::Sensor *max_temperature{nullptr};
  sensor::Sensor *mos_temperature{nullptr};
  sensor::Sensor *cycle_count{nullptr};
  std::array<sensor::Sensor *, 8> temperatures{};
  std::array<sensor::Sensor *, 15> cell_voltages{};

  sensor::Sensor *dcdc_bus_current{nullptr};
  sensor::Sensor *dcdc_discharge_bus_voltage_set{nullptr};
  sensor::Sensor *dcdc_discharge_bus_current_set{nullptr};
  sensor::Sensor *dcdc_discharge_bus_power_set{nullptr};
  sensor::Sensor *dcdc_charging_battery_voltage_set{nullptr};
  sensor::Sensor *dcdc_charge_current_set{nullptr};
  sensor::Sensor *dcdc_charging_battery_power_set{nullptr};
  sensor::Sensor *dcdc_bus_voltage_ladder{nullptr};
  sensor::Sensor *dcdc_depth_dod{nullptr};
  sensor::Sensor *dcdc_vbus_set_max_autoself{nullptr};
  std::array<sensor::Sensor *, 5> alarm_status{};
  std::array<SmartliBmsSelect *, 8> config_selects{};
  bool mode_loaded{false};
  bool loops_loaded{false};
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
  void set_pack_delay(uint32_t delay) { pack_delay_ = delay; }
  void set_request_delay(uint32_t delay) { request_delay_ = delay; }
  void set_flow_control_pin(InternalGPIOPin *pin) { flow_control_pin_ = pin; }
  void queue_modbus_write(uint8_t pack_address, uint16_t register_address,
                          uint16_t value, SmartliBmsSelect *source,
                          const std::string &option);
  void queue_mode_write_all(uint16_t value, SmartliBmsSelect *source,
                            const std::string &option);
  void set_config_select(uint8_t address, SmartliSelectType type,
                         SmartliBmsSelect *value);

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
  DECLARE_SETTER(total_charged_energy);
  DECLARE_SETTER(total_discharged_energy);
  DECLARE_SETTER(cell_min_voltage);
  DECLARE_SETTER(cell_max_voltage);
  DECLARE_SETTER(cell_delta_voltage);
  DECLARE_SETTER(cell_average_voltage);
  DECLARE_SETTER(power);
  DECLARE_SETTER(max_temperature);
  DECLARE_SETTER(mos_temperature);
  DECLARE_SETTER(cycle_count);
  DECLARE_SETTER(dcdc_bus_current);
  DECLARE_SETTER(dcdc_discharge_bus_voltage_set);
  DECLARE_SETTER(dcdc_discharge_bus_current_set);
  DECLARE_SETTER(dcdc_discharge_bus_power_set);
  DECLARE_SETTER(dcdc_charging_battery_voltage_set);
  DECLARE_SETTER(dcdc_charge_current_set);
  DECLARE_SETTER(dcdc_charging_battery_power_set);
  DECLARE_SETTER(dcdc_bus_voltage_ladder);
  DECLARE_SETTER(dcdc_depth_dod);
  DECLARE_SETTER(dcdc_vbus_set_max_autoself);
#undef DECLARE_SETTER
  void set_cell_voltage_sensor(uint8_t address, size_t index, sensor::Sensor *value);
  void set_alarm_status_sensor(uint8_t address, size_t index, sensor::Sensor *value);
  void set_temperature_sensor(uint8_t address, size_t index,
                              sensor::Sensor *value);
  void set_pcb_barcode_text_sensor(uint8_t address,
                                   text_sensor::TextSensor *value);
  void set_pack_barcode_text_sensor(uint8_t address,
                                    text_sensor::TextSensor *value);
  void set_modbus_pcb_barcode_text_sensor(uint8_t address,
                                          text_sensor::TextSensor *value);
  void set_modbus_pack_barcode_text_sensor(uint8_t address,
                                           text_sensor::TextSensor *value);
  void set_modbus_address_text_sensor(uint8_t address,
                                      text_sensor::TextSensor *value);
  void set_status_text_sensor(uint8_t address,
                              text_sensor::TextSensor *value);
  void set_mode_text_sensor(uint8_t address,
                            text_sensor::TextSensor *value);
  void set_last_update_text_sensor(uint8_t address,
                                   text_sensor::TextSensor *value);

 protected:
  enum class Phase : uint8_t {
    IDLE,
    TELEMETRY,
    DCDC,
    PCB_BARCODE,
    PACK_BARCODE,
    MODBUS_PCB_BARCODE,
    MODBUS_PACK_BARCODE,
    DISCOVERY_MODBUS_PCB,
    DISCOVERY_MODBUS_PACK,
    MODBUS_CONFIG_MODE,
    MODBUS_CONFIG_LOOPS,
    MODBUS_WRITE,
  };

  static constexpr size_t MAX_FRAME_SIZE = 300;

  SmartliPack *find_pack_(uint8_t address);
  void begin_pack_();
  void begin_modbus_discovery_();
  void begin_pending_write_();
  void schedule_phase_(Phase next, std::function<void()> action);
  void advance_modbus_discovery_(bool response_received);
  void finish_modbus_discovery_candidate_();
  void advance_(bool response_received);
  void send_binary_request_(uint8_t address, uint8_t command);
  void send_pack_barcode_request_(uint8_t address);
  void send_dcdc_request_(uint8_t address);
  void send_modbus_read_(uint8_t address, uint16_t start, uint16_t count);
  void send_modbus_write_(uint8_t address, uint16_t register_address,
                          uint16_t value);
  void send_bytes_(const uint8_t *data, size_t length);
  void reset_frame_();
  void process_byte_(uint8_t byte);
  bool process_binary_frame_();
  bool process_ascii_frame_();
  bool process_modbus_frame_();
  void parse_telemetry_(SmartliPack &pack, const uint8_t *payload, size_t length);
  void parse_dcdc_(SmartliPack &pack, const uint8_t *payload, size_t length);
  void publish_status_(SmartliPack &pack);
  uint16_t crc16_(const uint8_t *data, size_t length) const;
  uint16_t read_u16_(const uint8_t *data) const;
  uint32_t read_u32_(const uint8_t *data) const;
  uint8_t field_width_(uint8_t field_id) const;
  int8_t hex_nibble_(uint8_t value) const;
  bool decode_hex_byte_(size_t offset, uint8_t *value) const;
  std::string normalize_barcode_(const uint8_t *data, size_t length) const;
  uint8_t discovery_candidate_address_(size_t index) const;

  std::vector<SmartliPack> packs_;
  size_t pack_index_{0};
  Phase phase_{Phase::IDLE};
  uint32_t phase_started_at_{0};
  uint32_t response_timeout_{700};
  uint32_t pack_delay_{2000};
  uint32_t request_delay_{1000};
  uint32_t dcdc_update_interval_{60000};
  InternalGPIOPin *flow_control_pin_{nullptr};
  std::vector<uint8_t> frame_;
  size_t expected_frame_length_{0};
  bool ascii_frame_{false};
  bool modbus_echo_{false};
  bool discovery_completed_{false};
  size_t discovery_candidate_index_{0};
  size_t discovery_candidate_count_{0};
  size_t discovery_match_count_{0};
  std::string discovery_pcb_barcode_;
  std::string discovery_pack_barcode_;
  std::vector<SmartliPendingWrite> pending_writes_;
  SmartliBmsSelect *mode_select_{nullptr};
  bool waiting_for_request_{false};
  bool resume_poll_after_writes_{false};
};

}  // namespace smartli_bms
}  // namespace esphome
