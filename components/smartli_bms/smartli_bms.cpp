#include "smartli_bms.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace smartli_bms {

static const char *const TAG = "smartli_bms";

void SmartliBmsSelect::control(const std::string &option) {
  if (this->parent_ == nullptr)
    return;
  uint16_t reg = 0;
  uint16_t value = 0;
  switch (this->type_) {
    case VBUS_DISCHARGE:
      reg = 0x1010;
      value = static_cast<uint16_t>(std::strtof(option.c_str(), nullptr) * 100);
      break;
    case VBUS_DOD:
      reg = 0x1014;
      value = static_cast<uint16_t>(std::strtof(option.c_str(), nullptr) * 100);
      break;
    case IBUS_PERCENT:
      reg = 0x1011;
      value = static_cast<uint16_t>(std::atoi(option.c_str()) * 100);
      break;
    case PBUS_PERCENT:
      reg = 0x1012;
      value = static_cast<uint16_t>(std::atoi(option.c_str()) * 100);
      break;
    case AVG_CHARGE_PERCENT:
      reg = 0x1013;
      value = static_cast<uint16_t>(std::atoi(option.c_str()) * 100);
      break;
    case DOD_PERCENT:
      reg = 0x1015;
      value = static_cast<uint16_t>(std::atoi(option.c_str()) * 100);
      break;
    case CHARGING_LOOP:
      reg = 0x107D;
      value = option == "Enable" ? 0x0001 : 0x0055;
      break;
    case DISCHARGE_LOOP:
      reg = 0x107E;
      value = option == "Enable" ? 0x0001 : 0x0055;
      break;
    case MODE_ALL:
      this->parent_->queue_mode_write_all(
          option == "Constant" ? 0x0101 : 0x0303, this, option);
      return;
  }
  this->parent_->queue_modbus_write(this->address_, reg, value, this, option);
}

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
DEFINE_SETTER(cell_average_voltage)
DEFINE_SETTER(power)
DEFINE_SETTER(max_temperature)
DEFINE_SETTER(mos_temperature)
DEFINE_SETTER(cycle_count)
DEFINE_SETTER(dcdc_bus_current)
DEFINE_SETTER(dcdc_discharge_bus_voltage_set)
DEFINE_SETTER(dcdc_discharge_bus_current_set)
DEFINE_SETTER(dcdc_discharge_bus_power_set)
DEFINE_SETTER(dcdc_charging_battery_voltage_set)
DEFINE_SETTER(dcdc_charge_current_set)
DEFINE_SETTER(dcdc_charging_battery_power_set)
DEFINE_SETTER(dcdc_bus_voltage_ladder)
DEFINE_SETTER(dcdc_depth_dod)
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

void SmartliBms::set_temperature_sensor(uint8_t address, size_t index,
                                        sensor::Sensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr && index < pack->temperatures.size())
    pack->temperatures[index] = value;
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

void SmartliBms::set_modbus_address_text_sensor(
    uint8_t address, text_sensor::TextSensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr)
    pack->modbus_address_sensor = value;
}

void SmartliBms::set_status_text_sensor(
    uint8_t address, text_sensor::TextSensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr)
    pack->status_sensor = value;
}

void SmartliBms::set_mode_text_sensor(
    uint8_t address, text_sensor::TextSensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr)
    pack->mode_sensor = value;
}

void SmartliBms::set_last_update_text_sensor(
    uint8_t address, text_sensor::TextSensor *value) {
  auto *pack = this->find_pack_(address);
  if (pack != nullptr)
    pack->last_update_sensor = value;
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
  ESP_LOGCONFIG(TAG, "  Poll interval: %lu ms",
                static_cast<unsigned long>(this->get_update_interval()));
  ESP_LOGCONFIG(TAG, "  DCDC interval: %lu ms",
                static_cast<unsigned long>(this->dcdc_update_interval_));
  ESP_LOGCONFIG(TAG, "  Delay between packs: %lu ms",
                static_cast<unsigned long>(this->pack_delay_));
  ESP_LOGCONFIG(TAG, "  Delay between requests: %lu ms",
                static_cast<unsigned long>(this->request_delay_));
  for (const auto &pack : this->packs_) {
    ESP_LOGCONFIG(TAG, "  Pack %u: Modbus %u", pack.address,
                  pack.modbus_address);
  }
  LOG_PIN("  RS485 flow control pin: ", this->flow_control_pin_);
  this->check_uart_settings(9600);
}

