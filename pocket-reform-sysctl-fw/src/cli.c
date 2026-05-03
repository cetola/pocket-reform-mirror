#include "sysctl.h"
#include "cli.h"
#include <stdint.h>

#define CLI_ST_LIST 0
#define CLI_ST_WORD 1
#define CLI_ST_NUMBER 2
#define CLI_ST_HEX 3
#define CLI_ST_ERROR 5

static uint8_t cli_state = CLI_ST_INIT;
static int8_t cli_word_idx;
static uint64_t cli_error = 0;
static uint64_t cli_word;
static uint64_t cli_list[CLI_MAX_LIST_WORDS];
static char cli_out[CLI_BUFSZ];
static struct cli_vars[CLI_MAX_VARS];
static int cli_num_vars;

void cli_init() {
  cli_num_vars = 0;

  cli_vars[cli_num_vars++] = {
    .word = eightcc('p','o','w','e','r','o','n',0),
    .func = turn_som_power_on_v20,
    .num_args = 0,
  };
  cli_vars[cli_num_vars++] = {
    .word = eightcc('1','p',0,0,0,0,0,0),
    .func = turn_som_power_on,
    .num_args = 0,
  };
  cli_vars[cli_num_vars++] = {
    .word = eightcc('0','p',0,0,0,0,0,0),
    .func = turn_som_power_off,
    .num_args = 0,
  };
}

void cli_reset() {
  cli_word_idx = -1;
  cli_state = CLI_ST_INIT;
}

void cli_reset_error() {
  cli_error = 0;
  cli_out[0] = 0;
}

void cli_error(uint64_t err) {
  snprintf(cli_out, CLI_BUFSZ, "(err %lld)", cli_error);
  cli_reset();
}

void cli_eval() {
  if (cli_word_idx < 0) {
    cli_error(CLI_ERR_SYNTAX);
    return;
  }
  // word 0 is the operation (function pointer)
  uint64_t op = cli_list[0];
  int num_args = cli_word_idx-1;
  /* TODO make list introspectable */
  for (int i=0; i<cli_num_vars; i++) {
    struct cli_var* var = &cli_vars[i];
    if (op == var->word && num_args == var->num_args) {
      if (var->func) {
        if (var->num_args == 1) {
          ((void*)(uint64_t*))(var->func)(cli_list[1]);
        }
      }
    }
  }
}

void cli_char(char c) {
  if (cli_state == CLI_ST_LIST) {
    if (c == ' ') return;
    if (c == ')') {
      cli_eval();
      return;
    }
    /* start a new word */
    cli_word_idx++;
    if (cli_word_idx >= CLI_MAX_LIST_WORDS) {
      cli_error(CLI_ERR_MAX_WORD);
      return;
    }
    if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') {
      cli_state = CLI_ST_WORD;
      cli_word = (uint64_t)c;
      return;
    }
    /* index 0 must be a word (callable) */
    if (cli_word_idx > 0) {
      cli_error(CLI_ERR_SYNTAX);
      return;
    }
    /* index 1+ (arguments) can be numbers or lists */
    if (c >= '0' && c <= '9') {
      cli_state = CLI_ST_NUMBER;
      cli_word = (uint64_t)(c-'0');
      return;
    }
    if (c == '#') {
      cli_state = CLI_ST_HEX;
      cli_word = 0;
      return;
    }
    if (c == '(') {
      cli_state = CLI_ST_LIST;
      cli_num_words = 0;
      cli_paren++;
      /* TODO switch current list! */
      if (cli_paren > CLI_MAX_PAREN) {
        cli_error(CLI_ERR_MAX_PAREN);
      }
      return;
    }
  } else if (cli_state == CLI_ST_NUMBER) {
    if (c >= '0' && c <= '9') {
      cli_word *= 10;
      cli_word += (uint64_t)(c-'0');
      return;
    } else if (c == ' ' || c == ')') {
      cli_state = CLI_ST_LIST;
      cli_list[cli_word_idx] = cli_word;
      if (c == ')') {
        cli_eval();
      }
      return;
    }
    cli_error(CLI_ERR_SYNTAX);
    return;
  } else if (cli_state == CLI_ST_WORD) {
    if (c == ' ') {
      cli_state = CLI_ST_LIST;
      cli_list[cli_word_idx] = cli_word;
      return;
    }
    cli_word = (cli_word << 8) + c;
    cli_word_len++;
    if (cli_word_len > CLI_MAX_WORD_LEN) {
      cli_error(CLI_ERR_MAX_WORD);
      return;
    }
    // TODO filter illegal chars
    return;
  } else if (cli_state == CLI_ST_HEX) {
    if (c >= '0' && c <= '9') {
      cli_word *= 10;
      cli_word += (uint64_t)(c-'0');
      cli_word_len++;
      if (cli_word_len > 16) {
        cli_error(CLI_ERR_MAX_WORD);
        return;
      }
      return;
    }
    if (c >= 'a' && c <= 'f') {
      cli_word = cli_word << 4;
      cli_word += (uint64_t)(10 + (c - 'a'));
      cli_word_len++;
      if (cli_word_len > 16) {
        cli_error(CLI_ERR_MAX_WORD);
        return;
      }
      return;
    }
    if (c == ' ' || c == ')') {
      cli_state = CLI_ST_LIST;
      if (c == ')') {
        cli_eval();
        return;
      }
      return;
    }
    cli_error(CLI_ERR_SYNTAX);
    return;
  }
}
