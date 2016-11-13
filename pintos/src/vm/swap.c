#include "vm/swap.h"
#include "devices/disk.h"
#include <bitmap.h>
#include "threads/palloc.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

int spp = PGSIZE/DISK_SECTOR_SIZE;

void swap_init(void){
  swap_disk = disk_get(1,1);
  swap_table = bitmap_create(disk_size(swap_disk)/spp);
  lock_init(swap_lock);
}
void swap_remove(size_t index){
  lock_acquire(&swap_lock);
  if(index!=-1){
    bitmap_flip(swap_table, index);
  }
  lock_release(&swap_lock);
}
  
size_t swap_out(void *frame){
  lock_acquire(&swap_lock);
  size_t index = bitmap_scan_and_flip(swap_table, 0, 1, 0);
  int i;
  for(i=0;i<spp;i++){
    disk_write(swap_disk, index*spp+i, (uint8_t *)frame+512*i);
  }
  lock_release(&swap_lock);
  return index;
}

void swap_in(struct frame_entry *fe, void * frame){
  lock_acquire(&swap_lock);
  int i;
  size_t index = fe->swap_where;
  for(i=0;i<spp;i++){
    disk_read(swap_disk, index*spp+i , (uint8_t *) frame+512*i);
  }
  bitmap_flip(swap_table, index);
  fe->in_swap = 0;
  fe->swap_where = -1;
  fe->frame = frame;
  lock_release(&swap_lock);
}

