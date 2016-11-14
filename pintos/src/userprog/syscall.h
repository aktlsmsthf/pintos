#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <list.h>
#include "filesys/file.h"
#include "filesys/filesys.h"

void syscall_init (void);
struct thread* get_child_process(int tid);

struct file_fd{
  int fd;
  struct file *file;
  struct list_elem elem;
  bool is_closed;
};

bool check_valid(void*, void*);
bool check_valid_buffer(void*, void*, unsigned);




#endif /* userprog/syscall.h */
