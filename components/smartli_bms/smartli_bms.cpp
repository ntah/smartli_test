#include "smartli_bms.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace smartli_bms {

static const char *const TAG = "smartli_bms";

void SmartliBms::add_pack(uint8_t address, uint8_t modbus_address) {
  SmartliPack pack;
  pack.address = address;
  pack.modbus_address = modbus_address;
  pack.modbus_manual = modbus_address != 0;
  this->packs_.push_back(pack);
}

SmartliPack *SmartliBms::find_pack_(uint8_t address) {
  for (auto &pack : this->packs_)
    if (pack.address == address)
      return &pack;
  return nullptr;
}

#define DEFINE_SETTER(name) \
  void SmartliBms::set_##name##_sensor(uint8_t address, sensor::Sensor *value) { \
    auto *pack = this->find_pack_(address); \
    if (pack != nullptr) pack->name = value; \
  }
DEFINE_SETTER(current)
DEFINE_SETTER(pack_voltage)
DEFINE_SETTER(bus_voltage)
DEFINE_SETTER(state_of_charge)
DEFINE_SETTER(state_of_health)
DEFINE_SETTER(full_capacity)
DEFINE_SETTER(remaining_capacity)
DEFINE_SETTER(total_charged_ah)
DEFINE_SETTER(total_discharged_ah)
DEFINE_SETTER(cell_min_voltage)
DEFINE_SETTER(cell_max_voltage)
DEFINE_SETTER(cell_delta_voltage)
DEFINE_SETTER(dcdc_bus_voltage)
DEFINE_SETTER(dcdc_bus_current)
DEFINE_SETTER(dcdc_battery_port_voltage)
DEFINE_SETTER(dcdc_battery_current)
DEFINE_SETTER(dcdc_bus_negative_voltage)
DEFINE_SETTER(dcdc_battery_negative_voltage)
DEFINE_SETTER(dcdc_discharge_bus_voltage_set)
DEFINE_SETTER(dcdc_discharge_bus_current_set)
DEFINE_SETTER(dcdc_discharge_bus_power_set)
DEFINE_SETTER(dcdc_charging_battery_voltage_set)
DEFINE_SETTER(dcdc_charge_current_set)
DEFINE_SETTER(dcdc_charging_battery_power_set)
DEFINE_SETTER(dcdc_bus_voltage_dynamic)
DEFINE_SETTER(dcdc_bus_voltage_ladder)
DEFINE_SETTER(dcdc_depth_dod)
DEFINE_SETTER(dcdc_modbus_address)
DEFINE_SETTER(dcdc_vbus_set_max_autoself)
#undef DEFINE_SETTER

void SmartliBms::set_cell_voltage_sensor(uint8_t address, size_t index,
                                         sensor::Sensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr && index < pack->cell_voltages.size())
    pack->cell_voltages[index] = value;
}

void SmartliBms::set_alarm_status_sensor(uint8_t address, size_t index,
                                         sensor::Sensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr && index < pack->alarm_status.size())
    pack->alarm_status[index] = value;
}

void SmartliBms::set_pcb_barcode_text_sensor(
    uint8_t address, text_sensor::TextSensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr)
    pack->pcb_barcode_sensor = value;
}

void SmartliBms::set_pack_barcode_text_sensor(
    uint8_t address, text_sensor::TextSensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr)
    pack->pack_barcode_sensor = value;
}

void SmartliBms::set_modbus_pcb_barcode_text_sensor(
    uint8_t address, text_sensor::TextSensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr)
    pack->modbus_pcb_barcode_sensor = value;
}

void SmartliBms::set_modbus_pack_barcode_text_sensor(
    uint8_t address, text_sensor::TextSensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr)
    pack->modbus_pack_barcode_sensor = value;
}

void SmartliBms::set_status_text_sensor(
    uint8_t address, text_sensor::TextSensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr)
    pack->status_sensor = value;
}

void SmartliBms::setup() {
  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->setup();
    this->flow_control_pin_->digital_write(false);
  }
  this->frame_.reserve(MAX_FRAME_SIZE);
}

