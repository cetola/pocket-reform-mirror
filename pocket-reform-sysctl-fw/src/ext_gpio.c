#include "ext_gpio.h"
#include "sysctl.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

static uint8_t gpio_ext_state;
static uint8_t gpio_ext_uswitch_state;

bool pca9536_write_byte(uint8_t addr, uint8_t val) {
  uint8_t buf[2] = {addr, val};
  return 2 == i2c_write_timeout_us(i2c0, PCA9536_ADDR, buf, 2, false, I2C_TIMEOUT);
}

bool pca9557_write_byte(uint8_t addr, uint8_t val) {
  uint8_t buf[2] = {addr, val};
  return 2 == i2c_write_timeout_us(i2c0, PCA9557_ADDR, buf, 2, false, I2C_TIMEOUT);
}

// also called by gpio_ext_setup()
bool gpio_ext_uswitch_setup() {
  /*
    IO3: USWITCH_4: (Port 1) 0 = UART, 1 = USB (hub port 2)
    IO2: USWITCH_3
    IO1: USWITCH_2
    IO0: USWITCH_1
   */

  // default: all USWITCHes connect D to D2
  gpio_ext_uswitch_state = 0b1111;

  // config: all outputs
  pca9536_write_byte(3, 0b0000);
  // output port
  return pca9536_write_byte(1, gpio_ext_uswitch_state);
}

bool gpio_ext_uswitch_enable(uint8_t bit) {
  gpio_ext_uswitch_state |= (1<<bit);
  return pca9536_write_byte(1, gpio_ext_uswitch_state);
}

bool gpio_ext_uswitch_disable(uint8_t bit) {
  gpio_ext_uswitch_state &= ~(1<<bit);
  return pca9536_write_byte(1, gpio_ext_uswitch_state);
}

bool gpio_ext_setup() {
  /*
    7: USWITCH_OFF
    6: PCIE_PWR_EN
    5: DISP_BL_PWR_EN
    4: ~DISP_RESET
    3: HUB_PWR_EN
    2: 3V3_ENABLE
    1: 5V_ENABLE
    0: DISP_1EN_2BL (open drain?)
   */

  gpio_ext_state = 0b00000000;

  // output port:
  pca9557_write_byte(1, gpio_ext_state);
  // config: outputs(0)/inputs(1) (all outputs)
  return pca9557_write_byte(3, 0b00000000);
}

bool gpio_ext_enable(uint8_t bit) {
  gpio_ext_state |= (1<<bit);
  return pca9557_write_byte(1, gpio_ext_state);
}

bool gpio_ext_disable(uint8_t bit) {
  gpio_ext_state &= ~(1<<bit);
  return pca9557_write_byte(1, gpio_ext_state);
}
