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
  
void* swap_out(struct frame_entry *fe){
  ASSERT(fe->frame!=NULL);
  void* ret;
  lock_acquire(&swap_lock);
  size_t index = bitmap_scan_and_flip(swap_table, 0, 1, 0);
  //printf("%d\n", index);
  
  int i;
  for(i=0;i<spp;i++){
    disk_write(swap_disk, index*spp+i, (uint8_t *)fe->frame+DISK_SECTOR_SIZE*i);
  }
  fe->swap_where = index;
  fe->in_swap = 1;
  //printf("b %x\n", fe->frame);
  palloc_free_page(fe->frame);
  pagedir_clear_page(thread_current()->pagedir, fe->spte->page);
  //ret=fe->frame;
  //fe->frame = NULL;
  //printf("%x\n", ret);
  ret = palloc_get_page(PAL_USER);
  //printf("a %x\n", ret);
  //printf("%d\n", bitmap_count(swap_table, 0, disk_size(swap_disk)/spp, 1));
  lock_release(&swap_lock);
  return ret;
}

void swap_in(struct frame_entry *fe, void * frame){
  lock_acquire(&swap_lock);
  int i;
  size_t index = fe->swap_where;
  //printf("%d\n", -1*index);
  for(i=0;i<spp;i++){
    disk_read(swap_disk, index*spp+i , (uint8_t *) frame+DISK_SECTOR_SIZE*i);
  }
  bitmap_flip(swap_table, index);
  fe->in_swap = 0;
  fe->swap_where = -1;
  fe->frame = frame;
  install_page(fe->spte->page, frame, fe->spte->writable);
  lock_release(&swap_lock);
}

