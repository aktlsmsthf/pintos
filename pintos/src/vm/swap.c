#include "vm/swap.h"
#include "devices/disk.h"
#include <bitmap.h>
#include "threads/palloc.h"
#include "vm/frame.h"


void swap_init(void){
  swap_table = bitmap_create(11);
  swap_disk = disk_get(1,1);
}

size_t swap_out(void *frame){
  size_t index = bitmap_scan_and_flip(swap_table, 0, 1, 0);
  size_t i;
  for(i=0;i++;i<8){
    printf("1\n");
    disk_write(swap_disk, index+i, (uint8_t *)frame+512*i);
  }
  return index;
}

void swap_in(struct frame_entry *fe,void * frame){
  lock_acquire(&frame_lock);
  size_t i=0;
  size_t index = fe->swap_where;
  for(;i++;i<8){
    disk_read(swap_disk, index+i , (uint8_t *) frame+512*i);
  }
  bitmap_flip(swap_table, index);
  fe->in_swap = 0;
  fe->swap_where = -1;
  fe->frame = frame;
  lock_release(&frame_lock);
}

