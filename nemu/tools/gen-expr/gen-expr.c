#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static int buf_n = 0;

void gen_num() {

  if (buf_n == -1) {
    return;
  }

  unsigned int num = 0;
  for (int i = 0; i < sizeof(num); i++) {
    num += rand() % 256;
    num <<= 8;
  }

  int count = snprintf(buf + buf_n, sizeof(buf) - buf_n, "%uu", num);
  if (count >= 0 && count < sizeof(buf) - buf_n) {
    buf_n += count;
  }
  else {
    buf_n = -1;
  }
}

void gen_str(char* s) {
  if (buf_n == -1) {
    return;
  }
  int count = snprintf(buf + buf_n, sizeof(buf) - buf_n, "%s", s);
  if (count >= 0 && count < sizeof(buf) - buf_n) {
    buf_n += count;
  }
  else {
    buf_n = -1;
  }
}

void gen_op() {
  if (buf_n == -1) {
    return;
  }
  switch(rand() % 7) {
    case 0:
      gen_str("+");
      break;
    case 1:
      gen_str("-");
      break;
    case 2:
      gen_str("*");
      break;
    case 3:
      gen_str("/");
      break;
    case 4:
      gen_str("==");
      break;
    case 5:
      gen_str("!=");
      break;
    case 6:
      gen_str("&&");
      break;
  }
}

void gen_expr() {

  if (buf_n == -1) {
    return;
  }

  switch(rand() % 3) {
    case 0:
      gen_num();
      break;
    case 1:
      gen_str("("); gen_expr(); gen_str(")");
      break;
    default:
      gen_expr(); gen_op(); gen_expr();
  }

}

static inline void gen_rand_expr() {

  while (1) {
    buf_n = 0;
    gen_expr();
    if (buf_n != -1) {
      break;
    }
  }

}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -Werror=div-by-zero -o /tmp/.expr 2>/dev/null");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    unsigned result;
    if (fscanf(fp, "%u", &result) == 1) {
      char *p = buf;
      while (1) {
        p = strchr(p, 'u');
        if (p != NULL) {
          *p = ' ';
        }
        else {
          break;
        }
      }
      printf("%u %s\n", result, buf);
    }
    pclose(fp);

  }
  return 0;
}

