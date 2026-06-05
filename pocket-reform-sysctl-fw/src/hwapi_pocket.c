/*
  MNT Pocket Reform: Hardware Interface
 */

#include "cli.h"
// TODO: move battery_info_s to pocket specific file?
#include "ext_gpio.h"
#include "sysctl.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// TODO: need these inputs:
// - battery_info

static battery_info_s *battery_info;

#define LEGACY_BUF_SZ 128
#define LEGACY_SPI_SZ 8
static char legacy_buf[LEGACY_BUF_SZ];
static char legacy_spi_buf[LEGACY_SPI_SZ];

uint64_t hwapi_legacy_q() {
  // execute status query command
  memset(legacy_spi_buf, 0, LEGACY_SPI_SZ);
  uint8_t percentage = (uint8_t)battery_info->charge_percentage;
  int16_t volts_int = (int16_t)(battery_info->battery_volts * 1000.0);
  int16_t current_int = (int16_t)(battery_info->battery_amps * 1000.0);

  legacy_spi_buf[0] = (uint8_t)volts_int;
  legacy_spi_buf[1] = (uint8_t)(volts_int >> 8);
  legacy_spi_buf[2] = (uint8_t)current_int;
  legacy_spi_buf[3] = (uint8_t)(current_int >> 8);
  legacy_spi_buf[4] = (uint8_t)percentage;
  legacy_spi_buf[5] = (uint8_t)0;
  return eightcc(legacy_spi_buf);
}

uint64_t hwapi_legacy_v() {
  // get cell voltage
  memset(legacy_buf, 0, LEGACY_SPI_SZ);
  // pack 0
  int volts = battery_info->cell1_volts;
  legacy_spi_buf[0] = (uint8_t)volts;
  legacy_spi_buf[1] = (uint8_t)(volts >> 8);
  volts = battery_info->cell2_volts;
  legacy_spi_buf[2] = (uint8_t)volts;
  legacy_spi_buf[3] = (uint8_t)(volts >> 8);
  return eightcc(legacy_spi_buf);
}

uint64_t hwapi_legacy_c() {
  // get calculated capacity (emulated)
  memset(legacy_buf, 0, LEGACY_SPI_SZ);
  uint16_t cap_accu = (uint16_t)BATTERY_CAPACITY_MILLIAMP_HOURS * (((float)battery_info->charge_percentage) / 100.0);
  uint16_t cap_min = (uint16_t)0;
  uint16_t cap_max = (uint16_t)BATTERY_CAPACITY_MILLIAMP_HOURS;
  legacy_spi_buf[0] = (uint8_t)cap_accu;
  legacy_spi_buf[1] = (uint8_t)(cap_accu >> 8);
  legacy_spi_buf[2] = (uint8_t)cap_min;
  legacy_spi_buf[3] = (uint8_t)(cap_min >> 8);
  legacy_spi_buf[4] = (uint8_t)cap_max;
  legacy_spi_buf[5] = (uint8_t)(cap_max >> 8);
  return eightcc(legacy_spi_buf);
}

char* hwapi_legacy_kbd_s() {
  return (char*)"(Outdated Kbd FW!)\r";
}

char* hwapi_legacy_kbd_c() {
  int ma = (int)(battery_info->battery_amps * 1000.0);
  char ma_sign = ' ';
  if (ma < 0) {
    ma = -ma;
    ma_sign = '-';
  }
  int mv = (int)(battery_info->battery_volts * 1000.0);
  snprintf(legacy_buf, 128,
           "%02d %02d %02d %02d %02d %02d %02d %02d mA%c%04dmV%05d %3d%% P%d\r\n",
           (int)(battery_info->cell1_volts / 100),
           (int)(battery_info->cell2_volts / 100), (int)(0), (int)(0),
           (int)(0), (int)(0), (int)(0), (int)(0), ma_sign, ma, mv,
           battery_info->charge_percentage,
           battery_info->som_is_powered ? 1 : 0);

  legacy_buf[127] = 0;
  return (char*)legacy_buf;
}

