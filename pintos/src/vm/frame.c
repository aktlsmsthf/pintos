#include "vm/frame.h"
#include <list.h>
#include "threads/synch.h"
#include "threads/palloc.h"

void frame_init(void){
  list_init(&frame_table);
  lock_init(&frame_lock);
}

void frame_alloc(void* frame){
  struct frame_entry *f = malloc(sizeof(struct frame_entry));
  f->frame = frame;
  list_push_front(&frame_table, &f->elem);
}

struct frame_entry* frame_pop(void){
  
}

struct frame_entry* frame_evit(void){
  
}
