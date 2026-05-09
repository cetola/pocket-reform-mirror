/*
  MNT Pocket Reform: Hardware Interface
 */

#include "cli.h"
// TODO: move battery_info_s to pocket specific file?
#include "sysctl.h"
#include <stdint.h>

// TODO: need these inputs:
// - battery_info

static battery_info_s *battery_info;

#define LEGACY_BUF_SZ 8

uint64_t hwapi_legacy_q()
{
	// execute status query command
	uint8_t buf[LEGACY_BUF_SZ] = { 0 };
	uint8_t percentage = (uint8_t)battery_info->charge_percentage;
  int16_t volts_int = (int16_t)(battery_info->battery_volts * 1000.0);
  int16_t current_int = (int16_t)(battery_info->battery_amps * 1000.0);

  buf[0] = (uint8_t)volts_int;
	buf[1] = (uint8_t)(volts_int >> 8);
	buf[2] = (uint8_t)current_int;
	buf[3] = (uint8_t)(current_int >> 8);
	buf[4] = (uint8_t)percentage;
	buf[5] = (uint8_t)0;
	return eightcc(buf);
}

uint64_t hwapi_legacy_v()
{
  // get cell voltage
	uint8_t buf[LEGACY_BUF_SZ] = { 0 };
  // pack 0
  int volts = battery_info->cell1_volts;
  buf[0] = (uint8_t)volts;
  buf[1] = (uint8_t)(volts >> 8);
  volts = battery_info->cell2_volts;
  buf[2] = (uint8_t)volts;
  buf[3] = (uint8_t)(volts >> 8);
	return eightcc(buf);
}

uint64_t hwapi_legacy_c()
{
  // get calculated capacity (emulated)
	uint8_t buf[LEGACY_BUF_SZ] = { 0 };
  uint16_t cap_accu = (uint16_t)BATTERY_CAPACITY_MILLIAMP_HOURS * (((float)battery_info->charge_percentage) / 100.0);
  uint16_t cap_min = (uint16_t)0;
  uint16_t cap_max = (uint16_t)BATTERY_CAPACITY_MILLIAMP_HOURS;
  buf[0] = (uint8_t)cap_accu;
	buf[1] = (uint8_t)(cap_accu >> 8);
	buf[2] = (uint8_t)cap_min;
	buf[3] = (uint8_t)(cap_min >> 8);
	buf[4] = (uint8_t)cap_max;
	buf[5] = (uint8_t)(cap_max >> 8);
	return eightcc(buf);
}

uint64_t hwapi_legacy_b(uint64_t brightness)
{
  // only for display v2
  // 80% is a limit of the hardware (above, the backlight can flicker)
  if (brightness > 80)
    brightness = 80;
  set_display_backlight(brightness);
  return brightness;
}

uint64_t hwapi_set_rail(uint64_t rail, uint64_t state)
{
	if (rail == 0) {
		if (state == 0) {
			turn_som_power_off();
		} else {
			turn_som_power_on();
		}
	}
	return state;
}

uint64_t hwapi_set_gpio(uint64_t id, uint64_t high)
{
  switch (id) {
  case 0:
    // 0: Display Panel Reset (active low)
    gpio_put(PIN_DISP_RESET, high);
    break;
  case 1:
    gpio_put(PIN_3V3_ENABLE, high);
    break;
  case 2:
    gpio_put(PIN_1V1_ENABLE, high);
    break;
  }
	return high;
}

uint64_t hwapi_set_usb_mode(uint64_t port, uint64_t mode)
{
	// toggle USB muxing modes
	if (port == 1) {
    switch (mode) {
    case 0:
      // usb2: sysctl external
      gpio_ext_uswitch_enable(0);
      gpio_ext_uswitch_enable(1);
      gpio_ext_uswitch_enable(2);
      break;
    case 1:
      // usb2: host, sysctl internal
      gpio_ext_uswitch_disable(0);
      gpio_ext_uswitch_disable(1);
      gpio_ext_uswitch_disable(2);
      break;
    case 2:
      // usb2: EDL external
      gpio_ext_uswitch_enable(0);
      gpio_ext_uswitch_disable(1);
      gpio_ext_uswitch_enable(2);
      break;
    }
	}
	if (port == 0) {
    switch (mode) {
    case 0:
      // usb1: host
      gpio_ext_uswitch_enable(3);
      break;
    case 1:
      // usb1: SoC UART
      gpio_ext_uswitch_disable(3);
      break;
    }
  }
	return mode;
}

double hwapi_get_cell_volts(uint64_t cell_id)
{
	if (cell_id == 2) return battery_info->cell1_volts;
	return battery_info->cell1_volts;
}

double hwapi_get_pack_volts(/*uint64_t pack_id*/)
{
	// TODO what about pack_volts?
	return battery_info->battery_volts;
}

double hwapi_get_pack_amps(/*uint64_t pack_id*/)
{
	return battery_info->battery_amps;
}

double hwapi_get_pack_charge(/*uint64_t pack_id*/)
{
	return battery_info->charge_percentage;
}

uint64_t hwapi_get_pack_capacity_full(/*uint64_t pack_id*/)
{
	// TODO
	return 0;
}

double hwapi_get_sys_volts(/*uint64_t pack_id*/)
{
	// TODO
	return 0;
}

double hwapi_get_sys_amps(/*uint64_t pack_id*/)
{
	// TODO
	// also: input amps?
	return 0;
}

// TODO: need to instantiate multiple CLIs (ttys?)
// so state is not mixed between usb, uart, and spi interfaces
void hwapi_pocket_init(battery_info_s *binfo)
{
	battery_info = binfo;
	
	/* register all available functions */
	cli_add_func("set-rail", hwapi_set_rail, 2);
	cli_add_func("set-gpio", hwapi_set_gpio, 2);
	cli_add_func("set-usb\0", hwapi_set_usb_mode, 1);
	cli_add_func("cell-vlt", hwapi_get_cell_volts, 1);
	cli_add_func("pack-vlt", hwapi_get_pack_volts, 1);
	cli_add_func("pack-amp", hwapi_get_pack_amps, 1);
	cli_add_func("pack-cap", hwapi_get_pack_charge, 1);
	cli_add_func("pack-max", hwapi_get_pack_capacity_full, 1);
	cli_add_func("sys-vlt\0", hwapi_get_sys_volts, 0);
	cli_add_func("sys-amp\0", hwapi_get_sys_amps, 0);

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
	// enter_powersave
	// som_wake
	// GPIO_EXT_DISP_RESET_N
	// display brightness / pwm
	// serial fowarding (next)
  // gpio_put(PIN_USB_SRC_ENABLE, 0);
	
	/* register constants */
	cli_add_word("pack-cnt", 1);
	cli_add_word("cell-cnt", 2);

	/* legacy API commands */
	cli_add_func("0p\0\0\0\0\0\0", turn_som_power_off, 0);
	cli_add_func("1p\0\0\0\0\0\0", turn_som_power_on, 0);
	cli_add_func("0q\0\0\0\0\0\0\0", hwapi_legacy_q, 0);
	cli_add_func("0v\0\0\0\0\0\0\0", hwapi_legacy_v, 0);
	cli_add_func("0c\0\0\0\0\0\0\0", hwapi_legacy_c, 0);
	cli_add_word("0f\0\0\0\0\0\0", eightcc("OUTDATED"));
	cli_add_word("1f\0\0\0\0\0\0", eightcc("LPC     "));
	cli_add_word("2f\0\0\0\0\0\0", eightcc("DRIVER  "));

	// TODO: b?
}