void SmartliBms::update() {
  if (this->phase_ != Phase::IDLE || this->waiting_for_request_ ||
      this->packs_.empty()) {
    ESP_LOGW(TAG, "Skipping poll: previous multi-pack cycle is still active");
    return;
  }
  this->pack_index_ = 0;
  this->begin_pack_();
}

void SmartliBms::schedule_phase_(Phase next, std::function<void()> action) {
  this->phase_ = Phase::IDLE;
  this->waiting_for_request_ = true;
  this->set_timeout("next_request", this->request_delay_,
                    [this, next, action]() {
                      this->waiting_for_request_ = false;
                      this->phase_ = next;
                      action();
                    });
}

void SmartliBms::queue_modbus_write(uint8_t pack_address,
                                     uint16_t register_address,
                                     uint16_t value,
                                     SmartliBmsSelect *source,
                                     const std::string &option) {
  auto *pack = this->find_pack_(pack_address);
  if (pack == nullptr || pack->modbus_address == 0) {
    ESP_LOGW(TAG, "Cannot write pack %u: Modbus address unavailable",
             pack_address);
    return;
  }
  if (register_address == 0x107D || register_address == 0x107E)
    pack->loops_loaded = false;
  this->pending_writes_.push_back(
      {pack_address, register_address, value, source, option});
  if (this->phase_ == Phase::IDLE && !this->waiting_for_request_)
    this->begin_pending_write_();
}

void SmartliBms::queue_mode_write_all(uint16_t value,
                                      SmartliBmsSelect *source,
                                      const std::string &option) {
  size_t queued = 0;
  for (auto &pack : this->packs_) {
    if (pack.modbus_address == 0)
      continue;
    pack.mode_loaded = false;
    this->pending_writes_.push_back(
        {pack.address, 0x1016, value, nullptr, option});
    queued++;
  }
  if (queued != 0)
    this->pending_writes_.back().source = source;
  if (this->phase_ == Phase::IDLE && !this->waiting_for_request_ && queued != 0)
    this->begin_pending_write_();
}

void SmartliBms::set_config_select(uint8_t address, SmartliSelectType type,
                                   SmartliBmsSelect *value) {
  if (type == MODE_ALL) {
    this->mode_select_ = value;
    return;
  }
  auto *pack = this->find_pack_(address);
  const size_t index = static_cast<size_t>(type);
  if (pack != nullptr && index < pack->config_selects.size())
    pack->config_selects[index] = value;
}

void SmartliBms::begin_pending_write_() {
  while (!this->pending_writes_.empty()) {
    auto *pack =
        this->find_pack_(this->pending_writes_.front().pack_address);
    if (pack != nullptr && pack->modbus_address != 0) {
      const auto &write = this->pending_writes_.front();
      this->phase_ = Phase::MODBUS_WRITE;
      this->send_modbus_write_(pack->modbus_address, write.register_address,
                               write.value);
      return;
    }
    this->pending_writes_.erase(this->pending_writes_.begin());
  }
  this->phase_ = Phase::IDLE;
}

void SmartliBms::begin_pack_() {
  if (this->pack_index_ >= this->packs_.size()) {
    if (!this->discovery_completed_) {
      bool barcodes_ready = true;
      bool address_missing = false;
      for (const auto &pack : this->packs_) {
        barcodes_ready &= !pack.pcb_barcode.empty() && !pack.pack_barcode.empty();
        address_missing |= pack.modbus_address == 0;
      }
      if (address_missing && barcodes_ready) {
        this->begin_modbus_discovery_();
        return;
      }
      if (!address_missing)
        this->discovery_completed_ = true;
    }
    if (!this->pending_writes_.empty()) {
      this->begin_pending_write_();
      return;
    }
    this->phase_ = Phase::IDLE;
    ESP_LOGD(TAG, "Multi-pack polling cycle completed");
    return;
  }
  auto &pack = this->packs_[this->pack_index_];
  if (pack.modbus_address_sensor != nullptr && pack.modbus_address != 0)
    pack.modbus_address_sensor->publish_state(
        std::to_string(pack.modbus_address));
  this->phase_ = Phase::TELEMETRY;
  this->send_binary_request_(pack.address, 0x01);
}

