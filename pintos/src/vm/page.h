#include <hash.h>

struct hash supple_page_table;

struct spt_entry{
  void* page;
  bool is_swap;
  size_t index;
  struct hash_elem elem;
}
