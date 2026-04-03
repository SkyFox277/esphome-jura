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
  void loop() override;
  void update() override;
  void dump_config() override;

  // Sensor setters
  void set_tray_missing_sensor(binary_sensor::BinarySensor *s) { tray_missing_ = s; }
  void set_tank_empty_sensor(binary_sensor::BinarySensor *s) { tank_empty_ = s; }
  void set_need_clean_sensor(binary_sensor::BinarySensor *s) { need_clean_ = s; }
  void set_ready_sensor(binary_sensor::BinarySensor *s) { ready_ = s; }
  void set_needs_rinse_sensor(binary_sensor::BinarySensor *s) { needs_rinse_ = s; }
  void set_num_single_espresso_sensor(sensor::Sensor *s) { num_single_espresso_ = s; }
  void set_num_double_espresso_sensor(sensor::Sensor *s) { num_double_espresso_ = s; }
  void set_num_coffee_sensor(sensor::Sensor *s) { num_coffee_ = s; }
  void set_num_double_coffee_sensor(sensor::Sensor *s) { num_double_coffee_ = s; }
  void set_num_clean_sensor(sensor::Sensor *s) { num_clean_ = s; }
  void set_num_rinse_sensor(sensor::Sensor *s) { num_rinse_ = s; }
  void set_num_descale_sensor(sensor::Sensor *s) { num_descale_ = s; }
  void set_num_coffees_since_clean_sensor(sensor::Sensor *s) { num_coffees_since_clean_ = s; }
  void set_last_response_sensor(text_sensor::TextSensor *s) { last_response_ = s; }

  // IC: bit-position configuration (model-specific)
  void set_ic_tray_bit(uint8_t bit) { ic_tray_bit_ = bit; }
  void set_ic_tank_bit(uint8_t bit) { ic_tank_bit_ = bit; }
  void set_ic_need_clean_bit(uint8_t bit) { ic_need_clean_bit_ = bit; }
  void set_ic_needs_rinse_bit(uint8_t bit) { ic_needs_rinse_bit_ = bit; }
  // F50: bit=1 means tray PRESENT → invert to get tray_missing
  void set_ic_tray_inverted(bool inv) { ic_tray_inverted_ = inv; }
  // F50: bit=1 means tank OK → invert to get tank_empty
  void set_ic_tank_inverted(bool inv) { ic_tank_inverted_ = inv; }
  // F50: bit=1 means no cleaning needed → invert to get need_clean
  void set_ic_need_clean_inverted(bool inv) { ic_need_clean_inverted_ = inv; }
  // F50: bit=1 means needs rinse on power off (not inverted)
  void set_ic_needs_rinse_inverted(bool inv) { ic_needs_rinse_inverted_ = inv; }

  // Send a raw Jura command (e.g. "AN:01", "FA:02", "IC:")
  void send_command(const std::string &command);

  // Debug dump control
  void start_debug_dump(uint8_t rr_start, uint8_t rr_end, uint32_t interval_ms,
                        bool poll_ic, bool poll_rt);
  void stop_debug_dump();
  void annotate_debug(const std::string &note);

 protected:
  // Jura protocol: encodes ASCII command, sends over UART, returns response
  std::string cmd2jura(const std::string &command);

  void read_sensors();   // IC:  — tray / tank / need_clean / needs_rinse bits
  void read_ready();     // RR:03 — operating temperature reached (bit 2)
  void read_status();    // RT:0000 — EEPROM counters

  // Safety: block catastrophic commands (AN:0A = EEPROM clear)
  bool is_command_blocked(const std::string &command);

  binary_sensor::BinarySensor *tray_missing_{nullptr};
  binary_sensor::BinarySensor *tank_empty_{nullptr};
  binary_sensor::BinarySensor *need_clean_{nullptr};
  binary_sensor::BinarySensor *ready_{nullptr};
  binary_sensor::BinarySensor *needs_rinse_{nullptr};
  sensor::Sensor *num_single_espresso_{nullptr};
  sensor::Sensor *num_double_espresso_{nullptr};
  sensor::Sensor *num_coffee_{nullptr};
  sensor::Sensor *num_double_coffee_{nullptr};
  sensor::Sensor *num_clean_{nullptr};
  sensor::Sensor *num_rinse_{nullptr};
  sensor::Sensor *num_descale_{nullptr};
  sensor::Sensor *num_coffees_since_clean_{nullptr};
  text_sensor::TextSensor *last_response_{nullptr};

  // IC: bit positions — defaults match most Jura models
  uint8_t ic_tray_bit_{4};
  uint8_t ic_tank_bit_{5};
  uint8_t ic_need_clean_bit_{0};
  uint8_t ic_needs_rinse_bit_{0};
  bool    ic_tray_inverted_{false};
  bool    ic_tank_inverted_{false};
  bool    ic_need_clean_inverted_{false};
  bool    ic_needs_rinse_inverted_{false};

  // Every Nth update() call read EEPROM counters (they change slowly)
  uint8_t update_counter_{0};

  // Debug dump state machine
  enum DebugPhase : uint8_t {
    PHASE_IDLE,
    PHASE_IC,
    PHASE_RR,
    PHASE_RT,
    PHASE_WAIT,
  };
  bool debug_active_{false};
  DebugPhase debug_phase_{PHASE_IDLE};
  uint8_t debug_rr_start_{0x00};
  uint8_t debug_rr_end_{0x23};
  uint8_t debug_current_rr_{0};
  uint32_t debug_interval_ms_{5000};
  uint32_t debug_wait_start_{0};
  bool debug_poll_ic_{true};
  bool debug_poll_rt_{true};

