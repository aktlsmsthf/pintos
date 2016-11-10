#include <list.h>
#include "threads/synch.h"

struct list frame_table;
struct lock frame_lock;

struct frame_entry{
  void *frame;
  struct list_elem elem;
}

void frame_init(void);
void frame_alloc(void);
struct frame_entry* frame_pop(void);
