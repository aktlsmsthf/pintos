#include <hash.h>

struct spt_entry{
  void *page;
  struct hash_elem elem;
}

void spt_init(struct hash *spt);
void spt_alloc(struct hash *spt, void *page);
