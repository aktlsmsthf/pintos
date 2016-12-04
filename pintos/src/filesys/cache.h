#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include <list.h>
#include <bitmap.h>

struct list cache_list;
struct lock cache_lock;
struct list_elem *hand;
int count;

struct cache_entry{
  int sector;
  void *cache;
  bool dirty;
  bool accessed;
  struct list_elem elem;
};

#endif
