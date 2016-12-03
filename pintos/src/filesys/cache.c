#include "filesys/cache.h"
#include <list.h>

void cache_init(void){
  list_init(&cache_list);
}

struct cache_entry find_cache_by_sector(int sector_idx){
  struct list_elem *elem = list_front(&cache_list);
  
  while(elem->next = NULL){
    if(list_entry(elem, &cache_list, elem)->sector == sector_idx){
      return list_entry(elem, &cache_list, elem)
    }
  }
  return NULL;
}

void read_to_cache(int sector_idx){
  struct cache_entry *c = malloc(sizeof (struct cache_entry));
  c->sector = sector_idx;
  c->cache  = malloc(DISK_SECTOR_SIZE);
  c->dirty = 0;
  
  disk_read(filesys_disk, sector_idx, c->cache);
  list_push_front(&cache_list, &c->elem);
}

void read_ahead(int sector_idx){
  if(find_cache_by_sector(sector_idx)){
    read_to_cache(sector_idx);
  }
  if(find_cache_by_sector(sector_idx+1){
    read_to_cache(sector_idx+1);
  }
}

void write_behind(int sector_idx){
    struct cache_entry *c = find_cache_by_sector(sector_idx);
    if(c->dirty){
      disk_write(filesys_disk, sector_idx, c->cache);
    }
}
