#include "sysctl.h"
#include <stdio.h>
#include "../inc/cli.h"

/* stubs */

void turn_som_power_off()
{
}
void turn_som_power_on()
{
}

/* end stubs */

int main(int argc, char **argv)
{
	cli_init();
	cli_char('(');
	cli_char(')');
	printf("[cli_out] %s\n", cli_get_out());
	cli_char('(');
	cli_char('h');
	cli_char(')');
	printf("[cli_out] %s\n", cli_get_out());
	return 0;
}
