#ifndef PAGE_H
#define PAGE_H

#include <hash.h>
#include "threads/palloc.h"
#include "filesys/file.h"

struct spt_entry{
  void *page;
  struct hash_elem elem;
  struct frame_entry* fe;
  bool writable;
  enum palloc_flags flags;
  bool lazy;
  uint32_t read_bytes;
  uint32_t zero_bytes;
  struct file *file;
  struct thread *t;
  off_t ofs;
};

void spt_init(struct hash *spt);
struct spt_entry * spte_find(void *page);
void spt_destroy (struct hash *spt,bool pe);
void spt_alloc_lazy(struct hash * spt, void * page, bool writable, enum palloc_flags flags, uint32_t read_bytes, uint32_t zero_bytes
                    , struct file *file, off_t ofs);
/*
void spt_alloc(struct hash *spt, void *page);
*/
#endif
