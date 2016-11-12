#include "vm/frame.h"
#include <list.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "vm/swap.h"
#include "threads/malloc.h"

void frame_init(void){
  list_init(&frame_table);
  lock_init(&frame_lock);
  swap_init();
}

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

struct frame_entry* frame_pop(void){
  
}

void* frame_evict(void){
  void * ret;
  struct list_elem * frame_elem = list_front(&frame_table);
  struct frame_entry * fe;
  while(!list_entry(frame_elem, struct frame_entry, elem)->is_free){
          list_entry(frame_elem, struct frame_entry, elem)->is_free=1;
          frame_elem = frame_elem->next;
          if(frame_elem->next==NULL){
             frame_elem = list_front(&frame_table);
          }
      }
  fe = list_entry(frame_elem, struct frame_entry, elem);
  fe->in_swap = 1;
  fe->swap_where = swap_out(fe->frame);
  ret=fe->frame;
  fe->frame = NULL;
  return ret;  
}
