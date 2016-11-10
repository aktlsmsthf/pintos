#include "vm/page.h"
#include <hash.h>

unsigned hash_hash_func(const struct hash_elem *element, void *aux){
  struct spt_entry *spte = hash_entry(element, struct spt_entry, elem);
  return hash_int((int)spte->page);
}

bool hash_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux){
  struct sptentry *sa = hash_entry(elemtent, struct spt_entry, elem);
  struct sptentry *sb = hash_entry(elemtent, struct spt_entry, elem);
  return sa->page < sb->page;
}

void spt_init(struct hash spt){
  hash_init(spt, hash_hash_func, hash_less_func);
}

void spt_alloc(struct hash spt, void* page){
  struct spt_entry *spte = malloc(sizeof(struct spt_entry));
  spte->page = page;
  spte->is_swap = false;
  spte->index = -1;
  
  hash_insert(spt, spte->elem);
}
