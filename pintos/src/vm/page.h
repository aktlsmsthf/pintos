#ifndef PAGE_H
#define PAGE_H

#include <hash.h>

struct spt_entry{
  void *page;
  struct hash_elem elem;
  struct frame_entry* fe;
  bool writable;
};

void spt_init(struct hash *spt);
struct spt_entry  spte_find(void *page);
void spt_destroy (struct hash *spt);
/*
void spt_alloc(struct hash *spt, void *page);
*/
#endif
