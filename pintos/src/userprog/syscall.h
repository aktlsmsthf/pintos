#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <list.h>
#include "filesys/file.h"
#include "filesys/filesys.h"

struct lock sys_lock;

void syscall_init (void);
struct thread* get_child_process(int tid);

struct file_fd{
  int fd;
  struct file *file;
  struct list_elem elem;
  bool is_closed;
};

struct mmaped{
  int mid;
  struct file *file;
  void *addr;
  uint32_t size;
  struct list_elem elem;
}

bool check_valid(void* esp, void* addr);
bool check_valid_buffer(void* esp, void* addr, unsigned size);




#endif /* userprog/syscall.h */