uint64_t hwapi_set_backlight([[maybe_unused]] struct cli_context* ctx, uint64_t brightness) {
  // only for display v2
  // 80% is a limit of the hardware (above, the backlight can flicker)
  if (brightness > 80) brightness = 80;
  set_display_backlight(brightness);
  cli_add_word("lite\0\0\0\0", brightness);
  return brightness;
}

uint64_t hwapi_set_rail([[maybe_unused]] struct cli_context* ctx, uint64_t rail, uint64_t state) {
  if (rail == 0) {
    if (state == 0) {
      turn_som_power_off();
      cli_add_word("rail-0\0\0", 0);
    } else {
      turn_som_power_on();
      cli_add_word("rail-0\0\0", 1);
    }
  }
  return state;
}

uint64_t hwapi_set_gpio([[maybe_unused]] struct cli_context* ctx, uint64_t id, uint64_t high) {
  switch (id) {
  case 0: {
    // 0: Display Panel Reset (active low)
    // TODO: v10 only
    gpio_put(PIN_DISP_RESET, high);
    // TODO: v20 only
    if (high) {
      gpio_ext_enable(GPIO_EXT_DISP_RESET_N);
    } else {
      gpio_ext_disable(GPIO_EXT_DISP_RESET_N);
    }
    break;
  }
  case 1: {
    if (high) {
      gpio_ext_enable(GPIO_EXT_HUB_PWR_EN);
    } else {
      gpio_ext_disable(GPIO_EXT_HUB_PWR_EN);
    }
    break;
  }
  case 2: {
    if (high) {
      gpio_ext_enable(GPIO_EXT_PCIE_PWR_EN);
    } else {
      gpio_ext_disable(GPIO_EXT_PCIE_PWR_EN);
    }
    break;
  }
  case 3: {
    if (high) {
      gpio_ext_enable(GPIO_EXT_3V3_EN);
    } else {
      gpio_ext_disable(GPIO_EXT_3V3_EN);
    }
    break;
  }
  case 4: {
    if (high) {
      gpio_ext_enable(GPIO_EXT_USWITCH_OFF);
    } else {
      gpio_ext_disable(GPIO_EXT_USWITCH_OFF);
    }
    break;
  }
  case 5: {
    if (high) {
      gpio_ext_enable(GPIO_EXT_DISP_BL_PWR_EN);
    } else {
      gpio_ext_disable(GPIO_EXT_DISP_BL_PWR_EN);
    }
    break;
  }
  }
  return high;
}

uint64_t hwapi_set_usb_mode([[maybe_unused]] struct cli_context* ctx, uint64_t port, uint64_t mode) {
  // toggle USB muxing modes
  if (port == 1) {
    switch (mode) {
    case 0:
      // usb2: sysctl external
      // TODO: only for mb20
      gpio_put(PIN_USB_SRC_ENABLE, 0);
      gpio_ext_uswitch_enable(0);
      gpio_ext_uswitch_enable(1);
      gpio_ext_uswitch_enable(2);
      cli_add_word("usbmux-1", 0);
      break;
    case 1:
      // usb2: host, sysctl internal
      // TODO: only for mb20
      gpio_put(PIN_USB_SRC_ENABLE, 1);
      gpio_ext_uswitch_disable(0);
      gpio_ext_uswitch_disable(1);
      gpio_ext_uswitch_disable(2);
      cli_add_word("usbmux-1", 1);
      break;
    case 2:
      // usb2: EDL external
      // TODO: only for mb20
      gpio_put(PIN_USB_SRC_ENABLE, 0);
      gpio_ext_uswitch_enable(0);
      gpio_ext_uswitch_disable(1);
      gpio_ext_uswitch_enable(2);
      cli_add_word("usbmux-1", 2);
      break;
    }
  }
  if (port == 0) {
    switch (mode) {
    case 0:
      // usb1: SoC UART
      // TODO: should be controlled by PD logic
      usb_host_5v_disable();
      gpio_ext_uswitch_disable(3);
      cli_add_word("usbmux-0", 0);
      break;
    case 1:
      // usb1: host
      // TODO: should be controlled by PD logic
      usb_host_5v_enable();
      gpio_ext_uswitch_enable(3);
      cli_add_word("usbmux-0", 1);
      break;
    }
  }
  return mode;
}

