#include "filesys/cache.h"
#include <list.h>

void cache_init(void){
  list_init(&cache_list);
}

bool exist_cache(int sector_idx){
  struct list_elem *elem = list_front(&cache_list);
  bool exist = false;
  while(elem->next = NULL){
    if(list_entry(elem, &cache_list, elem)->sector == sector_idx){
      exist = true;
    }
  }
  return exist;
}

void read_ahead(int sector_idx){
  
}

void write_behind(void){
}
