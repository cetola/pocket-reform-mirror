#include "sysctl.h"
#include "cli.h"
#include <stdint.h>

#define CLI_ST_LIST 0
#define CLI_ST_WORD 1
#define CLI_ST_NUMBER 2
#define CLI_ST_HEX 3
#define CLI_ST_ERROR 5

//#define CLI_DEBUG_STATE
//#define CLI_DEBUG_EVAL

static uint8_t cli_state = CLI_ST_LIST;
static int8_t cli_word_idx; // current index of word in current list
static int8_t cli_paren; // current level of nesting
static uint64_t cli_err; // current error code
static uint64_t cli_word; // current word (its value)
static int8_t cli_word_len; // current word's length (number of characters)
static uint64_t cli_list[CLI_MAX_LIST_WORDS]; // current list
static char cli_out[CLI_BUFSZ]; // output buffer
static int cli_out_pos; // position in output buffer (= output data available)
static struct cli_var cli_vars[CLI_MAX_VARS]; // known variables (named words)
static int cli_num_vars; // number of known variables

/* resets input/parser state. call cli_reset_out() to reset output state! */
void cli_reset() {
  cli_word = 0;
  cli_word_len = 0;
  cli_paren = 0;
  cli_word_idx = 0;
  cli_state = CLI_ST_LIST;
}

void cli_reset_out() {
  cli_err = 0;
  cli_out[0] = 0;
  cli_out_pos = 0;
}

/* returns number of chars still available in output buffer */
int cli_out_available() {
  return CLI_BUFSZ - cli_out_pos;
}

char *cli_out_ptr() {
  return &cli_out[cli_out_pos];
}

int cli_get_out_pos() {
  return cli_out_pos;
}

char* fourcc_to_str(uint32_t word, char buf[static 5]) {
  for (int i = 0; i < 5; i++) {
    buf[i] = word&0xff;
    word >>= 8;
  }
  buf[4] = 0;
  return buf;
}

char* eightcc_to_str(uint64_t word, char buf[static 9]) {
  for (int i = 0; i < 8; i++) {
    buf[i] = word&0xff;
    word >>= 8;
  }
  buf[8] = 0;
  return buf;
}

int cli_out_str(char *str) {
  uint32_t len = snprintf(cli_out_ptr(), cli_out_available(), "%s", str);
  if (len < strlen(str)) {
    return 0;
  }
  cli_out_pos += len;
  return len;
}

uint64_t cli_get_err() {
  return cli_err;
}

void cli_error(uint64_t err) {
  cli_err = err;
  char buf[5];
  cli_out_pos = snprintf(cli_out, CLI_BUFSZ, "(err %s)", fourcc_to_str(err, buf));
  cli_reset();
}

int cli_out_int64(int64_t number) {
  char buf[22]; // (i64 -9223372036854775808)
  snprintf(buf, sizeof(buf), "(i64 %lld)", number);
  buf[sizeof(buf)-1] = 0;
  return cli_out_str(buf);
}

int cli_out_uint64(uint64_t number) {
  char buf[22+6]; // (u64 18446744073709551616)
  snprintf(buf, sizeof(buf), "(u64 %llu)", number);
  buf[sizeof(buf)-1] = 0;
  return cli_out_str(buf);
}

int cli_out_double(double number) {
  char buf[32+6]; // TODO: max output size of a double?
  snprintf(buf, sizeof(buf), "(f64 %10.10f)", number);
  buf[sizeof(buf)-1] = 0;
  return cli_out_str(buf);
}

// TODO generalize for any list?
uint64_t cli_list_vars(int64_t offset, int64_t limit_) {
#ifdef CLI_DEBUG_EVAL
  printf("[cli_list_vars] offset: %lld limit: %lld\n", offset, limit_);
#endif
  if (offset >= cli_num_vars || offset < 0 || limit_ < 0) {
    cli_error(CLI_ERR_ARGS);
    return 0;
  }
  int64_t limit = cli_num_vars - offset;
  // caller wants less entries than there are
  if (limit_ && limit_ < limit) {
    limit = limit_;
  }
  cli_out_str("(list ");
  cli_out_int64(offset);
  cli_out_str(" ");
  cli_out_int64(limit);
  cli_out_str(" ");
  for (int i = offset; i < offset + limit; i++) {
    char buf[9];
    if (!cli_out_str(eightcc_to_str(cli_vars[i].word, buf))) {
      cli_err = CLI_ERR_MAX_OUT;
      return 0;
    }
    if (i < offset + limit - 1) cli_out_str(" ");
  }
  cli_out_str(")");
  return 1;
}