void SmartliBms::dump_config() {
  ESP_LOGCONFIG(TAG, "SmartLi multi-pack controller:");
  ESP_LOGCONFIG(TAG, "  Packs: %u", this->packs_.size());
  ESP_LOGCONFIG(TAG, "  Poll interval: %u ms", this->get_update_interval());
  ESP_LOGCONFIG(TAG, "  DCDC interval: %u ms", this->dcdc_update_interval_);
  for (const auto &pack : this->packs_) {
    ESP_LOGCONFIG(TAG, "  Pack %u: Modbus %u", pack.address,
                  pack.modbus_address);
  }
  LOG_PIN("  RS485 flow control pin: ", this->flow_control_pin_);
  this->check_uart_settings(9600);
}

void SmartliBms::update() {
  if (this->phase_ != Phase::IDLE || this->packs_.empty()) {
    ESP_LOGW(TAG, "Skipping poll: previous multi-pack cycle is still active");
    return;
  }
  this->pack_index_ = 0;
  this->begin_pack_();
}

void SmartliBms::begin_pack_() {
  if (this->pack_index_ >= this->packs_.size()) {
    this->phase_ = Phase::IDLE;
    ESP_LOGD(TAG, "Multi-pack polling cycle completed");
    return;
  }
  auto &pack = this->packs_[this->pack_index_];
  this->phase_ = Phase::TELEMETRY;
  this->send_binary_request_(pack.address, 0x01);
}

void SmartliBms::advance_(bool response_received) {
  if (this->pack_index_ >= this->packs_.size()) {
    this->phase_ = Phase::IDLE;
    return;
  }
  auto &pack = this->packs_[this->pack_index_];
  const Phase completed = this->phase_;
  if (!response_received)
    ESP_LOGW(TAG, "Timeout in phase %u for pack %u",
             static_cast<uint8_t>(completed), pack.address);

  if (completed == Phase::TELEMETRY) {
    const uint32_t now = millis();
    if (pack.last_dcdc_at == 0 ||
        now - pack.last_dcdc_at >= this->dcdc_update_interval_) {
      pack.last_dcdc_at = now;
      this->phase_ = Phase::DCDC;
      this->send_dcdc_request_(pack.address);
      return;
    }
  }

  if (completed == Phase::TELEMETRY || completed == Phase::DCDC) {
    if (pack.pcb_barcode.empty()) {
      this->phase_ = Phase::PCB_BARCODE;
      this->send_binary_request_(pack.address, 0x42);
      return;
    }
  }

  if (completed == Phase::PCB_BARCODE ||
      completed == Phase::TELEMETRY || completed == Phase::DCDC) {
    if (pack.pack_barcode.empty()) {
      this->phase_ = Phase::PACK_BARCODE;
      this->send_pack_barcode_request_(pack.address);
      return;
    }
  }

  if (completed == Phase::PACK_BARCODE ||
      completed == Phase::PCB_BARCODE ||
      completed == Phase::TELEMETRY || completed == Phase::DCDC) {
    if (pack.modbus_pcb_barcode_sensor != nullptr &&
        pack.modbus_pcb_barcode.empty()) {
      this->phase_ = Phase::MODBUS_PCB_BARCODE;
      this->send_modbus_read_(pack.modbus_address, 0x104D, 10);
      return;
    }
  }

  if (completed == Phase::MODBUS_PCB_BARCODE ||
      completed == Phase::PACK_BARCODE ||
      completed == Phase::PCB_BARCODE ||
      completed == Phase::TELEMETRY || completed == Phase::DCDC) {
    if (pack.modbus_pack_barcode_sensor != nullptr &&
        pack.modbus_pack_barcode.empty()) {
      this->phase_ = Phase::MODBUS_PACK_BARCODE;
      this->send_modbus_read_(pack.modbus_address, 0x1065, 10);
      return;
    }
  }

  this->pack_index_++;
  // Prevent loop() from treating the intentional inter-pack delay as another
  // timeout for the phase that has just completed.
  this->phase_ = Phase::IDLE;
  this->set_timeout("next_pack", 150, [this]() { this->begin_pack_(); });
}

