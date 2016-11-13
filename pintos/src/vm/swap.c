#include "vm/swap.h"
#include "devices/disk.h"
#include <bitmap.h>
#include "threads/palloc.h"
#include "vm/frame.h"
#include "threads/thread.h"
#include "threads/vaddr.h"


void swap_init(void){
  swap_disk = disk_get(1,1);
  swap_table = bitmap_create(disk_size(swap_disk)/8);
  
}
void swap_remove(size_t index){  
  bitmap_flip(swap_table, index);
}
  
size_t swap_out(void *frame){
  size_t index = bitmap_scan_and_flip(swap_table, 0, 1, 0);
  size_t i=0;
  for(;i<8;i++){
    disk_write(swap_disk, index*8+i, (uint8_t *)frame+512*i);
  }
  return index;
}

void swap_in(struct spt_entry *spte, void * fault_addr){
  lock_acquire(&frame_lock);
  
  uint8_t *frame = palloc_get_page(PAL_USER);
  if(frame == NULL)
    frame = frame_evict();
  
  size_t i=0;
  size_t index = spte->fe->swap_where;
  for(;i<8;i++){
    disk_read(swap_disk, index*8+i , (uint8_t *) frame+512*i);
  }
  bitmap_flip(swap_table, index);
  spte->fe->in_swap = 0;
  spte->fe->swap_where = -1;
  spte->fe->frame = frame;
  
  pagedir_set_page (thread_current()->pagedir, pg_round_down(fault_addr), frame, spte->writable);
  install_page(spte->page, frame, spte->writable);
  
  lock_release(&frame_lock);
}

