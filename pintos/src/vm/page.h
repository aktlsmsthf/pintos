#ifndef PAGE_H
#define PAGE_H

#include <hash.h>

struct spt_entry{
  void *page;
  struct hash_elem elem;
  struct frame_entry* fe;
};

void spt_init(struct hash *spt);
/*
void spt_alloc(struct hash *spt, void *page);
*/
#endif
