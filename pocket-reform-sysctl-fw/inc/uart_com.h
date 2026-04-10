#ifndef _POCKET_UARTCOM_H
#define _POCKET_UARTCOM_H

#include <stdint.h>
#include "sysctl.h"
#include "kvstore.h"

#define UART_BUFSZ 255
#define CMD_NUMBER_INVALID 0xffff

void handle_uart_commands(battery_info_s *battery_info);
void handle_commands(char chr, battery_info_s *battery_info);

typedef struct uart_state_s {
	char remote_cmd;
	unsigned char cmd_state;
	unsigned int cmd_number;
	int echo;
	char kv_key[KV_KEY_STR_LEN];
	char kv_value[KV_VALUE_STR_LEN];
	int kv_key_len;
	int kv_value_len;
	int kv_count;
} uart_state_s;

#endif
