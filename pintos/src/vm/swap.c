#include "vm/swap.h"
#include <disk.h>
#include <bitmap.h>

void swap_table_init(){
  swap_table = bitmap_create(10);
  swap_disk = disk_get(1,1);
}

int swap_out(void *frame){
  int index = bitmap_scan_and_flip(swap_table, 0, 1, 0);
  int i=0;
  for(;i++;i<8){
    disk_write(swap_disk, index+i, frame+512*i);
  }
  return index;
}
