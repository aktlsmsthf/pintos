#include <hash.h>


struct spt_entry{
  void* page;
  bool is_swap;
  size_t index;
  struct hash_elem elem;
};

void spt_init(void);
void spt_alloc(void* page);
