#include <list.h>
#include "threads/synch.h"
#include "vm/page.h"

struct list frame_table;
struct lock frame_lock;

struct frame_entry{
  void *frame;
  struct spt_entry *spte;
  struct list_elem elem;
};

void frame_init(void);
void frame_alloc(void* frame);
struct frame_entry* frame_evict(void);
struct frame_entry* frame_pop(void);
