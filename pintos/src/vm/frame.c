#include "vm/frame.h"
#include <list.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"

void frame_init(void){
  list_init(&frame_table);
  lock_init(&frame_lock);

}
void frame_remove(struct frame_entry *fe, bool pe){
  
  lock_acquire(&frame_lock);
  
  /**struct list_elem *e = list_front(&frame_table);
  bool exist = false;
  while(e->next!=NULL){
    if(list_entry(e, struct frame_entry, elem)!=fe && list_entry(e, struct frame_entry, elem)->frame==fe->frame){
      exist = true;
      break;
    }
    e = e->next;
  }
  if(!exist){
    palloc_free_page(fe->frame);
  }*/
  if(!fe->in_swap){
    list_remove(&fe->elem);
  }
  //if(!pe){palloc_free_page(fe->frame);}
  //printf("%s %x a\n", thread_current()->name, fe->frame);
  palloc_free_page(fe->frame);
  
  //printf("%s\n", thread_current()->name);
  pagedir_clear_page(fe->t->pagedir, fe->spte->page);
  free(fe);
  lock_release(&frame_lock);
}
  
void * frame_spt_alloc( struct hash * spt, void * page, bool writable, enum palloc_flags flags){
  //lock_acquire(&frame_lock);
  struct spt_entry *spte = malloc(sizeof(struct spt_entry));
  struct frame_entry *fe = malloc(sizeof(struct frame_entry));
   
  uint8_t *frame = palloc_get_page(flags);
  if(frame==NULL){
    frame=frame_evict(flags);
  }
  spte->page = page;
  spte->fe = fe;
  spte->writable = writable;
  spte->flags = flags;
  hash_insert(spt ,&spte->elem);
  fe->frame = frame;
  fe->in_swap = 0;
  fe->swap_where = -1;
  fe->is_free = 0;
  fe->spte = spte;
  fe->t = thread_current();
  lock_acquire(&frame_lock);
  list_push_back(&frame_table, &fe->elem);
  lock_release(&frame_lock);
  //printf("%x %x\n",fe->frame,spte->page);
  return frame;
}
/**
void * frame_alloc(void * frame){
  struct frame_entry *fe = malloc(sizeof(struct frame_entry));
  
  if(frame==NULL){
    frame=frame_evict();
    
  }
  
  fe->frame = frame;
  fe->in_swap = 0;
  fe->is_free = 0;
  list_push_front(&frame_table, &fe->elem);
  return frame;
}
*/

void* frame_evict(enum palloc_flags flags){
  lock_acquire(&frame_lock);
  void * ret;
  struct list_elem * frame_elem = list_front(&frame_table);
  struct frame_entry * fe;
  fe = list_entry(frame_elem, struct frame_entry, elem);
  while(fe->frame == NULL
        || pagedir_is_accessed(fe->t->pagedir ,fe->spte->page)){
    //if(list_entry(frame_elem, struct frame_entry, elem)->spte->writable){
      
      if(fe->frame != NULL){
        pagedir_set_accessed(fe->t->pagedir ,fe->spte->page, false);
      }
      frame_elem = frame_elem->next;
      if(frame_elem->next==NULL){
        frame_elem = list_front(&frame_table);
      }
      fe = list_entry(frame_elem, struct frame_entry, elem);
    //}
  }
  ret = swap_out(fe, flags);
  lock_release(&frame_lock);
  return ret;
}
