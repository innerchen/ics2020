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
    wp_pool[i].expr = NULL;
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

void free_wp(WP *wp) {
  if (wp != NULL && head != NULL) {
    WP *p = head;
    while (p->next != NULL && p->next != wp) {
      p = p->next;
    }
    if (p->next == wp) {
      Assert(wp->expr, "expr point is NULL");
      free(wp->expr);
      wp->expr = NULL;
      p->next = wp->next;
      wp->next = free_;
      free_ = wp;
    }
  }
}

bool delete_wp(int no) {

  if (no >= 0 && no < NR_WP) {
    if (wp_pool[no].expr != NULL) {
      free_wp(&wp_pool[no]);
      return true;
    }
  }

  return false;
}

void watchpoint_display() {
  WP *wp;
  printf("No\tExpr\t\tValue\n");
  for (wp = head; wp != NULL; wp = wp->next) {
    printf("%d\t%s\t\t%u\n", wp->NO, wp->expr, wp->value);
  }
  return;
}

WP* check_watchpoint(word_t *old_value) {

  WP *wp;
  for (wp = head; wp != NULL; wp = wp->next) {
    bool success;
    word_t new_value  = expr(wp->expr, &success);
    if (success && new_value != wp->value) {
      *old_value = wp->value;
      wp->value = new_value;
      return wp;
    }
  }
  return NULL;
}