void SmartliBms::send_binary_request_(uint8_t address, uint8_t command) {
  // Captured SmartLi software requests use a fixed 0xFC check byte for the
  // PCB barcode command. Telemetry check bytes below are taken from accepted
  // requests in the five-pack capture; they are not a conventional sum.
  uint8_t check = static_cast<uint8_t>(0U - address - command);
  if (command == 0x42) {
    check = 0xFC;
  } else if (command == 0x01 && address >= 1 && address <= 5) {
    static const uint8_t TELEMETRY_CHECKS[5] = {
        0xFE, 0xFC, 0xFE, 0xF8, 0xFE};
    check = TELEMETRY_CHECKS[address - 1];
  }
  const uint8_t request[] = {0x7E, address, command, 0x00, check, 0x0D};
  this->send_bytes_(request, sizeof(request));
  ESP_LOGD(TAG, "Pack %u request command 0x%02X", address, command);
}

void SmartliBms::send_dcdc_request_(uint8_t address) {
  char body[32];
  const int body_length =
      std::snprintf(body, sizeof(body), "22%02XE592E00201", address);
  uint16_t sum = 0;
  for (int i = 0; i < body_length; i++)
    sum = static_cast<uint16_t>(sum + static_cast<uint8_t>(body[i]));
  char request[40];
  const int length = std::snprintf(request, sizeof(request), "~%s%04X\r", body,
                                   static_cast<uint16_t>(0U - sum));
  this->send_bytes_(reinterpret_cast<const uint8_t *>(request), length);
  ESP_LOGD(TAG, "Pack %u DCDC request", address);
}

void SmartliBms::send_pack_barcode_request_(uint8_t address) {
  static const uint8_t CHECKS[5] = {0xC2, 0xC0, 0xC2, 0xC4, 0xCA};
  const uint8_t check =
      address >= 1 && address <= 5 ? CHECKS[address - 1] : 0xC2;
  const uint8_t request[] = {
      0x7E, address, 0xDC, 0x03, 0x06, 0x00, 0x00, check, 0x0D};
  this->send_bytes_(request, sizeof(request));
  ESP_LOGD(TAG, "Pack %u request pack barcode", address);
}

void SmartliBms::send_modbus_read_(uint8_t address, uint16_t start,
                                    uint16_t count) {
  uint8_t request[8] = {
      address, 0x03, static_cast<uint8_t>(start >> 8),
      static_cast<uint8_t>(start), static_cast<uint8_t>(count >> 8),
      static_cast<uint8_t>(count), 0, 0};
  const uint16_t crc = this->crc16_(request, 6);
  request[6] = static_cast<uint8_t>(crc);
  request[7] = static_cast<uint8_t>(crc >> 8);
  this->send_bytes_(request, sizeof(request));
  ESP_LOGD(TAG, "Modbus %u read barcode 0x%04X count %u", address, start,
           count);
}

void SmartliBms::send_bytes_(const uint8_t *data, size_t length) {
  this->reset_frame_();
  uint8_t stale;
  while (this->available())
    this->read_byte(&stale);
  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->digital_write(true);
    delayMicroseconds(100);
  }
  this->write_array(data, length);
  this->flush();
  if (this->flow_control_pin_ != nullptr) {
    delayMicroseconds(200);
    this->flow_control_pin_->digital_write(false);
  }
  this->phase_started_at_ = millis();
}

void SmartliBms::loop() {
  while (this->available()) {
    uint8_t byte;
    if (this->read_byte(&byte))
      this->process_byte_(byte);
  }
  if (this->phase_ != Phase::IDLE &&
      millis() - this->phase_started_at_ > this->response_timeout_) {
    this->reset_frame_();
    this->advance_(false);
  }
}