void SmartliBms::advance_(bool response_received) {
  if (this->phase_ == Phase::MODBUS_WRITE) {
    if (!this->pending_writes_.empty()) {
      auto &completed = this->pending_writes_.front();
      if (response_received && completed.source != nullptr)
        completed.source->publish_state(completed.option);
      if (!response_received) {
        if (completed.retries == 0) {
          completed.retries++;
          ESP_LOGW(TAG,
                   "Modbus write timeout for pack %u register 0x%04X; retrying",
                   completed.pack_address, completed.register_address);
          this->phase_ = Phase::IDLE;
          this->waiting_for_request_ = true;
          this->set_timeout("retry_modbus_write", 150, [this]() {
            this->waiting_for_request_ = false;
            this->begin_pending_write_();
          });
          return;
        }
        ESP_LOGE(TAG,
                 "Modbus write failed for pack %u register 0x%04X after retry",
                 completed.pack_address, completed.register_address);
      }
      this->pending_writes_.erase(this->pending_writes_.begin());
    }
    this->phase_ = Phase::IDLE;
    if (!this->pending_writes_.empty()) {
      // Modbus RTU only needs a short silent interval between write frames.
      // Do not apply the one-second telemetry request delay to UI writes.
      this->waiting_for_request_ = true;
      this->set_timeout("next_modbus_write", 100, [this]() {
        this->waiting_for_request_ = false;
        this->begin_pending_write_();
      });
    } else if (this->resume_poll_after_writes_) {
      this->resume_poll_after_writes_ = false;
      this->waiting_for_request_ = true;
      this->set_timeout("resume_poll", this->pack_delay_, [this]() {
        this->waiting_for_request_ = false;
        this->begin_pack_();
      });
    }
    return;
  }
  if (this->phase_ == Phase::DISCOVERY_MODBUS_PCB ||
      this->phase_ == Phase::DISCOVERY_MODBUS_PACK) {
    this->advance_modbus_discovery_(response_received);
    return;
  }
  if (this->pack_index_ >= this->packs_.size()) {
    this->phase_ = Phase::IDLE;
    return;
  }
  auto &pack = this->packs_[this->pack_index_];
  const Phase completed = this->phase_;
  if (!response_received) {
    ESP_LOGW(TAG, "Timeout in phase %u for pack %u",
             static_cast<uint8_t>(completed), pack.address);
  }

  if (completed == Phase::TELEMETRY) {
    const uint32_t now = millis();
    if (pack.last_dcdc_at == 0 ||
        now - pack.last_dcdc_at >= this->dcdc_update_interval_) {
      pack.last_dcdc_at = now;
      this->schedule_phase_(Phase::DCDC, [this, address = pack.address]() {
        this->send_dcdc_request_(address);
      });
      return;
    }
  }

  if (completed == Phase::TELEMETRY || completed == Phase::DCDC) {
    if (pack.pcb_barcode.empty()) {
      this->schedule_phase_(Phase::PCB_BARCODE,
                            [this, address = pack.address]() {
                              this->send_binary_request_(address, 0x42);
                            });
      return;
    }
  }

  if (completed == Phase::PCB_BARCODE ||
      completed == Phase::TELEMETRY || completed == Phase::DCDC) {
    if (pack.pack_barcode.empty()) {
      this->schedule_phase_(Phase::PACK_BARCODE,
                            [this, address = pack.address]() {
                              this->send_pack_barcode_request_(address);
                            });
      return;
    }
  }

  if (completed == Phase::PACK_BARCODE ||
      completed == Phase::PCB_BARCODE ||
      completed == Phase::TELEMETRY || completed == Phase::DCDC) {
    if (pack.modbus_pcb_barcode_sensor != nullptr &&
        pack.modbus_pcb_barcode.empty() && pack.modbus_address != 0) {
      this->schedule_phase_(
          Phase::MODBUS_PCB_BARCODE,
          [this, address = pack.modbus_address]() {
            this->send_modbus_read_(address, 0x104D, 10);
          });
      return;
    }
  }

  if (completed == Phase::MODBUS_PCB_BARCODE ||
      completed == Phase::PACK_BARCODE ||
      completed == Phase::PCB_BARCODE ||
      completed == Phase::TELEMETRY || completed == Phase::DCDC) {
    if (pack.modbus_pack_barcode_sensor != nullptr &&
        pack.modbus_pack_barcode.empty() && pack.modbus_address != 0) {
      this->schedule_phase_(
          Phase::MODBUS_PACK_BARCODE,
          [this, address = pack.modbus_address]() {
            this->send_modbus_read_(address, 0x1065, 10);
          });
      return;
    }
  }

  if (completed == Phase::MODBUS_PACK_BARCODE ||
      completed == Phase::MODBUS_PCB_BARCODE ||
      completed == Phase::PACK_BARCODE ||
      completed == Phase::PCB_BARCODE ||
      completed == Phase::TELEMETRY || completed == Phase::DCDC) {
    if ((this->mode_select_ != nullptr || pack.mode_sensor != nullptr) &&
        !pack.mode_loaded &&
        pack.modbus_address != 0) {
      this->schedule_phase_(Phase::MODBUS_CONFIG_MODE,
                            [this, address = pack.modbus_address]() {
                              this->send_modbus_read_(address, 0x1016, 1);
                            });
      return;
    }
  }

  if (completed == Phase::MODBUS_CONFIG_MODE ||
      completed == Phase::MODBUS_PACK_BARCODE ||
      completed == Phase::MODBUS_PCB_BARCODE ||
      completed == Phase::PACK_BARCODE ||
      completed == Phase::PCB_BARCODE ||
      completed == Phase::TELEMETRY || completed == Phase::DCDC) {
    if (!pack.loops_loaded && pack.modbus_address != 0 &&
        (pack.config_selects[CHARGING_LOOP] != nullptr ||
         pack.config_selects[DISCHARGE_LOOP] != nullptr)) {
      this->schedule_phase_(Phase::MODBUS_CONFIG_LOOPS,
                            [this, address = pack.modbus_address]() {
                              this->send_modbus_read_(address, 0x107D, 2);
                            });
      return;
    }
  }

  this->pack_index_++;
  if (!this->pending_writes_.empty()) {
    // The current pack is complete, so this is a safe point to prioritize a
    // user write without waiting for every remaining pack.
    this->phase_ = Phase::IDLE;
    this->resume_poll_after_writes_ = true;
    this->waiting_for_request_ = true;
    this->set_timeout("first_modbus_write", 100, [this]() {
      this->waiting_for_request_ = false;
      this->begin_pending_write_();
    });
    return;
  }
  // Prevent loop() from treating the intentional inter-pack delay as another
  // timeout for the phase that has just completed.
  this->phase_ = Phase::IDLE;
  this->waiting_for_request_ = true;
  this->set_timeout("next_pack", this->pack_delay_,
                    [this]() {
                      this->waiting_for_request_ = false;
                      this->begin_pack_();
                    });
}

