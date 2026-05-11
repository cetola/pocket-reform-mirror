/**
 * SPI commands from the SOM
 *
 * Ported from MNT Reform reform2-lpc-fw.
 */

#include "spi_com.h"
#include "cli.h"

void init_spi_client() {
  gpio_set_function(PIN_SOM_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SOM_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SOM_SS0, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SOM_SCK, GPIO_FUNC_SPI);

  // 4 MHz
  spi_init(spi1, 4000 * 1000);
  // we don't appreciate the wording, but it's the API we are given
  spi_set_slave(spi1, true);
  spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);

  printf("# [spi] init_spi_client done\n");
}

static uint8_t lpc_calc_checksum(uint8_t *buffer, int len) {
  uint8_t sum = 0;
  for (int i=0; i<len-1; i++) {
    sum = sum ^ buffer[i];
  }
  return sum;
}

#define SPI_DEBUG_ENABLED 0
#define SPI_PRINTF_ENABLED 0

/* note that this runs in a timer interrupt:
   - no sleep_ms() calls
   - don't run longer than 4ms
 */
void handle_spi_commands(battery_info_s *battery_info) {
  if (!battery_info->som_is_powered) return;

  while (spi_is_readable(spi1)) {
    uint8_t rx = (uint8_t)spi_get_hw(spi1)->dr;
    cli_char(rx);
    int resp_len = cli_get_out_pos();
    char *cli_out_buf = cli_get_out();

    if (resp_len > 0) {
      for (int i=0; i<resp_len; i++) {
        spi_get_hw(spi1)->dr = cli_out_buf[i];
      }
      spi_get_hw(spi1)->dr = lpc_calc_checksum((uint8_t*)cli_out_buf, resp_len);
      cli_reset_out();
    }
  }

  // TODO lpc_calc_checksum(spi_buf, SPI_BUF_LEN);

  /*
    // reset SPI0 block
    // this is a workaround for confusion with
    // software spi from BPI-CM4 where we get
    // bit-shifted bytes
       init_spi_client();
       return;
   */
}
