#include "filesys/cache.h"
#include <list.h>
#include "filesys/filesys.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/disk.h"
#include "threads/malloc.h"

void cache_init(void){
  list_init(&cache_list);
  lock_init(&cache_lock);
  //thread_create("Write_behind_periodically", 0, thread_func_write_behind, NULL);
  count = 0;
}

struct cache_entry * find_cache_by_sector(int sector_idx){
  if(list_empty(&cache_list)){
    return NULL;
  }
  lock_acquire(&cache_lock);
  struct list_elem *elem = list_front(&cache_list);
  while(elem->next != NULL){
    if(list_entry(elem, struct cache_entry, elem)->sector == sector_idx){
      lock_release(&cache_lock);
      return list_entry(elem, struct cache_entry, elem);
    }
    elem = elem->next;
  }
  lock_release(&cache_lock);
  return NULL;
}

struct cache_entry * read_to_cache(int sector_idx, bool first){
  if(find_cache_by_sector(sector_idx)){
    return find_cache_by_sector(sector_idx);
  }
  
  
  if(count==64){
    lock_acquire(&cache_lock);
    printf("a\n");
    struct cache_entry *c = list_entry(hand, struct cache_entry, elem);
    while(c->accessed){
      c->accessed = false;
      hand = hand->next;
      if(hand->next == NULL){
        hand = list_front(&cache_list);
      }
    }
    hand = hand->next;
    write_behind(c);
    lock_release(&cache_lock);
  }
  lock_acquire(&cache_lock);
  struct cache_entry *c = malloc(sizeof (struct cache_entry));
  c->sector = sector_idx;
  c->cache = malloc(DISK_SECTOR_SIZE);
  while(c->cache==NULL){
    c->cache = malloc(DISK_SECTOR_SIZE);
  }
  c->dirty = false;
  c->accessed = true;
  
  disk_read(filesys_disk, sector_idx, c->cache);
  list_push_back(&cache_list, &c->elem);
  count++;
  printf("%d\n", count);
  if(count ==1){
    hand = &c->elem;
  }
  lock_release(&cache_lock);
  
  /**if(first){
    void *aux = sector_idx+1;
    thread_create("Read_ahead", 0, thread_func_read_ahead, aux);
  }**/
  
  return c;
}

void thread_func_read_ahead(void *aux){
  int idx = *(int*)aux;
  read_to_cache(idx, false);
}

void write_to_cache(int sector_idx, void *buffer){
  struct cache_entry *c = find_cache_by_sector(sector_idx);
  memcpy(c->cache, buffer, DISK_SECTOR_SIZE);
  c->dirty = true;
}

void write_behind(struct cache_entry *c){
    if(c->dirty){
      printf("1\n");
      disk_write(filesys_disk, c->sector, c->cache);
      list_remove(&c->elem);
      count--;
      free(c->cache);
      free(c);
    }
}

void write_behind_all(void){
  if(list_empty(&cache_list)){
    return;
  }
  struct list_elem *elem = list_front(&cache_list);
  struct cache_entry *c;
  lock_acquire(&cache_lock);
  while(elem->next != NULL){
    c = list_entry(elem, struct cache_entry, elem);
    elem = elem->next;
    write_behind(c);
  }
  lock_release(&cache_lock);
}

void thread_func_write_behind(void *aux){
  while(true){
    timer_sleep(5);
    write_behind_all();
  }
}
