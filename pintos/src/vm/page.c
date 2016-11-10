#include "vm/page.h"
#include <hash.h>

static unsigned hash_hash_func(const struct hash_elem *element, void *aux){
  struct spt_entry *spte = hash_entry(element, struct spt_entry, elem);
  return hash_int((int)spte->page);
}

static bool hash_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux){
  struct spt_entry *sa = hash_entry(a, struct spt_entry, elem);
  struct spt_entry *sb = hash_entry(b, struct spt_entry, elem);
  return sa->page < sb->page;
}

void spt_init(struct hash *spt){
  hash_init(spt, hash_hash_func, hash_less_func, NULL);
}

void spt_alloc(struct hash *spt, void* page){
  struct spt_entry *spte = malloc(sizeof(struct spt_entry));
  spte->page = page;

  hash_insert(spt, &spte->elem);
}
