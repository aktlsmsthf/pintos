#include "vm/swap.h"
#include "devices/disk.h"
#include <bitmap.h>
#include "threads/palloc.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "filesys/cache.h"

int spp = PGSIZE/DISK_SECTOR_SIZE;

void swap_init(void){
  swap_disk = disk_get(1,1);
  swap_table = bitmap_create(disk_size(swap_disk)/spp);
  lock_init(&swap_lock);
}
void swap_remove(size_t index){
  lock_acquire(&swap_lock);
  if(index!=-1){
    bitmap_flip(swap_table, index);
  }
  lock_release(&swap_lock);
}
  
void* swap_out(struct frame_entry *fe, enum palloc_flags flags){
  void* ret;

  lock_acquire(&swap_lock);
  size_t index = bitmap_scan_and_flip(swap_table, 0, 1, 0);
  
  int i;
  for(i=0;i<spp;i++){
    disk_write(swap_disk, index*spp+i, (uint8_t *)fe->frame+DISK_SECTOR_SIZE*i);
  }
  
  fe->swap_where = index;
  fe->in_swap = 1;

  palloc_free_page(fe->frame);
  ret = palloc_get_page(flags);
  pagedir_clear_page(fe->spte->t->pagedir, fe->spte->page);
  fe->frame = NULL;
  lock_release(&swap_lock);
  return ret;
}

void swap_in(struct frame_entry *fe, enum palloc_flags flags){
  
  void *frame = palloc_get_page(flags);
  while(frame == NULL){ frame = frame_evict(flags);}
  int i;
  size_t index = fe->swap_where;
  
  
  
  lock_acquire(&swap_lock);
  if (bitmap_test(swap_table, index) == 0){ return;}
  bitmap_flip(swap_table, index);
  
  for(i=0;i<spp;i++){
    disk_read(swap_disk, index*spp+i , (uint8_t *) frame + DISK_SECTOR_SIZE*i);
  }
  
  lock_release(&swap_lock);
  fe->in_swap = 0;
  fe->swap_where = -1;
  fe->frame = frame;
  install_page(fe->spte->page, frame, fe->spte->writable);
  lock_acquire(&frame_lock);
  list_push_back(&frame_table, &fe->elem);
  lock_release(&frame_lock);
  
}