#ifdef USE_DEBUG_HTTP
  // ── REST debug interface (port 8080) ───────────────────────────────────────
  void setup_debug_http_(uint16_t port = 8080);
  void http_loop_tick_();

  enum HttpTask : uint8_t { TASK_NONE, TASK_CMD, TASK_SCAN };
  HttpTask    http_task_{TASK_NONE};
  std::string http_cmd_pending_;
  std::string http_cmd_result_;
  bool        http_result_ready_{false};

  // Scan state (RE:XXXX range)
  uint16_t scan_from_{0x00};
  uint16_t scan_to_{0xF0};
  uint16_t scan_step_{0x10};
  uint16_t scan_current_{0x00};
  std::string scan_results_json_;
#endif
};

// ── Actions ──────────────────────────────────────────────────────────────────

template<typename... Ts>
class SendCommandAction : public Action<Ts...>, public Parented<JuraCoffee> {
 public:
  TEMPLATABLE_VALUE(std::string, command)
  void play(const Ts &...x) override { this->parent_->send_command(this->command_.value(x...)); }
};

template<typename... Ts>
class StartDebugAction : public Action<Ts...>, public Parented<JuraCoffee> {
 public:
  void set_rr_start(uint8_t v) { rr_start_ = v; }
  void set_rr_end(uint8_t v) { rr_end_ = v; }
  void set_interval_ms(uint32_t v) { interval_ms_ = v; }
  void set_poll_ic(bool v) { poll_ic_ = v; }
  void set_poll_rt(bool v) { poll_rt_ = v; }

  void play(const Ts &...x) override {
    this->parent_->start_debug_dump(rr_start_, rr_end_, interval_ms_, poll_ic_, poll_rt_);
  }

 protected:
  uint8_t rr_start_{0x00};
  uint8_t rr_end_{0x23};
  uint32_t interval_ms_{5000};
  bool poll_ic_{true};
  bool poll_rt_{true};
};

template<typename... Ts>
class StopDebugAction : public Action<Ts...>, public Parented<JuraCoffee> {
 public:
  void play(const Ts &...x) override { this->parent_->stop_debug_dump(); }
};

template<typename... Ts>
class AnnotateDebugAction : public Action<Ts...>, public Parented<JuraCoffee> {
 public:
  TEMPLATABLE_VALUE(std::string, message)
  void play(const Ts &...x) override { this->parent_->annotate_debug(this->message_.value(x...)); }
};

}  // namespace jura_coffee
}  // namespace esphome
