#pragma once

#include <esp32-hal-gpio.h>
#include <string>
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/time.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace max6921 {

#define ARRAY_ELEM_COUNT(array)  (sizeof(array)/sizeof(array[0]))


class MAX6921Component;
class Display;

using max6921_writer_t = std::function<void(MAX6921Component &)>;

class MAX6921Component : public PollingComponent,
                         public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                               spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_4MHZ> {
 public:
  Display *display_;
  void dump_config() override;
  float get_setup_priority() const override;
  uint8_t print(uint8_t pos, const char *str);
  uint8_t print(const char *str);
  void set_blank_pin(InternalGPIOPin *pin) { blank_pin_ = pin; }
  void set_brightness(float brightness);
  void set_load_pin(GPIOPin *load) { this->load_pin_ = load; }
  void set_seg_to_out_pin_map(const std::vector<uint8_t> &pin_map) { this->seg_to_out_map__ = pin_map; }
  void set_pos_to_out_pin_map(const std::vector<uint8_t> &pin_map) { this->pos_to_out_map__ = pin_map; }
  void set_text(const std::string& text);
  void set_writer(max6921_writer_t &&writer);
  void setup() override;
  uint8_t strftime(uint8_t pos, const char *format, ESPTime time) __attribute__((format(strftime, 3, 0)));
  uint8_t strftime(const char *format, ESPTime time) __attribute__((format(strftime, 2, 0)));
  void write_data(uint8_t *ptr, size_t length);
  void update() override;

 protected:
  GPIOPin *load_pin_{};
  InternalGPIOPin *blank_pin_;
  bool setup_finished{false};
  void disable_blank_() { digitalWrite(this->blank_pin_->get_pin(), LOW); }  // display on
  void IRAM_ATTR HOT disable_load_() { this->load_pin_->digital_write(false); }
  void enable_blank_() { digitalWrite(this->blank_pin_->get_pin(), HIGH); }  // display off
  void IRAM_ATTR HOT enable_load_() { this->load_pin_->digital_write(true); }
  void update_demo_mode_scroll_font_(void);
  optional<max6921_writer_t> writer_{};

 private:
  std::vector<uint8_t> seg_to_out_map__;                    // mapping of display segments to MAX6921 OUT pins
  std::vector<uint8_t> pos_to_out_map__;                    // mapping of display positions to MAX6921 OUT pins
};


}  // namespace max6921
}  // namespace esphome
