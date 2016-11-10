#include <hash.h>

struct spt_entry111{
  void *page;
  bool is_swap;
  size_t index;
  struct hash_elem elem;
};

void spt_init(struct hash *spt);
void spt_alloc(struct hash *spt, void* page);
