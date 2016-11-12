#ifndef FRAME_H
#define FRAME_H
#include <list.h>
#include "threads/synch.h"
#include "vm/page.h"

struct list frame_table;
struct lock frame_lock;

struct frame_entry{
  void *frame;
  bool in_swap;
  int swap_where;
  bool is_free;
  struct list_elem elem;
};

void frame_init(void);
void * frame_alloc(void* frame);
void * frame_evict(void);
struct frame_entry* frame_pop(void);

#endif
