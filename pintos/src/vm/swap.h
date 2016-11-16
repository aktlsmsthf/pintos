#ifndef SWAP_H
#define SWAP_H
#include <bitmap.h>
#include "devices/disk.h"
#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/palloc.h"

struct bitmap *swap_table ;
struct disk *swap_disk ;
struct lock swap_lock;

void swap_init(void);
void* swap_out(struct frame_entry *fe, enum palloc_flags flags);
void swap_remove(size_t index);
void swap_in(struct frame_entry *fe, enum palloc_flags flags);
#endif
