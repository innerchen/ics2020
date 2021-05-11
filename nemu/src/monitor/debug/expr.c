#include <isa.h>
#include <memory/vaddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NE, TK_AND, TK_DEC, TK_HEX, TK_REG, TK_NEG, TK_DEREF,

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\(", '('},
  {"\\)", ')'},
  {"\\+", '+'},
  {"-", '-'},
  {"\\*", '*'},
  {"/", '/'},
  {"==", TK_EQ},        // equal
  {"!=", TK_NE},        // not equal
  {"&&", TK_AND},       // and
  {"0x[0-9a-fA-F]+", TK_HEX},
  {"[0-9]+", TK_DEC},
  {"\\$[a-z]+", TK_REG},
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[65536] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          default:
            if (substr_len >= sizeof(tokens[nr_token].str)) {
              printf("the token is too long %s\n", substr_start);
              return false;
            }

            if (nr_token >= sizeof(tokens) / sizeof(Token)) {
              printf("too many tokens\n");
              return false;
            }

            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

int check_parentheses(int p, int q) {

  int n = 0;
  bool is_parent_exp = tokens[p].type == '(' && tokens[q].type == ')';

  for (int i = p; i <= q; i++) {
    int type = tokens[i].type;
    if (type == '(') {
      n++;
      continue;
    }
    if (type == ')') {
      n--;
      if (n < 0) {
        return -1;
      }
      if (is_parent_exp && n == 0) {
        is_parent_exp = i == q;
      }
    }
  }
  return n == 0 ? is_parent_exp : -1;
}

int get_binary_operator_priority(int t) {
  switch(t) {
    case '*': return 3;
    case '/': return 3;
    case '+': return 4;
    case '-': return 4;
    case TK_EQ: return 7;
    case TK_NE: return 7;
    case TK_AND: return 11;
  }
  return -1;
}

int get_binary_operator(int p, int q) {

  int op_priority = -1;
  int op_index = -1;
  int n = 0;

  for (int i = p; i <= q; i++) {

    int type = tokens[i].type;

    if (type == TK_DEC || type == TK_HEX || type == TK_REG || type == TK_NEG || type == TK_DEREF) {
      continue;
    }

    if (type == '(') {
      n++;
      continue;
    }

    if (type == ')') {
      Assert(n > 0, "parentheses dismatch");
      n--;
      continue;
    }

    if (n == 0) {
      int priority = get_binary_operator_priority(type);
      if (priority >= op_priority) {
        op_priority = priority;
        op_index = i;
      }
    }
  }
  return op_index;
}

int get_unary_operator(int p, int q) {
  if (tokens[p].type == TK_NEG || tokens[p].type == TK_DEREF) {
    return p;
  }
  return -1;
}

word_t eval(int p, int q, bool *success) {

  if (p > q) {
    *success = false;
    return 0;
  }

  if (p == q) {
    word_t v = 0;
    switch(tokens[p].type) {
      case TK_DEC:
        sscanf(tokens[p].str, "%u", &v);
        *success = true;
        break;
      case TK_HEX:
        sscanf(tokens[p].str, "%x", &v);
        *success = true;
        break;
      case TK_REG:
        v = isa_reg_str2val(tokens[p].str + 1, success);
        break;
      default:
        *success = false;
    }
    return v;
  }

  int valid = check_parentheses(p, q);
  if (valid == -1) {
    Log("invalid input");
    *success = false;
    return 0;
  }

  if (valid == 1) {
    return eval(p + 1, q - 1, success);
  }

  int op_index = get_binary_operator(p, q);
  if (op_index != -1) {
    word_t v1 = eval(p, op_index - 1, success);
    if (!success) return 0;
    word_t v2 = eval(op_index + 1, q, success);
    if (!success) return 0;
    switch(tokens[op_index].type) {
      case '+': return v1 + v2;
      case '-': return v1 - v2;
      case '*': return v1 * v2;
      case '/':
        if (v2 == 0) {
          *success = false;
          printf("Divied by zero");
          return 0;
        }
        return v1 / v2;
      case TK_EQ: return v1 == v2;
      case TK_NE: return v1 != v2;
      case TK_AND: return v1 && v2;
    }
  }

  op_index = get_unary_operator(p, q);
  if (op_index != -1) {
    word_t v = eval(op_index + 1, q, success);
    if (!success) return 0;
    switch(tokens[op_index].type) {
      case TK_NEG: return -v;
      case TK_DEREF: return vaddr_read(v, sizeof(word_t));
    }
  }

  Log("invalid input");

  *success = false;
  return 0;

}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i++) {
    int type = tokens[i].type;
    if (type == '-' || type == '*') {
      if (i == 0) {
        tokens[i].type = (type == '-' ? TK_NEG : TK_DEREF);
        continue;
      }
      int pre_type = tokens[i - 1].type;
      if (pre_type != ')' && pre_type != TK_DEC && pre_type != TK_HEX && pre_type != TK_REG) {
        tokens[i].type = (type == '-' ? TK_NEG : TK_DEREF);
      }
    }
  }

  return eval(0, nr_token - 1, success);

}

