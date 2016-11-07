#include <list.h>

struct list frame_table;

struct frame_elem{
  void *frame;
  struct list_elem elem;
}
