#include "jura_coffee.h"
#include "esphome/core/log.h"

#include <cctype>

namespace esphome {
namespace jura_coffee {

static const char *const TAG = "jura_coffee";

// Jura protocol: each ASCII byte is spread across 4 UART bytes.
// Bits 2 and 5 of each UART byte carry 2 bits of the ASCII character.
// All other bits are set to 1 (0xFF mask).
//
// Encoding: for ASCII byte A, send 4 UART bytes where:
//   byte0: bit2=A[0], bit5=A[1]
//   byte1: bit2=A[2], bit5=A[3]
//   byte2: bit2=A[4], bit5=A[5]
//   byte3: bit2=A[6], bit5=A[7]
//
// Timing: 8ms after each ASCII character (matches SoftwareSerial 9600 baud).

static uint8_t encode_jura_byte(uint8_t ascii, uint8_t bit_pair) {
  // bit_pair: 0=bits[1:0], 1=bits[3:2], 2=bits[5:4], 3=bits[7:6]
  uint8_t raw = 0xFF;
  uint8_t shift = bit_pair * 2;
  if (ascii & (1 << shift))
    raw |= (1 << 2);
  else
    raw &= ~(1 << 2);
  if (ascii & (1 << (shift + 1)))
    raw |= (1 << 5);
  else
    raw &= ~(1 << 5);
  return raw;
}

std::string JuraCoffee::cmd2jura(const std::string &command) {
  // Flush any stale bytes in the RX buffer
  while (this->available())
    this->read();

  std::string out = command + "\r\n";

  // Send each ASCII character as 4 UART bytes
  for (char c : out) {
    for (uint8_t s = 0; s < 4; s++) {
      this->write_byte(encode_jura_byte(static_cast<uint8_t>(c), s));
    }
    delay(8);
  }

  // Receive response until "\r\n" or timeout
  std::string response;
  uint8_t bit_pos = 0;
  char current_byte = 0;
  int watchdog = 0;

  while (true) {
    if (this->available()) {
      uint8_t raw = this->read();
      // Decode 2 bits from this UART byte
      if (raw & (1 << 2))
        current_byte |= (1 << (bit_pos * 2));
      else
        current_byte &= ~(1 << (bit_pos * 2));
      if (raw & (1 << 5))
        current_byte |= (1 << (bit_pos * 2 + 1));
      else
        current_byte &= ~(1 << (bit_pos * 2 + 1));

      bit_pos++;
      if (bit_pos >= 4) {
        bit_pos = 0;
        response += current_byte;
        current_byte = 0;
        // Check for line ending
        if (response.size() >= 2) {
          size_t n = response.size();
          if (response[n - 2] == '\r' && response[n - 1] == '\n') {
            return response.substr(0, n - 2);
          }
        }
      }
    } else {
      delay(10);
      if (++watchdog > 100)  // ~1s timeout (was 500 = 5s, too long for WDT)
        return "";
    }
  }
}

// ── Command safety ───────────────────────────────────────────────────────────

bool JuraCoffee::is_command_blocked(const std::string &command) {
  // Normalize: trim whitespace + uppercase
  std::string normalized;
  normalized.reserve(command.size());
  for (char c : command) {
    if (c != ' ' && c != '\t')
      normalized += static_cast<char>(toupper(static_cast<unsigned char>(c)));
  }
  // AN:0A = EEPROM clear — catastrophic, irreversible
  return normalized == "AN:0A";
}

// ── Sensor reading ───────────────────────────────────────────────────────────

void JuraCoffee::read_sensors() {
  // IC: returns hex string, first byte encodes machine status bits.
  // Bit positions are model-specific (configured via machine_type or ic_*_bit keys).
  std::string result = this->cmd2jura("IC:");
  ESP_LOGD(TAG, "IC: response: '%s'", result.c_str());

  if (result.size() < 5 || result.substr(0, 3) != "ic:")
    return;

  // Parse first hex byte after "ic:"
  std::string hex_byte = result.substr(3, 2);
  uint8_t val = static_cast<uint8_t>(strtol(hex_byte.c_str(), nullptr, 16));

  bool tray_bit   = (val >> ic_tray_bit_) & 1;
  bool tank_bit   = (val >> ic_tank_bit_) & 1;
  bool clean_bit  = (val >> ic_need_clean_bit_) & 1;
  bool rinse_bit  = (val >> ic_needs_rinse_bit_) & 1;
  bool tray_missing = ic_tray_inverted_        ? !tray_bit  : tray_bit;
  bool tank_empty   = ic_tank_inverted_        ? !tank_bit  : tank_bit;
  bool need_clean   = ic_need_clean_inverted_  ? !clean_bit : clean_bit;
  bool needs_rinse  = ic_needs_rinse_inverted_ ? !rinse_bit : rinse_bit;

  ESP_LOGD(TAG, "IC: byte=0x%02X tray=%d(inv=%d)->missing=%d tank=%d(inv=%d)->empty=%d clean=%d(inv=%d)->need=%d rinse=%d(inv=%d)->needs=%d",
           val,
           tray_bit,  ic_tray_inverted_,        tray_missing,
           tank_bit,  ic_tank_inverted_,         tank_empty,
           clean_bit, ic_need_clean_inverted_,   need_clean,
           rinse_bit, ic_needs_rinse_inverted_,  needs_rinse);

  if (tray_missing_ != nullptr)
    tray_missing_->publish_state(tray_missing);
  if (tank_empty_ != nullptr)
    tank_empty_->publish_state(tank_empty);
  if (need_clean_ != nullptr)
    need_clean_->publish_state(need_clean);
  if (needs_rinse_ != nullptr)
    needs_rinse_->publish_state(needs_rinse);
}

void JuraCoffee::read_ready() {
  // RR:03 bit 2 = operating temperature reached (confirmed on F50).
  // 0x0000 = cold/off, 0x0004 = at temperature (PID mode).
  std::string result = this->cmd2jura("RR:03");
  ESP_LOGD(TAG, "RR:03 response: '%s'", result.c_str());

  if (result.size() < 7 || result.substr(0, 3) != "rr:")
    return;

  uint16_t val = static_cast<uint16_t>(strtol(result.substr(3, 4).c_str(), nullptr, 16));
  bool is_ready = (val & 0x0004) != 0;

  ESP_LOGD(TAG, "RR:03=0x%04X ready=%d", val, is_ready);

  if (ready_ != nullptr)
    ready_->publish_state(is_ready);
}

void JuraCoffee::read_status() {
  // RT:0000 returns EEPROM line: "rt:" + 64 hex chars (32 bytes / 16 words).
  // EEPROM word map (confirmed from F50 sample data + speicheradressen.txt):
  //   offset  3..6  (word 0x0000): Bezüge Normal (coffee / normal size)
  //   offset  7..10 (word 0x0001): Bezüge doppelte Normal (double coffee)
  //   offset 11..14 (word 0x0002): Bezüge klein (small / espresso on other models)
  //   offset 15..18 (word 0x0003): Bezüge 2x klein (double small)
  //   offset 31..34 (word 0x0007): Spülvorgänge (rinse count)
  //   offset 35..38 (word 0x0008): Reinigungszyklen (cleaning cycles)
  //   offset 39..42 (word 0x0009): Entkalkungszyklen (descaling cycles)
  //   offset 59..62 (word 0x000E): Bezüge seit Reinigung (coffees since cleaning)
  std::string result = this->cmd2jura("RT:0000");
  ESP_LOGD(TAG, "RT:0000 response: '%s'", result.c_str());

  if (result.size() < 19)
    return;

  auto parse = [&](size_t start, size_t len) -> long {
    return strtol(result.substr(start, len).c_str(), nullptr, 16);
  };

  if (num_single_espresso_ != nullptr)
    num_single_espresso_->publish_state(parse(3, 4));
  if (num_double_espresso_ != nullptr)
    num_double_espresso_->publish_state(parse(7, 4));
  if (num_coffee_ != nullptr && result.size() >= 15)
    num_coffee_->publish_state(parse(11, 4));
  if (num_double_coffee_ != nullptr && result.size() >= 19)
    num_double_coffee_->publish_state(parse(15, 4));
  if (num_rinse_ != nullptr && result.size() >= 35)
    num_rinse_->publish_state(parse(31, 4));
  if (num_clean_ != nullptr && result.size() >= 39)
    num_clean_->publish_state(parse(35, 4));
  if (num_descale_ != nullptr && result.size() >= 43)
    num_descale_->publish_state(parse(39, 4));
  if (num_coffees_since_clean_ != nullptr && result.size() >= 63)
    num_coffees_since_clean_->publish_state(parse(59, 4));
}

// ── Command sending ──────────────────────────────────────────────────────────

void JuraCoffee::send_command(const std::string &command) {
  if (is_command_blocked(command)) {
    ESP_LOGE(TAG, "BLOCKED dangerous command: '%s' (AN:0A = EEPROM clear)", command.c_str());
    if (last_response_ != nullptr)
      last_response_->publish_state("BLOCKED");
    return;
  }

  if (debug_active_) {
    ESP_LOGI(TAG, "[CMD]  t=%lu %s", (unsigned long) millis(), command.c_str());
  }
  ESP_LOGD(TAG, "send_command: '%s'", command.c_str());

  std::string response = this->cmd2jura(command);

  if (debug_active_) {
    ESP_LOGI(TAG, "[CMD]  t=%lu %s -> %s", (unsigned long) millis(), command.c_str(),
             response.empty() ? "(no response)" : response.c_str());
  }

  if (!response.empty()) {
    ESP_LOGD(TAG, "response: '%s'", response.c_str());
    if (last_response_ != nullptr)
      last_response_->publish_state(response);
  } else {
    ESP_LOGW(TAG, "no response to command '%s'", command.c_str());
    if (last_response_ != nullptr)
      last_response_->publish_state("(no response)");
  }
}

// ── Debug dump ───────────────────────────────────────────────────────────────

void JuraCoffee::start_debug_dump(uint8_t rr_start, uint8_t rr_end, uint32_t interval_ms,
                                   bool poll_ic, bool poll_rt) {
  debug_rr_start_ = rr_start;
  debug_rr_end_ = rr_end;
  debug_interval_ms_ = interval_ms;
  debug_poll_ic_ = poll_ic;
  debug_poll_rt_ = poll_rt;
  debug_active_ = true;
  debug_phase_ = PHASE_IC;
  debug_current_rr_ = rr_start;
  ESP_LOGI(TAG, "[DUMP] Started: RR:%02X-RR:%02X every %ums (IC:%s RT:%s)",
           rr_start, rr_end, interval_ms,
           poll_ic ? "yes" : "no", poll_rt ? "yes" : "no");
}

void JuraCoffee::stop_debug_dump() {
  if (debug_active_) {
    ESP_LOGI(TAG, "[DUMP] Stopped at t=%lu", (unsigned long) millis());
  }
  debug_active_ = false;
  debug_phase_ = PHASE_IDLE;
}

void JuraCoffee::annotate_debug(const std::string &note) {
  ESP_LOGI(TAG, "[NOTE] t=%lu %s", (unsigned long) millis(), note.c_str());
}

void JuraCoffee::loop() {
  if (!debug_active_)
    return;

  switch (debug_phase_) {
    case PHASE_IC:
      if (debug_poll_ic_) {
        std::string r = this->cmd2jura("IC:");
        ESP_LOGI(TAG, "[DUMP] t=%lu IC: %s", (unsigned long) millis(), r.c_str());
      }
      if (debug_rr_start_ <= debug_rr_end_) {
        debug_phase_ = PHASE_RR;
        debug_current_rr_ = debug_rr_start_;
      } else {
        debug_phase_ = PHASE_RT;
      }
      break;

    case PHASE_RR: {
      char cmd[8];
      snprintf(cmd, sizeof(cmd), "RR:%02X", debug_current_rr_);
      std::string r = this->cmd2jura(cmd);
      ESP_LOGI(TAG, "[DUMP] t=%lu %s %s", (unsigned long) millis(), cmd, r.c_str());
      debug_current_rr_++;
      if (debug_current_rr_ > debug_rr_end_) {
        debug_phase_ = PHASE_RT;
      }
      break;
    }

    case PHASE_RT:
      if (debug_poll_rt_) {
        std::string r = this->cmd2jura("RT:0000");
        ESP_LOGI(TAG, "[DUMP] t=%lu RT:0000 %s", (unsigned long) millis(), r.c_str());
      }
      debug_phase_ = PHASE_WAIT;
      debug_wait_start_ = millis();
      ESP_LOGD(TAG, "[DUMP] Cycle complete, waiting %ums", debug_interval_ms_);
      break;

    case PHASE_WAIT:
      if (millis() - debug_wait_start_ >= debug_interval_ms_) {
        debug_phase_ = PHASE_IC;
      }
      break;

    default:
      break;
  }
}

// ── Component lifecycle ──────────────────────────────────────────────────────

void JuraCoffee::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Jura Coffee component...");
}

void JuraCoffee::update() {
  // During debug dump, skip normal polling — the dump handles IC: and RT:
  if (debug_active_)
    return;

  read_sensors();
  if (ready_ != nullptr)
    read_ready();

  // Read EEPROM counters only every 5th update (saves time, counters change rarely)
  if (++update_counter_ >= 5) {
    update_counter_ = 0;
    read_status();
  }
}

void JuraCoffee::dump_config() {
  ESP_LOGCONFIG(TAG, "Jura Coffee Machine (Toptronic):");
  ESP_LOGCONFIG(TAG, "  Update interval: %ums", this->get_update_interval());
  LOG_BINARY_SENSOR("  ", "Tray Missing", tray_missing_);
  LOG_BINARY_SENSOR("  ", "Tank Empty",   tank_empty_);
  LOG_BINARY_SENSOR("  ", "Need Clean",   need_clean_);
  LOG_SENSOR("  ", "Single Espressos", num_single_espresso_);
  LOG_SENSOR("  ", "Double Espressos", num_double_espresso_);
  LOG_SENSOR("  ", "Coffees",          num_coffee_);
  LOG_SENSOR("  ", "Double Coffees",   num_double_coffee_);
  LOG_SENSOR("  ", "Cleanings",        num_clean_);
  LOG_SENSOR("  ", "Rinses",           num_rinse_);
  LOG_SENSOR("  ", "Descalings",       num_descale_);
  LOG_SENSOR("  ", "Coffees Since Cleaning", num_coffees_since_clean_);
  LOG_BINARY_SENSOR("  ", "Ready",        ready_);
  LOG_BINARY_SENSOR("  ", "Needs Rinse",  needs_rinse_);
  LOG_TEXT_SENSOR("  ", "Last Response", last_response_);
}

}  // namespace jura_coffee
}  // namespace esphome
