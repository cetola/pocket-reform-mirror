#include <stdio.h>
#include "../inc/cli.h"

int main(int argc, char **argv)
{
	cli_init();
	cli_char('h');
	cli_char('\n');
	printf("[cli_out] %s\n", cli_get_out());
	return 0;
}
