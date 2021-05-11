#include "watchpoint.h"
#include "expr.h"
#include <stdlib.h>

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
  if (free_ != NULL) {
    WP *wp = free_;
    wp->expr = NULL;
    free_ = free_->next;
    wp->next = head;
    head = wp;
    return wp;
  }
  return NULL;
}

void free_wp(WP * wp) {
  if (wp != NULL && head != NULL) {
    WP * p = head;
    while (p->next != NULL && p->next != wp) {
      p = p->next;
    }
    if (p->next == wp) {
      if (!wp->expr) {
        free(wp->expr);
      }
      p->next = wp->next;
      wp->next = free_;
      free_ = wp;
    }
  }

}

