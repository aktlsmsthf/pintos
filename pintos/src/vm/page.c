#include "vm/page.h"
#include <hash.h>
#include "threads/malloc.h"
#include "threads/thread.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "filesys/file.h"

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
  
  if(!spte->lazy){
    if(spte->fe->in_swap){
    swap_remove(spte->fe->swap_where);}
     frame_remove(spte->fe);
  }
  free(spte);
}
void spt_destroy (struct hash *spt,bool pe){
  spt->aux=&pe;
  hash_destroy (spt, page_destroy_func);
}

struct spt_entry * spte_find(void * page){
  struct spt_entry spte;
  struct hash_elem * e;
  spte.page = page;
  e = hash_find(&thread_current()->spt, &spte.elem);
  return e != NULL ? hash_entry(e, struct spt_entry, elem) : NULL;
}

void spt_init(struct hash *spt){
  hash_init(spt, page_hash, page_less, NULL);
}

void spt_alloc_lazy(struct hash * spt, void * page, bool writable, enum palloc_flags flags, uint32_t read_bytes, uint32_t zero_bytes
                    , struct file *file, off_t ofs){
                   
  struct spt_entry *spte = malloc(sizeof(struct spt_entry));
  spte -> page = page;
  spte -> writable = writable;
  spte -> flags = flags;
  spte -> read_bytes = read_bytes;
  spte -> zero_bytes = zero_bytes;
  spte -> lazy = true;
  spte -> file = file;
  spte -> t = thread_current();
  spte -> ofs = ofs;
  hash_insert(spt, &spte->elem);
}