uint8_t SmartliBms::discovery_candidate_address_(size_t index) const {
  // Valid ranges documented by SmartLi are 214-221 and 224-231.
  return index < 8 ? static_cast<uint8_t>(214 + index)
                   : static_cast<uint8_t>(224 + index - 8);
}

void SmartliBms::begin_modbus_discovery_() {
  this->discovery_candidate_index_ = 0;
  this->discovery_candidate_count_ =
      std::min<size_t>(this->packs_.size() + 3, 16);
  this->discovery_match_count_ = 0;
  for (const auto &pack : this->packs_)
    if (pack.modbus_address != 0)
      this->discovery_match_count_++;
  this->discovery_pcb_barcode_.clear();
  this->discovery_pack_barcode_.clear();
  const uint8_t candidate = this->discovery_candidate_address_(0);
  ESP_LOGI(TAG, "Starting Modbus discovery: %u candidates for %u packs",
           this->discovery_candidate_count_, this->packs_.size());
  this->phase_ = Phase::DISCOVERY_MODBUS_PCB;
  this->send_modbus_read_(candidate, 0x104D, 10);
}

void SmartliBms::advance_modbus_discovery_(bool response_received) {
  const uint8_t candidate =
      this->discovery_candidate_address_(this->discovery_candidate_index_);
  if (!response_received) {
    ESP_LOGV(TAG, "No Modbus barcode response from address %u", candidate);
  }

  if (this->phase_ == Phase::DISCOVERY_MODBUS_PCB) {
    if (!response_received) {
      this->finish_modbus_discovery_candidate_();
      return;
    }
    this->schedule_phase_(Phase::DISCOVERY_MODBUS_PACK,
                          [this, candidate]() {
                            this->send_modbus_read_(candidate, 0x1065, 10);
                          });
    return;
  }

  this->finish_modbus_discovery_candidate_();
}

