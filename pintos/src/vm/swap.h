#ifndef SWAP_H
#define SWAP_H
#include <bitmap.h>
#include "devices/disk.h"


static struct bitmap *swap_table ;

static struct disk *swap_disk ;

void swap_init(void);
int swap_out(void *frame);

void swap_in(int index);
#endif
