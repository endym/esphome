// Datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/MAX6921-MAX6931.pdf

#include <cinttypes>
#include "display.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "max6921.h"

namespace esphome {
namespace max6921 {

static const char *const TAG = "max6921";

float MAX6921Component::get_setup_priority() const { return setup_priority::HARDWARE; }

void MAX6921Component::setup() {
  const uint32_t PWM_FREQ_WANTED = 5000;
  const uint8_t PWM_RESOLUTION = 8;

  ESP_LOGCONFIG(TAG, "Setting up MAX6921...");
  // global_max6921 = this;

  this->spi_setup();
  this->load_pin_->setup();
  this->load_pin_->pin_mode(gpio::FLAG_OUTPUT);
  this->disable_load_();  // disable output latch

  this->display_ = new Display(this);
  this->display_->setup(this->seg_to_out_map__, this->pos_to_out_map__);

  // setup display brightness (PWM for BLANK pin)...
  if (this->display_->config_brightness_pwm(this->blank_pin_->get_pin(), 0, PWM_RESOLUTION, PWM_FREQ_WANTED) == 0) {
    ESP_LOGE(TAG, "Failed to configure PWM -> set to max. brightness");
    this->blank_pin_->pin_mode(gpio::FLAG_OUTPUT);
    this->blank_pin_->setup();
    this->disable_blank_();  // enable display (max. brightness)
  }

  this->setup_finished = true;
}

void MAX6921Component::dump_config() {
  ESP_LOGCONFIG(TAG, "MAX6921:");
  LOG_PIN("  LOAD Pin: ", this->load_pin_);
  ESP_LOGCONFIG(TAG, "  BLANK Pin: GPIO%u", this->blank_pin_->get_pin());
  this->display_->dump_config();
}

void MAX6921Component::set_brightness(float brightness) {
  if (!this->setup_finished) {
    ESP_LOGD(TAG, "Set brightness: setup not finished -> discard brightness value");
    return;
  }
  if ((brightness == 0.0) || (brightness != this->display_->get_brightness())) {
    this->display_->set_brightness(brightness);
    ESP_LOGD(TAG, "Set brightness: %.1f", this->display_->get_brightness());
  }
}

/**
 * @brief Clocks data into MAX6921 via SPI (MSB first).
 *        Data must contain 3 bytes with following format:
 *          bit  | 23 | 22 | 21 | 20 | 19 | 18 | ... | 1 | 0
 *          ------------------------------------------------
 *          DOUT | x  | x  | x  | x  | 19 | 18 | ... | 1 | 0
 */
void HOT MAX6921Component::write_data(uint8_t *ptr, size_t length) {
  uint8_t data[3];
  static bool first_call_logged = false;

  assert(length == 3);
  this->disable_load_();            // set LOAD to low
  memcpy(data, ptr, sizeof(data));  // make copy of data, because transfer buffer will be overwritten with SPI answer
  if (!first_call_logged)
    ESP_LOGVV(TAG, "SPI(%u): 0x%02x%02x%02x", length, data[0], data[1], data[2]);
  first_call_logged = true;
  this->transfer_array(data, sizeof(data));
  this->enable_load_();  // set LOAD to high to update output latch
}

void MAX6921Component::update() {
  this->display_->update();

  if (this->writer_.has_value())
    (*this->writer_)(*this);
}

/*
 * Evaluates lambda function
 *   start_pos: 0..n = left..right display position
 *   vi_text      : display text
 */
uint8_t MAX6921Component::print(uint8_t start_pos, const char *str) {
  if (this->display_->mode != DISP_MODE_PRINT)  // not in "it.print" mode?
    return strlen(str);                         // yes -> abort
  return this->display_->set_text(str, start_pos);
}

uint8_t MAX6921Component::print(const char *str) { return this->print(0, str); }

uint8_t MAX6921Component::strftime(uint8_t pos, const char *format, ESPTime time) {
  char buffer[64];
  size_t ret = time.strftime(buffer, sizeof(buffer), format);
  if (ret > 0)
    return this->print(pos, buffer);
  return 0;
}
uint8_t MAX6921Component::strftime(const char *format, ESPTime time) { return this->strftime(0, format, time); }

void MAX6921Component::set_writer(max6921_writer_t &&writer) { this->writer_ = writer; }

}  // namespace max6921
}  // namespace esphome