uint64_t hwapi_set_usb_ports_sysctl([[maybe_unused]] struct cli_context* ctx) {
  // expose Sysctl on port 2, and SoC UART on port 1 (charging port)
  hwapi_set_usb_mode(ctx, 0, 0);
  hwapi_set_usb_mode(ctx, 1, 0);
  return 1;
}

uint64_t hwapi_set_usb_ports_host([[maybe_unused]] struct cli_context* ctx) {
  // set all ports to host mode
  hwapi_set_usb_mode(ctx, 0, 1);
  hwapi_set_usb_mode(ctx, 1, 1);
  return 1;
}

uint64_t hwapi_set_usb_ports_edl([[maybe_unused]] struct cli_context* ctx) {
  // expose EDL mode on port 2, and SoC UART on port 1 (charging port)
  hwapi_set_usb_mode(ctx, 0, 0);
  hwapi_set_usb_mode(ctx, 1, 2);
  return 1;
}

uint64_t hwapi_soc_wake([[maybe_unused]] struct cli_context* ctx) {
  som_wake();
  return 1;
}

/* prepare SoC suspend by turning off unneeded power sources */
uint64_t hwapi_soc_pre_suspend([[maybe_unused]] struct cli_context* ctx) {
  gpio_ext_disable(GPIO_EXT_HUB_PWR_EN);
  gpio_ext_disable(GPIO_EXT_3V3_EN);
  gpio_ext_disable(GPIO_EXT_DISP_BL_PWR_EN);
  return 1;
}

uint64_t hwapi_soc_post_suspend([[maybe_unused]] struct cli_context* ctx) {
  gpio_ext_enable(GPIO_EXT_3V3_EN);
  gpio_ext_enable(GPIO_EXT_HUB_PWR_EN);
  gpio_ext_enable(GPIO_EXT_DISP_BL_PWR_EN);
  return 1;
}

uint64_t hwapi_get_cell_mv([[maybe_unused]] struct cli_context* ctx, uint64_t cell_id) {
  if (cell_id == 1) return battery_info->cell1_volts;
  // TODO misnomer, actually mV
  return battery_info->cell1_volts;
}

uint64_t hwapi_get_pack_mv([[maybe_unused]] struct cli_context* ctx /*uint64_t pack_id*/) {
  // TODO what about pack_volts?
  return battery_info->battery_volts * 1000;
}

uint64_t hwapi_get_pack_ma([[maybe_unused]] struct cli_context* ctx /*uint64_t pack_id*/) {
  return battery_info->battery_amps * 1000;
}

uint64_t hwapi_get_pack_charge([[maybe_unused]] struct cli_context* ctx /*uint64_t pack_id*/) {
  return battery_info->charge_percentage;
}

uint64_t hwapi_get_pack_capacity_full([[maybe_unused]] struct cli_context* ctx /*uint64_t pack_id*/) {
  // TODO
  return 0;
}

uint64_t hwapi_get_sys_mv([[maybe_unused]] struct cli_context* ctx /*uint64_t pack_id*/) {
  // TODO
  return battery_info->input_volts;
}

uint64_t hwapi_get_sys_ma([[maybe_unused]] struct cli_context* ctx /*uint64_t pack_id*/) {
  // TODO
  // also: input amps?
  return 0;
}

