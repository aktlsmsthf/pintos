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
  /*
  struct list_elem *e = list_front(&frame_table);
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
  list_remove(&fe->elem);
  //if(!pe){palloc_free_page(fe->frame);}
  palloc_free_page(fe->frame);
  //printf("%s\n", thread_current()->name);
  pagedir_clear_page(thread_current()->pagedir, fe->spte->page);
  free(fe);
  lock_release(&frame_lock);
}
  
void * frame_spt_alloc(void * frame, struct hash * spt, void * page, bool writable, enum palloc_flags flags){
  //lock_acquire(&frame_lock);
  //uint8_t *frame = palloc_get_page(PAL_USER);
  //lock_release(&frame_lock);
  struct spt_entry *spte = malloc(sizeof(struct spt_entry));
  struct frame_entry *fe = malloc(sizeof(struct frame_entry));
    
  if(frame==NULL){
    frame=frame_evict();
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

void* frame_evict(void){
  lock_acquire(&frame_lock);
  void * ret;
  struct list_elem * frame_elem = list_front(&frame_table);
  struct frame_entry * fe;
  while(list_entry(frame_elem, struct frame_entry, elem)->frame == NULL
        || list_entry(frame_elem, struct frame_entry, elem)->spte->flags ==6
        || pagedir_is_accessed(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page)){
    //if(list_entry(frame_elem, struct frame_entry, elem)->spte->writable){
      
      if(list_entry(frame_elem, struct frame_entry, elem)->frame != NULL){
        pagedir_set_accessed(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page, false);
      }
      frame_elem = frame_elem->next;
      if(frame_elem->next==NULL){
        frame_elem = list_front(&frame_table);
      }
    //}
  }
  /*
  while(true){
    if(list_entry(frame_elem, struct frame_entry, elem)->frame != NULL
       &&!pagedir_is_accessed(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page)){
          break;
        }
    else{
      pagedir_set_accessed(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page, false);
      frame_elem = frame_elem->next;
      if(frame_elem->next==NULL){
        frame_elem = list_front(&frame_table);
      }
    }
  }*/
  /**while(true){
    if(list_entry(frame_elem, struct frame_entry, elem)->frame != NULL){
      if(pagedir_is_accessed(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page)){
        pagedir_set_accessed(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page, false);
      }
      else{
        if(pagedir_is_dirty(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page)){
          break;
        }
      }
    }
    frame_elem = frame_elem->next;
    if(frame_elem->next==NULL){
        frame_elem = list_front(&frame_table);
    }
  }**/
  /**while(true){
      if(!list_entry(frame_elem, struct frame_entry, elem)->in_swap){
        if(pagedir_is_accessed(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page)){
          pagedir_set_accessed(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page, false);
        }
        else{
          break;
        }
      }
      frame_elem = frame_elem->next;
      if(frame_elem->next==NULL){
         frame_elem = list_front(&frame_table);
      }
  }**/
  fe = list_entry(frame_elem, struct frame_entry, elem);
  /**fe->swap_where = swap_out(fe->frame);
  fe->in_swap = 1;**/
  
  //ret=fe->frame;
  //fe->frame = NULL;
  /**palloc_free_page(fe->frame);
  pagedir_clear_page(thread_current()->pagedir, fe->spte->page);
  fe->frame = NULL;
  ret = palloc_get_page(PAL_USER);**/
  
  ret = swap_out(fe);
  lock_release(&frame_lock);
  return ret;
}
