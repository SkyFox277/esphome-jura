#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace jura_coffee {

class JuraCoffee : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  // Sensor setters
  void set_tray_missing_sensor(binary_sensor::BinarySensor *s) { tray_missing_ = s; }
  void set_tank_empty_sensor(binary_sensor::BinarySensor *s) { tank_empty_ = s; }
  void set_need_clean_sensor(binary_sensor::BinarySensor *s) { need_clean_ = s; }
  void set_num_single_espresso_sensor(sensor::Sensor *s) { num_single_espresso_ = s; }
  void set_num_double_espresso_sensor(sensor::Sensor *s) { num_double_espresso_ = s; }
  void set_num_coffee_sensor(sensor::Sensor *s) { num_coffee_ = s; }
  void set_num_double_coffee_sensor(sensor::Sensor *s) { num_double_coffee_ = s; }
  void set_num_clean_sensor(sensor::Sensor *s) { num_clean_ = s; }

  // IC: bit-position configuration (model-specific)
  void set_ic_tray_bit(uint8_t bit) { ic_tray_bit_ = bit; }
  void set_ic_tank_bit(uint8_t bit) { ic_tank_bit_ = bit; }
  void set_ic_need_clean_bit(uint8_t bit) { ic_need_clean_bit_ = bit; }
  // Some models (e.g. F50): bit=1 means tray PRESENT → invert to get tray_missing
  void set_ic_tray_inverted(bool inv) { ic_tray_inverted_ = inv; }

  // Send a raw Jura command (e.g. "AN:01", "FA:02", "IC:")
  void send_command(const std::string &command);

 protected:
  // Jura protocol: encodes ASCII command, sends over UART, returns response
  std::string cmd2jura(const std::string &command);

  void read_sensors();   // IC:  — tray / tank / need_clean bits
  void read_status();    // RT:0000 — EEPROM counters

  binary_sensor::BinarySensor *tray_missing_{nullptr};
  binary_sensor::BinarySensor *tank_empty_{nullptr};
  binary_sensor::BinarySensor *need_clean_{nullptr};
  sensor::Sensor *num_single_espresso_{nullptr};
  sensor::Sensor *num_double_espresso_{nullptr};
  sensor::Sensor *num_coffee_{nullptr};
  sensor::Sensor *num_double_coffee_{nullptr};
  sensor::Sensor *num_clean_{nullptr};

  // IC: bit positions — defaults match most Jura models
  uint8_t ic_tray_bit_{4};
  uint8_t ic_tank_bit_{5};
  uint8_t ic_need_clean_bit_{0};
  bool    ic_tray_inverted_{false};  // set true for F50: bit=1 means tray present

  // Every Nth update() call read EEPROM counters (they change slowly)
  uint8_t update_counter_{0};
};

template<typename... Ts>
class SendCommandAction : public Action<Ts...>, public Parented<JuraCoffee> {
 public:
  TEMPLATABLE_VALUE(std::string, command)
  void play(Ts... x) override { this->parent_->send_command(this->command_.value(x...)); }
};

}  // namespace jura_coffee
}  // namespace esphome
