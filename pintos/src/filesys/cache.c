#include "filesys/cache.h"
#include <list.h>
#include "filesys/filesys.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/disk.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"
#include "filesys/inode.h"

void cache_init(void){
  list_init(&cache_list);
  lock_init(&cache_lock);
  //thread_create("Write_behind_periodically", 0, thread_func_write_behind, NULL);
  
  finish = false;
  ahead = false;
  next = 0;
  count = 0;
  
}

void cache_finish(void){
  finish = true;
}

struct cache_entry * find_cache_by_sector(int sector_idx){
  if(list_empty(&cache_list)){
    //lock_release(&cache_lock);
    return NULL;
  }
  //lock_acquire(&cache_lock);
  struct list_elem *elem = list_front(&cache_list);
  while(elem->next != NULL){
    if(list_entry(elem, struct cache_entry, elem)->sector == sector_idx){
      //lock_release(&cache_lock);
      return list_entry(elem, struct cache_entry, elem);
    }
    elem = elem->next;
  }
  //lock_release(&cache_lock);
  return NULL;
}

struct cache_entry * read_to_cache(int sector_idx, bool first){
  printf("2\n");
  struct cache_entry *c;
  lock_acquire(&cache_lock);
  c = find_cache_by_sector(sector_idx);
  if(c!=NULL){
    c->accessed = true;
    lock_release(&cache_lock);
    return c;
  }
  
  
  //lock_acquire(&cache_lock);
  if(count>=64){
    struct list_elem *elem = list_front(&cache_list);
    c = list_entry(elem, struct cache_entry, elem);
    while(c->accessed){
      c->accessed = false;
      list_remove(elem);
      list_push_back(&cache_list, elem);
      elem = list_front(&cache_list);
      c = list_entry(elem, struct cache_entry, elem);
    }
    
    //lock_acquire(&cache_lock);
    if(c->dirty){
      disk_write(filesys_disk, c->sector, c->cache);
    }
    count--;
    free(c->cache);
    
  }
  else{
    //lock_acquire(&cache_lock);
    c = malloc(sizeof (struct cache_entry));
    list_push_back(&cache_list, &c->elem);
  }
  c->sector = sector_idx;
  c->cache = malloc(DISK_SECTOR_SIZE);
  c->dirty = false;
  c->accessed = true;
  
  
  count++;
  //printf("%d\n", count);
  //hand = &c->elem;
  
  disk_read(filesys_disk, sector_idx, c->cache);
  
  
  /**int aux;
  if(first){
    aux = sector_idx+1;
    printf("%d\n", aux);
    thread_create("Read_ahead", 0, thread_func_read_ahead, &aux);
  }**/
  if(first){
    next = sector_idx+1;
    thread_create("read_ahead", 0, thread_func_read_ahead, NULL);
  }
  lock_release(&cache_lock);
  
  return c;
}

void thread_func_read_ahead(void *aux){
  printf("1\n");
  read_to_cache(next, false);
}

void write_to_cache(int sector_idx, void *buffer){
  struct cache_entry *c = find_cache_by_sector(sector_idx);
  memcpy(c->cache, buffer, DISK_SECTOR_SIZE);
  c->dirty = true;
}

void write_behind(struct cache_entry *c){
    lock_acquire(&inode_lock);
    if(c->dirty){
      disk_write(filesys_disk, c->sector, c->cache);
    }
    list_remove(&c->elem);
    count--;
    free(c->cache);
    free(c);
    lock_release(&inode_lock);
}

void write_behind_all(void){
  if(list_empty(&cache_list)){
    return;
  }
  struct list_elem *elem = list_front(&cache_list);
  struct cache_entry *c;
  //lock_acquire(&cache_lock);
  
  while(elem->next != NULL){
    c = list_entry(elem, struct cache_entry, elem);
    elem = elem->next;
    //lock_acquire(&inode_lock);
    write_behind(c);
    //lock_release(&inode_lock);
  }
  
  //lock_release(&cache_lock);
}

void thread_func_write_behind(void *aux){
  while(true){
    timer_sleep(5);
    write_behind_all();
  }
}
