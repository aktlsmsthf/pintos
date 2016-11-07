#include <list.h>

struct list frame_table;

struct frame_elem{
  void *frame;
  struct list_elem elem;
}

void frame_init(void);
void frame_push(void *frame);
struct fraem_elem* frame_pop(void);
