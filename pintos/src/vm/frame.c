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
void * frame_spt_alloc(void * frame, struct hash * spt, void * page){
  struct spt_entry *spte = malloc(sizeof(struct spt_entry));
  struct frame_entry *fe = malloc(sizeof(struct frame_entry));
    
  if(frame==NULL){
    frame=frame_evict();
      
  }  
  spte->page = page;
  spte->fe = fe;
  hash_insert(spt ,&spte->elem);
  fe->frame = frame;
  fe->in_swap = 0;
  fe->is_free = 0;
  fe->spte = spte;
  list_push_front(&frame_table, &fe->elem);
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
  void * ret;
  struct list_elem * frame_elem = list_front(&frame_table);
  struct frame_entry * fe;
  while(!list_entry(frame_elem, struct frame_entry, elem)->is_free){
      if(pagedir_is_accessed(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page)){
        pagedir_set_accessed(thread_current()->pagedir ,list_entry(frame_elem, struct frame_entry, elem)->spte->page, false);
      }
      else{
        break;
      }
      frame_elem = frame_elem->next;
      if(frame_elem->next==NULL){
         frame_elem = list_front(&frame_table);
      }
  }
  fe = list_entry(frame_elem, struct frame_entry, elem);
  fe->in_swap = 1;
  fe->swap_where = swap_out(fe->frame);
  pagedir_clear_page(thread_current()->pagedir, fe->spte->page);
  ret=fe->frame;
  fe->frame = NULL;
  printf("%x", ret);
  return ret;  
}
