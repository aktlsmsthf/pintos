#include "vm/frame.h"
#include <list.h>
#include "threads/synch.h"

void frame_init(void){
  list_init(frame_table);
  lock_init(frame_lock);
}

void frame_push(void *frame){
  struct frame_elem f;
  f->frame = frame;
  list_push_front(&frame_table, &f->elem);
}

void frame_pop(void){
  
}
