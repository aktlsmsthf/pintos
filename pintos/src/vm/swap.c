#include "vm/swap.h"
#include "devices/disk.h"
#include <bitmap.h>
#include "threads/palloc.h"
#include "vm/frame.h"
#include "threads/vaddr.h"

int spp = PGSIZE/DISK_SECTOR_SIZE

void swap_init(void){
  swap_disk = disk_get(1,1);
  swap_table = bitmap_create(disk_size(swap_disk)/spp);
  
}
void swap_remove(size_t index){  
  bitmap_flip(swap_table, index);
}
  
size_t swap_out(void *frame){
  size_t index = bitmap_scan_and_flip(swap_table, 0, 1, 0);
  size_t i=0;
  for(;i<spp;i++){
    disk_write(swap_disk, index*spp+i, (uint8_t *)frame+512*i);
  }
  return index;
}

void swap_in(struct frame_entry *fe, void * frame){
  lock_acquire(&frame_lock);
  size_t i=0;
  size_t index = fe->swap_where;
  for(;i<spp;i++){
    disk_read(swap_disk, index*spp+i , (uint8_t *) frame+512*i);
  }
  bitmap_flip(swap_table, index);
  fe->in_swap = 0;
  fe->swap_where = -1;
  fe->frame = frame;
  lock_release(&frame_lock);
}

