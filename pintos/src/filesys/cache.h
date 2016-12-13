#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include <list.h>
#include <bitmap.h>
#include "threads/synch.h"
#include "devices/disk.h"

struct list cache_list;
struct lock cache_lock;
struct list_elem *hand;
int count;
int next;
bool ahead;

struct cache_entry{
  int sector;
  void *cache;
  bool dirty;
  bool accessed;
  struct list_elem elem;
};

struct sector_data{
  int next_sector;
};

void cache_init(void);
struct cache_entry * read_to_cache(int sector_idx, bool first);
struct cache_entry * find_cache_by_sector(int sector_idx);
void write_behind(struct cache_entry *c);
void write_behind_all(void);
void thread_func_write_behind(void *aux);
void thread_func_read_ahead(void *aux);
void thread_fun_read(void *aux);

#endif