void SmartliBms::finish_modbus_discovery_candidate_() {
  const uint8_t candidate =
      this->discovery_candidate_address_(this->discovery_candidate_index_);
  if (!this->discovery_pcb_barcode_.empty() &&
      !this->discovery_pack_barcode_.empty()) {
    for (auto &pack : this->packs_) {
      if (pack.modbus_address == 0 &&
          pack.pcb_barcode == this->discovery_pcb_barcode_ &&
          pack.pack_barcode == this->discovery_pack_barcode_) {
        pack.modbus_address = candidate;
        this->discovery_match_count_++;
        if (pack.modbus_address_sensor != nullptr)
          pack.modbus_address_sensor->publish_state(std::to_string(candidate));
        if (pack.modbus_pcb_barcode_sensor != nullptr)
          pack.modbus_pcb_barcode_sensor->publish_state(
              this->discovery_pcb_barcode_);
        if (pack.modbus_pack_barcode_sensor != nullptr)
          pack.modbus_pack_barcode_sensor->publish_state(
              this->discovery_pack_barcode_);
        pack.modbus_pcb_barcode = this->discovery_pcb_barcode_;
        pack.modbus_pack_barcode = this->discovery_pack_barcode_;
        ESP_LOGI(TAG, "Pack %u matched Modbus %u using both barcodes",
                 pack.address, candidate);
        break;
      }
    }
  }

  this->discovery_pcb_barcode_.clear();
  this->discovery_pack_barcode_.clear();
  if (this->discovery_match_count_ >= this->packs_.size()) {
    this->discovery_completed_ = true;
    this->phase_ = Phase::IDLE;
    ESP_LOGI(TAG, "Modbus discovery completed: all %u packs matched",
             this->packs_.size());
    return;
  }
  this->discovery_candidate_index_++;
  if (this->discovery_candidate_index_ >= this->discovery_candidate_count_) {
    this->discovery_completed_ = true;
    this->phase_ = Phase::IDLE;
    ESP_LOGI(TAG, "Modbus discovery completed: %u/%u packs matched",
             this->discovery_match_count_, this->packs_.size());
    return;
  }

  const uint8_t next =
      this->discovery_candidate_address_(this->discovery_candidate_index_);
  this->schedule_phase_(Phase::DISCOVERY_MODBUS_PCB, [this, next]() {
    this->send_modbus_read_(next, 0x104D, 10);
  });
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

void SmartliBms::send_modbus_write_(uint8_t address,
                                     uint16_t register_address,
                                     uint16_t value) {
  uint8_t request[8] = {
      address, 0x06, static_cast<uint8_t>(register_address >> 8),
      static_cast<uint8_t>(register_address), static_cast<uint8_t>(value >> 8),
      static_cast<uint8_t>(value), 0, 0};
  const uint16_t crc = this->crc16_(request, 6);
  request[6] = static_cast<uint8_t>(crc);
  request[7] = static_cast<uint8_t>(crc >> 8);
  this->send_bytes_(request, sizeof(request));
  ESP_LOGI(TAG, "Modbus %u write 0x%04X = 0x%04X", address,
           register_address, value);
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
                      this->phase_ == Phase::MODBUS_PACK_BARCODE ||
                      this->phase_ == Phase::DISCOVERY_MODBUS_PCB ||
                      this->phase_ == Phase::DISCOVERY_MODBUS_PACK ||
                      this->phase_ == Phase::DISCOVERY_MODBUS_PACK ||
                      this->phase_ == Phase::MODBUS_CONFIG_MODE ||
                      this->phase_ == Phase::MODBUS_CONFIG_LOOPS ||
                      this->phase_ == Phase::MODBUS_WRITE;
  if (this->frame_.empty()) {
    if (!modbus && byte != 0x7E)
      return;
    if (modbus) {
      uint8_t expected = 0;
      if (this->phase_ == Phase::MODBUS_WRITE &&
          !this->pending_writes_.empty()) {
        auto *pack =
            this->find_pack_(this->pending_writes_.front().pack_address);
        expected = pack != nullptr ? pack->modbus_address : 0;
      } else {
        expected =
          this->phase_ == Phase::DISCOVERY_MODBUS_PCB ||
                  this->phase_ == Phase::DISCOVERY_MODBUS_PACK
              ? this->discovery_candidate_address_(
                    this->discovery_candidate_index_)
              : this->packs_[this->pack_index_].modbus_address;
      }
      if (byte != expected)
        return;
    }
    this->frame_.push_back(byte);
    return;
  }

  this->frame_.push_back(byte);
  if (modbus) {
    if (this->phase_ == Phase::MODBUS_WRITE && this->frame_.size() == 2 &&
        this->frame_[1] == 0x06)
      this->expected_frame_length_ = 8;
    else if (this->frame_.size() == 2 && (this->frame_[1] & 0x80))
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
  if (this->phase_ == Phase::MODBUS_WRITE) {
    if (this->frame_[1] != 0x06 || this->frame_.size() != 8 ||
        this->pending_writes_.empty())
      return false;
    const auto &pending = this->pending_writes_.front();
    return this->read_u16_(&this->frame_[2]) == pending.register_address &&
           this->read_u16_(&this->frame_[4]) == pending.value;
  }
  if (this->frame_[1] != 0x03)
    return true;
  if (this->phase_ == Phase::MODBUS_CONFIG_MODE) {
    if (this->frame_[2] != 2 || this->frame_.size() != 7)
      return false;
    auto &pack = this->packs_[this->pack_index_];
    const uint16_t value = this->read_u16_(&this->frame_[3]);
    if (this->mode_select_ != nullptr) {
      if (value == 0x0101)
        this->mode_select_->publish_state("Constant");
      else if (value == 0x0303)
        this->mode_select_->publish_state("Battery");
    }
    if (pack.mode_sensor != nullptr) {
      if (value == 0x0101)
        pack.mode_sensor->publish_state("Constant");
      else if (value == 0x0303)
        pack.mode_sensor->publish_state("Battery");
      else
        pack.mode_sensor->publish_state("Unknown");
    }
    pack.mode_loaded = true;
    return true;
  }
  if (this->phase_ == Phase::MODBUS_CONFIG_LOOPS) {
    if (this->frame_[2] != 4 || this->frame_.size() != 9)
      return false;
    auto &pack = this->packs_[this->pack_index_];
    auto publish = [&](size_t index, uint16_t value) {
      if (pack.config_selects[index] != nullptr)
        pack.config_selects[index]->publish_state(
            value == 0x0001 ? "Enable" : "Disable");
    };
    publish(CHARGING_LOOP, this->read_u16_(&this->frame_[3]));
    publish(DISCHARGE_LOOP, this->read_u16_(&this->frame_[5]));
    pack.loops_loaded = true;
    return true;
  }
  if (this->frame_[2] != 20 || this->frame_.size() != 25)
    return false;

  const std::string barcode = this->normalize_barcode_(&this->frame_[3], 20);
  if (this->phase_ == Phase::DISCOVERY_MODBUS_PCB) {
    this->discovery_pcb_barcode_ = barcode;
  } else if (this->phase_ == Phase::DISCOVERY_MODBUS_PACK) {
    this->discovery_pack_barcode_ = barcode;
  } else if (this->phase_ == Phase::MODBUS_PCB_BARCODE) {
    auto &pack = this->packs_[this->pack_index_];
    pack.modbus_pcb_barcode = barcode;
    if (pack.modbus_pcb_barcode_sensor != nullptr)
      pack.modbus_pcb_barcode_sensor->publish_state(barcode);
    ESP_LOGI(TAG, "Pack %u Modbus PCB barcode: %s", pack.address,
             barcode.c_str());
  } else if (this->phase_ == Phase::MODBUS_PACK_BARCODE) {
    auto &pack = this->packs_[this->pack_index_];
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
  PUB(dcdc_bus_current, 3, 100.0f);
  PUB(dcdc_discharge_bus_voltage_set, 13, 100.0f);
  PUB(dcdc_discharge_bus_current_set, 15, 100.0f);
  PUB(dcdc_discharge_bus_power_set, 17, 100.0f);
  PUB(dcdc_charging_battery_voltage_set, 19, 100.0f);
  PUB(dcdc_charge_current_set, 21, 100.0f);
  PUB(dcdc_charging_battery_power_set, 23, 100.0f);
  PUB(dcdc_bus_voltage_ladder, 83, 100.0f);
  PUB(dcdc_depth_dod, 85, 100.0f);
  PUB(dcdc_vbus_set_max_autoself, 89, 100.0f);
#undef PUB
  auto publish_voltage = [&](size_t index, size_t offset) {
    auto *item = pack.config_selects[index];
    if (item == nullptr)
      return;
    char option[12];
    std::snprintf(option, sizeof(option), "%.1fV",
                  this->read_u16_(&p[offset]) / 100.0f);
    item->publish_state(option);
  };
  auto publish_percent = [&](size_t index, size_t offset) {
    auto *item = pack.config_selects[index];
    if (item == nullptr)
      return;
    char option[12];
    std::snprintf(option, sizeof(option), "%u%%",
                  this->read_u16_(&p[offset]) / 100);
    item->publish_state(option);
  };
  publish_voltage(VBUS_DISCHARGE, 13);
  publish_percent(IBUS_PERCENT, 15);
  publish_percent(PBUS_PERCENT, 17);
  publish_percent(AVG_CHARGE_PERCENT, 21);
  publish_voltage(VBUS_DOD, 83);
  publish_percent(DOD_PERCENT, 85);
  const uint16_t reported_modbus_address = this->read_u16_(&p[87]);
  if (pack.modbus_address == 0) {
    ESP_LOGV(TAG,
             "Pack %u DCDC reports default Modbus %u; waiting for barcode discovery",
             pack.address, reported_modbus_address);
  }
}

void SmartliBms::parse_telemetry_(SmartliPack &pack, const uint8_t *p,
                                  size_t n) {
  size_t offset = 0;
  std::array<uint16_t, 15> cells{};
  size_t cell_count = 0;
  float full = 0, state_of_charge = 0;
  float current = 0, pack_voltage = 0;
  bool have_full = false, have_state_of_charge = false;
  bool have_current = false, have_pack_voltage = false;
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
    } else if (id == 0x02) {
      current =
          (static_cast<int32_t>(this->read_u16_(&p[offset])) - 30000) / 100.0f;
      have_current = true;
      if (pack.current != nullptr)
        pack.current->publish_state(current);
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
    } else if (id == 0x05 && count >= 8) {
      std::array<int16_t, 8> temperatures{};
      for (size_t i = 0; i < temperatures.size(); i++) {
        const uint16_t raw = this->read_u16_(&p[offset + i * 2]);
        temperatures[i] = static_cast<int16_t>(raw & 0x00FFU) - 50;
        if (pack.temperatures[i] != nullptr)
          pack.temperatures[i]->publish_state(temperatures[i]);
      }
      if (pack.max_temperature != nullptr) {
        pack.max_temperature->publish_state(
            *std::max_element(temperatures.begin(), temperatures.begin() + 4));
      }
      if (pack.mos_temperature != nullptr) {
        pack.mos_temperature->publish_state(
            std::max(temperatures[5], temperatures[6]));
      }
    } else if (id == 0x06 && count >= 5) {
      for (size_t i = 0; i < 5; i++) {
        pack.alarm_values[i] = this->read_u16_(&p[offset + i * 2]);
        if (pack.alarm_status[i] != nullptr)
          pack.alarm_status[i]->publish_state(pack.alarm_values[i]);
      }
      this->publish_status_(pack);
    } else if (id == 0x07 && pack.cycle_count != nullptr) {
      pack.cycle_count->publish_state(this->read_u16_(&p[offset]));
    } else if (id == 0x08) {
      pack_voltage = this->read_u16_(&p[offset]) / 100.0f;
      have_pack_voltage = true;
      if (pack.pack_voltage != nullptr)
        pack.pack_voltage->publish_state(pack_voltage);
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
  if (have_current && have_pack_voltage && pack.power != nullptr)
    pack.power->publish_state(pack_voltage * current);
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
    if (pack.cell_average_voltage != nullptr) {
      uint32_t sum = 0;
      for (size_t i = 0; i < cell_count; i++)
        sum += cells[i];
      pack.cell_average_voltage->publish_state(
          static_cast<float>(sum) / cell_count / 1000.0f);
    }
  }
  if (pack.last_update_sensor != nullptr) {
    pack.telemetry_sequence++;
    pack.last_update_sensor->publish_state(
        "Update " + std::to_string(pack.telemetry_sequence));
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
  for (size_t i = 0; i < length; i++) {
    // SmartLi fills unused barcode bytes with '^' (0x5E) or NUL.
    if (data[i] == '^' || data[i] == 0)
      break;
    if (!std::isspace(data[i]))
      result.push_back(static_cast<char>(data[i]));
  }
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
