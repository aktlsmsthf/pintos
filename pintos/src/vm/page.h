#ifndef PAGE_H
#define PAGE_H

#include <hash.h>
#include "threads/palloc.h"

struct spt_entry{
  void *page;
  struct hash_elem elem;
  struct frame_entry* fe;
  bool writable;
  enum palloc_flags flags;
  bool lazy;
  uint32_t read_bytes;
  uint32_t zero_bytes;
};

void spt_init(struct hash *spt);
struct spt_entry * spte_find(void *page);
void spt_destroy (struct hash *spt,bool pe);
/*
void spt_alloc(struct hash *spt, void *page);
*/
#endif
