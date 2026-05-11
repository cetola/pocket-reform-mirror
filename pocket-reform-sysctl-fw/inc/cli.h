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

#define fourcc(str) ((uint64_t)(str[0]) | ((uint64_t)(str[1]) << 8) | ((uint64_t)(str[2]) << 16) | ((uint64_t)(str[3]) << 24))

#define eightcc(str) ((uint64_t)(str[0]) | ((uint64_t)(str[1]) << 8) | ((uint64_t)(str[2]) << 16) | ((uint64_t)(str[3]) << 24) | ((uint64_t)(str[4]) << 32) | ((uint64_t)(str[5]) << 40) | ((uint64_t)(str[6]) << 48) | ((uint64_t)(str[7]) << 56))

#define CLI_BUFSZ 1024
#define CLI_MAX_PAREN 1
#define CLI_MAX_WORD_LEN 8
#define CLI_MAX_LIST_WORDS 5
#define CLI_MAX_VARS 64
#define CLI_ERR_MAX_PAREN   fourcc("maxp") /* nesting too deep */
#define CLI_ERR_MAX_WORD    fourcc("wlen") /* word too long */
#define CLI_ERR_MAX_LIST    fourcc("llen") /* list too long */
#define CLI_ERR_SYNTAX_CALL fourcc("stxc")
#define CLI_ERR_SYNTAX_EVAL fourcc("stxe")
#define CLI_ERR_SYNTAX_NUM  fourcc("stxn")
#define CLI_ERR_SYNTAX_HEX  fourcc("stxh")
#define CLI_ERR_ARGS        fourcc("args") /* illegal argument values */
#define CLI_ERR_UNDEFINED   fourcc("undf")
#define CLI_ERR_MAX_OUT     fourcc("maxo") /* outbut buffer overflow */
#define CLI_TYPE_UINT64 0
#define CLI_TYPE_INT64 1
#define CLI_TYPE_DOUBLE 2
#define CLI_TYPE_STR256 3
#define CLI_TYPE_STR1024 4
#define CLI_TYPE_FUNC 5

struct cli_var {
  uint64_t word;
  uint64_t value_u64;
  int64_t value_i64;
  double value_dbl;
  void *func;
  uint8_t type;
  uint8_t return_type;
  uint8_t num_args;
};

void cli_reset();
void cli_reset_out();
void cli_init();
void cli_char(char c);
char* cli_get_out();
int cli_get_out_pos();
int cli_add_func(char word[static 9], void *funcptr, uint8_t num_args, uint8_t return_type);
int cli_add_word(char word[static 9], uint64_t value);

#endif
