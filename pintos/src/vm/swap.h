#ifndef SWAP_H
#define SWAP_H
#include <bitmap.h>

struct bitmap swap_table;

struct disk swap_disk;
void swap_table_init(void);

int swap_out(void *frame);

void swap_in(int index);

#endif
