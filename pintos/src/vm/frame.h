#include <list.h>
#include "threads/synch.h"

struct list frame_table;
struct lock frame_lock;

struct frame_elem{
  void *frame;
  struct list_elem elem;
}

void frame_init(void);
void frame_push(void *frame);
struct frame_elem* frame_pop(void);
