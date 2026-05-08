#ifndef MNTSC_CLI_H
#define MNTSC_CLI_H

#include <stdint.h>

/*
  goals:
  - in sc: proper command parser that supersedes the legacy lpc protocol
  - in kbd: shell mode with prompt/cursor, line based, some scrollback?
  - in sc: soc uart passthrough mode
  - getting and setting of variables

  examples:

  > 1p                # legacy power on command -> eval special case?
  > (power 1)         # new function call
  > set('bootpref,1)  # 16 chars
  > set bootpref 1    # no markers to rely on and no easy eval for variables
  > (set 'bootpref 1) # 17 chars. easier to parse. eval for free later?

  how:

  - line buffer
  - paren counter
  - if newline and paren counter = 0 -> eval

*/

#define fourcc(a, b, c, d) ((uint64_t)(a) | ((uint64_t)(b) << 8) | ((uint64_t)(c) << 16) | ((uint64_t)(d) << 24))

#define eightcc(a, b, c, d, e, f, g, h) ((uint64_t)(a) | ((uint64_t)(b) << 8) | ((uint64_t)(c) << 16) | ((uint64_t)(d) << 24) | ((uint64_t)(e) << 32) | ((uint64_t)(f) << 40) | ((uint64_t)(g) << 48) | ((uint64_t)(h) << 56))

#define CLI_BUFSZ 128
#define CLI_MAX_PAREN 2
#define CLI_MAX_WORD_LEN 8
#define CLI_MAX_LIST_WORDS 4
#define CLI_MAX_VARS 64
#define CLI_ERR_MAX_PAREN fourcc('m','a','x','p') /* nesting too deep */
#define CLI_ERR_MAX_WORD  fourcc('w','l','e','n') /* word too long */
#define CLI_ERR_MAX_LIST  fourcc('l','l','e','n') /* list too long */
#define CLI_ERR_SYNTAX    fourcc('s','n','t','x')

struct cli_var {
  uint64_t word;
  void* func;
  uint8_t num_args;
};

void cli_reset();
void cli_reset_error();
void cli_init();
void cli_char(char c);
// TODO should this be a streaming output?
// TODO: len/err/success indicatin
char* cli_get_out();

#endif
