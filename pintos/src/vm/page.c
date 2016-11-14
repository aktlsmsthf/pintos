#include "vm/page.h"
#include <hash.h>
#include "threads/malloc.h"
#include "threads/thread.h"
#include "vm/frame.h"
#include "vm/swap.h"

static unsigned page_hash(const struct hash_elem *element, void *aux){
  struct spt_entry *spte = hash_entry(element, struct spt_entry, elem);
  return hash_int((int)spte->page);
}

static bool page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux){
  struct spt_entry *sa = hash_entry(a, struct spt_entry, elem);
  struct spt_entry *sb = hash_entry(b, struct spt_entry, elem);
  return sa->page < sb->page;
}
static void page_destroy_func(struct hash_elem *helem, void *aux){
  struct spt_entry *spte = hash_entry(helem, struct spt_entry,elem);
  if(spte->fe->in_swap){
    swap_remove(spte->fe->swap_where);}
  frame_remove(spte->fe);  
  free(spte);
}
void spt_destroy (struct hash *spt){
  hash_destroy (spt, page_destroy_func);
}
/*
void spt_alloc(struct hash * spt, void * page){
  struct spt_entry *spte = malloc(sizeof(struct spt_entry));
  spte->page = page;
  hash_insert(spt ,&spte->elem);
}
*/
struct spt_entry * spte_find(void * page){
  lock_acquire(&frame_lock);
  struct spt_entry spte;
  struct hash_elem * e;
  spte.page = page;
  e = hash_find(&thread_current()->spt, &spte.elem);
  return e != NULL ? hash_entry(e, struct spt_entry, elem) : NULL;
  lock_release(&frame_lock);
}

void spt_init(struct hash *spt){
  hash_init(spt, page_hash, page_less, NULL);
}