int cli_word_valid(uint64_t word) {
  return ((word & 0xff) >= 'a' || (word & 0xff) <= 'z') || ((word & 0xff) >= 'A' || (word & 0xff) <= 'Z');
}

uint64_t cli_set_var_uint64(uint64_t word, uint64_t value) {
#ifdef CLI_DEBUG_EVAL
  char buf[9];
  printf("[cli_set_var_uint64] %s 0x%llx\n", eightcc_to_str(word, buf), value);
#endif
  if (!cli_word_valid(word)) {
    cli_error(CLI_ERR_ARGS);
    return 0;
  }
  for (int i = 0; i < cli_num_vars; i++) {
    if (cli_vars[i].word == word) {
      cli_vars[i].value_u64 = value;
      return value;
    }
  }
  cli_vars[cli_num_vars++] = (struct cli_var)
  {
    .word = word,
    .value_u64 = value,
    .type = CLI_TYPE_UINT64
  };
  return value;
}

int cli_add_func(char word[static 9], void *funcptr, uint8_t num_args, uint8_t return_type) {
  if (cli_num_vars >= CLI_MAX_VARS) {
    return 0;
  }
  cli_vars[cli_num_vars++] = (struct cli_var){
    .word = eightcc(word),
    .type = CLI_TYPE_FUNC,
    .func = funcptr,
    .num_args = num_args,
    .return_type = return_type,
  };
  return cli_num_vars;
}

int cli_add_word(char word[static 9], uint64_t value) {
  if (cli_num_vars >= CLI_MAX_VARS) {
    return 0;
  }
  cli_vars[cli_num_vars++] = (struct cli_var){
    .word = eightcc(word),
    .type = CLI_TYPE_UINT64,
    .value_u64 = value,
  };
  return cli_num_vars;
}

void cli_init() {
  cli_reset();
  cli_reset_out();
  cli_num_vars = 0;

  cli_add_func("vars\0\0\0\0", cli_list_vars, 2, CLI_TYPE_UINT64);
  cli_add_func("set\0\0\0\0\0", cli_set_var_uint64, 2, CLI_TYPE_UINT64);
}

void cli_eval() {
#ifdef CLI_DEBUG_EVAL
  //printf("# [cli_eval] cli_word_idx: %d\n", cli_word_idx);
#endif
  if (cli_word_idx <= 0) {
    return;
  }
  // word 0 is the operation (function pointer)
  uint64_t op = cli_list[0];
  int num_args = cli_word_idx - 1;
#ifdef CLI_DEBUG_EVAL
  char buf[9];
  printf("# [cli_eval] op: 0x%llx (%s) arity: %d\n", op, eightcc_to_str(op, buf), num_args);
#endif
  for (int i=0; i < cli_num_vars; i++) {
    struct cli_var* var = &cli_vars[i];
    //printf("# [cli_eval] candidate: 0x%llx\n", var->word);
    if (op == var->word) {
#ifdef CLI_DEBUG_EVAL
      //printf("# [cli_eval] word %d matches var table entry %d, type %d\n", cli_word_idx, i, var->type);
#endif
      if (var->type == CLI_TYPE_FUNC && var->func) {
        // maximum 4 args, all default to 0
        uint64_t args[4] = { 0, 0, 0, 0 };
        for (int j = 0; j < num_args && j < 4; j++) {
          // if an arg is passed, set it
          args[j] = cli_list[j + 1];
        }
        if (var->return_type == CLI_TYPE_UINT64) {
          uint64_t (*func)(uint64_t, uint64_t, uint64_t, uint64_t) = var->func;
          uint64_t result = func(args[0], args[1], args[2], args[3]);
          cli_out_uint64(result);
          return;
        } else if (var->return_type == CLI_TYPE_INT64) {
          int64_t (*func)(uint64_t, uint64_t, uint64_t, uint64_t) = var->func;
          int64_t result = func(args[0], args[1], args[2], args[3]);
          cli_out_int64(result);
          return;
        } else if (var->return_type == CLI_TYPE_DOUBLE) {
          double (*func)(uint64_t, uint64_t, uint64_t, uint64_t) = var->func;
          double result = func(args[0], args[1], args[2], args[3]);
          cli_out_double(result);
          return;
        }
        return;
      }
      if (var->type == CLI_TYPE_UINT64) {
        cli_out_uint64(var->value_u64);
        return;
      } else if (var->type == CLI_TYPE_INT64) {
        cli_out_int64(var->value_i64);
        return;
      } else if (var->type == CLI_TYPE_DOUBLE) {
        cli_out_double(var->value_dbl);
        return;
      }
    }
  }
  cli_error(CLI_ERR_UNDEFINED);
}

