#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include <list.h>

struct list cache_list;
struct cache_entry{
  int index;
  void *cache;
  bool dirty;
};

#endif
