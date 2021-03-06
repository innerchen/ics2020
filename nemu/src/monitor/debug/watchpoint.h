#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <common.h>

typedef struct watchpoint {
  int NO;
  char *expr;
  word_t value;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

} WP;


extern WP* new_wp();
extern void free_wp(WP * wp);
extern bool delete_wp(int no);
extern void watchpoint_display();
extern WP* check_watchpoint();

#endif
