#include "filesys/cache.h"
#include <list.h>
#include "filesys/filesys.h"
#include "threads/synch.h"
#include "threads/thread.h"

void cache_init(void){
  list_init(&cache_list);
  lock_init(&cache_lock);
  thread_create("Write_behind_periodically", 0, thread_func_write_behind, NULL);
  count = 0;
}

struct cache_entry find_cache_by_sector(int sector_idx){
  struct list_elem *elem = list_front(&cache_list);
  
  lock_acquire(&cache_lock);
  while(elem->next != NULL){
    if(list_entry(elem, &cache_list, elem)->sector == sector_idx){
      lock_release(&cache_lock);
      return list_entry(elem, &cache_list, elem);
    }
    elem = elem->next;
  }
  lock_release(&cache_lock);
  return NULL;
}

struct cache_entry read_to_cache(int sector_idx, bool first){
  if(find_cache_by_sector(sector_idx)){
    return find_cache_by_sector(sector_idx);
  }
  
  lock_acquire(&cache_lock);
  if(count==64){
    struct cache_entry *c = list_entry(hand, &cache_list, elem);
    while(c->accessed){
      c->accessed = false;
      hand = hand->next;
      if(hand->next == NULL){
        hand = list_front(&cache_list);
      }
    }
    hand = hand->next;
    write_behind(c);
  }
  struct cache_entry *c = malloc(sizeof (struct cache_entry));
  c->sector = sector_idx;
  c->cache  = malloc(DISK_SECTOR_SIZE);
  c->dirty = false;
  c->accessed = true;
  
  disk_read(filesys_disk, sector_idx, c->cache);
  list_push_back(&cache_list, &c->elem);
  count++;
  if(count ==1){
    hand = &c->elem;
  }
  lock_release(&cache_lock);
  
  if(first){
    void *aux = sector_idx+1;
    thread_create("Read_ahead", 0, thread_func_read_ahead, aux);
  }
  
  return c;
}

void thread_func_read_ahead(void *aux){
  int idx = *aux;
  read_to_cache(idx, false);
}

void write_to_cache(int sector_idx, void *buffer){
  struct cache_entry *c = find_cache_by_sector(sector_idx);
  memcpy(c->cache, buffer, DISK_SECTOR_SIZE);
  if(!c->dirty){
    c->dirty = true;
  }
}

void write_behind(struct cache_entry *c){
    if(c->dirty){
      disk_write(filesys_disk, sector_idx, c->cache);
      list_remove(&c->elem);
      count--;
      free(c);
    }
}

void write_behind_all(void){
  struct list_elem *elem = list_front(&cache_list);
  struct cache_entry *c;
  lock_acquire(&cache_lock);
  while(elem->next == NULL){
    c = list_entry(elem, &cache_list, elem)
    write_behind(c);
  }
  lock_release(&cache_lock);
}

void thread_func_write_behind(void *aux){
  while(true){
    timer_sleep(1);
    write_behind_all();
  }
}