void SmartliBms::process_byte_(uint8_t byte) {
  const bool modbus = this->phase_ == Phase::MODBUS_PCB_BARCODE ||
                      this->phase_ == Phase::MODBUS_PACK_BARCODE;
  if (this->frame_.empty()) {
    if (!modbus && byte != 0x7E)
      return;
    if (modbus && byte != this->packs_[this->pack_index_].modbus_address)
      return;
    this->frame_.push_back(byte);
    return;
  }

  this->frame_.push_back(byte);
  if (modbus) {
    if (this->frame_.size() == 2 && (this->frame_[1] & 0x80))
      this->expected_frame_length_ = 5;
    else if (this->frame_.size() == 3 && this->frame_[1] == 0x03) {
      // Ignore an echoed 8-byte request before waiting for the real response.
      this->modbus_echo_ = this->frame_[2] == 0x10;
      this->expected_frame_length_ =
          this->modbus_echo_ ? 8U : 5U + this->frame_[2];
    }
  } else if (this->frame_.size() == 4) {
    this->ascii_frame_ =
        this->hex_nibble_(this->frame_[1]) >= 0 &&
        this->hex_nibble_(this->frame_[2]) >= 0 &&
        this->hex_nibble_(this->frame_[3]) >= 0;
    if (!this->ascii_frame_)
      this->expected_frame_length_ = 6U + this->frame_[3];
  }
  if (this->ascii_frame_ && this->frame_.size() == 13) {
    uint16_t length_field = 0;
    for (size_t i = 9; i < 13; i++) {
      const int8_t nibble = this->hex_nibble_(this->frame_[i]);
      if (nibble < 0) {
        this->reset_frame_();
        return;
      }
      length_field = static_cast<uint16_t>((length_field << 4) | nibble);
    }
    this->expected_frame_length_ = 18U + (length_field & 0x0FFFU);
  }

  if (this->expected_frame_length_ > MAX_FRAME_SIZE) {
    this->reset_frame_();
    return;
  }
  if (this->expected_frame_length_ != 0 &&
      this->frame_.size() == this->expected_frame_length_) {
    if (modbus && this->modbus_echo_) {
      this->reset_frame_();
      return;
    }
    bool accepted = modbus ? this->process_modbus_frame_()
                           : (this->ascii_frame_ ? this->process_ascii_frame_()
                                                 : this->process_binary_frame_());
    this->reset_frame_();
    if (accepted)
      this->advance_(true);
  }
}

bool SmartliBms::process_binary_frame_() {
  if (this->frame_.size() < 6 || this->frame_.back() != 0x0D)
    return false;
  auto *pack = this->find_pack_(this->frame_[1]);
  if (pack == nullptr)
    return false;
  const uint8_t command = this->frame_[2];
  const uint8_t length = this->frame_[3];
  if (command == 0x01 && length > 0 && this->phase_ == Phase::TELEMETRY) {
    this->parse_telemetry_(*pack, &this->frame_[4], length);
    return true;
  }
  if (command == 0x42 && length > 0 && this->phase_ == Phase::PCB_BARCODE) {
    pack->pcb_barcode = this->normalize_barcode_(&this->frame_[4], length);
    if (pack->pcb_barcode_sensor != nullptr)
      pack->pcb_barcode_sensor->publish_state(pack->pcb_barcode);
    ESP_LOGI(TAG, "Pack %u PCB barcode: %s", pack->address,
             pack->pcb_barcode.c_str());
    return true;
  }
  if (command == 0xDC && length > 3 &&
      this->phase_ == Phase::PACK_BARCODE) {
    // Observed response: 00 06 + pack/system barcode padded with '^'.
    size_t barcode_length = 0;
    const uint8_t *barcode = &this->frame_[6];
    const size_t available = length - 2;
    while (barcode_length < available && barcode[barcode_length] != '^' &&
           barcode[barcode_length] != 0)
      barcode_length++;
    pack->pack_barcode =
        this->normalize_barcode_(barcode, barcode_length);
    if (pack->pack_barcode_sensor != nullptr)
      pack->pack_barcode_sensor->publish_state(pack->pack_barcode);
    ESP_LOGI(TAG, "Pack %u pack barcode: %s", pack->address,
             pack->pack_barcode.c_str());
    return true;
  }
  return false;
}

bool SmartliBms::process_ascii_frame_() {
  if (this->frame_.size() < 18 || this->frame_.back() != 0x0D)
    return false;
  uint8_t address, cid1, cid2;
  if (!this->decode_hex_byte_(3, &address) ||
      !this->decode_hex_byte_(5, &cid1) ||
      !this->decode_hex_byte_(7, &cid2))
    return false;
  auto *pack = this->find_pack_(address);
  if (pack == nullptr || cid1 != 0xE5 || cid2 != 0x00)
    return false;
  uint16_t length_field = 0;
  for (size_t i = 9; i < 13; i++)
    length_field = static_cast<uint16_t>(
        (length_field << 4) | this->hex_nibble_(this->frame_[i]));
  const size_t chars = length_field & 0x0FFFU;
  const size_t checksum_offset = 13U + chars;
  uint16_t sum = 0;
  for (size_t i = 1; i < checksum_offset; i++)
    sum = static_cast<uint16_t>(sum + this->frame_[i]);
  uint16_t transmitted = 0;
  for (size_t i = checksum_offset; i < checksum_offset + 4; i++)
    transmitted = static_cast<uint16_t>(
        (transmitted << 4) | this->hex_nibble_(this->frame_[i]));
  if (static_cast<uint16_t>(0U - sum) != transmitted)
    return false;
  std::vector<uint8_t> payload;
  payload.reserve(chars / 2);
  for (size_t i = 13; i < checksum_offset; i += 2) {
    uint8_t value;
    if (!this->decode_hex_byte_(i, &value))
      return false;
    payload.push_back(value);
  }
  this->parse_dcdc_(*pack, payload.data(), payload.size());
  return true;
}

