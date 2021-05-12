#include <isa.h>
#include "expr.h"
#include "memory/vaddr.h"
#include "watchpoint.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
int is_batch_mode();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
  int n = 1;
  if (args) {
    sscanf(args, "%d", &n);
  }
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {

  char *arg = strtok(NULL, " ");

  if (strcmp(arg, "r") == 0) {
    isa_reg_display();
    return 0;
  }
  if (strcmp(arg, "w") == 0) {
    watchpoint_display();
    return 0;
  }
  return 0;
}

static int cmd_x(char *args) {
  char *arg = strtok(NULL, " ");
  int n;
  if (sscanf(arg, "%d", &n) == 1) {
    bool success;
    word_t v = expr(args + strlen(arg) + 1, &success);
    if (success) {
      for (int i = 0; i < n; i++) {
        printf("%x\t%u\n", v + i * 4, vaddr_read(v + i * 4, 4));
      }
    }
  }
  return 0;
}

static int cmd_p(char *args) {
  bool success;
  word_t v = expr(args, &success);
  if (success) {
    printf("%u\n", v);
  }
  return 0;
}

static int cmd_w(char *args) {
  bool success;
  word_t v = expr(args, &success);
  if (success) {
    WP* wp = new_wp();
    if (wp == NULL) {
      printf("no extra watchpoint");
      return 0;
    }
    wp->expr = malloc(sizeof(args) + 1);
    strcpy(wp->expr, args);
    wp->value = v;
  }
  return 0;
}

static int cmd_d(char *args) {
  if (args) {
    int no;
    if (scanf("%d", &no) == 1) {
      delete_wp(no);
    }
  }
  return 0;
}

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Execute N instructions", cmd_si},
  { "info", "Print info", cmd_info},
  { "x", "Print memory", cmd_x},
  { "p", "Calculate expression", cmd_p},
  { "w", "Calculate expression", cmd_w},
  { "d", "Delete watchpoint", cmd_d},

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop() {
  if (is_batch_mode()) {
    cmd_c(NULL);
    return;
  }

  /*
  FILE *fp = fopen("./tools/gen-expr/input", "r");
  if (fp) {
    word_t result;
    char expr_buf[65536];
    while (1) {
      if (fscanf(fp, "%u", &result) != 1) {
        break;
      };
      if (fgets(expr_buf, 65536, fp) == NULL) {
        break;
      }
      char *p = strchr(expr_buf, '\n');
      if (p) {
        *p = '\0';
      }
      bool success;
      word_t v = expr(expr_buf, &success);
      Assert(success, "%s\n", expr_buf);
      if (success) {
        Assert(result == v, "%s\n%u\n%u\n", expr_buf, result, v);
      }
    }
    fclose(fp);
  }
  */

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
