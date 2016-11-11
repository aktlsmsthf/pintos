#include "vm/frame.h"
#include <list.h>
#include "threads/synch.h"
#include "threads/palloc.h"

void frame_init(void){
  list_init(&frame_table);
  lock_init(&frame_lock);
}

void frame_alloc(void* frame, struct spt_entry * spte){
  if(frame==NULL){
    frame=frame_evict();
  }
  struct frame_entry *f = malloc(sizeof(struct frame_entry));
  f->frame = frame;
  f->in_swap = 0;
  f->is_free = 0;
  f->spte = spte;
  list_push_front(&frame_table, &f->elem);
}

struct frame_entry* frame_pop(void){
  
}

void* frame_evict(void){
  void * ret;
  struct list_elem * frame_elem = list_front(frame_table);
  struct frame_entry * fe;
  while(!list_entry(frame_elem, struct frame_entry, elem)->is_free){
          frame_elem = frame_elem->next;
          if(felem->next==NULL){
             frame_elem = list_front(frame_table);
             break;
          }
      }
  fe = list_entry(frame_elem, struct frame_entry, elem);
  fe->in_swap = 1;
  fe->swap_where = swap_out(fe->frame);
  ret=fe->frame;
  fe->frame = NULL;
  return ret;  
}