bool SmartliBms::process_modbus_frame_() {
  if (this->frame_.size() < 5)
    return false;
  const uint16_t received =
      this->frame_[this->frame_.size() - 2] |
      (static_cast<uint16_t>(this->frame_.back()) << 8);
  if (this->crc16_(this->frame_.data(), this->frame_.size() - 2) != received)
    return false;
  if (this->frame_[1] != 0x03)
    return true;
  if (this->frame_[2] != 20 || this->frame_.size() != 25)
    return false;

  auto &pack = this->packs_[this->pack_index_];
  const std::string barcode = this->normalize_barcode_(&this->frame_[3], 20);
  if (this->phase_ == Phase::MODBUS_PCB_BARCODE) {
    pack.modbus_pcb_barcode = barcode;
    if (pack.modbus_pcb_barcode_sensor != nullptr)
      pack.modbus_pcb_barcode_sensor->publish_state(barcode);
    ESP_LOGI(TAG, "Pack %u Modbus PCB barcode: %s", pack.address,
             barcode.c_str());
  } else if (this->phase_ == Phase::MODBUS_PACK_BARCODE) {
    pack.modbus_pack_barcode = barcode;
    if (pack.modbus_pack_barcode_sensor != nullptr)
      pack.modbus_pack_barcode_sensor->publish_state(barcode);
    ESP_LOGI(TAG, "Pack %u Modbus pack barcode: %s", pack.address,
             barcode.c_str());
  }
  return true;
}

void SmartliBms::publish_status_(SmartliPack &pack) {
  if (pack.status_sensor == nullptr)
    return;

  std::vector<std::string> active;
  auto high = [&](size_t word, uint8_t bit, const char *name) {
    if (pack.alarm_values[word] & (1U << (bit + 8)))
      active.emplace_back(name);
  };
  auto low = [&](size_t word, uint8_t bit, const char *name) {
    if (pack.alarm_values[word] & (1U << bit))
      active.emplace_back(name);
  };

  high(0, 3, "Cell voltage too low fault");
  high(0, 4, "Voltage sampling disconnection");
  high(0, 5, "Charging MOS damaged");
  high(0, 6, "Discharge MOS damaged");
  high(0, 7, "Voltage sampling element damaged");
  low(0, 0, "NTC disconnection");
  low(0, 1, "ADC damaged");
  low(0, 2, "Reverse battery connection");
  low(0, 3, "Fan failure");
  low(0, 4, "Battery lock");

  high(1, 0, "Discharge over-temperature protection");
  high(1, 1, "Discharge under-temperature protection");
  high(1, 2, "Overall overvoltage protection");
  high(1, 3, "Startup failed");
  high(1, 4, "Charging MOS off");
  high(1, 5, "Discharge MOS off");
  low(1, 2, "Short circuit protection");
  low(1, 4, "Overvoltage protection");
  low(1, 5, "Undervoltage protection");
  low(1, 6, "Charging over-temperature protection");
  low(1, 7, "Charging under-temperature protection");

  high(2, 0, "Environment low-temperature protection");
  high(2, 1, "Environment high-temperature protection");
  low(2, 5, "MOSFET over-temperature protection");
  low(2, 6, "MOSFET low-temperature protection");
  low(2, 7, "Charging temperature too low");

  low(3, 1, "Vibration alarm");
  low(3, 7, "BMS module serial number duplicated");

  high(4, 0, "Ambient over-temperature alarm");
  high(4, 1, "Environment under-temperature alarm");
  high(4, 2, "MOS over-temperature alarm");
  high(4, 3, "SOC too low alarm");
  high(4, 4, "Overpressure alarm");
  high(4, 5, "Battery over-temperature warning");
  high(4, 6, "Battery discharge under-temperature alarm");
  low(4, 0, "Cell overvoltage alarm");
  low(4, 1, "Cell undervoltage alarm");
  low(4, 2, "Overall overvoltage warning");
  low(4, 3, "Overall undervoltage warning");
  low(4, 4, "Overcharge alarm");
  low(4, 5, "Overcurrent warning ignored");
  low(4, 6, "Battery charging over-temperature alarm");
  low(4, 7, "Battery charging under-temperature alarm");

  if (active.empty()) {
    pack.status_sensor->publish_state("Normal");
    return;
  }
  std::string result;
  for (size_t i = 0; i < active.size(); i++) {
    if (i != 0)
      result += ", ";
    result += active[i];
  }
  pack.status_sensor->publish_state(result);
}

