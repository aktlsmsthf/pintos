#include <hash.h>

struct spte_entry{
  void* page;
  bool is_swap;
  size_t index;
  struct hash_elem elem;
};

void spt_init(struct hash *spt);
void spt_alloc(struct hash *spt, void* page);
