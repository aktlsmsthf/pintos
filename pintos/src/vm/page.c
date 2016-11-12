#include "vm/page.h"
#include <hash.h>
#include "threads/malloc.h"

static unsigned page_hash(const struct hash_elem *element, void *aux){
  struct spt_entry *spte = hash_entry(element, struct spt_entry, elem);
  return hash_int((int)spte->page);
}

static bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux){
  struct spt_entry *sa = hash_entry(a, struct spt_entry, elem);
  struct spt_entry *sb = hash_entry(b, struct spt_entry, elem);
  return sa->page < sb->page;
}

void spt_alloc(struct hash * spt, void * page){
  struct spt_entry *spte = malloc(sizeof(struct spt_entry));
  spte->page = page;
  hash_insert(spt ,&spte->elem);
}

void spt_init(struct hash *spt){
  hash_init(spt, page_hash, page_less, NULL);
}