void SmartliBms::parse_dcdc_(SmartliPack &pack, const uint8_t *p, size_t n) {
  if (n < 101 || p[0] != 0)
    return;
#define PUB(member, offset, divisor) \
  if (pack.member != nullptr) pack.member->publish_state(this->read_u16_(&p[offset]) / divisor)
  PUB(dcdc_bus_voltage, 1, 100.0f);
  PUB(dcdc_bus_current, 3, 100.0f);
  PUB(dcdc_battery_port_voltage, 5, 100.0f);
  PUB(dcdc_battery_current, 7, 100.0f);
  PUB(dcdc_bus_negative_voltage, 9, 100.0f);
  PUB(dcdc_battery_negative_voltage, 11, 100.0f);
  PUB(dcdc_discharge_bus_voltage_set, 13, 100.0f);
  PUB(dcdc_discharge_bus_current_set, 15, 100.0f);
  PUB(dcdc_discharge_bus_power_set, 17, 100.0f);
  PUB(dcdc_charging_battery_voltage_set, 19, 100.0f);
  PUB(dcdc_charge_current_set, 21, 100.0f);
  PUB(dcdc_charging_battery_power_set, 23, 100.0f);
  PUB(dcdc_bus_voltage_dynamic, 81, 100.0f);
  PUB(dcdc_bus_voltage_ladder, 83, 100.0f);
  PUB(dcdc_depth_dod, 85, 100.0f);
  PUB(dcdc_vbus_set_max_autoself, 89, 100.0f);
#undef PUB
  const uint16_t reported_modbus_address = this->read_u16_(&p[87]);
  if (pack.dcdc_modbus_address != nullptr && pack.modbus_address != 0)
    pack.dcdc_modbus_address->publish_state(pack.modbus_address);
  if (pack.modbus_address == 0)
    ESP_LOGV(TAG,
             "Pack %u DCDC reports default Modbus %u; waiting for barcode discovery",
             pack.address, reported_modbus_address);
}

