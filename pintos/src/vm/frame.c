#include "vm/frame.h"
#include <list.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "userprog/syscall.h"

#include "vm/page.h"

void frame_init(void){
  list_init(&frame_table);
  lock_init(&frame_lock);
}
void frame_remove(struct frame_entry *fe){
  
  lock_acquire(&frame_lock);
  
  if(!fe->in_swap){
    list_remove(&fe->elem);
  }
  if(fe->frame!=NULL){
  palloc_free_page(fe->frame);
  }

  pagedir_clear_page(fe->spte->t->pagedir, fe->spte->page);
  free(fe);
  
  lock_release(&frame_lock);
  
}
  
void * frame_spt_alloc( struct hash * spt, void * page, bool writable, enum palloc_flags flags){

  struct spt_entry *spte = malloc(sizeof(struct spt_entry));
  struct frame_entry *fe = malloc(sizeof(struct frame_entry));

  uint8_t *frame = palloc_get_page(flags);

  while(frame==NULL){
    frame=frame_evict(flags);
  }
  
  spte->page = page;
  spte->fe = fe;
  spte->writable = writable;
  spte->flags = flags;
  spte->lazy = false;
  spte->t = thread_current();
  spte->ofs = 0;
  hash_insert(spt ,&spte->elem);
  
  fe->frame = frame;
  fe->in_swap = 0;
  fe->swap_where = -1;
  fe->spte = spte;
  
  lock_acquire(&frame_lock);
  list_push_back(&frame_table, &fe->elem);
  lock_release(&frame_lock);
  return frame;
}

void* frame_evict(enum palloc_flags flags){
  lock_acquire(&frame_lock);
  void * ret;
  struct list_elem * frame_elem = list_front(&frame_table);
  struct frame_entry * fe;
  fe = list_entry(frame_elem, struct frame_entry, elem);
  while(!pagedir_is_dirty(fe->spte->t->pagedir, fe->spte->page), fe->frame == NULL || pagedir_is_accessed(fe->spte->t->pagedir ,fe->spte->page)){
      if(fe->frame != NULL){
        pagedir_set_accessed(fe->spte->t->pagedir ,fe->spte->page, false);
      }
      list_remove(frame_elem);
      list_push_back(&frame_table, frame_elem);
      frame_elem = list_front(&frame_table); 
      fe = list_entry(frame_elem, struct frame_entry, elem);
  }

  list_remove(&fe->elem);
  
  lock_release(&frame_lock);
  ret = swap_out(fe, flags);
  return ret;
}

bool file_frame_alloc(struct spt_entry * spte){
  struct frame_entry * fe = malloc(sizeof(struct frame_entry));
  void * frame = palloc_get_page(spte->flags);
  while(frame == NULL){ frame = frame_evict(spte->flags);}
  
  fe -> frame = frame;
  fe -> swap_where = -1;
  fe -> in_swap = 0;
  fe -> spte = spte;
  
  spte -> fe = fe;
  spte -> lazy = 0;
  
  file_read_at(spte->file, frame, spte->read_bytes, spte->ofs); 
  memset(frame+spte->read_bytes, 0, spte->zero_bytes);
  install_page(spte->page, frame, spte->writable);
  
  lock_acquire(&frame_lock);
  list_push_back(&frame_table, &fe->elem);
  lock_release(&frame_lock);
  return 1;
}
