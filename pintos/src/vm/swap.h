#ifndef SWAP_H
#define SWAP_H
#include <bitmap.h>
#include "devices/disk.h"
#include "vm/page.h"


struct bitmap *swap_table ;
struct disk *swap_disk ;

void swap_init(void);
size_t swap_out(void *frame);
void swap_remove(size_t index);
void swap_in(struct spt_entry *spte,void *fault_addr);
#endif
