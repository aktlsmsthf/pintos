#ifndef FRAME_H
#define FRAME_H
#include <list.h>
#include "threads/synch.h"
#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/thread.h"

struct list frame_table;
struct lock frame_lock;

struct frame_entry{
  void *frame;
  bool in_swap;
  size_t swap_where;
  struct spt_entry * spte;
  struct list_elem elem;
};

void frame_init(void);
void frame_remove(struct frame_entry *fe);

void * frame_spt_alloc(struct hash * spt, void * page, bool writable, enum palloc_flags flags);
void * frame_evict(enum palloc_flags flags);
void * frame_evict1(enum palloc_flags flags);
struct frame_entry* frame_pop(void);
bool file_frame_alloc(struct spt_entry * spte);

#endif
