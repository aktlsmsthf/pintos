#ifndef SWAP_H
#define SWAP_H
#include <bitmap.h>


struct bitmap * swap_table = bitmap_create(10);

struct disk * swap_disk = disk_get(1,1);

int swap_out(void *frame);

void swap_in(int index);

#endif