void SmartliBms::parse_telemetry_(SmartliPack &pack, const uint8_t *p,
                                  size_t n) {
  size_t offset = 0;
  std::array<uint16_t, 15> cells{};
  size_t cell_count = 0;
  float full = 0, state_of_charge = 0;
  bool have_full = false, have_state_of_charge = false;
  while (offset + 2 <= n) {
    const uint8_t id = p[offset++];
    const uint8_t count = p[offset++];
    const uint8_t width = this->field_width_(id);
    const size_t bytes = static_cast<size_t>(count) * width;
    if (width == 0 || offset + bytes > n)
      return;
    if (id == 0x01) {
      cell_count = std::min<size_t>(count, cells.size());
      for (size_t i = 0; i < cell_count; i++) {
        cells[i] = this->read_u16_(&p[offset + i * 2]);
        if (pack.cell_voltages[i] != nullptr)
          pack.cell_voltages[i]->publish_state(cells[i] / 1000.0f);
      }
    } else if (id == 0x02 && pack.current != nullptr) {
      pack.current->publish_state(
          (static_cast<int32_t>(this->read_u16_(&p[offset])) - 30000) / 100.0f);
    } else if (id == 0x03) {
      state_of_charge = this->read_u16_(&p[offset]) / 100.0f;
      have_state_of_charge = true;
      if (pack.state_of_charge != nullptr)
        pack.state_of_charge->publish_state(state_of_charge);
    } else if (id == 0x04) {
      full = this->read_u16_(&p[offset]) / 100.0f;
      have_full = true;
      if (pack.full_capacity != nullptr)
        pack.full_capacity->publish_state(full);
    } else if (id == 0x06 && count >= 5) {
      for (size_t i = 0; i < 5; i++) {
        pack.alarm_values[i] = this->read_u16_(&p[offset + i * 2]);
        if (pack.alarm_status[i] != nullptr)
          pack.alarm_status[i]->publish_state(pack.alarm_values[i]);
      }
      this->publish_status_(pack);
    } else if (id == 0x08 && pack.pack_voltage != nullptr) {
      pack.pack_voltage->publish_state(this->read_u16_(&p[offset]) / 100.0f);
    } else if (id == 0x09 && pack.state_of_health != nullptr) {
      pack.state_of_health->publish_state(this->read_u16_(&p[offset]) / 100.0f);
    } else if (id == 0x0B && pack.total_charged_ah != nullptr) {
      pack.total_charged_ah->publish_state(this->read_u32_(&p[offset]));
    } else if (id == 0x0C && pack.total_discharged_ah != nullptr) {
      pack.total_discharged_ah->publish_state(this->read_u32_(&p[offset]));
    } else if (id == 0x11 && pack.bus_voltage != nullptr) {
      pack.bus_voltage->publish_state(this->read_u16_(&p[offset]) / 100.0f);
    }
    offset += bytes;
  }
  if (have_full && have_state_of_charge && pack.remaining_capacity != nullptr)
    pack.remaining_capacity->publish_state(full * state_of_charge / 100.0f);
  if (cell_count > 0) {
    auto begin = cells.begin();
    auto end = cells.begin() + cell_count;
    const uint16_t lo = *std::min_element(begin, end);
    const uint16_t hi = *std::max_element(begin, end);
    if (pack.cell_min_voltage != nullptr)
      pack.cell_min_voltage->publish_state(lo / 1000.0f);
    if (pack.cell_max_voltage != nullptr)
      pack.cell_max_voltage->publish_state(hi / 1000.0f);
    if (pack.cell_delta_voltage != nullptr)
      pack.cell_delta_voltage->publish_state((hi - lo) / 1000.0f);
  }
}

uint16_t SmartliBms::read_u16_(const uint8_t *data) const {
  return (static_cast<uint16_t>(data[0]) << 8) | data[1];
}
uint16_t SmartliBms::crc16_(const uint8_t *data, size_t length) const {
  uint16_t crc = 0xFFFF;
  while (length--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++)
      crc = (crc & 1) ? static_cast<uint16_t>((crc >> 1) ^ 0xA001)
                      : static_cast<uint16_t>(crc >> 1);
  }
  return crc;
}
uint32_t SmartliBms::read_u32_(const uint8_t *data) const {
  return (static_cast<uint32_t>(data[0]) << 24) |
         (static_cast<uint32_t>(data[1]) << 16) |
         (static_cast<uint32_t>(data[2]) << 8) | data[3];
}
uint8_t SmartliBms::field_width_(uint8_t id) const {
  if (id >= 0x01 && id <= 0x0A) return 2;
  if (id >= 0x0B && id <= 0x10) return 4;
  if (id == 0x11 || id == 0x12) return 2;
  return 0;
}
int8_t SmartliBms::hex_nibble_(uint8_t value) const {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'A' && value <= 'F') return value - 'A' + 10;
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  return -1;
}
bool SmartliBms::decode_hex_byte_(size_t offset, uint8_t *value) const {
  if (offset + 1 >= this->frame_.size()) return false;
  const int8_t hi = this->hex_nibble_(this->frame_[offset]);
  const int8_t lo = this->hex_nibble_(this->frame_[offset + 1]);
  if (hi < 0 || lo < 0) return false;
  *value = static_cast<uint8_t>((hi << 4) | lo);
  return true;
}
std::string SmartliBms::normalize_barcode_(const uint8_t *data,
                                           size_t length) const {
  std::string result;
  for (size_t i = 0; i < length; i++)
    if (data[i] != 0 && !std::isspace(data[i]))
      result.push_back(static_cast<char>(data[i]));
  return result;
}
void SmartliBms::reset_frame_() {
  this->frame_.clear();
  this->expected_frame_length_ = 0;
  this->ascii_frame_ = false;
  this->modbus_echo_ = false;
}

}  // namespace smartli_bms
}  // namespace esphome