// TODO: need to instantiate multiple CLIs (ttys?)
// so state is not mixed between usb, uart, and spi interfaces
void hwapi_pocket_init(battery_info_s *binfo) {
  battery_info = binfo;

  /* register all available functions */
  cli_add_func("set-rail", hwapi_set_rail, 2, CLI_TYPE_UINT64);
  cli_add_func("set-gpio", hwapi_set_gpio, 2, CLI_TYPE_UINT64);
  cli_add_func("set-usb\0", hwapi_set_usb_mode, 1, CLI_TYPE_UINT64);
  cli_add_func("usb-sc\0\0", hwapi_set_usb_ports_sysctl, 0, CLI_TYPE_UINT64);
  cli_add_func("usb-edl\0", hwapi_set_usb_ports_edl, 0, CLI_TYPE_UINT64);
  cli_add_func("usb-host", hwapi_set_usb_ports_host, 0, CLI_TYPE_UINT64);
  cli_add_func("set-lite", hwapi_set_backlight, 1, CLI_TYPE_UINT64);
  cli_add_func("cell-mv\0", hwapi_get_cell_mv, 1, CLI_TYPE_UINT64);
  cli_add_func("pack-mv\0", hwapi_get_pack_mv, 1, CLI_TYPE_UINT64);
  cli_add_func("pack-ma\0", hwapi_get_pack_ma, 1, CLI_TYPE_UINT64);
  cli_add_func("pack-crg", hwapi_get_pack_charge, 1, CLI_TYPE_UINT64);
  cli_add_func("pack-max", hwapi_get_pack_capacity_full, 1, CLI_TYPE_UINT64);
  cli_add_func("sys-mv\0\0", hwapi_get_sys_mv, 0, CLI_TYPE_UINT64);
  cli_add_func("sys-ma\0\0", hwapi_get_sys_ma, 0, CLI_TYPE_UINT64);
  cli_add_func("soc-wake", hwapi_soc_wake, 0, CLI_TYPE_UINT64);
  cli_add_func("soc-susp", hwapi_soc_pre_suspend, 0, CLI_TYPE_UINT64);
  cli_add_func("soc-psus", hwapi_soc_post_suspend, 0, CLI_TYPE_UINT64);
  cli_add_func("pwrsave\0", enter_powersave, 0, CLI_TYPE_VOID);

  // TODO: DP/altmode_set configs
  // altmode_set(0b110);
  // altmode_set(0b111);
  // altmode_set(0b100);
  // altmode_set(0b101);
  // altmode_set(0b000);
  // altmode_set(0b010);
  // gpio_set_dir(PIN_USB_LOADER_SW, GPIO_OUT);
  // gpio_put(PIN_USB_LOADER_SW, 1);
  // PIN_V20_DP_HPD z/1/0
  // serial fowarding (next)

  /* register constants */
  cli_add_word("pack-cnt", 1);
  cli_add_word("cell-cnt", 2);

  /* legacy API commands */
  cli_add_func("0p\0\0\0\0\0\0", turn_som_power_off, 0, CLI_TYPE_VOID);
  cli_add_func("1p\0\0\0\0\0\0", turn_som_power_on, 0, CLI_TYPE_VOID);
  cli_add_func("0q\0\0\0\0\0\0\0", hwapi_legacy_q, 0, CLI_TYPE_UINT64);
  cli_add_func("0v\0\0\0\0\0\0\0", hwapi_legacy_v, 0, CLI_TYPE_UINT64);
  cli_add_func("0c\0\0\0\0\0\0\0", hwapi_legacy_c, 0, CLI_TYPE_UINT64);
  cli_add_func("1w\0\0\0\0\0\0\0", som_wake, 0, CLI_TYPE_VOID);
  cli_add_word("0f\0\0\0\0\0\0", eightcc("OUTDATED"));
  cli_add_word("1f\0\0\0\0\0\0", eightcc("LPC     "));
  cli_add_word("2f\0\0\0\0\0\0", eightcc("DRIVER  "));
  cli_add_func("s\0\0\0\0\0\0\0", hwapi_legacy_kbd_s, 0, CLI_TYPE_STR256);
  cli_add_func("c\0\0\0\0\0\0\0", hwapi_legacy_kbd_c, 0, CLI_TYPE_STR256);
}
