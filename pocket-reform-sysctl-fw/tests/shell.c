#include "sysctl.h"
#include <stdio.h>
#include "../inc/cli.h"

/* stubs */

void turn_som_power_off()
{
	printf("[cli_stub] turn_som_power_off()\n");
}
void turn_som_power_on()
{
	printf("[cli_stub] turn_som_power_on()\n");
}

/* end stubs */

int main(int argc, char **argv)
{
	cli_init();
	cli_char('(');
	cli_char(')');
	printf("[cli_out] %s\n", cli_get_out());
	cli_reset_error();
	cli_char('(');
	cli_char('h');
	cli_char(')');
	printf("[cli_out] %s\n", cli_get_out());
	cli_reset_error();
	cli_char('(');
	cli_char('p');
	cli_char('o');
	cli_char('w');
	cli_char('e');
	cli_char('r');
	cli_char('o');
	cli_char('f');
	cli_char('f');
	cli_char(')');
	return 0;
}
