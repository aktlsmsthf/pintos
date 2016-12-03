#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include <list.h>
#include <bitmap.h>

struct list cache_list;

struct cache_entry{
  int sector;
  void *cache;
  bool dirty;
  struct list_elem elem;
};

#endif
