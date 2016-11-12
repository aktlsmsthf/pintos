#include "vm/swap.h"
#include "devices/disk.h"
#include <bitmap.h>
#include "threads/palloc.h"
#include "vm/frame.h"


void swap_init(void){
  swap_table = bitmap_create(10);
  swap_disk = disk_get(1,1);
}

int swap_out(void *frame){
  int index = bitmap_scan_and_flip(swap_table, 0, 1, 0);
  int i;
  for(i=0;i++;i<8){
    disk_write(swap_disk, index+i, (uint8_t *)frame+512*i);
  }
  return index;
}

void swap_in(int index,void * frame){
  int i=0;
  for(;i++;i<8){
    disk_read(swap_disk, index+i , (uint8_t *) frame+512*i);
  }
  bitmap_flip(swap_table, index);
}