char *cli_get_out() {
  return cli_out;
}

void cli_char(char c) {
#ifdef CLI_DEBUG_STATE
  printf("# [cli] s:%d c:%d w:%d\n", cli_state, c, cli_word_idx);
#endif
  if (cli_state == CLI_ST_LIST) {
    if (c == ' ') return;
    if (c == ')' || (c == '\n' && cli_paren == 0)) {
      cli_eval();
      cli_reset();
      return;
    }
    /* start a new word */
    if (cli_word_idx >= CLI_MAX_LIST_WORDS) {
      cli_error(CLI_ERR_MAX_WORD);
      return;
    }
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      cli_state = CLI_ST_WORD;
      cli_word = (uint64_t)c;
      cli_word_len = 1;
      return;
    }
    if (c >= '0' && c <= '9') {
      if (cli_word_idx == 0) {
        /* for backwards compat (old LPC API), the first word can start with a single digit */
        cli_state = CLI_ST_WORD;
        cli_word = (uint64_t)c;
        cli_word_len = 1;
      } else {
        cli_state = CLI_ST_NUMBER;
        cli_word = (uint64_t)(c-'0');
      }
      return;
    }
    if (c == '(') {
      cli_state = CLI_ST_LIST;
      cli_word_idx = 0;
      cli_paren++;
      /* TODO switch current list! */
      if (cli_paren > CLI_MAX_PAREN) {
        cli_error(CLI_ERR_MAX_PAREN);
      }
      return;
    }
    /* index 0 must be a word (callable) */
    if (cli_word_idx == 0) {
      cli_error(CLI_ERR_SYNTAX_CALL);
      return;
    }
    /* index 1+ (arguments) can be numbers or lists */
    if (c == '#') {
      cli_state = CLI_ST_HEX;
      cli_word = 0;
      return;
    }
  } else if (cli_state == CLI_ST_NUMBER) {
    if (c >= '0' && c <= '9') {
      cli_word *= 10;
      cli_word += (uint64_t)(c-'0');
      return;
    } else if (c == ' ' || c == ')' || c == '\n') {
      cli_state = CLI_ST_LIST;
      cli_list[cli_word_idx++] = cli_word;
      if (c == ')' || (c == '\n' && cli_paren == 0)) {
        cli_eval();
        cli_reset();
      }
      return;
    }
    cli_error(CLI_ERR_SYNTAX_NUM);
    return;
  } else if (cli_state == CLI_ST_WORD) {
    if (c == ' ' || c == ')' || c == '\n') {
      cli_state = CLI_ST_LIST;
#ifdef CLI_DEBUG_STATE
      printf("# [cli_word] storing word at idx %d\n", cli_word_idx);
#endif
      cli_list[cli_word_idx++] = cli_word;
      if (c == ')' || (c == '\n' && cli_paren == 0)) {
        cli_eval();
        cli_reset();
      }
      return;
    }
    cli_word |= (uint64_t)c << (cli_word_len*8);
    cli_word_len++;
#ifdef CLI_DEBUG_STATE
    printf("# [cli_word] 0x%llx (len: %d)\n", cli_word, cli_word_len);
#endif

    if (cli_word_len > CLI_MAX_WORD_LEN) {
      cli_error(CLI_ERR_MAX_WORD);
      return;
    }
    // TODO filter illegal chars
    return;
  } else if (cli_state == CLI_ST_HEX) {
    if (c >= '0' && c <= '9') {
      cli_word = cli_word << 4;
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
    if (c == ' ' || c == ')' || c == '\n') {
      cli_state = CLI_ST_LIST;
      cli_list[cli_word_idx++] = cli_word;
      if (c == ')' || (c == '\n' && cli_paren == 0)) {
        cli_eval();
        cli_reset();
        return;
      }
      return;
    }
    cli_error(CLI_ERR_SYNTAX_HEX);
    return;
  }
}
