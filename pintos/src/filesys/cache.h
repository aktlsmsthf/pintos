#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include <list.h>
#include <bitmap.h>

struct list cache_list;
struct bitmap *cache_bitmap;
struct cache_entry{
  int index;
  void *cache;
  bool dirty;
};

#endif
